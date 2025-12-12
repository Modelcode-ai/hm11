#include <gtest/gtest.h>

#include "hm11/util/type_traits.hpp"

using namespace hm11::util;

// Helper macro for death tests (expects assertion failure)
#define EXPECT_ASSERT_DEATH(statement)                                                                                 \
    EXPECT_DEATH({ statement; }, ".*") // NOLINT(readability-macro-usage, cppcoreguidelines-macro-usage)

TEST(RangedIntegerInvalidTest, OutOfRangeConstruction) {
    // Values below minimum
    EXPECT_ASSERT_DEATH(Percent p{static_cast<std::uint8_t>(-1)});
    // Values above maximum
    EXPECT_ASSERT_DEATH(Percent p{101});
}

TEST(RangedIntegerInvalidTest, ArithmeticOverflow) {
    Percent a{60};
    Percent b{50};
    // This addition exceeds the maximum (100) and should trigger an assert
    EXPECT_ASSERT_DEATH(
        Percent c [[maybe_unused]] = a + b;); // NOLINT(unused-variable, clang-diagnostic-unused-variable)
}

TEST(FixedStringInvalidTest, WrongLength) {
    // Length not equal to required N (12 for MACAddress)
    EXPECT_ASSERT_DEATH(MacAddress mac{"0123AB"});
}

TEST(FixedStringInvalidTest, InvalidCharacters) {
    // Contains non‑hex characters
    EXPECT_ASSERT_DEATH(MacAddress mac{"GHIJKLmnopqr"});
}
