/**
 * @file wait_event_arm.cpp
 * @brief ARM Cortex-M implementation of WaitEvent using WFE/SEV instructions
 *
 * This file implements the WaitEvent abstraction for ARM Cortex-M platforms,
 * using the WFE (Wait For Event) and SEV (Send Event) instructions for power-efficient
 * thread synchronization.
 *
 * Only compiled when targeting ARM Cortex-M platforms.
 */

#if defined(HM11_TARGET_ARM_CORTEX_M)

#include <hm11/platform/wait_event.hpp>

// Include ARM CMSIS core headers
#include <cmsis_compiler.h>
#include <cmsis_gcc.h>

namespace hm11::platform {

bool WaitEvent::wait_with_timeout(
    const volatile std::uint32_t* wait_addr,
    std::uint32_t expected_val,
    std::chrono::milliseconds timeout_ms) {
    // Calculate deadline
    auto deadline = std::chrono::steady_clock::now() + timeout_ms;

    // Loop until signaled or timeout
    while (true) {
        // Check if the value has changed from the expected value
        // Using ldrex to ensure exclusive access to the memory location
        // LDREXW requires a non-const pointer, so we need to cast
        // This is safe because LDREXW doesn't modify the memory location
        std::uint32_t* non_const_addr = const_cast<std::uint32_t*>(wait_addr);
        if (__LDREXW(non_const_addr) != expected_val) {
            // Value changed, event occurred
            return true;
        }

        // Check if we've reached the timeout
        if (std::chrono::steady_clock::now() >= deadline) {
            return false; // Timeout occurred
        }

        // Put the processor into a low-power state until an event occurs
        // This is the ARM WFE (Wait For Event) instruction
        __WFE();

        // After the WFE instruction, the processor wakes up when an event occurs,
        // which could be an interrupt or a SEV instruction from another thread/ISR

        // No need for a memory fence here because ldrex includes memory barriers
    }
}

void WaitEvent::signal(volatile std::uint32_t* wait_addr) {
    // Ensure memory operations are completed before signaling
    __DMB();

    // Send event to wake up any cores in WFE state
    // This is the ARM SEV (Send Event) instruction
    __SEV();
}

void WaitEvent::signal_from_isr(volatile std::uint32_t* wait_addr) {
    // In ARM Cortex-M, signaling from ISR is the same as from thread context
    // because SEV can be safely used in both contexts

    // Ensure memory operations are completed before signaling
    __DMB();

    // Send event to wake up any cores in WFE state
    __SEV();
}

} // namespace hm11::platform

#endif // HM11_TARGET_ARM_CORTEX_M