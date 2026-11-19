// test_fixed_string.cpp - Unit tests for ada::FixedString, ada::String and the character validators.
//
// Built in the same three modes as test_ranged_integer.cpp (see CMakeLists.txt). FixedString does not use
// ENABLE_RUNTIME_RANGE_CHECKS, so the saturate build runs the same tests as the default build.

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include "ada/fixed_string.hpp"

namespace {

// Types from the HM-11 Ada source.
using MacAddress = ada::FixedString<12, ada::HexDigit, struct MacAddressTag>;   // array (1 .. 12) of Hex_Character
using UUID = ada::FixedString<4, ada::HexDigit, struct UUIDTag>;                // array (1 .. 4) of Hex_Character
using HandleType = ada::FixedString<4, ada::HexDigit, struct HandleTypeTag>;    // array (1 .. 4) of Hex_Character
using IBeaconUUID = ada::FixedString<32, ada::HexDigit, struct IBeaconUUIDTag>; // array (1 .. 32) of Hex_Character
using PinType = ada::FixedString<6, ada::DecimalDigit, struct PinTypeTag>;      // array (1 .. 6) of Digit_Character
using WorkInterval =
    ada::FixedString<2, ada::DecimalDigit, struct WorkIntervalTag>; // array (1 .. 2) of Digit_Character
using InternalTemperature = ada::String<7>;                         // subtype ... is String (1 .. 7)

// Exact Ada mapping of `Hex_Character is new Character range '0' .. 'F'`.
using AdaHexCharacter = ada::CharRange<'0', 'F'>;

// ============================================================================
// Validators
// ============================================================================

static_assert(ada::HexDigit::is_valid('0') && ada::HexDigit::is_valid('9'));
static_assert(ada::HexDigit::is_valid('A') && ada::HexDigit::is_valid('F'));
static_assert(!ada::HexDigit::is_valid('a') && !ada::HexDigit::is_valid('G') && !ada::HexDigit::is_valid(':'));

static_assert(AdaHexCharacter::is_valid('0') && AdaHexCharacter::is_valid('F'));
static_assert(AdaHexCharacter::is_valid(':') && AdaHexCharacter::is_valid('@')); // inside Ada's '0' .. 'F'
static_assert(!AdaHexCharacter::is_valid('/') && !AdaHexCharacter::is_valid('G') && !AdaHexCharacter::is_valid('a'));

static_assert(ada::DecimalDigit::is_valid('0') && ada::DecimalDigit::is_valid('9'));
static_assert(!ada::DecimalDigit::is_valid('A') && !ada::DecimalDigit::is_valid('.'));

static_assert(ada::AnyChar::is_valid('.') && ada::AnyChar::is_valid('\0') && ada::AnyChar::is_valid('\xFF'));

// Ranges above 0x7F work regardless of char signedness.
using HighChars = ada::CharRange<'\x80', '\xFF'>;
static_assert(HighChars::is_valid('\xA0') && HighChars::is_valid('\xFF') && !HighChars::is_valid('A'));

// A validator's default character must itself be valid.
struct DefaultNotValid {
    static constexpr char DEFAULT_CHAR = 'x';
    static constexpr bool is_valid(char c) noexcept { return c >= '0' && c <= '9'; }
};
static_assert(ada::CharValidator<ada::HexDigit> && ada::CharValidator<ada::AnyChar>);
static_assert(ada::CharValidator<AdaHexCharacter>);
static_assert(!ada::CharValidator<DefaultNotValid>);

// ============================================================================
// Compile-time properties
// ============================================================================

static_assert(MacAddress::LENGTH == 12 && MacAddress::FIRST == 0 && MacAddress::LAST == 11);
static_assert(WorkInterval::FIRST == 0 && WorkInterval::LAST == 1);
static_assert(MacAddress::size() == 12);

static_assert(sizeof(MacAddress) == 13); // 12 characters plus '\0'
static_assert(std::is_trivially_copyable_v<MacAddress>);

// Tags keep Ada's distinct array types apart, even with equal length and validator.
template <typename A, typename B>
concept EqualityComparable = requires(A a, B b) { a == b; };
static_assert(!std::is_constructible_v<UUID, HandleType> && !std::is_assignable_v<UUID&, HandleType>);
static_assert(EqualityComparable<UUID, UUID> && !EqualityComparable<UUID, HandleType>);

// All Strings of one length are one type, like Ada subtypes of String.
static_assert(std::is_same_v<InternalTemperature, ada::String<7>>);
static_assert(!std::is_same_v<ada::String<7>, ada::String<8>>);

// Reference to a string literal: M - 1 characters plus the terminator.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
template <std::size_t M> using LiteralRef = const char (&)[M];

// Literals must have exactly N characters; construction is explicit; no implicit conversion to string_view.
static_assert(std::is_constructible_v<MacAddress, LiteralRef<13>>);
static_assert(!std::is_constructible_v<MacAddress, LiteralRef<12>>);
static_assert(!std::is_constructible_v<MacAddress, LiteralRef<14>>);
static_assert(!std::is_convertible_v<LiteralRef<13>, MacAddress>);
static_assert(!std::is_convertible_v<std::string_view, MacAddress>);
static_assert(!std::is_convertible_v<MacAddress, std::string_view>);

// Reads return char; writes go through the validating proxy.
static_assert(std::is_same_v<decltype(std::declval<const MacAddress&>()[0]), char>);
static_assert(std::is_same_v<decltype(std::declval<MacAddress&>()[0]), MacAddress::Reference>);

// Invalid characters, wrong lengths and bad indexes are rejected during constant evaluation in every build mode.
template <std::size_t M> struct Literal {
    std::array<char, M> chars{};
    constexpr Literal(LiteralRef<M> str) noexcept { // NOLINT(google-explicit-constructor)
        std::copy_n(str, M, chars.begin());
    }
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {chars.data(), M - 1}; }
};

