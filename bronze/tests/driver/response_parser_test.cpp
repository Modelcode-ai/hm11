/**
 * @file response_parser_test.cpp
 * @brief Tests for HM11Driver response parser utility functions
 *
 * This file contains tests for the response parser utility functions used by
 * the HM11Driver class.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "hm11/driver.hpp"
#include "hm11/hm11.hpp"
#include "hm11/util/at_response_utils.hpp"
#include "hm11/util/fixed_string.hpp"
#include "hm11/util/response_parser.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for response parser tests
 *
 * This fixture provides a testing environment for the response parser
 * utility functions.
 */
class ResponseParserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No specific setup needed
    }

    void TearDown() override {
        // No specific teardown needed
    }
};

// Test parsing responses with integer values
TEST_F(ResponseParserTest, ParseInteger) {
    // Test parsing a valid integer
    {
        const std::string response = "OK+Get:42";
        int value = 0;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, 42);
    }

    // Test parsing a negative integer
    {
        const std::string response = "OK+Get:-42";
        int value = 0;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, -42);
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+Get:42";
        int value = 0;
        bool success = util::parse_response(response, "INVALID:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, 0); // Should not be modified
    }

    // Test parsing with invalid integer
    {
        const std::string response = "OK+Get:not_an_integer";
        int value = 0;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, 0); // Should not be modified
    }
}

// Test parsing responses with boolean values
TEST_F(ResponseParserTest, ParseBoolean) {
    // Test parsing a valid boolean (true)
    {
        const std::string response = "OK+Get:1";
        bool value = false;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_TRUE(value);
    }

    // Test parsing a valid boolean (false)
    {
        const std::string response = "OK+Get:0";
        bool value = true;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_FALSE(value);
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+Get:1";
        bool value = false;
        bool success = util::parse_response(response, "INVALID:", value);
        EXPECT_FALSE(success);
        EXPECT_FALSE(value); // Should not be modified
    }

    // Test parsing with invalid boolean
    {
        const std::string response = "OK+Get:not_a_boolean";
        bool value = false;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_FALSE(success);
        EXPECT_FALSE(value); // Should not be modified
    }
}

// Test parsing responses with enum values
TEST_F(ResponseParserTest, ParseEnum) {
    // Test parsing a valid enum (Central)
    {
        const std::string response = "OK+Get:1";
        Role value = Role::Peripheral;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, Role::Central);
    }

    // Test parsing a valid enum (Peripheral)
    {
        const std::string response = "OK+Get:0";
        Role value = Role::Central;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, Role::Peripheral);
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+Get:1";
        Role value = Role::Peripheral;
        bool success = util::parse_response(response, "INVALID:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, Role::Peripheral); // Should not be modified
    }

    // Test parsing with invalid enum value
    {
        const std::string response = "OK+Get:999"; // Out of range for Role
        Role value = Role::Peripheral;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, Role::Peripheral); // Should not be modified
    }
}

