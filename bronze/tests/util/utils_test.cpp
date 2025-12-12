/**
 * @file utils_test.cpp
 * @brief Unit tests for utility functions
 *
 * This file is part of the HM11 driver C++ migration project.
 * It tests the utility functions.
 */

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "hm11/hal/types.hpp"
#include "hm11/util/utils.hpp"

// Test fixture for utility tests
class UtilsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test string utility functions
TEST_F(UtilsTest, StringUtils) {
    // Test starts_with
    std::string_view str = "Hello, world!";
    EXPECT_TRUE(hm11::util::string::starts_with(str, "Hello"));
    EXPECT_FALSE(hm11::util::string::starts_with(str, "World"));
    EXPECT_TRUE(hm11::util::string::starts_with(str, "")); // Empty prefix

    // Test find
    EXPECT_EQ(hm11::util::string::find(str, "world"), 7);
    EXPECT_EQ(hm11::util::string::find(str, "not found"), std::string_view::npos);

    // Test substring
    EXPECT_EQ(hm11::util::string::substring(str, 0, 5), "Hello");
    EXPECT_EQ(hm11::util::string::substring(str, 7, 12), "world");
    EXPECT_EQ(hm11::util::string::substring(str, 100, 105), "");

    // Test find_zero
    std::string str_with_nulls = "Hello\0world";
    EXPECT_EQ(hm11::util::string::find_zero(std::string_view(str_with_nulls.data(), str_with_nulls.size())), 5);
    EXPECT_EQ(hm11::util::string::find_zero(str), str.size()); // No null, returns size

    // Test calc_length
    EXPECT_EQ(hm11::util::string::calc_length(std::string_view(str_with_nulls.data(), str_with_nulls.size())), 5);
    EXPECT_EQ(hm11::util::string::calc_length(str), str.size());
    EXPECT_EQ(hm11::util::string::calc_length(str, 7), 6); // From "world!" to end
}

// Test buffer utility functions
TEST_F(UtilsTest, BufferUtils) {
    // Create a test buffer
    std::array<hm11::hal::UInt8, 10> buffer = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Test starts_with
    std::array<hm11::hal::UInt8, 3> pattern1 = {0, 1, 2};
    std::array<hm11::hal::UInt8, 3> pattern2 = {5, 6, 7};
    std::array<hm11::hal::UInt8, 3> pattern3 = {8, 9, 0};

    EXPECT_TRUE(hm11::util::buffer::starts_with(buffer, pattern1, 0, 10));
    EXPECT_TRUE(hm11::util::buffer::starts_with(buffer, pattern2, 5, 10));
    EXPECT_FALSE(hm11::util::buffer::starts_with(buffer, pattern3, 8, 10));

    // Test wrapped case
    EXPECT_TRUE(hm11::util::buffer::starts_with(buffer, pattern3, 8, 1));

    // Test find
    EXPECT_EQ(hm11::util::buffer::find(buffer, pattern1, 0, 10), 0);
    EXPECT_EQ(hm11::util::buffer::find(buffer, pattern2, 0, 10), 5);

    // Find in wrapped buffer
    std::array<hm11::hal::UInt8, 2> pattern_wrap = {9, 0};
    EXPECT_EQ(hm11::util::buffer::find(buffer, pattern_wrap, 9, 1), 9);

    // Test calc_length
    EXPECT_EQ(hm11::util::buffer::calc_length(buffer, 3, 7), 4);
    EXPECT_EQ(hm11::util::buffer::calc_length(buffer, 7, 3), 6); // Wrapped case
    EXPECT_EQ(hm11::util::buffer::calc_length(buffer, 5, 5), 0); // Empty range

    // Test move
    size_t pos = 3;
    size_t length = 0;
    hm11::util::buffer::move(buffer, pos, 2, 8, length);
    EXPECT_EQ(pos, 5);
    EXPECT_EQ(length, 3);

    // Test move with wrap
    pos = 8;
    length = 0;
    hm11::util::buffer::move(buffer, pos, 3, 5, length);
    EXPECT_EQ(pos, 1);    // Wrapped to position 1
    EXPECT_EQ(length, 4); // Length from 1 to 5

    // Test copy
    std::array<hm11::hal::UInt8, 4> dest1{};
    EXPECT_EQ(hm11::util::buffer::copy(buffer, dest1, 3), 4);
    EXPECT_EQ(dest1[0], 3);
    EXPECT_EQ(dest1[3], 6);

    // Test copy with wrap
    std::array<hm11::hal::UInt8, 3> dest2{};
    EXPECT_EQ(hm11::util::buffer::copy(buffer, dest2, 8), 3);
    EXPECT_EQ(dest2[0], 8);
    EXPECT_EQ(dest2[1], 9);
    EXPECT_EQ(dest2[2], 0);

    // Test append to vector
    std::vector<hm11::hal::UInt8> dest_vec;
    EXPECT_EQ(hm11::util::buffer::append(buffer, dest_vec, 2, 6), 4);
    EXPECT_EQ(dest_vec.size(), 4);
    EXPECT_EQ(dest_vec[0], 2);
    EXPECT_EQ(dest_vec[3], 5);

    // Test append with wrap
    EXPECT_EQ(hm11::util::buffer::append(buffer, dest_vec, 8, 2), 4);
    EXPECT_EQ(dest_vec.size(), 8);
    EXPECT_EQ(dest_vec[4], 8);
    EXPECT_EQ(dest_vec[5], 9);
    EXPECT_EQ(dest_vec[6], 0);
    EXPECT_EQ(dest_vec[7], 1);

    // Test append to fixed-size array (should be limited by capacity)
    std::array<hm11::hal::UInt8, 2> dest_arr = {0, 0};
    EXPECT_EQ(hm11::util::buffer::append(buffer, dest_arr, 4, 7), 2); // Only copies 2 out of 3 bytes
    EXPECT_EQ(dest_arr[0], 4);
    EXPECT_EQ(dest_arr[1], 5);
}

// Test find_zero with different null positions
TEST_F(UtilsTest, FindZero) {
    std::array<hm11::hal::UInt8, 10> buffer = {1, 2, 3, 0, 5, 6, 7, 8, 9, 0};

    EXPECT_EQ(hm11::util::buffer::find_zero(buffer, 0), 3);
    EXPECT_EQ(hm11::util::buffer::find_zero(buffer, 4), 9);
    EXPECT_EQ(hm11::util::buffer::find_zero(buffer, 10), 11); // Not found (returns size+1)
}