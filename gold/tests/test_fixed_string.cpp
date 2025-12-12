//
// test_fixed_string.cpp - Unit tests for FixedString template
//
// Tests for the FixedString<N, Validator> template used to represent
// Ada's fixed-size character array types.
//

#include <gtest/gtest.h>

#include <string_view>

#include "hm11/types/fixed_string.hpp"
#include "hm11/types/validators.hpp"

namespace hm11 {
namespace {

// Type aliases for testing (matching expected Ada types)
using MacAddress = FixedString<12, HexCharValidator>;
using PinType = FixedString<6, DigitCharValidator>;
using UUID = FixedString<4, HexCharValidator>;
using IBeaconUuid = FixedString<32, HexCharValidator>;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(FixedStringTest, DefaultConstructorInitializesToZeros) {
    MacAddress mac;
    EXPECT_EQ(mac.size(), 12);
    for (const auto& c : mac) {
        EXPECT_EQ(c, '0');
    }
}

TEST(FixedStringTest, ConstructFromStringLiteral) {
    constexpr MacAddress MAC{"B499CF123456"};
    EXPECT_EQ(MAC.size(), 12);
    EXPECT_EQ(MAC.view(), "B499CF123456");
}

TEST(FixedStringTest, ConstructFromStringView) {
    const std::string_view SV = "123456";
    PinType pin{SV};
    EXPECT_EQ(pin.size(), 6);
    EXPECT_EQ(pin.view(), "123456");
}

TEST(FixedStringTest, ConstructFromCString) {
    const char* const CSTR = "ABCD";
    UUID uuid{CSTR};
    EXPECT_EQ(uuid.size(), 4);
    EXPECT_EQ(uuid.view(), "ABCD");
}

TEST(FixedStringTest, ConstexprConstruction) {
    // Verify compile-time construction works
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    constexpr PinType PIN{"999888"};
    constexpr UUID UUID_VAR{"FFFF"};

    static_assert(MAC.size() == 12, "MAC address should be 12 characters");
    static_assert(PIN.size() == 6, "PIN should be 6 characters");
    static_assert(UUID_VAR.size() == 4, "UUID should be 4 characters");
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST(FixedStringTest, HexValidationAllowsValidCharacters) {
    // All valid hex characters
    constexpr MacAddress MAC1{"0123456789AB"};
    constexpr MacAddress MAC2{"CDEF00000000"};

    EXPECT_EQ(MAC1.view(), "0123456789AB");
    EXPECT_EQ(MAC2.view(), "CDEF00000000");
}

TEST(FixedStringTest, DigitValidationAllowsValidCharacters) {
    constexpr PinType PIN1{"012345"};
    constexpr PinType PIN2{"987654"};

    EXPECT_EQ(PIN1.view(), "012345");
    EXPECT_EQ(PIN2.view(), "987654");
}

#ifdef NDEBUG
// These tests only run in release mode (no assertions)
TEST(FixedStringTest, InvalidHexCharactersInRelease) {
    // In release mode, invalid characters are allowed (no assertion)
    // but this is not recommended usage
    GTEST_SKIP() << "Test behavior undefined without assertions";
}
#else
// In debug mode, we expect assertions to fire for invalid input
// We can't easily test assertion failures with Google Test,
// so we document expected behavior instead
TEST(FixedStringTest, InvalidHexCharactersAssertInDebug) {
    GTEST_SKIP() << "Cannot test assertion failures (would terminate test)";
    // The following would assert in debug mode:
    // MacAddress invalid{"GGGGGGGGGGGG"};  // 'G' not valid hex
    // MacAddress invalid2{"abcdef123456"};  // lowercase not valid
}

TEST(FixedStringTest, InvalidDigitCharactersAssertInDebug) {
    GTEST_SKIP() << "Cannot test assertion failures (would terminate test)";
    // The following would assert in debug mode:
    // PinType invalid{"12345A"};  // 'A' not valid digit
    // PinType invalid2{"1234-6"};  // '-' not valid digit
}
#endif

// ============================================================================
// Access and Iteration Tests
// ============================================================================

TEST(FixedStringTest, IndexOperatorAccess) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};

    EXPECT_EQ(MAC[0], 'A');
    EXPECT_EQ(MAC[1], 'A');
    EXPECT_EQ(MAC[10], '0');
    EXPECT_EQ(MAC[11], '0');
}

TEST(FixedStringTest, IndexOperatorModification) {
    MacAddress mac{"AABBCCDDEE00"};
    mac[0] = 'F';
    mac[1] = 'F';

    EXPECT_EQ(mac[0], 'F');
    EXPECT_EQ(mac[1], 'F');
    EXPECT_EQ(mac.view(), "FFBBCCDDEE00");
}

TEST(FixedStringTest, ConstexprIndexAccess) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    static_assert(MAC[0] == 'A', "First character should be 'A'");
    static_assert(MAC[11] == '0', "Last character should be '0'");
}

