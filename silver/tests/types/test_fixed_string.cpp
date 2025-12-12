#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include "hm11/types.hpp" // Include full type system

using namespace hm11;

class FixedStringTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(FixedStringTest, DefaultConstructor) {
    FixedString<10, AnyCharValidator> str;

    EXPECT_TRUE(str.empty());
    EXPECT_EQ(str.size(), 0);
    EXPECT_EQ(str.capacity(), 10);
    EXPECT_EQ(str.remaining_capacity(), 10);
}

TEST_F(FixedStringTest, CStringConstructor) {
    FixedString<20, AnyCharValidator> str("Hello World");

    EXPECT_FALSE(str.empty());
    EXPECT_EQ(str.size(), 11);
    EXPECT_EQ(std::string_view(str), "Hello World");
    EXPECT_STREQ(str.c_str(), "Hello World");
}

TEST_F(FixedStringTest, CStringConstructorEmpty) {
    FixedString<10, AnyCharValidator> str("");

    EXPECT_TRUE(str.empty());
    EXPECT_EQ(str.size(), 0);
    EXPECT_STREQ(str.c_str(), "");
}

TEST_F(FixedStringTest, CStringConstructorNull) {
    FixedString<10, AnyCharValidator> str(nullptr);

    EXPECT_TRUE(str.empty());
    EXPECT_EQ(str.size(), 0);
}

TEST_F(FixedStringTest, CStringConstructorTooLong) {
    bool threw_exception = false;
    try {
        FixedString<5, AnyCharValidator> str("Hello World");
    } catch (const std::length_error&) {
        threw_exception = true;
    }
    EXPECT_TRUE(threw_exception);
}

TEST_F(FixedStringTest, StringViewConstructor) {
    std::string_view sv = "Test String";
    FixedString<20, AnyCharValidator> str(sv);

    EXPECT_EQ(str.size(), 11);
    EXPECT_EQ(std::string_view(str), sv);
}

TEST_F(FixedStringTest, ArrayConstructor) {
    std::array<char, 4> arr = {'T', 'e', 's', 't'};
    FixedString<10, AnyCharValidator> str(arr);

    EXPECT_EQ(str.size(), 4);
    EXPECT_EQ(std::string_view(str), "Test");
}

// ============================================================================
// Character Validation Tests
// ============================================================================

TEST_F(FixedStringTest, HexValidationValid) {
    FixedString<12, HexCharValidator> mac("AB12CD34EF56");

    EXPECT_EQ(mac.size(), 12);
    EXPECT_EQ(std::string_view(mac), "AB12CD34EF56");
    EXPECT_TRUE(mac.is_valid());
}

TEST_F(FixedStringTest, HexValidationInvalid) {
    bool threw_exception = false;
    try {
        FixedString<12, HexCharValidator> str("AB12GH34EF56");
    } catch (const std::invalid_argument&) {
        threw_exception = true;
    }
    EXPECT_TRUE(threw_exception);
}

TEST_F(FixedStringTest, DigitValidationValid) {
    FixedString<6, DigitCharValidator> pin("123456");

    EXPECT_EQ(pin.size(), 6);
    EXPECT_EQ(std::string_view(pin), "123456");
    EXPECT_TRUE(pin.is_valid());
}

TEST_F(FixedStringTest, DigitValidationInvalid) {
    bool threw_exception = false;
    try {
        FixedString<6, DigitCharValidator> str("12A456");
    } catch (const std::invalid_argument&) {
        threw_exception = true;
    }
    EXPECT_TRUE(threw_exception);
}

TEST_F(FixedStringTest, ValidatorName) {
    const char* hex_name = FixedString<10, HexCharValidator>::validator_name();
    const char* digit_name = FixedString<10, DigitCharValidator>::validator_name();
    const char* any_name = FixedString<10, AnyCharValidator>::validator_name();

    EXPECT_STREQ(hex_name, "HexChar");
    EXPECT_STREQ(digit_name, "DigitChar");
    EXPECT_STREQ(any_name, "AnyChar");
}

// ============================================================================
// Element Access Tests
// ============================================================================

TEST_F(FixedStringTest, ElementAccess) {
    FixedString<10, AnyCharValidator> str("Hello");

    EXPECT_EQ(str[0], 'H');
    EXPECT_EQ(str[4], 'o');
    EXPECT_EQ(str.at(0), 'H');
    EXPECT_EQ(str.at(4), 'o');

    EXPECT_EQ(str.front(), 'H');
    EXPECT_EQ(str.back(), 'o');
}

