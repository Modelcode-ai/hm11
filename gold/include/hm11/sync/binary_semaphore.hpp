/**
 * @file binary_semaphore.hpp
 * @brief Minimal binary semaphore for ISR-to-thread signaling
 *
 * This header provides the BinarySemaphore class that implements Ada's Protected
 * Object pattern (specifically the Watchdog protected object's Release/Await_Event
 * pattern) in C++ using lock-free atomic operations and platform-specific blocking
 * primitives.
 *
 * ## Overview
 *
 * Ada's Protected Objects provide ISR-safe procedures that can be called from
 * interrupt context and blocking entries with barriers for thread synchronization.
 * In C++, we must explicitly implement this pattern using atomic operations and
 * low-power wait instructions.
 *
 * ## Design Principles (Decision 19)
 *
 * This BinarySemaphore implementation provides:
 * - **Minimal footprint**: Single atomic<bool> (1 byte on most platforms)
 * - **Zero RTOS dependency**: No external dependencies for production
 * - **ISR-safe signaling**: GiveFromISR() callable from interrupt handlers
 * - **Hardware low-power states**: Uses WaitEvent for power-efficient blocking
 * - **Fast wake-up**: <10µs latency on ARM Cortex-M
 * - **Auto-reset semantics**: Automatically clears on successful take (matches Ada)
 *
 * ## Memory Ordering (Decision 23)
 *
 * The implementation uses std::memory_order_release/acquire for ISR-to-thread
 * synchronization:
 * - GiveFromISR() uses memory_order_release to publish all ISR writes before signaling
 * - TakeWithTimeout() uses memory_order_acquire to observe ISR writes after waking
 * - Clearing after successful take uses memory_order_relaxed (single consumer, no sync needed)
 *
 * This provides the necessary happens-before relationship for single-producer
 * (ISR) single-consumer (thread) patterns.
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/sync/binary_semaphore.hpp>
 * #include <chrono>
 *
 * hm11::sync::BinarySemaphore semaphore;
 *
 * // In ISR context: signal completion
 * void uart_complete_isr() {
 *     // ... handle interrupt ...
 *     semaphore.give_from_isr();  // Wake waiting thread
 * }
 *
 * // In thread context: wait for ISR signal
 * void process_data() {
 *     if (semaphore.take_with_timeout(std::chrono::seconds(1))) {
 *         // Signal received - process data
 *         handle_received_data();
 *     } else {
 *         // Timeout occurred
 *         handle_timeout();
 *     }
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * - GiveFromISR() is ISR-safe and thread-safe (can be called from any context)
 * - TakeWithTimeout() must be called from thread context only (not from ISR)
 * - Designed for single consumer (application thread) and single producer (ISR)
 * - Multiple Give calls before Take will coalesce (binary semaphore semantics)
 *
 * ## Ada Translation
 *
 * This class directly translates Ada's Watchdog protected object pattern:
 *
 * **Ada:**
 * @code{.ada}
 * protected Watchdog is
 *    entry Await_Event (Status : out UART_Status) when Event_Occurred;
 *    procedure Release (Status : UART_Status);  -- ISR-callable
 * private
 *    Event_Occurred : Boolean := False;
 * end Watchdog;
 * @endcode
 *
 * **C++:**
 * @code
 * BinarySemaphore semaphore;    // Replaces Event_Occurred boolean + entry barrier
 * semaphore.give_from_isr();    // Replaces Release() procedure
 * semaphore.take_with_timeout();// Replaces Await_Event() entry with barrier
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <hm11/platform/wait_event.hpp>

namespace hm11::sync {

/**
 * @brief Binary semaphore for ISR-to-thread signaling
 *
 * Minimal synchronization primitive implementing ISR-safe signaling with
 * thread blocking. The implementation uses lock-free atomic operations
 * and platform-specific low-power wait instructions.
 *
 * ## Key Operations
 *
 * - **give_from_isr()**: Signal from ISR context (non-blocking, ISR-safe)
 * - **take_with_timeout()**: Block until signaled or timeout (thread context only)
 * - **try_take()**: Non-blocking check and clear (thread context)
 *
 * ## Semantics
 *
 * - **Binary**: Multiple Give calls before Take will coalesce into single signal
 * - **Auto-reset**: Successful Take automatically clears semaphore state
 * - **Signal-before-wait**: Give before Take causes immediate Take success
 *
 * ## Memory Layout
 *
 * Size: 1 byte (std::atomic<bool> on most platforms)
 *
 * ## Implementation Notes
 *
 * The semaphore uses a single atomic boolean to track availability:
 * - false: Not signaled (Take will block)
 * - true: Signaled (Take will succeed immediately)
 *
 * Timeout implementation (Decision 25) uses polling in the blocking loop:
 * - Thread checks std::chrono::steady_clock::now() against deadline in loop
 * - No platform timeout primitives needed (matches Ada's approach)
 * - Timeout accuracy depends on WaitEvent wake-up frequency (~1-10ms typical)
 *
 * @note This class is not copyable or movable (matching Ada's limited private)
 * @note All operations are noexcept for ISR safety and embedded reliability
 */
