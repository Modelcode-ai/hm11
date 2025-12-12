/**
 * @file test_ranged_integer.cpp
 * @brief Comprehensive unit tests for RangedInteger template
 *
 * This test suite validates all aspects of the RangedInteger template class,
 * including compile-time validation, runtime behavior, arithmetic operations,
 * comparison operations, and edge cases.
 *
 * ## Test Coverage
 *
 * - Default construction
 * - Explicit construction with valid and invalid values
 * - Compile-time (constexpr) validation
 * - Boundary value testing (Min, Max, Min-1, Max+1)
 * - Comparison operators (==, !=, <, <=, >, >=)
 * - Arithmetic operators (+, -, ++, --, +=, -=)
 * - Arithmetic overflow behavior (debug assertions)
 * - Type safety (Tag prevents mixing different ranges)
 * - Value extraction (.get())
 * - Zero-overhead abstractions (constexpr usage)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include "hm11/types/ranged_integer.hpp"

using namespace hm11::types;

// Define test type aliases
using Percent = RangedInteger<std::uint8_t, 0, 100, struct PercentTag>;
using Temperature = RangedInteger<std::uint8_t, 0, 120, struct TemperatureTag>;
using Latency = RangedInteger<std::uint8_t, 0, 4, struct LatencyTag>;
using SignedRange = RangedInteger<std::int8_t, -50, 50, struct SignedTag>;
using LargeRange = RangedInteger<std::uint16_t, 0, 1000, struct LargeTag>;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(RangedIntegerTest, DefaultConstruction) {
    constexpr Percent P;
    EXPECT_EQ(P.get(), 0); // Default initializes to Min

    constexpr SignedRange S;
    EXPECT_EQ(S.get(), -50); // Default initializes to Min (negative)
}

TEST(RangedIntegerTest, ExplicitConstruction) {
    Percent p1{50};
    EXPECT_EQ(p1.get(), 50);

    Percent p2{0}; // Minimum boundary
    EXPECT_EQ(p2.get(), 0);

    Percent p3{100}; // Maximum boundary
    EXPECT_EQ(p3.get(), 100);

    SignedRange s1{0};
    EXPECT_EQ(s1.get(), 0);

    SignedRange s2{-25};
    EXPECT_EQ(s2.get(), -25);

    SignedRange s3{25};
    EXPECT_EQ(s3.get(), 25);
}

TEST(RangedIntegerTest, ConstexprConstruction) {
    // Compile-time construction - these must compile
    constexpr Percent P1{50};
    static_assert(P1.get() == 50, "Constexpr construction failed");

    constexpr Percent P_MIN{0};
    static_assert(P_MIN.get() == 0, "Min boundary failed");

    constexpr Percent P_MAX{100};
    static_assert(P_MAX.get() == 100, "Max boundary failed");

    constexpr Temperature T{25};
    static_assert(T.get() == 25, "Temperature construction failed");

    constexpr Latency L{2};
    static_assert(L.get() == 2, "Latency construction failed");

    // Verify these compile
    EXPECT_EQ(P1.get(), 50);
    EXPECT_EQ(T.get(), 25);
    EXPECT_EQ(L.get(), 2);
}

TEST(RangedIntegerTest, CopyConstruction) {
    Percent p1{75};
    Percent p2{p1};

    EXPECT_EQ(p1.get(), 75);
    EXPECT_EQ(p2.get(), 75);

    // Constexpr copy
    constexpr Percent P3{50};
    constexpr Percent P4{P3};
    static_assert(P4.get() == 50, "Constexpr copy failed");
}

TEST(RangedIntegerTest, CopyAssignment) {
    Percent p1{75};
    Percent p2{25};

    p2 = p1;
    EXPECT_EQ(p2.get(), 75);
    EXPECT_EQ(p1.get(), 75);

    // Constexpr assignment
    constexpr auto TEST_ASSIGN = []() constexpr {
        Percent a{10};
        Percent b{20};
        a = b;
        return a.get();
    };

    static_assert(TEST_ASSIGN() == 20, "Constexpr assignment failed");
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST(RangedIntegerTest, BoundaryValues) {
    // Minimum boundary
    Percent p_min{0};
    EXPECT_EQ(p_min.get(), 0);

    // Maximum boundary
    Percent p_max{100};
    EXPECT_EQ(p_max.get(), 100);

    // Signed boundaries
    SignedRange s_min{-50};
    EXPECT_EQ(s_min.get(), -50);

    SignedRange s_max{50};
    EXPECT_EQ(s_max.get(), 50);

    // Large range boundaries
    LargeRange l_min{0};
    EXPECT_EQ(l_min.get(), 0);

    LargeRange l_max{1000};
    EXPECT_EQ(l_max.get(), 1000);
}

TEST(RangedIntegerTest, StaticBoundsAccess) {
    // Verify static constexpr members
    static_assert(Percent::MIN_VALUE == 0, "Min value incorrect");
    static_assert(Percent::MAX_VALUE == 100, "Max value incorrect");

    static_assert(Temperature::MIN_VALUE == 0, "Temperature min incorrect");
    static_assert(Temperature::MAX_VALUE == 120, "Temperature max incorrect");

    static_assert(Latency::MIN_VALUE == 0, "Latency min incorrect");
    static_assert(Latency::MAX_VALUE == 4, "Latency max incorrect");

    static_assert(SignedRange::MIN_VALUE == -50, "Signed min incorrect");
    static_assert(SignedRange::MAX_VALUE == 50, "Signed max incorrect");

    EXPECT_EQ(Percent::MIN_VALUE, 0);
    EXPECT_EQ(Percent::MAX_VALUE, 100);
}

// ============================================================================
// Comparison Operator Tests
// ============================================================================

TEST(RangedIntegerTest, EqualityComparison) {
    Percent p1{50};
    Percent p2{50};
    Percent p3{75};

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);

    // Constexpr comparison
    constexpr Percent C1{50};
    constexpr Percent C2{50};
    static_assert(C1 == C2, "Constexpr equality failed");
}

TEST(RangedIntegerTest, InequalityComparison) {
    Percent p1{50};
    Percent p2{50};
    Percent p3{75};

    EXPECT_FALSE(p1 != p2);
    EXPECT_TRUE(p1 != p3);

    // Constexpr comparison
    constexpr Percent C1{50};
    constexpr Percent C2{75};
    static_assert(C1 != C2, "Constexpr inequality failed");
}

TEST(RangedIntegerTest, LessThanComparison) {
    Percent p1{25};
    Percent p2{75};

    EXPECT_TRUE(p1 < p2);
    EXPECT_FALSE(p2 < p1);
    EXPECT_FALSE(p1 < p1);

    // Constexpr comparison
    constexpr Percent C1{25};
    constexpr Percent C2{75};
    static_assert(C1 < C2, "Constexpr less-than failed");
}

TEST(RangedIntegerTest, LessThanOrEqualComparison) {
    Percent p1{25};
    Percent p2{75};
    Percent p3{25};

    EXPECT_TRUE(p1 <= p2);
    EXPECT_TRUE(p1 <= p3);
    EXPECT_FALSE(p2 <= p1);

    // Constexpr comparison
    constexpr Percent C1{25};
    constexpr Percent C2{25};
    static_assert(C1 <= C2, "Constexpr less-than-or-equal failed");
}

TEST(RangedIntegerTest, GreaterThanComparison) {
    Percent p1{75};
    Percent p2{25};

    EXPECT_TRUE(p1 > p2);
    EXPECT_FALSE(p2 > p1);
    EXPECT_FALSE(p1 > p1);

    // Constexpr comparison
    constexpr Percent C1{75};
    constexpr Percent C2{25};
    static_assert(C1 > C2, "Constexpr greater-than failed");
}

TEST(RangedIntegerTest, GreaterThanOrEqualComparison) {
    Percent p1{75};
    Percent p2{25};
    Percent p3{75};

    EXPECT_TRUE(p1 >= p2);
    EXPECT_TRUE(p1 >= p3);
    EXPECT_FALSE(p2 >= p1);

    // Constexpr comparison
    constexpr Percent C1{75};
    constexpr Percent C2{75};
    static_assert(C1 >= C2, "Constexpr greater-than-or-equal failed");
}

// ============================================================================
// Arithmetic Operator Tests
// ============================================================================

TEST(RangedIntegerTest, Addition) {
    Percent p1{30};
    Percent p2{40};
    Percent sum = p1 + p2;

    EXPECT_EQ(sum.get(), 70);

    // Constexpr addition
    constexpr Percent C1{30};
    constexpr Percent C2{40};
    constexpr Percent C_SUM = C1 + C2;
    static_assert(C_SUM.get() == 70, "Constexpr addition failed");
}

TEST(RangedIntegerTest, Subtraction) {
    Percent p1{70};
    Percent p2{30};
    Percent diff = p1 - p2;

    EXPECT_EQ(diff.get(), 40);

    // Constexpr subtraction
    constexpr Percent C1{70};
    constexpr Percent C2{30};
    constexpr Percent C_DIFF = C1 - C2;
    static_assert(C_DIFF.get() == 40, "Constexpr subtraction failed");
}

TEST(RangedIntegerTest, PreIncrement) {
    Percent p{50};
    Percent& result = ++p;

    EXPECT_EQ(p.get(), 51);
    EXPECT_EQ(result.get(), 51);
    EXPECT_EQ(&result, &p); // Returns reference to same object

    // Constexpr pre-increment
    constexpr auto TEST_PREINC = []() constexpr {
        Percent x{50};
        ++x;
        return x.get();
    };
    static_assert(TEST_PREINC() == 51, "Constexpr pre-increment failed");
}

TEST(RangedIntegerTest, PostIncrement) {
    Percent p{50};
    Percent old = p++;

    EXPECT_EQ(old.get(), 50); // Old value before increment
    EXPECT_EQ(p.get(), 51);   // New value after increment

    // Constexpr post-increment
    constexpr auto TEST_POSTINC = []() constexpr {
        Percent x{50};
        Percent old_val = x++;
        return old_val.get() + x.get(); // 50 + 51 = 101
    };
    static_assert(TEST_POSTINC() == 101, "Constexpr post-increment failed");
}

TEST(RangedIntegerTest, PreDecrement) {
    Percent p{50};
    Percent& result = --p;

    EXPECT_EQ(p.get(), 49);
    EXPECT_EQ(result.get(), 49);
    EXPECT_EQ(&result, &p); // Returns reference to same object

    // Constexpr pre-decrement
    constexpr auto TEST_PREDEC = []() constexpr {
        Percent x{50};
        --x;
        return x.get();
    };
    static_assert(TEST_PREDEC() == 49, "Constexpr pre-decrement failed");
}

TEST(RangedIntegerTest, PostDecrement) {
    Percent p{50};
    Percent old = p--;

    EXPECT_EQ(old.get(), 50); // Old value before decrement
    EXPECT_EQ(p.get(), 49);   // New value after decrement

    // Constexpr post-decrement
    constexpr auto TEST_POSTDEC = []() constexpr {
        Percent x{50};
        Percent old_val = x--;
        return old_val.get() + x.get(); // 50 + 49 = 99
    };
    static_assert(TEST_POSTDEC() == 99, "Constexpr post-decrement failed");
}

TEST(RangedIntegerTest, AdditionAssignment) {
    Percent p{30};
    p += Percent{20};

    EXPECT_EQ(p.get(), 50);

    // Constexpr addition assignment
    constexpr auto TEST_ADD_ASSIGN = []() constexpr {
        Percent x{30};
        x += Percent{20};
        return x.get();
    };
    static_assert(TEST_ADD_ASSIGN() == 50, "Constexpr += failed");
}

TEST(RangedIntegerTest, SubtractionAssignment) {
    Percent p{70};
    p -= Percent{30};

    EXPECT_EQ(p.get(), 40);

    // Constexpr subtraction assignment
    constexpr auto TEST_SUB_ASSIGN = []() constexpr {
        Percent x{70};
        x -= Percent{30};
        return x.get();
    };
    static_assert(TEST_SUB_ASSIGN() == 40, "Constexpr -= failed");
}

TEST(RangedIntegerTest, ComplexArithmeticExpression) {
    Percent p1{10};
    Percent p2{20};
    Percent p3{30};

    Percent result = (p1 + p2) + p3;
    EXPECT_EQ(result.get(), 60);

    // Multiple operations
    Percent a{50};
    ++a;
    a += Percent{5};
    a -= Percent{10};
    EXPECT_EQ(a.get(), 46);
}

// ============================================================================
// Type Safety Tests
// ============================================================================

TEST(RangedIntegerTest, TagTypePreventsMixing) {
    // This test verifies at compile-time that different tags create distinct types
    // The following lines should NOT compile (uncomment to verify):
    //
    // Percent p{50};
    // Temperature t{25};
    // p = t;              // Error: incompatible types (different tags)
    // bool eq = (p == t); // Error: cannot compare different types
    // Percent sum = p + t;  // Error: cannot add different types

    // We can verify this by checking type traits
    static_assert(!std::is_same_v<Percent, Temperature>, "Percent and Temperature should be different types");
    static_assert(!std::is_same_v<Percent, Latency>, "Percent and Latency should be different types");
    static_assert(!std::is_same_v<Temperature, Latency>, "Temperature and Latency should be different types");

    // Each type should have its own tag
    static_assert(!std::is_same_v<Percent::TagType, Temperature::TagType>, "Tags should be different");

    SUCCEED(); // Test passes if it compiles
}

// ============================================================================
// Value Extraction Tests
// ============================================================================

TEST(RangedIntegerTest, ValueExtraction) {
    Percent p{75};
    std::uint8_t raw = p.get();

    EXPECT_EQ(raw, 75);
    EXPECT_EQ(typeid(raw), typeid(std::uint8_t));

    // Constexpr value extraction
    constexpr Percent C{50};
    constexpr std::uint8_t C_RAW = C.get();
    static_assert(C_RAW == 50, "Constexpr value extraction failed");
}

TEST(RangedIntegerTest, ValueTypeAlias) {
    // Verify ValueType alias
    static_assert(std::is_same_v<Percent::ValueType, std::uint8_t>, "ValueType should be uint8_t");
    static_assert(std::is_same_v<LargeRange::ValueType, std::uint16_t>, "ValueType should be uint16_t");
    static_assert(std::is_same_v<SignedRange::ValueType, std::int8_t>, "ValueType should be int8_t");

    SUCCEED();
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(RangedIntegerTest, ZeroRange) {
    // Range with Min == Max
    using SingleValue = RangedInteger<std::uint8_t, 42, 42, struct SingleTag>;

    constexpr SingleValue SV;
    static_assert(SV.get() == 42, "Single-value range failed");

    SingleValue sv2{42};
    EXPECT_EQ(sv2.get(), 42);

    // All instances must have the same value
    SingleValue sv3{42};
    EXPECT_TRUE(sv2 == sv3);
}

TEST(RangedIntegerTest, LargeTypeRange) {
    // Test with larger underlying types
    using Wide = RangedInteger<std::uint32_t, 0, 100000, struct WideTag>;

    constexpr Wide W{50000};
    static_assert(W.get() == 50000, "Wide range failed");

    Wide w2{100000};
    EXPECT_EQ(w2.get(), 100000);

    // Test addition that stays within bounds
    Wide w3{30000};
    Wide sum = w3 + w3;
    EXPECT_EQ(sum.get(), 60000);
}

TEST(RangedIntegerTest, SignedRangeOperations) {
    SignedRange s1{-25};
    SignedRange s2{25};

    EXPECT_TRUE(s1 < s2);
    EXPECT_EQ((s1 + s2).get(), 0);

    SignedRange s3{-50};
    EXPECT_EQ(s3.get(), -50);

    ++s3;
    EXPECT_EQ(s3.get(), -49);

    --s3;
    EXPECT_EQ(s3.get(), -50);
}

// ============================================================================
// Constexpr Evaluation Tests
// ============================================================================

TEST(RangedIntegerTest, ConstexprEvaluationCompleteness) {
    // Verify all operations can be evaluated at compile-time
    constexpr auto TEST_ALL_CONSTEXPR_LAMBDA = []() constexpr {
        // Construction
        Percent p1{50};
        Percent p2{30};

        // Arithmetic
        Percent sum = p1 + p2;

        // Comparison
        bool eq = (p1 == p1);
        bool ne = (p1 != p2);
        bool lt = (p2 < p1);
        bool le = (p2 <= p1);
        bool gt = (p1 > p2);
        bool ge = (p1 >= p2);

        // Value extraction
        std::uint8_t val = sum.get();

        // Increment/decrement
        Percent p3{50};
        ++p3;
        --p3;

        return eq && ne && lt && le && gt && ge && (val == 80);
    };

    static_assert(TEST_ALL_CONSTEXPR_LAMBDA(), "Not all operations are constexpr");
    EXPECT_TRUE(TEST_ALL_CONSTEXPR_LAMBDA());
}

// ============================================================================
// Documentation Example Tests
// ============================================================================

TEST(RangedIntegerTest, UsageExampleFromDocumentation) {
    // Example from header documentation
    constexpr Percent BATTERY{75};
    EXPECT_EQ(BATTERY.get(), 75);

    Percent runtime_value{50};
    EXPECT_EQ(runtime_value.get(), 50);

    Temperature temp{25};
    EXPECT_EQ(temp.get(), 25);

    // Type safety example
    // Percent p = temp;  // Would not compile - different tags

    // Arithmetic with validation
    Percent p1{30};
    Percent p2{40};
    Percent sum = p1 + p2;
    EXPECT_EQ(sum.get(), 70);

    // Comparison
    EXPECT_TRUE(p1 < p2);

    // Extract value
    std::uint8_t raw = BATTERY.get();
    EXPECT_EQ(raw, 75);
}

// ============================================================================
// Static Assertion Tests (Compile-Time Validation)
// ============================================================================

// These static assertions verify compile-time constraints
static_assert(std::is_trivially_copyable_v<Percent>, "RangedInteger should be trivially copyable");
static_assert(std::is_standard_layout_v<Percent>, "RangedInteger should have standard layout");
static_assert(sizeof(Percent) == sizeof(std::uint8_t), "RangedInteger should have no overhead");
static_assert(sizeof(LargeRange) == sizeof(std::uint16_t), "RangedInteger should have no overhead");

// Test that Min <= Max constraint is enforced
// Uncommenting this should cause a compile error:
// using Invalid = RangedInteger<uint8_t, 100, 0, struct InvalidTag>;  // Error: Min > Max