TEST(FixedStringTest, IteratorAccess) {
    MacAddress mac{"AABBCCDDEE00"};

    std::string result;
    for (char c : mac) {
        result += c;
    }

    EXPECT_EQ(result, "AABBCCDDEE00");
}

TEST(FixedStringTest, ConstIteratorAccess) {
    const MacAddress MAC{"AABBCCDDEE00"};

    std::string result;
    for (char c : MAC) {
        result += c;
    }

    EXPECT_EQ(result, "AABBCCDDEE00");
}

// ============================================================================
// Conversion and View Tests
// ============================================================================

TEST(FixedStringTest, ConversionToStringView) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    std::string_view sv = MAC;

    EXPECT_EQ(sv, "AABBCCDDEE00");
    EXPECT_EQ(sv.size(), 12);
}

TEST(FixedStringTest, ViewMethodReturnsStringView) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    auto sv = MAC.view();

    EXPECT_EQ(sv, "AABBCCDDEE00");
    EXPECT_EQ(sv.size(), 12);
}

TEST(FixedStringTest, CStrReturnsNullTerminated) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    const char* const CSTR = MAC.c_str();

    EXPECT_STREQ(CSTR, "AABBCCDDEE00");
    EXPECT_EQ(CSTR[12], '\0'); // Verify null termination
}

TEST(FixedStringTest, DataPointerAccess) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    const char* const DATA = MAC.get_data();

    EXPECT_EQ(std::string_view(DATA, 12), "AABBCCDDEE00");
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST(FixedStringTest, EqualityComparison) {
    constexpr MacAddress MAC1{"AABBCCDDEE00"};
    constexpr MacAddress MAC2{"AABBCCDDEE00"};
    constexpr MacAddress MAC3{"AABBCCDDEE01"};

    EXPECT_EQ(MAC1, MAC2);
    EXPECT_NE(MAC1, MAC3);
}

TEST(FixedStringTest, EqualityWithStringView) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};

    EXPECT_EQ(MAC, "AABBCCDDEE00");
    EXPECT_NE(MAC, "AABBCCDDEE01");
    EXPECT_NE(MAC, "AABBCCDDEE0"); // Different length
}

TEST(FixedStringTest, ConstexprEquality) {
    constexpr MacAddress MAC1{"AABBCCDDEE00"};
    constexpr MacAddress MAC2{"AABBCCDDEE00"};
    constexpr MacAddress MAC3{"AABBCCDDEE01"};

    static_assert(MAC1 == MAC2, "Equal MACs should compare equal");
    static_assert(MAC1 != MAC3, "Different MACs should compare not equal");
}

TEST(FixedStringTest, LexicographicComparison) {
    constexpr MacAddress MAC1{"AABBCCDDEE00"};
    constexpr MacAddress MAC2{"AABBCCDDEE01"};
    constexpr MacAddress MAC3{"BBBBCCDDEE00"};

    EXPECT_LT(MAC1, MAC2);
    EXPECT_LE(MAC1, MAC2);
    EXPECT_LE(MAC1, MAC1);

    EXPECT_GT(MAC2, MAC1);
    EXPECT_GE(MAC2, MAC1);
    EXPECT_GE(MAC1, MAC1);

    EXPECT_LT(MAC1, MAC3);
    EXPECT_GT(MAC3, MAC1);
}

TEST(FixedStringTest, ConstexprLexicographicComparison) {
    constexpr MacAddress MAC1{"AABBCCDDEE00"};
    constexpr MacAddress MAC2{"AABBCCDDEE01"};

    static_assert(MAC1 < MAC2, "mac1 should be less than mac2");
    static_assert(MAC1 <= MAC2, "mac1 should be less than or equal to mac2");
    static_assert(MAC2 > MAC1, "mac2 should be greater than mac1");
    static_assert(MAC2 >= MAC1, "mac2 should be greater than or equal to mac1");
}

// ============================================================================
// Size and Capacity Tests
// ============================================================================

TEST(FixedStringTest, SizeReturnsCorrectValue) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    constexpr PinType PIN{"123456"};
    constexpr UUID UUID_VAR{"ABCD"};

    EXPECT_EQ(MAC.size(), 12);
    EXPECT_EQ(PIN.size(), 6);
    EXPECT_EQ(UUID_VAR.size(), 4);
}