TEST_F(FixedStringTest, ElementAccessOutOfBounds) {
    FixedString<10, AnyCharValidator> str("Test");

    EXPECT_THROW(str.at(4), std::out_of_range);
    EXPECT_THROW(str.at(10), std::out_of_range);
}

TEST_F(FixedStringTest, ElementAccessEmpty) {
    FixedString<10, AnyCharValidator> str;

    EXPECT_THROW(str.front(), std::out_of_range);
    EXPECT_THROW(str.back(), std::out_of_range);
}

TEST_F(FixedStringTest, ElementModification) {
    FixedString<10, AnyCharValidator> str("Hello");

    str[0] = 'J';
    str.at(1) = 'a';

    EXPECT_EQ(std::string_view(str), "Jallo");
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(FixedStringTest, Iterators) {
    FixedString<10, AnyCharValidator> str("Test");

    std::string result;
    for (char it : str) {
        result += it;
    }
    EXPECT_EQ(result, "Test");

    // Range-based for loop
    std::string result2;
    for (char c : str) {
        result2 += c;
    }
    EXPECT_EQ(result2, "Test");
}

TEST_F(FixedStringTest, ConstIterators) {
    const FixedString<10, AnyCharValidator> STR("Test");

    std::string result;
    for (char it : STR) {
        result += it;
    }
    EXPECT_EQ(result, "Test");
}

// ============================================================================
// Capacity Tests
// ============================================================================

TEST_F(FixedStringTest, CapacityMethods) {
    FixedString<20, AnyCharValidator> str("Hello");

    EXPECT_FALSE(str.empty());
    EXPECT_EQ(str.size(), 5);
    EXPECT_EQ(str.length(), 5);
    EXPECT_EQ(str.capacity(), 20);
    EXPECT_EQ(str.remaining_capacity(), 15);
    EXPECT_FALSE(str.full());

    FixedString<5, AnyCharValidator> full_str("12345");
    EXPECT_TRUE(full_str.full());
    EXPECT_EQ(full_str.remaining_capacity(), 0);
}

// ============================================================================
// Modification Tests
// ============================================================================

TEST_F(FixedStringTest, Clear) {
    FixedString<10, AnyCharValidator> str("Hello");

    EXPECT_FALSE(str.empty());
    str.clear();
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(str.size(), 0);
}

TEST_F(FixedStringTest, PushBack) {
    FixedString<10, HexCharValidator> str;

    str.push_back('A');
    str.push_back('B');
    str.push_back('1');
    str.push_back('2');

    EXPECT_EQ(std::string_view(str), "AB12");
    EXPECT_EQ(str.size(), 4);
}

TEST_F(FixedStringTest, PushBackValidation) {
    FixedString<10, HexCharValidator> str;

    EXPECT_NO_THROW(str.push_back('A'));
    EXPECT_NO_THROW(str.push_back('1'));
    EXPECT_THROW(str.push_back('G'), std::invalid_argument);
}

TEST_F(FixedStringTest, PushBackCapacityExceeded) {
    FixedString<2, AnyCharValidator> str("AB");

    EXPECT_THROW(str.push_back('C'), std::length_error);
}

TEST_F(FixedStringTest, PopBack) {
    FixedString<10, AnyCharValidator> str("Hello");

    EXPECT_EQ(str.size(), 5);
    str.pop_back();
    EXPECT_EQ(str.size(), 4);
    EXPECT_EQ(std::string_view(str), "Hell");

    str.pop_back();
    str.pop_back();
    str.pop_back();
    str.pop_back();
    EXPECT_TRUE(str.empty());

    EXPECT_THROW(str.pop_back(), std::out_of_range);
}

TEST_F(FixedStringTest, AppendFixedString) {
    FixedString<20, HexCharValidator> str1("AB12");
    FixedString<10, HexCharValidator> str2("CD34");

    str1.append(str2);
    EXPECT_EQ(std::string_view(str1), "AB12CD34");
    EXPECT_EQ(str1.size(), 8);
}

TEST_F(FixedStringTest, AppendStringView) {
    FixedString<20, HexCharValidator> str("AB12");

    str.append(std::string_view("CD34"));
    EXPECT_EQ(std::string_view(str), "AB12CD34");
    EXPECT_EQ(str.size(), 8);
}

TEST_F(FixedStringTest, AppendCapacityExceeded) {
    FixedString<10, AnyCharValidator> str("Hello");

    EXPECT_THROW(str.append(std::string_view("World!")), std::length_error);
}

TEST_F(FixedStringTest, AppendValidationFailure) {
    FixedString<10, HexCharValidator> str("AB12");

    EXPECT_THROW(str.append(std::string_view("GH34")), std::invalid_argument);
}

TEST_F(FixedStringTest, Resize) {
    FixedString<10, AnyCharValidator> str("Hello");

    // Shrink
    str.resize(3);
    EXPECT_EQ(std::string_view(str), "Hel");
    EXPECT_EQ(str.size(), 3);

    // Expand with default fill
    str.resize(5, 'X');
    EXPECT_EQ(str.size(), 5);
    EXPECT_EQ(str[3], 'X');
    EXPECT_EQ(str[4], 'X');

    // Expand beyond capacity
    EXPECT_THROW(str.resize(15), std::length_error);
}

// ============================================================================
// String Operations Tests
// ============================================================================

TEST_F(FixedStringTest, Substr) {
    FixedString<20, AnyCharValidator> str("Hello World");

    auto sub1 = str.substr(0, 5);
    EXPECT_EQ(sub1, "Hello");

    auto sub2 = str.substr(6);
    EXPECT_EQ(sub2, "World");

    auto sub3 = str.substr(6, 3);
    EXPECT_EQ(sub3, "Wor");

    EXPECT_THROW(str.substr(15), std::out_of_range);
}

TEST_F(FixedStringTest, Find) {
    FixedString<20, AnyCharValidator> str("Hello World");

    EXPECT_EQ(str.find('H'), 0);
    EXPECT_EQ(str.find('o'), 4);
    EXPECT_EQ(str.find('W'), 6);
    EXPECT_EQ(str.find('x'), std::string_view::npos);
    EXPECT_EQ(str.find('o', 5), 7); // Find second 'o'
}

TEST_F(FixedStringTest, StartsWith) {
    FixedString<20, AnyCharValidator> str("Hello World");

    EXPECT_TRUE(str.starts_with("Hello"));
    EXPECT_TRUE(str.starts_with("H"));
    EXPECT_TRUE(str.starts_with(""));
    EXPECT_FALSE(str.starts_with("World"));
    EXPECT_FALSE(str.starts_with("Hello World Extended"));
}

TEST_F(FixedStringTest, EndsWith) {
    FixedString<20, AnyCharValidator> str("Hello World");

    EXPECT_TRUE(str.ends_with("World"));
    EXPECT_TRUE(str.ends_with("d"));
    EXPECT_TRUE(str.ends_with(""));
    EXPECT_FALSE(str.ends_with("Hello"));
    EXPECT_FALSE(str.ends_with("Extended Hello World"));
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(FixedStringTest, EqualityComparison) {
    FixedString<10, AnyCharValidator> str1("Hello");
    FixedString<10, AnyCharValidator> str2("Hello");
    FixedString<10, AnyCharValidator> str3("World");

    EXPECT_TRUE(str1 == str2);
    EXPECT_FALSE(str1 == str3);
    EXPECT_FALSE(str1 != str2);
    EXPECT_TRUE(str1 != str3);
}

TEST_F(FixedStringTest, OrderingComparison) {
    FixedString<10, AnyCharValidator> str1("Apple");
    FixedString<10, AnyCharValidator> str2("Banana");

    EXPECT_TRUE(str1 < str2);
    EXPECT_TRUE(str1 <= str2);
    EXPECT_FALSE(str1 > str2);
    EXPECT_FALSE(str1 >= str2);

    EXPECT_FALSE(str2 < str1);
    EXPECT_FALSE(str2 <= str1);
    EXPECT_TRUE(str2 > str1);
    EXPECT_TRUE(str2 >= str1);
}

TEST_F(FixedStringTest, StringViewComparison) {
    FixedString<10, AnyCharValidator> str("Hello");
    std::string_view sv("Hello");
    std::string_view sv2("World");

    EXPECT_TRUE(str == sv);
    EXPECT_FALSE(str == sv2);
    EXPECT_TRUE(sv == str);
    EXPECT_FALSE(sv2 == str);

    EXPECT_TRUE(str < std::string_view("World"));
    EXPECT_TRUE(std::string_view("Apple") < str);
}

// ============================================================================
// Assignment Tests
// ============================================================================

TEST_F(FixedStringTest, CopyAssignment) {
    FixedString<10, AnyCharValidator> str1("Hello");
    FixedString<10, AnyCharValidator> str2("World");

    str2 = str1;
    EXPECT_EQ(std::string_view(str2), "Hello");
    EXPECT_EQ(str1, str2);
}

TEST_F(FixedStringTest, CStringAssignment) {
    FixedString<20, AnyCharValidator> str;

    str = "Hello World";
    EXPECT_EQ(std::string_view(str), "Hello World");
    EXPECT_EQ(str.size(), 11);
}

TEST_F(FixedStringTest, StringViewAssignment) {
    FixedString<20, AnyCharValidator> str;
    std::string_view sv("Test String");

    str = sv;
    EXPECT_EQ(std::string_view(str), sv);
    EXPECT_EQ(str.size(), sv.size());
}

// ============================================================================
// Data Access Tests
// ============================================================================

TEST_F(FixedStringTest, DataAccess) {
    FixedString<10, AnyCharValidator> str("Hello");

    const char* data_ptr = str.data();
    EXPECT_NE(data_ptr, nullptr);
    EXPECT_EQ(std::string_view(data_ptr, str.size()), "Hello");

    // Modify through data pointer
    char* mutable_data = str.data();
    mutable_data[0] = 'J';
    EXPECT_EQ(std::string_view(str), "Jello");
}

TEST_F(FixedStringTest, CStringConversion) {
    FixedString<10, AnyCharValidator> str("Hello");

    const char* c_str = str.c_str();
    EXPECT_STREQ(c_str, "Hello");

    // Test with full capacity
    FixedString<5, AnyCharValidator> full_str("12345");
    const char* full_c_str = full_str.c_str();
    EXPECT_STREQ(full_c_str, "12345");
}

TEST_F(FixedStringTest, StringViewConversion) {
    FixedString<10, AnyCharValidator> str("Hello");

    std::string_view sv = str;
    EXPECT_EQ(sv, "Hello");
    EXPECT_EQ(sv.size(), 5);

    // Implicit conversion in function call
    auto check_sv = [](std::string_view view) {
        return view == "Hello";
    };
    EXPECT_TRUE(check_sv(str));
}

// ============================================================================
// Concatenation Tests
// ============================================================================

TEST_F(FixedStringTest, Concatenation) {
    FixedString<5, AnyCharValidator> str1("Hello");
    FixedString<5, AnyCharValidator> str2("World");

    auto result = str1 + str2;
    EXPECT_EQ(std::string_view(result), "HelloWorld");
    EXPECT_EQ(result.capacity(), 10);
}

// ============================================================================
// Integration Tests with HM11 Types
// ============================================================================

TEST_F(FixedStringTest, HM11MacAddress) {
    MacAddress mac("AB12CD34EF56");

    EXPECT_EQ(mac.size(), 12);
    EXPECT_EQ(mac.capacity(), 12);
    EXPECT_EQ(std::string_view(mac), "AB12CD34EF56");
    EXPECT_TRUE(mac.is_valid());

    // Invalid MAC address
    bool threw_mac_exception = false;
    try {
        MacAddress invalid_mac("GH12CD34EF56");
    } catch (const std::invalid_argument&) {
        threw_mac_exception = true;
    }
    EXPECT_TRUE(threw_mac_exception);
}

TEST_F(FixedStringTest, HM11PinType) {
    PinType pin("123456");

    EXPECT_EQ(pin.size(), 6);
    EXPECT_EQ(pin.capacity(), 6);
    EXPECT_EQ(std::string_view(pin), "123456");
    EXPECT_TRUE(pin.is_valid());

    // Invalid PIN (contains letter)
    bool threw_pin_exception = false;
    try {
        PinType invalid_pin("12A456");
    } catch (const std::invalid_argument&) {
        threw_pin_exception = true;
    }
    EXPECT_TRUE(threw_pin_exception);
}

TEST_F(FixedStringTest, HM11UUID) {
    UUID uuid("FE01");

    EXPECT_EQ(uuid.size(), 4);
    EXPECT_EQ(uuid.capacity(), 4);
    EXPECT_EQ(std::string_view(uuid), "FE01");
    EXPECT_TRUE(uuid.is_valid());
}

// ============================================================================
// Performance and Memory Layout Tests
// ============================================================================

TEST_F(FixedStringTest, MemoryLayout) {
    // Ensure reasonable memory usage
    FixedString<10, AnyCharValidator> str;

    // Should not be much larger than the capacity plus metadata
    // On 64-bit systems: 10 bytes (array) + 8 bytes (size_t) + 6 bytes (padding) = 24 bytes
    EXPECT_LE(sizeof(str), 32); // Allow for reasonable overhead on different platforms

    // Should be trivially copyable for performance
    static_assert(std::is_trivially_copyable_v<FixedString<10, AnyCharValidator>>);
}

TEST_F(FixedStringTest, ConstexprSupport) {
    // Test compile-time evaluation where possible
    constexpr auto CAPACITY = FixedString<10, HexCharValidator>::MAX_SIZE;
    static_assert(CAPACITY == 10);

    EXPECT_EQ(CAPACITY, 10);
}

// Tests will be run by gtest_main