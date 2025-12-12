/**
 * @file watchdog.cpp
 * @brief Implementation of the Watchdog class
 *
 * This file implements the Watchdog class, providing timeout handling and
 * error propagation for asynchronous operations in the HM11 driver.
 * It follows Decisions 25 and 26 from the modernization specs.
 */

#include <hm11/sync/watchdog.hpp>

namespace hm11::sync {

Watchdog::Watchdog() : semaphore(false) {
    // Initialize deadline to "now" as a reasonable default
    deadline.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
}

Watchdog::~Watchdog() {
    // Ensure any waiting thread is unblocked before destruction
    if (is_started()) {
        release(UartStatus::ErrInvalidState);
    }
}

bool Watchdog::start(void* buf, std::size_t length, std::chrono::milliseconds timeout_ms) {
    // Check if already started - prevent re-entrancy
    bool expected_started = false;
    if (!started
             .compare_exchange_strong(expected_started, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        // Already started, so don't start again
        return false;
    }

    // Store buffer information
    buffer = buf;
    buffer_length = length;

    // Reset position to 1 (1-based indexing to match Ada)
    position.store(1, std::memory_order_relaxed);

    // Calculate deadline as current time + timeout
    auto now = std::chrono::steady_clock::now();
    auto deadline_time = now + timeout_ms;
    deadline.store(deadline_time, std::memory_order_release);

    // Reset the semaphore to non-signaled state
    semaphore.reset();

    return true;
}

void Watchdog::stop() {
    // Check if already stopped
    bool expected_started = true;
    if (!started
             .compare_exchange_strong(expected_started, false, std::memory_order_acquire, std::memory_order_relaxed)) {
        // Already stopped, nothing to do
        return;
    }

    // Reset buffer info
    buffer = nullptr;
    buffer_length = 0;
}

bool Watchdog::await_event(UartStatus& status) {
    // Check if watchdog is started
    if (!is_started()) {
        status = UartStatus::ErrInvalidState;
        return false;
    }

    // Wait on the semaphore with infinite timeout (the Watchdog itself handles the timeout)
    bool result = semaphore.take_with_timeout(status, std::chrono::milliseconds(0xFFFFFFFF));

    // The release() method is supposed to call stop(), but we need to ensure
    // the watchdog is stopped even if the semaphore was signaled directly
    // without calling release() or if release() was called from another thread.
    if (result) {
        stop();
    }

    return result;
}

void Watchdog::release(UartStatus status) {
    // Signal completion with the provided status
    semaphore.give_from_isr(status);

    // Stop the watchdog
    stop();
}

bool Watchdog::is_deadline_exceeded() const {
    // Only check deadline if watchdog is started
    if (!is_started()) {
        return false;
    }

    // Check if current time exceeds deadline
    auto now = std::chrono::steady_clock::now();
    auto deadline_time = deadline.load(std::memory_order_acquire);

    return now > deadline_time;
}

void Watchdog::get_position(bool& is_stopped, std::size_t& pos_out) const {
    is_stopped = !is_started();
    pos_out = position.load(std::memory_order_acquire);
}

void Watchdog::set_position(std::size_t pos) {
    position.store(pos, std::memory_order_release);
}

bool Watchdog::is_started() const {
    return started.load(std::memory_order_acquire);
}

} // namespace hm11::sync