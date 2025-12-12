#pragma once

// HM11 Synchronization Primitives - Binary Semaphore
// ISR-safe binary semaphore for event notification matching Ada's Protected Object pattern
// Replaces Ada's Watchdog protected object with C++ atomic operations and platform wait primitives

#include <atomic>
#include <chrono>

#include "hm11/hal/uart_types.hpp"
#include "hm11/platform/wait_event.hpp"

namespace hm11::sync {

/// ISR-safe binary semaphore for event notification
/// Equivalent to Ada's Watchdog protected object with entry barriers
///
/// Usage Pattern:
/// 1. Thread calls TakeWithTimeout() to wait for event
/// 2. ISR calls GiveFromISR() to signal event completion
/// 3. Optional: GiveFromISRWithStatus() for error propagation
///
/// Memory Ordering:
/// - GiveFromISR: uses memory_order_release for ISR writes
/// - TakeWithTimeout: uses memory_order_acquire for thread reads
/// - Status operations: use appropriate ordering for error propagation
///
/// Corresponds to Ada's:
/// - protected Watchdog entry Await_Event when Event_Occurred
/// - procedure Release called from ISR context
/// - automatic state management with entry barriers
class BinarySemaphore {
  public:
    /// Default constructor - initializes semaphore in non-signaled state
    BinarySemaphore() noexcept = default;

    /// Non-copyable and non-movable (following RAII ownership model Decision 22)
    BinarySemaphore(const BinarySemaphore&) = delete;
    BinarySemaphore& operator=(const BinarySemaphore&) = delete;
    BinarySemaphore(BinarySemaphore&&) = delete;
    BinarySemaphore& operator=(BinarySemaphore&&) = delete;

    /// Default destructor
    ~BinarySemaphore() noexcept = default;

    /// Signal semaphore from ISR context
    /// Equivalent to Ada's Watchdog.Release(Ok) procedure
    /// Uses memory_order_release to ensure visibility of preceding ISR operations
    /// Must be callable from interrupt handlers with minimal latency (<10µs)
    void GiveFromISR() noexcept;

    /// Signal semaphore with status from ISR context
    /// Equivalent to Ada's Watchdog.Release(Status) with error propagation
    /// Uses memory_order_release for both status and signaling
    /// Status will be returned by corresponding TakeWithTimeout call
    void GiveFromISRWithStatus(hal::UARTStatus status) noexcept;

    /// Wait for semaphore with timeout from thread context
    /// Equivalent to Ada's Watchdog entry Await_Event(Status) when Event_Occurred
    /// Returns status indicating success/timeout/error
    /// Uses memory_order_acquire to ensure visibility of ISR operations
    ///
    /// @param timeout Maximum time to wait in milliseconds
    /// @return UARTStatus::Ok if signaled, UARTStatus::ErrTimeout if timeout,
    ///         or error status passed from GiveFromISRWithStatus
    [[nodiscard]] hal::UARTStatus TakeWithTimeout(std::chrono::milliseconds timeout) noexcept;

    /// Check if semaphore is currently signaled without waiting
    /// Equivalent to checking Ada's Event_Occurred flag
    /// Uses memory_order_acquire for consistency
    [[nodiscard]] bool IsSignaled() const noexcept;

    /// Reset semaphore to non-signaled state
    /// Equivalent to Ada's Event_Occurred := False
    /// Typically called internally after successful take
    void Reset() noexcept;

  private:
    /// Platform-specific wait/signal mechanism
    /// Encapsulates ARM WFE/SEV or Linux futex operations
    platform::WaitEvent wait_event_;

    /// Status value passed from ISR context
    /// Equivalent to Ada's Read_Status variable in protected object
    /// Uses atomic operations for ISR-to-thread communication:
    /// - ISR stores with memory_order_release
    /// - Thread loads with memory_order_acquire
    std::atomic<hal::UARTStatus> status_{hal::UARTStatus::Ok};

    /// Internal signaled state for atomic operations
    /// true = signaled (event occurred), false = not signaled (waiting)
    /// Managed through wait_event_ but exposed for status checks
    /// Memory ordering handled by wait_event_ operations

    /// Atomic flag to ensure only one waiter succeeds (for multi-consumer use)
    /// Used to implement proper binary semaphore semantics
    std::atomic<bool> taken_{false};
};

// Implementation note: ISR-side timeout polling (Decision 25) is implemented
// by storing deadline in the semaphore and checking in ISR when it fires
// for other events. This matches Ada's Until_Time pattern exactly.

} // namespace hm11::sync