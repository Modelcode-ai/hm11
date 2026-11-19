// test_integer_subtype.cpp - Unit tests for ada::IntegerSubtype, ada::Natural and ada::Positive.
//
// Built in the same three modes as test_ranged_integer.cpp (see CMakeLists.txt).

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <type_traits>

#include "ada/integer_subtype.hpp"

namespace {

// Subtypes from the HM-11 Ada source.
using Temperature = ada::IntegerSubtype<ada::Natural, 0, 120>;  // subtype Temperature_Type is Natural range 0 .. 120
using Humidity = ada::IntegerSubtype<ada::Natural, 0, 100>;     // subtype Humidity_Type is Natural range 0 .. 100
using DiscoveryTime = ada::IntegerSubtype<ada::Positive, 1, 9>; // subtype Discovery_Time is Positive range 1 .. 9

// IntegerSubtype of a RangedInteger type, and a subtype of a subtype.
using Percent = ada::RangedInteger<std::uint8_t, 0, 100, struct PercentTag>;
using Level = ada::RangedInteger<std::uint8_t, 0, 100, struct LevelTag>;
using LowBattery = ada::IntegerSubtype<Percent, 0, 20>;
using Critical = ada::IntegerSubtype<LowBattery, 0, 5>;

// Narrow storage when the base is a narrow built-in integer.
using Small = ada::IntegerSubtype<std::uint8_t, 0, 200>;

// ============================================================================
// Compile-time properties
// ============================================================================

static_assert(ada::Natural::MIN_VALUE == 0 && ada::Natural::MAX_VALUE == std::numeric_limits<int>::max());
static_assert(ada::Positive::MIN_VALUE == 1 && ada::Positive::MAX_VALUE == std::numeric_limits<int>::max());

// The base type is the outermost non-subtype parent.
static_assert(std::is_same_v<Temperature::ParentType, ada::Natural>);
static_assert(std::is_same_v<Temperature::BaseType, int>);
static_assert(std::is_same_v<DiscoveryTime::BaseType, int>);
static_assert(std::is_same_v<Critical::ParentType, LowBattery>);
static_assert(std::is_same_v<Critical::BaseType, Percent>);
static_assert(std::is_same_v<Critical::ValueType, std::uint8_t>);

static_assert(sizeof(Temperature) == sizeof(int));
static_assert(sizeof(Small) == sizeof(std::uint8_t));
static_assert(sizeof(LowBattery) == sizeof(Percent));
static_assert(std::is_trivially_copyable_v<Temperature>);

// Built-in base: implicit both ways, from any integer type, and between subtypes of the same base.
static_assert(std::is_convertible_v<int, Temperature>);
static_assert(std::is_convertible_v<std::uint64_t, Temperature>);
static_assert(std::is_convertible_v<Temperature, int>);
static_assert(std::is_convertible_v<Temperature, Humidity>);
static_assert(std::is_convertible_v<Humidity, Temperature>);
static_assert(std::is_convertible_v<Temperature, ada::Natural>);
static_assert(std::is_convertible_v<DiscoveryTime, ada::Natural>);
static_assert(!std::is_constructible_v<Temperature, bool>);

// RangedInteger base: implicit to and from the base, explicit from integers, like RangedInteger itself.
static_assert(std::is_convertible_v<Percent, LowBattery>);
static_assert(std::is_convertible_v<LowBattery, Percent>);
static_assert(std::is_convertible_v<Critical, LowBattery>);
static_assert(std::is_convertible_v<Critical, Percent>);
static_assert(std::is_constructible_v<LowBattery, int>);
static_assert(!std::is_convertible_v<int, LowBattery>);
static_assert(!std::is_convertible_v<LowBattery, int>);

// Subtypes of different base types do not mix.
static_assert(!std::is_convertible_v<Level, LowBattery>);
static_assert(!std::is_constructible_v<LowBattery, Level>);
static_assert(!std::is_constructible_v<LowBattery, Temperature>);
static_assert(!std::is_constructible_v<Temperature, LowBattery>);
static_assert(!std::is_constructible_v<Small, Temperature>);

template <typename A, typename B>
concept Addable = requires(A a, B b) { a + b; };
static_assert(Addable<LowBattery, LowBattery> && Addable<LowBattery, Percent> && Addable<Critical, Percent>);
static_assert(!Addable<LowBattery, Level> && !Addable<LowBattery, int>);

// Arithmetic results have the base type.
static_assert(std::is_same_v<decltype(Temperature{} + Humidity{}), int>);
static_assert(std::is_same_v<decltype(LowBattery{} + LowBattery{}), Percent::Base>);
static_assert(std::is_same_v<decltype(Critical{} + Percent{}), Percent::Base>);

// Out-of-range values are rejected during constant evaluation in every build mode, including NDEBUG.
template <typename S, auto V>
concept ConstantConstructible = requires { typename std::integral_constant<typename S::ValueType, S{V}.get()>; };

template <typename To, typename From, auto V> constexpr auto constant_convert() {
    return To{From{V}}.get();
}
template <typename To, typename From, auto V>
concept ConstantConvertible = requires {
    typename std::integral_constant<typename To::ValueType, constant_convert<To, From, V>()>;
};

static_assert(ConstantConstructible<Temperature, 0> && ConstantConstructible<Temperature, 120>);
static_assert(!ConstantConstructible<Temperature, -1> && !ConstantConstructible<Temperature, 121>);
static_assert(!ConstantConstructible<DiscoveryTime, 0> && !ConstantConstructible<DiscoveryTime, 10>);
static_assert(!ConstantConstructible<ada::Positive, 0>);
static_assert(!ConstantConstructible<Temperature, 4'294'967'301LL>); // would be 5 if narrowed to int first
static_assert(!ConstantConstructible<Small, 256>);                   // would be 0 if narrowed to uint8_t first
static_assert(ConstantConstructible<LowBattery, 20> && !ConstantConstructible<LowBattery, 21>);
static_assert(!ConstantConstructible<Critical, 6>);

static_assert(ConstantConvertible<Temperature, Humidity, 100>);
static_assert(ConstantConvertible<Humidity, Temperature, 100> && !ConstantConvertible<Humidity, Temperature, 101>);
static_assert(ConstantConvertible<LowBattery, Percent, 20> && !ConstantConvertible<LowBattery, Percent, 21>);
static_assert(ConstantConvertible<Critical, LowBattery, 5> && !ConstantConvertible<Critical, LowBattery, 6>);

// ============================================================================
// Built-in base type
// ============================================================================

TEST(AdaIntegerSubtypeTest, DefaultConstructsToMin) {
    EXPECT_EQ(Temperature{}.get(), 0);
    EXPECT_EQ(DiscoveryTime{}.get(), 1);
    EXPECT_EQ(ada::Positive{}.get(), 1);
}

TEST(AdaIntegerSubtypeTest, ConstructsAtBoundaries) {
    EXPECT_EQ(Temperature{0}.get(), 0);
    EXPECT_EQ(Temperature{120}.get(), 120);
    EXPECT_EQ(DiscoveryTime{1}.get(), 1);
    EXPECT_EQ(DiscoveryTime{9}.get(), 9);
    EXPECT_EQ(ada::Natural{std::numeric_limits<int>::max()}.get(), std::numeric_limits<int>::max());
}

TEST(AdaIntegerSubtypeTest, ImplicitFromAnyIntegerType) {
    const long long from_long_long = 42;
    const std::uint64_t from_u64 = 42;
    const Temperature a = 42;
    const Temperature b = from_long_long;
    const Temperature c = from_u64;
    EXPECT_EQ(a.get(), 42);
    EXPECT_EQ(b.get(), 42);
    EXPECT_EQ(c.get(), 42);
}

TEST(AdaIntegerSubtypeTest, ImplicitToBaseType) {
    const Temperature t = 75;
    const int raw = t;
    EXPECT_EQ(raw, 75);
}

TEST(AdaIntegerSubtypeTest, ConvertsBetweenSubtypesOfTheSameBase) {
    const Temperature t = 80;
    const Humidity h = t;
    EXPECT_EQ(h.get(), 80);

    const DiscoveryTime d = 3;
    const ada::Positive p = d;
    const ada::Natural n = p;
    const Temperature from_discovery = d;
    EXPECT_EQ(p.get(), 3);
    EXPECT_EQ(n.get(), 3);
    EXPECT_EQ(from_discovery.get(), 3);
}

TEST(AdaIntegerSubtypeTest, ArithmeticUsesBaseType) {
    const Temperature t = 30;
    const Humidity h = 40;
    const int sum = t + h; // legal in Ada: both are Integer
    EXPECT_EQ(sum, 70);

    // Ada: T := T + 1;
    Temperature u = 119;
    u = u + 1;
    EXPECT_EQ(u.get(), 120);

    // Intermediate results may leave the subtype's range; only the stored result is checked.
    const Temperature v = t + 200 - 200;
    EXPECT_EQ(v.get(), 30);
}

TEST(AdaIntegerSubtypeTest, NarrowBaseStorage) {
    Small s = 199;
    s = s + 1;
    EXPECT_EQ(s.get(), 200);
}

TEST(AdaIntegerSubtypeTest, Comparison) {
    const Temperature t = 50;
    const Humidity h = 60;
    EXPECT_TRUE(t < h);
    EXPECT_TRUE(t == 50);
    EXPECT_TRUE(h != t);
    EXPECT_TRUE(t >= Temperature{50});
}

TEST(AdaIntegerSubtypeTest, Constexpr) {
    constexpr Temperature t = 100;
    constexpr Humidity h = t;
    static_assert(h.get() == 100);
    static_assert(t + h == 200);
    EXPECT_EQ(h.get(), 100);
}

// ============================================================================
// RangedInteger base type
// ============================================================================

TEST(AdaIntegerSubtypeTest, RangedIntegerBaseConversions) {
    const LowBattery low = Percent{15};
    const Percent p = low;
    EXPECT_EQ(low.get(), 15);
    EXPECT_EQ(p.get(), 15);

    const LowBattery explicit_low{10};
    EXPECT_EQ(explicit_low.get(), 10);
}

TEST(AdaIntegerSubtypeTest, RangedIntegerBaseArithmeticAndComparison) {
    const LowBattery a = Percent{15};
    const LowBattery b = Percent{20};
    const Percent sum = a + b; // Percent arithmetic: 35 is a valid Percent
    EXPECT_EQ(sum.get(), 35);
    EXPECT_EQ(Percent{a - Percent{5}}.get(), 10);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a == Percent{15});
}

// Percent arithmetic is evaluated in Percent'Base; only the stored result is checked.
TEST(AdaIntegerSubtypeTest, RangedIntegerBaseIntermediateResult) {
    const LowBattery a = Percent{20};
    const Percent result = a - Percent{30} + Percent{40};
    EXPECT_EQ(result.get(), 30);

    const LowBattery low = Percent{50} - Percent{45};
    EXPECT_EQ(low.get(), 5);
}

TEST(AdaIntegerSubtypeTest, SubtypeOfSubtype) {
    const Critical c{3};
    const LowBattery low = c;
    const Percent p = c;
    EXPECT_EQ(low.get(), 3);
    EXPECT_EQ(p.get(), 3);
    EXPECT_EQ(Percent{c + Percent{1}}.get(), 4);
}

// ============================================================================
// Out-of-range behaviour
// ============================================================================

#if defined(ENABLE_RUNTIME_RANGE_CHECKS)

TEST(AdaIntegerSubtypeTest, SaturatesConstruction) {
    const int too_high = 121;
    const int too_low = -5;
    const long long wraps_to_five = 4'294'967'301LL;
    EXPECT_EQ(Temperature{too_high}.get(), 120);
    EXPECT_EQ(Temperature{too_low}.get(), 0);
    EXPECT_EQ(Temperature{wraps_to_five}.get(), 120);
    EXPECT_EQ(DiscoveryTime{0}.get(), 1);
}

TEST(AdaIntegerSubtypeTest, SaturatesConversion) {
    const Temperature t = 110;
    const Humidity h = t;
    EXPECT_EQ(h.get(), 100);

    const LowBattery low = Percent{50};
    EXPECT_EQ(low.get(), 20);
}

#elif !defined(NDEBUG)

TEST(AdaIntegerSubtypeDeathTest, ConstructionOutOfRange) {
    const int too_high = 121;
    const int too_low = -1;
    EXPECT_DEATH(
        {
            Temperature t = too_high;
            (void)t;
        },
        "Value above maximum bound");
    EXPECT_DEATH(
        {
            Temperature t = too_low;
            (void)t;
        },
        "Value below minimum bound");
    EXPECT_DEATH(
        {
            DiscoveryTime d = 0;
            (void)d;
        },
        "Value below minimum bound");
}

TEST(AdaIntegerSubtypeDeathTest, ConstructionChecksValueBeforeNarrowing) {
    const long long wraps_to_five = 4'294'967'301LL; // 2^32 + 5
    EXPECT_DEATH(
        {
            Temperature t = wraps_to_five;
            (void)t;
        },
        "Value above maximum bound");

    const int wraps_to_zero = 256;
    EXPECT_DEATH(
        {
            Small s = wraps_to_zero;
            (void)s;
        },
        "Value above maximum bound");
}

TEST(AdaIntegerSubtypeDeathTest, ConversionBetweenSubtypes) {
    const Temperature t = 110;
    EXPECT_DEATH(
        {
            Humidity h = t;
            (void)h;
        },
        "Value above maximum bound");

    const ada::Natural zero = 0;
    EXPECT_DEATH(
        {
            ada::Positive p = zero;
            (void)p;
        },
        "Value below minimum bound");
}

TEST(AdaIntegerSubtypeDeathTest, AssignmentFromArithmetic) {
    Temperature t = 120;
    EXPECT_DEATH({ t = t + 1; }, "Value above maximum bound");
}

TEST(AdaIntegerSubtypeDeathTest, RangedIntegerBase) {
    const Percent p{50};
    EXPECT_DEATH(
        {
            LowBattery low = p;
            (void)low;
        },
        "Value above maximum bound");

    const LowBattery low = Percent{10};
    EXPECT_DEATH(
        {
            Critical c = low;
            (void)c;
        },
        "Value above maximum bound");

    // Percent arithmetic is not checked until stored: storing -10 in Percent (range 0 .. 100) fails.
    const LowBattery a = Percent{20};
    EXPECT_DEATH(
        {
            Percent negative = a - Percent{30};
            (void)negative;
        },
        "Value below minimum bound");
    EXPECT_DEATH(
        {
            LowBattery too_high = Percent{50} - Percent{20};
            (void)too_high;
        },
        "Value above maximum bound");
}

#endif

} // namespace
