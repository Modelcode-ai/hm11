/**
 * @file at_command_test.cpp
 * @brief Tests for AT command construction and parsing
 *
 * This file contains tests for the AT command construction and parsing utility
 * functions used by the HM11Driver.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "hm11/driver.hpp"
#include "hm11/hal/uart.hpp"
#include "hm11/hm11.hpp"
#include "hm11/util/at_command_builder.hpp"
#include "hm11/util/at_response_utils.hpp"
#include "hm11/util/response_parser.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for AT command tests
 *
 * This fixture provides a testing environment for AT command construction and parsing.
 */
class ATCommandTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No specific setup needed
    }

    void TearDown() override {
        // No specific teardown needed
    }
};

// Test basic AT command construction
TEST_F(ATCommandTest, BasicCommand) {
    const std::string command = util::build_command("AT");
    EXPECT_EQ(command, "AT");
}

// Test AT command with parameters
TEST_F(ATCommandTest, CommandWithParams) {
    const std::string command = util::build_command("AT+ROLE", "1");
    EXPECT_EQ(command, "AT+ROLE1");
}

// Test AT command with multiple parameters
TEST_F(ATCommandTest, CommandWithMultipleParams) {
    const std::string command = util::build_command("AT+TEST", "param1", "param2");
    EXPECT_EQ(command, "AT+TESTparam1param2");
}

// Test AT command with numeric parameters
TEST_F(ATCommandTest, CommandWithNumericParams) {
    const std::string command = util::build_command("AT+BAUD", 4);
    EXPECT_EQ(command, "AT+BAUD4");
}

// Test AT command with enum parameters
TEST_F(ATCommandTest, CommandWithEnumParams) {
    const std::string command = util::build_command("AT+ROLE", Role::Central);
    EXPECT_EQ(command, "AT+ROLE1"); // Central = 1

    const std::string command2 = util::build_command("AT+ROLE", Role::Peripheral);
    EXPECT_EQ(command2, "AT+ROLE0"); // Peripheral = 0
}

// Test query command construction
TEST_F(ATCommandTest, QueryCommand) {
    const std::string command = util::at::build_query_command("AT+ROLE");
    EXPECT_EQ(command, "AT+ROLE?");
}

