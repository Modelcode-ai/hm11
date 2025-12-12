#include <gtest/gtest.h>

#include <type_traits>

#include "hm11/types.hpp" // Include full type system

using namespace hm11;

// Test tags for different types
struct TestTag1 {};
struct TestTag2 {};

// Type aliases for testing
using TestPercent = RangedInteger<uint8_t, 0, 100, TestTag1>;
using TestTemperature = RangedInteger<int8_t, -20, 50, TestTag2>;

class RangedIntegerTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Construction and Basic Functionality Tests
// ============================================================================

TEST_F(RangedIntegerTest, DefaultConstructor) {
    TestPercent percent;
    EXPECT_EQ(percent.value(), 0); // Default constructs to minimum value
    EXPECT_EQ(percent, TestPercent::MIN_VALUE);
}

TEST_F(RangedIntegerTest, ExplicitConstructorValidValues) {
    TestPercent percent50(50);
    EXPECT_EQ(percent50.value(), 50);

    TestPercent percent_min(0);
    EXPECT_EQ(percent_min.value(), 0);

    TestPercent percent_max(100);
    EXPECT_EQ(percent_max.value(), 100);
}

TEST_F(RangedIntegerTest, ExplicitConstructorInvalidValues) {
    // In debug builds, this should trigger assertions/exceptions
    // In release builds, behavior is undefined but constrained by design
#ifndef NDEBUG
    EXPECT_THROW(TestPercent(200), std::out_of_range); // Use clearly invalid value
    EXPECT_THROW(TestPercent(101), std::out_of_range);
#endif
}

TEST_F(RangedIntegerTest, CopyConstructorAndAssignment) {
    TestPercent original(75);
    TestPercent copy1(original);
    TestPercent copy2 = original;

    EXPECT_EQ(copy1.value(), 75);
    EXPECT_EQ(copy2.value(), 75);
    EXPECT_EQ(original, copy1);
    EXPECT_EQ(original, copy2);
}

TEST_F(RangedIntegerTest, MoveConstructorAndAssignment) {
    TestPercent original(75);
    TestPercent moved1(std::move(original));

    TestPercent another(80);
    TestPercent moved2 = std::move(another);

    EXPECT_EQ(moved1.value(), 75);
    EXPECT_EQ(moved2.value(), 80);
}

// ============================================================================
// Type Safety Tests
// ============================================================================

TEST_F(RangedIntegerTest, TypeSafety) {
    using Type1 = RangedInteger<uint8_t, 0, 100, TestTag1>;
    using Type2 = RangedInteger<uint8_t, 0, 100, TestTag2>;

    // These should be different types even with same range
    static_assert(!std::is_same_v<Type1, Type2>);

    Type1 val1(50);
    Type2 val2(50);

    // Should not be able to compare different tagged types directly
    // This should not compile (comment out to test):
    // bool result = (val1 == val2);  // Compilation error expected

    // Suppress unused variable warnings
    (void)val1;
    (void)val2;
}

