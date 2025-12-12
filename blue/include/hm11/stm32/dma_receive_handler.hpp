#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/watchdog.hpp"

namespace hm11 {

/**
 * @brief DMA Receive Handler translation from Ada.
 *
 * Simulates the Ada Drivers.DMA_Receive_Handler procedure. It starts the
 * watchdog, optionally waits for completion, and returns the status.
 */
void dma_receive_handler(
    UARTPort& uart,
    void* received,
    std::size_t length,
    Status& status,
    unsigned int timeout = 1000,
    bool as_stream = false);

} // namespace hm11
