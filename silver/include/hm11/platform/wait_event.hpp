#pragma once

// HM11 Platform Abstraction Layer - Wait/Signal Primitives
// Platform-specific implementation of wait/signal operations for ISR-to-thread communication
// Uses ARM WFE/SEV for bare-metal ARM targets, Linux futex for testing

#include <atomic>
#include <chrono>
#include <cstdint>

namespace hm11::platform {

/// Platform-specific wait/signal abstraction for low-power blocking
/// Provides ISR-safe signaling with minimal overhead and power consumption
///
/// Implementation Strategy:
/// - ARM Cortex-M: WFE/SEV instructions (~10µs latency, µA power consumption)
/// - Linux x86: futex syscalls (testing only, higher overhead)
/// - Fallback: std::atomic with yield (compatibility, power inefficient)
class WaitEvent {
  public:
    /// Default constructor - initializes wait state
    WaitEvent() noexcept : signaled_{false} {}

    /// Non-copyable and non-movable (following RAII ownership model)
    WaitEvent(const WaitEvent&) = delete;
    WaitEvent& operator=(const WaitEvent&) = delete;
    WaitEvent(WaitEvent&&) = delete;
    WaitEvent& operator=(WaitEvent&&) = delete;

    /// Default destructor
    ~WaitEvent() noexcept = default;

    /// Signal from ISR context
    /// Must be callable from interrupt handlers with minimal latency
    /// Uses memory_order_release to ensure visibility of preceding writes
    void SignalFromISR() noexcept;

    /// Wait with timeout from thread context
    /// Blocks until signaled or timeout expires
    /// Returns true if signaled, false if timeout
    /// Uses memory_order_acquire to ensure visibility of ISR writes
    [[nodiscard]] bool WaitWithTimeout(std::chrono::milliseconds timeout) noexcept;

    /// Clear signaled state (typically called after successful wait)
    /// Uses memory_order_relaxed as no synchronization needed for single consumer
    void Clear() noexcept;

    /// Check if currently signaled without waiting
    /// Uses memory_order_acquire for consistency with WaitWithTimeout
    [[nodiscard]] bool IsSignaled() const noexcept;

    /// Atomically check and clear the signal (for binary semaphore behavior)
    /// Returns true if signal was set and successfully cleared, false otherwise
    /// Only one thread can successfully take the signal
    [[nodiscard]] bool TakeSignal() noexcept;

  private:
    /// Atomic signal state for ISR-to-thread communication
    /// true = signaled (ready), false = not signaled (waiting)
    /// Memory ordering:
    /// - SignalFromISR: store with memory_order_release
    /// - WaitWithTimeout/IsSignaled: load with memory_order_acquire
    /// - Clear: store with memory_order_relaxed
    std::atomic<bool> signaled_;

#if defined(__arm__) || defined(__aarch64__)
    /// ARM platform: Use WFE/SEV instructions for hardware low-power wait
    static void PlatformWait() noexcept;
    static void PlatformSignal() noexcept;
#elif defined(__linux__)
    /// Linux platform: Use futex system calls for testing
    void PlatformWaitWithTimeout(std::chrono::milliseconds timeout) noexcept;
    static void PlatformSignal() noexcept;

    /// Linux-specific futex state (32-bit value expected by futex syscalls)
    std::atomic<std::uint32_t> futex_word_{0};
#endif
};

// Platform-specific implementation details
// Implementation is in wait_event.cpp to keep header lightweight

} // namespace hm11::platform