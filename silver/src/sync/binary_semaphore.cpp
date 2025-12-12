// HM11 Synchronization Primitives - Binary Semaphore Implementation
// ISR-safe binary semaphore implementation

#include "hm11/sync/binary_semaphore.hpp"

#include <thread>

namespace hm11::sync {

void BinarySemaphore::GiveFromISR() noexcept {
    // Set status to OK and signal
    // Use release ordering to ensure all preceding ISR operations are visible
    // before signaling completion
    status_.store(hal::UARTStatus::Ok, std::memory_order_release);
    wait_event_.SignalFromISR();
}

void BinarySemaphore::GiveFromISRWithStatus(hal::UARTStatus status) noexcept {
    // Store error status and signal
    // Use release ordering to ensure error information is visible
    // before thread wakes up
    status_.store(status, std::memory_order_release);
    wait_event_.SignalFromISR();
}

hal::UARTStatus BinarySemaphore::TakeWithTimeout(std::chrono::milliseconds timeout) noexcept {
    // Simple implementation optimized for rapid cycling
    // First try immediate check using the wait event's TakeSignal if available
    if (wait_event_.TakeSignal()) {
        return status_.load(std::memory_order_acquire);
    }

    // If no immediate signal, do a quick wait
    const bool signaled = wait_event_.WaitWithTimeout(timeout);

    if (signaled && wait_event_.TakeSignal()) {
        return status_.load(std::memory_order_acquire);
    }

    return hal::UARTStatus::ErrTimeout;
}

bool BinarySemaphore::IsSignaled() const noexcept {
    // Check if currently signaled without waiting
    // Use acquire ordering for consistency with TakeWithTimeout
    return wait_event_.IsSignaled();
}

void BinarySemaphore::Reset() noexcept {
    // Reset semaphore to non-signaled state
    // This matches Ada's manual Event_Occurred := False
    wait_event_.Clear();
    status_.store(hal::UARTStatus::Ok, std::memory_order_relaxed);
}

} // namespace hm11::sync