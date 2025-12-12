/**
 * @file uart_port_impl.inl
 * @brief STM32_UART_DMA implementation (inline functions and templates)
 *
 * This file provides the implementation of the STM32_UART_DMA class template.
 * It is included at the end of uart_port_impl.hpp to keep the interface
 * declaration separate from the implementation details.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <chrono>

namespace hm11::stm32 {

// ============================================================================
// Constructor and Destructor
// ============================================================================

template <typename ReceiveHandler>
STM32_UART_DMA<ReceiveHandler>::STM32_UART_DMA(ReceiveHandler& receive_handler)
    : receive_handler_(receive_handler) {

#if defined(STM32F429xx) || defined(STM32F4)
    // Step 1: Enable peripheral clocks
    // Enable UART5 clock (APB1 bus)
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN;

    // Enable GPIOC and GPIOD clocks (AHB1 bus)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN;

    // Step 2: Configure GPIO pins for UART function
    configure_gpio();

    // Step 3: Configure UART5 peripheral
    configure_uart();
#else
    // Stub for non-STM32 builds (testing/compilation)
    static_cast<void>(receive_handler_);
#endif
}

template <typename ReceiveHandler> STM32_UART_DMA<ReceiveHandler>::~STM32_UART_DMA() noexcept {
#if defined(STM32F429xx) || defined(STM32F4)
    // Disable UART interrupts
    UART5->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE);
    UART5->CR3 &= ~(USART_CR3_EIE);

    // Disable UART peripheral
    UART5->CR1 &= ~USART_CR1_UE;

    // Note: We do NOT disable clocks here because other code may be using them.
    // The clocks will be disabled by the reset/power management system if needed.
#endif
}

// ============================================================================
// Private Helper Methods
// ============================================================================

template <typename ReceiveHandler> void STM32_UART_DMA<ReceiveHandler>::configure_gpio() noexcept {
#if defined(STM32F429xx) || defined(STM32F4)
    // Configure PC12 (UART5_TX)
    // Pin 12 is in the high half, so we use MODER bits [25:24]

    // Set mode to Alternate Function (10b)
    GPIOC->MODER &= ~(0x3U << (12U * 2U)); // Clear bits [25:24]
    GPIOC->MODER |= (0x2U << (12U * 2U));  // Set to AF mode (10b)

    // Set output type to Push-Pull (0)
    GPIOC->OTYPER &= ~(0x1U << 12U); // Clear bit 12 for push-pull

    // Set speed to 50 MHz (10b)
    GPIOC->OSPEEDR &= ~(0x3U << (12U * 2U)); // Clear bits [25:24]
    GPIOC->OSPEEDR |= (0x2U << (12U * 2U));  // Set to 50MHz (10b)

    // Set pull-up (01b)
    GPIOC->PUPDR &= ~(0x3U << (12U * 2U)); // Clear bits [25:24]
    GPIOC->PUPDR |= (0x1U << (12U * 2U));  // Set to pull-up (01b)

    // Set alternate function to AF7 (UART5)
    // Pin 12 uses AFR[1] (AFRH), bits [19:16] (4 bits per pin, pin 12 is index 4 in high register)
    GPIOC->AFR[1] &= ~(0xFU << ((12U - 8U) * 4U)); // Clear bits [19:16]
    GPIOC->AFR[1] |= (0x7U << ((12U - 8U) * 4U));  // Set to AF7 (0111b)

    // Configure PD2 (UART5_RX)
    // Pin 2 is in the low half, so we use MODER bits [5:4]

    // Set mode to Alternate Function (10b)
    GPIOD->MODER &= ~(0x3U << (2U * 2U)); // Clear bits [5:4]
    GPIOD->MODER |= (0x2U << (2U * 2U));  // Set to AF mode (10b)

    // Set pull-up (01b)
    GPIOD->PUPDR &= ~(0x3U << (2U * 2U)); // Clear bits [5:4]
    GPIOD->PUPDR |= (0x1U << (2U * 2U));  // Set to pull-up (01b)

    // Set alternate function to AF7 (UART5)
    // Pin 2 uses AFR[0] (AFRL), bits [11:8] (4 bits per pin, pin 2 is index 2)
    GPIOD->AFR[0] &= ~(0xFU << (2U * 4U)); // Clear bits [11:8]
    GPIOD->AFR[0] |= (0x7U << (2U * 4U));  // Set to AF7 (0111b)
#endif
}

template <typename ReceiveHandler> void STM32_UART_DMA<ReceiveHandler>::configure_uart() noexcept {
#if defined(STM32F429xx) || defined(STM32F4)
    // Step 1: Disable UART for configuration
    UART5->CR1 &= ~USART_CR1_UE;

    // Step 2: Set baud rate to 9600
    // Assume APB1 clock is 42 MHz (default for STM32F429 with 168 MHz HCLK)
    constexpr uint32_t APB1_CLOCK_HZ = 42000000U;
    constexpr uint32_t BAUD_RATE = 9600U;
    UART5->BRR = calculate_brr(APB1_CLOCK_HZ, BAUD_RATE);

    // Step 3: Configure word length to 8 bits (M = 0)
    UART5->CR1 &= ~USART_CR1_M;

    // Step 4: Configure parity to none (PCE = 0)
    UART5->CR1 &= ~USART_CR1_PCE;

    // Step 5: Configure stop bits to 1 (STOP = 00b)
    UART5->CR2 &= ~USART_CR2_STOP;

    // Step 6: Enable transmitter (TE = 1)
    UART5->CR1 |= USART_CR1_TE;

    // Step 7: Enable receiver (RE = 1)
    UART5->CR1 |= USART_CR1_RE;

    // Step 8: Enable error interrupts
    // Error interrupt enable (EIE) for framing, noise, overrun errors
    UART5->CR3 |= USART_CR3_EIE;

    // Parity error interrupt enable (PEIE)
    UART5->CR1 |= USART_CR1_PEIE;

    // RXNE interrupt enable (data received)
    UART5->CR1 |= USART_CR1_RXNEIE;

    // Step 9: Enable UART peripheral
    UART5->CR1 |= USART_CR1_UE;
#endif
}

template <typename ReceiveHandler> bool STM32_UART_DMA<ReceiveHandler>::has_uart_error() const noexcept {
#if defined(STM32F429xx) || defined(STM32F4)
    // Check error flags in SR register
    const uint32_t sr = UART5->SR;
    return (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) != 0;
#else
    return false;
#endif
}

template <typename ReceiveHandler> void STM32_UART_DMA<ReceiveHandler>::clear_uart_errors() noexcept {
#if defined(STM32F429xx) || defined(STM32F4)
    // Standard STM32 error clearing sequence: Read SR, then read DR
    volatile uint32_t sr = UART5->SR;
    volatile uint32_t dr = UART5->DR;
    static_cast<void>(sr); // Suppress unused variable warning
    static_cast<void>(dr);
#endif
}

// ============================================================================
// Public Interface Methods
// ============================================================================

template <typename ReceiveHandler>
void STM32_UART_DMA<ReceiveHandler>::transmit(
    hal::UartData8bConst data,
    hal::UartStatus& status,
    uint32_t timeout_ms) noexcept {

#if defined(STM32F429xx) || defined(STM32F4)
    // Cache coherency: Clean cache before DMA reads buffer (Cortex-M7 only, no-op on M4)
    platform::clean_dcache_range(const_cast<hal::UInt8*>(data.data()), data.size());

    // Calculate absolute deadline for timeout
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    // Transmit each byte
    for (std::size_t i = 0; i < data.size(); ++i) {
        // Wait for TXE flag (transmit data register empty)
        while ((UART5->SR & USART_SR_TXE) == 0) {
            // Check for timeout
            if (std::chrono::steady_clock::now() > deadline) {
                status = hal::UartStatus::ErrTimeout;
                return;
            }

            // Check for errors
            if (has_uart_error()) {
                clear_uart_errors();
                status = hal::UartStatus::ErrError;
                return;
            }
        }

        // Write byte to data register
        UART5->DR = data[i];
    }

    // Wait for transmission complete (TC flag)
    while ((UART5->SR & USART_SR_TC) == 0) {
        // Check for timeout
        if (std::chrono::steady_clock::now() > deadline) {
            status = hal::UartStatus::ErrTimeout;
            return;
        }

        // Check for errors
        if (has_uart_error()) {
            clear_uart_errors();
            status = hal::UartStatus::ErrError;
            return;
        }
    }

    // Success
    status = hal::UartStatus::Ok;

#else
    // Stub for non-STM32 builds
    static_cast<void>(data);
    static_cast<void>(timeout_ms);
    status = hal::UartStatus::ErrError;
#endif
}

template <typename ReceiveHandler>
void STM32_UART_DMA<ReceiveHandler>::receive(
    hal::UartData8b data,
    hal::UartStatus& status,
    uint32_t timeout_ms,
    bool as_stream) noexcept {

    // Delegate to receive handler (Decision 1: Dependency Injection, Decision 3: Strategy Pattern)
    // The handler implements the DMA-based receive logic and manages buffer synchronization
    receive_handler_(*this, data, status, timeout_ms, as_stream);
}

} // namespace hm11::stm32
