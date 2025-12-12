/**
 * @file integration_test.cpp
 * @brief Integration tests for HAL interface, command builder, and response parser
 *
 * These tests validate that the HAL interfaces, command construction utilities,
 * and response parsing utilities work together correctly. Integration tests
 * combine multiple components to verify end-to-end functionality.
 *
 * Test scenarios:
 * - Build commands using CommandBuilder with enum mappings
 * - Parse responses using parsing utilities
 * - Use mock UARTPort implementations to simulate hardware
 * - Verify command-response cycles with realistic HM-11 data
 * - Test error handling across component boundaries
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"
#include "hm11/util/command_builder.hpp"
#include "hm11/util/enum_mapping.hpp"
#include "hm11/util/parsing_utils.hpp"

using namespace hm11::hal;
using namespace hm11::types;
using namespace hm11::util;

// =============================================================================
// Mock UARTPort Implementation for Integration Testing
// =============================================================================

/**
 * @brief Mock UART port that stores transmitted data and can return canned responses
 *
 * This mock allows integration tests to:
 * 1. Build commands using CommandBuilder
 * 2. Transmit via the mock UART
 * 3. Verify transmitted commands
 * 4. Receive canned responses
 * 5. Parse responses using parsing utilities
 */
class MockUARTForIntegration {
  public:
    MockUARTForIntegration() = default;

    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    // Satisfies UARTPort concept - Must be const for concept
    [[nodiscard]] constexpr auto data_size() const noexcept -> UartDataSize { return UartDataSize::DataSize8b; }
    // NOLINTEND(readability-convert-member-functions-to-static)

    // Store transmitted data for verification
    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        status = UartStatus::Ok;
        m_last_transmitted_data.assign(data.begin(), data.end());
    }

    // Return canned response data
    void
    receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool /*as_stream*/ = false) noexcept {
        if (m_receive_data_buffer.empty()) {
            status = UartStatus::ErrTimeout;
            return;
        }

        const std::size_t BYTES_TO_COPY = std::min(data.size(), m_receive_data_buffer.size());
        std::copy_n(m_receive_data_buffer.begin(), BYTES_TO_COPY, data.begin());
        status = UartStatus::Ok;
    }

    // Test utilities
    void set_response(std::string_view response) { m_receive_data_buffer.assign(response.begin(), response.end()); }

    [[nodiscard]] auto get_last_transmitted() const -> std::string {
        return {m_last_transmitted_data.begin(), m_last_transmitted_data.end()};
    }

    void clear() {
        m_last_transmitted_data.clear();
        m_receive_data_buffer.clear();
    }

  private:
    std::vector<std::uint8_t> m_last_transmitted_data;
    std::vector<std::uint8_t> m_receive_data_buffer;
};

// Verify mock satisfies type trait
static_assert(IS_UART_PORT_V<MockUARTForIntegration>, "MockUARTForIntegration must satisfy UART port interface");

// =============================================================================
// Command Builder + Enum Mapping Integration Tests
// =============================================================================

TEST(IntegrationTest, BuildRoleQueryCommand) {
    CommandBuilder builder;
    builder.append("AT+ROLE?");

    EXPECT_EQ(builder.get(), "AT+ROLE?");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildRoleSetCommandPeripheral) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append(enum_to_string(Role::Peripheral));

    EXPECT_EQ(builder.get(), "AT+ROLE0");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildRoleSetCommandCentral) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append(enum_to_string(Role::Central));

    EXPECT_EQ(builder.get(), "AT+ROLE1");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildAdvertisingIntervalCommand) {
    CommandBuilder builder;
    builder.append("AT+ADVI").append(enum_to_string(AdvertisingInterval::Ms100));

    EXPECT_EQ(builder.get(), "AT+ADVI0");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildUartBaudRateCommand) {
    CommandBuilder builder;
    builder.append("AT+BAUD").append(enum_to_string(UartBaudRate::Br115200));

    EXPECT_EQ(builder.get(), "AT+BAUD4");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildBondModeCommand) {
    CommandBuilder builder;
    builder.append("AT+TYPE").append(enum_to_string(BondMode::AuthWithPin));

    EXPECT_EQ(builder.get(), "AT+TYPE2");
    EXPECT_FALSE(builder.overflow());
}

// =============================================================================
// Command Builder + Type Aliases Integration Tests
// =============================================================================

