/**
 * @file watchdog.hpp
 * @brief Watchdog timeout manager for coordinating ISR-to-thread operations
 *
 * This header provides the Watchdog class that manages the lifecycle of blocking
 * UART operations with timeout semantics. It orchestrates event signaling, error
 * propagation, and deadline-based timeout detection between ISR and application
 * contexts.
 *
 * ## Overview
 *
 * The Watchdog class directly translates Ada's Watchdog protected object
 * (drivers.ads lines 88-112, drivers.adb lines 47-187) into a C++ class that
 * coordinates all blocking receive operations in the HM11 driver.
 *
 * The Ada Watchdog protected object provides:
 * - Start procedure: Begin operation with timeout deadline
 * - Stop procedure: Cancel operation and cleanup hardware state
 * - Release procedure: ISR signals completion with status
 * - Await_Event entry: Application blocks until completion or timeout
 *
 * This C++ implementation preserves the same semantics using atomic operations,
 * BinarySemaphore for blocking/signaling, and ISR-side deadline polling for
 * timeout detection.
 *
 * ## Design Principles
 *
 * **Decision 25 (ISR-Side Deadline Polling):**
 * - Deadline stored as absolute time_point calculated at Start()
 * - ISR checks `steady_clock::now() > deadline` opportunistically when it fires
 * - No dedicated timer interrupt needed
 * - Timeout accuracy depends on ISR firing frequency
 *
 * **Decision 26 (Atomic Status Flags for Error Propagation):**
 * - std::atomic<UartStatus> for propagating errors from ISR to thread
 * - Release() uses memory_order_release when storing status
 * - Await_Event() uses memory_order_acquire when reading status
 * - Ensures all ISR buffer writes are visible when status becomes visible
 *
 * ## Lifecycle of a Blocking Operation
 *
 * 1. **Start**: Application calls Start(buffer, length, timeout_ms)
 *    - Calculates deadline = now() + timeout
 *    - Clears status to Ok
 *    - Resets semaphore state
 *    - In full implementation: configures DMA/UART interrupts
 *
 * 2. **Await**: Application calls Await_Event(timeout_ms)
 *    - Blocks on BinarySemaphore::take_with_timeout()
 *    - Returns when ISR signals via Release() or timeout expires
 *    - Reads and returns status code
 *
 * 3. **ISR Processing**: Interrupt handler processes data
 *    - On completion: calls Release(Ok)
 *    - On error: calls Release(ErrError)
 *    - On timeout detected (Clock > deadline): calls Release(ErrTimeout)
 *
 * 4. **Release**: ISR signals completion
 *    - Stores status with memory_order_release
 *    - Signals semaphore via give_from_isr()
 *    - Calls Stop() to cleanup hardware
 *
 * 5. **Stop**: Cleanup (called by Release or application)
 *    - Disables interrupts
 *    - Clears hardware status flags
 *    - Sets Started = false
 *    - Idempotent (safe to call multiple times)
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/sync/watchdog.hpp>
 * #include <chrono>
 *
 * using namespace std::chrono_literals;
 * using hm11::sync::Watchdog;
 * using hm11::hal::UartStatus;
 *
 * Watchdog watchdog;
 * uint8_t buffer[128];
 *
 * // Start receive operation with 1 second timeout
 * watchdog.start(buffer, sizeof(buffer), 1000ms);
 *
 * // Wait for completion (blocks until ISR signals or timeout)
 * UartStatus status = watchdog.await_event(1000ms);
 *
 * if (status == UartStatus::Ok) {
 *     // Process received data in buffer
 *     process_data(buffer);
 * } else if (status == UartStatus::ErrTimeout) {
 *     // Handle timeout
 *     handle_timeout();
 * } else {
 *     // Handle error
 *     handle_error(status);
 * }
 *
 * // ISR context: signal completion
 * void uart_complete_isr() {
 *     // ... receive data into buffer ...
 *     watchdog.release(UartStatus::Ok);  // Wake waiting thread
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * - Start() must be called from application thread context only
 * - Stop() can be called from both application and ISR contexts
 * - Release() is ISR-safe (non-blocking, no locks)
 * - Await_Event() must be called from application thread context only
 * - Designed for single application thread and single ISR producer
 *
 * ## Ada Translation Notes
 *
 * **Ada Protected Object:**
 * @code{.ada}
 * protected Watchdog is
 *    pragma Interrupt_Priority;
 *    procedure Start(Received, Length, Timeout);
 *    procedure Stop;
 *    entry Await_Event(Status) when Event_Occurred;
 *    procedure Release(Status);
 * private
 *    Started : Boolean := False;
 *    Event_Occurred : Boolean := False;
 *    Read_Status : UartStatus := Ok;
 * end Watchdog;
 * @endcode
 *
 * **C++ Translation:**
 * @code
 * class Watchdog {
 *   BinarySemaphore semaphore_;        // Replaces Event_Occurred + entry barrier
 *   std::atomic<UartStatus> status_;  // Replaces Read_Status (atomic for ISR safety)
 *   std::atomic<bool> started_;        // Replaces Started (atomic for ISR safety)
 *   time_point deadline_;              // Replaces Until_Time
 * };
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
#include <cstddef>
#include <cstdint>
#include <hm11/hal/base_types.hpp>
#include <hm11/sync/binary_semaphore.hpp>

namespace hm11::sync {

/**
 * @brief Watchdog timeout manager for blocking UART operations
 *
 * Coordinates timeout handling, event signaling, and error propagation between
 * ISR and application contexts. This class implements Ada's Watchdog protected
 * object pattern in C++ using atomic operations and BinarySemaphore.
 *
 * ## Key Operations
 *
 * - **start()**: Begin operation with timeout deadline (thread context)
 * - **stop()**: Cancel operation and cleanup (thread or ISR context)
 * - **release()**: Signal completion with status (ISR context)
 * - **await_event()**: Block until completion or timeout (thread context)
 * - **check_deadline()**: ISR checks if timeout expired (ISR context)
 *
 * ## Memory Layout
 *
 * Size: ~16 bytes (BinarySemaphore + atomics + time_point)
 *
 * ## Timeout Implementation (Decision 25)
 *
 * The timeout mechanism uses ISR-side deadline polling:
 * - Deadline stored as absolute steady_clock::time_point
 * - Calculated as `current_time + timeout` when start() is called
 * - ISR calls check_deadline() opportunistically when it fires
 * - If `now() > deadline`, ISR calls release(ErrTimeout)
 * - No dedicated timer interrupt needed
 * - Accuracy depends on ISR firing frequency (~1-10ms typical)
 *
 * ## Error Propagation (Decision 26)
 *
 * Status codes propagate from ISR to application using atomic operations:
 * - release() stores status with memory_order_release before signaling
 * - await_event() reads status with memory_order_acquire after waking
 * - This ensures all ISR buffer writes are visible when status is read
 *
 * ## State Management
 *
 * The Started flag prevents invalid operation sequences:
 * - start() fails if already Started (idempotent, returns early)
 * - stop() succeeds only if Started (idempotent, returns early if not)
 * - release() calls stop() to ensure cleanup happens exactly once
 *
 * @note This class is not copyable or movable (matching Ada's limited private)
 * @note All public methods are noexcept for embedded reliability
 */
