#pragma once

#include "hm11/stm32/uart_port.hpp"
#include "hm11/watchdog.hpp"

namespace hm11 {

/**
 * @brief ISR‑safe handler for UART interrupts.
 *
 * In the original Ada code this is a protected object whose `On_UART_IRQ`
 * procedure is attached to the UART interrupt vector.  The procedure checks
 * various UART status flags, clears them, stores received bytes into the buffer
 * managed by the watchdog and notifies the watchdog of errors or completion.
 *
 * This C++ translation provides a lightweight function that can be called from a
 * platform‑specific ISR.  It operates on a shared `Watchdog` instance.
 */
void irq_uart_handler(
    Watchdog& wd,
    bool is_stream,
    std::uint8_t* received_address,
    std::size_t& position,
    std::size_t maximum,
    bool& started,
    std::chrono::steady_clock::time_point until_time,
    Status& status,
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

} // namespace hm11