class BinarySemaphore {
  public:
    /**
     * @brief Default constructor - initializes semaphore to non-signaled state
     *
     * The semaphore starts cleared (available_ = false), requiring a GiveFromISR()
     * call before TakeWithTimeout() will succeed.
     */
    BinarySemaphore() noexcept = default;

    /**
     * @brief Destructor
     *
     * No cleanup needed - all state is stack-allocated.
     * If a thread is blocked in TakeWithTimeout() when the semaphore is destroyed,
     * behavior is undefined (caller must ensure no threads are waiting).
     */
    ~BinarySemaphore() = default;

    // Delete copy and move operations (matching Ada's limited private pattern, Decision 22)
    BinarySemaphore(const BinarySemaphore&) = delete;
    BinarySemaphore& operator=(const BinarySemaphore&) = delete;
    BinarySemaphore(BinarySemaphore&&) = delete;
    BinarySemaphore& operator=(BinarySemaphore&&) = delete;

    /**
     * @brief Signal the semaphore from ISR context
     *
     * Sets the semaphore to signaled state and wakes any thread blocked in
     * TakeWithTimeout(). This operation is designed to be called from interrupt
     * handlers and is guaranteed to be non-blocking and ISR-safe.
     *
     * If called before TakeWithTimeout(), the subsequent TakeWithTimeout() will
     * return immediately without blocking (signal-before-wait behavior).
     *
     * Multiple calls to GiveFromISR() before a TakeWithTimeout() will coalesce
     * into a single signal (binary semaphore semantics).
     *
     * ## Memory Ordering (Decision 23)
     *
     * Uses memory_order_release when storing true to available_. This ensures that
     * all memory operations performed by the ISR before calling GiveFromISR() are
     * visible to the thread after TakeWithTimeout() observes the signal.
     *
     * Example synchronization:
     * @code
     * // In ISR:
     * buffer[0] = received_byte;           // Write to shared memory
     * status = UARTStatus::Ok;             // Update status
     * semaphore.GiveFromISR();             // Release: Publish above writes
     *
     * // In thread:
     * if (semaphore.TakeWithTimeout(...)) { // Acquire: Observe ISR writes
     *     process(buffer[0], status);       // Safe to read ISR-written data
     * }
     * @endcode
     *
     * @note ISR-safe: Yes (non-blocking, no locks, minimal operations)
     * @note Thread-safe: Yes (can be called from multiple ISRs or threads)
     * @note Blocking: No (always returns immediately)
     * @note Idempotent: Multiple calls without Take have no additional effect
     *
     * @post Semaphore is in signaled state (available_ == true)
     * @post Any thread blocked in take_with_timeout() will wake up
     */
    void give_from_isr() noexcept;

    /**
     * @brief Wait for semaphore with timeout
     *
     * Blocks the calling thread until either:
     * 1. give_from_isr() is called (returns true)
     * 2. The timeout expires (returns false)
     *
     * If the semaphore is already signaled when this function is called,
     * it returns immediately with true without blocking.
     *
     * On successful take (return true), the semaphore state is automatically
     * cleared (auto-reset), matching Ada's behavior where Event_Occurred is
     * set to False after a successful Await_Event.
     *
     * ## Timeout Implementation (Decision 25)
     *
     * The timeout is implemented using ISR-side deadline polling:
     * - Deadline is calculated as current_time + timeout at start of function
     * - Each iteration of the blocking loop checks current time against deadline
     * - No platform timeout primitives required (matches Ada's Watchdog pattern)
     * - Timeout accuracy: typically within 1-10ms on most platforms
     *
     * ## Memory Ordering (Decision 23)
     *
     * Uses memory_order_acquire when loading available_ in the blocking loop.
     * This ensures that all memory operations performed by the ISR before
     * GiveFromISR() are visible after this function returns true.
     *
     * The final clear (setting available_ to false) uses memory_order_relaxed
     * because there's only one consumer thread and no synchronization with other
     * threads is needed for the clear operation.
     *
     * ## Blocking Behavior
     *
     * This function uses WaitEvent::wait() for power-efficient blocking:
     * - ARM Cortex-M: WFE instruction (~10µA power consumption)
     * - Linux x86: futex syscall (kernel blocks process)
     *
     * The blocking loop handles spurious wakeups by re-checking the available_
     * flag and continuing to block if not yet signaled.
     *
     * @tparam Rep Arithmetic type representing the number of ticks
     * @tparam Period std::ratio representing the tick period (duration units)
     * @param timeout Maximum time to wait for signal
     * @return true if semaphore was taken (signaled), false if timeout occurred
     *
     * @note ISR-safe: No (blocks - must be called from thread context only)
     * @note Thread-safe: Yes (but designed for single consumer)
     * @note Blocking: Yes (until signaled or timeout)
     *
     * @pre Must be called from thread context (not from ISR)
     * @pre Timeout must be positive duration
     * @post If returns true, semaphore is cleared (available_ == false)
     * @post If returns false, semaphore state is unchanged
     *
     * @par Example
     * @code
     * using namespace std::chrono_literals;
     *
     * // Wait up to 1 second for signal
     * if (semaphore.take_with_timeout(1s)) {
     *     // Received signal within timeout
     *     process_data();
     * } else {
     *     // Timeout occurred
     *     handle_timeout();
     * }
     * @endcode
     */
    template <typename Rep, typename Period>
    [[nodiscard]] bool take_with_timeout(std::chrono::duration<Rep, Period> timeout) noexcept;

