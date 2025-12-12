/**
 * @file gpio_config.hpp
 * @brief GPIO configuration functions for STM32 UART pins
 *
 * This file provides functions to configure GPIO pins for UART operations,
 * including TX, RX, RTS, and CTS pins. It builds on the generic GPIO functionality
 * provided by the platform and adds specific configurations needed for UART operation.
 *
 * This implementation follows Design Decision 30 by using direct register access
 * via CMSIS headers without the STM32 HAL library dependency.
 */

#ifndef HM11_PLATFORM_STM32_GPIO_CONFIG_HPP
#define HM11_PLATFORM_STM32_GPIO_CONFIG_HPP

#include "device.hpp"
#include "gpio.hpp"
#include "peripherals.hpp"

namespace hm11::platform::stm32::gpio_config {

/**
 * @brief Configure GPIO pins for UART communication
 *
 * This function configures the GPIO pins for a UART peripheral, including
 * enabling the appropriate GPIO clocks and setting the correct alternate functions.
 * The pins must be valid for the specified UART peripheral.
 *
 * @param uart_index The UART peripheral index
 * @param tx_pin The TX pin configuration
 * @param rx_pin The RX pin configuration
 */
void configure_uart_pins(device::UARTIndex uart_index, const device::GPIOPin& tx_pin, const device::GPIOPin& rx_pin);

/**
 * @brief Configure GPIO pins for UART communication with hardware flow control
 *
 * This function configures the GPIO pins for a UART peripheral with hardware
 * flow control (RTS/CTS), including enabling the appropriate GPIO clocks and
 * setting the correct alternate functions. The pins must be valid for the
 * specified UART peripheral.
 *
 * @param uart_index The UART peripheral index
 * @param tx_pin The TX pin configuration
 * @param rx_pin The RX pin configuration
 * @param rts_pin The RTS pin configuration
 * @param cts_pin The CTS pin configuration
 */
void configure_uart_pins_with_flow_control(
    device::UARTIndex uart_index,
    const device::GPIOPin& tx_pin,
    const device::GPIOPin& rx_pin,
    const device::GPIOPin& rts_pin,
    const device::GPIOPin& cts_pin);

/**
 * @brief Get the default TX pin for a UART peripheral
 *
 * This function returns the default TX pin for the specified UART peripheral.
 * These defaults match the pins used in the original Ada implementation.
 *
 * @param uart_index The UART peripheral index
 * @return device::GPIOPin The default TX pin
 */
device::GPIOPin get_default_tx_pin(device::UARTIndex uart_index);

/**
 * @brief Get the default RX pin for a UART peripheral
 *
 * This function returns the default RX pin for the specified UART peripheral.
 * These defaults match the pins used in the original Ada implementation.
 *
 * @param uart_index The UART peripheral index
 * @return device::GPIOPin The default RX pin
 */
device::GPIOPin get_default_rx_pin(device::UARTIndex uart_index);

} // namespace hm11::platform::stm32::gpio_config

#endif // HM11_PLATFORM_STM32_GPIO_CONFIG_HPP