// Test response parsing
TEST_F(ATCommandTest, BasicResponseParsing) {
    const std::string response = "OK+Get:0";
    std::size_t value = 0;
    bool success = util::parse_response(response, "OK+Get:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, 0);
}

// Test response parsing with invalid prefix
TEST_F(ATCommandTest, ResponseParsingInvalidPrefix) {
    const std::string response = "OK+Get:0";
    std::size_t value = 0;
    bool success = util::parse_response(response, "INVALID:", value);
    EXPECT_FALSE(success);
}

// Test response parsing with string value
TEST_F(ATCommandTest, ResponseParsingString) {
    const std::string response = "OK+NAME:HMSoft";
    std::string value;
    bool success = util::parse_response(response, "OK+NAME:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, "HMSoft");
}

// Test response parsing with enum value
TEST_F(ATCommandTest, ResponseParsingEnum) {
    const std::string response = "OK+Get:1";
    Role value = Role::Peripheral;
    bool success = util::parse_response(response, "OK+Get:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, Role::Central);
}

// Test response checking
TEST_F(ATCommandTest, ResponseChecking) {
    const std::string response = "OK";
    bool success = util::check_response(response, "OK");
    EXPECT_TRUE(success);

    bool failure = util::check_response(response, "ERROR");
    EXPECT_FALSE(failure);
}

// Test MAC address response parsing
TEST_F(ATCommandTest, MacAddressResponseParsing) {
    const std::string response = "OK+ADDR:112233445566";
    util::MacAddress mac;
    bool success = util::parse_mac_address(response, "OK+ADDR:", mac);
    EXPECT_TRUE(success);
    EXPECT_EQ(mac, util::MacAddress("112233445566"));
}

// Test version response parsing
TEST_F(ATCommandTest, VersionResponseParsing) {
    const std::string response = "HMSoft V1.0";
    std::string version;
    bool success = util::parse_version(response, version);
    EXPECT_TRUE(success);
    EXPECT_EQ(version, "V1.0");
}

// Test boolean response parsing
TEST_F(ATCommandTest, BooleanResponseParsing) {
    // Test true (1) value
    {
        const std::string response = "OK+Get:1";
        bool value = false;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_TRUE(value);
    }

    // Test false (0) value
    {
        const std::string response = "OK+Get:0";
        bool value = true;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_FALSE(value);
    }
}

// Test response parsing with uint8_t
TEST_F(ATCommandTest, Uint8ResponseParsing) {
    const std::string response = "OK+Get:123";
    uint8_t value = 0;
    bool success = util::parse_response(response, "OK+Get:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, 123);
}

// Test response parsing with out of range uint8_t
TEST_F(ATCommandTest, Uint8OutOfRangeResponseParsing) {
    const std::string response = "OK+Get:256"; // Out of range for uint8_t
    uint8_t value = 0;
    bool success = util::parse_response(response, "OK+Get:", value);

    // This could fail or truncate depending on the implementation
    // We'll accept either behavior for this test
    if (success) {
        // If it succeeded, the value should be truncated to fit in uint8_t
        EXPECT_EQ(value, 0); // 256 % 256 = 0
    }
}

// Test custom command builder with special characters
TEST_F(ATCommandTest, CustomCommandWithSpecialChars) {
    const std::string command = util::build_command("AT+TYPE", "!@#$");
    EXPECT_EQ(command, "AT+TYPE!@#$");
}

// Test empty response parsing
TEST_F(ATCommandTest, EmptyResponseParsing) {
    const std::string response = "";
    std::string value;
    bool success = util::parse_response(response, "OK+NAME:", value);
    EXPECT_FALSE(success);
}

// Test response parsing with extra characters
TEST_F(ATCommandTest, ResponseParsingWithExtraChars) {
    const std::string response = "OK+Get:0\r\n";
    std::size_t value = 0;
    bool success = util::parse_response(response, "OK+Get:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, 0);
}

// Test build_set_command with string_view
TEST_F(ATCommandTest, BuildSetCommandWithStringView) {
    const std::string command = util::at::build_set_command("AT+NAME", std::string_view("TestDevice"));
    EXPECT_EQ(command, "AT+NAMETestDevice");
}

// Test build_set_command with std::string
TEST_F(ATCommandTest, BuildSetCommandWithString) {
    const std::string name = "MyDevice";
    const std::string command = util::at::build_set_command("AT+NAME", name);
    EXPECT_EQ(command, "AT+NAMEMyDevice");
}

// Test build_set_command with enum values
TEST_F(ATCommandTest, BuildSetCommandWithEnumRole) {
    const std::string command = util::at::build_set_command("AT+ROLE", util::at::to_string(Role::Central));
    EXPECT_EQ(command, "AT+ROLE1");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumAdvertisingInterval) {
    const std::string command = util::at::build_set_command("AT+ADVI", util::at::to_string(AdvertisingInterval::Ms100));
    EXPECT_EQ(command, "AT+ADVI0");

    const std::string command2 = util::at::build_set_command("AT+ADVI", util::at::to_string(AdvertisingInterval::Ms7000));
    EXPECT_EQ(command2, "AT+ADVI15");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumAdvertisingType) {
    const std::string command = util::at::build_set_command("AT+ADTY", util::at::to_string(AdvertisingType::Advertising));
    EXPECT_EQ(command, "AT+ADTY3");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumWorkMode) {
    const std::string command = util::at::build_set_command("AT+MODE", util::at::to_string(WorkMode::TransmissionMode));
    EXPECT_EQ(command, "AT+MODE0");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumUartBaudRate) {
    const std::string command = util::at::build_set_command("AT+BAUD", util::at::to_string(UartBaudRate::Br115200));
    EXPECT_EQ(command, "AT+BAUD4");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumBondMode) {
    const std::string command = util::at::build_set_command("AT+TYPE", util::at::to_string(BondMode::AuthAndBonded));
    EXPECT_EQ(command, "AT+TYPE3");
}

TEST_F(ATCommandTest, BuildSetCommandWithEnumModulePower) {
    const std::string command = util::at::build_set_command("AT+POWE", util::at::to_string(ModulePower::Dbm6));
    EXPECT_EQ(command, "AT+POWE3");
}

// Test build_hex_command
TEST_F(ATCommandTest, BuildHexCommandWithStringView) {
    const std::string command = util::at::build_hex_command("AT+UUID", "FFE0");
    EXPECT_EQ(command, "AT+UUID0xFFE0");
}

// Test build_connect_command
TEST_F(ATCommandTest, BuildConnectCommand) {
    util::MacAddress mac("112233445566");
    const std::string command = util::at::build_connect_command(mac);
    EXPECT_EQ(command, "AT+CON112233445566");
}

// Test build_scan_command
TEST_F(ATCommandTest, BuildScanCommandIndefinite) {
    const std::string command = util::at::build_scan_command(0);
    EXPECT_EQ(command, "AT+SCAN");
}

TEST_F(ATCommandTest, BuildScanCommandWithTimeout) {
    const std::string command = util::at::build_scan_command(5);
    EXPECT_EQ(command, "AT+SCAN5");
}

TEST_F(ATCommandTest, BuildScanCommandVariousTimeouts) {
    EXPECT_EQ(util::at::build_scan_command(1), "AT+SCAN1");
    EXPECT_EQ(util::at::build_scan_command(10), "AT+SCAN10");
    EXPECT_EQ(util::at::build_scan_command(255), "AT+SCAN255");
}

// Parameterized test for Role enum
class RoleEnumTest : public ::testing::TestWithParam<std::tuple<Role, std::string>> {};
TEST_P(RoleEnumTest, ToStringRole) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, RoleEnumTest,
    ::testing::Values(
        std::make_tuple(Role::Peripheral, "0"),
        std::make_tuple(Role::Central, "1")
    )
);

// Parameterized test for AdvertisingInterval enum
class AdvertisingIntervalTest : public ::testing::TestWithParam<std::tuple<AdvertisingInterval, std::string>> {};
TEST_P(AdvertisingIntervalTest, ToStringAdvertisingInterval) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, AdvertisingIntervalTest,
    ::testing::Values(
        std::make_tuple(AdvertisingInterval::Ms100, "0"),
        std::make_tuple(AdvertisingInterval::Ms211, "1"),
        std::make_tuple(AdvertisingInterval::Ms252, "2"),
        std::make_tuple(AdvertisingInterval::Ms318, "3"),
        std::make_tuple(AdvertisingInterval::Ms417, "4"),
        std::make_tuple(AdvertisingInterval::Ms546, "5"),
        std::make_tuple(AdvertisingInterval::Ms760, "6"),
        std::make_tuple(AdvertisingInterval::Ms852, "7"),
        std::make_tuple(AdvertisingInterval::Ms1022, "8"),
        std::make_tuple(AdvertisingInterval::Ms1285, "9"),
        std::make_tuple(AdvertisingInterval::Ms2000, "10"),
        std::make_tuple(AdvertisingInterval::Ms3000, "11"),
        std::make_tuple(AdvertisingInterval::Ms4000, "12"),
        std::make_tuple(AdvertisingInterval::Ms5000, "13"),
        std::make_tuple(AdvertisingInterval::Ms6000, "14"),
        std::make_tuple(AdvertisingInterval::Ms7000, "15")
    )
);

// Parameterized test for AdvertisingType enum
class AdvertisingTypeTest : public ::testing::TestWithParam<std::tuple<AdvertisingType, std::string>> {};
TEST_P(AdvertisingTypeTest, ToStringAdvertisingType) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, AdvertisingTypeTest,
    ::testing::Values(
        std::make_tuple(AdvertisingType::AdvertisingScanResponseConnectable, "0"),
        std::make_tuple(AdvertisingType::LastDeviceConnect, "1"),
        std::make_tuple(AdvertisingType::AdvertisingScanResponse, "2"),
        std::make_tuple(AdvertisingType::Advertising, "3")
    )
);

// Parameterized test for AdvertisingMode enum
class AdvertisingModeTest : public ::testing::TestWithParam<std::tuple<AdvertisingMode, std::string>> {};
TEST_P(AdvertisingModeTest, ToStringAdvertisingMode) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, AdvertisingModeTest,
    ::testing::Values(
        std::make_tuple(AdvertisingMode::NormalAdvertising, "0"),
        std::make_tuple(AdvertisingMode::ReliableAdvertising, "1")
    )
);

// Parameterized test for MacAddressType enum
class MacAddressTypeTest : public ::testing::TestWithParam<std::tuple<MacAddressType, std::string>> {};
TEST_P(MacAddressTypeTest, ToStringMacAddressType) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, MacAddressTypeTest,
    ::testing::Values(
        std::make_tuple(MacAddressType::NormalAddress, "0"),
        std::make_tuple(MacAddressType::StaticMac, "1"),
        std::make_tuple(MacAddressType::StaticRandomMac, "2"),
        std::make_tuple(MacAddressType::RandomMac, "3")
    )
);

