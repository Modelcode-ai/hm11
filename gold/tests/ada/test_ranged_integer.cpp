// test_ranged_integer.cpp - Unit tests for ada::RangedInteger.
//
// Built three times (see CMakeLists.txt) because out-of-range behaviour depends on the build mode:
//   ada_tests           asserts enabled            -> death tests
//   ada_tests_saturate  ENABLE_RUNTIME_RANGE_CHECKS -> saturation tests
//   ada_tests_ndebug    NDEBUG                     -> constant-evaluation checks still hold

#include <gtest/gtest.h>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "ada/ranged_integer.hpp"

namespace {

using Percent = ada::RangedInteger<std::uint8_t, 0, 100, struct PercentTag>;
using Temperature = ada::RangedInteger<std::uint8_t, 0, 100, struct TemperatureTag>; // same range, distinct type
using Index = ada::RangedInteger<std::uint8_t, 1, 3, struct IndexTag>;
using Signed = ada::RangedInteger<std::int8_t, -50, 50, struct SignedTag>;
using Big = ada::RangedInteger<std::uint8_t, 0, 250, struct BigTag>;
using Byte = ada::RangedInteger<std::uint8_t, 0, 255, struct ByteTag>;
using High = ada::RangedInteger<std::uint8_t, 10, 255, struct HighTag>;
using Large32 = ada::RangedInteger<std::uint32_t, 0, 4'000'000'000U, struct Large32Tag>;
using Full32 = ada::RangedInteger<std::int32_t, INT32_MIN, INT32_MAX, struct Full32Tag>;
using Small64 = ada::RangedInteger<std::uint64_t, 0, 1000, struct Small64Tag>;
using Half32 = ada::RangedInteger<std::int32_t, 0, INT32_MAX / 2, struct Half32Tag>; // three Max values overflow Wide

// ============================================================================
// Compile-time properties
// ============================================================================

// Wide is the narrowest type holding every intermediate result.
static_assert(std::is_same_v<Percent::Wide, std::int32_t>);
static_assert(std::is_same_v<Signed::Wide, std::int32_t>);
static_assert(std::is_same_v<Small64::Wide, std::int32_t>);
static_assert(std::is_same_v<Large32::Wide, std::int64_t>);
static_assert(std::is_same_v<Full32::Wide, std::int64_t>);
static_assert(std::is_same_v<Half32::Wide, std::int32_t>);

// Zero overhead: same size as T, trivially copyable.
static_assert(sizeof(Percent) == sizeof(std::uint8_t));
static_assert(sizeof(Small64) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<Percent>);

// Construction is explicit, and bool is rejected.
static_assert(std::is_constructible_v<Percent, int>);
static_assert(std::is_constructible_v<Percent, std::uint64_t>);
static_assert(!std::is_convertible_v<int, Percent>);
static_assert(!std::is_constructible_v<Percent, bool>);
static_assert(!std::is_convertible_v<Percent, int>);

// A base-type result exposes no value until stored in the range type, which checks it.
template <typename T>
concept Readable = requires(T t) { t.get(); };
static_assert(Readable<Percent> && !Readable<Percent::Base>);
static_assert(std::is_convertible_v<Percent::Base, Percent>);

// Tags prevent mixing types, even with identical ranges.
template <typename A, typename B>
concept AddAssignable = requires(A a, B b) { a += b; };
template <typename A, typename B>
concept Addable = requires(A a, B b) { a + b; };
template <typename A, typename B>
concept EqualityComparable = requires(A a, B b) { a == b; };
template <typename A, typename B>
concept Ordered = requires(A a, B b) { a < b; };

static_assert(AddAssignable<Percent, Percent> && !AddAssignable<Percent, Temperature>);
static_assert(Addable<Percent, Percent> && !Addable<Percent, Temperature>);
static_assert(EqualityComparable<Percent, Percent> && !EqualityComparable<Percent, Temperature>);
static_assert(Ordered<Percent, Percent> && !Ordered<Percent, Temperature>);
static_assert(!AddAssignable<Percent, int>);
static_assert(!std::is_assignable_v<Percent&, Temperature>);

// Out-of-range values are rejected during constant evaluation in every build mode, including NDEBUG.
template <typename R, auto V>
concept ConstantConstructible = requires { typename std::integral_constant<typename R::ValueType, R{V}.get()>; };

template <typename R, auto A, auto B> constexpr auto constant_sum() {
    return R{R{A} + R{B}}.get();
}
template <typename R, auto A, auto B> constexpr auto constant_difference() {
    return R{R{A} - R{B}}.get();
}
template <typename R, auto V> constexpr bool constant_triple() {
    return R{V} + R{V} + R{V} > R{V};
}
template <typename R, auto V> constexpr auto constant_increment() {
    R r{V};
    ++r;
    return r.get();
}
template <typename R, auto V> constexpr auto constant_decrement() {
    R r{V};
    --r;
    return r.get();
}

template <typename R, auto A, auto B>
concept ConstantAddable = requires { typename std::integral_constant<typename R::ValueType, constant_sum<R, A, B>()>; };
template <typename R, auto A, auto B>
concept ConstantSubtractable = requires {
    typename std::integral_constant<typename R::ValueType, constant_difference<R, A, B>()>;
};
template <typename R, auto V>
concept ConstantTriplable = requires { typename std::integral_constant<bool, constant_triple<R, V>()>; };
template <typename R, auto V>
concept ConstantIncrementable = requires {
    typename std::integral_constant<typename R::ValueType, constant_increment<R, V>()>;
};
template <typename R, auto V>
concept ConstantDecrementable = requires {
    typename std::integral_constant<typename R::ValueType, constant_decrement<R, V>()>;
};

static_assert(ConstantConstructible<Percent, 0> && ConstantConstructible<Percent, 100>);
static_assert(!ConstantConstructible<Percent, -1> && !ConstantConstructible<Percent, 101>);
static_assert(!ConstantConstructible<Percent, 256>); // would wrap to 0 if narrowed before the check
static_assert(!ConstantConstructible<Index, 0> && !ConstantConstructible<Index, 4>);

static_assert(ConstantAddable<Big, 200, 50> && !ConstantAddable<Big, 200, 100>);          // 300 would wrap to 44
static_assert(ConstantSubtractable<High, 20, 10> && !ConstantSubtractable<High, 10, 20>); // -10 would wrap to 246
static_assert(ConstantIncrementable<Byte, 254> && !ConstantIncrementable<Byte, 255>);     // 256 would wrap to 0
static_assert(ConstantDecrementable<Percent, 1> && !ConstantDecrementable<Percent, 0>);
static_assert(!ConstantIncrementable<Full32, INT32_MAX>); // would be signed overflow in T
static_assert(ConstantTriplable<Half32, 1000> && !ConstantTriplable<Half32, INT32_MAX / 2>); // base overflow

// ============================================================================
// Construction
// ============================================================================

TEST(AdaRangedIntegerTest, DefaultConstructsToMin) {
    EXPECT_EQ(Percent{}.get(), 0);
    EXPECT_EQ(Index{}.get(), 1);
    EXPECT_EQ(Signed{}.get(), -50);
}

TEST(AdaRangedIntegerTest, ExposesBounds) {
    EXPECT_EQ(Percent::MIN_VALUE, 0);
    EXPECT_EQ(Percent::MAX_VALUE, 100);
    EXPECT_EQ(Signed::MIN_VALUE, -50);
    EXPECT_EQ(Signed::MAX_VALUE, 50);
}

TEST(AdaRangedIntegerTest, ConstructsAtBoundaries) {
    EXPECT_EQ(Percent{0}.get(), 0);
    EXPECT_EQ(Percent{100}.get(), 100);
    EXPECT_EQ(Signed{-50}.get(), -50);
    EXPECT_EQ(Signed{50}.get(), 50);
    EXPECT_EQ(Byte{255}.get(), 255);
}

TEST(AdaRangedIntegerTest, ConstructsFromAnyIntegerType) {
    const int from_int = 42;
    const long long from_long_long = 42;
    const std::uint64_t from_u64 = 42;
    const std::int8_t from_i8 = 42;

    EXPECT_EQ(Percent{from_int}.get(), 42);
    EXPECT_EQ(Percent{from_long_long}.get(), 42);
    EXPECT_EQ(Percent{from_u64}.get(), 42);
    EXPECT_EQ(Percent{from_i8}.get(), 42);
}

TEST(AdaRangedIntegerTest, ConstexprConstruction) {
    constexpr Percent p{75};
    static_assert(p.get() == 75);
    EXPECT_EQ(p.get(), 75);
}

// ============================================================================
// Comparison
// ============================================================================

TEST(AdaRangedIntegerTest, ComparisonOperators) {
    const Percent a{30};
    const Percent b{70};
    const Percent a2{30};

    EXPECT_TRUE(a == a2);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a <= a2);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_EQ(a <=> b, std::strong_ordering::less);
    EXPECT_EQ(a <=> a2, std::strong_ordering::equal);
}

