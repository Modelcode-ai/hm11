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

} // namespace hm11::tests