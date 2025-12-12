/**
 * @file hm11_driver_ibeacon.inl
 * @brief Inline template implementations for iBeacon configuration operations
 *
 * This file contains the template implementations for iBeacon configuration
 * operations. It is included at the end of hm11_driver.hpp to provide
 * template definitions without requiring explicit instantiation.
 *
 * iBeacon operations configure the HM-11 module to broadcast iBeacon-compatible
 * advertising packets for proximity sensing applications. These operations
 * follow the individual setter pattern (Decision 14), where each configuration
 * parameter is independently configurable via separate AT commands.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

namespace hm11 {

// ============================================================================
// iBeacon Configuration Operations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_switch(
    bool switch_on,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1644-1652
    // AT command: AT+IBEA{0|1}
    // Response: OK+Set:{0|1}

    // Convert boolean to "0" or "1" string
    std::string_view switch_str = switch_on ? "1" : "0";

    util::CommandBuilder cmd;
    cmd.append("AT+IBEA").append(switch_str);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(switch_str);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_ibeacon_switch(
    bool& switch_on,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1658-1675
    // AT command: AT+IBEA?
    // Response: OK+Get:{0|1}

    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 1; // prefix + 1 char

    // Transmit query command
    transmit("AT+IBEA?", status);
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

    // Parse boolean value (character '0' or '1')
    const char value_char = static_cast<char>(buffer.data()[EXPECT_PREFIX.size()]);
    switch_on = (value_char == '1');
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_deploy_mode(
    types::IBeaconDeployMode mode,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1162-1171
    // AT command: AT+DELO{1|2}
    // Response: OK+DELO{1|2}
    // Note: Module resets after 500ms and enters non-connectable state

    util::CommandBuilder cmd;
    cmd.append("AT+DELO").append(util::enum_to_string(mode));

    util::CommandBuilder expected;
    expected.append("OK+DELO").append(util::enum_to_string(mode));

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_uuid(
    const types::IBeaconUuid& uuid,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1681-1704
    // AT command: AT+IBE{0-3}0x{8-char hex} (sent 4 times for 32-char UUID)
    // Response: OK+Set:{0-3}0x{8-char hex}
    // UUID sent in 4 chunks of 8 characters each

    // Helper lambda to set one 8-character chunk
    auto set_chunk = [this, &uuid, &status](std::size_t chunk_index) {
        // Extract 8-character chunk from uuid
        const std::size_t CHUNK_SIZE = 8;
        const std::size_t offset = chunk_index * CHUNK_SIZE;
        std::string_view chunk = uuid.view().substr(offset, CHUNK_SIZE);

        // Build command: AT+IBE{0-3}0x{8-char}
        util::CommandBuilder cmd;
        cmd.append("AT+IBE");
        if (chunk_index == 0) {
            cmd.append('0');
        } else if (chunk_index == 1) {
            cmd.append('1');
        } else if (chunk_index == 2) {
            cmd.append('2');
        } else {
            cmd.append('3');
        }
        cmd.append("0x").append(chunk);

        // Build expected response: OK+Set:{0-3}0x{8-char}
        util::CommandBuilder expected;
        expected.append("OK+Set:");
        if (chunk_index == 0) {
            expected.append('0');
        } else if (chunk_index == 1) {
            expected.append('1');
        } else if (chunk_index == 2) {
            expected.append('2');
        } else {
            expected.append('3');
        }
        expected.append("0x").append(chunk);

        transmit_and_check(cmd.get(), expected.get(), status);
    };

    // Send all 4 chunks, stop on first error
    for (std::size_t i = 0; i < 4 && status == hal::UartStatus::Ok; ++i) {
        set_chunk(i);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_ibeacon_uuid(
    types::IBeaconUuid& uuid,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1710-1748
    // AT command: AT+IBE{0-3}? (sent 4 times)
    // Response: OK+Get:0x{8-char hex}

    // Temporary buffer to accumulate UUID chunks
    std::array<char, 32> uuid_buffer{};

    // Helper lambda to get one 8-character chunk
    auto get_chunk = [this, &uuid_buffer, &status](std::size_t chunk_index) {
        constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
        constexpr std::size_t CHUNK_SIZE = 8;
        // Response: "OK+Get:" + "0x" + 8 chars = 18 bytes
        constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 2 + CHUNK_SIZE;

        // Build command: AT+IBE{0-3}?
        util::CommandBuilder cmd;
        cmd.append("AT+IBE");
        if (chunk_index == 0) {
            cmd.append('0');
        } else if (chunk_index == 1) {
            cmd.append('1');
        } else if (chunk_index == 2) {
            cmd.append('2');
        } else {
            cmd.append('3');
        }
        cmd.append('?');

        // Transmit query
        transmit(cmd.get(), status);
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

        // Extract 8-character hex chunk (skip "OK+Get:0x" prefix)
        const std::size_t HEX_OFFSET = EXPECT_PREFIX.size() + 2; // Skip "OK+Get:" and "0x"
        const std::size_t chunk_start = chunk_index * CHUNK_SIZE;

        // Copy 8 characters to the appropriate position in uuid_buffer
        for (std::size_t i = 0; i < CHUNK_SIZE; ++i) {
            uuid_buffer[chunk_start + i] = static_cast<char>(buffer.data()[HEX_OFFSET + i]);
        }
    };

    // Get all 4 chunks, stop on first error
    for (std::size_t i = 0; i < 4 && status == hal::UartStatus::Ok; ++i) {
        get_chunk(i);
    }

    // If successful, construct UUID from buffer
    if (status == hal::UartStatus::Ok) {
        std::string_view uuid_str(uuid_buffer.data(), uuid_buffer.size());
        uuid = types::IBeaconUuid{uuid_str};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_major_version(
    const types::VersionType& version,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1754-1764
    // AT command: AT+MARJ0x{4-char hex}
    // Response: OK+Set:0x{4-char hex}

    util::CommandBuilder cmd;
    cmd.append("AT+MARJ0x").append(version);

    util::CommandBuilder expected;
    expected.append("OK+Set:0x").append(version);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_ibeacon_major_version(
    types::VersionType& version,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1770-1788
    // AT command: AT+MARJ?
    // Response: OK+Get:0x{4-char hex}

    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t VERSION_SIZE = 4;
    // Response: "OK+Get:" + "0x" + 4 chars = 14 bytes
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 2 + VERSION_SIZE;

    // Transmit query
    transmit("AT+MARJ?", status);
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

    // Extract 4-character version (skip "OK+Get:0x" prefix)
    const std::size_t HEX_OFFSET = EXPECT_PREFIX.size() + 2; // Skip "OK+Get:" and "0x"
    std::string_view version_str = util::to_string_view(std::span(buffer.data() + HEX_OFFSET, VERSION_SIZE));

    version = types::VersionType{version_str};
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_minor_version(
    const types::VersionType& version,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1794-1804
    // AT command: AT+MINO0x{4-char hex}
    // Response: OK+Set:0x{4-char hex}

    util::CommandBuilder cmd;
    cmd.append("AT+MINO0x").append(version);

    util::CommandBuilder expected;
    expected.append("OK+Set:0x").append(version);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_ibeacon_minor_version(
    types::VersionType& version,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1810-1828
    // AT command: AT+MINO?
    // Response: OK+Get:0x{4-char hex}

    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t VERSION_SIZE = 4;
    // Response: "OK+Get:" + "0x" + 4 chars = 14 bytes
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 2 + VERSION_SIZE;

    // Transmit query
    transmit("AT+MINO?", status);
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

    // Extract 4-character version (skip "OK+Get:0x" prefix)
    const std::size_t HEX_OFFSET = EXPECT_PREFIX.size() + 2; // Skip "OK+Get:" and "0x"
    std::string_view version_str = util::to_string_view(std::span(buffer.data() + HEX_OFFSET, VERSION_SIZE));

    version = types::VersionType{version_str};
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_ibeacon_measured_power(
    const types::MeasuredPower& power,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1834-1844
    // AT command: AT+MEAS0x{2-char hex}
    // Response: OK+Set:0x{2-char hex}

    util::CommandBuilder cmd;
    cmd.append("AT+MEAS0x").append(power);

    util::CommandBuilder expected;
    expected.append("OK+Set:0x").append(power);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_ibeacon_measured_power(
    types::MeasuredPower& power,
    hal::UartStatus& status) noexcept {
    // Ada: hm11.adb:1850-1868
    // AT command: AT+MEAS?
    // Response: OK+Get:0x{2-char hex}

    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";
    constexpr std::size_t POWER_SIZE = 2;
    // Response: "OK+Get:" + "0x" + 2 chars = 12 bytes
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + 2 + POWER_SIZE;

    // Transmit query
    transmit("AT+MEAS?", status);
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

    // Extract 2-character power (skip "OK+Get:0x" prefix)
    const std::size_t HEX_OFFSET = EXPECT_PREFIX.size() + 2; // Skip "OK+Get:" and "0x"
    std::string_view power_str = util::to_string_view(std::span(buffer.data() + HEX_OFFSET, POWER_SIZE));

    power = types::MeasuredPower{power_str};
}

} // namespace hm11
