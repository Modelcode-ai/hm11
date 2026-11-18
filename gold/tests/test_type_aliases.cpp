/**
 * @file test_type_aliases.cpp
 * @brief Checks each alias in hm11/types/type_aliases.hpp against its declaration in the Ada source (hm11.ads)
 *
 * tests/ada tests the ada:: templates; this file tests the project's instantiations of them:
 * - integer ranges, and whether each is a new type (Ada `type ... is range`) or a subtype of Natural/Positive
 * - string lengths and character sets
 * - that look-alike types cannot be mixed, while Ada subtypes of the same base convert as in Ada
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>

#include "hm11/types/type_aliases.hpp"

using namespace hm11::types;

namespace {

// True if T{V} is a constant expression, i.e. V is in range (out-of-range constants do not compile).
template <typename T, auto V>
concept ConstantInRange = requires { typename std::integral_constant<int, static_cast<int>(T{V}.get())>; };

template <typename T, auto Min, auto Max>
constexpr bool HAS_RANGE = T::MIN_VALUE == Min && T::MAX_VALUE == Max && ConstantInRange<T, Min> &&
                           ConstantInRange<T, Max> && !ConstantInRange<T, Min - 1> && !ConstantInRange<T, Max + 1>;

template <typename A, typename B>
constexpr bool MIXABLE = std::is_convertible_v<A, B> || std::is_convertible_v<B, A> ||
                         std::is_constructible_v<A, B> || std::is_constructible_v<B, A>;

template <typename T, typename Validator, std::size_t N>
constexpr bool IS_STRING = std::is_same_v<typename T::ValidatorType, Validator> && T::LENGTH == N;

constexpr std::string_view HEX_32 = "0123456789ABCDEF0123456789ABCDEF";
constexpr std::string_view DIGITS_7 = "0123456";

} // namespace

// =============================================================================
// Ada `type X is range Min .. Max` -> ada::RangedInteger (a new type)
// =============================================================================

static_assert(HAS_RANGE<MacWhiteListIndex, 1, 3>);     // hm11.ads:109
static_assert(HAS_RANGE<Percent, 0, 100>);             // hm11.ads:111 (Persent)
static_assert(HAS_RANGE<LayerConnectionLatency, 0, 4>); // hm11.ads:120
static_assert(HAS_RANGE<DiscoveredIndex, 0, 5>);       // hm11.ads:129
static_assert(HAS_RANGE<PioCollectionRate, 0, 99>);    // hm11.ads:272

// New types do not convert from int or from each other.
static_assert(!std::is_convertible_v<int, Percent>);
static_assert(!MIXABLE<Percent, HumidityType>);
static_assert(!MIXABLE<MacWhiteListIndex, PioNumber>);
static_assert(!MIXABLE<DiscoveredIndex, LayerConnectionLatency>);
static_assert(!MIXABLE<PioCollectionRate, Percent>);

// =============================================================================
// Ada `subtype X is Natural/Positive range Min .. Max` -> ada::IntegerSubtype
// =============================================================================

static_assert(HAS_RANGE<PioNumber, 1, 3>);         // hm11.ads:232, Positive
static_assert(HAS_RANGE<DiscoveryTime, 1, 9>);     // hm11.ads:249, Positive
static_assert(HAS_RANGE<TemperatureType, 0, 120>); // hm11.ads:256, Natural
static_assert(HAS_RANGE<HumidityType, 0, 100>);    // hm11.ads:257, Natural

static_assert(std::is_same_v<PioNumber::ParentType, ada::Positive>);
static_assert(std::is_same_v<DiscoveryTime::ParentType, ada::Positive>);
static_assert(std::is_same_v<TemperatureType::ParentType, ada::Natural>);
static_assert(std::is_same_v<HumidityType::ParentType, ada::Natural>);

// As in Ada, subtypes are their base type (Integer): they convert to and from int and between each other.
static_assert(std::is_convertible_v<int, DiscoveryTime> && std::is_convertible_v<DiscoveryTime, int>);
static_assert(std::is_convertible_v<HumidityType, TemperatureType> && std::is_convertible_v<TemperatureType, HumidityType>);
static_assert(std::is_convertible_v<PioNumber, DiscoveryTime> && std::is_convertible_v<DiscoveryTime, PioNumber>);

// =============================================================================
// Ada `type X is array (1 .. N) of Hex_Character / Digit_Character` -> ada::FixedString
// =============================================================================

static_assert(IS_STRING<AdvertisingFlag, ada::HexDigit, 2>);            // hm11.ads:91
static_assert(IS_STRING<AdvertisingTemperatureType, ada::HexDigit, 2>); // hm11.ads:96
static_assert(IS_STRING<MacAddress, ada::HexDigit, 12>);                // hm11.ads:101
static_assert(IS_STRING<UUID, ada::HexDigit, 4>);                       // hm11.ads:104
static_assert(IS_STRING<HandleType, ada::HexDigit, 4>);                 // hm11.ads:189
static_assert(IS_STRING<HumiInformation, ada::HexDigit, 2>);            // hm11.ads:212
static_assert(IS_STRING<IBeaconUuid, ada::HexDigit, 32>);               // hm11.ads:217
static_assert(IS_STRING<VersionType, ada::HexDigit, 4>);                // hm11.ads:219
static_assert(IS_STRING<MeasuredPower, ada::HexDigit, 2>);              // hm11.ads:221
static_assert(IS_STRING<PioNumbers, ada::HexDigit, 3>);                 // hm11.ads:238
static_assert(IS_STRING<AdvertisementData, ada::HexDigit, 12>);         // hm11.ads:268
static_assert(IS_STRING<CharacteristicType, ada::HexDigit, 4>);         // hm11.ads:270

static_assert(IS_STRING<PinType, ada::DecimalDigit, 6>);        // hm11.ads:241
static_assert(IS_STRING<WorkInterval, ada::DecimalDigit, 2>);   // hm11.ads:245
static_assert(IS_STRING<ConnectTimeout, ada::DecimalDigit, 6>); // hm11.ads:261

// hm11.ads:259 `subtype Internal_Temperature_Type is String (1 .. 7)`: any characters.
static_assert(std::is_same_v<InternalTemperatureType, ada::String<7>>);

// Same length and character set, but distinct Ada types.
static_assert(!MIXABLE<MacAddress, AdvertisementData>);
static_assert(!MIXABLE<UUID, HandleType> && !MIXABLE<UUID, VersionType> && !MIXABLE<UUID, CharacteristicType>);
static_assert(!MIXABLE<HandleType, VersionType> && !MIXABLE<HandleType, CharacteristicType>);
static_assert(!MIXABLE<VersionType, CharacteristicType>);
static_assert(!MIXABLE<AdvertisingFlag, AdvertisingTemperatureType> && !MIXABLE<AdvertisingFlag, HumiInformation>);
static_assert(!MIXABLE<AdvertisingFlag, MeasuredPower> && !MIXABLE<HumiInformation, MeasuredPower>);
static_assert(!MIXABLE<AdvertisingTemperatureType, HumiInformation> && !MIXABLE<AdvertisingTemperatureType, MeasuredPower>);
static_assert(!MIXABLE<PinType, ConnectTimeout>);

// =============================================================================
// Runtime checks: the same constraints on values a device reply would supply
// =============================================================================

TEST(TypeAliasesTest, HexTypesAcceptUppercaseHexOnly) {
    // Deliberately stricter than Ada's Hex_Character ('0' .. 'F'): ':' through '@' are rejected.
    EXPECT_TRUE(MacAddress::is_valid(HEX_32.substr(0, 12)));
    EXPECT_TRUE(IBeaconUuid::is_valid(HEX_32));
    EXPECT_FALSE(UUID::is_valid("abcd"));
    EXPECT_FALSE(UUID::is_valid("12:4"));
    EXPECT_FALSE(UUID::is_valid("12@4"));
    EXPECT_FALSE(UUID::is_valid("12G4"));
}

TEST(TypeAliasesTest, DigitTypesAcceptDecimalDigitsOnly) {
    EXPECT_TRUE(PinType::is_valid(DIGITS_7.substr(0, 6)));
    EXPECT_TRUE(WorkInterval::is_valid("99"));
    EXPECT_FALSE(PinType::is_valid("12345A"));
    EXPECT_FALSE(ConnectTimeout::is_valid("1234.5"));
}

TEST(TypeAliasesTest, StringTypesRejectWrongLength) {
    EXPECT_FALSE(MacAddress::is_valid(HEX_32.substr(0, 11)));
    EXPECT_FALSE(MacAddress::is_valid(HEX_32.substr(0, 13)));
    EXPECT_FALSE(PinType::is_valid(DIGITS_7));
    EXPECT_FALSE(MacAddress::try_make(HEX_32.substr(0, 11)).has_value());
}

TEST(TypeAliasesTest, InternalTemperatureAcceptsDecimalPoint) {
    InternalTemperatureType temp{"025.50 "};
    EXPECT_EQ(temp.view(), "025.50 ");
}

TEST(TypeAliasesTest, SubtypesConvertWithinRange) {
    TemperatureType temp = 25;
    HumidityType humidity = temp; // Ada: both are Natural, so this is a range-checked conversion
    EXPECT_EQ(humidity.get(), 25);
    int as_int = DiscoveryTime{9};
    EXPECT_EQ(as_int, 9);
}
