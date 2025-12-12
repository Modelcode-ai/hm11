/**
 * @file hm11_driver_device.inl
 * @brief Inline template implementations for device management operations
 *
 * This file contains the template implementations for device management
 * operations. It is included at the end of hm11_driver.hpp to provide
 * template definitions without requiring explicit instantiation.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

namespace hm11 {

// ============================================================================
// Device Management Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_module_name(
    std::string_view& name,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+NAME:";
    constexpr std::size_t DEVICE_NAME_MAX_LENGTH = 12;
    constexpr std::size_t MAX_RESPONSE_SIZE = EXPECT_PREFIX.size() + DEVICE_NAME_MAX_LENGTH;

    // Transmit query command
    transmit("AT+NAME?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response (variable length name, up to 12 characters)
    hal::UartData8b buffer(response_buffer_.data(), MAX_RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse name string (everything after "OK+NAME:" prefix, up to 12 chars)
    // Find null terminator or CRLF or end of buffer
    std::string_view full_response = util::to_string_view(buffer);

    // Skip prefix
    if (full_response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    std::string_view name_part = full_response.substr(EXPECT_PREFIX.size());

    // Find CRLF or null terminator (CRLF takes precedence)
    std::size_t end_pos = name_part.find('\r');
    if (end_pos == std::string_view::npos) {
        end_pos = name_part.find('\0');
    }

    if (end_pos == std::string_view::npos) {
        // No terminator found, use up to MAX_NAME_LENGTH
        name = name_part.substr(0, MAX_NAME_LENGTH);
    } else {
        // Found terminator, use substring up to terminator
        name = name_part.substr(0, end_pos);
    }

    // Validate name length (should be 1-12 characters)
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        status = hal::UartStatus::ErrError;
        return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_module_name(
    std::string_view name,
    hal::UartStatus& status) noexcept {
    // Precondition check: name length must be 1-12 characters
    // Ada: with Pre => (Name'Length in 1 .. 12);
    assert(!name.empty() && name.size() <= 12 && "Module name must be 1-12 characters");

    if (name.empty() || name.size() > 12) {
        status = hal::UartStatus::ErrError;
        return;
    }

    util::CommandBuilder cmd;
    cmd.append("AT+NAME").append(name);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(name);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_mac_address(
    types::MacAddress& mac,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+ADDR:";
    constexpr std::size_t MAC_SIZE = 12;
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + MAC_SIZE;

    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+ADDR?", EXPECT_PREFIX, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto* mac_start = buffer.data() + EXPECT_PREFIX.size();
        std::string_view mac_view = util::to_string_view(std::span(mac_start, MAC_SIZE));
        mac = types::MacAddress{mac_view};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::query_battery_information(
    types::Percent& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t MAX_BATTERY_DIGITS = 3; // "100" is max
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + MAX_BATTERY_DIGITS;

    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+BATT?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Parse battery percentage from response
        // Find null terminator or CRLF to determine actual length
        std::string_view full_response = util::to_string_view(buffer);

        // Skip prefix
        if (full_response.size() <= OK_GET.size()) {
            status = hal::UartStatus::ErrError;
            return;
        }

        std::string_view value_part = full_response.substr(OK_GET.size());

        // Find CRLF or null terminator
        std::size_t end_pos = value_part.find('\r');
        if (end_pos == std::string_view::npos) {
            end_pos = value_part.find('\0');
        }

        if (end_pos != std::string_view::npos) {
            value_part = value_part.substr(0, end_pos);
        }

        // Parse as integer
        auto parsed_value = util::parse_int<int>(value_part);
        if (!parsed_value || *parsed_value < 0 || *parsed_value > 100) {
            status = hal::UartStatus::ErrError;
            return;
        }

        result = types::Percent{static_cast<uint8_t>(*parsed_value)};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_battery_information(
    types::Percent value,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+BATT";

    util::CommandBuilder cmd;
    cmd.append("AT+BATT").append(value);

    // Response is "OK+BATT" without the value echoed
    constexpr std::size_t MAX_RESPONSE_DIGITS = 3;
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + MAX_RESPONSE_DIGITS;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check(cmd.get(), EXPECT_PREFIX, buffer, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_battery_monitor_switch(
    bool enable,
    hal::UartStatus& status) noexcept {
    const char VALUE_CHAR = enable ? '1' : '0';

    util::CommandBuilder cmd;
    cmd.append("AT+BATC").append(VALUE_CHAR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(VALUE_CHAR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_battery_monitor_switch(
    bool& enabled,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+BATC?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto VALUE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        enabled = (VALUE_CHAR == '1');
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_module_temperature(
    std::string_view& temperature,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t MAX_TEMP_LENGTH = 7; // Ada: Internal_Temperature_Type'Length = 7
    constexpr std::size_t MAX_RESPONSE_SIZE = EXPECT_PREFIX.size() + MAX_TEMP_LENGTH;

    // Transmit query command
    transmit("AT+TEMP?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response (variable length temperature)
    hal::UartData8b buffer(response_buffer_.data(), MAX_RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse temperature string (everything after "OK+Get:" prefix)
    std::string_view full_response = util::to_string_view(buffer);

    // Skip prefix
    if (full_response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    std::string_view temp_part = full_response.substr(EXPECT_PREFIX.size());

    // Find CRLF or null terminator
    std::size_t end_pos = temp_part.find('\r');
    if (end_pos == std::string_view::npos) {
        end_pos = temp_part.find('\0');
    }

    if (end_pos == std::string_view::npos) {
        // No terminator found, use up to MAX_TEMP_LENGTH
        temperature = temp_part.substr(0, MAX_TEMP_LENGTH);
    } else {
        // Found terminator, use substring up to terminator
        temperature = temp_part.substr(0, end_pos);
    }

    // Validate temperature is not empty
    if (temperature.empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_temperature_information_byte_in_advertising(
    const types::AdvertisingTemperatureType& temperature,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+TEMP0x").append(temperature);

    util::CommandBuilder expected;
    expected.append("OK+Set:0x").append(temperature);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_humi_information_byte_in_advertising(
    const types::HumiInformation& humidity,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+HUMI").append(humidity);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(humidity);

    transmit_and_check(cmd.get(), expected.get(), status);
}

} // namespace hm11
