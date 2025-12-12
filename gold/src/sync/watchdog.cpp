/**
 * @file watchdog.cpp
 * @brief Implementation of Watchdog timeout manager
 *
 * This file implements the non-template methods of the Watchdog class that
 * coordinates timeout handling, event signaling, and error propagation between
 * ISR and application contexts.
 *
 * The template methods (start and await_event) are implemented in the header
 * file to allow the compiler to generate specialized versions for different
 * duration types.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <hm11/sync/watchdog.hpp>

namespace hm11::sync {

void Watchdog::stop() noexcept {
    // Check if started - if not, return early (idempotent behavior)
    // This matches Ada's pattern (drivers.adb:145-147):
    //   if not Started then
    //      return;
    //   end if;
    if (!started.load(std::memory_order_acquire)) {
        return;
    }

    // Mark as not started
    // Use release ordering to ensure all previous operations are visible
    started.store(false, std::memory_order_release);

    // Note: Hardware cleanup handled by drivers on STM32 targets:
    // - Disable DMA/UART interrupts
    // - Clear hardware status flags
    // - Pause DMA reception
    //
    // These operations must be ISR-safe since stop() can be called from
    // both application and ISR contexts (via release()).
}

void Watchdog::release(hal::UartStatus status_param) noexcept {
    // Check if Event_Occurred already to prevent double-signal
    // This matches Ada's pattern exactly (drivers.adb:65):
    //   if not Event_Occurred then
    //      Read_Status    := Status;
    //      Event_Occurred := True;
    //      Stop;
    //   end if;
    //
    // Use atomic compare_exchange to atomically check and set event_occurred.
    // Only if we successfully transition from false to true (were the first call),
    // proceed with storing status and signaling.
    bool expected = false;
    if (!event_occurred.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed)) {
        // Already signaled by a previous release() call - this is a no-op
        // Second and subsequent calls return early without side effects
        return;
    }

    // We successfully set event_occurred from false to true, so we are the first
    // (and only) release() call for this operation. Proceed with status storage.

    // Store status with memory_order_release (Decision 26)
    // This ensures all ISR buffer writes are visible before status becomes visible
    // to the application thread when it reads the status in await_event().
    //
    // Memory ordering explanation:
    // - ISR writes data to buffer: buffer[i] = received_byte;
    // - ISR stores status: status.store(Ok, memory_order_release);  [happens-before]
    // - ISR signals semaphore: semaphore.give_from_isr();
    // - Thread wakes from semaphore
    // - Thread reads status: status_var = status.load(memory_order_acquire);  [synchronizes-with
    // release]
    // - Thread reads buffer: process(buffer[i]);  [observes ISR writes]
    status.store(status_param, std::memory_order_release);

    // Signal semaphore to wake waiting thread
    // This matches Ada's Event_Occurred := True which releases the entry barrier
    semaphore.give_from_isr();

    // Cleanup hardware state
    // This matches Ada calling Stop at the end of Release (drivers.adb:68)
    stop();
}

bool Watchdog::check_deadline() const noexcept {
    // Check if current time exceeds deadline (Decision 25: ISR-side deadline polling)
    // This matches Ada's pattern (drivers.adb:282): if Clock > Until_Time then
    //
    // The ISR calls this method opportunistically when it fires (for any reason:
    // data received, idle line detected, error occurred). If the deadline has
    // expired, the ISR should call release(ErrTimeout).
    //
    // Note: We only check the deadline if an operation is started. If not started,
    // we return false (no timeout).
    if (!started.load(std::memory_order_acquire)) {
        return false;
    }

    // Compare current time against stored deadline
    // No memory ordering needed for deadline read because:
    // - It's only written by start() (application thread)
    // - It's read by check_deadline() (ISR)
    // - The started flag provides sufficient synchronization
    //   (start() stores deadline before setting started with release,
    //    check_deadline() reads started with acquire before reading deadline)
    return std::chrono::steady_clock::now() > deadline;
}

} // namespace hm11::sync
