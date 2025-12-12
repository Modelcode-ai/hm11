#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/sync/binary_semaphore.hpp"

namespace hm11 {

/**
 * @brief Simple Watchdog implementation mimicking Ada protected Watchdog.
 *
 * It uses a BinarySemaphore for ISR‑to‑thread synchronization and stores the
 * status reported by the ISR. The timeout is expressed as a std::chrono::milliseconds
 * duration.
 */
class Watchdog {
  public:
    Watchdog() = default;
    ~Watchdog() = default;

    // Non‑copyable / non‑movable to match Ada protected object semantics.
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /**
     * @brief Start the watchdog.
     * @param received Pointer to the receive buffer.
     * @param length   Length of the buffer.
     * @param timeout  Timeout duration.
     * @param is_stream Whether the operation is a stream.
     */
    void
    start(void* received_in, std::size_t length_in, std::chrono::milliseconds timeout, bool is_stream_in) noexcept {
        if (started) {
            return;
        }
        received = static_cast<std::uint8_t*>(received_in);
        length = length_in;
        position = 0;
        deadline = std::chrono::steady_clock::now() + timeout;
        started = true;
        is_stream = is_stream_in;
        // Clear buffer if not a stream
        if (!is_stream && received != nullptr) {
            std::fill_n(received, length, 0);
        }
    }

    /** @brief Stop the watchdog. */
    void stop() noexcept { started = false; }

    /**
     * @brief Await an event from ISR.
     * @param status Output status set by ISR.
     * @return true if an event occurred before timeout, false otherwise.
     */
    bool await_event(Status& status) noexcept {
        if (!started) {
            return false;
        }
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        bool signaled = event_semaphore.take_with_timeout(remaining);
        if (signaled) {
            status = isr_status;
            return true;
        }
        return false;
    }

    /** @brief Called from ISR to release waiting thread with a status. */
    void release(Status status) noexcept {
        if (!event_occurred) {
            isr_status = status;
            event_occurred = true;
            event_semaphore.give_from_isr();
        }
    }

    /**
     * @brief Retrieve read state for Last_Read_Handler.
     * @param closed Output flag indicating if watchdog stopped.
     * @param zero  Output position (0‑based index of next byte).
     */
    void readed(bool& closed, std::size_t& zero) const noexcept {
        closed = !started;
        zero = position;
    }

    /**
     * @brief Store a received byte; used by UART IRQ handler.
     * @param byte Received byte.
     */
    void store_byte(std::uint8_t byte) noexcept {
        if (!started || received == nullptr) {
            return;
        }
        if (position < length) {
            received[position++] = byte;
        } else {
            // wrap for stream mode
            position = 0;
            received[position] = byte;
        }
        // If non‑stream and buffer filled, signal OK.
        if (!is_stream && position == length) {
            release(Status::Ok);
        }
    }

  private:
    hm11::BinarySemaphore event_semaphore; // ISR‑to‑thread event flag
    std::atomic<bool> started{false};
    std::atomic<bool> event_occurred{false};
    Status isr_status{Status::Ok};
    std::uint8_t* received{nullptr};
    std::size_t length{0};
    std::size_t position{0};
    bool is_stream{false};
    std::chrono::steady_clock::time_point deadline;
};

} // namespace hm11
