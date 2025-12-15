// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/util/numeric_formatter.hpp"

#include <gtest/gtest.h>

#include <string>

using hm11::ATCommandBuffer;
using hm11::util::NumericFormatter;

// Test basic construction and properties
TEST(ATCommandBufferTest, DefaultConstruction) {
    ATCommandBuffer<128> buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_FALSE(buffer.overflow());
    EXPECT_EQ(buffer.capacity(), 128);
}

// Test reset with command base
TEST(ATCommandBufferTest, ResetWithCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE");
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.view(), "AT+ROLE");
    EXPECT_FALSE(buffer.overflow());
}

// Test simple append operations
TEST(ATCommandBufferTest, AppendChar) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE");
    buffer.append('1');
    EXPECT_EQ(buffer.view(), "AT+ROLE1");
}

TEST(ATCommandBufferTest, AppendStringView) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE");
    buffer.append("1");
    EXPECT_EQ(buffer.view(), "AT+ROLE1");
}

// Test fluent interface
TEST(ATCommandBufferTest, FluentInterface) {
    ATCommandBuffer<128> buffer;
    auto view = buffer.reset("AT+ROLE").append("1").finalize();
    EXPECT_EQ(view, "AT+ROLE1\r\n");
}

// Test finalize adds CR LF
TEST(ATCommandBufferTest, FinalizeAddsCRLF) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE");
    buffer.append("1");
    auto view = buffer.finalize();
    EXPECT_EQ(view, "AT+ROLE1\r\n");
}

// Test c_str() method
TEST(ATCommandBufferTest, CStrNullTerminated) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE").append("1");
    const char* str = buffer.c_str();
    EXPECT_STREQ(str, "AT+ROLE1");
}

// Test typical AT commands
TEST(ATCommandBufferTest, SetRoleCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE").append("1").finalize();
    EXPECT_EQ(buffer.view(), "AT+ROLE1\r\n");
}

TEST(ATCommandBufferTest, SetAdvertisingIntervalCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ADVI").append("5").finalize();
    EXPECT_EQ(buffer.view(), "AT+ADVI5\r\n");
}

TEST(ATCommandBufferTest, SetBaudRateCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+BAUD")
        .append(NumericFormatter::format(9600))
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+BAUD9600\r\n");
}

TEST(ATCommandBufferTest, SetMACAddressCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+AD")
        .append("1")
        .append("001122334455")
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+AD1001122334455\r\n");
}

TEST(ATCommandBufferTest, SetiBeaconUUIDCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+IBE")
        .append("0")
        .append("0x12345678")
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+IBE00x12345678\r\n");
}

// Test overflow detection
TEST(ATCommandBufferTest, OverflowDetection) {
    ATCommandBuffer<16> buffer;  // Small buffer
    buffer.reset("AT+ROLE");  // "AT+ROLE" = 7 chars
    // Append enough to overflow (16 - 7 = 9 available)
    buffer.append("0123456789ABCDEF");  // 16 chars, should overflow
    EXPECT_TRUE(buffer.overflow());
    EXPECT_EQ(buffer.size(), 16);  // Should saturate at capacity
}

TEST(ATCommandBufferTest, OverflowSaturation) {
    ATCommandBuffer<16> buffer;
    buffer.reset("AT+ROLE");  // "AT+ROLE" = 7 chars
    buffer.append("0123456789");  // Total would be 17, but should saturate at 16
    EXPECT_TRUE(buffer.overflow());
    EXPECT_EQ(buffer.size(), 16);
    // Should have written "AT+ROLE012345678" (first 16 chars)
    EXPECT_EQ(buffer.view(), "AT+ROLE012345678");
}

TEST(ATCommandBufferTest, NoOverflowWhenFits) {
    ATCommandBuffer<16> buffer;
    buffer.reset("AT+ROLE");  // "AT+ROLE" = 7 chars
    buffer.append("1");  // Total 8 chars, fits
    EXPECT_FALSE(buffer.overflow());
    EXPECT_EQ(buffer.size(), 8);
}

