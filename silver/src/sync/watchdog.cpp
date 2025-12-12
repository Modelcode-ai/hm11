// HM11 Synchronization Primitives - Watchdog Timer Implementation
// ISR-side deadline polling implementation

#include "hm11/sync/watchdog.hpp"

namespace hm11::sync {

void Watchdog::Start(std::chrono::milliseconds timeout) noexcept {
    // Calculate absolute deadline
    // Equivalent to Ada's Until_Time := Clock + Timeout
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    // Store deadline with release ordering to ensure visibility to ISR
    deadline_.store(deadline, std::memory_order_release);

    // Mark as started with release ordering
    started_.store(true, std::memory_order_release);
}

void Watchdog::Stop() noexcept {
    // Mark as stopped - ISR will no longer check timeout
    // Use release ordering to ensure ISR sees the stop
    started_.store(false, std::memory_order_release);

    // Reset semaphore state for next operation
    semaphore_.Reset();
}

bool Watchdog::CheckTimeoutFromISR() noexcept {
    // Check if watchdog is started (quick exit if not timing)
    if (!started_.load(std::memory_order_acquire)) {
        return false; // Not started, no timeout to check
    }

    // Get deadline and check against current time
    // Equivalent to Ada's "if Clock > Until_Time then"
    const auto deadline = deadline_.load(std::memory_order_acquire);
    const auto now = std::chrono::steady_clock::now();

    if (now > deadline) {
        // Timeout occurred - atomically stop the watchdog to prevent repeated signaling
        // Only one thread should be able to signal timeout
        bool expected = true;
        if (started_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
            // Successfully claimed the timeout - signal via semaphore
            semaphore_.GiveFromISRWithStatus(hal::UARTStatus::ErrTimeout);
            return true; // Timeout signaled by this thread
        }
        // Another thread already signaled timeout
        return false;
    }

    return false; // Still within deadline
}

hal::UARTStatus Watchdog::AwaitCompletion() noexcept {
    // Wait indefinitely for either completion or timeout signal
    // The timeout checking is done by ISR calling CheckTimeoutFromISR()
    // This matches Ada's entry barrier pattern where timeout is managed
    // by the protected object internally

    // Use a large but safe timeout value (10 minutes)
    // This avoids integer overflow while still being effectively infinite
    // for the purposes of this method since ISR handles actual timeout
    return semaphore_.TakeWithTimeout(std::chrono::minutes(10));
}

void Watchdog::SignalCompletionFromISR(hal::UARTStatus status) noexcept {
    // Mark watchdog as stopped since operation completed
    started_.store(false, std::memory_order_relaxed);

    // Signal completion with status
    semaphore_.GiveFromISRWithStatus(status);
}

bool Watchdog::IsStarted() const noexcept {
    // Check if currently timing
    return started_.load(std::memory_order_acquire);
}

} // namespace hm11::sync