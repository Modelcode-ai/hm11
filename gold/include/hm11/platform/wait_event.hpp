/**
 * @file wait_event.hpp
 * @brief Platform abstraction for low-power blocking primitives
 *
 * This header provides the WaitEvent abstraction that wraps hardware-specific
 * wait mechanisms for both ARM Cortex-M (production target) and Linux x86
 * (testing/development). This abstraction is fundamental to the entire
 * synchronization infrastructure, enabling BinarySemaphore and other primitives
 * to block threads efficiently without spinning.
 *
 * ## Overview
 *
 * The HM11 driver must support both production deployment on ARM microcontrollers
 * and automated testing on x86 Linux development machines. Ada's protected objects
 * with entry barriers abstract away platform differences; in C++, we must
 * explicitly handle these platform variations.
 *
 * ## Platform Implementations
 *
 * ### ARM Cortex-M (Production)
 * - Uses WFE (Wait For Event) instruction to put CPU in low-power state
 * - Uses SEV (Send Event) instruction to wake waiting cores
 * - Sub-10µs wakeup latency
 * - µA-level power consumption in WFE state
 * - ISR-safe (signal() callable from interrupt handlers)
 * - Memory barriers (DMB) ensure proper synchronization with atomic operations
 *
 * ### Linux x86 (Testing/Development)
 * - Uses futex(2) syscalls (FUTEX_WAIT/FUTEX_WAKE)
 * - Provides kernel-level blocking without spinning
 * - Enables identical blocking semantics to ARM WFE/SEV for testing
 * - ThreadSanitizer compatible for race detection
 *
 * ## Design Principles (Decision 19)
 *
 * - **Zero RTOS dependency**: No external dependencies for production
 * - **Hardware low-power states**: 10,000x power savings vs spinning
 * - **Minimal memory footprint**: 4 bytes per instance (atomic uint32_t)
 * - **ISR-safe signaling**: signal() callable from interrupt handlers
 * - **Zero-cost abstraction**: Inline operations, compile-time platform selection
 * - **Testing support**: Enables automated testing on x86 development machines
 *
 * ## Memory Ordering (Decision 23)
 *
 * This class uses std::memory_order_release/acquire for proper synchronization:
 * - signal() uses memory_order_release when publishing event
 * - wait() uses memory_order_acquire when observing event
 * - Provides happens-before relationship for ISR-to-thread communication
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/platform/wait_event.hpp>
 *
 * // Typical usage in ISR-to-thread synchronization
 * hm11::platform::WaitEvent event;
 *
 * // In thread context: block until ISR signals
 * void worker_thread() {
 *     while (running) {
 *         event.wait();  // Blocks in low-power state
 *         // Process data signaled by ISR
 *         handle_isr_event();
 *         event.clear();  // Reset for next event
 *     }
 * }
 *
 * // In ISR context: signal waiting thread
 * void uart_isr() {
 *     // ... handle interrupt ...
 *     event.signal();  // Wake waiting thread
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * - signal() is ISR-safe and thread-safe (can be called from any context)
 * - wait() must be called from thread context only (not from ISR)
 * - Multiple threads can wait on the same event (broadcast wake semantics)
 * - No internal locks or blocking operations in signal()
 *
 * ## Performance Characteristics
 *
 * ### ARM Cortex-M
 * - signal() latency: ~2-3 CPU cycles (SEV instruction + DMB barrier)
 * - wait() wakeup latency: <10µs
 * - Power consumption in wait: <10µA (depends on MCU)
 * - Memory: 4 bytes per instance
 *
 * ### Linux x86
 * - signal() latency: ~1-2µs (syscall overhead)
 * - wait() wakeup latency: ~5-10µs (context switch)
 * - Memory: 4 bytes per instance
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace hm11::platform {

/**
 * @brief Platform-independent event synchronization primitive
 *
 * Provides low-power blocking wait with ISR-safe signaling. The implementation
 * is selected at compile-time based on the target platform:
 * - ARM Cortex-M: WFE/SEV instructions with memory barriers
 * - Linux x86: futex(2) syscalls
 *
 * ## Memory Ordering Guarantees
 *
 * This class provides happens-before relationships for ISR-to-thread
 * communication:
 * - All memory operations before signal() are visible after wait() returns
 * - Uses memory_order_release in signal() and memory_order_acquire in wait()
 * - Compatible with std::atomic operations on shared data
 *
 * ## Typical Usage Pattern
 *
 * 1. Thread calls wait() and blocks in low-power state
 * 2. ISR signals event completion via signal()
 * 3. Thread wakes up and processes data
 * 4. Thread calls clear() to reset event for next iteration
 *
 * ## Signal-Before-Wait Behavior
 *
 * If signal() is called before wait(), the subsequent wait() will return
 * immediately without blocking. This prevents lost wakeups and ensures
 * reliable event delivery.
 *
 * @note This class is not copyable or movable (similar to Ada's limited private)
 * @note All operations are noexcept for ISR safety
 */
