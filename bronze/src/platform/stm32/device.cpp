/**
 * @file device.cpp
 * @brief Implementation of STM32 device-specific functions
 *
 * This file contains the implementation of device-specific functions for the STM32F4xx
 * platform, including clock control and peripheral initialization.
 */

#include "hm11/platform/stm32/device.hpp"

#include "hm11/platform/stm32/registers.hpp"

namespace hm11 {
namespace platform {
namespace stm32 {
namespace device {

void enable_uart_clock(UARTIndex uart) {
    // Get the RCC registers
    RCC_TypeDef* rcc = RCC;

    // Enable the appropriate clock based on the UART
    switch (uart) {
        case UARTIndex::USART1:
            reg::set_bits(&rcc->APB2ENR, reg::rcc::APB2ENR_USART1EN);
            break;
        case UARTIndex::USART2:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_USART2EN);
            break;
        case UARTIndex::USART3:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_USART3EN);
            break;
        case UARTIndex::UART4:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_UART4EN);
            break;
        case UARTIndex::UART5:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_UART5EN);
            break;
        case UARTIndex::USART6:
            reg::set_bits(&rcc->APB2ENR, reg::rcc::APB2ENR_USART6EN);
            break;
        case UARTIndex::UART7:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_UART7EN);
            break;
        case UARTIndex::UART8:
            reg::set_bits(&rcc->APB1ENR, reg::rcc::APB1ENR_UART8EN);
            break;
    }
}

void enable_gpio_clock(GPIOPort port) {
    // Get the RCC registers
    RCC_TypeDef* rcc = RCC;

    // Enable the appropriate clock based on the GPIO port
    switch (port) {
        case GPIOPort::PortA:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOAEN);
            break;
        case GPIOPort::PortB:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOBEN);
            break;
        case GPIOPort::PortC:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOCEN);
            break;
        case GPIOPort::PortD:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIODEN);
            break;
        case GPIOPort::PortE:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOEEN);
            break;
        case GPIOPort::PortF:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOFEN);
            break;
        case GPIOPort::PortG:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOGEN);
            break;
        case GPIOPort::PortH:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOHEN);
            break;
        case GPIOPort::PortI:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOIEN);
            break;
        case GPIOPort::PortJ:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOJEN);
            break;
        case GPIOPort::PortK:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_GPIOKEN);
            break;
    }
}

void enable_dma_clock(DMAIndex dma) {
    // Get the RCC registers
    RCC_TypeDef* rcc = RCC;

    // Enable the appropriate clock based on the DMA controller
    switch (dma) {
        case DMAIndex::DMA1:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_DMA1EN);
            break;
        case DMAIndex::DMA2:
            reg::set_bits(&rcc->AHB1ENR, reg::rcc::AHB1ENR_DMA2EN);
            break;
    }
}

} // namespace device
} // namespace stm32
} // namespace platform
} // namespace hm11