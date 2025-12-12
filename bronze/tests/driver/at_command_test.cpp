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

} // namespace hm11::tests