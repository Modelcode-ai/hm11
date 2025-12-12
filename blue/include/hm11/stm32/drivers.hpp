#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/watchdog.hpp"

namespace hm11 {

/** Initialize UART hardware (stub for simulation). */
void init_uart();

/** Initialize DMA for UART reception (stub for simulation). */
void initialize_dma();

/** DMA receive handler translation from Ada. */
void dma_receive_handler(
    UARTPort& uart,
    void* received,
    std::size_t length,
    Status& status,
    unsigned int timeout = 1000,
    bool as_stream = false);

/** IRQ UART handler translation from Ada. */
void irq_uart_handler(
    Watchdog& wd,
    bool is_stream,
    std::uint8_t* received_address,
    std::size_t& position,
    std::size_t maximum,
    bool& started,
    std::chrono::steady_clock::time_point until_time,
    Status& status,
    // status flags (simulated)
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
    bool error_flag);

/** IRQ DMA handler translation from Ada. */
void irq_dma_handler(
    Watchdog& wd,
    bool transfer_error,
    bool fifo_error,
    bool direct_mode_error,
    bool transfer_complete);

/** Last read handler translation from Ada. */
void last_read_handler(bool& closed, std::size_t& zero);

} // namespace hm11