class Watchdog {
  public:
    /**
     * @brief Default constructor - initializes Watchdog to idle state
     *
     * The Watchdog starts in non-started state (started_ = false) with Ok status.
     * All internal state is stack-allocated.
     */
    Watchdog() noexcept = default;

    /**
     * @brief Destructor
     *
     * No cleanup needed - all state is stack-allocated.
     * If an operation is in progress when the Watchdog is destroyed, behavior
     * is undefined (caller must ensure operations are stopped before destruction).
     */
    ~Watchdog() = default;

    // Delete copy and move operations (matching Ada's limited private pattern, Decision 22)
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /**
     * @brief Begin a blocking UART operation with timeout
     *
     * Initializes the Watchdog for a new operation by setting the deadline,
     * clearing status, and resetting semaphore state. This method prepares
     * internal state but does not directly configure hardware (hardware
     * configuration is handled by the UART/DMA drivers on STM32 targets).
     *
     * ## Operation Sequence
     *
     * 1. Check if already started - if yes, return early (idempotent)
     * 2. Set Started = true
     * 3. Calculate deadline = now() + timeout (Decision 25)
     * 4. Clear status to Ok
     * 5. Reset semaphore state (try_take to clear any stale signals)
     * 6. (DMA/UART interrupt configuration handled by drivers on STM32 targets)
     *
     * ## Idempotency
     *
     * Calling start() while already Started is safe and returns immediately
     * without side effects. This matches Ada's behavior (drivers.adb:85-87):
     * @code{.ada}
     * if Started then
     *    return;
     * end if;
     * @endcode
     *
     * @tparam Rep Arithmetic type representing the number of ticks
     * @tparam Period std::ratio representing the tick period (duration units)
     * @param received_buffer Pointer to buffer for received data
     * @param length Number of bytes expected
     * @param timeout Maximum time to wait for operation completion
     *
     * @note ISR-safe: No (must be called from thread context only)
     * @note Thread-safe: No (designed for single application thread)
     * @note Blocking: No (returns immediately after setup)
     *
     * @pre Must be called from thread context (not from ISR)
     * @pre received_buffer must remain valid until release() is called
     * @pre length must be > 0
     * @pre timeout must be positive duration
     *
     * @post Started flag is set to true
     * @post Deadline is calculated and stored
     * @post Status is cleared to Ok
     * @post Semaphore is in non-signaled state
     *
     * @par Example
     * @code
     * using namespace std::chrono_literals;
     *
     * uint8_t buffer[128];
     * watchdog.start(buffer, sizeof(buffer), 1s);  // Begin 1 second timeout
     * @endcode
     */
    template <typename Rep, typename Period>
    void start(void* received_buffer, std::size_t length, std::chrono::duration<Rep, Period> timeout) noexcept;

