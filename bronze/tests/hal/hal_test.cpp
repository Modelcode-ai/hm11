/**
 * @file hal_test.cpp
 * @brief Unit tests for HAL types
 *
 * This file contains unit tests for the HAL types defined in types.hpp.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include "hm11/hal/types.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for HAL type tests
 */
class HALTypesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No specific setup needed
    }

    void TearDown() override {
        // No specific teardown needed
    }
};

// Test UInt8 type
TEST_F(HALTypesTest, UInt8Type) {
    hal::UInt8 value = 255;
    EXPECT_EQ(value, 255);
    EXPECT_EQ(sizeof(hal::UInt8), 1);
}

// Test UInt16 type
TEST_F(HALTypesTest, UInt16Type) {
    hal::UInt16 value = 65535;
    EXPECT_EQ(value, 65535);
    EXPECT_EQ(sizeof(hal::UInt16), 2);
}

// Test UInt32 type
TEST_F(HALTypesTest, UInt32Type) {
    hal::UInt32 value = 4294967295;
    EXPECT_EQ(value, 4294967295);
    EXPECT_EQ(sizeof(hal::UInt32), 4);
}

// Test UInt8Span type
TEST_F(HALTypesTest, UInt8SpanType) {
    hal::UInt8 buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    hal::UInt8Span span(buffer, 10);

    EXPECT_EQ(span.size(), 10);
    EXPECT_EQ(span[0], 0);
    EXPECT_EQ(span[9], 9);
}

// Test UInt16Span type
TEST_F(HALTypesTest, UInt16SpanType) {
    hal::UInt16 buffer[5] = {10, 20, 30, 40, 50};
    hal::UInt16Span span(buffer, 5);

    EXPECT_EQ(span.size(), 5);
    EXPECT_EQ(span[0], 10);
    EXPECT_EQ(span[4], 50);
}

} // namespace hm11::tests