template <typename S, Literal L>
concept ConstantFromText = requires { typename std::integral_constant<std::size_t, S{L.view()}.view().size()>; };

template <typename S, std::size_t I, char C> constexpr char constant_set() {
    S s{};
    s.set(I, C);
    return s[I];
}
template <typename S, std::size_t I, char C> constexpr char constant_proxy_write() {
    S s{};
    s[I] = C;
    return std::as_const(s)[I];
}
template <typename S, std::size_t I> constexpr char constant_read() {
    const S s{};
    return s[I];
}
template <typename S, std::size_t I, char C>
concept ConstantSettable = requires { typename std::integral_constant<char, constant_set<S, I, C>()>; };
template <typename S, std::size_t I, char C>
concept ConstantProxyWritable = requires { typename std::integral_constant<char, constant_proxy_write<S, I, C>()>; };
template <typename S, std::size_t I>
concept ConstantReadable = requires { typename std::integral_constant<char, constant_read<S, I>()>; };

static_assert(ConstantFromText<MacAddress, "B499CF123456">);
static_assert(!ConstantFromText<MacAddress, "B499CF12345Z">);
static_assert(!ConstantFromText<MacAddress, "b499cf123456">);
static_assert(!ConstantFromText<MacAddress, "B499CF12345">);   // too short: no read past the end
static_assert(!ConstantFromText<MacAddress, "B499CF1234567">); // too long
static_assert(ConstantFromText<InternalTemperature, "024.50C">);

static_assert(ConstantSettable<WorkInterval, WorkInterval::LAST, '7'>);
static_assert(!ConstantSettable<WorkInterval, WorkInterval::LAST, 'A'>);
static_assert(!ConstantSettable<WorkInterval, 2, '7'>);
static_assert(ConstantProxyWritable<WorkInterval, 0, '9'> && !ConstantProxyWritable<WorkInterval, 0, 'x'>);
static_assert(ConstantReadable<WorkInterval, 1> && !ConstantReadable<WorkInterval, 2>);