class WaitEvent {
  public:
    /**
     * @brief Default constructor - initializes event to non-signaled state
     *
     * The event starts in the cleared state (0), requiring a signal() call
     * before wait() will return.
     */
    WaitEvent() noexcept;

    /**
     * @brief Destructor
     *
     * No cleanup needed - all state is stack-allocated.
     */
    ~WaitEvent() = default;

    // Delete copy and move operations (matching Ada's limited private pattern)
    WaitEvent(const WaitEvent&) = delete;
    WaitEvent& operator=(const WaitEvent&) = delete;
    WaitEvent(WaitEvent&&) = delete;
    WaitEvent& operator=(WaitEvent&&) = delete;

    /**
     * @brief Block until event is signaled
     *
     * Puts the calling thread into a low-power blocking state until signal()
     * is called. On ARM Cortex-M, this uses the WFE instruction to enter a
     * low-power state. On Linux, this uses futex FUTEX_WAIT.
     *
     * If the event was already signaled (signal() called before wait()), this
     * function returns immediately without blocking.
     *
     * ## Platform Behavior
     *
     * ### ARM Cortex-M:
     * - Executes WFE instruction to enter low-power state
     * - CPU wakes on SEV event or any interrupt
     * - Spins if event not signaled after wakeup (handles spurious wakeups)
     * - Memory barrier (DMB) ensures memory_order_acquire semantics
     *
     * ### Linux x86:
     * - Calls futex(FUTEX_WAIT) to block in kernel
     * - Wakes on futex(FUTEX_WAKE) from signal()
     * - Handles spurious wakeups by checking atomic state
     *
     * @note This function must be called from thread context only (not ISR)
     * @note After wait() returns, call clear() before the next wait() cycle
     *
     * @post Event remains in signaled state (must call clear() to reset)
     */
    void wait() noexcept;

    /**
     * @brief Signal event to wake waiting threads
     *
     * Signals the event to wake any threads blocked in wait(). On ARM Cortex-M,
     * this executes the SEV instruction. On Linux, this calls futex FUTEX_WAKE.
     *
     * If called before wait(), the subsequent wait() will return immediately
     * without blocking (signal-before-wait behavior).
     *
     * ## Platform Behavior
     *
     * ### ARM Cortex-M:
     * - Sets atomic flag to 1 with memory_order_release
     * - Executes DMB barrier to ensure memory visibility
     * - Executes SEV instruction to wake cores in WFE
     *
     * ### Linux x86:
     * - Sets atomic flag to 1 with memory_order_release
     * - Calls futex(FUTEX_WAKE) to wake one waiting thread
     *
     * @note This function is ISR-safe and thread-safe
     * @note Can be called multiple times without harm (idempotent until clear())
     * @note No blocking operations - safe for real-time and ISR contexts
     */
    void signal() noexcept;

    /**
     * @brief Clear event to prepare for next wait
     *
     * Resets the event to the non-signaled state. This should be called after
     * wait() returns and the signaled event has been processed, preparing for
     * the next wait/signal cycle.
     *
     * ## Memory Ordering
     *
     * Uses memory_order_relaxed since this is typically called by the single
     * consumer thread after processing the event. No synchronization with other
     * threads is needed for the clear operation itself.
     *
     * @note Should be called from the same thread that called wait()
     * @note Safe to call even if event is already cleared (idempotent)
     */
    void clear() noexcept;

    /**
     * @brief Check if event is currently signaled
     *
     * Returns true if the event is in the signaled state (signal() was called
     * and clear() has not been called yet).
     *
     * ## Memory Ordering
     *
     * Uses memory_order_acquire to ensure visibility of memory operations
     * from the thread that called signal().
     *
     * @return true if event is signaled, false otherwise
     *
     * @note This is a polling operation - prefer wait() for blocking
     */
    [[nodiscard]] bool is_signaled() const noexcept;

  private:
    /**
     * @brief Platform-specific wait implementation
     *
     * Blocks until the event is signaled. Implementation varies by platform:
     * - ARM: WFE instruction with spin loop
     * - Linux: futex FUTEX_WAIT syscall
     */
    void wait_impl() noexcept;

    /**
     * @brief Platform-specific signal implementation
     *
     * Wakes threads waiting on this event. Implementation varies by platform:
     * - ARM: SEV instruction
     * - Linux: futex FUTEX_WAKE syscall
     */
    void signal_impl() noexcept;

    /**
     * @brief Atomic event state
     *
     * States:
     * - 0: Event not signaled (cleared)
     * - 1: Event signaled (waiting threads should wake)
     *
     * Memory ordering:
     * - signal() uses memory_order_release (publish event)
     * - wait() uses memory_order_acquire (observe event)
     * - clear() uses memory_order_relaxed (single consumer)
     *
     * Size: 4 bytes (uint32_t for futex compatibility on Linux)
     */
    std::atomic<uint32_t> state{0};
};

} // namespace hm11::platform
