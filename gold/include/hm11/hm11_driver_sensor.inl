/**
 * @file hm11_driver_sensor.inl
 * @brief Inline template implementations for HMSensor-specific operations
 *
 * This file contains the template implementations for HMSensor variant-specific
 * operations (sensor type configuration, temperature/humidity readings, work
 * interval). It is included at the end of hm11_driver.hpp to provide template
 * definitions without requiring explicit instantiation.
 *
 * ## Overview (Decision 15: HMSensor-Specific Features)
 *
 * HMSensor operations enable applications using the HMSensor variant to read
 * environmental data and configure sensor behavior. These operations are:
 * - Always present in the API (no conditional compilation)
 * - Documented as "HMSensor-only" in API documentation
 * - Hardware returns error codes if called on non-HMSensor modules
 *
 * Sensor operations supported:
 * - Sensor type configuration (None, DHT11, DS18B20)
 * - Temperature and humidity readings (parsed to numeric uint8_t values)
 * - Work interval configuration (how often sensor takes readings, in minutes)
 *
 * ## Implementation Pattern
 *
 * All sensor operations follow the standard AT command pattern:
 * 1. Set operations: Send "AT+CMD{value}", expect "OK+Set:{value}"
 * 2. Get operations: Send "AT+CMD?", expect "OK+Get:{value}", parse result
 *
 * ## Numeric Parsing (Decision 29: Sensor Reading Value Representation)
 *
 * Temperature and humidity readings are parsed into numeric uint8_t values
 * matching Ada's approach:
 * - Temperature: uint8_t (0-120 range, Natural in Ada)
 * - Humidity: uint8_t (0-100 range, Natural in Ada)
 * - Decimal values (e.g., "25.5") are truncated to integers (25)
 * - Uses std::from_chars for parsing (zero-copy, efficient)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <charconv>
#include <cstdint>

#include "hm11/util/enum_mapping.hpp"
#include "hm11/util/parsing_utils.hpp"

namespace hm11 {

// ============================================================================
// HMSensor-Specific Operations (Decision 15)
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_sensor_type(
    types::SensorType sensor_type,
    hal::UartStatus& status) noexcept {
    // Build command using enum_to_string mapping (Decision 5: AT Command Construction)
    // Ada: Transmit_And_Check (This, "AT+SENS" & S, Ok_Set & S, Status);
    // where S := Image (Sensor_Type'Pos (Value))
    util::CommandBuilder cmd;
    cmd.append("AT+SENS").append(util::enum_to_string(sensor_type));

    // Build expected response
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(sensor_type));

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_sensor_type(
    types::SensorType& sensor_type,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 digit

    // Transmit query command
    // Ada: Transmit_And_Check (This, "AT+SENS?", Ok_Get, ...)
    transmit("AT+SENS?", status);
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
    // Ada: Result := Sensor_Type'Val (Value (This.Responce (Ok_Get'Length + 1)))
    std::string_view response = util::to_string_view(buffer);

    if (response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char value_char = response[EXPECT_PREFIX.size()];

    // Map response character to enum value (0-2)
    // Response is ASCII digit '0'-'2'
    if (value_char < '0' || value_char > '2') {
        status = hal::UartStatus::ErrError;
        return;
    }

    uint8_t enum_ordinal = static_cast<uint8_t>(value_char - '0');

    // Convert ordinal to enum
    switch (enum_ordinal) {
        case 0:
            sensor_type = types::SensorType::None;
            break;
        case 1:
            sensor_type = types::SensorType::DHT11;
            break;
        case 2:
            sensor_type = types::SensorType::DS18B20;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_sensor_temperature_and_humidity(
    uint8_t& temperature,
    uint8_t& humidity,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 7; // "OK+Get:" + "TTT:HHH" (3+1+3)

    // Initialize outputs to safe defaults
    temperature = 0;
    humidity = 0;

    // Transmit query command
    // Ada: Transmit_And_Check (This, "AT+TEHU?", Ok_Get, ...)
    transmit("AT+TEHU?", status);
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

    // Parse temperature and humidity from response
    // Ada code (hm11.adb:2495-2504):
    //   T : String (1 .. 3) with Import, Address => This.Responce (Ok_Get'Length + 1)'Address;
    //   H : String (1 .. 3) with Import, Address => This.Responce (Ok_Get'Length + 4)'Address;
    //   Temperature := Temperature_Type'Value (T);
    //   Humidity    := Humidity_Type'Value (H);
    //
    // Response format: "OK+Get:TTT:HHH" where TTT and HHH are numeric strings (Decision 29)

    std::string_view response = util::to_string_view(buffer);

    if (response.size() < RESPONSE_SIZE) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Extract temperature string (3 characters starting at position 7)
    std::string_view temp_str = response.substr(EXPECT_PREFIX.size(), 3);

    // Extract humidity string (3 characters starting at position 11 = 7 + 3 + 1 for ':')
    std::string_view humid_str = response.substr(EXPECT_PREFIX.size() + 4, 3);

    // Parse temperature using std::from_chars (Decision 6: Response Parsing)
    // Handles both integer ("25") and decimal ("25.5") by truncating at decimal point
    uint16_t temp_value = 0;
    auto [temp_ptr, temp_ec] =
        std::from_chars(temp_str.data(), temp_str.data() + temp_str.size(), temp_value);
    if (temp_ec != std::errc{}) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Validate temperature range (0-120, matching Ada's Temperature_Type)
    if (temp_value > 120) {
        status = hal::UartStatus::ErrError;
        return;
    }
    temperature = static_cast<uint8_t>(temp_value);

    // Parse humidity using std::from_chars
    uint16_t humid_value = 0;
    auto [humid_ptr, humid_ec] =
        std::from_chars(humid_str.data(), humid_str.data() + humid_str.size(), humid_value);
    if (humid_ec != std::errc{}) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Validate humidity range (0-100, matching Ada's Humidity_Type)
    if (humid_value > 100) {
        status = hal::UartStatus::ErrError;
        return;
    }
    humidity = static_cast<uint8_t>(humid_value);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_sensor_work_interval(
    const types::WorkInterval& interval,
    hal::UartStatus& status) noexcept {
    // Build command based on interval format
    // Ada code (hm11.adb:266-281):
    //   if Interval (Interval'First) = '0' then
    //     -- Send single digit if leading zero
    //     C : constant Character := Character (Interval (Interval'Last));
    //     Transmit_And_Check (This, "AT+RAT" & C, Ok_Set & C, Status);
    //   else
    //     -- Send both digits
    //     Transmit_And_Check (This, "AT+RAT" & S, Ok_Set & S, Status);
    //   end if;

    util::CommandBuilder cmd;
    util::CommandBuilder expected;

    std::string_view interval_view = interval.view();

    if (interval_view[0] == '0') {
        // Leading zero: send only last digit
        char last_digit = interval_view[1];
        cmd.append("AT+RAT").append(last_digit);
        expected.append("OK+Set:").append(last_digit);
    } else {
        // Both digits non-zero: send both
        cmd.append("AT+RAT").append(interval_view);
        expected.append("OK+Set:").append(interval_view);
    }

    // Send command and validate response
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_sensor_work_interval(
    types::WorkInterval& interval,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t MIN_RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + at least 1 digit
    constexpr std::size_t MAX_RESPONSE_SIZE = EXPECT_PREFIX.size() + 2; // prefix + up to 2 digits

    // Initialize to default "00"
    interval = types::WorkInterval{"00"};

    // Transmit query command
    // Ada: Transmit_And_Check (This, "AT+RAT??", Ok_Get, ...)
    transmit("AT+RAT?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response (can be 1 or 2 digits)
    // Try to receive maximum size, but accept minimum size too
    hal::UartData8b buffer(response_buffer_.data(), MAX_RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse interval from response
    // Ada code (hm11.adb:238-253):
    //   if This.Responce (Ok_Get'Length + 2) = 0 then
    //     -- Single digit response, pad with leading zero
    //     L : Digit_Character with Import, Address => This.Responce (Ok_Get'Length + 1)'Address;
    //     Interval (Interval'Last) := L;
    //   else
    //     -- Two digit response
    //     L : Work_Interval with Import, Address => This.Responce (Ok_Get'Length + 1)'Address;
    //     Interval := L;
    //   end if;

    std::string_view response = util::to_string_view(buffer);

    // Check prefix manually (don't use check_response since we need flexible size)
    if (response.size() < MIN_RESPONSE_SIZE || !util::starts_with(response, EXPECT_PREFIX)) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Check if single or two digit response
    // Ada code checks: if This.Responce (Ok_Get'Length + 2) = 0 then (single digit)
    // This means: if the byte at position (prefix_length + 2) is null, it's single digit
    std::size_t digit_start = EXPECT_PREFIX.size();

    // Access raw buffer data to check for null byte (Ada pattern)
    bool is_single_digit = false;
    if (buffer.size() > digit_start + 1) {
        // Check if second position after prefix is null or invalid
        char second_char = response[digit_start + 1];
        is_single_digit = (second_char == '\0' || second_char < '0' || second_char > '9');
    } else {
        is_single_digit = true; // Only one digit in response
    }

    if (is_single_digit) {
        // Single digit: pad with leading zero "0X"
        char digit = response[digit_start];
        if (digit < '0' || digit > '9') {
            status = hal::UartStatus::ErrError;
            return;
        }
        char padded[3] = {'0', digit, '\0'};
        interval = types::WorkInterval{padded};
    } else {
        // Two digits: use both
        char digit1 = response[digit_start];
        char digit2 = response[digit_start + 1];
        if (digit1 < '0' || digit1 > '9' || digit2 < '0' || digit2 > '9') {
            status = hal::UartStatus::ErrError;
            return;
        }
        char both[3] = {digit1, digit2, '\0'};
        interval = types::WorkInterval{both};
    }
}

} // namespace hm11