    /**
     * @brief Cancel operation and cleanup state
     *
     * Stops the current operation by disabling interrupts and clearing
     * hardware status flags. This method is idempotent and can be called
     * from both application and ISR contexts.
     *
     * ## Operation Sequence
     *
     * 1. Check if started - if not, return early (idempotent)
     * 2. Set Started = false
     * 3. (DMA/UART interrupt disabling handled by drivers on STM32 targets)
     * 4. (Hardware status flag clearing handled by drivers on STM32 targets)
     *
     * ## Idempotency
     *
     * Calling stop() when not Started is safe and returns immediately without
     * side effects. This matches Ada's behavior (drivers.adb:145-147).
     *
     * ## Context Safety
     *
     * stop() can be safely called from both application and ISR contexts
     * because it uses atomic operations for state management. However, the
     * hardware cleanup operations must be ISR-safe.
     *
     * @note ISR-safe: Yes (non-blocking, atomic operations only)
     * @note Thread-safe: Yes (atomic operations ensure correctness)
     * @note Blocking: No (returns immediately)
     *
     * @post If was Started: Started flag is cleared and hardware is disabled
     * @post If was not Started: no side effects
     *
     * @par Example
     * @code
     * // Cancel operation early (e.g., user abort)
     * watchdog.stop();
     *
     * // Safe to call multiple times
     * watchdog.stop();  // No-op on second call
     * @endcode
     */
    void stop() noexcept;

