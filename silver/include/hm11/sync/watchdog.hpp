#pragma once

// HM11 Synchronization Primitives - Watchdog Timer
// ISR-side deadline polling mechanism matching Ada's timeout pattern
// Replaces Ada's Until_Time variable with C++ std::chrono and atomic operations

#include <atomic>
#include <chrono>

#include "hm11/hal/uart_types.hpp"
#include "hm11/sync/binary_semaphore.hpp"

namespace hm11::sync {

/// Watchdog timer with ISR-side deadline polling
/// Equivalent to Ada's Until_Time timeout mechanism in Watchdog protected object
///
/// Usage Pattern:
/// 1. Thread calls Start() with timeout duration before beginning operation
/// 2. ISR calls CheckTimeout() opportunistically during other interrupt events
/// 3. If deadline exceeded, ISR signals timeout via binary semaphore
/// 4. Thread waits on semaphore for either data completion or timeout
///
/// Timeout Implementation:
/// - Start() calculates deadline as current_time + timeout
/// - CheckTimeout() compares current_time > deadline (ISR context)
/// - No additional timer hardware required - timeout check piggybacks on other interrupts
/// - Matches Ada's "if Clock > Until_Time then" pattern exactly
///
/// Corresponds to Ada's:
/// - Until_Time : Time; variable in drivers.adb line 41
/// - Until_Time := Clock + Timeout; in Watchdog.Start line 90
/// - if Clock > Until_Time then check in On_UART_IRQ line 282
class Watchdog {
  public:
    /// Construct watchdog in stopped state
    Watchdog() noexcept : started_(false) {}

    /// Non-copyable and non-movable (following RAII ownership model)
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /// Default destructor
    ~Watchdog() noexcept = default;

    /// Start watchdog timer with specified timeout
    /// Equivalent to Ada's Watchdog.Start with Until_Time := Clock + Timeout
    /// Sets deadline for ISR-side timeout checking
    ///
    /// @param timeout Duration to wait before timing out
    void Start(std::chrono::milliseconds timeout) noexcept;

    /// Stop watchdog timer
    /// Equivalent to Ada's Watchdog.Stop procedure
    /// Disables timeout checking and resets state
    void Stop() noexcept;

    /// Check for timeout from ISR context
    /// Equivalent to Ada's "if Clock > Until_Time then" check in IRQ handler
    /// Should be called opportunistically from UART/DMA interrupts
    /// Signals timeout via semaphore if deadline exceeded
    ///
    /// @return true if timeout occurred and signaled, false if still within deadline
    bool CheckTimeoutFromISR() noexcept;

    /// Wait for completion or timeout
    /// Equivalent to Ada's Watchdog entry Await_Event(Status) when Event_Occurred
    /// Blocks until either signaled by operation completion or timeout
    ///
    /// @return UARTStatus::Ok if operation completed,
    ///         UARTStatus::ErrTimeout if timeout occurred,
    ///         or other error status from operation
    [[nodiscard]] hal::UARTStatus AwaitCompletion() noexcept;

    /// Signal operation completion from ISR context
    /// Equivalent to Ada's Watchdog.Release(Status) procedure called from IRQ
    /// Signals completion with given status
    ///
    /// @param status Result status of the completed operation
    void SignalCompletionFromISR(hal::UARTStatus status) noexcept;

    /// Check if watchdog is currently started
    /// Equivalent to Ada's Started flag in protected object
    [[nodiscard]] bool IsStarted() const noexcept;

  private:
    /// Binary semaphore for ISR-to-thread signaling
    /// Handles the actual wait/signal mechanism
    BinarySemaphore semaphore_;

    /// Whether watchdog is currently active
    /// Equivalent to Ada's Started : Boolean := False
    /// true = timing, false = stopped
    std::atomic<bool> started_;

    /// Deadline for timeout checking
    /// Equivalent to Ada's Until_Time : Time variable
    /// ISR compares current time against this deadline
    /// Updated atomically when starting watchdog
    std::atomic<std::chrono::steady_clock::time_point> deadline_;
};

} // namespace hm11::sync