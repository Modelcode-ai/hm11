// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file string_parser.hpp
 * @brief Zero-allocation string parsing utilities for embedded systems.
 *
 * Provides parsing functions that work directly on string_view without heap
 * allocation, suitable for embedded systems with constrained memory.
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace hm11::util {

/**
 * @brief Parse a decimal unsigned integer from a string_view.
 *
 * Parses the string_view as a base-10 unsigned integer without heap allocation.
 * Stops at the first non-digit character or end of string.
 *
 * @param str String view containing decimal digits.
 * @param result Output parameter for the parsed value.
 * @return true if at least one digit was parsed successfully, false otherwise.
 *
 * @par Example Usage
 * @code
 * std::string_view data = "123";
 * unsigned int value = 0;
 * if (util::parse_decimal(data, value)) {
 *     // value now contains 123
 * }
 * @endcode
 */
inline constexpr bool parse_decimal(std::string_view str, unsigned int& result) noexcept {
    if (str.empty()) {
        return false;
    }

    result = 0;
    bool parsed_any = false;

    for (char c : str) {
        if (c >= '0' && c <= '9') {
            result = result * 10 + static_cast<unsigned int>(c - '0');
            parsed_any = true;
        } else {
            break;  // Stop at first non-digit
        }
    }

    return parsed_any;
}

/**
 * @brief Parse a signed decimal integer from a string_view.
 *
 * Parses the string_view as a base-10 signed integer without heap allocation.
 * Handles optional leading minus sign. Stops at the first non-digit character.
 *
 * @param str String view containing decimal digits (optionally prefixed with '-').
 * @param result Output parameter for the parsed value.
 * @return true if at least one digit was parsed successfully, false otherwise.
 *
 * @par Example Usage
 * @code
 * std::string_view data = "-42";
 * int value = 0;
 * if (util::parse_signed_decimal(data, value)) {
 *     // value now contains -42
 * }
 * @endcode
 */
inline constexpr bool parse_signed_decimal(std::string_view str, int& result) noexcept {
    if (str.empty()) {
        return false;
    }

    bool negative = false;
    std::size_t start = 0;

    // Check for minus sign
    if (str[0] == '-') {
        negative = true;
        start = 1;
    }

    if (start >= str.size()) {
        return false;
    }

    unsigned int abs_value = 0;
    bool parsed_any = false;

    for (std::size_t i = start; i < str.size(); ++i) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            abs_value = abs_value * 10 + static_cast<unsigned int>(c - '0');
            parsed_any = true;
        } else {
            break;  // Stop at first non-digit
        }
    }

    if (!parsed_any) {
        return false;
    }

    result = negative ? -static_cast<int>(abs_value) : static_cast<int>(abs_value);
    return true;
}

} // namespace hm11::util
