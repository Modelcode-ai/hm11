/**
 * @file hm11_driver_uart.inl
 * @brief Inline template implementations for UART configuration operations
 *
 * This file contains the template implementations for UART configuration
 * operations (baud rate, flow control, parity bit, stop bit). It is included
 * at the end of hm11_driver.hpp to provide template definitions without
 * requiring explicit instantiation.
 *
 * ## Overview
 *
 * UART configuration operations allow applications to adjust serial
 * communication parameters between the host MCU and the HM-11 module.
 * These operations configure:
 * - Baud rate (9 supported rates from 1200 to 230400)
 * - Flow control (RTS/CTS hardware flow control)
 * - Parity bit (None, Odd, Even)
 * - Stop bit (1 or 2 stop bits)
 *
 * ## Implementation Pattern
 *
 * All UART operations follow the standard AT command pattern:
 * 1. Set operations: Send "AT+CMD{value}", expect "OK+Set:{value}"
 * 2. Get operations: Send "AT+CMD?", expect "OK+Get:{value}", parse result
 *
 * ## Enum Mapping
 *
 * UART enums map to command parameters using enum_to_string utilities:
 * - UartBaudRate: Maps by declaration order (0-8), not by numeric value
 * - ParityBit: Sequential mapping (None=0, Odd=1, Even=2)
 * - StopBit: Sequential mapping (OneStopBit=0, TwoStopBit=1)
 * - Flow control: Boolean mapping (false=0, true=1)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include "hm11/util/enum_mapping.hpp"

namespace hm11 {

// ============================================================================
// UART Configuration Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_uart_baud_rate(
    types::UartBaudRate value,
    hal::UartStatus& status) noexcept {
    // Build command using enum_to_string mapping
    util::CommandBuilder cmd;
    cmd.append("AT+BAUD").append(util::enum_to_string(value));

    // Build expected response
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(value));

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_uart_baud_rate(
    types::UartBaudRate& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 digit

    // Transmit query command
    transmit("AT+BAUD?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse enum value from single character response
    std::string_view response = util::to_string_view(buffer);

    if (response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char value_char = response[EXPECT_PREFIX.size()];

    // Map response character to enum value (0-8)
    // Response is ASCII digit '0'-'8'
    if (value_char < '0' || value_char > '8') {
        status = hal::UartStatus::ErrError;
        return;
    }

    uint8_t enum_ordinal = static_cast<uint8_t>(value_char - '0');

    // Convert ordinal to enum using explicit mapping
    // Ada: UART_Baud_Rate'Val (Value (This.Responce (Ok_Get'Length + 1)))
    switch (enum_ordinal) {
        case 0:
            result = types::UartBaudRate::Br9600;
            break;
        case 1:
            result = types::UartBaudRate::Br19200;
            break;
        case 2:
            result = types::UartBaudRate::Br38400;
            break;
        case 3:
            result = types::UartBaudRate::Br57600;
            break;
        case 4:
            result = types::UartBaudRate::Br115200;
            break;
        case 5:
            result = types::UartBaudRate::Br4800;
            break;
        case 6:
            result = types::UartBaudRate::Br2400;
            break;
        case 7:
            result = types::UartBaudRate::Br1200;
            break;
        case 8:
            result = types::UartBaudRate::Br230400;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_flow_control_switch(
    bool enable,
    hal::UartStatus& status) noexcept {
    // Build command using bool_to_string mapping
    util::CommandBuilder cmd;
    cmd.append("AT+FIOW").append(util::bool_to_string(enable));

    // Build expected response
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::bool_to_string(enable));

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_flow_control_switch(
    bool& enabled,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 digit

    // Transmit query command
    transmit("AT+FIOW?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse boolean value from single character response
    std::string_view response = util::to_string_view(buffer);

    if (response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char value_char = response[EXPECT_PREFIX.size()];

    // Map response character to boolean ('0' or '1')
    // Ada: Switch := S = '1';
    if (value_char == '0') {
        enabled = false;
    } else if (value_char == '1') {
        enabled = true;
    } else {
        status = hal::UartStatus::ErrError;
        return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_parity_bit(
    types::ParityBit parity,
    hal::UartStatus& status) noexcept {
    // Build command using enum_to_string mapping
    util::CommandBuilder cmd;
    cmd.append("AT+PARI").append(util::enum_to_string(parity));

    // Build expected response
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(parity));

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_parity_bit(
    types::ParityBit& parity,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 digit

    // Transmit query command
    transmit("AT+PARI?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse enum value from single character response
    std::string_view response = util::to_string_view(buffer);

    if (response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char value_char = response[EXPECT_PREFIX.size()];

    // Map response character to enum value (0-2)
    // Ada: Parity := Parity_Bit'Val (Value (This.Responce (Ok_Get'Length + 1)))
    if (value_char < '0' || value_char > '2') {
        status = hal::UartStatus::ErrError;
        return;
    }

    uint8_t enum_ordinal = static_cast<uint8_t>(value_char - '0');

    switch (enum_ordinal) {
        case 0:
            parity = types::ParityBit::None;
            break;
        case 1:
            parity = types::ParityBit::Odd;
            break;
        case 2:
            parity = types::ParityBit::Even;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_stop_bit(
    types::StopBit stop_bit,
    hal::UartStatus& status) noexcept {
    // Build command using enum_to_string mapping
    util::CommandBuilder cmd;
    cmd.append("AT+STOP").append(util::enum_to_string(stop_bit));

    // Build expected response
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(stop_bit));

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_stop_bit(
    types::StopBit& stop_bit,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 digit

    // Transmit query command
    transmit("AT+STOP?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse enum value from single character response
    std::string_view response = util::to_string_view(buffer);

    if (response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char value_char = response[EXPECT_PREFIX.size()];

    // Map response character to enum value (0-1)
    // Ada: Result := Stop_Bit'Val (Value (This.Responce (Ok_Get'Length + 1)))
    if (value_char < '0' || value_char > '1') {
        status = hal::UartStatus::ErrError;
        return;
    }

    uint8_t enum_ordinal = static_cast<uint8_t>(value_char - '0');

    switch (enum_ordinal) {
        case 0:
            stop_bit = types::StopBit::OneStopBit;
            break;
        case 1:
            stop_bit = types::StopBit::TwoStopBit;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

} // namespace hm11
