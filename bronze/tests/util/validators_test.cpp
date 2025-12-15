/**
 * @file validators_test.cpp
 * @brief Tests for character validators
 *
 * This file contains tests for the character validator policy classes
 * used by FixedString.
 */

#include <gtest/gtest.h>

#include "hm11/util/validators.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for validator tests
 */
class ValidatorsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No specific setup needed
    }

    void TearDown() override {
        // No specific teardown needed
    }
};

// Test HexCharValidator
TEST_F(ValidatorsTest, HexCharValidatorValidChars) {
    using util::HexCharValidator;

    // Valid hex digits
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
    EXPECT_TRUE(HexCharValidator::is_valid('A'));
    EXPECT_TRUE(HexCharValidator::is_valid('B'));
    EXPECT_TRUE(HexCharValidator::is_valid('C'));
    EXPECT_TRUE(HexCharValidator::is_valid('D'));
    EXPECT_TRUE(HexCharValidator::is_valid('E'));
    EXPECT_TRUE(HexCharValidator::is_valid('F'));
}

TEST_F(ValidatorsTest, HexCharValidatorInvalidChars) {
    using util::HexCharValidator;

    // Invalid chars (lowercase, special chars, etc.)
    EXPECT_FALSE(HexCharValidator::is_valid('a'));
    EXPECT_FALSE(HexCharValidator::is_valid('f'));
    EXPECT_FALSE(HexCharValidator::is_valid('g'));
    EXPECT_FALSE(HexCharValidator::is_valid('G'));
    EXPECT_FALSE(HexCharValidator::is_valid('Z'));
    EXPECT_FALSE(HexCharValidator::is_valid('-'));
    EXPECT_FALSE(HexCharValidator::is_valid('+'));
    EXPECT_FALSE(HexCharValidator::is_valid(' '));
    EXPECT_FALSE(HexCharValidator::is_valid('\n'));
}

TEST_F(ValidatorsTest, HexCharValidatorDescription) {
    using util::HexCharValidator;
    EXPECT_EQ(HexCharValidator::description(), "Hexadecimal character (0-9, A-F)");
}

// Test DigitValidator
TEST_F(ValidatorsTest, DigitValidatorValidChars) {
    using util::DigitValidator;

    EXPECT_TRUE(DigitValidator::is_valid('0'));
    EXPECT_TRUE(DigitValidator::is_valid('1'));
    EXPECT_TRUE(DigitValidator::is_valid('2'));
    EXPECT_TRUE(DigitValidator::is_valid('3'));
    EXPECT_TRUE(DigitValidator::is_valid('4'));
    EXPECT_TRUE(DigitValidator::is_valid('5'));
    EXPECT_TRUE(DigitValidator::is_valid('6'));
    EXPECT_TRUE(DigitValidator::is_valid('7'));
    EXPECT_TRUE(DigitValidator::is_valid('8'));
    EXPECT_TRUE(DigitValidator::is_valid('9'));
}

TEST_F(ValidatorsTest, DigitValidatorInvalidChars) {
    using util::DigitValidator;

    EXPECT_FALSE(DigitValidator::is_valid('A'));
    EXPECT_FALSE(DigitValidator::is_valid('a'));
    EXPECT_FALSE(DigitValidator::is_valid('F'));
    EXPECT_FALSE(DigitValidator::is_valid('z'));
    EXPECT_FALSE(DigitValidator::is_valid('-'));
    EXPECT_FALSE(DigitValidator::is_valid('+'));
    EXPECT_FALSE(DigitValidator::is_valid(' '));
}

TEST_F(ValidatorsTest, DigitValidatorDescription) {
    using util::DigitValidator;
    EXPECT_EQ(DigitValidator::description(), "Decimal digit character (0-9)");
}

// Test AnyCharValidator
TEST_F(ValidatorsTest, AnyCharValidatorAcceptsAll) {
    using util::AnyCharValidator;

    // Should accept any character
    EXPECT_TRUE(AnyCharValidator::is_valid('0'));
    EXPECT_TRUE(AnyCharValidator::is_valid('A'));
    EXPECT_TRUE(AnyCharValidator::is_valid('a'));
    EXPECT_TRUE(AnyCharValidator::is_valid('z'));
    EXPECT_TRUE(AnyCharValidator::is_valid(' '));
    EXPECT_TRUE(AnyCharValidator::is_valid('\n'));
    EXPECT_TRUE(AnyCharValidator::is_valid('\t'));
    EXPECT_TRUE(AnyCharValidator::is_valid('!'));
    EXPECT_TRUE(AnyCharValidator::is_valid('@'));
    EXPECT_TRUE(AnyCharValidator::is_valid('#'));
}

