/**
 * @file hal_test.cpp
 * @brief Unit tests for the HAL interface
 *
 * This file is part of the HM11 driver C++ migration project.
 * It tests the HAL interface implementations.
 */

#include <gtest/gtest.h>

#include "hm11/hal/hal.hpp"
#include "hm11/hal/uart.hpp"

// Test fixture for HAL tests
class HalTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test basic HAL types
TEST_F(HalTest, BasicHalTypes) {
    // Verify that the HAL types match the expected sizes
    EXPECT_EQ(sizeof(hm11::hal::uint8_t), 1);
    EXPECT_EQ(sizeof(hm11::hal::uint16_t), 2);
    EXPECT_EQ(sizeof(hm11::hal::uint32_t), 4);
    EXPECT_EQ(sizeof(hm11::hal::uint64_t), 8);
}

// Test HAL array types
TEST_F(HalTest, HalArrayTypes) {
    // Create array instances and verify their sizes
    hm11::hal::uint8_array<10> u8arr;
    EXPECT_EQ(u8arr.size(), 10);

    hm11::hal::uint16_array<5> u16arr;
    EXPECT_EQ(u16arr.size(), 5);

    hm11::hal::uint32_array<3> u32arr;
    EXPECT_EQ(u32arr.size(), 3);
}

// More tests to be added as implementation progresses