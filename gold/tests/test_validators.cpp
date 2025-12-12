//
// test_validators.cpp - Unit tests for character validators
//
// Tests for HexCharValidator and DigitCharValidator used by FixedString template.
//

#include <gtest/gtest.h>

#include "hm11/types/validators.hpp"

namespace hm11 {
namespace {

// ============================================================================
// HexCharValidator Tests
// ============================================================================

TEST(HexCharValidatorTest, ValidatesDecimalDigits) {
    EXPECT_TRUE(HexCharValidator::is_valid('0'));
    EXPECT_TRUE(HexCharValidator::is_valid('1'));
    EXPECT_TRUE(HexCharValidator::is_valid('2'));
    EXPECT_TRUE(HexCharValidator::is_valid('3'));
    EXPECT_TRUE(HexCharValidator::is_valid('4'));
    EXPECT_TRUE(HexCharValidator::is_valid('5'));
    EXPECT_TRUE(HexCharValidator::is_valid('6'));
    EXPECT_TRUE(HexCharValidator::is_valid('7'));
    EXPECT_TRUE(HexCharValidator::is_valid('8'));
    EXPECT_TRUE(HexCharValidator::is_valid('9'));
}

TEST(HexCharValidatorTest, ValidatesUppercaseHexLetters) {
    EXPECT_TRUE(HexCharValidator::is_valid('A'));
    EXPECT_TRUE(HexCharValidator::is_valid('B'));
    EXPECT_TRUE(HexCharValidator::is_valid('C'));
    EXPECT_TRUE(HexCharValidator::is_valid('D'));
    EXPECT_TRUE(HexCharValidator::is_valid('E'));
    EXPECT_TRUE(HexCharValidator::is_valid('F'));
}

TEST(HexCharValidatorTest, RejectsLowercaseHexLetters) {
    EXPECT_FALSE(HexCharValidator::is_valid('a'));
    EXPECT_FALSE(HexCharValidator::is_valid('b'));
    EXPECT_FALSE(HexCharValidator::is_valid('c'));
    EXPECT_FALSE(HexCharValidator::is_valid('d'));
    EXPECT_FALSE(HexCharValidator::is_valid('e'));
    EXPECT_FALSE(HexCharValidator::is_valid('f'));
}

TEST(HexCharValidatorTest, RejectsNonHexCharacters) {
    // Characters between '9' and 'A' in ASCII
    EXPECT_FALSE(HexCharValidator::is_valid(':'));
    EXPECT_FALSE(HexCharValidator::is_valid(';'));
    EXPECT_FALSE(HexCharValidator::is_valid('<'));
    EXPECT_FALSE(HexCharValidator::is_valid('='));
    EXPECT_FALSE(HexCharValidator::is_valid('>'));
    EXPECT_FALSE(HexCharValidator::is_valid('?'));
    EXPECT_FALSE(HexCharValidator::is_valid('@'));

    // Characters after 'F'
    EXPECT_FALSE(HexCharValidator::is_valid('G'));
    EXPECT_FALSE(HexCharValidator::is_valid('Z'));

    // Characters before '0'
    EXPECT_FALSE(HexCharValidator::is_valid('/'));
    EXPECT_FALSE(HexCharValidator::is_valid(' '));

    // Special characters
    EXPECT_FALSE(HexCharValidator::is_valid('\0'));
    EXPECT_FALSE(HexCharValidator::is_valid('\n'));
    EXPECT_FALSE(HexCharValidator::is_valid('\t'));
    EXPECT_FALSE(HexCharValidator::is_valid('-'));
    EXPECT_FALSE(HexCharValidator::is_valid('_'));
}

TEST(HexCharValidatorTest, ValidateMethodDoesNotThrow) {
    // In release builds, validate() should not throw
    // In debug builds, it will assert
#ifndef NDEBUG
    // Skip this test in debug mode as it will assert
    GTEST_SKIP() << "Skipping in debug mode (assertions enabled)";
#else
    HexCharValidator::validate('0', 0);
    HexCharValidator::validate('A', 0);
    HexCharValidator::validate('F', 0);
    SUCCEED();
#endif
}

TEST(HexCharValidatorTest, HasCorrectName) {
    EXPECT_STREQ(HexCharValidator::name(), "HexCharValidator");
}

TEST(HexCharValidatorTest, ConstexprValidation) {
    // Test that is_valid can be used in constexpr context
    static_assert(HexCharValidator::is_valid('0'), "Should validate '0'");
    static_assert(HexCharValidator::is_valid('9'), "Should validate '9'");
    static_assert(HexCharValidator::is_valid('A'), "Should validate 'A'");
    static_assert(HexCharValidator::is_valid('F'), "Should validate 'F'");
    static_assert(!HexCharValidator::is_valid('G'), "Should reject 'G'");
    static_assert(!HexCharValidator::is_valid('a'), "Should reject 'a'");
}

// ============================================================================
// DigitCharValidator Tests
// ============================================================================

TEST(DigitCharValidatorTest, ValidatesDecimalDigits) {
    EXPECT_TRUE(DigitCharValidator::is_valid('0'));
    EXPECT_TRUE(DigitCharValidator::is_valid('1'));
    EXPECT_TRUE(DigitCharValidator::is_valid('2'));
    EXPECT_TRUE(DigitCharValidator::is_valid('3'));
    EXPECT_TRUE(DigitCharValidator::is_valid('4'));
    EXPECT_TRUE(DigitCharValidator::is_valid('5'));
    EXPECT_TRUE(DigitCharValidator::is_valid('6'));
    EXPECT_TRUE(DigitCharValidator::is_valid('7'));
    EXPECT_TRUE(DigitCharValidator::is_valid('8'));
    EXPECT_TRUE(DigitCharValidator::is_valid('9'));
}

TEST(DigitCharValidatorTest, RejectsHexLetters) {
    EXPECT_FALSE(DigitCharValidator::is_valid('A'));
    EXPECT_FALSE(DigitCharValidator::is_valid('B'));
    EXPECT_FALSE(DigitCharValidator::is_valid('C'));
    EXPECT_FALSE(DigitCharValidator::is_valid('D'));
    EXPECT_FALSE(DigitCharValidator::is_valid('E'));
    EXPECT_FALSE(DigitCharValidator::is_valid('F'));

    EXPECT_FALSE(DigitCharValidator::is_valid('a'));
    EXPECT_FALSE(DigitCharValidator::is_valid('b'));
    EXPECT_FALSE(DigitCharValidator::is_valid('c'));
    EXPECT_FALSE(DigitCharValidator::is_valid('d'));
    EXPECT_FALSE(DigitCharValidator::is_valid('e'));
    EXPECT_FALSE(DigitCharValidator::is_valid('f'));
}

TEST(DigitCharValidatorTest, RejectsNonDigitCharacters) {
    // Characters before '0'
    EXPECT_FALSE(DigitCharValidator::is_valid('/'));
    EXPECT_FALSE(DigitCharValidator::is_valid(' '));

    // Characters after '9'
    EXPECT_FALSE(DigitCharValidator::is_valid(':'));
    EXPECT_FALSE(DigitCharValidator::is_valid(';'));

    // Special characters
    EXPECT_FALSE(DigitCharValidator::is_valid('\0'));
    EXPECT_FALSE(DigitCharValidator::is_valid('\n'));
    EXPECT_FALSE(DigitCharValidator::is_valid('\t'));
    EXPECT_FALSE(DigitCharValidator::is_valid('-'));
    EXPECT_FALSE(DigitCharValidator::is_valid('+'));
    EXPECT_FALSE(DigitCharValidator::is_valid('.'));
}

TEST(DigitCharValidatorTest, ValidateMethodDoesNotThrow) {
    // In release builds, validate() should not throw
    // In debug builds, it will assert
#ifndef NDEBUG
    // Skip this test in debug mode as it will assert
    GTEST_SKIP() << "Skipping in debug mode (assertions enabled)";
#else
    DigitCharValidator::validate('0', 0);
    DigitCharValidator::validate('5', 0);
    DigitCharValidator::validate('9', 0);
    SUCCEED();
#endif
}

TEST(DigitCharValidatorTest, HasCorrectName) {
    EXPECT_STREQ(DigitCharValidator::name(), "DigitCharValidator");
}

TEST(DigitCharValidatorTest, ConstexprValidation) {
    // Test that is_valid can be used in constexpr context
    static_assert(DigitCharValidator::is_valid('0'), "Should validate '0'");
    static_assert(DigitCharValidator::is_valid('9'), "Should validate '9'");
    static_assert(!DigitCharValidator::is_valid('A'), "Should reject 'A'");
    static_assert(!DigitCharValidator::is_valid('a'), "Should reject 'a'");
    static_assert(!DigitCharValidator::is_valid('/'), "Should reject '/'");
    static_assert(!DigitCharValidator::is_valid(':'), "Should reject ':'");
}

// ============================================================================
// Validator Comparison Tests
// ============================================================================

TEST(ValidatorComparisonTest, HexValidatorIsMorePermissiveThanDigit) {
    // All digits should be valid hex
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_TRUE(HexCharValidator::is_valid(c));
        EXPECT_TRUE(DigitCharValidator::is_valid(c));
    }

    // Hex letters should be valid for hex but not for digit
    for (char c = 'A'; c <= 'F'; ++c) {
        EXPECT_TRUE(HexCharValidator::is_valid(c));
        EXPECT_FALSE(DigitCharValidator::is_valid(c));
    }
}

} // namespace
} // namespace hm11
