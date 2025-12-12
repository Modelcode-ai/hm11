#pragma once

/**
 * @file wait_event.hpp
 * @brief Platform abstraction for low‑power waiting and ISR‑safe signaling.
 *
 * The implementation provides two back‑ends:
 *   * Embedded targets (TARGET_EMBEDDED defined): uses ARM WFE/SEV intrinsics.
 *   * Host / testing targets: uses a condition variable.
 *
 * Only the minimal interface required by BinarySemaphore is exposed:
 *   * static void Wait();   // block until signal
 *   * static void Send();   // wake waiting thread(s)
 *   * static void SleepShort(std::size_t iteration); // low‑cost back‑off for timeout handling
 */

#if defined(TARGET_EMBEDDED)
// Embedded implementation – rely on CMSIS intrinsics.
// The actual intrinsic headers are platform‑specific; we include <arm_acle.h> if available.
#if defined(__ARM_ARCH) && (__ARM_ARCH >= 7)
#include <cmsis_gcc.h>
#endif

namespace hm11::platform {
inline void Wait() {
    // Wait For Event – low‑power sleep until SEV is executed.
    __WFE();
}

inline void Send() {
    // Send Event – wakes any core waiting on __WFE.
    __SEV();
}

inline void SleepShort(std::size_t /*iteration*/) {
    // On an embedded target we typically rely on a periodic systick to break out.
    // For simplicity we just re‑enter Wait(); the ISR or systick will wake us.
    __WFE();
}
} // namespace hm11::platform
#else
// Host implementation – use std::condition_variable.
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace hm11::platform {
// These static objects are intentionally leaky; they exist for the program lifetime.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::condition_variable cv;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::mutex mtx;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::atomic<bool> signaled{false};

inline void wait() {
    std::unique_lock<std::mutex> lock(mtx);
    // Spurious wake‑ups are possible; loop until signaled_ becomes true.
    cv.wait(lock, [] { return signaled.load(std::memory_order_acquire); });
    // Do not clear the flag here – BinarySemaphore will handle it.
}

inline void send() {
    signaled.store(true, std::memory_order_release);
    cv.notify_one();
}

inline void sleep_short(std::size_t iteration) {
    if (iteration < 10) {
        // Yield for the first few iterations – cheap.
        std::this_thread::yield();
    } else {
        // Then sleep a millisecond to avoid busy‑wait.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
} // namespace hm11::platform
#endif // TARGET_EMBEDDED