// ============================================================================
// Construction and access
// ============================================================================

TEST(AdaFixedStringTest, DefaultFillsWithValidatorDefault) {
    EXPECT_EQ(MacAddress{}.view(), "000000000000");
    EXPECT_EQ(InternalTemperature{}.view(), "       ");
    EXPECT_EQ((ada::FixedString<3, ada::CharRange<'A', 'Z'>, struct LettersTag>{}.view()), "AAA");
}

TEST(AdaFixedStringTest, ConstructsFromLiteral) {
    const MacAddress mac{"B499CF123456"};
    EXPECT_EQ(mac.view(), "B499CF123456");
    EXPECT_EQ(mac[MacAddress::FIRST], 'B');
    EXPECT_EQ(mac[MacAddress::LAST], '6');
}

TEST(AdaFixedStringTest, ConstructsFromRuntimeText) {
    const std::string text = "74278BDAB64445208F0C720EAF059935";
    const IBeaconUUID uuid{std::string_view{text}};
    EXPECT_EQ(uuid.view(), text);
}

TEST(AdaFixedStringTest, CStrIsNullTerminated) {
    const PinType pin{"123456"};
    EXPECT_STREQ(pin.c_str(), "123456");
}

TEST(AdaFixedStringTest, Iteration) {
    const PinType pin{"123456"};
    std::string collected;
    for (const char c : pin) {
        collected += c;
    }
    EXPECT_EQ(collected, "123456");
}

TEST(AdaFixedStringTest, ConstexprConstruction) {
    constexpr MacAddress mac{"B499CF123456"};
    static_assert(mac[MacAddress::LAST] == '6');
    static_assert(mac == "B499CF123456");
    EXPECT_EQ(mac.view(), "B499CF123456");
}

// ============================================================================
// Validated writes
// ============================================================================

TEST(AdaFixedStringTest, SetWritesValidCharacter) {
    WorkInterval interval{"00"};
    interval.set(WorkInterval::LAST, '7');
    EXPECT_EQ(interval.view(), "07");
}

TEST(AdaFixedStringTest, ProxyWritesValidCharacter) {
    // Ada: Interval (Interval'Last) := L;
    WorkInterval interval{"00"};
    interval[WorkInterval::LAST] = '5';
    EXPECT_EQ(interval.view(), "05");
}

TEST(AdaFixedStringTest, ProxyReadsCharacter) {
    WorkInterval interval{"08"};
    const char first = interval[WorkInterval::FIRST];
    EXPECT_EQ(first, '0');
    EXPECT_TRUE(interval[WorkInterval::FIRST] == '0'); // Ada: if Interval (Interval'First) = '0'
}

TEST(AdaFixedStringTest, ProxyToProxyAssignmentCopiesValue) {
    MacAddress a{"000000000000"};
    MacAddress b{"ABCDEF123456"};
    a[0] = b[5]; // copies 'F'; does not rebind a[0] to b
    a[1] = b[0];
    EXPECT_EQ(a.view(), "FA0000000000");
    EXPECT_EQ(b.view(), "ABCDEF123456");
}

// ============================================================================
// Comparison
// ============================================================================

TEST(AdaFixedStringTest, Equality) {
    const MacAddress a{"B499CF123456"};
    const MacAddress b{"B499CF123456"};
    const MacAddress c{"B499CF123457"};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
}

TEST(AdaFixedStringTest, EqualityWithTextRequiresSameLength) {
    const MacAddress mac{"B499CF123456"};
    EXPECT_TRUE(mac == "B499CF123456");
    EXPECT_TRUE(mac == std::string_view{"B499CF123456"});
    EXPECT_FALSE(mac == "B499CF123456EXTRA"); // not a prefix match
    EXPECT_FALSE(mac == "B499CF");
    EXPECT_TRUE(mac != "B499CF");
}

