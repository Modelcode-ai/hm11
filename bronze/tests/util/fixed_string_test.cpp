/**
 * @file fixed_string_test.cpp
 * @brief Unit tests for FixedString template and validators
 *
 * This file contains unit tests for the FixedString template class and
 * the character validator classes.
 */

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

#include "hm11/util/fixed_string.hpp"

namespace hm11::util::test {

// Test type for FixedString instances
using TestHexString = FixedString<8, HexCharValidator>;
using TestDigitString = FixedString<6, DigitValidator>;
using TestAnyString = FixedString<10, AnyCharValidator>;

// Test validators
TEST(ValidatorsTest, HexCharValidator) {
    EXPECT_TRUE(HexCharValidator::is_valid('0'));
    EXPECT_TRUE(HexCharValidator::is_valid('9'));
    EXPECT_TRUE(HexCharValidator::is_valid('A'));
    EXPECT_TRUE(HexCharValidator::is_valid('F'));

    EXPECT_FALSE(HexCharValidator::is_valid('G'));
    EXPECT_FALSE(HexCharValidator::is_valid('a')); // Lowercase not allowed
    EXPECT_FALSE(HexCharValidator::is_valid('-'));
    EXPECT_FALSE(HexCharValidator::is_valid(' '));
}

TEST(ValidatorsTest, DigitValidator) {
    EXPECT_TRUE(DigitValidator::is_valid('0'));
    EXPECT_TRUE(DigitValidator::is_valid('9'));

    EXPECT_FALSE(DigitValidator::is_valid('A'));
    EXPECT_FALSE(DigitValidator::is_valid('a'));
    EXPECT_FALSE(DigitValidator::is_valid('-'));
    EXPECT_FALSE(DigitValidator::is_valid(' '));
}

TEST(ValidatorsTest, AnyCharValidator) {
    EXPECT_TRUE(AnyCharValidator::is_valid('0'));
    EXPECT_TRUE(AnyCharValidator::is_valid('A'));
    EXPECT_TRUE(AnyCharValidator::is_valid('a'));
    EXPECT_TRUE(AnyCharValidator::is_valid(' '));
    EXPECT_TRUE(AnyCharValidator::is_valid('-'));
    EXPECT_TRUE(AnyCharValidator::is_valid('\n'));
}

TEST(ValidatorsTest, AlphaCharValidator) {
    EXPECT_TRUE(AlphaCharValidator::is_valid('A'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('Z'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('a'));
    EXPECT_TRUE(AlphaCharValidator::is_valid('z'));

    EXPECT_FALSE(AlphaCharValidator::is_valid('0'));
    EXPECT_FALSE(AlphaCharValidator::is_valid('9'));
    EXPECT_FALSE(AlphaCharValidator::is_valid('-'));
    EXPECT_FALSE(AlphaCharValidator::is_valid(' '));
}

TEST(ValidatorsTest, AlphaNumValidator) {
    EXPECT_TRUE(AlphaNumValidator::is_valid('A'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('Z'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('a'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('z'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('0'));
    EXPECT_TRUE(AlphaNumValidator::is_valid('9'));

    EXPECT_FALSE(AlphaNumValidator::is_valid('-'));
    EXPECT_FALSE(AlphaNumValidator::is_valid(' '));
}

TEST(ValidatorsTest, PrintableCharValidator) {
    EXPECT_TRUE(PrintableCharValidator::is_valid('A'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('0'));
    EXPECT_TRUE(PrintableCharValidator::is_valid(' '));
    EXPECT_TRUE(PrintableCharValidator::is_valid('!'));
    EXPECT_TRUE(PrintableCharValidator::is_valid('~'));

    EXPECT_FALSE(PrintableCharValidator::is_valid('\n'));
    EXPECT_FALSE(PrintableCharValidator::is_valid('\0'));
}

// Test that values within range are accepted for HexCharValidator
TEST(FixedStringTest, HexValidConstruction) {
    // Test constructor with valid values
    TestHexString a("01234567");
    EXPECT_EQ("01234567", a.to_string());

    // Test default constructor (should initialize to zeros)
    TestHexString b;
    EXPECT_EQ("00000000", b.to_string());

    // Test constructor with initializer list
    TestHexString c({'0', '1', '2', '3', '4', '5', '6', '7'});
    EXPECT_EQ("01234567", c.to_string());

    // Test constructor with std::string
    std::string str = "ABCDEF01";
    TestHexString d(str);
    EXPECT_EQ("ABCDEF01", d.to_string());

    // Test constructor with std::string_view
    std::string_view sv = "12345678";
    TestHexString e(sv);
    EXPECT_EQ("12345678", e.to_string());

    // Test constructor with std::array
    std::array<char, 8> arr = {'A', 'B', 'C', 'D', 'E', 'F', '0', '1'};
    TestHexString f(arr);
    EXPECT_EQ("ABCDEF01", f.to_string());
}

// Test that values within range are accepted for DigitValidator
TEST(FixedStringTest, DigitValidConstruction) {
    // Test constructor with valid values
    TestDigitString a("012345");
    EXPECT_EQ("012345", a.to_string());

    // Test default constructor (should initialize to zeros)
    TestDigitString b;
    EXPECT_EQ("000000", b.to_string());

    // Test constructor with initializer list
    TestDigitString c({'0', '1', '2', '3', '4', '5'});
    EXPECT_EQ("012345", c.to_string());

    // Test constructor with std::string
    std::string str = "123456";
    TestDigitString d(str);
    EXPECT_EQ("123456", d.to_string());

    // Test constructor with std::string_view
    std::string_view sv = "789012";
    TestDigitString e(sv);
    EXPECT_EQ("789012", e.to_string());

    // Test constructor with std::array
    std::array<char, 6> arr = {'1', '2', '3', '4', '5', '6'};
    TestDigitString f(arr);
    EXPECT_EQ("123456", f.to_string());
}

// Test that invalid values are rejected or handled
TEST(FixedStringTest, InvalidValues) {
    // Using a non-hex character should reset to default in release builds
    // and trigger an assertion in debug builds
#ifdef NDEBUG
    TestHexString a("WXYZ1234");
    EXPECT_EQ("00000000", a.to_string()); // Default value
#endif

    // Test with partially invalid string
#ifdef NDEBUG
    TestHexString b("123G5678");
    EXPECT_EQ("00000000", b.to_string()); // Default value

    // Test with invalid std::string
    std::string str = "123g5678";
    TestHexString c(str);
    EXPECT_EQ("00000000", c.to_string()); // Default value
#endif
}

// Test assignment operations
TEST(FixedStringTest, Assignment) {
    // Test assignment operator with C-style string
    TestHexString a;
    a = "12345678";
    EXPECT_EQ("12345678", a.to_string());

    // Test assignment operator with std::string
    std::string str = "ABCDEF01";
    TestHexString b;
    b = str;
    EXPECT_EQ("ABCDEF01", b.to_string());

    // Test assignment operator with std::string_view
    std::string_view sv = "87654321";
    TestHexString c;
    c = sv;
    EXPECT_EQ("87654321", c.to_string());

    // Test copy assignment
    TestHexString d = a;
    EXPECT_EQ("12345678", d.to_string());

    // Test self-assignment
    d = d;
    EXPECT_EQ("12345678", d.to_string());
}

// Test comparison operators
TEST(FixedStringTest, Comparison) {
    TestHexString a("12345678");
    TestHexString b("12345678");
    TestHexString c("ABCDEF01");

    // Test equality operators
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a == "12345678");
    EXPECT_FALSE(a == "ABCDEF01");

    std::string str = "12345678";
    EXPECT_TRUE(a == str);

    std::string_view sv = "12345678";
    EXPECT_TRUE(a == sv);

    // Test inequality operators
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != "12345678");
    EXPECT_TRUE(a != "ABCDEF01");

    // Test less than operators
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(a < c); // '1' < 'A' in ASCII

    // Test less than or equal operators
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);

    // Test greater than operators
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a > c);

    // Test greater than or equal operators
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(a >= c);
}

// Test access operators
TEST(FixedStringTest, Access) {
    TestHexString a("12345678");

    // Test operator[]
    EXPECT_EQ('1', a[0]);
    EXPECT_EQ('8', a[7]);

    // Test at()
    EXPECT_EQ('1', a.at(0));
    EXPECT_EQ('8', a.at(7));

    // Test at() with out of bounds index
    EXPECT_THROW(a.at(8), std::out_of_range);

    // Test front() and back()
    EXPECT_EQ('1', a.front());
    EXPECT_EQ('8', a.back());

    // Test data() and c_str()
    EXPECT_STREQ("12345678", a.to_array().data());
    EXPECT_STREQ("12345678", a.c_str());

    // Test modify through operator[]
    a[0] = '0';
    EXPECT_EQ("02345678", a.to_string());

    // Test modify through at()
    a.at(7) = '9';
    EXPECT_EQ("02345679", a.to_string());
}

// Test iterator support
TEST(FixedStringTest, Iterators) {
    TestHexString a("12345678");

    // Test begin() and end()
    std::string result;
    for (auto it = a.begin(); it != a.end(); ++it) {
        result += *it;
    }
    EXPECT_EQ("12345678", result);

    // Test cbegin() and cend()
    result.clear();
    for (auto it = a.cbegin(); it != a.cend(); ++it) {
        result += *it;
    }
    EXPECT_EQ("12345678", result);

    // Test rbegin() and rend()
    result.clear();
    for (auto it = a.rbegin(); it != a.rend(); ++it) {
        result += *it;
    }
    EXPECT_EQ("87654321", result);

    // Test crbegin() and crend()
    result.clear();
    for (auto it = a.crbegin(); it != a.crend(); ++it) {
        result += *it;
    }
    EXPECT_EQ("87654321", result);

    // Test range-based for loop
    result.clear();
    for (char c : a) {
        result += c;
    }
    EXPECT_EQ("12345678", result);
}

// Test conversion methods
TEST(FixedStringTest, Conversion) {
    TestHexString a("12345678");

    // Test to_string()
    EXPECT_EQ("12345678", a.to_string());

    // Test conversion to std::string_view
    std::string_view sv = a;
    EXPECT_EQ("12345678", sv);

    // Test to_array()
    std::array<char, 8> arr = a.to_array();
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(a[i], arr[i]);
    }

    // Test substr()
    EXPECT_EQ("345", a.substr(2, 3));
    EXPECT_EQ("345678", a.substr(2));
    EXPECT_THROW(a.substr(10), std::out_of_range);
}

// Test size-related methods
TEST(FixedStringTest, Size) {
    TestHexString a;

    // Test size() and max_size()
    EXPECT_EQ(8, a.size());
    EXPECT_EQ(8, a.max_size());

    // Test empty()
    EXPECT_FALSE(a.empty());
}

// Test is_valid() method
TEST(FixedStringTest, Validation) {
    TestHexString a("12345678");
    EXPECT_TRUE(a.is_valid());

    TestHexString b;
    b[0] = 'G'; // Invalid hex character
    EXPECT_FALSE(b.is_valid());
}

// Test Ada type aliases
TEST(FixedStringTest, AdaTypeAliases) {
    // Test MacAddress
    MacAddress mac("123456789ABC");
    EXPECT_EQ("123456789ABC", mac.to_string());

    // Test UUID
    Uuid uuid("ABCD");
    EXPECT_EQ("ABCD", uuid.to_string());

    // Test PIN_Type
    PinType pin("123456");
    EXPECT_EQ("123456", pin.to_string());

    // Test iBeacon_UUID
    IBeaconUuid beacon_uuid("0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ("0123456789ABCDEF0123456789ABCDEF", beacon_uuid.to_string());

    // Test VersionType
    VersionType version("ABCD");
    EXPECT_EQ("ABCD", version.to_string());

    // Test MeasuredPower
    MeasuredPower power("C5");
    EXPECT_EQ("C5", power.to_string());

    // Test WorkInterval
    WorkInterval interval("99");
    EXPECT_EQ("99", interval.to_string());
}

// Test constexpr evaluation
TEST(FixedStringTest, ConstexprEvaluation) {
    // Test that various operations can be evaluated at compile time
    constexpr TestHexString a;
    constexpr bool is_empty = a.empty();
    constexpr std::size_t size = a.size();
    constexpr std::size_t max_size = a.max_size();

    EXPECT_FALSE(is_empty);
    EXPECT_EQ(8, size);
    EXPECT_EQ(8, max_size);

    // Test that validator_description() is constexpr
    constexpr std::string_view desc = TestHexString::validator_description();
    EXPECT_EQ("Hexadecimal character (0-9, A-F)", desc);
}

} // namespace hm11::util::test