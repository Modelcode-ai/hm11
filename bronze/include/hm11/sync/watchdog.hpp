/**
 * @file watchdog.hpp
 * @brief Timeout handling mechanism for asynchronous operations
 * @details Implements Decision 25 and 26 regarding timeout handling and ISR error propagation
 *
 * This file is part of the HM11 driver C++ migration project.
 * Provides an ISR-to-thread timeout and error propagation mechanism equivalent
 * to Ada's Watchdog protected object.
 */

#ifndef HM11_SYNC_WATCHDOG_HPP
#define HM11_SYNC_WATCHDOG_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/sync/binary_semaphore.hpp>
#include <system_error>

namespace hm11::sync {

/**
 * @brief Watchdog class for timeout handling in asynchronous operations
 *
 * This class implements ISR-side deadline polling for timeouts and atomic
 * status flags for error propagation from ISR to thread context. It matches
 * the functionality of Ada's Watchdog protected object.
 *
 * The Watchdog follows the same ownership semantics as other synchronization primitives
 * in this library, with deleted copy and move operations to ensure exclusive ownership.
 */
class Watchdog {
  public:
    /**
     * @brief Constructs a new Watchdog instance
     *
     * Creates a Watchdog in stopped state.
     */
    Watchdog();

    /**
     * @brief Destructor
     *
     * Ensures any waiting thread is unblocked before destruction.
     */
    ~Watchdog();

    // Disable copying and moving
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /**
     * @brief Start a watchdog-monitored operation
     *
     * This method stores the timeout deadline and prepares the watchdog for
     * monitoring an operation. It is similar to Ada's Start procedure.
     *
     * @param buffer Pointer to the receiving buffer
     * @param length Length of the receiving buffer
     * @param timeout_ms Timeout duration in milliseconds
     * @return true if successfully started, false if already started
     */
    bool start(void* buffer, std::size_t length, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Stop the watchdog-monitored operation
     *
     * This method stops the watchdog monitoring and resets its state.
     * It is similar to Ada's Stop procedure.
     */
    void stop();

    /**
     * @brief Wait for the operation to complete
     *
     * This method blocks the calling thread until either the operation completes
     * or the timeout expires. It is similar to Ada's Await_Event entry.
     *
     * @param status Output parameter to receive the operation status
     * @return true if operation completed, false if timeout occurred
     */
    bool await_event(UartStatus& status);

    /**
     * @brief Signal operation completion from ISR context
     *
     * This method signals operation completion from an ISR context, waking any thread
     * waiting in await_event. It is similar to Ada's Release procedure.
     *
     * @param status The status to pass to the waiting thread
     */
    void release(UartStatus status);

    /**
     * @brief Check if the deadline has been reached
     *
     * This method checks if the current time has exceeded the deadline.
     * It should be called from ISR context periodically.
     *
     * @return true if deadline exceeded, false otherwise
     */
    bool is_deadline_exceeded() const;

    /**
     * @brief Get the current buffer position
     *
     * This method returns the current position in the buffer, used for
     * streaming operations. It is similar to Ada's Readed procedure.
     *
     * @param is_stopped Output parameter to indicate if the watchdog is stopped
     * @param pos_out Output parameter to receive the current position
     */
    void get_position(bool& is_stopped, std::size_t& pos_out) const;

    /**
     * @brief Set the current buffer position
     *
     * This method sets the current position in the buffer, used for
     * streaming operations.
     *
     * @param pos The new position
     */
    void set_position(std::size_t pos);

    /**
     * @brief Check if the watchdog is currently started
     *
     * @return true if watchdog is started, false otherwise
     */
    bool is_started() const;

  private:
    // State variables
    std::atomic<bool> started{false};     ///< Flag indicating if watchdog is started
    std::atomic<std::size_t> position{1}; ///< Current position in buffer (1-based like Ada)

    // Buffer information
    void* buffer{nullptr};        ///< Pointer to the buffer being monitored
    std::size_t buffer_length{0}; ///< Length of the buffer being monitored

    // Deadline for timeout handling
    std::atomic<std::chrono::steady_clock::time_point> deadline; ///< Deadline timestamp

    // Binary semaphore for thread synchronization
    BinarySemaphore semaphore;
};

} // namespace hm11::sync

#endif // HM11_SYNC_WATCHDOG_HPP