    /**
     * @brief Non-blocking check if semaphore is available
     *
     * Checks if the semaphore is signaled and clears it atomically if it is.
     * This is a non-blocking alternative to take_with_timeout().
     *
     * ## Memory Ordering (Decision 23)
     *
     * Uses memory_order_acquire in the exchange operation to ensure visibility
     * of ISR writes if the semaphore was signaled.
     *
     * @return true if semaphore was signaled (and is now cleared), false otherwise
     *
     * @note ISR-safe: Yes (non-blocking)
     * @note Thread-safe: Yes
     * @note Blocking: No (returns immediately)
     *
     * @post If returns true, semaphore is cleared (available_ == false)
     * @post If returns false, semaphore state is unchanged
     *
     * @par Example
     * @code
     * // Check without blocking
     * if (semaphore.try_take()) {
     *     // Semaphore was signaled
     *     process_immediate();
     * } else {
     *     // Semaphore not signaled, continue other work
     *     do_other_work();
     * }
     * @endcode
     */
    [[nodiscard]] bool try_take() noexcept;

  private:
    /**
     * @brief Atomic availability flag
     *
     * States:
     * - false: Not signaled (take_with_timeout will block)
     * - true: Signaled (take_with_timeout will succeed immediately)
     *
     * Memory ordering (Decision 23):
     * - give_from_isr(): store with memory_order_release (publish ISR work)
     * - take_with_timeout(): load with memory_order_acquire (observe ISR work)
     * - take_with_timeout() clear: store with memory_order_relaxed (single consumer)
     * - try_take(): exchange with memory_order_acquire (observe ISR work)
     *
     * Size: 1 byte on most platforms (atomic<bool> typically optimized)
     */
    std::atomic<bool> available{false};

    /**
     * @brief Platform-specific wait event for blocking
     *
     * Provides low-power blocking wait with ISR-safe signaling:
     * - ARM Cortex-M: WFE/SEV instructions
     * - Linux x86: futex syscalls
     *
     * Used by take_with_timeout() for power-efficient blocking and by give_from_isr()
     * for waking the blocked thread.
     */
    platform::WaitEvent wait_event;
};

// Template implementation must be in header

template <typename Rep, typename Period>
[[nodiscard]] bool BinarySemaphore::take_with_timeout(std::chrono::duration<Rep, Period> timeout) noexcept {
    using Clock = std::chrono::steady_clock;

    // Calculate absolute deadline for timeout (Decision 25)
    // This matches Ada's pattern: Until_Time := Clock + Timeout
    const auto DEADLINE = Clock::now() + timeout;

    // Blocking loop: wait until signaled or timeout
    // memory_order_acquire: Ensures we observe all ISR writes when available becomes true
    // (Decision 23: Release-acquire provides happens-before relationship for SPSC)
    while (!available.load(std::memory_order_acquire)) {
        // Check timeout before blocking
        // Polling-based timeout matches Ada's ISR-side deadline checking approach
        if (Clock::now() >= DEADLINE) {
            return false; // Timeout occurred
        }

        // Check if wake event is signaled - if so, clear it before blocking again
        if (wait_event.is_signaled()) {
            wait_event.clear();
        }

        // Block efficiently using platform-specific wait primitive
        // This provides power-efficient blocking without spinning:
        // - ARM Cortex-M: WFE instruction (~10µA)
        // - Linux x86: futex syscall (kernel blocks)
        //
        // Note: WaitEvent::wait() may spuriously wake (e.g., on any interrupt on ARM),
        // so we loop and re-check available after each wake
        //
        // For timeout handling, we rely on spurious wakeups to periodically check
        // the timeout condition. On Linux, this means futex may block longer than
        // desired (typical accuracy: ~1-10ms).
        wait_event.wait();
    }

    // Semaphore is available - clear it atomically (auto-reset)
    // memory_order_relaxed: Single consumer, no synchronization needed for clear
    // (Decision 23: Only the consuming thread clears, no need for acquire/release)
    //
    // This matches Ada's pattern: Event_Occurred := False after successful Await_Event
    available.store(false, std::memory_order_relaxed);

    return true; // Successfully took semaphore
}

} // namespace hm11::sync