TEST(AdaFixedStringTest, Ordering) {
    const UUID low{"0001"};
    const UUID high{"FFFF"};
    EXPECT_TRUE(low < high);
    EXPECT_TRUE(high > low);
    EXPECT_EQ(low <=> UUID{"0001"}, std::strong_ordering::equal);
}

// Ada orders Character by position 0 .. 255, so bytes from 0x80 sort after 0x7F even where char is signed.
TEST(AdaFixedStringTest, OrderingUsesCharacterPosition) {
    EXPECT_TRUE(ada::String<1>{"\x7f"} < ada::String<1>{"\x80"});
    EXPECT_TRUE(ada::String<1>{"\xff"} > ada::String<1>{"\x01"});
    static_assert(ada::String<1>{"\x7f"} < ada::String<1>{"\x80"});
}

// ============================================================================
// Untrusted input
// ============================================================================

TEST(AdaFixedStringTest, IsValid) {
    EXPECT_TRUE(MacAddress::is_valid("B499CF123456"));
    EXPECT_FALSE(MacAddress::is_valid("b499cf123456"));
    EXPECT_FALSE(MacAddress::is_valid("B499CF12345"));
    EXPECT_FALSE(MacAddress::is_valid("B499CF1234567"));
    EXPECT_FALSE(WorkInterval::is_valid("1A"));
}

TEST(AdaFixedStringTest, TryMake) {
    const auto good = MacAddress::try_make("B499CF123456");
    ASSERT_TRUE(good.has_value());
    EXPECT_EQ(good, MacAddress{"B499CF123456"});

    EXPECT_FALSE(MacAddress::try_make("b499cf123456").has_value());
    EXPECT_FALSE(MacAddress::try_make("B499CF12345").has_value());
    EXPECT_FALSE(MacAddress::try_make("").has_value());
    EXPECT_TRUE(InternalTemperature::try_make("024.50C").has_value());
}

// ============================================================================
// Invalid input
// ============================================================================

#ifndef NDEBUG

TEST(AdaFixedStringDeathTest, InvalidCharacterInLiteral) {
    EXPECT_DEATH(
        {
            MacAddress m{"B499CF12345Z"};
            (void)m;
        },
        "Invalid character");
    EXPECT_DEATH(
        {
            MacAddress m{"b499cf123456"};
            (void)m;
        },
        "Invalid character");
}

TEST(AdaFixedStringDeathTest, WrongLengthText) {
    const std::string short_text = "B499";
    EXPECT_DEATH(
        {
            MacAddress m{std::string_view{short_text}};
            (void)m;
        },
        "String length does not match");
    const std::string long_text = "B499CF1234567";
    EXPECT_DEATH(
        {
            MacAddress m{std::string_view{long_text}};
            (void)m;
        },
        "String length does not match");
}

TEST(AdaFixedStringDeathTest, InvalidWrite) {
    EXPECT_DEATH(
        {
            WorkInterval w;
            w.set(0, 'A');
        },
        "Invalid character");
    EXPECT_DEATH(
        {
            WorkInterval w;
            w[0] = 'A';
        },
        "Invalid character");
}

TEST(AdaFixedStringDeathTest, IndexOutOfRange) {
    // Declared outside the macro so clang-tidy reads test/.clang-tidy for it with every GTest version
    const WorkInterval read_only;
    EXPECT_DEATH((void)read_only[2], "index out of range");
    EXPECT_DEATH(
        {
            WorkInterval w;
            w.set(2, '1');
        },
        "index out of range");
    EXPECT_DEATH(
        {
            WorkInterval w;
            w[2] = '1';
        },
        "index out of range");
}

#else

// Release builds skip the checks but never read past the input (verified under AddressSanitizer).
TEST(AdaFixedStringTest, ReleaseShortTextDoesNotOverread) {
    const std::array<char, 4> buffer{'B', '4', '9', '9'};
    const MacAddress mac{std::string_view{buffer.data(), buffer.size()}};
    EXPECT_EQ(mac.view(), "B49900000000");
}

#endif

} // namespace
