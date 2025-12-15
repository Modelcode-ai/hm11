// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

#include "hm11/util/numeric_formatter.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>

using hm11::util::NumericFormatter;

// Test unsigned integer formatting
TEST(NumericFormatterTest, FormatUnsignedZero) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_uint(0, buffer);
    EXPECT_EQ(result, "0");
}

TEST(NumericFormatterTest, FormatUnsignedSingleDigit) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_uint(5, buffer);
    EXPECT_EQ(result, "5");
}

TEST(NumericFormatterTest, FormatUnsignedMultipleDigits) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_uint(42, buffer);
    EXPECT_EQ(result, "42");
}

TEST(NumericFormatterTest, FormatUnsignedLarge) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_uint(999999, buffer);
    EXPECT_EQ(result, "999999");
}

TEST(NumericFormatterTest, FormatUnsignedMax) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_uint(UINT32_MAX, buffer);
    EXPECT_EQ(result, "4294967295");
}

TEST(NumericFormatterTest, FormatUnsignedTooSmallBuffer) {
    std::array<char, 2> buffer;  // Too small for 1000
    auto result = NumericFormatter::format_uint(1000, buffer);
    EXPECT_TRUE(result.empty());
}

TEST(NumericFormatterTest, FormatUnsignedEmptyBuffer) {
    std::span<char> empty_buffer;
    auto result = NumericFormatter::format_uint(42, empty_buffer);
    EXPECT_TRUE(result.empty());
}

// Test signed integer formatting
TEST(NumericFormatterTest, FormatSignedZero) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(0, buffer);
    EXPECT_EQ(result, "0");
}

TEST(NumericFormatterTest, FormatSignedPositive) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(42, buffer);
    EXPECT_EQ(result, "42");
}

TEST(NumericFormatterTest, FormatSignedNegative) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(-42, buffer);
    EXPECT_EQ(result, "-42");
}

TEST(NumericFormatterTest, FormatSignedNegativeSingleDigit) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(-1, buffer);
    EXPECT_EQ(result, "-1");
}

TEST(NumericFormatterTest, FormatSignedLargeNegative) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(-999999, buffer);
    EXPECT_EQ(result, "-999999");
}

TEST(NumericFormatterTest, FormatSignedMax) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(INT32_MAX, buffer);
    EXPECT_EQ(result, "2147483647");
}

TEST(NumericFormatterTest, FormatSignedMin) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(INT32_MIN, buffer);
    EXPECT_EQ(result, "-2147483648");
}

TEST(NumericFormatterTest, FormatSignedMinusOne) {
    std::array<char, 12> buffer;
    auto result = NumericFormatter::format_int(-1, buffer);
    EXPECT_EQ(result, "-1");
}

TEST(NumericFormatterTest, FormatSignedTooSmallBuffer) {
    std::array<char, 2> buffer;  // Too small for -100
    auto result = NumericFormatter::format_int(-100, buffer);
    EXPECT_TRUE(result.empty());
}

TEST(NumericFormatterTest, FormatSignedEmptyBuffer) {
    std::span<char> empty_buffer;
    auto result = NumericFormatter::format_int(-42, empty_buffer);
    EXPECT_TRUE(result.empty());
}

// Test convenience format() method
TEST(NumericFormatterTest, FormatConvenienceUnsigned) {
    auto result = NumericFormatter::format(42u);
    EXPECT_EQ(result, "42");
}

TEST(NumericFormatterTest, FormatConvenienceSigned) {
    auto result = NumericFormatter::format(-42);
    EXPECT_EQ(result, "-42");
}

TEST(NumericFormatterTest, FormatConvenienceZero) {
    auto result = NumericFormatter::format(0);
    EXPECT_EQ(result, "0");
}

TEST(NumericFormatterTest, FormatConvenienceUint8) {
    std::uint8_t value = 255;
    auto result = NumericFormatter::format(value);
    EXPECT_EQ(result, "255");
}

TEST(NumericFormatterTest, FormatConvenienceInt8) {
    std::int8_t value = -128;
    auto result = NumericFormatter::format(value);
    EXPECT_EQ(result, "-128");
}

TEST(NumericFormatterTest, FormatConvenienceUint16) {
    std::uint16_t value = 65535;
    auto result = NumericFormatter::format(value);
    EXPECT_EQ(result, "65535");
}

TEST(NumericFormatterTest, FormatConvenienceInt16) {
    std::int16_t value = -32768;
    auto result = NumericFormatter::format(value);
    EXPECT_EQ(result, "-32768");
}

// Test that format() matches std::to_string() for common values
TEST(NumericFormatterTest, MatchesStdToStringPositive) {
    for (int i = 0; i <= 1000; i += 100) {
        auto result = NumericFormatter::format(i);
        auto expected = std::to_string(i);
        EXPECT_EQ(result, expected) << "Mismatch for value: " << i;
    }
}

TEST(NumericFormatterTest, MatchesStdToStringNegative) {
    for (int i = 0; i >= -1000; i -= 100) {
        auto result = NumericFormatter::format(i);
        auto expected = std::to_string(i);
        EXPECT_EQ(result, expected) << "Mismatch for value: " << i;
    }
}

TEST(NumericFormatterTest, MatchesStdToStringEdgeCases) {
    // Test edge cases
    std::array<std::int32_t, 7> values = {0,
                                          1,
                                          -1,
                                          INT32_MAX,
                                          INT32_MIN,
                                          INT32_MAX - 1,
                                          INT32_MIN + 1};

    for (auto value : values) {
        auto result = NumericFormatter::format(value);
        auto expected = std::to_string(value);
        EXPECT_EQ(result, expected) << "Mismatch for value: " << value;
    }
}

// Test typical use cases from AT commands
TEST(NumericFormatterTest, ATCommandTypicalValues) {
    // Role: 0-1
    EXPECT_EQ(NumericFormatter::format(0), "0");
    EXPECT_EQ(NumericFormatter::format(1), "1");

    // Advertising interval: 0-9
    EXPECT_EQ(NumericFormatter::format(5), "5");

    // Baud rate: e.g., 9600
    EXPECT_EQ(NumericFormatter::format(9600), "9600");

    // Timeout: e.g., 000000-999999 (6 digits)
    EXPECT_EQ(NumericFormatter::format(123456), "123456");

    // Temperature: -40 to 125
    EXPECT_EQ(NumericFormatter::format(-40), "-40");
    EXPECT_EQ(NumericFormatter::format(125), "125");
}
