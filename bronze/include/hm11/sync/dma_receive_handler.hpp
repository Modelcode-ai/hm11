/**
 * @file dma_receive_handler.hpp
 * @brief DMA receive handler interface for HM11 driver
 * @details Implements Decision 12 regarding circular buffer synchronization
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides the DMA receive handler function for data reception via DMA.
 */

#ifndef HM11_SYNC_DMA_RECEIVE_HANDLER_HPP
#define HM11_SYNC_DMA_RECEIVE_HANDLER_HPP

#include <chrono>
#include <cstddef>
#include <hm11/hal/uart.hpp>
#include <hm11/sync/uart_status.hpp>

namespace hm11::sync {

/**
 * @brief DMA receive handler function type
 *
 * This type defines the signature for the DMA receive handler function,
 * which is used to receive data via DMA with timeout handling.
 * It matches the Ada DMA_Receive_Handler procedure.
 */
using DmaReceiveHandlerFn = void (*)(
    hal::UARTPort& port,
    void* received,
    std::size_t length,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms,
    bool as_stream);

/**
 * @brief DMA receive handler implementation
 *
 * This function provides a DMA-based implementation of data reception
 * for UART peripherals. It supports both normal (one-shot) and stream
 * modes, with proper timeout handling.
 *
 * @param port The UART port to receive data from
 * @param received Pointer to the receive buffer
 * @param length Length of the receive buffer
 * @param status Output parameter for operation status
 * @param timeout_ms Timeout in milliseconds
 * @param as_stream True for stream mode, false for normal mode
 */
void dma_receive_handler(
    hal::UARTPort& port,
    void* received,
    std::size_t length,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000),
    bool as_stream = false);

/**
 * @brief Get the current read position in stream mode
 *
 * This function provides the current read position in stream mode,
 * allowing the caller to read data from the buffer.
 *
 * @param is_stopped Output parameter to indicate if reception is stopped
 * @param position Output parameter to receive the current position
 */
void last_read_handler(bool& is_stopped, std::size_t& position);

} // namespace hm11::sync

#endif // HM11_SYNC_DMA_RECEIVE_HANDLER_HPP