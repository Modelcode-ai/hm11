#include <chrono>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/sync/watchdog.hpp"

namespace hm11 {

void init_uart() {
    // Stub implementation – in real hardware this would configure GPIO and UART registers.
    // For simulation/testing we do nothing.
}

void initialize_dma() {
    // Stub implementation – in real hardware this would configure DMA controller.
    // For simulation/testing we do nothing.
}

void dma_receive_handler(
    UARTPort& uart [[maybe_unused]],
    void* received,
    std::size_t length,
    Status& status,
    unsigned int timeout,
    bool as_stream) {
    // In the Ada version this uses a global Watchdog instance. We create a temporary one.
    static Watchdog wd; // static to preserve state across calls as Ada does.
    // Start the watchdog with the buffer.
    wd.start(received, length, std::chrono::milliseconds(timeout), as_stream);
    if (as_stream) {
        // If stream mode, we stop immediately after start (mirrors Ada behavior).
        wd.stop();
        status = Status::Ok;
        return;
    }
    // Wait for event.
    if (!wd.await_event(status)) {
        // Timeout – stop watchdog and set error.
        wd.stop();
        status = Status::Error;
        return;
    }
    // Event occurred – stop watchdog.
    wd.stop();
}

void irq_uart_handler(
    Watchdog& wd,
    bool is_stream,
    std::uint8_t* received_address [[maybe_unused]],
    std::size_t& position,
    std::size_t maximum,
    bool& started [[maybe_unused]],
    std::chrono::steady_clock::time_point until_time,
    Status& status [[maybe_unused]],
    // status flags (simulated inputs)
    bool idle_line_detected,
    bool idle_line_interrupt_enabled,
    bool read_data_not_empty,
    bool received_data_not_empty,
    bool parity_error,
    bool framing_error,
    bool noise_error,
    bool overrun_error,
    bool line_break_detection,
    bool clear_to_send,
    bool error_flag) {
    // This function mimics the Ada IRQ_UART_Handler.On_UART_IRQ logic.
    // For each flag, if set and interrupt enabled, clear and signal error.
    if (parity_error) {
        wd.release(Status::Error);
        return;
    }
    if (framing_error || noise_error || overrun_error || line_break_detection || clear_to_send || error_flag) {
        wd.release(Status::Error);
        return;
    }
    if (idle_line_detected && idle_line_interrupt_enabled) {
        if (std::chrono::steady_clock::now() > until_time) {
            wd.release(Status::Ok);
        }
        // In either case we consider the idle flag handled.
        return;
    }
    if (read_data_not_empty && received_data_not_empty) {
        // Simulate reading a byte from UART into buffer.
        // Here we just store a dummy value (e.g., 0x00).
        std::uint8_t byte = 0x00;
        wd.store_byte(byte);
        if (!is_stream) {
            if (position == maximum) {
                wd.release(Status::Ok);
            } else {
                ++position;
            }
        } else {
            if (position < maximum) {
                ++position;
            } else {
                position = 0;
            }
        }
    }
}

void irq_dma_handler(
    Watchdog& wd,
    bool transfer_error,
    bool fifo_error,
    bool direct_mode_error,
    bool transfer_complete) {
    // Mimic Ada IRQ_DMA_Handler.On_DMA_IRQ checks.
    if (transfer_error || fifo_error || direct_mode_error) {
        wd.release(Status::Error);
        return;
    }
    if (transfer_complete) {
        wd.release(Status::Ok);
    }
}

} // namespace hm11
