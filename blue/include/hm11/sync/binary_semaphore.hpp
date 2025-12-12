#pragma once

/**
 * @file binary_semaphore.hpp
 * @brief ISR‑safe binary semaphore implementation.
 *
 * The semaphore provides a one‑bit event flag that can be signaled from
 * interrupt context via @c GiveFromISR() and waited on from thread context
 * via @c TakeWithTimeout().
 *
 * It uses a platform‑specific @c hm11::platform::WaitEvent abstraction to
 * perform low‑power waiting on embedded targets and a condition‑variable on
 * host builds.
 */

#include <atomic>
#include <chrono>

#include "hm11/platform/wait_event.hpp"

namespace hm11 {

/**
 * @brief Minimal binary semaphore for ISR‑to‑thread synchronization.
 *
 * The semaphore occupies a single byte (an @c std::atomic<bool>).  All
 * operations are lock‑free and ISR‑safe.
 */
class BinarySemaphore {
  public:
    /**
     * @brief Signal the semaphore from ISR context.
     *
     * Sets the internal flag with @c memory_order_release to publish any
     * preceding ISR writes, then wakes a waiting thread using the platform
     * wait‑event.
     */
    void give_from_isr() noexcept {
        flag.store(true, std::memory_order_release);
        platform::send();
    }

    /**
     * @brief Wait for the semaphore with a timeout.
     *
     * Blocks the calling thread until the flag becomes true or the timeout
     * expires.  On success the flag is cleared (store @c false with
     * @c memory_order_relaxed) and @c true is returned.
     */
    template <typename Rep, typename Period>
    bool take_with_timeout(std::chrono::duration<Rep, Period> timeout) noexcept {
        using Clock = std::chrono::steady_clock;
        const auto DEADLINE = Clock::now() + timeout;
        std::size_t iteration = 0;

        // Acquire: observe flag set by ISR.
        while (!flag.load(std::memory_order_acquire)) {
            if (Clock::now() >= DEADLINE) {
                return false; // timeout
            }
            if (iteration < 1000) {
                platform::wait();
            } else {
                platform::sleep_short(iteration - 1000);
            }
            ++iteration;
        }
        // Flag was observed, clear it for the next use.
        flag.store(false, std::memory_order_relaxed);
        return true;
    }

    /**
     * @brief Non‑blocking check if the semaphore is signaled.
     *
     * If the flag is true it is cleared and @c true is returned; otherwise
     * @c false is returned.
     */
    bool try_take() noexcept { return flag.exchange(false, std::memory_order_acquire); }

  private:
    std::atomic<bool> flag{false};
};

} // namespace hm11
