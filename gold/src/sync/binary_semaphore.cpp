/**
 * @file binary_semaphore.cpp
 * @brief Implementation of BinarySemaphore for ISR-to-thread signaling
 *
 * This file implements the non-template member functions of the BinarySemaphore
 * class. The template member function (TakeWithTimeout) is implemented in the
 * header file.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <hm11/sync/binary_semaphore.hpp>

namespace hm11::sync {

void BinarySemaphore::give_from_isr() noexcept {
    // memory_order_release: Ensures all memory operations performed by the ISR
    // before this call are visible to the thread after take_with_timeout() observes
    // the signal (Decision 23: Release-acquire for SPSC synchronization)
    //
    // Example synchronization pattern:
    //   ISR writes to buffer → store(true, release) → thread load(acquire) → thread reads buffer
    //
    // This establishes a happens-before relationship: all ISR work before this
    // store is visible to the thread after it observes available == true.
    available.store(true, std::memory_order_release);

    // Wake any thread blocked in take_with_timeout()
    // Platform-specific implementation:
    // - ARM Cortex-M: SEV instruction to wake cores in WFE
    // - Linux x86: futex FUTEX_WAKE syscall
    //
    // Note: signal() internally uses memory_order_release for its own atomic
    // operations, ensuring the wake signal is properly ordered after our store above
    wait_event.signal();
}

bool BinarySemaphore::try_take() noexcept {
    // memory_order_acquire: If we successfully exchange true for false, we need
    // to observe all memory operations from the ISR that called give_from_isr()
    // (Decision 23: Acquire ensures visibility of ISR work)
    //
    // exchange() atomically:
    // 1. Reads current value of available
    // 2. Stores false to available
    // 3. Returns the old value
    //
    // If old value was true: semaphore was signaled, we took it (return true)
    // If old value was false: semaphore was not signaled (return false)
    return available.exchange(false, std::memory_order_acquire);
}

} // namespace hm11::sync