// Test parsing responses with string values
TEST_F(ResponseParserTest, ParseString) {
    // Test parsing a valid string
    {
        const std::string response = "OK+NAME:HMSoft";
        std::string value;
        bool success = util::parse_response(response, "OK+NAME:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, "HMSoft");
    }

    // Test parsing a empty string
    {
        const std::string response = "OK+NAME:";
        std::string value;
        bool success = util::parse_response(response, "OK+NAME:", value);
        EXPECT_TRUE(success);
        EXPECT_TRUE(value.empty());
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+NAME:HMSoft";
        std::string value;
        bool success = util::parse_response(response, "INVALID:", value);
        EXPECT_FALSE(success);
        EXPECT_TRUE(value.empty()); // Should not be modified
    }
}

// Test parsing responses with fixed-length string values (MacAddress, UUID, etc.)
TEST_F(ResponseParserTest, ParseFixedString) {
    // Test parsing a valid MAC address
    {
        const std::string response = "OK+ADDR:112233445566";
        util::MacAddress value;
        bool success = util::parse_response(response, "OK+ADDR:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, util::MacAddress("112233445566"));
    }

    // Test parsing a valid UUID
    {
        const std::string response = "OK+Get:AABB";
        util::Uuid value;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, util::Uuid("AABB"));
    }

    // Test parsing a valid iBeacon UUID
    {
        const std::string response = "OK+Get:AABBCCDDEEFF00112233445566778899";
        util::IBeaconUuid value;
        bool success = util::parse_response(response, "OK+Get:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, util::IBeaconUuid("AABBCCDDEEFF00112233445566778899"));
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+ADDR:112233445566";
        util::MacAddress value;
        bool success = util::parse_response(response, "INVALID:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, util::MacAddress("000000000000")); // Should not be modified from default
    }

    // Test parsing with invalid MAC address (non-hex characters)
    {
        const std::string response = "OK+ADDR:GGHHIIJJKKLL"; // 'G' is not a hex character
        util::MacAddress value;
        bool success = util::parse_response(response, "OK+ADDR:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, util::MacAddress("000000000000")); // Should not be modified from default
    }

    // Test parsing with too short MAC address
    {
        const std::string response = "OK+ADDR:1122334455"; // Only 10 characters
        util::MacAddress value;
        bool success = util::parse_response(response, "OK+ADDR:", value);
        EXPECT_FALSE(success);
        EXPECT_EQ(value, util::MacAddress("000000000000")); // Should not be modified from default
    }

    // Test parsing with too long MAC address (should be truncated)
    {
        const std::string response = "OK+ADDR:112233445566778899"; // 18 characters
        util::MacAddress value;
        bool success = util::parse_response(response, "OK+ADDR:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, util::MacAddress("112233445566")); // Should be truncated
    }
}

// Test checking responses
TEST_F(ResponseParserTest, CheckResponse) {
    // Test checking a valid response
    {
        const std::string response = "OK";
        bool success = util::check_response(response, "OK");
        EXPECT_TRUE(success);
    }

    // Test checking a response with trailing characters
    {
        const std::string response = "OK+EXTRA";
        bool success = util::check_response(response, "OK");
        EXPECT_TRUE(success);
    }

    // Test checking a response with case difference
    {
        const std::string response = "ok";
        bool success = util::check_response(response, "OK");
        EXPECT_FALSE(success); // Should be case sensitive
    }

    // Test checking an invalid response
    {
        const std::string response = "ERROR";
        bool success = util::check_response(response, "OK");
        EXPECT_FALSE(success);
    }

    // Test checking an empty response
    {
        const std::string response = "";
        bool success = util::check_response(response, "OK");
        EXPECT_FALSE(success);
    }
}

// Test parsing MAC addresses
TEST_F(ResponseParserTest, ParseMacAddress) {
    // Test parsing a valid MAC address
    {
        const std::string response = "OK+ADDR:112233445566";
        util::MacAddress mac;
        bool success = util::parse_mac_address(response, "OK+ADDR:", mac);
        EXPECT_TRUE(success);
        EXPECT_EQ(mac, util::MacAddress("112233445566"));
    }

    // Test parsing with invalid prefix
    {
        const std::string response = "OK+ADDR:112233445566";
        util::MacAddress mac;
        bool success = util::parse_mac_address(response, "INVALID:", mac);
        EXPECT_FALSE(success);
        EXPECT_EQ(mac, util::MacAddress("000000000000")); // Should not be modified from default
    }

    // Test parsing with invalid MAC address (non-hex characters)
    {
        const std::string response = "OK+ADDR:GGHHIIJJKKLL"; // 'G' is not a hex character
        util::MacAddress mac;
        bool success = util::parse_mac_address(response, "OK+ADDR:", mac);
        EXPECT_FALSE(success);
        EXPECT_EQ(mac, util::MacAddress("000000000000")); // Should not be modified from default
    }

    // Test parsing with too short MAC address
    {
        const std::string response = "OK+ADDR:1122334455"; // Only 10 characters
        util::MacAddress mac;
        bool success = util::parse_mac_address(response, "OK+ADDR:", mac);
        EXPECT_FALSE(success);
        EXPECT_EQ(mac, util::MacAddress("000000000000")); // Should not be modified from default
    }
}

// Test parsing version strings
TEST_F(ResponseParserTest, ParseVersion) {
    // Test parsing a valid version string
    {
        const std::string response = "HMSoft V1.0";
        std::string version;
        bool success = util::parse_version(response, version);
        EXPECT_TRUE(success);
        EXPECT_EQ(version, "V1.0");
    }

    // Test parsing a different format version string
    {
        const std::string response = "HMSoft Version 2.1";
        std::string version;
        bool success = util::parse_version(response, version);
        EXPECT_TRUE(success);
        EXPECT_EQ(version, "Version 2.1");
    }

    // Test parsing an invalid version string
    {
        const std::string response = "InvalidResponse";
        std::string version;
        bool success = util::parse_version(response, version);
        EXPECT_FALSE(success);
        EXPECT_TRUE(version.empty()); // Should not be modified
    }

    // Test parsing an empty response
    {
        const std::string response = "";
        std::string version;
        bool success = util::parse_version(response, version);
        EXPECT_FALSE(success);
        EXPECT_TRUE(version.empty()); // Should not be modified
    }
}

// Test handling of responses with special characters or edge cases
TEST_F(ResponseParserTest, SpecialCases) {
    // Test parsing a response with special characters
    {
        const std::string response = "OK+NAME:Special!@#$%^&*()";
        std::string value;
        bool success = util::parse_response(response, "OK+NAME:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, "Special!@#$%^&*()");
    }

    // Test parsing a response with newline characters
    {
        const std::string response = "OK+NAME:Line1\r\nLine2";
        std::string value;
        bool success = util::parse_response(response, "OK+NAME:", value);
        EXPECT_TRUE(success);
        EXPECT_EQ(value, "Line1\r\nLine2");
    }

    // Test parsing a response with exactly the prefix (no value)
    {
        const std::string response = "OK+NAME:";
        std::string value;
        bool success = util::parse_response(response, "OK+NAME:", value);
        EXPECT_TRUE(success);
        EXPECT_TRUE(value.empty());
    }
}

// Test contains function
TEST_F(ResponseParserTest, Contains) {
    EXPECT_TRUE(util::parser::contains("OK+Get:1", "OK"));
    EXPECT_TRUE(util::parser::contains("OK+Get:1", "Get"));
    EXPECT_TRUE(util::parser::contains("ERROR", "ERROR"));
    EXPECT_FALSE(util::parser::contains("OK+Get:1", "NOTFOUND"));
    EXPECT_FALSE(util::parser::contains("", "OK"));
}

// Test starts_with function
TEST_F(ResponseParserTest, StartsWith) {
    EXPECT_TRUE(util::parser::starts_with("OK+Get:1", "OK"));
    EXPECT_TRUE(util::parser::starts_with("OK+Get:1", "OK+"));
    EXPECT_TRUE(util::parser::starts_with("ERROR", "ERROR"));
    EXPECT_FALSE(util::parser::starts_with("OK+Get:1", "Get"));
    EXPECT_FALSE(util::parser::starts_with("", "OK"));
    EXPECT_TRUE(util::parser::starts_with("Test", ""));
}

// Test find function
TEST_F(ResponseParserTest, Find) {
    auto pos1 = util::parser::find("OK+Get:1", "Get");
    EXPECT_TRUE(pos1.has_value());
    EXPECT_EQ(*pos1, 3);

    auto pos2 = util::parser::find("OK+Get:1", "OK");
    EXPECT_TRUE(pos2.has_value());
    EXPECT_EQ(*pos2, 0);

    auto pos3 = util::parser::find("OK+Get:1", "NOTFOUND");
    EXPECT_FALSE(pos3.has_value());

    auto pos4 = util::parser::find("", "OK");
    EXPECT_FALSE(pos4.has_value());
}

// Test extract function
TEST_F(ResponseParserTest, Extract) {
    auto result1 = util::parser::extract("OK+Get:1", 0, 2);
    EXPECT_EQ(result1, "OK");

    auto result2 = util::parser::extract("OK+Get:1", 3, 6);
    EXPECT_EQ(result2, "Get");

    auto result3 = util::parser::extract("OK+Get:1", 7, std::string_view::npos);
    EXPECT_EQ(result3, "1");

    auto result4 = util::parser::extract("OK+Get:1", 100, 200);
    EXPECT_TRUE(result4.empty());

    auto result5 = util::parser::extract("Test", 0, std::string_view::npos);
    EXPECT_EQ(result5, "Test");
}

// Test extract_after function
TEST_F(ResponseParserTest, ExtractAfter) {
    auto result1 = util::parser::extract_after("OK+Get:1", "OK+Get:");
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, "1");

    auto result2 = util::parser::extract_after("OK+NAME:TestDevice", "OK+NAME:");
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, "TestDevice");

    auto result3 = util::parser::extract_after("OK+Get:1", "NOTFOUND");
    EXPECT_FALSE(result3.has_value());

    auto result4 = util::parser::extract_after("OK+NAME:", "OK+NAME:");
    EXPECT_TRUE(result4.has_value());
    EXPECT_TRUE(result4->empty());

    // Test with delimiter
    auto result5 = util::parser::extract_after("OK+Get:1,2,3", "OK+Get:", ',');
    EXPECT_TRUE(result5.has_value());
    EXPECT_EQ(*result5, "1");
}

// Test extract_role function
TEST_F(ResponseParserTest, ExtractRole) {
    hal::UartStatus status;

    // Test extracting Peripheral role
    {
        status = hal::UartStatus::Ok;
        auto role = util::parser::extract_role("OK+Get:0", status);
        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(role, Role::Peripheral);
    }

    // Test extracting Central role
    {
        status = hal::UartStatus::Ok;
        auto role = util::parser::extract_role("OK+Get:1", status);
        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(role, Role::Central);
    }

    // Test with invalid prefix
    {
        status = hal::UartStatus::Ok;
        auto role = util::parser::extract_role("INVALID", status);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
        EXPECT_EQ(role, Role::Peripheral);
    }

    // Test with invalid value
    {
        status = hal::UartStatus::Ok;
        auto role = util::parser::extract_role("OK+Get:9", status);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
        EXPECT_EQ(role, Role::Peripheral);
    }

    // Test with empty value
    {
        status = hal::UartStatus::Ok;
        auto role = util::parser::extract_role("OK+Get:", status);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
        EXPECT_EQ(role, Role::Peripheral);
    }
}

// Test extract_mac_address function
TEST_F(ResponseParserTest, ExtractMacAddress) {
    hal::UartStatus status;
    util::MacAddress mac;

    // Test extracting a valid MAC address
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:112233445566", "OK+ADDR:", mac, status);
        EXPECT_TRUE(success);
        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(mac, util::MacAddress("112233445566"));
    }

    // Test with invalid prefix
    {
        mac = util::MacAddress("000000000000");
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:112233445566", "INVALID:", mac, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with invalid MAC address (non-hex)
    {
        mac = util::MacAddress("000000000000");
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:GGHHIIJJKKLL", "OK+ADDR:", mac, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with lowercase hex (should fail)
    {
        mac = util::MacAddress("000000000000");
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:aabbccddeeff", "OK+ADDR:", mac, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with too short MAC address
    {
        mac = util::MacAddress("000000000000");
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:112233", "OK+ADDR:", mac, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with empty value
    {
        mac = util::MacAddress("000000000000");
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_mac_address("OK+ADDR:", "OK+ADDR:", mac, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }
}

// Test extract_version function - note that VersionType uses HexCharValidator
// so version strings must be convertible to hex (or the function must handle conversion)
TEST_F(ResponseParserTest, ExtractVersion) {
    hal::UartStatus status;
    util::VersionType version;

    // Test with no 'V' character
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_version("NoVersion", version, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with empty response
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_version("", version, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with version too long (should fail)
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_version("V12345", version, status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }
}

// Test extract_name function
TEST_F(ResponseParserTest, ExtractName) {
    hal::UartStatus status;
    char name[50];

    // Test extracting a valid name
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_name("OK+Get:TestDevice", name, sizeof(name), status);
        EXPECT_TRUE(success);
        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_STREQ(name, "TestDevice");
    }

    // Test extracting with limited buffer
    {
        char small_buffer[6];
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_name("OK+Get:LongDeviceName", small_buffer, sizeof(small_buffer), status);
        EXPECT_TRUE(success);
        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_STREQ(small_buffer, "LongD");
    }

    // Test with invalid prefix
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_name("INVALID", name, sizeof(name), status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }

    // Test with empty value
    {
        status = hal::UartStatus::Ok;
        bool success = util::parser::extract_name("OK+Get:", name, sizeof(name), status);
        EXPECT_FALSE(success);
        EXPECT_EQ(status, hal::UartStatus::ErrError);
    }
}

// Test is_success function
TEST_F(ResponseParserTest, IsSuccess) {
    EXPECT_TRUE(util::parser::is_success("OK"));
    EXPECT_TRUE(util::parser::is_success("OK+Get:1"));
    EXPECT_TRUE(util::parser::is_success("Response OK"));
    EXPECT_FALSE(util::parser::is_success("ERROR"));
    EXPECT_FALSE(util::parser::is_success("Failed"));
    EXPECT_FALSE(util::parser::is_success(""));
}

// Test is_error function
TEST_F(ResponseParserTest, IsError) {
    EXPECT_TRUE(util::parser::is_error("ERROR"));
    EXPECT_TRUE(util::parser::is_error("Some ERROR message"));
    EXPECT_FALSE(util::parser::is_error("OK"));
    EXPECT_FALSE(util::parser::is_error("Success"));
    EXPECT_FALSE(util::parser::is_error(""));
}

// Test to_string_view function with UInt8 buffer
TEST_F(ResponseParserTest, ToStringViewUInt8) {
    // Test with null-terminated buffer
    {
        const hal::UInt8 buffer[] = {65, 66, 67, 0, 68, 69};
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_EQ(result, "ABC");
    }

    // Test with non-null-terminated buffer
    {
        const hal::UInt8 buffer[] = {65, 66, 67, 68, 69};
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_EQ(result, "ABCDE");
    }

    // Test with empty buffer (immediate null)
    {
        const hal::UInt8 buffer[] = {0, 65, 66};
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_TRUE(result.empty());
    }

    // Test with zero size
    {
        const hal::UInt8 buffer[] = {65, 66, 67};
        auto result = util::parser::to_string_view(buffer, 0);
        EXPECT_TRUE(result.empty());
    }
}

// Test to_string_view function with char buffer
TEST_F(ResponseParserTest, ToStringViewChar) {
    // Test with null-terminated buffer
    {
        const char buffer[] = "ABC\0DE";
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_EQ(result, "ABC");
    }

    // Test with non-null-terminated buffer
    {
        const char buffer[] = {'A', 'B', 'C', 'D', 'E'};
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_EQ(result, "ABCDE");
    }

    // Test with empty buffer (immediate null)
    {
        const char buffer[] = "\0AB";
        auto result = util::parser::to_string_view(buffer, sizeof(buffer));
        EXPECT_TRUE(result.empty());
    }

    // Test with zero size
    {
        const char buffer[] = "ABC";
        auto result = util::parser::to_string_view(buffer, 0);
        EXPECT_TRUE(result.empty());
    }
}

} // namespace hm11::tests