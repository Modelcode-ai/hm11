/**
 * @file hm11_driver_gatt_access.inl
 * @brief Inline template implementations for GATT characteristic access operations
 *
 * This file contains the template implementations for GATT characteristic
 * access operations including notify/indicate control, characteristic read/write,
 * and peripheral characteristic configuration. It is included at the end of
 * hm11_driver.hpp to provide template definitions.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

namespace hm11 {

// ============================================================================
// GATT Characteristic Access Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::enable_characteristic_notify(
    const types::HandleType& handle,
    hal::UartStatus& status,
    types::NotifyResponse& response) noexcept {
    // Transmit "AT+NOTIFY_ON{handle}" command
    util::CommandBuilder cmd;
    cmd.append("AT+NOTIFY_ON").append(handle.view());
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Read and parse notify response: "OK+SEND-OK", "OK+SEND-ER", or "OK+DATA-ER"
    read_characteristic_notify_response(status, response);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::disable_characteristic_notify(
    const types::HandleType& handle,
    hal::UartStatus& status,
    types::NotifyResponse& response) noexcept {
    // Transmit "AT+NOTIFYOFF{handle}" command
    util::CommandBuilder cmd;
    cmd.append("AT+NOTIFYOFF").append(handle.view());
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Read and parse notify response
    read_characteristic_notify_response(status, response);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::read_characteristic_notify(
    const types::HandleType& handle,
    hal::UartStatus& status,
    types::NotifyResponse& response) noexcept {
    // Transmit "AT+READDATA{handle}" command
    util::CommandBuilder cmd;
    cmd.append("AT+READDATA").append(handle.view());
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Read and parse notify response
    read_characteristic_notify_response(status, response);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::read_characteristic_notify_response(
    hal::UartStatus& status,
    types::NotifyResponse& response) noexcept {
    // Parse response: "OK+SEND-OK" (10 bytes) or "OK+DATA-ER" (10 bytes) or "OK+SEND-ER" (default)
    constexpr std::size_t RESPONSE_SIZE = 10;
    constexpr std::string_view SEND_OK = "OK+SEND-OK";
    constexpr std::string_view DATA_ER = "OK+DATA-ER";

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse response string
    std::string_view response_str = util::to_string_view(buffer);

    if (response_str == SEND_OK) {
        response = types::NotifyResponse::SendOk;
    } else if (response_str == DATA_ER) {
        response = types::NotifyResponse::DataEr;
    } else {
        // Default to SendEr for any other response
        response = types::NotifyResponse::SendEr;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_method_and_characteristic_handle(
    const types::HandleType& handle,
    types::SendDataMethod method,
    hal::UartStatus& status) noexcept {
    // Transmit "AT+SET_WAY{method}{handle}" command
    // Method: WR, WN, IN, NO
    util::CommandBuilder cmd;
    cmd.append("AT+SET_WAY").append(util::enum_to_string(method)).append(handle.view());

    // Expected response: "OK+SEND-OK"
    constexpr std::string_view EXPECT = "OK+SEND-OK";
    transmit_and_check(cmd.get(), EXPECT, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::send_data_to_characteristic(
    const types::HandleType& handle,
    types::SendDataCharacteristic method,
    hal::UartData8bConst data,
    hal::UartStatus& status) noexcept {
    // Transmit "AT+SEND_DATA{method}{handle}" followed by data
    // Method: WR or WN
    util::CommandBuilder cmd;
    cmd.append("AT+SEND_DATA").append(util::enum_to_string(method)).append(handle.view());

    // Send command prefix
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Send data payload (no AT command prefix, just raw data)
    uart_port_.transmit(data, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_use_characteristic_uuid_count(
    types::CharacteristicUuidCount count,
    hal::UartStatus& status) noexcept {
    // Transmit "AT+FFE2{count}" command where count is "?", "0", "1", or "2"
    util::CommandBuilder cmd;
    cmd.append("AT+FFE2").append(util::enum_to_string(count));

    // Build expected response: "OK+Set:{count}"
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(count));

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_characteristic(
    const types::CharacteristicType& value,
    hal::UartStatus& status) noexcept {
    // Transmit "AT+CHAR0x{uuid}" command
    util::CommandBuilder cmd;
    cmd.append("AT+CHAR0x").append(value.view());

    // Expected response: "OK+Set:0x{uuid}" (16 bytes)
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";
    constexpr std::size_t RESPONSE_SIZE = 16; // "OK+Set:0xFFE1" = 14 bytes + some margin

    // Transmit command
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive and verify response
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix "OK+Set:"
    check_response(buffer, EXPECT_PREFIX, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_characteristic(
    types::CharacteristicType& result,
    hal::UartStatus& status) noexcept {
    // Transmit "AT+CHAR?" command
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:0x";
    constexpr std::size_t CHAR_UUID_LENGTH = 4; // 4 hex characters
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + CHAR_UUID_LENGTH;

    transmit("AT+CHAR?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response: "OK+Get:0xFFE1"
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

    // Parse characteristic UUID (4 hex chars after "OK+Get:0x")
    std::string_view full_response = util::to_string_view(buffer);

    if (full_response.size() < EXPECT_PREFIX.size() + CHAR_UUID_LENGTH) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Extract UUID (skip "OK+Get:0x" prefix)
    std::string_view uuid_str = full_response.substr(EXPECT_PREFIX.size(), CHAR_UUID_LENGTH);

    // Copy to result (CharacteristicType validates hex characters)
    result = types::CharacteristicType(uuid_str);
}

} // namespace hm11