TEST(AdaRangedIntegerTest, SignedComparison) {
    EXPECT_TRUE(Signed{-10} < Signed{5});
    EXPECT_TRUE(Signed{-50} < Signed{-49});
}

TEST(AdaRangedIntegerTest, ConstexprComparison) {
    static_assert(Percent{10} < Percent{20});
    static_assert(Percent{10} == Percent{10});
    SUCCEED();
}

// ============================================================================
// Arithmetic within range
// ============================================================================

TEST(AdaRangedIntegerTest, AdditionAndSubtraction) {
    EXPECT_EQ(Percent{Percent{30} + Percent{40}}.get(), 70);
    EXPECT_EQ(Percent{Percent{70} - Percent{30}}.get(), 40);
    EXPECT_EQ(Percent{Percent{60} + Percent{40}}.get(), 100);
    EXPECT_EQ(Percent{Percent{40} - Percent{40}}.get(), 0);
}

TEST(AdaRangedIntegerTest, CompoundAssignment) {
    Percent p{30};
    p += Percent{20};
    EXPECT_EQ(p.get(), 50);
    p -= Percent{50};
    EXPECT_EQ(p.get(), 0);
}

TEST(AdaRangedIntegerTest, IncrementDecrement) {
    Percent p{50};
    EXPECT_EQ((++p).get(), 51);
    EXPECT_EQ((p++).get(), 51);
    EXPECT_EQ(p.get(), 52);
    EXPECT_EQ((--p).get(), 51);
    EXPECT_EQ((p--).get(), 51);
    EXPECT_EQ(p.get(), 50);
}

