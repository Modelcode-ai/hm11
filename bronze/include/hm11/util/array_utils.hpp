/**
 * @file array_utils.hpp
 * @brief Utility functions for array operations
 *
 * This file contains utility functions for working with arrays and spans,
 * including operations like copying, comparing, and manipulating array data.
 * These functions help translate Ada's array operations to C++.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_UTIL_ARRAY_UTILS_HPP
#define HM11_UTIL_ARRAY_UTILS_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "../hal/types.hpp"

namespace hm11::util {

/**
 * @brief Copies data from one array-like container to another
 *
 * @tparam T Element type
 * @param dst Destination span
 * @param src Source span
 * @param count Number of elements to copy (default: as many as possible)
 * @return std::size_t Number of elements actually copied
 */
template <typename T>
std::size_t
copy(std::span<T> dst, std::span<const T> src, std::size_t count = std::numeric_limits<std::size_t>::max()) {
    const std::size_t COPY_COUNT = std::min({dst.size(), src.size(), count});
    if (COPY_COUNT > 0) {
        std::memcpy(dst.data(), src.data(), COPY_COUNT * sizeof(T));
    }
    return COPY_COUNT;
}

/**
 * @brief Fills an array-like container with a specific value
 *
 * @tparam T Element type
 * @param dst Destination span
 * @param value Value to fill with
 * @param count Number of elements to fill (default: all)
 * @return std::size_t Number of elements actually filled
 */
template <typename T>
std::size_t fill(std::span<T> dst, T value, std::size_t count = std::numeric_limits<std::size_t>::max()) {
    const std::size_t FILL_COUNT = std::min(dst.size(), count);
    if (FILL_COUNT > 0) {
        std::fill_n(dst.data(), FILL_COUNT, value);
    }
    return FILL_COUNT;
}

/**
 * @brief Compares two array-like containers for equality
 *
 * @tparam T Element type
 * @param a First span
 * @param b Second span
 * @return true If the spans have the same size and all elements are equal
 * @return false Otherwise
 */
template <typename T> bool equal(std::span<const T> a, std::span<const T> b) {
    if (a.size() != b.size()) {
        return false;
    }
    return std::memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0;
}

/**
 * @brief Checks if an array starts with a specific pattern
 *
 * @tparam T Element type
 * @param array Array to check
 * @param pattern Pattern to look for
 * @return true If array starts with the pattern
 * @return false Otherwise
 */
template <typename T> bool starts_with(std::span<const T> array, std::span<const T> pattern) {
    if (pattern.size() > array.size()) {
        return false;
    }
    return std::memcmp(array.data(), pattern.data(), pattern.size() * sizeof(T)) == 0;
}

/**
 * @brief Finds the first occurrence of a pattern in an array
 *
 * @tparam T Element type
 * @param array Array to search in
 * @param pattern Pattern to find
 * @param start_pos Starting position for the search
 * @return std::size_t Index of the first match or npos if not found
 */
template <typename T>
std::size_t find(std::span<const T> array, std::span<const T> pattern, std::size_t start_pos = 0) {
    static constexpr std::size_t NPOS = std::numeric_limits<std::size_t>::max();

    if (pattern.empty() || start_pos >= array.size() || pattern.size() > array.size() - start_pos) {
        return NPOS;
    }

    // Special case for single element pattern (faster)
    if (pattern.size() == 1) {
        for (std::size_t i = start_pos; i < array.size(); ++i) {
            if (array[i] == pattern[0]) {
                return i;
            }
        }
        return NPOS;
    }

    // General case for multi-element pattern
    for (std::size_t i = start_pos; i <= array.size() - pattern.size(); ++i) {
        if (std::memcmp(&array[i], pattern.data(), pattern.size() * sizeof(T)) == 0) {
            return i;
        }
    }
    return NPOS;
}

/**
 * @brief Creates a subspan (view) of an array
 *
 * @tparam T Element type
 * @param array Source array
 * @param start Start index (inclusive)
 * @param end End index (exclusive) - defaults to end of array
 * @return std::span<const T> View of the specified portion of the array
 *
 * @throws std::out_of_range If start is beyond the array bounds
 * @note If end is beyond array bounds, it's clamped to the array size
 */
template <typename T>
std::span<const T>
subspan(std::span<const T> array, std::size_t start, std::size_t end = std::numeric_limits<std::size_t>::max()) {
    if (start > array.size()) {
        throw std::out_of_range("Start index out of bounds");
    }

    end = std::min(end, array.size());
    if (end <= start) {
        return std::span<const T>();
    }

    return array.subspan(start, end - start);
}

/**
 * @brief Converts a sequence of bytes to a hex string
 *
 * @param data Byte array to convert
 * @param upper_case Whether to use uppercase hex characters (default: true)
 * @return std::string Hexadecimal representation of the data
 */
inline std::string to_hex_string(std::span<const hm11::hal::UInt8> data, bool upper_case = true) {
    static const char* lowercase_hex_chars = "0123456789abcdef";
    static const char* uppercase_hex_chars = "0123456789ABCDEF";
    const char* hex_chars = upper_case ? uppercase_hex_chars : lowercase_hex_chars;

    std::string result;
    result.reserve(data.size() * 2);

    for (auto byte : data) {
        result.push_back(hex_chars[byte >> 4]);
        result.push_back(hex_chars[byte & 0x0F]);
    }

    return result;
}

/**
 * @brief Converts a hex string to a byte array
 *
 * @param hex_str Hexadecimal string to convert
 * @return std::vector<hm11::hal::UInt8> Byte array parsed from the hex string
 * @throws std::invalid_argument If the string contains non-hex characters
 */
inline std::vector<hm11::hal::UInt8> from_hex_string(std::string_view hex_str) {
    if (hex_str.size() % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even number of characters");
    }

    std::vector<hm11::hal::UInt8> result;
    result.reserve(hex_str.size() / 2);

    for (std::size_t i = 0; i < hex_str.size(); i += 2) {
        char high = hex_str[i];
        char low = hex_str[i + 1];

        // Convert hex characters to values
        auto to_value = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            throw std::invalid_argument("Invalid hex character");
        };

        result.push_back(static_cast<hm11::hal::UInt8>((to_value(high) << 4) | to_value(low)));
    }

    return result;
}

} // namespace hm11::util

#endif // HM11_UTIL_ARRAY_UTILS_HPP