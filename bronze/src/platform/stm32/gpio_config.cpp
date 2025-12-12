/**
 * @file gpio_config.cpp
 * @brief GPIO configuration implementation for STM32 UART pins
 *
 * This file provides the implementation of functions to configure GPIO pins
 * for UART operations, including TX, RX, RTS, and CTS pins.
 *
 * This implementation follows Design Decision 30 by using direct register access
 * via CMSIS headers without the STM32 HAL library dependency.
 */

#include "hm11/platform/stm32/gpio_config.hpp"

namespace hm11::platform::stm32::gpio_config {

void configure_uart_pins(device::UARTIndex uart_index, const device::GPIOPin& tx_pin, const device::GPIOPin& rx_pin) {
    // Determine the appropriate alternate function for this UART
    // UART pins use AF7 for USART1-3 and AF8 for UART4-8
    GPIOAlternateFunction af = GPIOAlternateFunction::AF8;

    if (uart_index == device::UARTIndex::USART1 || uart_index == device::UARTIndex::USART2 ||
        uart_index == device::UARTIndex::USART3) {
        af = GPIOAlternateFunction::AF7;
    }

    // Enable GPIO clocks for the ports
    device::enable_gpio_clock(tx_pin.port);
    device::enable_gpio_clock(rx_pin.port);

    // Configure TX pin as alternate function, push-pull, no pull-up/down
    gpio::configure_alternate_function(tx_pin, af, GPIOOutputType::PushPull, GPIOPull::NoPull, GPIOSpeed::High);

    // Configure RX pin as alternate function, push-pull, pull-up
    gpio::configure_alternate_function(rx_pin, af, GPIOOutputType::PushPull, GPIOPull::PullUp, GPIOSpeed::High);
}

void configure_uart_pins_with_flow_control(
    device::UARTIndex uart_index,
    const device::GPIOPin& tx_pin,
    const device::GPIOPin& rx_pin,
    const device::GPIOPin& rts_pin,
    const device::GPIOPin& cts_pin) {
    // First configure TX and RX pins
    configure_uart_pins(uart_index, tx_pin, rx_pin);

    // Determine the appropriate alternate function for this UART
    GPIOAlternateFunction af = GPIOAlternateFunction::AF8;

    if (uart_index == device::UARTIndex::USART1 || uart_index == device::UARTIndex::USART2 ||
        uart_index == device::UARTIndex::USART3) {
        af = GPIOAlternateFunction::AF7;
    }

    // Enable GPIO clocks for the flow control ports
    device::enable_gpio_clock(rts_pin.port);
    device::enable_gpio_clock(cts_pin.port);

    // Configure RTS pin as alternate function, push-pull, no pull-up/down
    gpio::configure_alternate_function(rts_pin, af, GPIOOutputType::PushPull, GPIOPull::NoPull, GPIOSpeed::High);

    // Configure CTS pin as alternate function, push-pull, pull-up
    gpio::configure_alternate_function(cts_pin, af, GPIOOutputType::PushPull, GPIOPull::PullUp, GPIOSpeed::High);
}

device::GPIOPin get_default_tx_pin(device::UARTIndex uart_index) {
    // Return the default TX pin for the specified UART
    // These defaults match the pins used in the original Ada implementation
    switch (uart_index) {
        case device::UARTIndex::USART1:
            return device::GPIOPin(device::GPIOPort::PortA, 9);
        case device::UARTIndex::USART2:
            return device::GPIOPin(device::GPIOPort::PortA, 2);
        case device::UARTIndex::USART3:
            return device::GPIOPin(device::GPIOPort::PortB, 10);
        case device::UARTIndex::UART4:
            return device::GPIOPin(device::GPIOPort::PortA, 0);
        case device::UARTIndex::UART5:
            return device::GPIOPin(device::GPIOPort::PortC, 12); // Used for HM-11 in original Ada implementation
        case device::UARTIndex::USART6:
            return device::GPIOPin(device::GPIOPort::PortC, 6);
        case device::UARTIndex::UART7:
            return device::GPIOPin(device::GPIOPort::PortE, 8);
        case device::UARTIndex::UART8:
            return device::GPIOPin(device::GPIOPort::PortE, 1);
        default:
            // Default to UART5 (HM-11 default)
            return device::GPIOPin(device::GPIOPort::PortC, 12);
    }
}

device::GPIOPin get_default_rx_pin(device::UARTIndex uart_index) {
    // Return the default RX pin for the specified UART
    // These defaults match the pins used in the original Ada implementation
    switch (uart_index) {
        case device::UARTIndex::USART1:
            return device::GPIOPin(device::GPIOPort::PortA, 10);
        case device::UARTIndex::USART2:
            return device::GPIOPin(device::GPIOPort::PortA, 3);
        case device::UARTIndex::USART3:
            return device::GPIOPin(device::GPIOPort::PortB, 11);
        case device::UARTIndex::UART4:
            return device::GPIOPin(device::GPIOPort::PortA, 1);
        case device::UARTIndex::UART5:
            return device::GPIOPin(device::GPIOPort::PortD, 2); // Used for HM-11 in original Ada implementation
        case device::UARTIndex::USART6:
            return device::GPIOPin(device::GPIOPort::PortC, 7);
        case device::UARTIndex::UART7:
            return device::GPIOPin(device::GPIOPort::PortE, 7);
        case device::UARTIndex::UART8:
            return device::GPIOPin(device::GPIOPort::PortE, 0);
        default:
            // Default to UART5 (HM-11 default)
            return device::GPIOPin(device::GPIOPort::PortD, 2);
    }
}

} // namespace hm11::platform::stm32::gpio_config