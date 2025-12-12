/**
 * @file array_utils_test.cpp
 * @brief Unit tests for array utility functions
 *
 * This file contains unit tests for the array utility functions in array_utils.hpp.
 * Tests ensure that array operations work correctly on different types of containers.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <vector>

#include "hm11/hal/types.hpp"
#include "hm11/util/array_utils.hpp"

namespace {

TEST(ArrayUtilsTest, CopyWorks) {
    // Test copying between arrays
    std::array<hm11::hal::UInt8, 5> src = {1, 2, 3, 4, 5};
    std::array<hm11::hal::UInt8, 5> dst = {0};

    std::size_t copied = hm11::util::copy(std::span<hm11::hal::UInt8>(dst), std::span<const hm11::hal::UInt8>(src));
    EXPECT_EQ(copied, 5U);
    EXPECT_EQ(dst, src);

    // Test partial copy (smaller destination)
    std::array<hm11::hal::UInt8, 3> small_dst = {0};
    copied = hm11::util::copy(std::span<hm11::hal::UInt8>(small_dst), std::span<const hm11::hal::UInt8>(src));
    EXPECT_EQ(copied, 3U);
    EXPECT_EQ(small_dst[0], 1);
    EXPECT_EQ(small_dst[1], 2);
    EXPECT_EQ(small_dst[2], 3);

    // Test partial copy (limited count)
    std::array<hm11::hal::UInt8, 5> limited_dst = {0};
    copied = hm11::util::copy(std::span<hm11::hal::UInt8>(limited_dst), std::span<const hm11::hal::UInt8>(src), 2);
    EXPECT_EQ(copied, 2U);
    EXPECT_EQ(limited_dst[0], 1);
    EXPECT_EQ(limited_dst[1], 2);
    EXPECT_EQ(limited_dst[2], 0);
}

TEST(ArrayUtilsTest, FillWorks) {
    // Test filling an entire array
    std::array<hm11::hal::UInt8, 5> arr = {0};

    std::size_t filled = hm11::util::fill(std::span<hm11::hal::UInt8>(arr), static_cast<hm11::hal::UInt8>(42));
    EXPECT_EQ(filled, 5U);
    for (auto val : arr) {
        EXPECT_EQ(val, 42);
    }

    // Test partial fill
    filled = hm11::util::fill(std::span<hm11::hal::UInt8>(arr), static_cast<hm11::hal::UInt8>(84), 3);
    EXPECT_EQ(filled, 3U);
    EXPECT_EQ(arr[0], 84);
    EXPECT_EQ(arr[1], 84);
    EXPECT_EQ(arr[2], 84);
    EXPECT_EQ(arr[3], 42);
    EXPECT_EQ(arr[4], 42);
}

TEST(ArrayUtilsTest, EqualWorks) {
    // Test equal arrays
    std::array<hm11::hal::UInt16, 4> a = {1, 2, 3, 4};
    std::array<hm11::hal::UInt16, 4> b = {1, 2, 3, 4};
    EXPECT_TRUE(hm11::util::equal(std::span<const hm11::hal::UInt16>(a), std::span<const hm11::hal::UInt16>(b)));

    // Test unequal arrays (different values)
    std::array<hm11::hal::UInt16, 4> c = {1, 2, 3, 5};
    EXPECT_FALSE(hm11::util::equal(std::span<const hm11::hal::UInt16>(a), std::span<const hm11::hal::UInt16>(c)));

    // Test unequal arrays (different sizes)
    std::array<hm11::hal::UInt16, 3> d = {1, 2, 3};
    EXPECT_FALSE(hm11::util::equal(std::span<const hm11::hal::UInt16>(a), std::span<const hm11::hal::UInt16>(d)));
}

TEST(ArrayUtilsTest, StartsWithWorks) {
    // Test exact match
    std::array<hm11::hal::UInt8, 4> arr = {1, 2, 3, 4};
    std::array<hm11::hal::UInt8, 4> pattern1 = {1, 2, 3, 4};
    EXPECT_TRUE(
        hm11::util::starts_with(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern1)));

    // Test prefix match
    std::array<hm11::hal::UInt8, 2> pattern2 = {1, 2};
    EXPECT_TRUE(
        hm11::util::starts_with(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern2)));

    // Test non-match
    std::array<hm11::hal::UInt8, 2> pattern3 = {2, 3};
    EXPECT_FALSE(
        hm11::util::starts_with(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern3)));

    // Test pattern too long
    std::array<hm11::hal::UInt8, 5> pattern4 = {1, 2, 3, 4, 5};
    EXPECT_FALSE(
        hm11::util::starts_with(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern4)));
}

TEST(ArrayUtilsTest, FindWorks) {
    // Test exact match at beginning
    std::array<hm11::hal::UInt8, 5> arr = {1, 2, 3, 2, 3};
    std::array<hm11::hal::UInt8, 1> pattern1 = {1};
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern1)),
        0U);

    // Test match in the middle
    std::array<hm11::hal::UInt8, 2> pattern2 = {2, 3};
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern2)),
        1U);

    // Test multiple matches (find first)
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern2), 2),
        3U);

    // Test non-match
    std::array<hm11::hal::UInt8, 2> pattern3 = {3, 1};
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern3)),
        std::numeric_limits<std::size_t>::max());

    // Test empty pattern
    std::array<hm11::hal::UInt8, 0> pattern4 = {};
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern4)),
        std::numeric_limits<std::size_t>::max());

    // Test start position beyond array bounds
    EXPECT_EQ(
        hm11::util::find(std::span<const hm11::hal::UInt8>(arr), std::span<const hm11::hal::UInt8>(pattern1), 10),
        std::numeric_limits<std::size_t>::max());
}

TEST(ArrayUtilsTest, SubspanWorks) {
    // Test regular subspan
    std::array<hm11::hal::UInt8, 5> arr = {1, 2, 3, 4, 5};
    auto span = hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 1, 4);
    EXPECT_EQ(span.size(), 3U);
    EXPECT_EQ(span[0], 2);
    EXPECT_EQ(span[1], 3);
    EXPECT_EQ(span[2], 4);

    // Test subspan to end
    span = hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 3);
    EXPECT_EQ(span.size(), 2U);
    EXPECT_EQ(span[0], 4);
    EXPECT_EQ(span[1], 5);

    // Test empty subspan (start equals end)
    span = hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 2, 2);
    EXPECT_EQ(span.size(), 0U);

    // Test empty subspan (end before start)
    span = hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 3, 1);
    EXPECT_EQ(span.size(), 0U);

    // Test out of bounds
    EXPECT_THROW(hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 10), std::out_of_range);

    // Test clamping end index
    span = hm11::util::subspan(std::span<const hm11::hal::UInt8>(arr), 3, 10);
    EXPECT_EQ(span.size(), 2U);
}

TEST(ArrayUtilsTest, HexStringConversion) {
    // Test to_hex_string
    std::array<hm11::hal::UInt8, 4> data = {0x12, 0x34, 0xAB, 0xCD};
    std::string hex = hm11::util::to_hex_string(std::span<const hm11::hal::UInt8>(data));
    EXPECT_EQ(hex, "1234ABCD");

    // Test to_hex_string with lowercase
    hex = hm11::util::to_hex_string(std::span<const hm11::hal::UInt8>(data), false);
    EXPECT_EQ(hex, "1234abcd");

    // Test from_hex_string
    auto bytes = hm11::util::from_hex_string("1234ABCD");
    EXPECT_EQ(bytes.size(), 4U);
    EXPECT_EQ(bytes[0], 0x12);
    EXPECT_EQ(bytes[1], 0x34);
    EXPECT_EQ(bytes[2], 0xAB);
    EXPECT_EQ(bytes[3], 0xCD);

    // Test from_hex_string with lowercase
    bytes = hm11::util::from_hex_string("1234abcd");
    EXPECT_EQ(bytes.size(), 4U);
    EXPECT_EQ(bytes[0], 0x12);
    EXPECT_EQ(bytes[1], 0x34);
    EXPECT_EQ(bytes[2], 0xAB);
    EXPECT_EQ(bytes[3], 0xCD);

    // Test from_hex_string with odd length (should throw)
    EXPECT_THROW(hm11::util::from_hex_string("123"), std::invalid_argument);

    // Test from_hex_string with invalid characters (should throw)
    EXPECT_THROW(hm11::util::from_hex_string("12XY"), std::invalid_argument);
}

} // namespace