TEST_F(RangedIntegerTest, StaticConstants) {
    static_assert(TestPercent::MIN_VALUE == 0);
    static_assert(TestPercent::MAX_VALUE == 100);

    static_assert(TestTemperature::MIN_VALUE == -20);
    static_assert(TestTemperature::MAX_VALUE == 50);

    EXPECT_EQ(TestPercent::MIN_VALUE, 0);
    EXPECT_EQ(TestPercent::MAX_VALUE, 100);
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(RangedIntegerTest, Addition) {
    TestPercent a(30);
    TestPercent b(20);

    TestPercent result = a + b;
    EXPECT_EQ(result.value(), 50);

    // Addition with raw value (explicit cast to avoid ambiguity)
    TestPercent result2 = a + static_cast<uint8_t>(25);
    EXPECT_EQ(result2.value(), 55);

    TestPercent result3 = static_cast<uint8_t>(15) + a;
    EXPECT_EQ(result3.value(), 45);
}

TEST_F(RangedIntegerTest, AdditionOverflow) {
    TestPercent a(90);
    TestPercent b(20);

    // This would overflow the range, behavior depends on debug/release
#ifndef NDEBUG
    EXPECT_THROW(a + b, std::out_of_range);
#else
    // In release mode, just verify the values were created properly
    EXPECT_EQ(a.value(), 90);
    EXPECT_EQ(b.value(), 20);
#endif
}

TEST_F(RangedIntegerTest, Subtraction) {
    TestPercent a(70);
    TestPercent b(30);

    TestPercent result = a - b;
    EXPECT_EQ(result.value(), 40);

    // Subtraction with raw value (explicit cast to avoid ambiguity)
    TestPercent result2 = a - static_cast<uint8_t>(25);
    EXPECT_EQ(result2.value(), 45);
}

TEST_F(RangedIntegerTest, Multiplication) {
    TestPercent a(10);
    TestPercent b(5);

    TestPercent result = a * b;
    EXPECT_EQ(result.value(), 50);

    // Multiplication with raw value (explicit cast to avoid ambiguity)
    TestPercent result2 = a * static_cast<uint8_t>(6);
    EXPECT_EQ(result2.value(), 60);
}

TEST_F(RangedIntegerTest, Division) {
    TestPercent a(80);
    TestPercent b(4);

    TestPercent result = a / b;
    EXPECT_EQ(result.value(), 20);

    // Division with raw value (explicit cast to avoid ambiguity)
    TestPercent result2 = a / static_cast<uint8_t>(2);
    EXPECT_EQ(result2.value(), 40);
}

TEST_F(RangedIntegerTest, Modulo) {
    TestPercent a(85);
    TestPercent b(7);

    TestPercent result = a % b;
    EXPECT_EQ(result.value(), 1); // 85 % 7 = 1

    // Modulo with raw value (explicit cast to avoid ambiguity)
    TestPercent result2 = a % static_cast<uint8_t>(10);
    EXPECT_EQ(result2.value(), 5); // 85 % 10 = 5
}

// ============================================================================
// Compound Assignment Tests
// ============================================================================

TEST_F(RangedIntegerTest, CompoundAssignment) {
    TestPercent a(30);

    a += TestPercent(20);
    EXPECT_EQ(a.value(), 50);

    a -= static_cast<uint8_t>(10);
    EXPECT_EQ(a.value(), 40);

    a *= static_cast<uint8_t>(2);
    EXPECT_EQ(a.value(), 80);

    a /= static_cast<uint8_t>(4);
    EXPECT_EQ(a.value(), 20);

    a %= static_cast<uint8_t>(7);
    EXPECT_EQ(a.value(), 6);
}

// ============================================================================
// Increment/Decrement Tests
// ============================================================================

TEST_F(RangedIntegerTest, IncrementDecrement) {
    TestPercent a(50);

    // Pre-increment
    ++a;
    EXPECT_EQ(a.value(), 51);

    // Post-increment
    TestPercent b = a++;
    EXPECT_EQ(b.value(), 51);
    EXPECT_EQ(a.value(), 52);

    // Pre-decrement
    --a;
    EXPECT_EQ(a.value(), 51);

    // Post-decrement
    TestPercent c = a--;
    EXPECT_EQ(c.value(), 51);
    EXPECT_EQ(a.value(), 50);
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(RangedIntegerTest, ComparisonOperators) {
    TestPercent a(30);
    TestPercent b(50);
    TestPercent c(30);

    // Equality
    EXPECT_TRUE(a == c);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a != c);
    EXPECT_TRUE(a != b);

    // Ordering
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b <= a);

    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(c >= a);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
}

TEST_F(RangedIntegerTest, ComparisonWithRawValues) {
    TestPercent a(75);

    EXPECT_TRUE(a == static_cast<uint8_t>(75));
    EXPECT_FALSE(a == static_cast<uint8_t>(50));
    EXPECT_TRUE(a != static_cast<uint8_t>(50));
    EXPECT_FALSE(a != static_cast<uint8_t>(75));

    EXPECT_TRUE(a < static_cast<uint8_t>(100));
    EXPECT_TRUE(a <= static_cast<uint8_t>(75));
    EXPECT_TRUE(a > static_cast<uint8_t>(50));
    EXPECT_TRUE(a >= static_cast<uint8_t>(75));

    // Symmetric comparisons
    EXPECT_TRUE(static_cast<uint8_t>(75) == a);
    EXPECT_TRUE(static_cast<uint8_t>(50) < a);
    EXPECT_TRUE(static_cast<uint8_t>(100) > a);
}

// ============================================================================
// Utility Methods Tests
// ============================================================================