TEST(IntegrationTest, BuildPINCommand) {
    PinType pin("123456");
    CommandBuilder builder;
    builder.append("AT+PASS").append(pin.view());

    EXPECT_EQ(builder.get(), "AT+PASS123456");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildNameCommand) {
    CommandBuilder builder;
    builder.append("AT+NAME").append("MyDevice");

    EXPECT_EQ(builder.get(), "AT+NAMEMyDevice");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildMACAddressCommand) {
    MacAddress mac("AABBCCDDEEFF");
    CommandBuilder builder;
    builder.append("AT+CON").append(mac.view());

    EXPECT_EQ(builder.get(), "AT+CONAABBCCDDEEFF");
    EXPECT_FALSE(builder.overflow());
}

TEST(IntegrationTest, BuildiBeaconUUIDCommand) {
    IBeaconUuid uuid("FDA50693A4E24FB1AFCFC6EB07647825");
    CommandBuilder builder;
    builder.append("AT+IBE0").append(uuid.view());

    EXPECT_EQ(builder.get(), "AT+IBE0FDA50693A4E24FB1AFCFC6EB07647825");
    EXPECT_FALSE(builder.overflow());
}

// =============================================================================
// Response Parsing Integration Tests
// =============================================================================

TEST(IntegrationTest, ParseOKResponse) {
    std::string_view response = "OK+Set:0";

    EXPECT_TRUE(starts_with(response, "OK+"));

    auto remaining = response.substr(3);
    EXPECT_EQ(remaining, "Set:0");
}

TEST(IntegrationTest, ParseRoleResponse) {
    std::string_view response = "OK+Get:0";

    EXPECT_TRUE(starts_with(response, "OK+Get:"));

    auto value_str = response.substr(7); // Skip "OK+Get:"
    EXPECT_EQ(value_str, "0");
}

TEST(IntegrationTest, ParseNameResponse) {
    std::string_view response = "OK+NAME:MyDevice";

    EXPECT_TRUE(starts_with(response, "OK+NAME:"));

    auto name = response.substr(8); // Skip "OK+NAME:"
    EXPECT_EQ(name, "MyDevice");
}

TEST(IntegrationTest, ParseMACAddressResponse) {
    std::string_view response = "OK+RADD:AABBCCDDEEFF";

    EXPECT_TRUE(starts_with(response, "OK+RADD:"));

    auto mac_str = response.substr(8); // Skip "OK+RADD:"
    EXPECT_EQ(mac_str, "AABBCCDDEEFF");
    EXPECT_EQ(mac_str.size(), 12);

    // Validate MAC address format (all hex)
    EXPECT_TRUE(is_all_hex(mac_str));
}

TEST(IntegrationTest, ParseVersionResponse) {
    std::string_view response = "OK+Get:HMSoft V547";

    EXPECT_TRUE(starts_with(response, "OK+Get:"));

    auto version = response.substr(7); // Skip "OK+Get:"
    EXPECT_EQ(version, "HMSoft V547");
}

TEST(IntegrationTest, ParseDiscoveryResponse) {
    std::string_view response = "OK+DISC:4C000C0E0001:HM-10-0001:-045";

    EXPECT_TRUE(starts_with(response, "OK+DISC:"));

    auto data = response.substr(8); // Skip "OK+DISC:"

    // Parse MAC address (first 12 hex chars)
    auto mac = data.substr(0, 12);
    EXPECT_EQ(mac, "4C000C0E0001");
    EXPECT_TRUE(is_all_hex(mac));

    data = data.substr(12);
    EXPECT_TRUE(starts_with(data, ":"));
    data = data.substr(1);

    // Parse device name (until next ':')
    auto colon_pos = data.find(':');
    EXPECT_NE(colon_pos, std::string_view::npos);

    auto name = data.substr(0, colon_pos);
    EXPECT_EQ(name, "HM-10-0001");

    data = data.substr(colon_pos + 1);

    // Parse RSSI (remaining data)
    EXPECT_EQ(data, "-045");
}

TEST(IntegrationTest, ParseTemperatureResponse) {
    std::string_view response = "OK+Get:025C";

    EXPECT_TRUE(starts_with(response, "OK+Get:"));

    auto temp_str = response.substr(7); // Skip "OK+Get:"
    EXPECT_EQ(temp_str, "025C");

    // Parse temperature (first 2 hex digits) and humidity (last 2 hex digits)
    EXPECT_EQ(temp_str.size(), 4);

    auto temp_hex = temp_str.substr(0, 2);
    auto humidity_hex = temp_str.substr(2, 2);

    EXPECT_EQ(temp_hex, "02");
    EXPECT_EQ(humidity_hex, "5C");

    // Verify hex digits are valid
    EXPECT_TRUE(is_all_hex(temp_hex));
    EXPECT_TRUE(is_all_hex(humidity_hex));
}

// =============================================================================
// End-to-End Command-Response Integration Tests
// =============================================================================

