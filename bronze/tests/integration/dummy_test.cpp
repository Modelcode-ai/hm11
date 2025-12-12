/**
 * @file dummy_test.cpp
 * @brief Dummy test file to verify testing environment
 *
 * This file contains dummy test functions that will always pass.
 */

#include <gtest/gtest.h>

// Simple test that always passes
TEST(DummyTest, AlwaysPass) {
    EXPECT_TRUE(true);
}

// Another simple test that always passes
TEST(DummyTest, AnotherTest) {
    EXPECT_EQ(1, 1);
    EXPECT_NE(1, 2);
}