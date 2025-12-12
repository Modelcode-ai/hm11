/**
 * @file hm11_driver_security.inl
 * @brief Template implementation for HM11Driver security and authentication operations
 *
 * This file contains inline template implementations for BLE security operations
 * including bond mode configuration, PIN code management, work type settings,
 * white list management, and service UUID configuration. These operations enable
 * authenticated pairing, access control, and secure connectivity.
 *
 * ## Operations Covered
 *
 * - **Bond Mode**: Configure pairing and authentication requirements
 * - **PIN Code**: Set and query 6-digit pairing PIN
 * - **Work Type**: Control module startup behavior (critical for Central scanning)
 * - **White List**: MAC address filtering (up to 3 addresses)
 * - **Remove Bond**: Clear all pairing data
 * - **Service UUID**: Configure advertised GATT service
 * - **UART Sleep**: Control UART behavior in sleep mode (HMSensor-specific)
 *
 * ## Design Patterns
 *
 * All operations follow standard patterns established in other driver modules:
 * - Out-parameter status reporting (Decision 4)
 * - AT command construction using enum mappings (Decision 5)
 * - Zero-copy parsing with string_view (Decision 6)
 * - No internal state caching (Decision 7)
 * - Document preconditions, no runtime validation (Decision 8)
 *
 * ## Security Architecture
 *
 * The security operations enable multiple layers of access control:
 * 1. **Bond Mode** determines if pairing is required and what type
 * 2. **PIN Code** provides authentication when bond mode requires it
 * 3. **White List** restricts connections to known MAC addresses
 * 4. **Work Type** controls whether module auto-connects (security consideration)
 *
 * For production deployments, recommend:
 * - AuthAndBonded bond mode for persistent pairing
 * - Non-default PIN code (not "000000")
 * - White list enabled with known device MACs
 * - RespondAtCommand work type for Central role to prevent auto-connect
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "hm11/hal/base_types.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"
#include "hm11/util/command_builder.hpp"
#include "hm11/util/enum_mapping.hpp"
#include "hm11/util/parsing_utils.hpp"

namespace hm11 {

// =============================================================================
// Bond Mode Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_bond_mode(types::BondMode mode, hal::UartStatus& status) noexcept {
    // Build command: AT+TYPE{0-3}
    constexpr std::string_view CMD_PREFIX = "AT+TYPE";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(util::enum_to_string(mode));

    // Build expected response
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(util::enum_to_string(mode));

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_bond_mode(types::BondMode& mode, hal::UartStatus& status) noexcept {
    // Send command: AT+TYPE?
    // Response: OK+Get:{0-3}
    constexpr std::string_view CMD = "AT+TYPE?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_LEN = 8; // "OK+Get:X"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Parse bond mode from character after "OK+Get:" (position 7)
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            char mode_char = response_view[EXPECT_PREFIX.size()];
            uint8_t mode_value = char_to_digit(mode_char);
            mode = static_cast<types::BondMode>(mode_value);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// PIN Code Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_pin_code(
    const types::PinType& pin,
    hal::UartStatus& status) noexcept {
    // Build command: AT+PASS{6-digit PIN}
    constexpr std::string_view CMD_PREFIX = "AT+PASS";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(pin.view());

    // Build expected response
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(pin.view());

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_pin_code(types::PinType& pin, hal::UartStatus& status) noexcept {
    // Send command: AT+PASS?
    // Response: OK+Get:{6-digit PIN}
    constexpr std::string_view CMD = "AT+PASS?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t PIN_LENGTH = 6;
    constexpr std::size_t RESPONSE_LEN = EXPECT_PREFIX.size() + PIN_LENGTH; // "OK+Get:XXXXXX"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Extract PIN code from response (6 characters after "OK+Get:")
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            std::string_view pin_view = response_view.substr(EXPECT_PREFIX.size(), PIN_LENGTH);
            pin = types::PinType(pin_view);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// Work Type Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_work_type(types::WorkType work, hal::UartStatus& status) noexcept {
    // Build command: AT+IMME{0-1}
    constexpr std::string_view CMD_PREFIX = "AT+IMME";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(util::enum_to_string(work));

    // Build expected response
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(util::enum_to_string(work));

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_work_type(types::WorkType& work, hal::UartStatus& status) noexcept {
    // Send command: AT+IMME?
    // Response: OK+Get:{0-1}
    constexpr std::string_view CMD = "AT+IMME?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_LEN = 8; // "OK+Get:X"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Parse work type from character after "OK+Get:" (position 7)
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            char work_char = response_view[EXPECT_PREFIX.size()];
            uint8_t work_value = char_to_digit(work_char);
            work = static_cast<types::WorkType>(work_value);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// White List Switch Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_white_list_switch(bool enable, hal::UartStatus& status) noexcept {
    // Build command: AT+ALLO{0|1}
    constexpr std::string_view CMD_PREFIX = "AT+ALLO";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(util::bool_to_string(enable));

    // Build expected response
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(util::bool_to_string(enable));

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_white_list_switch(bool& enabled, hal::UartStatus& status) noexcept {
    // Send command: AT+ALLO?
    // Response: OK+Get:{0|1}
    constexpr std::string_view CMD = "AT+ALLO?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_LEN = 8; // "OK+Get:X"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Parse enabled state from character after "OK+Get:" (position 7)
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            char enabled_char = response_view[EXPECT_PREFIX.size()];
            enabled = (enabled_char == '1');
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// White List MAC Address Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_white_list_mac_address(
    types::MacWhiteListIndex index,
    const types::MacAddress& mac_address,
    hal::UartStatus& status) noexcept {
    // Build command: AT+AD{1-3}{MAC}
    constexpr std::string_view CMD_PREFIX = "AT+AD";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(index);
    builder.append(mac_address.view());

    // Build expected response: OK+Set:{MAC}
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(mac_address.view());

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_white_list_mac_address(
    types::MacWhiteListIndex index,
    types::MacAddress& mac_address,
    hal::UartStatus& status) noexcept {
    // Send command: AT+AD{1-3}??
    // Response: OK+AD{1-3}?:{MAC}
    constexpr std::string_view CMD_PREFIX = "AT+AD";
    constexpr std::string_view CMD_SUFFIX = "??";
    constexpr std::string_view EXPECT_PREFIX_PART1 = "OK+AD";
    constexpr std::string_view EXPECT_PREFIX_PART2 = "?:";
    constexpr std::size_t MAC_LENGTH = 12;

    util::CommandBuilder cmd_builder;
    cmd_builder.append(CMD_PREFIX);
    cmd_builder.append(index);
    cmd_builder.append(CMD_SUFFIX);

    // Build expected prefix: "OK+AD{1-3}?:"
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX_PART1);
    expect_builder.append(index);
    expect_builder.append(EXPECT_PREFIX_PART2);

    const std::string_view expect_prefix = expect_builder.get();
    constexpr std::size_t MAX_RESPONSE_LEN = 20; // "OK+AD3?:XXXXXXXXXXXX" is max 20 chars

    std::array<uint8_t, MAX_RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), MAX_RESPONSE_LEN);

    transmit_and_check(cmd_builder.get(), expect_prefix, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Extract MAC address from response (12 characters after "OK+AD{1-3}?:")
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= expect_prefix.size() + MAC_LENGTH) {
            std::string_view mac_view = response_view.substr(expect_prefix.size(), MAC_LENGTH);
            mac_address = types::MacAddress(mac_view);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// Remove Bond Information
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::remove_bond_information(hal::UartStatus& status) noexcept {
    // Send command: AT+ERASE
    // Response: OK+ERASE
    constexpr std::string_view CMD = "AT+ERASE";
    constexpr std::string_view EXPECT = "OK+ERASE";

    transmit_and_check(CMD, EXPECT, response_buffer_, status);
}

// =============================================================================
// Service UUID Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_service_uuid(
    const types::UUID& service_uuid,
    hal::UartStatus& status) noexcept {
    // Build command: AT+UUID0x{UUID}
    constexpr std::string_view CMD_PREFIX = "AT+UUID0x";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:0x";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(service_uuid.view());

    // Build expected response: OK+Set:0x{UUID}
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(service_uuid.view());

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_service_uuid(
    types::UUID& service_uuid,
    hal::UartStatus& status) noexcept {
    // Send command: AT+UUID?
    // Response: OK+Get:0x{UUID}
    constexpr std::string_view CMD = "AT+UUID?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:0x";
    constexpr std::size_t UUID_LENGTH = 4;
    constexpr std::size_t RESPONSE_LEN = EXPECT_PREFIX.size() + UUID_LENGTH; // "OK+Get:0xXXXX"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Extract UUID from response (4 characters after "OK+Get:0x")
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            std::string_view uuid_view = response_view.substr(EXPECT_PREFIX.size(), UUID_LENGTH);
            service_uuid = types::UUID(uuid_view);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// =============================================================================
// UART Sleep Type Operations
// =============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_uart_sleep_type(
    types::UartSleepType sleep_type,
    hal::UartStatus& status) noexcept {
    // Build command: AT+UART{0-1}
    constexpr std::string_view CMD_PREFIX = "AT+UART";
    constexpr std::string_view EXPECT_PREFIX = "OK+Set:";

    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(util::enum_to_string(sleep_type));

    // Build expected response
    util::CommandBuilder expect_builder;
    expect_builder.append(EXPECT_PREFIX);
    expect_builder.append(util::enum_to_string(sleep_type));

    // Transmit and check response
    transmit_and_check(builder.get(), expect_builder.get(), response_buffer_, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_uart_sleep_type(
    types::UartSleepType& sleep_type,
    hal::UartStatus& status) noexcept {
    // Send command: AT+UART?
    // Response: OK+Get:{0-1}
    constexpr std::string_view CMD = "AT+UART?";
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_LEN = 8; // "OK+Get:X"

    std::array<uint8_t, RESPONSE_LEN> buffer{};
    hal::UartData8b receive_buffer(buffer.data(), RESPONSE_LEN);

    transmit_and_check(CMD, EXPECT_PREFIX, receive_buffer, status);

    if (status == hal::UartStatus::Ok) {
        // Parse UART sleep type from character after "OK+Get:" (position 7)
        std::string_view response_view = util::to_string_view(receive_buffer);
        if (response_view.size() >= RESPONSE_LEN) {
            char sleep_char = response_view[EXPECT_PREFIX.size()];
            uint8_t sleep_value = char_to_digit(sleep_char);
            sleep_type = static_cast<types::UartSleepType>(sleep_value);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

} // namespace hm11