TEST(IntegrationTest, QueryRoleEndToEnd) {
    MockUARTForIntegration uart;

    // 1. Build command
    CommandBuilder builder;
    builder.append("AT+ROLE?");

    // 2. Transmit command
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    UartStatus tx_status = UartStatus::ErrError;
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);
    EXPECT_EQ(uart.get_last_transmitted(), "AT+ROLE?");

    // 3. Simulate response
    uart.set_response("OK+Get:0");

    // 4. Receive response
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status = UartStatus::ErrError;
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::Ok);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Byte buffer to char conversion for string parsing
    std::string_view response(reinterpret_cast<const char*>(rx_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    // Find actual response length (until first null)
    const std::size_t RESPONSE_LEN = response.find('\0');
    if (RESPONSE_LEN != std::string_view::npos) {
        response = response.substr(0, RESPONSE_LEN);
    }

    EXPECT_TRUE(starts_with(response, "OK+Get:"));
    auto value_str = response.substr(7);
    EXPECT_EQ(value_str, "0"); // Peripheral
}

TEST(IntegrationTest, SetRoleEndToEnd) {
    MockUARTForIntegration uart;

    // 1. Build command to set Central role
    CommandBuilder builder;
    builder.append("AT+ROLE").append(enum_to_string(Role::Central));

    // 2. Transmit command
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    UartStatus tx_status = UartStatus::ErrError;
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);
    EXPECT_EQ(uart.get_last_transmitted(), "AT+ROLE1");

    // 3. Simulate response
    uart.set_response("OK+Set:1");

    // 4. Receive response
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status = UartStatus::ErrError;
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::Ok);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Byte buffer to char conversion for string parsing
    std::string_view response(reinterpret_cast<const char*>(rx_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::size_t RESPONSE_LEN = response.find('\0');
    if (RESPONSE_LEN != std::string_view::npos) {
        response = response.substr(0, RESPONSE_LEN);
    }

    EXPECT_TRUE(starts_with(response, "OK+Set:"));
    auto value_str = response.substr(7);
    EXPECT_EQ(value_str, "1"); // Central
}

TEST(IntegrationTest, SetNameEndToEnd) {
    MockUARTForIntegration uart;

    // 1. Build command
    CommandBuilder builder;
    builder.append("AT+NAME").append("TestDevice");

    // 2. Transmit
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    UartStatus tx_status = UartStatus::ErrError;
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);
    EXPECT_EQ(uart.get_last_transmitted(), "AT+NAMETestDevice");

    // 3. Simulate response
    uart.set_response("OK+Set:TestDevice");

    // 4. Receive
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status = UartStatus::ErrError;
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::Ok);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Byte buffer to char conversion for string parsing
    std::string_view response(reinterpret_cast<const char*>(rx_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::size_t RESPONSE_LEN = response.find('\0');
    if (RESPONSE_LEN != std::string_view::npos) {
        response = response.substr(0, RESPONSE_LEN);
    }

    EXPECT_TRUE(starts_with(response, "OK+Set:"));
    auto name = response.substr(7);
    EXPECT_EQ(name, "TestDevice");
}

TEST(IntegrationTest, ConnectWithMACAddressEndToEnd) {
    MockUARTForIntegration uart;

    // 1. Build command with MAC address
    MacAddress target_mac("AABBCCDDEEFF");
    CommandBuilder builder;
    builder.append("AT+CON").append(target_mac.view());

    // 2. Transmit
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    UartStatus tx_status = UartStatus::ErrError;
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);
    EXPECT_EQ(uart.get_last_transmitted(), "AT+CONAABBCCDDEEFF");

    // 3. Simulate response
    uart.set_response("OK+CONNA");

    // 4. Receive
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status = UartStatus::ErrError;
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::Ok);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Byte buffer to char conversion for string parsing
    std::string_view response(reinterpret_cast<const char*>(rx_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::size_t RESPONSE_LEN = response.find('\0');
    if (RESPONSE_LEN != std::string_view::npos) {
        response = response.substr(0, RESPONSE_LEN);
    }

    EXPECT_EQ(response, "OK+CONNA");
}

TEST(IntegrationTest, ErrorResponseHandling) {
    MockUARTForIntegration uart;

    // 1. Build invalid command
    CommandBuilder builder;
    builder.append("AT+INVALID");

    // 2. Transmit
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    UartStatus tx_status = UartStatus::ErrError;
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);

    // 3. Simulate error response
    uart.set_response("OK+ERR");

    // 4. Receive
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status = UartStatus::ErrError;
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::Ok);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Byte buffer to char conversion for string parsing
    std::string_view response(reinterpret_cast<const char*>(rx_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::size_t RESPONSE_LEN = response.find('\0');
    if (RESPONSE_LEN != std::string_view::npos) {
        response = response.substr(0, RESPONSE_LEN);
    }

    EXPECT_EQ(response, "OK+ERR");
    // Error responses typically start with "OK+ERR" or similar
    EXPECT_TRUE(starts_with(response, "OK+ERR"));
}

TEST(IntegrationTest, TimeoutHandling) {
    MockUARTForIntegration uart;

    // 1. Build command
    CommandBuilder builder;
    builder.append("AT+ROLE?");

    // 2. Transmit
    std::string cmd = std::string(builder.get());
    std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
    // NOLINTBEGIN(cppcoreguidelines-init-variables)
    // Initialized for testing error path
    UartStatus tx_status{UartStatus::ErrError};
    // NOLINTEND(cppcoreguidelines-init-variables)
    uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), tx_status);

    EXPECT_EQ(tx_status, UartStatus::Ok);

    // 3. Don't set response (simulating timeout)

    // 4. Receive (should timeout)
    std::array<std::uint8_t, 64> rx_buffer{};
    UartStatus rx_status{UartStatus::Ok};
    uart.receive(UartData8b(rx_buffer.data(), rx_buffer.size()), rx_status);

    EXPECT_EQ(rx_status, UartStatus::ErrTimeout);
}