TEST_F(ValidatorsTest, AnyCharValidatorDescription) {
    using util::AnyCharValidator;
    EXPECT_EQ(AnyCharValidator::description(), "Any character");
}

// Test AlphaCharValidator
TEST_F(ValidatorsTest, AlphaCharValidatorValidChars) {
    using util::AlphaCharValidator;

    // Valid alphabetic characters
    EXPECT_TRUE(AlphaCharValidator::is_valid('A'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('B'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('Z'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('a'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('b'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('z'));
}

TEST_F(ValidatorsTest, AlphaCharValidatorInvalidChars) {
    using util::AlphaCharValidator;

    // Invalid chars (digits, special chars)
    EXPECT_FALSE(AlphaCharValidator::is_valid('0'));
    EXPECT_FALSE(AlphaCharValidator::is_valid('9'));
    EXPECT_FALSE(AlphaCharValidator::is_valid(' '));
    EXPECT_FALSE(AlphaCharValidator::is_valid('-'));
    EXPECT_FALSE(AlphaCharValidator::is_valid('_'));
    EXPECT_FALSE(AlphaCharValidator::is_valid('!'));
}

TEST_F(ValidatorsTest, AlphaCharValidatorDescription) {
    using util::AlphaCharValidator;
    EXPECT_EQ(AlphaCharValidator::description(), "Alphabetic character (A-Z, a-z)");
}

// Test AlphaNumValidator
TEST_F(ValidatorsTest, AlphaNumValidatorValidChars) {
    using util::AlphaNumValidator;

    // Valid alphanumeric characters
    EXPECT_TRUE(AlphaNumValidator::is_valid('A'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('Z'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('a'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('z'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('0'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('9'));
}

TEST_F(ValidatorsTest, AlphaNumValidatorInvalidChars) {
    using util::AlphaNumValidator;

    // Invalid chars (special chars)
    EXPECT_FALSE(AlphaNumValidator::is_valid(' '));
    EXPECT_FALSE(AlphaNumValidator::is_valid('-'));
    EXPECT_FALSE(AlphaNumValidator::is_valid('_'));
    EXPECT_FALSE(AlphaNumValidator::is_valid('!'));
    EXPECT_FALSE(AlphaNumValidator::is_valid('@'));
    EXPECT_FALSE(AlphaNumValidator::is_valid('\n'));
}

TEST_F(ValidatorsTest, AlphaNumValidatorDescription) {
    using util::AlphaNumValidator;
    EXPECT_EQ(AlphaNumValidator::description(), "Alphanumeric character (A-Z, a-z, 0-9)");
}

// Test PrintableCharValidator
TEST_F(ValidatorsTest, PrintableCharValidatorValidChars) {
    using util::PrintableCharValidator;

    // Valid printable characters (ASCII 32-126)
    EXPECT_TRUE(PrintableCharValidator::is_valid(' '));  // ASCII 32
    EXPECT_TRUE(PrintableCharValidator::is_valid('!'));  // ASCII 33
    EXPECT_TRUE(PrintableCharValidator::is_valid('0'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('A'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('Z'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('a'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('z'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('~'));  // ASCII 126
}

TEST_F(ValidatorsTest, PrintableCharValidatorInvalidChars) {
    using util::PrintableCharValidator;

    // Invalid chars (control characters)
    EXPECT_FALSE(PrintableCharValidator::is_valid('\0'));  // ASCII 0
    EXPECT_FALSE(PrintableCharValidator::is_valid('\n'));  // ASCII 10
    EXPECT_FALSE(PrintableCharValidator::is_valid('\r'));  // ASCII 13
    EXPECT_FALSE(PrintableCharValidator::is_valid('\t'));  // ASCII 9
    EXPECT_FALSE(PrintableCharValidator::is_valid(static_cast<char>(127))); // DEL
    EXPECT_FALSE(PrintableCharValidator::is_valid(static_cast<char>(31)));  // Below space
}

TEST_F(ValidatorsTest, PrintableCharValidatorDescription) {
    using util::PrintableCharValidator;
    EXPECT_EQ(PrintableCharValidator::description(), "Printable character (ASCII 32-126)");
}

// Test CharacterValidator concept
TEST_F(ValidatorsTest, CharacterValidatorConcept) {
    // Verify that all validators satisfy the CharacterValidator concept
    static_assert(util::CharacterValidator<util::HexCharValidator>);
    static_assert(util::CharacterValidator<util::DigitValidator>);
    static_assert(util::CharacterValidator<util::AnyCharValidator>);
    static_assert(util::CharacterValidator<util::AlphaCharValidator>);
    static_assert(util::CharacterValidator<util::AlphaNumValidator>);
    static_assert(util::CharacterValidator<util::PrintableCharValidator>);
}

} // namespace hm11::tests
