/**
 * @file gpio.hpp
 * @brief STM32 GPIO configuration types and utilities
 *
 * This file contains detailed GPIO pin configuration types and utilities for
 * STM32F4xx microcontrollers. It provides a C++ wrapper around the STM32 GPIO
 * peripheral with direct register access.
 *
 * @note This implementation follows Design Decision 30 by using direct register
 * access via CMSIS headers without the STM32 HAL library dependency.
 */

#ifndef HM11_PLATFORM_STM32_GPIO_HPP
#define HM11_PLATFORM_STM32_GPIO_HPP

#include <cstdint>

#include "device.hpp"
#include "peripherals.hpp"
#include "registers.hpp"

namespace hm11 {
namespace platform {
namespace stm32 {
namespace gpio {

/**
 * @brief Configure a GPIO pin
 *
 * This function configures a GPIO pin with the specified mode, pull, speed, and output type.
 *
 * @param pin The GPIO pin specification
 * @param mode The GPIO mode
 * @param pull The pull-up/pull-down setting
 * @param speed The output speed setting
 * @param output_type The output type setting
 */
inline void configure_pin(
    const device::GPIOPin& pin,
    GPIOMode mode = GPIOMode::Input,
    GPIOPull pull = GPIOPull::NoPull,
    GPIOSpeed speed = GPIOSpeed::High,
    GPIOOutputType output_type = GPIOOutputType::PushPull) {
    // Get the GPIO peripheral
    GPIO_TypeDef* gpio = device::get_gpio_peripheral(pin.port);

    // Enable the GPIO clock
    device::enable_gpio_clock(pin.port);

    // Configure the pin mode (Input, Output, Alternate, Analog)
    uint32_t mode_val = 0;
    switch (mode) {
        case GPIOMode::Input:
            mode_val = reg::gpio::MODER_INPUT;
            break;
        case GPIOMode::Output:
            mode_val = reg::gpio::MODER_OUTPUT;
            break;
        case GPIOMode::AlternateFunction:
            mode_val = reg::gpio::MODER_ALTERNATE;
            break;
        case GPIOMode::Analog:
            mode_val = reg::gpio::MODER_ANALOG;
            break;
    }
    reg::modify_field(&gpio->MODER, pin.pin * 2, 2, mode_val);

    // Configure the output type (Push-pull or Open-drain)
    if (mode == GPIOMode::Output || mode == GPIOMode::AlternateFunction) {
        uint32_t otype_val = (output_type == GPIOOutputType::PushPull) ? reg::gpio::OTYPER_PUSH_PULL
                                                                       : reg::gpio::OTYPER_OPEN_DRAIN;
        reg::modify_field(&gpio->OTYPER, pin.pin, 1, otype_val);
    }

    // Configure the output speed
    uint32_t speed_val = 0;
    switch (speed) {
        case GPIOSpeed::Low:
            speed_val = reg::gpio::OSPEEDR_LOW;
            break;
        case GPIOSpeed::Medium:
            speed_val = reg::gpio::OSPEEDR_MEDIUM;
            break;
        case GPIOSpeed::Fast:
        case GPIOSpeed::High:
            speed_val = reg::gpio::OSPEEDR_VERY_HIGH;
            break;
    }
    reg::modify_field(&gpio->OSPEEDR, pin.pin * 2, 2, speed_val);

    // Configure the pull-up/pull-down resistors
    uint32_t pull_val = 0;
    switch (pull) {
        case GPIOPull::NoPull:
            pull_val = reg::gpio::PUPDR_NONE;
            break;
        case GPIOPull::PullUp:
            pull_val = reg::gpio::PUPDR_PULL_UP;
            break;
        case GPIOPull::PullDown:
            pull_val = reg::gpio::PUPDR_PULL_DOWN;
            break;
    }
    reg::modify_field(&gpio->PUPDR, pin.pin * 2, 2, pull_val);
}

/**
 * @brief Configure a GPIO pin for alternate function mode
 *
 * This function configures a GPIO pin for alternate function mode with the specified
 * alternate function, output type, pull, and speed settings.
 *
 * @param pin The GPIO pin specification
 * @param af The alternate function
 * @param output_type The output type setting
 * @param pull The pull-up/pull-down setting
 * @param speed The output speed setting
 */
inline void configure_alternate_function(
    const device::GPIOPin& pin,
    GPIOAlternateFunction af,
    GPIOOutputType output_type = GPIOOutputType::PushPull,
    GPIOPull pull = GPIOPull::NoPull,
    GPIOSpeed speed = GPIOSpeed::High) {
    // First configure the pin with the correct mode
    configure_pin(pin, GPIOMode::AlternateFunction, pull, speed, output_type);

    // Then set the alternate function
    GPIO_TypeDef* gpio = device::get_gpio_peripheral(pin.port);

    // Determine which alternate function register to use (AFR[0] for pins 0-7, AFR[1] for pins 8-15)
    const uint8_t afr_index = pin.pin / 8;
    const uint8_t afr_offset = (pin.pin % 8) * 4;

    // Set the alternate function
    reg::modify_field(&gpio->AFR[afr_index], afr_offset, 4, static_cast<uint32_t>(af));
}

/**
 * @brief Set a GPIO pin output value
 *
 * @param pin The GPIO pin specification
 * @param state true to set the pin high, false to set it low
 */
inline void set_pin(const device::GPIOPin& pin, bool state) {
    GPIO_TypeDef* gpio = device::get_gpio_peripheral(pin.port);

    // Use atomic bit set/reset register for thread safety
    if (state) {
        // Set the pin
        gpio->BSRR = 1U << pin.pin;
    } else {
        // Reset the pin
        gpio->BSRR = 1U << (pin.pin + 16);
    }
}

/**
 * @brief Toggle a GPIO pin output value
 *
 * @param pin The GPIO pin specification
 */
inline void toggle_pin(const device::GPIOPin& pin) {
    GPIO_TypeDef* gpio = device::get_gpio_peripheral(pin.port);

    // Read current state
    uint32_t odr = gpio->ODR;

    // Toggle the pin
    gpio->BSRR = ((odr & (1U << pin.pin)) ? (1U << (pin.pin + 16)) : (1U << pin.pin));
}

/**
 * @brief Read a GPIO pin input value
 *
 * @param pin The GPIO pin specification
 * @return true if the pin is high, false if low
 */
inline bool read_pin(const device::GPIOPin& pin) {
    GPIO_TypeDef* gpio = device::get_gpio_peripheral(pin.port);

    // Read from input data register
    return (gpio->IDR & (1U << pin.pin)) != 0;
}

/**
 * @brief Configure UART TX/RX pins for the specified UART
 *
 * This function configures the GPIO pins for UART transmit and receive functions.
 *
 * @param uart The UART index
 * @param tx_pin The TX pin
 * @param rx_pin The RX pin
 */
inline void configure_uart_pins(device::UARTIndex uart, const device::GPIOPin& tx_pin, const device::GPIOPin& rx_pin) {
    // UART pins use alternate function AF7 for USART1, USART2, USART3
    // and AF8 for UART4, UART5, USART6, UART7, UART8
    GPIOAlternateFunction af = GPIOAlternateFunction::AF8;

    if (uart == device::UARTIndex::USART1 || uart == device::UARTIndex::USART2 || uart == device::UARTIndex::USART3) {
        af = GPIOAlternateFunction::AF7;
    }

    // Configure TX pin as alternate function, push-pull, no pull-up/down
    configure_alternate_function(tx_pin, af, GPIOOutputType::PushPull, GPIOPull::NoPull, GPIOSpeed::High);

    // Configure RX pin as alternate function, push-pull, pull-up
    configure_alternate_function(rx_pin, af, GPIOOutputType::PushPull, GPIOPull::PullUp, GPIOSpeed::High);
}

/**
 * @brief Configure UART flow control pins (RTS/CTS)
 *
 * This function configures the GPIO pins for UART hardware flow control.
 *
 * @param uart The UART index
 * @param rts_pin The RTS pin
 * @param cts_pin The CTS pin
 */
inline void configure_uart_flow_control_pins(
    device::UARTIndex uart,
    const device::GPIOPin& rts_pin,
    const device::GPIOPin& cts_pin) {
    // UART pins use alternate function AF7 for USART1, USART2, USART3
    // and AF8 for UART4, UART5, USART6, UART7, UART8
    GPIOAlternateFunction af = GPIOAlternateFunction::AF8;

    if (uart == device::UARTIndex::USART1 || uart == device::UARTIndex::USART2 || uart == device::UARTIndex::USART3) {
        af = GPIOAlternateFunction::AF7;
    }

    // Configure RTS pin as alternate function, push-pull, no pull-up/down
    configure_alternate_function(rts_pin, af, GPIOOutputType::PushPull, GPIOPull::NoPull, GPIOSpeed::High);

    // Configure CTS pin as alternate function, push-pull, pull-up
    configure_alternate_function(cts_pin, af, GPIOOutputType::PushPull, GPIOPull::PullUp, GPIOSpeed::High);
}

/**
 * @brief Configure LED pins for output
 *
 * This utility function configures a GPIO pin to be used as an LED output.
 *
 * @param pin The GPIO pin to configure
 */
inline void configure_led_pin(const device::GPIOPin& pin) {
    configure_pin(pin, GPIOMode::Output, GPIOPull::NoPull, GPIOSpeed::Low, GPIOOutputType::PushPull);
}

/**
 * @brief Configure button pins for input
 *
 * This utility function configures a GPIO pin to be used as a button input with pull-up.
 *
 * @param pin The GPIO pin to configure
 * @param use_pull_up true to use pull-up, false to use pull-down
 */
inline void configure_button_pin(const device::GPIOPin& pin, bool use_pull_up = true) {
    configure_pin(
        pin,
        GPIOMode::Input,
        use_pull_up ? GPIOPull::PullUp : GPIOPull::PullDown,
        GPIOSpeed::Low,
        GPIOOutputType::PushPull);
}

} // namespace gpio
} // namespace stm32
} // namespace platform
} // namespace hm11

#endif // HM11_PLATFORM_STM32_GPIO_HPP