// =============================================================================
// Complex Multi-Command Integration Tests
// =============================================================================

TEST(IntegrationTest, ConfigurationSequence) {
    MockUARTForIntegration uart;

    // Simulate a typical configuration sequence: Set role, baud rate, and name

    // Step 1: Set role to Central
    {
        CommandBuilder builder;
        builder.append("AT+ROLE").append(enum_to_string(Role::Central));

        std::string cmd = std::string(builder.get());
        std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
        // NOLINTBEGIN(cppcoreguidelines-init-variables)
        // Initialized for testing error path
        UartStatus status{UartStatus::ErrError};
        // NOLINTEND(cppcoreguidelines-init-variables)
        uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), status);

        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(uart.get_last_transmitted(), "AT+ROLE1");

        uart.clear();
    }

    // Step 2: Set baud rate to 115200
    {
        CommandBuilder builder;
        builder.append("AT+BAUD").append(enum_to_string(UartBaudRate::Br115200));

        std::string cmd = std::string(builder.get());
        std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
        // NOLINTBEGIN(cppcoreguidelines-init-variables)
        // Initialized for testing error path
        UartStatus status{UartStatus::ErrError};
        // NOLINTEND(cppcoreguidelines-init-variables)
        uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), status);

        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(uart.get_last_transmitted(), "AT+BAUD4");

        uart.clear();
    }

    // Step 3: Set device name
    {
        CommandBuilder builder;
        builder.append("AT+NAME").append("CentralDevice");

        std::string cmd = std::string(builder.get());
        std::vector<std::uint8_t> cmd_bytes(cmd.begin(), cmd.end());
        // NOLINTBEGIN(cppcoreguidelines-init-variables)
        // Initialized for testing error path
        UartStatus status{UartStatus::ErrError};
        // NOLINTEND(cppcoreguidelines-init-variables)
        uart.transmit(UartData8bConst(cmd_bytes.data(), cmd_bytes.size()), status);

        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(uart.get_last_transmitted(), "AT+NAMECentralDevice");
    }
}

// =============================================================================
// Constexpr Integration Tests
// =============================================================================

TEST(IntegrationTest, ConstexprCommandBuilding) {
    // Verify that command building can be done at compile time where possible
    constexpr auto ROLE_STR = enum_to_string(Role::Peripheral);
    static_assert(ROLE_STR == "0", "Role::Peripheral should map to '0'");

    constexpr auto INTERVAL_STR = enum_to_string(AdvertisingInterval::Ms100);
    static_assert(INTERVAL_STR == "0", "AdvertisingInterval::Ms100 should map to '0'");

    constexpr auto BAUD_STR = enum_to_string(UartBaudRate::Br115200);
    static_assert(BAUD_STR == "4", "UartBaudRate::Br115200 should map to '4'");

    // These compile-time guarantees ensure zero runtime overhead for enum-to-string mappings
    SUCCEED();
}

TEST(IntegrationTest, ConstexprParsingUtilities) {
    // Verify parsing utilities are constexpr-friendly
    constexpr std::string_view TEST_STRING = "OK+Get:123";
    constexpr bool HAS_OK = starts_with(TEST_STRING, "OK+");
    static_assert(HAS_OK, "Should detect OK+ prefix");

    // Verify compile-time substring operations
    constexpr auto VALUE_PART = TEST_STRING.substr(7);
    static_assert(VALUE_PART == "123", "Should extract value");

    SUCCEED();
}
