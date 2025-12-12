/**
 * @file wait_event_linux.cpp
 * @brief Linux implementation of WaitEvent using futex syscalls
 *
 * This file implements the WaitEvent abstraction for Linux platforms,
 * using futex (fast userspace mutex) system calls for thread synchronization.
 * This implementation is primarily used for testing on development machines.
 *
 * Only compiled when targeting Linux platforms.
 */

#if defined(HM11_TARGET_LINUX)

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <hm11/platform/wait_event.hpp>

namespace {

// Internal helper function to get a non-const, non-volatile pointer for syscall
// This function is only used internally to make the implementation work with Linux syscalls
// We need to suppress the linting warnings as we can't change the Linux futex API
std::uint32_t* get_futex_addr(const volatile std::uint32_t* futex_addr) {
    // This is needed to work with Linux futex API which requires non-const, non-volatile pointers
    // First remove volatile qualifier, then remove const qualifier
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast,modernize-use-auto)
    const std::uint32_t* non_volatile_addr = const_cast<const std::uint32_t*>(futex_addr);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<std::uint32_t*>(non_volatile_addr);
}

// Helper function to perform futex wait operation
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
long futex_wait(const volatile std::uint32_t* futex_addr, std::uint32_t expected_val, const struct timespec* timeout) {
    std::uint32_t* addr = get_futex_addr(futex_addr);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected_val, timeout, nullptr, 0);
}

// Helper function to perform futex wake operation
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
long futex_wake(const volatile std::uint32_t* futex_addr, int num_to_wake) {
    std::uint32_t* addr = get_futex_addr(futex_addr);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, num_to_wake, nullptr, nullptr, 0);
}

// Convert milliseconds to timespec
struct timespec ms_to_timespec(std::chrono::milliseconds ms) {
    // Use designated initializers
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ts.tv_sec = sec.count();
    ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(ms - sec).count();
    return ts;
}
} // anonymous namespace

namespace hm11::platform {

bool WaitEvent::wait_with_timeout(
    const volatile std::uint32_t* wait_addr,
    std::uint32_t expected_val,
    std::chrono::milliseconds timeout_ms) {
    // First, check if the value is already not the expected value
    // This avoids unnecessary syscall if the semaphore is already signaled
    if (*wait_addr != expected_val) {
        return true; // Already signaled
    }

    // Convert relative timeout to timespec
    struct timespec ts = ms_to_timespec(timeout_ms);

    // Wait using futex, which will atomically check if the value is still expected_val
    // and put the thread to sleep if it is
    // Discard the return value - we'll check the actual state after the call
    long result = futex_wait(wait_addr, expected_val, &ts);
    (void)result; // Suppress unused variable warning

    // Check if we were signaled or timed out
    // futex_wait returns 0 if we were woken up by futex_wake
    // returns -1 with errno = ETIMEDOUT if timeout occurred
    // returns -1 with errno = EAGAIN if value changed before we went to sleep

    // Regardless of how we exited the futex_wait, we need to check the value again
    return *wait_addr != expected_val;
}

void WaitEvent::signal(volatile std::uint32_t* wait_addr) {
    // Wake up one thread waiting on this address
    (void)futex_wake(wait_addr, 1);
}

void WaitEvent::signal_from_isr(volatile std::uint32_t* wait_addr) {
    // In Linux testing, signaling from "ISR" is the same as from thread context
    // In real ISR context, we would need to handle differences between ISR and thread context
    signal(wait_addr);
}

} // namespace hm11::platform

#endif // HM11_TARGET_LINUX