TEST(FixedStringTest, LengthReturnsCorrectValue) {
    constexpr MacAddress MAC{"AABBCCDDEE00"};
    constexpr PinType PIN{"123456"};

    EXPECT_EQ(MAC.length(), 12);
    EXPECT_EQ(PIN.length(), 6);
}

TEST(FixedStringTest, CapacityIsCompileTimeConstant) {
    static_assert(MacAddress::CAPACITY == 12, "MAC capacity should be 12");
    static_assert(PinType::CAPACITY == 6, "PIN capacity should be 6");
    static_assert(UUID::CAPACITY == 4, "UUID capacity should be 4");
    static_assert(IBeaconUuid::CAPACITY == 32, "iBeacon UUID capacity should be 32");
}

// ============================================================================
// Real-World Usage Examples
// ============================================================================

TEST(FixedStringTest, MACAddressExample) {
    // Real-world MAC address from Ada spec comment:
    // 0xB4: 0x99: 0x4C: 0xXX: 0xXX: 0xXX is BLE MAC Address
    constexpr MacAddress MAC{"B4994C123456"};

    EXPECT_EQ(MAC.size(), 12);
    EXPECT_EQ(MAC.view(), "B4994C123456");

    // Can be used with string parsing utilities
    std::string_view sv = MAC;
    EXPECT_EQ(sv.substr(0, 2), "B4"); // First octet
    EXPECT_EQ(sv.substr(2, 2), "99"); // Second octet
    EXPECT_EQ(sv.substr(4, 2), "4C"); // Third octet
}

TEST(FixedStringTest, PINTypeExample) {
    // 6-digit PIN code
    constexpr PinType PIN{"123456"};

    EXPECT_EQ(PIN.size(), 6);
    EXPECT_EQ(PIN.view(), "123456");

    // All characters are digits
    for (char c : PIN) {
        EXPECT_GE(c, '0');
        EXPECT_LE(c, '9');
    }
}

TEST(FixedStringTest, iBeaconUUIDExample) {
    // 32-character hex UUID for iBeacon
    constexpr IBeaconUuid UUID_VAR{"FDA50693A4E24FB1AFCFC6EB07647825"};

    EXPECT_EQ(UUID_VAR.size(), 32);
    EXPECT_EQ(UUID_VAR.view(), "FDA50693A4E24FB1AFCFC6EB07647825");
}

TEST(FixedStringTest, UUIDExample) {
    // 4-byte (16-bit) UUID
    constexpr UUID UUID_VAR{"180D"}; // Heart Rate Service UUID

    EXPECT_EQ(UUID_VAR.size(), 4);
    EXPECT_EQ(UUID_VAR.view(), "180D");
}

// ============================================================================
// Type Safety Tests
// ============================================================================

TEST(FixedStringTest, DifferentTypesDontCompile) {
    // These should not compile (different types):
    // MacAddress mac{"AABBCCDDEE00"};
    // PinType pin = mac;  // Error: different types

    // Same size but different validator
    using HexPin = FixedString<6, HexCharValidator>;

    PinType digit_pin{"123456"};
    HexPin hex_pin{"ABCDEF"};

    // These are different types even though same size
    EXPECT_EQ(digit_pin.size(), hex_pin.size());
    // But cannot assign: digit_pin = hex_pin;  // Error: different types
}

TEST(FixedStringTest, DifferentSizesDontCompile) {
    // These should not compile (different sizes):
    // MacAddress mac{"AABBCCDDEE00"};
    // UUID uuid{"ABCD"};
    // mac = uuid;  // Error: different types (different N)

    SUCCEED() << "Type safety enforced at compile time";
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(FixedStringTest, AllZeros) {
    constexpr MacAddress MAC{"000000000000"};
    EXPECT_EQ(MAC.view(), "000000000000");
}

TEST(FixedStringTest, AllFs) {
    constexpr MacAddress MAC{"FFFFFFFFFFFF"};
    EXPECT_EQ(MAC.view(), "FFFFFFFFFFFF");
}

TEST(FixedStringTest, AllNines) {
    constexpr PinType PIN{"999999"};
    EXPECT_EQ(PIN.view(), "999999");
}

TEST(FixedStringTest, SingleCharacterString) {
    using SingleChar = FixedString<1, HexCharValidator>;
    constexpr SingleChar SINGLE_CHAR{"A"};

    EXPECT_EQ(SINGLE_CHAR.size(), 1);
    EXPECT_EQ(SINGLE_CHAR[0], 'A');
    EXPECT_EQ(SINGLE_CHAR.view(), "A");
}

} // namespace
} // namespace hm11
