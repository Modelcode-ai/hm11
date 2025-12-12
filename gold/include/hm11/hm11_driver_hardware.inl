/**
 * @file hm11_driver_hardware.inl
 * @brief Inline template implementations for hardware control operations
 *
 * This file contains the template implementations for hardware pin control and
 * power management operations (PIO control, power settings, work modes, notifications).
 * It is included at the end of hm11_driver.hpp to provide template definitions without
 * requiring explicit instantiation.
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
// PIO Pin Control Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_pio_output_status(
    types::PioNumber pio,
    types::PioOutput output,
    hal::UartStatus& status) noexcept {
    // PIO1 has different command format than PIO2-3
    // Ada: if PIO = 1 then AT+PIO1{0-1}, else AT+PIO{2-3}{0-1}
    const auto pio_str = std::to_string(pio.get());
    const auto output_str = util::enum_to_string(output);
    
    util::CommandBuilder cmd;
    util::CommandBuilder expected;
    
    if (pio.get() == 1) {
        cmd.append("AT+PIO1").append(output_str);
        expected.append("OK+Set:").append(output_str);
    } else {
        cmd.append("AT+PIO").append(pio_str).append(output_str);
        expected.append("OK+PIO").append(pio_str).append(":").append(output_str);
    }
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_pio_output_status(
    types::PioNumber pio,
    types::PioOutput& output,
    hal::UartStatus& status) noexcept {
    const auto pio_str = std::to_string(pio.get());

    util::CommandBuilder cmd;
    util::CommandBuilder prefix_builder;  // Moved to outer scope for lifetime safety
    std::string_view expect_prefix;
    std::size_t response_size;

    if (pio.get() == 1) {
        cmd.append("AT+PIO1?");
        expect_prefix = "OK+Get:";
        response_size = 8; // "OK+Get:0"
    } else {
        cmd.append("AT+PIO").append(pio_str).append("?");
        prefix_builder.append("OK+PIO").append(pio_str).append(":");
        expect_prefix = prefix_builder.get();
        response_size = expect_prefix.size() + 1;
    }
    
    transmit(cmd.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), response_size);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, expect_prefix, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Parse output value (0 or 1)
    const auto value_char = static_cast<char>(buffer[expect_prefix.size()]);
    const auto value = util::char_to_digit(value_char);
    
    if (value <= 1) {
        output = static_cast<types::PioOutput>(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_pios_output_status(
    const types::PioNumbers& pios,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+MPIO").append(pios.view());
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(pios.view());
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_pios_output_status(
    types::PioNumbers& pios,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+PIO?";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 3; // "OK+PIO?000"
    
    transmit("AT+PIO??", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Extract 3-character hex string
    std::string_view hex_str = util::to_string_view(std::span(buffer.data() + EXPECT_PREFIX.size(), 3));
    pios = types::PioNumbers(hex_str);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_connect_pin_output(
    const types::PioNumbers& value,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+AFTC").append(value.view());
    
    util::CommandBuilder expected;
    expected.append("OK+Get:").append(value.view());
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_connect_pin_output(
    types::PioNumbers& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 3;
    
    transmit("AT+AFTC?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    std::string_view hex_str = util::to_string_view(std::span(buffer.data() + EXPECT_PREFIX.size(), 3));
    result = types::PioNumbers(hex_str);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_power_pin_output(
    const types::PioNumbers& value,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+BEFC").append(value.view());
    
    util::CommandBuilder expected;
    expected.append("OK+Get:").append(value.view());
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_power_pin_output(
    types::PioNumbers& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 3;
    
    transmit("AT+BEFC?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    std::string_view hex_str = util::to_string_view(std::span(buffer.data() + EXPECT_PREFIX.size(), 3));
    result = types::PioNumbers(hex_str);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_pio_collection_rate(
    types::PioCollectionRate value,
    hal::UartStatus& status) noexcept {
    // Format as 2-digit zero-padded string (00-99)
    const auto value_str = std::to_string(value.get());
    const auto padded = (value.get() < 10) ? "0" + value_str : value_str;
    
    util::CommandBuilder cmd;
    cmd.append("AT+CYC").append(padded);
    
    util::CommandBuilder expected;
    expected.append("OK+Get:").append(padded);
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_pio_collection_rate(
    types::PioCollectionRate& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 2; // Max 2 digits
    
    transmit("AT+CYC??", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse 1-2 digit number (variable length response)
    // Hardware returns "OK+Get:5\0" for 1-digit or "OK+Get:99" for 2-digit values
    // Ada implementation: checks byte after first digit for null terminator to determine length
    const uint8_t* digit_start = buffer.data() + EXPECT_PREFIX.size();
    std::size_t digit_count = (digit_start[1] == 0) ? 1 : 2;

    // Parse the actual number of digits present
    std::string_view number_str = util::to_string_view(std::span(digit_start, digit_count));
    uint8_t value = 0;
    const auto [ptr, ec] = std::from_chars(number_str.data(), number_str.data() + number_str.size(), value);

    if (ec == std::errc() && value <= 99) {
        result = types::PioCollectionRate(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// ============================================================================
// Power Management Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_output_power(
    types::OutputPower power,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+PCTL").append(util::enum_to_string(power));
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(power));
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_output_power(
    types::OutputPower& power,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+PCTL?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    const auto value = util::char_to_digit(value_char);
    
    if (value <= 1) {
        power = static_cast<types::OutputPower>(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_module_power(
    types::ModulePower power,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+POWE").append(util::enum_to_string(power));
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(power));
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_module_power(
    types::ModulePower& power,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+POWE?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    const auto value = util::char_to_digit(value_char);

    // Map digit to enum value (0→-23dBm, 1→-6dBm, 2→0dBm, 3→+6dBm)
    switch (value) {
        case 0:
            power = types::ModulePower::DbmMinus23;
            break;
        case 1:
            power = types::ModulePower::DbmMinus6;
            break;
        case 2:
            power = types::ModulePower::Dbm0;
            break;
        case 3:
            power = types::ModulePower::Dbm6;
            break;
        default:
            status = hal::UartStatus::ErrError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_module_rx_gain(
    types::RxGain gain,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+GAIN").append(util::enum_to_string(gain));
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(gain));
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_module_rx_gain(
    types::RxGain& gain,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+GAIN?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    const auto value = util::char_to_digit(value_char);
    
    if (value <= 1) {
        gain = static_cast<types::RxGain>(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_module_auto_sleep(
    bool sleep,
    hal::UartStatus& status) noexcept {
    // Note: Inverted logic - sleep=true sends "0", sleep=false sends "1"
    const std::string_view value = sleep ? "0" : "1";
    
    util::CommandBuilder cmd;
    cmd.append("AT+PWRM").append(value);
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(value);
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_module_auto_sleep(
    bool& sleep,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+PWRM?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Inverted: '0' = enabled, '1' = disabled
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    sleep = (value_char == '0');
}

// ============================================================================
// Work Mode and Notification Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_notify_information(
    bool notify,
    hal::UartStatus& status) noexcept {
    const std::string_view value = notify ? "1" : "0";
    
    util::CommandBuilder cmd;
    cmd.append("AT+NOTI").append(value);
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(value);
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_notify_information(
    bool& notify,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+NOTI?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    notify = (value_char == '1');
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_notify_mode(
    types::NotifyMode mode,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+NOTP").append(util::enum_to_string(mode));
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(mode));
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_notify_mode(
    types::NotifyMode& mode,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+NOTP?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    const auto value = util::char_to_digit(value_char);
    
    if (value <= 1) {
        mode = static_cast<types::NotifyMode>(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_work_mode(
    types::WorkMode mode,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+MODE").append(util::enum_to_string(mode));
    
    util::CommandBuilder expected;
    expected.append("OK+Set:").append(util::enum_to_string(mode));
    
    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_work_mode(
    types::WorkMode& mode,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1;
    
    transmit("AT+MODE?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    const auto value_char = static_cast<char>(buffer[EXPECT_PREFIX.size()]);
    const auto value = util::char_to_digit(value_char);
    
    if (value <= 2) {
        mode = static_cast<types::WorkMode>(value);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

} // namespace hm11
