// String Builder Coverage Tests
// Increase coverage for util/string_builder.hpp from 23% to target

#include <gtest/gtest.h>
#include <hm11/util/string_builder.hpp>

using namespace hm11::util;

class StringBuilderCoverageTest : public ::testing::Test {
};

// ============================================================================
// Basic Operations
// ============================================================================

TEST_F(StringBuilderCoverageTest, DefaultConstruction) {
    StringBuilder<64> sb;
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0);
    EXPECT_EQ(sb.remaining_capacity(), 64);
    EXPECT_FALSE(sb.full());
}

TEST_F(StringBuilderCoverageTest, ConstructionFromStringView) {
    StringBuilder<64> sb("Hello");
    EXPECT_FALSE(sb.empty());
    EXPECT_EQ(sb.size(), 5);
    EXPECT_EQ(sb.view(), "Hello");
}

TEST_F(StringBuilderCoverageTest, CopyConstructor) {
    StringBuilder<64> sb1("Test");
    StringBuilder<64> sb2(sb1);
    EXPECT_EQ(sb1.view(), sb2.view());
    EXPECT_EQ(sb1.size(), sb2.size());
}

TEST_F(StringBuilderCoverageTest, CopyAssignment) {
    StringBuilder<64> sb1("Original");
    StringBuilder<64> sb2;
    sb2 = sb1;
    EXPECT_EQ(sb1.view(), sb2.view());
}

TEST_F(StringBuilderCoverageTest, MoveConstructor) {
    StringBuilder<64> sb1("Moving");
    StringBuilder<64> sb2(std::move(sb1));
    EXPECT_EQ(sb2.view(), "Moving");
}

TEST_F(StringBuilderCoverageTest, MoveAssignment) {
    StringBuilder<64> sb1("Move");
    StringBuilder<64> sb2;
    sb2 = std::move(sb1);
    EXPECT_EQ(sb2.view(), "Move");
}

TEST_F(StringBuilderCoverageTest, Clear) {
    StringBuilder<64> sb("Data");
    EXPECT_FALSE(sb.empty());
    sb.clear();
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0);
}

TEST_F(StringBuilderCoverageTest, ViewAndData) {
    StringBuilder<64> sb("Content");
    EXPECT_EQ(sb.view(), "Content");
    EXPECT_EQ(std::string_view(sb.data(), sb.size()), "Content");
}

TEST_F(StringBuilderCoverageTest, CString) {
    StringBuilder<64> sb("NullTerm");
    const char* cstr = sb.c_str();
    EXPECT_STREQ(cstr, "NullTerm");
}

// ============================================================================
// Append Operations
// ============================================================================

TEST_F(StringBuilderCoverageTest, AppendStringView) {
    StringBuilder<64> sb;
    sb.append("Hello");
    EXPECT_EQ(sb.view(), "Hello");
    sb.append(" World");
    EXPECT_EQ(sb.view(), "Hello World");
}

TEST_F(StringBuilderCoverageTest, AppendChar) {
    StringBuilder<64> sb;
    sb.append('A');
    sb.append('T');
    EXPECT_EQ(sb.view(), "AT");
}

TEST_F(StringBuilderCoverageTest, AppendCharAlias) {
    StringBuilder<64> sb;
    sb.append_char('X');
    sb.append_char('Y');
    EXPECT_EQ(sb.view(), "XY");
}

TEST_F(StringBuilderCoverageTest, AppendMultiple) {
    StringBuilder<64> sb;
    sb.append("AT").append('+').append("VER");
    EXPECT_EQ(sb.view(), "AT+VER");
}

// ============================================================================
// Integer Append
// ============================================================================

TEST_F(StringBuilderCoverageTest, AppendPositiveInt) {
    StringBuilder<64> sb;
    sb.append_int(42);
    EXPECT_EQ(sb.view(), "42");
}

TEST_F(StringBuilderCoverageTest, AppendNegativeInt) {
    StringBuilder<64> sb;
    sb.append_int(-123);
    EXPECT_EQ(sb.view(), "-123");
}

TEST_F(StringBuilderCoverageTest, AppendZero) {
    StringBuilder<64> sb;
    sb.append_int(0);
    EXPECT_EQ(sb.view(), "0");
}

TEST_F(StringBuilderCoverageTest, AppendLargePositiveInt) {
    StringBuilder<64> sb;
    sb.append_int(999999);
    EXPECT_EQ(sb.view(), "999999");
}

TEST_F(StringBuilderCoverageTest, AppendLargeNegativeInt) {
    StringBuilder<64> sb;
    sb.append_int(-999999);
    EXPECT_EQ(sb.view(), "-999999");
}

TEST_F(StringBuilderCoverageTest, AppendUnsignedInt) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<uint32_t>(12345));
    EXPECT_EQ(sb.view(), "12345");
}

TEST_F(StringBuilderCoverageTest, AppendInt8) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<int8_t>(127));
    EXPECT_EQ(sb.view(), "127");

    sb.clear();
    sb.append_int(static_cast<int8_t>(-128));
    EXPECT_EQ(sb.view(), "-128");
}

