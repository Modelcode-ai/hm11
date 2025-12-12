/**
 * @file watchdog.cpp
 * @brief STM32-specific Watchdog implementation for timeout handling
 *
 * This file implements the STM32-specific extensions to the Watchdog class,
 * providing timeout handling and error detection for UART and DMA operations.
 */

#include <hm11/platform/stm32/watchdog.hpp>

namespace hm11::platform::stm32 {

bool Stm32Watchdog::start_dma(
    void* buffer,
    std::size_t length,
    std::chrono::milliseconds timeout_ms,
    USART_TypeDef* uart,
    bool stream_mode) {
    // Store UART and mode information
    uart_ = uart;
    stream_mode_ = stream_mode;

    // Call base class start
    return this->start(buffer, length, timeout_ms);
}

void Stm32Watchdog::stop_dma() {
    // Clear UART and mode information
    uart_ = nullptr;
    stream_mode_ = false;

    // Call base class stop
    this->stop();
}

bool Stm32Watchdog::check_uart_errors() {
    // Check if watchdog is started and UART is valid
    if (!is_started() || !uart_) {
        return false;
    }

    // Check UART status register for errors
    std::uint32_t isr = uart_->SR;
    bool has_error = false;

    // Check for overrun error
    if (isr & USART_SR_ORE) {
        this->release(sync::UartStatus::ErrOverflow);
        has_error = true;
    }

    // Check for framing error
    if (isr & USART_SR_FE) {
        this->release(sync::UartStatus::ErrError);
        has_error = true;
    }

    // Check for noise error
    if (isr & USART_SR_NE) {
        this->release(sync::UartStatus::ErrError);
        has_error = true;
    }

    // Check for parity error
    if (isr & USART_SR_PE) {
        this->release(sync::UartStatus::ErrError);
        has_error = true;
    }

    // Return true if any error was detected
    return has_error;
}

} // namespace hm11::platform::stm32