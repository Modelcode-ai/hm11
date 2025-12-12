/**
 * @file types_test.cpp
 * @brief Unit tests for HAL type definitions
 *
 * This file contains unit tests for the HAL type definitions in types.hpp.
 * Tests ensure that types have correct sizes, ranges, and behaviors.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "hm11/hal/types.hpp"

namespace {

using namespace hm11::hal;

TEST(HALTypesTest, IntegerTypesHaveCorrectSize) {
    EXPECT_EQ(sizeof(UInt8), 1u);
    EXPECT_EQ(sizeof(UInt16), 2u);
    EXPECT_EQ(sizeof(UInt32), 4u);
    EXPECT_EQ(sizeof(UInt64), 8u);

    // UInt9 uses UInt16 as its underlying type
    EXPECT_EQ(sizeof(UInt9), 2u);
}

TEST(HALTypesTest, ArrayTypesWork) {
    // Test fixed-size array
    Array<UInt8, 4> arr1 = {1, 2, 3, 4};
    EXPECT_EQ(arr1.size(), 4u);
    EXPECT_EQ(arr1[0], 1);
    EXPECT_EQ(arr1[3], 4);

    // Test UIntX_Array types
    UInt8_Array arr8 = {5, 6, 7, 8};
    EXPECT_EQ(arr8.size(), 4u);
    EXPECT_EQ(arr8[0], 5);
    EXPECT_EQ(arr8[3], 8);

    UInt16_Array arr16 = {0x1234, 0x5678};
    EXPECT_EQ(arr16.size(), 2u);
    EXPECT_EQ(arr16[0], 0x1234);
    EXPECT_EQ(arr16[1], 0x5678);

    UInt32_Array arr32 = {0x12345678, 0x87654321};
    EXPECT_EQ(arr32.size(), 2u);
    EXPECT_EQ(arr32[0], 0x12345678u);
    EXPECT_EQ(arr32[1], 0x87654321u);
}

TEST(HALTypesTest, SpanTypesWork) {
    // Create arrays for testing spans
    std::array<UInt8, 4> data8 = {1, 2, 3, 4};
    std::array<UInt16, 3> data16 = {0x1111, 0x2222, 0x3333};
    std::array<UInt32, 2> data32 = {0x11111111, 0x22222222};

    // Test UInt8_Span
    UInt8_Span span8(data8);
    EXPECT_EQ(span8.size(), 4u);
    EXPECT_EQ(span8[0], 1);
    EXPECT_EQ(span8[3], 4);

    // Test UInt16_Span
    UInt16_Span span16(data16);
    EXPECT_EQ(span16.size(), 3u);
    EXPECT_EQ(span16[0], 0x1111);
    EXPECT_EQ(span16[2], 0x3333);

    // Test UInt32_Span
    UInt32_Span span32(data32);
    EXPECT_EQ(span32.size(), 2u);
    EXPECT_EQ(span32[0], 0x11111111u);
    EXPECT_EQ(span32[1], 0x22222222u);
}

TEST(HALTypesTest, ConstSpanTypesWork) {
    // Create const arrays for testing const spans
    const std::array<UInt8, 4> data8 = {1, 2, 3, 4};
    const std::array<UInt16, 3> data16 = {0x1111, 0x2222, 0x3333};
    const std::array<UInt32, 2> data32 = {0x11111111, 0x22222222};

    // Test UInt8_ConstSpan
    UInt8_ConstSpan span8(data8);
    EXPECT_EQ(span8.size(), 4u);
    EXPECT_EQ(span8[0], 1);
    EXPECT_EQ(span8[3], 4);

    // Test UInt16_ConstSpan
    UInt16_ConstSpan span16(data16);
    EXPECT_EQ(span16.size(), 3u);
    EXPECT_EQ(span16[0], 0x1111);
    EXPECT_EQ(span16[2], 0x3333);

    // Test UInt32_ConstSpan
    UInt32_ConstSpan span32(data32);
    EXPECT_EQ(span32.size(), 2u);
    EXPECT_EQ(span32[0], 0x11111111u);
    EXPECT_EQ(span32[1], 0x22222222u);
}

} // namespace