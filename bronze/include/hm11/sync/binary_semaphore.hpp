/**
 * @file binary_semaphore.hpp
 * @brief ISR-safe binary semaphore implementation
 * @details Implements Decision 19 regarding Protected Objects Translation
 *
 * This file is part of the HM11 driver C++ migration project.
 * Provides an ISR-to-thread signaling primitive equivalent to Ada protected objects.
 */

#ifndef HM11_SYNC_BINARY_SEMAPHORE_HPP
#define HM11_SYNC_BINARY_SEMAPHORE_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <hm11/platform/wait_event.hpp>

namespace hm11::sync {

/**
 * @brief Error status codes
 *
 * These status codes are used to communicate operation results between
 * ISR and thread contexts, matching the Ada UART_Status enum.
 */
enum class UartStatus : std::uint8_t {
    Ok,              ///< Operation completed successfully
    ErrError,        ///< General error occurred
    ErrTimeout,      ///< Operation timed out
    ErrInvalidState, ///< Operation attempted in invalid state
    ErrNotSupported  ///< Operation not supported
};

/**
 * @brief Binary semaphore for ISR-to-thread signaling
 *
 * Custom implementation that provides ISR-safe signaling with timeout support.
 * Uses WFE/SEV instructions on ARM Cortex-M for low power consumption.
 * Uses futex syscalls on Linux for testing purposes.
 *
 * This class implements the Rule of 5 with deleted copy/move operations
 * to ensure exclusive ownership semantics, matching Ada's limited private pattern.
 */
class BinarySemaphore {
  public:
    /**
     * @brief Construct a binary semaphore
     * @param initial_state Initial state of the semaphore (false = not signaled)
     */
    BinarySemaphore(bool initial_state = false);

    /**
     * @brief Destroy the semaphore
     *
     * Ensures proper cleanup of any resources.
     */
    ~BinarySemaphore();

    // Disable copying and moving
    BinarySemaphore(const BinarySemaphore&) = delete;
    BinarySemaphore& operator=(const BinarySemaphore&) = delete;
    BinarySemaphore(BinarySemaphore&&) = delete;
    BinarySemaphore& operator=(BinarySemaphore&&) = delete;

    /**
     * @brief Wait for the semaphore to be signaled with a timeout
     *
     * This method blocks the calling thread until either the semaphore
     * is signaled or the timeout expires. It is similar to Ada's
     * Await_Event entry.
     *
     * @param status Output parameter to receive the operation status
     * @param timeout_ms Maximum time to wait in milliseconds
     * @return true if the semaphore was signaled, false if timeout occurred
     */
    bool take_with_timeout(UartStatus& status, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Signal the semaphore from a thread context
     *
     * This method signals the semaphore, potentially waking a thread
     * waiting in take_with_timeout. It is safe to call from thread context.
     *
     * @param status The status to pass to the waiting thread
     */
    void give(UartStatus status = UartStatus::Ok);

    /**
     * @brief Signal the semaphore from an ISR context
     *
     * Thread-safe and can be called from interrupt context.
     * Uses memory_order_release for synchronization. This is similar
     * to Ada's Release procedure.
     *
     * @param status The status to pass to the waiting thread
     */
    void give_from_isr(UartStatus status = UartStatus::Ok);

    /**
     * @brief Reset the semaphore to non-signaled state
     *
     * This method resets the semaphore to non-signaled state. It should
     * only be called when no threads are waiting on the semaphore.
     */
    void reset();

    /**
     * @brief Check if the semaphore is currently signaled
     *
     * @return true if the semaphore is signaled, false otherwise
     */
    bool is_signaled() const;

  private:
    // Use uint32_t for alignment requirements and to match platform primitives
    std::atomic<std::uint32_t> signaled{0};

    // Use atomic for status to ensure proper visibility between threads and ISRs
    std::atomic<UartStatus> status{UartStatus::Ok};

    // Absolute deadline timestamp for timeout handling
    std::chrono::steady_clock::time_point deadline;
};

} // namespace hm11::sync

#endif // HM11_SYNC_BINARY_SEMAPHORE_HPP