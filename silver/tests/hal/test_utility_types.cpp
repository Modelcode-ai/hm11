// Utility Types Tests - Tests for validators, parsing, string_builder, and fixed_string
// Ensures comprehensive coverage of utility types and helper functions

#include <gtest/gtest.h>
#include <hm11/types/validators.hpp>
#include <hm11/types/fixed_string.hpp>
#include <hm11/util/parsing.hpp>
#include <hm11/util/string_builder.hpp>
#include <hm11/hm11_driver.hpp>  // For VariableString definition
#include <cstring>

using namespace hm11;
using namespace hm11::util;

// ============================================================================
// Validator Tests - validators.hpp
// ============================================================================

TEST(UtilityTypesTest, UpperAlphaValidator_Validate) {
    EXPECT_TRUE(UpperAlphaValidator::validate('A'));
    EXPECT_TRUE(UpperAlphaValidator::validate('Z'));
    EXPECT_FALSE(UpperAlphaValidator::validate('a'));
    EXPECT_FALSE(UpperAlphaValidator::validate('0'));
}

TEST(UtilityTypesTest, UpperAlphaValidator_Name) {
    EXPECT_STREQ(UpperAlphaValidator::name(), "UpperAlpha");
}

TEST(UtilityTypesTest, LowerAlphaValidator_Validate) {
    EXPECT_TRUE(LowerAlphaValidator::validate('a'));
    EXPECT_TRUE(LowerAlphaValidator::validate('z'));
    EXPECT_FALSE(LowerAlphaValidator::validate('A'));
    EXPECT_FALSE(LowerAlphaValidator::validate('0'));
}

TEST(UtilityTypesTest, LowerAlphaValidator_Name) {
    EXPECT_STREQ(LowerAlphaValidator::name(), "LowerAlpha");
}

TEST(UtilityTypesTest, AlphaNumValidator_Validate) {
    EXPECT_TRUE(AlphaNumValidator::validate('A'));
    EXPECT_TRUE(AlphaNumValidator::validate('z'));
    EXPECT_TRUE(AlphaNumValidator::validate('5'));
    EXPECT_FALSE(AlphaNumValidator::validate('!'));
}

TEST(UtilityTypesTest, AlphaNumValidator_Name) {
    EXPECT_STREQ(AlphaNumValidator::name(), "AlphaNum");
}

TEST(UtilityTypesTest, MacCharValidator_Validate) {
    EXPECT_TRUE(MacCharValidator::validate('A'));
    EXPECT_TRUE(MacCharValidator::validate('5'));
    EXPECT_TRUE(MacCharValidator::validate(':'));
    EXPECT_FALSE(MacCharValidator::validate('G'));
}

TEST(UtilityTypesTest, MacCharValidator_Name) {
    EXPECT_STREQ(MacCharValidator::name(), "MacChar");
}

// ============================================================================
// Parsing Functions Tests - parsing.hpp
// ============================================================================

TEST(UtilityTypesTest, Parse_AdvertisingInterval) {
    auto result = parse_advertising_interval('0');
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, AdvertisingInterval::Ms100);

    auto invalid = parse_advertising_interval('X');
    EXPECT_FALSE(invalid.has_value());
}

TEST(UtilityTypesTest, Parse_AdvertisingMode) {
    auto result = parse_advertising_mode('0');
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, AdvertisingMode::NormalAdvertising);

    auto invalid = parse_advertising_mode('X');
    EXPECT_FALSE(invalid.has_value());
}

TEST(UtilityTypesTest, Parse_AdvertisingType) {
    auto result = parse_advertising_type('0');
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, AdvertisingType::AdvertisingScanResponseConnectable);

    auto invalid = parse_advertising_type('X');
    EXPECT_FALSE(invalid.has_value());
}

TEST(UtilityTypesTest, Parse_ConnectResult) {
    auto result = parse_connect_result('L');
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, ConnectResult::Connecting);

    auto connected = parse_connect_result(' ');
    EXPECT_TRUE(connected.has_value());
    EXPECT_EQ(*connected, ConnectResult::Connected);
}

// ============================================================================
// StringBuilder Tests - string_builder.hpp
// ============================================================================

TEST(UtilityTypesTest, StringBuilder_CStr) {
    StringBuilder<32> builder;
    builder.append("Hello");
    builder.append(' ');
    builder.append("World");

    const char* c_string = builder.c_str();
    EXPECT_STREQ(c_string, "Hello World");
    EXPECT_EQ(std::strlen(c_string), 11);
}

TEST(UtilityTypesTest, StringBuilder_ConstructorWithStringView) {
    StringBuilder<32> builder("Initial Content");
    EXPECT_EQ(builder.view(), "Initial Content");
    EXPECT_EQ(builder.size(), 15);
}