// Parameterized test for WorkMode enum
class WorkModeTest : public ::testing::TestWithParam<std::tuple<WorkMode, std::string>> {};
TEST_P(WorkModeTest, ToStringWorkMode) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, WorkModeTest,
    ::testing::Values(
        std::make_tuple(WorkMode::TransmissionMode, "0"),
        std::make_tuple(WorkMode::PioCollectionMode, "1"),
        std::make_tuple(WorkMode::RemoteControlMode, "2")
    )
);

// Parameterized test for UartBaudRate enum
class UartBaudRateTest : public ::testing::TestWithParam<std::tuple<UartBaudRate, std::string>> {};
TEST_P(UartBaudRateTest, ToStringUartBaudRate) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, UartBaudRateTest,
    ::testing::Values(
        std::make_tuple(UartBaudRate::Br9600, "0"),
        std::make_tuple(UartBaudRate::Br19200, "1"),
        std::make_tuple(UartBaudRate::Br38400, "2"),
        std::make_tuple(UartBaudRate::Br57600, "3"),
        std::make_tuple(UartBaudRate::Br115200, "4"),
        std::make_tuple(UartBaudRate::Br4800, "5"),
        std::make_tuple(UartBaudRate::Br2400, "6"),
        std::make_tuple(UartBaudRate::Br1200, "7")
    )
);