TEST_F(RangedIntegerTest, UtilityMethods) {
    TestPercent min_val(0);
    TestPercent max_val(100);
    TestPercent mid_val(50);

    EXPECT_TRUE(min_val.at_min());
    EXPECT_FALSE(min_val.at_max());

    EXPECT_FALSE(max_val.at_min());
    EXPECT_TRUE(max_val.at_max());

    EXPECT_FALSE(mid_val.at_min());
    EXPECT_FALSE(mid_val.at_max());
}

TEST_F(RangedIntegerTest, ValidationMethods) {
    EXPECT_TRUE(TestPercent::is_valid(0));
    EXPECT_TRUE(TestPercent::is_valid(50));
    EXPECT_TRUE(TestPercent::is_valid(100));

    EXPECT_FALSE(TestPercent::is_valid(101));
    EXPECT_FALSE(TestPercent::is_valid(255));
}

TEST_F(RangedIntegerTest, ImplicitConversion) {
    TestPercent percent(75);

    // Implicit conversion to underlying type
    uint8_t raw_value = percent;
    EXPECT_EQ(raw_value, 75);

    // Use in expressions requiring underlying type
    int sum = static_cast<uint8_t>(percent) + 25; // Explicit conversion to avoid ambiguity
    EXPECT_EQ(sum, 100);
}

// ============================================================================
// Constexpr Tests
// ============================================================================

TEST_F(RangedIntegerTest, ConstexprSupport) {
    // These should all be evaluable at compile time
    constexpr TestPercent CONST_PERCENT(42);
    constexpr uint8_t CONST_VALUE = CONST_PERCENT.value();
    constexpr bool IS_MIN = CONST_PERCENT.at_min();
    constexpr bool IS_MAX = CONST_PERCENT.at_max();
    constexpr bool IS_VALID = TestPercent::is_valid(42);

    static_assert(CONST_VALUE == 42);
    static_assert(!IS_MIN);
    static_assert(!IS_MAX);
    static_assert(IS_VALID);

    EXPECT_EQ(CONST_VALUE, 42);
    EXPECT_FALSE(IS_MIN);
    EXPECT_FALSE(IS_MAX);
    EXPECT_TRUE(IS_VALID);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(RangedIntegerTest, NegativeRanges) {
    TestTemperature temp(-10);
    EXPECT_EQ(temp.value(), -10);

    TestTemperature temp_min(-20);
    TestTemperature temp_max(50);

    EXPECT_TRUE(temp_min.at_min());
    EXPECT_TRUE(temp_max.at_max());
    EXPECT_FALSE(temp.at_min());
    EXPECT_FALSE(temp.at_max());
}

TEST_F(RangedIntegerTest, SingleValueRange) {
    using SingleValue = RangedInteger<uint8_t, 42, 42, struct SingleTag>;

    SingleValue val; // Should default to 42
    EXPECT_EQ(val.value(), 42);
    EXPECT_TRUE(val.at_min());
    EXPECT_TRUE(val.at_max());

    SingleValue val2(42);
    EXPECT_EQ(val2.value(), 42);

#ifndef NDEBUG
    EXPECT_THROW(SingleValue(41), std::out_of_range);
    EXPECT_THROW(SingleValue(43), std::out_of_range);
#endif
}

// ============================================================================
// Performance Tests (compile-time checks)
// ============================================================================

TEST_F(RangedIntegerTest, ZeroOverheadAbstraction) {
    // Ensure the wrapper doesn't add runtime overhead
    static_assert(sizeof(TestPercent) == sizeof(uint8_t));
    static_assert(std::is_trivially_copyable_v<TestPercent>);
    static_assert(std::is_standard_layout_v<TestPercent>);

    EXPECT_EQ(sizeof(TestPercent), sizeof(uint8_t));
}

// ============================================================================
// Integration Tests with Real HM11 Types
// ============================================================================

TEST_F(RangedIntegerTest, HM11TypeAliases) {
    // Test actual HM11 type aliases work correctly
    Percent battery(85);
    EXPECT_EQ(battery.value(), 85);

    TemperatureType temp(25);
    EXPECT_EQ(temp.value(), 25);

    HumidityType humidity(60);
    EXPECT_EQ(humidity.value(), 60);

    // Ensure different types can't be mixed
    static_assert(!std::is_same_v<Percent, TemperatureType>);
    static_assert(!std::is_same_v<TemperatureType, HumidityType>);

    // This should not compile (different semantic types):
    // bool mixed = (battery == temp);  // Compilation error expected
}

// Tests will be run by gtest_main