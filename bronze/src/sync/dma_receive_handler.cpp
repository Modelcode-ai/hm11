/**
 * @file dma_receive_handler.cpp
 * @brief Implementation of DMA receive handler for HM11 driver
 *
 * This file implements the DMA receive handler function, providing
 * DMA-based reception for UART peripherals with timeout handling.
 */

#include <hm11/platform/stm32/dma_receive_handler.hpp>
#include <hm11/platform/stm32/uart.hpp>
#include <hm11/platform/stm32/watchdog.hpp>
#include <hm11/sync/dma_receive_handler.hpp>
#include <memory>

namespace hm11::sync {

// Global DMA receive handler for reference
static std::unique_ptr<platform::stm32::DmaReceiveHandler> g_dma_handler;

// Global watchdog for timeout handling
static std::unique_ptr<platform::stm32::Stm32Watchdog> g_watchdog;

void dma_receive_handler(
    hal::UARTPort& port,
    void* received,
    std::size_t length,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms,
    bool as_stream) {
    // Check if port is a STM32UART
    auto* stm32_uart = dynamic_cast<platform::stm32::STM32UART*>(&port);
    if (!stm32_uart) {
        // Not a STM32UART, can't use DMA
        status = hal::UartStatus::ErrNotSupported;
        return;
    }

    // Get UART peripheral
    USART_TypeDef* uart_peripheral = stm32_uart->get_peripheral();

    // Create DMA stream if needed
    platform::stm32::device::DMAIndex dma_index = platform::stm32::device::DMAIndex::DMA1;
    platform::stm32::device::DMAStream stream = platform::stm32::device::DMAStream::Stream2;
    platform::stm32::device::DMAChannel channel =
        platform::stm32::get_dma_channel_for_peripheral(uart_peripheral, dma_index, stream);

    static platform::stm32::DMAStream dma_stream(dma_index, stream, channel);

    // Create handler if needed
    if (!g_dma_handler) {
        g_dma_handler = std::make_unique<platform::stm32::DmaReceiveHandler>(dma_stream, uart_peripheral);
    }

    // Create watchdog if needed
    if (!g_watchdog) {
        g_watchdog = std::make_unique<platform::stm32::Stm32Watchdog>();
    }

    // Start reception
    bool success = false;
    if (as_stream) {
        success = g_dma_handler->start_stream(received, length, timeout_ms, *g_watchdog);
    } else {
        success = g_dma_handler->start_normal(received, length, timeout_ms, *g_watchdog);
    }

    if (!success) {
        status = hal::UartStatus::ErrBusy;
        return;
    }

    // Wait for completion
    UartStatus watchdog_status;
    g_watchdog->await_event(watchdog_status);

    // Convert status
    switch (watchdog_status) {
        case UartStatus::Ok:
            status = hal::UartStatus::Ok;
            break;
        case UartStatus::ErrTimeout:
            status = hal::UartStatus::ErrTimeout;
            break;
        case UartStatus::ErrOverflow:
            status = hal::UartStatus::ErrOverflow;
            break;
        default:
            status = hal::UartStatus::ErrError;
            break;
    }

    // Stop reception if not in stream mode or on error
    if (!as_stream || status != hal::UartStatus::Ok) {
        g_dma_handler->stop();
    }
}

void last_read_handler(bool& is_stopped, std::size_t& position) {
    // Check if watchdog exists
    if (!g_watchdog) {
        is_stopped = true;
        position = 1; // 1-based index like Ada
        return;
    }

    // Get position from watchdog
    g_watchdog->get_position(is_stopped, position);
}

} // namespace hm11::sync