    /**
     * @brief Signal operation completion from ISR context
     *
     * Called by the interrupt handler to signal that the UART operation has
     * completed (successfully or with error). This method stores the status
     * code, signals the semaphore to wake the waiting thread, and calls
     * stop() to cleanup hardware state.
     *
     * ## Operation Sequence (Decision 26)
     *
     * 1. Check if Event_Occurred already (prevent double-signal)
     * 2. Store status with memory_order_release (publish ISR work)
     * 3. Signal semaphore via give_from_isr() (wake waiting thread)
     * 4. Call stop() to cleanup hardware
     *
     * ## Memory Ordering (Decision 26)
     *
     * Uses memory_order_release when storing status to ensure all ISR writes
     * (buffer data, internal state) are visible before the status becomes
     * visible to the application thread:
     *
     * @code
     * // In ISR:
     * buffer[0] = received_byte;           // Write to shared buffer
     * watchdog.release(UartStatus::Ok);   // Release: Publish above writes
     *
     * // In application thread:
     * status = watchdog.await_event(...);  // Acquire: Observe ISR writes
     * if (status == Ok) {
     *     process(buffer[0]);               // Safe to read ISR-written data
     * }
     * @endcode
     *
     * ## Idempotency
     *
     * If release() is called multiple times before await_event(), only the
     * first call has effect (status is not overwritten). This matches Ada's
     * behavior (drivers.adb:65): `if not Event_Occurred then ...`
     *
     * The second and subsequent calls are no-ops that return early.
     *
     * @param status Status code to report (Ok, ErrError, ErrTimeout, etc.)
     *
     * @note ISR-safe: Yes (non-blocking, atomic operations, no locks)
     * @note Thread-safe: Yes (can be called from multiple ISRs)
     * @note Blocking: No (returns immediately)
     *
     * @post Semaphore is signaled (any waiting thread will wake)
     * @post Status is stored and will be returned by await_event()
     * @post Hardware is stopped (interrupts disabled)
     * @post Second call before await_event() is no-op
     *
     * @par Example
     * @code
     * // ISR: Data received successfully
     * void uart_rx_complete_isr() {
     *     // ... copy data to buffer ...
     *     watchdog.release(UartStatus::Ok);
     * }
     *
     * // ISR: Timeout detected
     * void uart_idle_isr() {
     *     if (watchdog.check_deadline()) {
     *         watchdog.release(UartStatus::ErrTimeout);
     *     }
     * }
     *
     * // ISR: Hardware error
     * void uart_error_isr() {
     *     watchdog.release(UartStatus::ErrError);
     * }
     * @endcode
     */
    void release(hal::UartStatus status) noexcept;

    /**
     * @brief Block until operation completes or timeout expires
     *
     * Waits for the ISR to signal completion via release(). This method blocks
     * the calling thread until either the semaphore is signaled or the timeout
     * expires, then returns the status code stored by release().
     *
     * ## Operation Sequence
     *
     * 1. Block on semaphore.take_with_timeout() (power-efficient wait)
     * 2. On wake: read status with memory_order_acquire (Decision 26)
     * 3. Return status code to application
     *
     * ## Memory Ordering (Decision 26)
     *
     * Uses memory_order_acquire when reading status to ensure all ISR writes
     * are visible after waking from semaphore. This pairs with the
     * memory_order_release in release() to provide a happens-before relationship.
     *
     * ## Timeout Behavior
     *
     * If the timeout expires before release() is called, this method returns
     * the current status (typically Ok if no error occurred yet). The
     * application should check if the operation actually completed or if this
     * is a spurious timeout.
     *
     * Note: The actual timeout is handled by the ISR checking the deadline
     * (Decision 25). This timeout parameter should match the timeout passed
     * to start() to ensure consistent behavior.
     *
     * @tparam Rep Arithmetic type representing the number of ticks
     * @tparam Period std::ratio representing the tick period (duration units)
     * @param timeout Maximum time to wait for signal
     * @return Status code set by release() (Ok, ErrError, ErrTimeout, etc.)
     *
     * @note ISR-safe: No (blocks - must be called from thread context only)
     * @note Thread-safe: Yes (but designed for single application thread)
     * @note Blocking: Yes (until signaled or timeout)
     *
     * @pre Must be called from thread context (not from ISR)
     * @pre start() must have been called first
     * @pre timeout should match the timeout passed to start()
     *
     * @post Semaphore is cleared (auto-reset)
     * @post Returns status set by release()
     *
     * @par Example
     * @code
     * using namespace std::chrono_literals;
     *
     * watchdog.start(buffer, length, 1s);
     *
     * // Wait for ISR to signal completion
     * UartStatus status = watchdog.await_event(1s);
     *
     * switch (status) {
     *     case UartStatus::Ok:
     *         // Success - process data
     *         break;
     *     case UartStatus::ErrTimeout:
     *         // Timeout - no data received
     *         break;
     *     case UartStatus::ErrError:
     *         // Hardware error
     *         break;
     *     default:
     *         // Other status
     *         break;
     * }
     * @endcode
     */
    template <typename Rep, typename Period>
    [[nodiscard]] hal::UartStatus await_event(std::chrono::duration<Rep, Period> timeout) noexcept;

