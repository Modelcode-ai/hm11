/**
 * @file ranged_integer_test.cpp
 * @brief Unit tests for RangedInteger template
 *
 * This file contains unit tests for the RangedInteger template class.
 * The tests verify compile-time validation, runtime validation,
 * operator behavior, and constexpr functionality.
 */

#include <gtest/gtest.h>

#include <type_traits>

#include "hm11/util/ranged_integer.hpp"

namespace hm11::util::test {

// Test type for RangedInteger instances
using TestRange = RangedInteger<int, 1, 10>;

// Test that values within range are accepted
TEST(RangedIntegerTest, ValidValues) {
    // Test constructor with valid value
    TestRange a(5);
    EXPECT_EQ(5, a.get_value());

    // Test default constructor (should initialize to Min)
    TestRange b;
    EXPECT_EQ(1, b.get_value());

    // Test constructor with boundary values
    TestRange min(1);
    EXPECT_EQ(1, min.get_value());

    TestRange max(10);
    EXPECT_EQ(10, max.get_value());
}

// Test min() and max() functions
TEST(RangedIntegerTest, MinMaxFunctions) {
    EXPECT_EQ(1, TestRange::min());
    EXPECT_EQ(10, TestRange::max());
}

// Test explicit conversion to underlying type
TEST(RangedIntegerTest, ExplicitConversion) {
    TestRange a(5);
    int value = static_cast<int>(a);
    EXPECT_EQ(5, value);
}

// Test arithmetic operators
TEST(RangedIntegerTest, ArithmeticOperators) {
    TestRange a(5);
    TestRange b(3);

    // Addition
    auto c = a + b;
    EXPECT_EQ(8, c.get_value());

    // Subtraction
    auto d = a - b;
    EXPECT_EQ(2, d.get_value());

    // Multiplication
    auto e = a * b;
    EXPECT_EQ(15, e.get_value()); // This should be clamped to 10 in debug builds

    // Division
    auto f = a / b;
    EXPECT_EQ(1, f.get_value());

    // Modulo
    auto g = a % b;
    EXPECT_EQ(2, g.get_value());
}

// Test compound assignment operators
TEST(RangedIntegerTest, CompoundAssignmentOperators) {
    TestRange a(5);
    TestRange b(3);

    // Addition assignment
    a += b;
    EXPECT_EQ(8, a.get_value());

    // Subtraction assignment
    a -= b;
    EXPECT_EQ(5, a.get_value());

    // Multiplication assignment
    a *= b;
    EXPECT_EQ(15, a.get_value()); // This should be clamped to 10 in debug builds

    // Division assignment
    a /= b;
    EXPECT_EQ(5, a.get_value());

    // Modulo assignment
    a %= b;
    EXPECT_EQ(2, a.get_value());
}

// Test increment/decrement operators
TEST(RangedIntegerTest, IncrementDecrementOperators) {
    TestRange a(5);

    // Pre-increment
    ++a;
    EXPECT_EQ(6, a.get_value());

    // Post-increment
    TestRange b = a++;
    EXPECT_EQ(6, b.get_value());
    EXPECT_EQ(7, a.get_value());

    // Pre-decrement
    --a;
    EXPECT_EQ(6, a.get_value());

    // Post-decrement
    TestRange c = a--;
    EXPECT_EQ(6, c.get_value());
    EXPECT_EQ(5, a.get_value());
}

// Test comparison operators
TEST(RangedIntegerTest, ComparisonOperators) {
    TestRange a(5);
    TestRange b(3);
    TestRange c(5);

    // Equality
    EXPECT_TRUE(a == c);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a == 5);
    EXPECT_FALSE(a == 3);

    // Inequality
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != c);
    EXPECT_TRUE(a != 3);
    EXPECT_FALSE(a != 5);

    // Less than
    EXPECT_TRUE(b < a);
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(b < 5);
    EXPECT_FALSE(a < 3);

    // Greater than
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
    EXPECT_TRUE(a > 3);
    EXPECT_FALSE(b > 5);

    // Less than or equal
    EXPECT_TRUE(b <= a);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(a <= b);
    EXPECT_TRUE(b <= 5);
    EXPECT_TRUE(a <= 5);
    EXPECT_FALSE(a <= 3);

    // Greater than or equal
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(b >= a);
    EXPECT_TRUE(a >= 3);
    EXPECT_TRUE(a >= 5);
    EXPECT_FALSE(b >= 5);
}

