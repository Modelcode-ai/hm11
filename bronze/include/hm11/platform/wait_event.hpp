/**
 * @file wait_event.hpp
 * @brief Platform-specific event waiting mechanism abstraction
 *
 * This file provides a platform-agnostic interface for efficient thread waiting and
 * event signaling, with platform-specific implementations for ARM Cortex-M (using WFE/SEV)
 * and Linux (using futex system calls for testing purposes).
 *
 * The WaitEvent abstraction isolates platform-specific code and provides a consistent
 * interface for the BinarySemaphore synchronization primitive.
 */

#ifndef HM11_PLATFORM_WAIT_EVENT_HPP
#define HM11_PLATFORM_WAIT_EVENT_HPP

#include <chrono>
#include <cstdint>

namespace hm11::platform {

/**
 * @brief Platform-agnostic interface for efficient thread waiting and signaling
 *
 * This class provides methods to wait for events and signal events across threads
 * and interrupt contexts in an efficient, platform-specific way.
 *
 * For ARM Cortex-M, this uses the WFE/SEV instructions for power-efficient waiting.
 * For Linux testing, this uses futex syscalls.
 */
class WaitEvent {
  public:
    /**
     * @brief Wait for an event with timeout
     *
     * This method efficiently waits for an event to be signaled, with a timeout.
     *
     * @param wait_addr The address of a memory location to be monitored for changes
     * @param expected_val The expected value at the wait_addr location
     * @param timeout_ms The maximum time to wait in milliseconds
     * @return true if the event was signaled, false if timeout occurred
     */
    static bool wait_with_timeout(
        const volatile std::uint32_t* wait_addr,
        std::uint32_t expected_val,
        std::chrono::milliseconds timeout_ms);

    /**
     * @brief Signal an event
     *
     * This method signals an event, waking any threads waiting on it.
     * It is safe to call from both thread and interrupt contexts.
     *
     * @param wait_addr The address of the memory location to signal changes on
     */
    static void signal(volatile std::uint32_t* wait_addr);

    /**
     * @brief Signal an event from ISR context
     *
     * This method signals an event from an ISR context, waking any threads
     * waiting on it. It is specialized for interrupt contexts and may have
     * different implementation details than the thread-context version.
     *
     * @param wait_addr The address of the memory location to signal changes on
     */
    static void signal_from_isr(volatile std::uint32_t* wait_addr);
};

} // namespace hm11::platform

#endif // HM11_PLATFORM_WAIT_EVENT_HPP