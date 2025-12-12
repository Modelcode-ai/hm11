/**
 * @file test_main.cpp
 * @brief Minimal Google Test setup for HM11 driver project
 *
 * This file provides a basic test suite to verify that the Google Test
 * framework is correctly configured. Additional unit tests are located
 * in other test files for specific driver components.
 */

#include <gtest/gtest.h>

/**
 * @brief Minimal sanity test to verify Google Test is working
 */
TEST(BuildSystemTest, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
}

/**
 * @brief Test C++20 feature availability
 */
TEST(BuildSystemTest, Cpp20Features) {
    // Verify C++20 is enabled
    static_assert(__cplusplus >= 202002L, "C++20 or later required");

    // Test concepts (C++20 feature)
    auto lambda = []<typename T>(T value) {
        return value;
    };
    EXPECT_EQ(lambda(42), 42);
}

/**
 * @brief Test that standard library is available
 */
TEST(BuildSystemTest, StandardLibrary) {
    std::vector<int> vec = {1, 2, 3};
    EXPECT_EQ(vec.size(), 3);
}