TEST_F(StringBuilderCoverageTest, AppendUint8) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<uint8_t>(255));
    EXPECT_EQ(sb.view(), "255");
}

TEST_F(StringBuilderCoverageTest, AppendInt16) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<int16_t>(32767));
    EXPECT_EQ(sb.view(), "32767");

    sb.clear();
    sb.append_int(static_cast<int16_t>(-32768));
    EXPECT_EQ(sb.view(), "-32768");
}

TEST_F(StringBuilderCoverageTest, AppendUint16) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<uint16_t>(65535));
    EXPECT_EQ(sb.view(), "65535");
}

TEST_F(StringBuilderCoverageTest, AppendInt64) {
    StringBuilder<64> sb;
    sb.append_int(static_cast<int64_t>(9223372036854775807LL));
    EXPECT_EQ(sb.view(), "9223372036854775807");
}

// ============================================================================
// Hex Append
// ============================================================================

TEST_F(StringBuilderCoverageTest, AppendHexChar_Digits) {
    StringBuilder<64> sb;
    for (uint8_t i = 0; i < 10; ++i) {
        sb.append_hex_char(i);
    }
    EXPECT_EQ(sb.view(), "0123456789");
}

TEST_F(StringBuilderCoverageTest, AppendHexChar_Letters) {
    StringBuilder<64> sb;
    for (uint8_t i = 10; i < 16; ++i) {
        sb.append_hex_char(i);
    }
    EXPECT_EQ(sb.view(), "ABCDEF");
}

TEST_F(StringBuilderCoverageTest, AppendHex8) {
    StringBuilder<64> sb;
    sb.append_hex(static_cast<uint8_t>(0xFF));
    EXPECT_EQ(sb.view(), "FF");

    sb.clear();
    sb.append_hex(static_cast<uint8_t>(0x00));
    EXPECT_EQ(sb.view(), "00");

    sb.clear();
    sb.append_hex(static_cast<uint8_t>(0xA5));
    EXPECT_EQ(sb.view(), "A5");
}

TEST_F(StringBuilderCoverageTest, AppendHex16) {
    StringBuilder<64> sb;
    sb.append_hex(static_cast<uint16_t>(0xABCD));
    EXPECT_EQ(sb.view(), "ABCD");

    sb.clear();
    sb.append_hex(static_cast<uint16_t>(0x0000));
    EXPECT_EQ(sb.view(), "0000");

    sb.clear();
    sb.append_hex(static_cast<uint16_t>(0x1234));
    EXPECT_EQ(sb.view(), "1234");
}

TEST_F(StringBuilderCoverageTest, AppendHex32) {
    StringBuilder<64> sb;
    sb.append_hex(static_cast<uint32_t>(0xDEADBEEF));
    EXPECT_EQ(sb.view(), "DEADBEEF");

    sb.clear();
    sb.append_hex(static_cast<uint32_t>(0x12345678));
    EXPECT_EQ(sb.view(), "12345678");
}

TEST_F(StringBuilderCoverageTest, AppendHex64) {
    StringBuilder<64> sb;
    sb.append_hex(static_cast<uint64_t>(0x0123456789ABCDEF));
    EXPECT_EQ(sb.view(), "0123456789ABCDEF");
}

// ============================================================================
// Capacity and Error Handling
// ============================================================================

TEST_F(StringBuilderCoverageTest, RemainingCapacity) {
    StringBuilder<10> sb;
    EXPECT_EQ(sb.remaining_capacity(), 10);

    sb.append("Hello");
    EXPECT_EQ(sb.remaining_capacity(), 5);

    sb.append("World");
    EXPECT_EQ(sb.remaining_capacity(), 0);
    EXPECT_TRUE(sb.full());
}

TEST_F(StringBuilderCoverageTest, AppendStringView_OverCapacity) {
    StringBuilder<5> sb;
    EXPECT_THROW(sb.append("TooLong"), std::length_error);
}

TEST_F(StringBuilderCoverageTest, AppendChar_AtCapacity) {
    StringBuilder<3> sb;
    sb.append('A');
    sb.append('B');
    sb.append('C');
    EXPECT_TRUE(sb.full());
    EXPECT_THROW(sb.append('D'), std::length_error);
}

TEST_F(StringBuilderCoverageTest, AppendExactCapacity) {
    StringBuilder<5> sb;
    sb.append("12345");
    EXPECT_TRUE(sb.full());
    EXPECT_EQ(sb.remaining_capacity(), 0);
}

TEST_F(StringBuilderCoverageTest, MultipleAppendsReachingCapacity) {
    StringBuilder<10> sb;
    sb.append("AT");      // 2 chars
    sb.append('+');       // 1 char -> 3 total
    sb.append("VER");     // 3 chars -> 6 total
    sb.append("1234");    // 4 chars -> 10 total
    EXPECT_TRUE(sb.full());
    EXPECT_EQ(sb.size(), 10);
}

// ============================================================================
// Complex Scenarios
// ============================================================================