// Test edge cases
TEST(ATCommandBufferTest, AppendEmptyString) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE");
    auto size_before = buffer.size();
    buffer.append("");
    EXPECT_EQ(buffer.size(), size_before);
    EXPECT_FALSE(buffer.overflow());
}

TEST(ATCommandBufferTest, MultipleAppends) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+AD")
        .append("1")
        .append("0011")
        .append("2233")
        .append("4455");
    EXPECT_EQ(buffer.view(), "AT+AD1001122334455");
}

TEST(ATCommandBufferTest, ResetClearsOverflow) {
    ATCommandBuffer<16> buffer;
    buffer.reset("AT+ROLE");
    buffer.append("0123456789ABCDEF");  // Cause overflow
    EXPECT_TRUE(buffer.overflow());

    buffer.reset("AT+IMME?");  // Reset should clear overflow
    EXPECT_FALSE(buffer.overflow());
    EXPECT_EQ(buffer.view(), "AT+IMME?");
}

// Test with different buffer sizes
TEST(ATCommandBufferTest, SmallBuffer32) {
    ATCommandBuffer<32> buffer;
    buffer.reset("AT+ROLE").append("1").finalize();
    EXPECT_EQ(buffer.view(), "AT+ROLE1\r\n");
    EXPECT_FALSE(buffer.overflow());
}

TEST(ATCommandBufferTest, LargeBuffer256) {
    ATCommandBuffer<256> buffer;
    buffer.reset("AT+NAME");
    // Append a long name (up to 12 chars typically)
    buffer.append("MyBLEDevice1").finalize();
    EXPECT_EQ(buffer.view(), "AT+NAMEMyBLEDevice1\r\n");
    EXPECT_FALSE(buffer.overflow());
}

// Test all query commands (with '?')
TEST(ATCommandBufferTest, QueryCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+IMME?").finalize();
    EXPECT_EQ(buffer.view(), "AT+IMME?\r\n");
}

TEST(ATCommandBufferTest, QueryWithParamsCommand) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+RAT??").finalize();
    EXPECT_EQ(buffer.view(), "AT+RAT??\r\n");
}

// Test numeric parameter formatting
TEST(ATCommandBufferTest, WithNumericFormatterPositive) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+COMI")
        .append(NumericFormatter::format(100))
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+COMI100\r\n");
}

TEST(ATCommandBufferTest, WithNumericFormatterZero) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+ROLE")
        .append(NumericFormatter::format(0))
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+ROLE0\r\n");
}

TEST(ATCommandBufferTest, WithNumericFormatterLarge) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+TCON")
        .append(NumericFormatter::format(123456))
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+TCON123456\r\n");
}

// Test longest commands to verify buffer sizing
TEST(ATCommandBufferTest, LongestCommandName) {
    ATCommandBuffer<128> buffer;
    buffer.reset("AT+FINDALLSERVICES?").finalize();
    EXPECT_EQ(buffer.view(), "AT+FINDALLSERVICES?\r\n");
    EXPECT_FALSE(buffer.overflow());
}

TEST(ATCommandBufferTest, LongestCommandWithParams) {
    ATCommandBuffer<128> buffer;
    // AT+PACK with 12 hex chars
    buffer.reset("AT+PACK")
        .append("0123456789AB")
        .finalize();
    EXPECT_EQ(buffer.view(), "AT+PACK0123456789AB\r\n");
    EXPECT_FALSE(buffer.overflow());
}

// Verify 128 byte buffer is sufficient for all commands
TEST(ATCommandBufferTest, WorstCaseCommand) {
    ATCommandBuffer<128> buffer;
    // Longest base + longest param + terminator
    buffer.reset("AT+FINDALLSERVICES?");  // 21 chars
    buffer.append("012345678901234567890123456789");  // 30 chars padding
    buffer.finalize();  // +2 for \r\n
    // Total: ~53 chars, well under 128
    EXPECT_FALSE(buffer.overflow());
}
