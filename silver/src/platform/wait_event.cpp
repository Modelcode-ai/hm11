// HM11 Platform Abstraction Layer - Wait/Signal Implementation
// Platform-specific implementation of wait/signal operations

#include "hm11/platform/wait_event.hpp"

#include <thread>

#if defined(__linux__)
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#elif defined(__arm__) || defined(__aarch64__)
// ARM CMSIS headers would be included here for bare-metal targets
// For now, we'll use compiler intrinsics and inline assembly
#endif

namespace hm11::platform {

void WaitEvent::SignalFromISR() noexcept {
    // Store with release ordering to ensure all preceding writes are visible
    // before signaling. This is critical for ISR-to-thread data communication.
    signaled_.store(true, std::memory_order_release);

#if defined(__arm__) || defined(__aarch64__)
    PlatformSignal();
#elif defined(__linux__)
    PlatformSignal();
#else
    // Fallback: No platform-specific signal mechanism
    // Thread will wake up through polling in WaitWithTimeout
#endif
}

bool WaitEvent::WaitWithTimeout(std::chrono::milliseconds timeout) noexcept {
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!signaled_.load(std::memory_order_acquire)) {
        // Check timeout before waiting
        if (std::chrono::steady_clock::now() >= deadline) {
            return false; // Timeout expired
        }

#if defined(__arm__) || defined(__aarch64__)
        PlatformWait();
#elif defined(__linux__)
        // Use futex wait with remaining timeout
        auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining.count() <= 0) {
            return false;
        }
        PlatformWaitWithTimeout(std::chrono::duration_cast<std::chrono::milliseconds>(remaining));
#else
        // Fallback: yield and check again (power inefficient but portable)
        std::this_thread::yield();
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(100));
#endif
    }

    return true; // Successfully signaled
}

void WaitEvent::Clear() noexcept {
    // Use relaxed ordering as no synchronization is needed for clearing
    // (single consumer pattern)
    signaled_.store(false, std::memory_order_relaxed);
}

bool WaitEvent::IsSignaled() const noexcept {
    // Use acquire ordering for consistency with WaitWithTimeout
    return signaled_.load(std::memory_order_acquire);
}

bool WaitEvent::TakeSignal() noexcept {
    // Atomically check and clear the signal for binary semaphore behavior
    // This ensures only one waiter can successfully "take" the signal
    return signaled_.exchange(false, std::memory_order_acq_rel);
}

#if defined(__arm__) || defined(__aarch64__)

void WaitEvent::PlatformWait() noexcept {
    // ARM WFE (Wait for Event) instruction
    // Enters low-power state until SEV, interrupt, or debug event
    // ~10µs wake latency, µA-level power consumption
#ifdef __GNUC__
    __asm__ volatile("wfe" ::: "memory");
#else
    // For other compilers, we'd use CMSIS intrinsics: __WFE()
    // Fallback to yield for now during development
    std::this_thread::yield();
#endif
}

void WaitEvent::PlatformSignal() noexcept {
    // ARM SEV (Send Event) instruction
    // Wakes all processors waiting on WFE
    // ~2-3 cycle overhead
#ifdef __GNUC__
    __asm__ volatile("sev" ::: "memory");
#else
    // For other compilers, we'd use CMSIS intrinsics: __SEV()
    // No fallback needed - signaling through atomic store is sufficient
#endif
}

#elif defined(__linux__)

void WaitEvent::PlatformWaitWithTimeout(std::chrono::milliseconds timeout) noexcept {
    // Linux futex system call for efficient blocking
    // Only used for testing on development machines
    struct timespec ts;
    ts.tv_sec = timeout.count() / 1000;
    ts.tv_nsec = (timeout.count() % 1000) * 1000000;

    // FUTEX_WAIT: block if futex_word_ == 0
    syscall(SYS_futex, &futex_word_, FUTEX_WAIT, 0, &ts, nullptr, 0);
    // Return value not checked - timeout/signal handling via atomic variable
}

void WaitEvent::PlatformSignal() noexcept {
    // Set futex word and wake waiting threads
    futex_word_.store(1, std::memory_order_relaxed);

    // FUTEX_WAKE: wake up to 1 thread waiting on futex_word_
    syscall(SYS_futex, &futex_word_, FUTEX_WAKE, 1, nullptr, nullptr, 0);

    // Reset for next wait cycle
    futex_word_.store(0, std::memory_order_relaxed);
}

#endif

} // namespace hm11::platform