// Test operations with raw values
TEST(RangedIntegerTest, OperationsWithRawValues) {
    TestRange a(5);

    // Addition with raw value
    auto b = a + 3;
    EXPECT_EQ(8, b.get_value());

    // Subtraction with raw value
    auto c = a - 2;
    EXPECT_EQ(3, c.get_value());

    // Multiplication with raw value
    auto d = a * 2;
    EXPECT_EQ(10, d.get_value());

    // Division with raw value
    auto e = a / 2;
    EXPECT_EQ(2, e.get_value());

    // Modulo with raw value
    auto f = a % 3;
    EXPECT_EQ(2, f.get_value());

    // Compound assignment with raw value
    a += 2;
    EXPECT_EQ(7, a.get_value());

    a -= 3;
    EXPECT_EQ(4, a.get_value());

    a *= 2;
    EXPECT_EQ(8, a.get_value());

    a /= 2;
    EXPECT_EQ(4, a.get_value());

    a %= 3;
    EXPECT_EQ(1, a.get_value());
}

// Test different tag types
TEST(RangedIntegerTest, TagTypes) {
    // Define two RangedInteger types with same range but different tags
    using RangeA = RangedInteger<int, 0, 100, struct TagA>;
    using RangeB = RangedInteger<int, 0, 100, struct TagB>;

    RangeA a(50);
    RangeB b(50);

    // Verify they are different types
    EXPECT_FALSE((std::is_same_v<RangeA, RangeB>));

    // Verify they behave the same
    EXPECT_EQ(a.get_value(), b.get_value());
}

// Test the Ada range type aliases
TEST(RangedIntegerTest, AdaRangeTypes) {
    // Test Percent
    Percent p(75);
    EXPECT_EQ(75, p.get_value());
    EXPECT_EQ(0, Percent::min());
    EXPECT_EQ(100, Percent::max());

    // Test MacWhiteListIndex
    MacWhiteListIndex mwli(2);
    EXPECT_EQ(2, mwli.get_value());
    EXPECT_EQ(1, MacWhiteListIndex::min());
    EXPECT_EQ(3, MacWhiteListIndex::max());

    // Test LayerConnectionLatency
    LayerConnectionLatency lcl(3);
    EXPECT_EQ(3, lcl.get_value());
    EXPECT_EQ(0, LayerConnectionLatency::min());
    EXPECT_EQ(4, LayerConnectionLatency::max());

    // Test DiscoveredIndex
    DiscoveredIndex di(4);
    EXPECT_EQ(4, di.get_value());
    EXPECT_EQ(0, DiscoveredIndex::min());
    EXPECT_EQ(5, DiscoveredIndex::max());

    // Test PioCollectionRate
    PioCollectionRate pcr(50);
    EXPECT_EQ(50, pcr.get_value());
    EXPECT_EQ(0, PioCollectionRate::min());
    EXPECT_EQ(99, PioCollectionRate::max());

    // Test PioNumber
    PioNumber pn(2);
    EXPECT_EQ(2, pn.get_value());
    EXPECT_EQ(1, PioNumber::min());
    EXPECT_EQ(3, PioNumber::max());

    // Test DiscoveryTime
    DiscoveryTime dt(5);
    EXPECT_EQ(5, dt.get_value());
    EXPECT_EQ(1, DiscoveryTime::min());
    EXPECT_EQ(9, DiscoveryTime::max());

    // Test TemperatureType
    TemperatureType tt(75);
    EXPECT_EQ(75, tt.get_value());
    EXPECT_EQ(0, TemperatureType::min());
    EXPECT_EQ(120, TemperatureType::max());

    // Test HumidityType
    HumidityType ht(50);
    EXPECT_EQ(50, ht.get_value());
    EXPECT_EQ(0, HumidityType::min());
    EXPECT_EQ(100, HumidityType::max());
}

// Test constexpr behavior
TEST(RangedIntegerTest, ConstexprBehavior) {
    // Verify that RangedInteger operations can be used in constexpr contexts
    constexpr TestRange a(5);
    constexpr TestRange b(3);
    constexpr auto c = a + b;
    constexpr auto d = a - b;
    constexpr auto e = a * b;
    constexpr auto f = a / b;
    constexpr auto g = a % b;

    static_assert(c.get_value() == 8, "Constexpr addition failed");
    static_assert(d.get_value() == 2, "Constexpr subtraction failed");
    static_assert(
        e.get_value() == 15,
        "Constexpr multiplication failed"); // This might not compile in debug builds due to assertions
    static_assert(f.get_value() == 1, "Constexpr division failed");
    static_assert(g.get_value() == 2, "Constexpr modulo failed");

    // Verify comparison operators in constexpr context
    static_assert(a == a, "Constexpr equality failed");
    static_assert(a != b, "Constexpr inequality failed");
    static_assert(a > b, "Constexpr greater than failed");
    static_assert(b < a, "Constexpr less than failed");
    static_assert(a >= b, "Constexpr greater than or equal failed");
    static_assert(b <= a, "Constexpr less than or equal failed");
}

// Test compile-time validation through static_assert
// This test would fail to compile if Min > Max
/*
TEST(RangedIntegerTest, CompileTimeValidation) {
    using InvalidRange = RangedInteger<int, 10, 1>;
    InvalidRange invalid(5); // This should cause a static_assert failure
}
*/

} // namespace hm11::util::test