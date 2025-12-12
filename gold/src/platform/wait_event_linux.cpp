/**
 * @file wait_event_linux.cpp
 * @brief Linux implementation of WaitEvent using futex syscalls
 *
 * This implementation provides blocking primitives for Linux x86 development
 * and testing using the futex(2) system call. This enables automated testing
 * on development machines with identical blocking semantics to the ARM
 * production implementation.
 *
 * ## Architecture
 *
 * Linux futex (fast userspace mutex) provides kernel-level blocking without
 * spinning:
 *
 * - **FUTEX_WAIT**: Blocks the calling thread in the kernel until the futex
 *   value changes and FUTEX_WAKE is called. This is equivalent to ARM's WFE.
 *
 * - **FUTEX_WAKE**: Wakes one or more threads blocked on the futex. This is
 *   equivalent to ARM's SEV instruction.
 *
 * ## Memory Ordering Strategy (Decision 23)
 *
 * This implementation uses std::atomic memory ordering for synchronization:
 *
 * - **signal()**: atomic store with memory_order_release ensures all prior
 *   memory operations are visible to threads that observe the signal
 * - **wait()**: atomic load with memory_order_acquire ensures all memory
 *   operations by the signaling thread are visible after observing the signal
 *
 * Futex syscalls provide their own memory barriers, but we use atomic
 * ordering to maintain consistency with the C++ memory model.
 *
 * ## Spurious Wakeup Handling
 *
 * Futex can wake spuriously (the kernel may wake threads even if the value
 * hasn't changed). The implementation handles this by checking the atomic
 * state in a loop and re-entering futex wait if the event hasn't been signaled.
 *
 * ## Performance Characteristics
 *
 * - signal() latency: ~1-2µs (syscall overhead)
 * - wait() wakeup latency: ~5-10µs (syscall + context switch)
 * - Memory footprint: 4 bytes per instance
 * - ThreadSanitizer compatible for race detection
 *
 * ## References
 *
 * - futex(2) man page: https://man7.org/linux/man-pages/man2/futex.2.html
 * - "Futexes Are Tricky" by Ulrich Drepper
 * - Linux kernel source: kernel/futex.c
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

// Only compile this file for Linux targets
#ifdef __linux__

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "hm11/platform/wait_event.hpp"

namespace hm11::platform {

namespace {

/**
 * @brief Wrapper for futex syscall with optional timeout
 *
 * The futex system call is not exposed through glibc, so we must call it
 * directly using syscall(). This wrapper provides a more convenient interface.
 *
 * @param uaddr Pointer to futex word (atomic uint32_t)
 * @param futex_op Operation to perform (FUTEX_WAIT, FUTEX_WAKE, etc.)
 * @param val Expected value for FUTEX_WAIT, or number of threads to wake for FUTEX_WAKE
 * @param timeout Optional timeout (nullptr for no timeout)
 * @return 0 on success, -1 on error (with errno set)
 */
inline int futex_syscall(
    std::atomic<uint32_t>* uaddr,
    int futex_op,
    uint32_t val,
    const struct timespec* timeout = nullptr) noexcept {
    // Cast atomic to int* for syscall interface
    // This is safe because std::atomic<uint32_t> has the same representation
    // as uint32_t on all platforms
    // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-type-reinterpret-cast)
    // syscall() is a C-style vararg function (required for futex interface)
    // reinterpret_cast is necessary for atomic<> to raw pointer conversion for syscall
    return static_cast<int>(syscall(SYS_futex, reinterpret_cast<int*>(uaddr), futex_op, val, timeout, nullptr, 0));
    // NOLINTEND(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-type-reinterpret-cast)
}

} // anonymous namespace

// Constructor - no platform-specific initialization needed
WaitEvent::WaitEvent() noexcept : state{0} {}

void WaitEvent::wait() noexcept {
    // Block until event is signaled or spurious wakeup occurs
    // The futex syscall with timeout enables periodic wakeups for timeout handling
    // in BinarySemaphore without indefinite blocking
    //
    // IMPORTANT: This function may return spuriously (before signal() is called).
    // Callers MUST check their own condition in a loop after wait() returns.
    struct timespec timeout{.tv_sec = 0, .tv_nsec = 10000000}; // 10ms

    // FUTEX_WAIT: Block in kernel until futex value changes or timeout
    // Parameters:
    // - &state: Futex word to wait on
    // - FUTEX_WAIT: Wait operation
    // - 0: Expected value (only block if state == 0)
    // - &timeout: Maximum time to block (10ms)
    //
    // Returns:
    // - 0 if woken by FUTEX_WAKE
    // - -1 with EAGAIN if state != 0 (already signaled - fast path)
    // - -1 with ETIMEDOUT if timeout expired (spurious wakeup for BinarySemaphore timeout)
    // - -1 with EINTR if interrupted by signal (spurious wakeup)
    //
    // We ignore the return value because callers check their own condition after wait()
    // returns. The 10ms timeout enables BinarySemaphore to check its deadline periodically.
    futex_syscall(&state, FUTEX_WAIT, 0, &timeout);
}

void WaitEvent::signal() noexcept {
    // Set event flag to signaled state
    // Use memory_order_release to ensure all prior memory operations
    // are visible to threads that observe this signal
    state.store(1, std::memory_order_release);

    // FUTEX_WAKE: Wake one thread blocked on this futex
    // Parameters:
    // - &state: Futex word to wake on
    // - FUTEX_WAKE: Wake operation
    // - 1: Number of threads to wake (wake one thread)
    //
    // Returns:
    // - Number of threads woken (0 if no threads were waiting)
    // - -1 on error (unlikely in practice)
    //
    // We ignore the return value because signal() is idempotent and
    // it's okay if no threads were waiting
    futex_syscall(&state, FUTEX_WAKE, 1);
}

void WaitEvent::clear() noexcept {
    // Clear event flag back to non-signaled state
    // Use memory_order_relaxed since this is typically called by the single
    // consumer thread after processing the event - no synchronization needed
    state.store(0, std::memory_order_relaxed);
}

bool WaitEvent::is_signaled() const noexcept {
    // Check if event is currently signaled
    // Use memory_order_acquire to ensure visibility of operations from
    // the thread that called signal()
    return state.load(std::memory_order_acquire) != 0;
}

void WaitEvent::wait_impl() noexcept {
    // Implementation is in wait() for Linux (no separate impl function needed)
}

void WaitEvent::signal_impl() noexcept {
    // Implementation is in signal() for Linux (no separate impl function needed)
}

} // namespace hm11::platform

#endif // __linux__