    /**
     * @brief Check if deadline has expired (ISR-side timeout detection)
     *
     * This method is called opportunistically by the ISR to detect timeout
     * expiration. It compares the current time against the stored deadline
     * and returns true if the deadline has passed.
     *
     * ## Decision 25: ISR-Side Deadline Polling
     *
     * Instead of using platform timeout primitives or dedicated timer
     * interrupts, this implementation matches Ada's approach where the ISR
     * checks `Clock > Until_Time` opportunistically (drivers.adb:282):
     *
     * @code{.ada}
     * if Clock > Until_Time then
     *    Disable_Interrupts(UART, Source => Idle_Line_Detection);
     *    Clear_Status(UART, Idle_Line_Detection_Indicated);
     *    Watchdog.Release(Ok);  -- Actually releases with ErrTimeout in real code
     * end if;
     * @endcode
     *
     * The ISR calls this method whenever it fires (for any reason: data
     * received, idle detected, error occurred). If the deadline has expired,
     * the ISR calls release(ErrTimeout).
     *
     * ## Timeout Accuracy
     *
     * Accuracy depends on ISR firing frequency:
     * - High-frequency ISRs (DMA, byte-by-byte receive): ~1ms accuracy
     * - Low-frequency ISRs (idle line, periodic timer): ~10ms accuracy
     * - No ISR activity: timeout may not be detected until next interrupt
     *
     * To ensure precise timeouts for receive operations, enable UART idle
     * line detection interrupt, which fires periodically when no data is
     * being received.
     *
     * @return true if current time > deadline (timeout expired), false otherwise
     *
     * @note ISR-safe: Yes (non-blocking, reads atomic and time_point)
     * @note Thread-safe: Yes
     * @note Blocking: No (returns immediately)
     *
     * @par Example
     * @code
     * // ISR: Check timeout on each interrupt
     * void uart_isr() {
     *     // Check if operation timed out
     *     if (watchdog.check_deadline()) {
     *         watchdog.release(UartStatus::ErrTimeout);
     *         return;
     *     }
     *
     *     // Normal interrupt processing
     *     if (data_available()) {
     *         process_received_byte();
     *         watchdog.release(UartStatus::Ok);
     *     }
     * }
     * @endcode
     */
    [[nodiscard]] bool check_deadline() const noexcept;

  private:
    /**
     * @brief Binary semaphore for ISR-to-thread signaling
     *
     * Replaces Ada's Event_Occurred boolean + entry barrier pattern.
     * - release() calls give_from_isr() to signal (sets Event_Occurred = True)
     * - await_event() calls take_with_timeout() to block (entry barrier)
     *
     * The semaphore provides power-efficient blocking using platform-specific
     * wait instructions (WFE on ARM, futex on Linux).
     */
    BinarySemaphore semaphore;

    /**
     * @brief Atomic status code for error propagation (Decision 26)
     *
     * Replaces Ada's Read_Status variable. Uses std::atomic for lock-free
     * propagation from ISR to application thread:
     * - release() stores with memory_order_release
     * - await_event() reads with memory_order_acquire
     *
     * This provides a happens-before relationship ensuring all ISR buffer
     * writes are visible when the application reads the status.
     *
     * Initialized to Ok (no error).
     */
    std::atomic<hal::UartStatus> status{hal::UartStatus::Ok};

    /**
     * @brief Atomic started flag for operation state tracking
     *
     * Replaces Ada's Started boolean. Uses std::atomic for thread-safe
     * state management between application and ISR contexts.
     *
     * States:
     * - false: No operation in progress (idle)
     * - true: Operation in progress
     *
     * This flag enables:
     * - start() idempotency (return early if already started)
     * - stop() idempotency (return early if not started)
     * - Preventing invalid operation sequences
     *
     * Initialized to false (not started).
     */
    std::atomic<bool> started{false};

