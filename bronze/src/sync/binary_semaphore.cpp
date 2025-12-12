/**
 * @file binary_semaphore.cpp
 * @brief Implementation of the BinarySemaphore class
 *
 * This file implements the BinarySemaphore class, providing ISR-safe synchronization
 * primitives for the HM11 driver. The implementation follows Decision 19 from the
 * modernization specs, using platform-specific primitives for efficient thread waiting.
 */

#include <hm11/platform/wait_event.hpp>
#include <hm11/sync/binary_semaphore.hpp>

namespace hm11::sync {

BinarySemaphore::BinarySemaphore(bool initial_state) : signaled(initial_state ? 1U : 0U), status(UartStatus::Ok) {
    // Nothing else to initialize
}

BinarySemaphore::~BinarySemaphore() {
    // Ensure we don't have any hanging waiters before destruction
    give_from_isr(UartStatus::ErrInvalidState);
}

bool BinarySemaphore::take_with_timeout(UartStatus& status_out, std::chrono::milliseconds timeout_ms) {
    // Calculate absolute deadline for timeout handling
    deadline = std::chrono::steady_clock::now() + timeout_ms;

    // If already signaled, get the status and return immediately
    if (signaled.load(std::memory_order_acquire) != 0) {
        status_out = status.load(std::memory_order_acquire);
        signaled.store(0U, std::memory_order_release); // Reset signaled state
        return true;
    }

    // Use platform-specific wait mechanism
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    bool result = hm11::platform::WaitEvent::wait_with_timeout(
        // Cast to the expected type - this is safe because we're just monitoring the address
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const volatile std::uint32_t*>(&signaled),
        0, // Expected value (not signaled)
        timeout_ms);

    if (result) {
        // If signaled, load the status atomically with acquire semantics
        status_out = status.load(std::memory_order_acquire);
        signaled.store(0U, std::memory_order_release); // Reset signaled state
        return true;
    }

    // If timeout, set status to timeout
    status_out = UartStatus::ErrTimeout;
    return false;
}

void BinarySemaphore::give(UartStatus status_val) {
    // Store status with release semantics to ensure visibility to TakeWithTimeout
    status.store(status_val, std::memory_order_release);

    // Signal the semaphore with release semantics
    signaled.store(1U, std::memory_order_release);

    // Signal the waiting thread
    hm11::platform::WaitEvent::signal(
        // Cast to the expected type - this is safe because we're just monitoring the address
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<volatile std::uint32_t*>(&signaled));
}

void BinarySemaphore::give_from_isr(UartStatus status_val) {
    // Store status with release semantics to ensure visibility to TakeWithTimeout
    status.store(status_val, std::memory_order_release);

    // Signal the semaphore with release semantics
    signaled.store(1U, std::memory_order_release);

    // Signal the waiting thread from ISR context
    hm11::platform::WaitEvent::signal_from_isr(
        // Cast to the expected type - this is safe because we're just monitoring the address
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<volatile std::uint32_t*>(&signaled));
}

void BinarySemaphore::reset() {
    // Reset to non-signaled state with relaxed ordering
    // This is safe because it should only be called when no threads are waiting
    signaled.store(0U, std::memory_order_relaxed);
}

bool BinarySemaphore::is_signaled() const {
    // Check if signaled with acquire semantics to ensure visibility of changes
    return signaled.load(std::memory_order_acquire) != 0;
}

} // namespace hm11::sync