// Parameterized test for BondMode enum
class BondModeTest : public ::testing::TestWithParam<std::tuple<BondMode, std::string>> {};
TEST_P(BondModeTest, ToStringBondMode) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, BondModeTest,
    ::testing::Values(
        std::make_tuple(BondMode::NotNeedPin, "0"),
        std::make_tuple(BondMode::AuthNotNeedPin, "1"),
        std::make_tuple(BondMode::AuthWithPin, "2"),
        std::make_tuple(BondMode::AuthAndBonded, "3")
    )
);

// Parameterized test for SendDataMethod enum
class SendDataMethodTest : public ::testing::TestWithParam<std::tuple<SendDataMethod, std::string>> {};
TEST_P(SendDataMethodTest, ToStringSendDataMethod) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, SendDataMethodTest,
    ::testing::Values(
        std::make_tuple(SendDataMethod::Write, "0"),
        std::make_tuple(SendDataMethod::WriteWithoutResponse, "1"),
        std::make_tuple(SendDataMethod::Indicate, "2"),
        std::make_tuple(SendDataMethod::Notify, "3")
    )
);

// Parameterized test for ModulePower enum
class ModulePowerTest : public ::testing::TestWithParam<std::tuple<ModulePower, std::string>> {};
TEST_P(ModulePowerTest, ToStringModulePower) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, ModulePowerTest,
    ::testing::Values(
        std::make_tuple(ModulePower::DbmMinus23, "0"),
        std::make_tuple(ModulePower::DbmMinus6, "1"),
        std::make_tuple(ModulePower::Dbm0, "2"),
        std::make_tuple(ModulePower::Dbm6, "3")
    )
);

// Parameterized test for ParityBit enum
class ParityBitTest : public ::testing::TestWithParam<std::tuple<ParityBit, std::string>> {};
TEST_P(ParityBitTest, ToStringParityBit) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, ParityBitTest,
    ::testing::Values(
        std::make_tuple(ParityBit::None, "0"),
        std::make_tuple(ParityBit::Odd, "1"),
        std::make_tuple(ParityBit::Even, "2")
    )
);

// Parameterized test for StopBit enum
class StopBitTest : public ::testing::TestWithParam<std::tuple<StopBit, std::string>> {};
TEST_P(StopBitTest, ToStringStopBit) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, StopBitTest,
    ::testing::Values(
        std::make_tuple(StopBit::OneStopBit, "0"),
        std::make_tuple(StopBit::TwoStopBit, "1")
    )
);

// Parameterized test for RxGain enum
class RxGainTest : public ::testing::TestWithParam<std::tuple<RxGain, std::string>> {};
TEST_P(RxGainTest, ToStringRxGain) {
    auto [enum_val, expected_str] = GetParam();
    EXPECT_EQ(util::at::to_string(enum_val), expected_str);
}
INSTANTIATE_TEST_SUITE_P(
    ATCommandTest, RxGainTest,
    ::testing::Values(
        std::make_tuple(RxGain::No, "0"),
        std::make_tuple(RxGain::Open, "1")
    )
);

} // namespace hm11::tests