TEST_F(StringBuilderCoverageTest, ATCommandConstruction) {
    StringBuilder<64> sb;
    sb.append("AT+SET:");
    sb.append_int(42);
    EXPECT_EQ(sb.view(), "AT+SET:42");
}

TEST_F(StringBuilderCoverageTest, HexATCommand) {
    StringBuilder<64> sb;
    sb.append("AT+UUID:0x");
    sb.append_hex(static_cast<uint32_t>(0xABCD1234));
    EXPECT_EQ(sb.view(), "AT+UUID:0xABCD1234");
}

TEST_F(StringBuilderCoverageTest, MixedContentCommand) {
    StringBuilder<128> sb;
    sb.append("AT+CONFIG:");
    sb.append_int(100);
    sb.append(',');
    sb.append("0x");
    sb.append_hex(static_cast<uint8_t>(0xFF));
    sb.append(',');
    sb.append_int(-50);
    EXPECT_EQ(sb.view(), "AT+CONFIG:100,0xFF,-50");
}

TEST_F(StringBuilderCoverageTest, ReuseAfterClear) {
    StringBuilder<64> sb;
    sb.append("First");
    EXPECT_EQ(sb.view(), "First");

    sb.clear();
    sb.append("Second");
    EXPECT_EQ(sb.view(), "Second");

    sb.clear();
    sb.append_int(123);
    EXPECT_EQ(sb.view(), "123");
}

TEST_F(StringBuilderCoverageTest, EmptyStringAppend) {
    StringBuilder<64> sb;
    sb.append("");
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0);
}

TEST_F(StringBuilderCoverageTest, SingleDigitNumbers) {
    StringBuilder<64> sb;
    for (int i = 0; i < 10; ++i) {
        sb.append_int(i);
    }
    EXPECT_EQ(sb.view(), "0123456789");
}

TEST_F(StringBuilderCoverageTest, NegativeSingleDigit) {
    StringBuilder<64> sb;
    sb.append_int(-1);
    EXPECT_EQ(sb.view(), "-1");

    sb.clear();
    sb.append_int(-9);
    EXPECT_EQ(sb.view(), "-9");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StringBuilderCoverageTest, VerySmallCapacity) {
    StringBuilder<1> sb;
    sb.append('X');
    EXPECT_TRUE(sb.full());
    EXPECT_THROW(sb.append('Y'), std::length_error);
}

TEST_F(StringBuilderCoverageTest, AppendToFull) {
    StringBuilder<5> sb("12345");
    EXPECT_TRUE(sb.full());
    EXPECT_THROW(sb.append("6"), std::length_error);
    EXPECT_THROW(sb.append('6'), std::length_error);
}

TEST_F(StringBuilderCoverageTest, LargeCapacity) {
    StringBuilder<512> sb;
    std::string long_string(500, 'A');
    sb.append(long_string);
    EXPECT_EQ(sb.size(), 500);
    EXPECT_EQ(sb.remaining_capacity(), 12);
}

TEST_F(StringBuilderCoverageTest, IntWithMaxValues) {
    StringBuilder<128> sb;

    // Max int32_t
    sb.append_int(std::numeric_limits<int32_t>::max());
    sb.clear();

    // Min int32_t
    sb.append_int(std::numeric_limits<int32_t>::min());
    sb.clear();

    // Max uint32_t
    sb.append_int(std::numeric_limits<uint32_t>::max());
    EXPECT_FALSE(sb.empty());
}

TEST_F(StringBuilderCoverageTest, ChainedOperations) {
    StringBuilder<128> sb;
    sb.append("AT")
      .append('+')
      .append("CMD")
      .append(':')
      .append_int(100)
      .append(',')
      .append_hex(static_cast<uint8_t>(0xAB));

    EXPECT_EQ(sb.view(), "AT+CMD:100,AB");
}

TEST_F(StringBuilderCoverageTest, RepeatedClearAndAppend) {
    StringBuilder<64> sb;

    for (int i = 0; i < 10; ++i) {
        sb.append("Test");
        EXPECT_EQ(sb.size(), 4);
        sb.clear();
        EXPECT_TRUE(sb.empty());
    }
}

TEST_F(StringBuilderCoverageTest, AllHexDigits) {
    StringBuilder<64> sb;

    // Test all possible hex nibbles
    for (uint8_t i = 0; i <= 15; ++i) {
        sb.clear();
        sb.append_hex_char(i);
        EXPECT_EQ(sb.size(), 1);
    }
}

TEST_F(StringBuilderCoverageTest, IntegerBoundaries) {
    StringBuilder<128> sb;

    // Test around zero
    sb.append_int(-1);
    sb.append(',');
    sb.append_int(0);
    sb.append(',');
    sb.append_int(1);
    EXPECT_EQ(sb.view(), "-1,0,1");
}

TEST_F(StringBuilderCoverageTest, HexWithLeadingZeros) {
    StringBuilder<64> sb;
    sb.append_hex(static_cast<uint8_t>(0x01));
    EXPECT_EQ(sb.view(), "01");

    sb.clear();
    sb.append_hex(static_cast<uint16_t>(0x000F));
    EXPECT_EQ(sb.view(), "000F");
}