TEST(AdaRangedIntegerTest, IncrementToMaxAndDecrementToMin) {
    Byte b{254};
    ++b;
    EXPECT_EQ(b.get(), 255);

    Index i{2};
    --i;
    EXPECT_EQ(i.get(), 1);
}

TEST(AdaRangedIntegerTest, SignedArithmetic) {
    EXPECT_EQ(Signed{Signed{-30} + Signed{-20}}.get(), -50);
    EXPECT_EQ(Signed{Signed{-10} - Signed{40}}.get(), -50);
    EXPECT_EQ(Signed{Signed{30} - Signed{-20}}.get(), 50);
}

TEST(AdaRangedIntegerTest, IntermediateResultsBeyondTFitInWide) {
    // Values near the top of uint32_t: arithmetic runs in int64_t.
    EXPECT_EQ(Large32{Large32{3'000'000'000U} + Large32{1'000'000'000U}}.get(), 4'000'000'000U);
    EXPECT_EQ(Large32{Large32{1'000'000'000U} - Large32{1'000'000'000U}}.get(), 0U);

    // Full int32_t range: no signed overflow at the edges.
    EXPECT_EQ(Full32{Full32{INT32_MAX} - Full32{1}}.get(), INT32_MAX - 1);
    EXPECT_EQ(Full32{Full32{INT32_MIN} + Full32{1}}.get(), INT32_MIN + 1);

    // 64-bit storage with a small range still works.
    Small64 s{500};
    s += Small64{500};
    EXPECT_EQ(s.get(), 1000U);
}

TEST(AdaRangedIntegerTest, ConstexprArithmetic) {
    constexpr Percent sum = Percent{30} + Percent{40};
    static_assert(sum.get() == 70);
    EXPECT_EQ(sum.get(), 70);
}

// As in Ada, A + B - C is evaluated in the base type; only the stored result is range-checked.
TEST(AdaRangedIntegerTest, IntermediateResultMayLeaveRange) {
    const Percent a{80};
    const Percent b{30};
    const Percent c{20};
    const Percent result = a + b - c;
    EXPECT_EQ(result.get(), 90);

    constexpr Percent constant_result = Percent{80} + Percent{30} - Percent{20};
    static_assert(constant_result.get() == 90);

    // Base-type results are compared without a range check.
    EXPECT_TRUE(a + b > Percent{100});
    EXPECT_TRUE(a + b == Percent{100} + Percent{10});
    EXPECT_TRUE(c - b < Percent{0});

    Percent p{90};
    p += b - c;
    EXPECT_EQ(p.get(), 100);
}

// ============================================================================
// Out-of-range behaviour
// ============================================================================

#if defined(ENABLE_RUNTIME_RANGE_CHECKS)

// Saturation clamps the true result, not a wrapped one (e.g. 300 -> 250, not 44).
TEST(AdaRangedIntegerTest, SaturatesConstruction) {
    const int too_high = 300;
    const int too_low = -1;
    EXPECT_EQ(Percent{too_high}.get(), 100);
    EXPECT_EQ(Percent{too_low}.get(), 0);
    EXPECT_EQ(Index{0}.get(), 1);
}

TEST(AdaRangedIntegerTest, SaturatesArithmetic) {
    // Saturation applies when the result is stored, not to each operator.
    EXPECT_EQ(Big{Big{200} + Big{100}}.get(), 250);
    EXPECT_EQ(High{High{10} - High{20}}.get(), 10);
    const Big stored = Big{200} + Big{100} - Big{60};
    EXPECT_EQ(stored.get(), 240);

    // Base-range overflow saturates to the Wide limit, then to Max when stored.
    const Half32 max{INT32_MAX / 2};
    const Half32 overflowed = max + max + max;
    EXPECT_EQ(overflowed.get(), INT32_MAX / 2);

    Big b{200};
    b += Big{100};
    EXPECT_EQ(b.get(), 250);

    High h{10};
    h -= High{20};
    EXPECT_EQ(h.get(), 10);
}

TEST(AdaRangedIntegerTest, SaturatesIncrementDecrement) {
    Byte b{255};
    ++b;
    EXPECT_EQ(b.get(), 255);

    Percent p{0};
    p--;
    EXPECT_EQ(p.get(), 0);

    Full32 f{INT32_MAX};
    ++f;
    EXPECT_EQ(f.get(), INT32_MAX);
}

#elif !defined(NDEBUG)

// Each case below previously wrapped to an in-range value and passed the check.
TEST(AdaRangedIntegerDeathTest, ConstructionAboveMax) {
    const int too_high = 300; // narrowed to uint8_t first, this would be 44
    EXPECT_DEATH(
        {
            Percent p{too_high};
            (void)p;
        },
        "Value above maximum bound");

    const int wraps_to_zero = 256;
    EXPECT_DEATH(
        {
            Percent p{wraps_to_zero};
            (void)p;
        },
        "Value above maximum bound");

    const std::uint64_t huge = std::numeric_limits<std::uint64_t>::max();
    EXPECT_DEATH(
        {
            Percent p{huge};
            (void)p;
        },
        "Value above maximum bound");
}

TEST(AdaRangedIntegerDeathTest, ConstructionBelowMin) {
    const int negative = -1; // narrowed to uint8_t first, this would be 255
    EXPECT_DEATH(
        {
            Percent p{negative};
            (void)p;
        },
        "Value below minimum bound");

    const int zero = 0;
    EXPECT_DEATH(
        {
            Index i{zero};
            (void)i;
        },
        "Value below minimum bound");
}

TEST(AdaRangedIntegerDeathTest, AdditionOverflow) {
    const Big a{200};
    const Big b{100};
    EXPECT_DEATH(
        {
            Big c = a + b;
            (void)c;
        },
        "Value above maximum bound");
    EXPECT_DEATH(
        {
            Big c{a};
            c += b;
        },
        "Value above maximum bound");
}

TEST(AdaRangedIntegerDeathTest, BaseRangeOverflow) {
    const Half32 max{INT32_MAX / 2};
    EXPECT_DEATH({ (void)(max + max + max); }, "Base range overflow");
}

TEST(AdaRangedIntegerDeathTest, SubtractionUnderflow) {
    const High a{10};
    const High b{20};
    EXPECT_DEATH(
        {
            High c = a - b;
            (void)c;
        },
        "Value below minimum bound");
    EXPECT_DEATH(
        {
            High c{a};
            c -= b;
        },
        "Value below minimum bound");
}

TEST(AdaRangedIntegerDeathTest, IncrementAtMax) {
    EXPECT_DEATH(
        {
            Byte b{255};
            ++b;
        },
        "Value above maximum bound");
    EXPECT_DEATH(
        {
            Byte b{255};
            b++;
        },
        "Value above maximum bound");
    EXPECT_DEATH(
        {
            Full32 f{INT32_MAX};
            ++f;
        },
        "Value above maximum bound");
}

TEST(AdaRangedIntegerDeathTest, DecrementAtMin) {
    EXPECT_DEATH(
        {
            Percent p{0};
            --p;
        },
        "Value below minimum bound");
    EXPECT_DEATH(
        {
            Percent p{0};
            p--;
        },
        "Value below minimum bound");
    EXPECT_DEATH(
        {
            Full32 f{INT32_MIN};
            --f;
        },
        "Value below minimum bound");
}

#endif

} // namespace