TEST(UtilityTypesTest, StringBuilder_OperatorPlusEqualsBuilder) {
    StringBuilder<32> builder1;
    builder1.append("Hello");

    StringBuilder<32> builder2;
    builder2.append(" World");

    builder1 += builder2;
    EXPECT_EQ(builder1.view(), "Hello World");
}

TEST(UtilityTypesTest, StringBuilder_Append) {
    StringBuilder<32> builder;
    builder.append("Test");
    EXPECT_EQ(builder.view(), "Test");
    EXPECT_EQ(builder.size(), 4);
}

TEST(UtilityTypesTest, StringBuilder_AppendChar) {
    StringBuilder<32> builder;
    builder.append_char('A');
    builder.append_char('B');
    EXPECT_EQ(builder.view(), "AB");
    EXPECT_EQ(builder.size(), 2);
}

TEST(UtilityTypesTest, StringBuilder_Clear) {
    StringBuilder<32> builder("Hello");
    builder.clear();
    EXPECT_EQ(builder.size(), 0);
    EXPECT_EQ(builder.view(), "");
}


// ============================================================================
// FixedString Tests - fixed_string.hpp
// ============================================================================

TEST(UtilityTypesTest, FixedString_Construction) {
    FixedString<6, HexCharValidator> hex("ABCDEF");
    EXPECT_EQ(hex.view(), "ABCDEF");
    EXPECT_EQ(hex.size(), 6);
}

TEST(UtilityTypesTest, FixedString_Comparison) {
    FixedString<4, HexCharValidator> hex1("ABCD");
    FixedString<4, HexCharValidator> hex2("ABCD");
    FixedString<4, HexCharValidator> hex3("1234");
    
    EXPECT_EQ(hex1, hex2);
    EXPECT_NE(hex1, hex3);
}

TEST(UtilityTypesTest, FixedString_View) {
    FixedString<8, HexCharValidator> hex("12345678");
    std::string_view view = hex.view();
    EXPECT_EQ(view, "12345678");
}

// ============================================================================
// VariableString Capacity Tests - hm11_driver.hpp
// ============================================================================

TEST(UtilityTypesTest, VariableString32_AssignExceedsCapacity) {
    VariableString<32> str;
    // Try to assign a string that's too long (33 chars)
    EXPECT_THROW(str.assign("123456789012345678901234567890123"), std::length_error);
}

TEST(UtilityTypesTest, VariableString16_AssignExceedsCapacity) {
    VariableString<16> str;
    // Try to assign a string that's too long (17 chars)
    EXPECT_THROW(str.assign("12345678901234567"), std::length_error);
}

TEST(UtilityTypesTest, VariableString64_AppendExceedsCapacity) {
    VariableString<64> str;
    // Fill to capacity
    for (int i = 0; i < 64; ++i) {
        str.append('X');
    }
    // Try to append one more - should throw
    EXPECT_THROW(str.append('Y'), std::length_error);
}

TEST(UtilityTypesTest, VariableString248_AppendStringViewExceedsCapacity) {
    VariableString<248> str;
    // Fill to near capacity (240 chars)
    std::string filler(240, 'A');
    str.assign(filler);

    // Try to append 9 more chars (would exceed capacity of 248)
    EXPECT_THROW(str.append("123456789"), std::length_error);
}

TEST(UtilityTypesTest, VariableString32_AppendStringViewExceedsCapacity) {
    VariableString<32> str;
    str.assign("12345678901234567890");  // 20 chars

    // Try to append 13 more chars (total would be 33, exceeds 32)
    EXPECT_THROW(str.append("1234567890123"), std::length_error);
}

TEST(UtilityTypesTest, VariableString16_AppendCharExceedsCapacity) {
    VariableString<16> str;
    // Fill to capacity
    for (int i = 0; i < 16; ++i) {
        str.append('X');
    }
    // Try to append one more
    EXPECT_THROW(str.append('Y'), std::length_error);
}

TEST(UtilityTypesTest, VariableString4_AssignExactCapacity) {
    VariableString<4> str;
    // Assign exactly 4 chars - should succeed
    EXPECT_NO_THROW(str.assign("ABCD"));
    EXPECT_EQ(str.view(), "ABCD");
}

TEST(UtilityTypesTest, VariableString4_AssignOverCapacity) {
    VariableString<4> str;
    // Try to assign 5 chars - should throw
    EXPECT_THROW(str.assign("ABCDE"), std::length_error);
}

TEST(UtilityTypesTest, VariableString3_AppendExceedsCapacity) {
    VariableString<3> str;
    str.append('A');
    str.append('B');
    str.append('C');
    // Try to append one more
    EXPECT_THROW(str.append('D'), std::length_error);
}

TEST(UtilityTypesTest, VariableString248_AssignExceedsCapacity) {
    VariableString<248> str;
    // Create a string that's 249 chars (exceeds capacity of 248)
    std::string too_long(249, 'X');
    EXPECT_THROW(str.assign(too_long), std::length_error);
}
