/**
 * @file minimal_test.cpp
 * @brief Minimal test file for verifying the test setup
 */

#include <gtest/gtest.h>

// Test fixture for minimal tests
class MinimalTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Basic test that always passes
TEST_F(MinimalTest, AlwaysPass) {
    EXPECT_TRUE(true);
}

// Main function
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}