    /**
     * @brief Atomic event occurred flag for double-signal prevention
     *
     * Replaces Ada's Event_Occurred boolean. Uses std::atomic for thread-safe
     * state management between ISR and application contexts.
     *
     * States:
     * - false: Event not yet signaled (awaiting completion)
     * - true: Event signaled (release() has been called)
     *
     * This flag enables:
     * - release() idempotency (only first call has effect, matching Ada)
     * - Preventing status overwrite from multiple release() calls
     * - Exact translation of Ada's protected object pattern
     *
     * The flag is set to true by release() using atomic compare_exchange,
     * ensuring only the first release() call stores the status. It is
     * cleared by start() to prepare for the next operation.
     *
     * Initialized to false (not signaled).
     */
    std::atomic<bool> event_occurred{false};

    /**
     * @brief Absolute deadline for timeout detection (Decision 25)
     *
     * Replaces Ada's Until_Time variable. Stores the absolute time point
     * when the operation should timeout, calculated as `now() + timeout`
     * when start() is called.
     *
     * The ISR checks `steady_clock::now() > deadline` opportunistically
     * to detect timeout expiration without dedicated timer interrupts.
     *
     * Note: Not atomic because only written by start() (application thread)
     * and read by check_deadline() (ISR), and the ISR never writes to it.
     * The Started flag provides sufficient synchronization.
     */
    std::chrono::steady_clock::time_point deadline;

    /**
     * @brief Pointer to receive buffer
     *
     * Stores the buffer address passed to start(). This is used by the ISR
     * (on STM32F429 targets) to write received data directly to the buffer.
     *
     * The application must ensure this buffer remains valid until release()
     * is called.
     *
     * Note: On non-STM32 targets (test/mock environments), this is stored
     * but not actively used by ISRs.
     */
    void* received_buffer{nullptr};

    /**
     * @brief Expected receive length in bytes
     *
     * Stores the number of bytes expected to receive. This is used by the ISR
     * (on STM32F429 targets) to determine when the receive operation is complete.
     *
     * Note: On non-STM32 targets (test/mock environments), this is stored
     * but not actively used by ISRs.
     */
    std::size_t length{0};
};

// Template implementation must be in header

template <typename Rep, typename Period>
void Watchdog::start(
    void* received_buffer_param,
    std::size_t length_param,
    std::chrono::duration<Rep, Period> timeout) noexcept {
    // Check if already started (idempotent behavior, matches Ada drivers.adb:85-87)
    if (started.load(std::memory_order_acquire)) {
        return;
    }

    // Mark as started
    started.store(true, std::memory_order_release);

    // Store buffer and length for ISR use
    received_buffer = received_buffer_param;
    length = length_param;

    // Calculate absolute deadline (Decision 25: ISR-side deadline polling)
    // This matches Ada's pattern: Until_Time := Clock + Timeout (drivers.adb:90)
    deadline = std::chrono::steady_clock::now() + timeout;

    // Clear status to Ok (matching Ada's implicit initialization)
    status.store(hal::UartStatus::Ok, std::memory_order_relaxed);

    // Clear event_occurred flag to prepare for new operation
    // This matches Ada's Event_Occurred := False in the Await_Event entry (drivers.adb:56)
    event_occurred.store(false, std::memory_order_relaxed);

    // Reset semaphore state to ensure clean start
    // If there's a stale signal from a previous operation, clear it
    (void)semaphore.try_take();

    // Note: DMA/UART interrupt configuration handled by drivers on STM32 targets
    // - Clear hardware status flags
    // - Start DMA transfer
    // - Enable UART interrupts (idle, error, etc.)
}

template <typename Rep, typename Period>
[[nodiscard]] hal::UartStatus Watchdog::await_event(std::chrono::duration<Rep, Period> timeout) noexcept {
    // Block until ISR signals via release() or timeout expires
    // The timeout here should match the timeout passed to start() to ensure
    // consistent behavior. The actual timeout is detected by the ISR checking
    // the deadline (Decision 25), but we also need a timeout here to prevent
    // indefinite blocking if the ISR fails to signal.
    (void)semaphore.take_with_timeout(timeout);

    // Read status with memory_order_acquire (Decision 26)
    // This ensures we observe all ISR writes that happened before release()
    // stored the status with memory_order_release
    return status.load(std::memory_order_acquire);
}

} // namespace hm11::sync
