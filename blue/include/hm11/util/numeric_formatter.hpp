// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file numeric_formatter.hpp
 * @brief Zero-allocation integer-to-string formatting for embedded systems.
 *
 * Provides fixed-buffer numeric formatting without heap allocation, suitable
 * for embedded systems with constrained memory and real-time requirements.
 */

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace hm11::util {

/**
 * @brief Zero-allocation integer-to-string formatter.
 *
 * This class provides static methods for formatting integers to string
 * without heap allocation. All operations are noexcept and use fixed-size
 * buffers on the stack.
 *
 * @par Thread Safety
 * The format_uint() and format_int() methods are thread-safe when the caller
 * provides their own buffer. The convenience format() method uses thread_local
 * storage and is thread-safe on platforms that support it.
 *
 * @par Performance
 * All operations are O(log10(n)) where n is the value being formatted.
 * No dynamic memory allocation.
 *
 * @par Example Usage
 * @code
 * // Method 1: Convenience (uses thread_local buffer)
 * auto str = NumericFormatter::format(12345);
 * uart.transmit(str);
 *
 * // Method 2: Caller-provided buffer
 * std::array<char, 12> buffer;
 * auto str = NumericFormatter::format_uint(12345, buffer);
 * uart.transmit(str);
 * @endcode
 */
class NumericFormatter {
  public:
    /**
     * @brief Format an unsigned integer to a buffer.
     *
     * Formats the value as a decimal string (base 10) into the provided buffer.
     * The formatted string is not null-terminated.
     *
     * @param value The unsigned integer to format.
     * @param buffer The buffer to write to. Must be large enough for the result.
     *              Maximum required size: 10 bytes for UINT32_MAX (4294967295).
     * @return std::string_view pointing to the formatted string within buffer.
     *         Returns empty view if buffer is too small.
     *
     * @note If the buffer is too small, this function returns an empty string_view.
     *       For uint32_t, a buffer of 10 bytes is always sufficient.
     */
    static constexpr std::string_view format_uint(std::uint32_t value,
                                                  std::span<char> buffer) noexcept {
        if (buffer.empty()) {
            return {};
        }

        // Handle zero case
        if (value == 0) {
            buffer[0] = '0';
            return {buffer.data(), 1};
        }

        // Format from right to left
        std::size_t pos = buffer.size();
        while (value > 0 && pos > 0) {
            buffer[--pos] = static_cast<char>('0' + (value % 10));
            value /= 10;
        }

        // Check if we ran out of space
        if (value > 0) {
            return {};  // Buffer too small
        }

        return {buffer.data() + pos, buffer.size() - pos};
    }

    /**
     * @brief Format a signed integer to a buffer.
     *
     * Formats the value as a decimal string (base 10) with optional minus sign.
     * The formatted string is not null-terminated.
     *
     * @param value The signed integer to format.
     * @param buffer The buffer to write to. Must be large enough for the result.
     *              Maximum required size: 11 bytes for INT32_MIN (-2147483648).
     * @return std::string_view pointing to the formatted string within buffer.
     *         Returns empty view if buffer is too small.
     *
     * @note If the buffer is too small, this function returns an empty string_view.
     *       For int32_t, a buffer of 11 bytes is always sufficient.
     */
    static constexpr std::string_view format_int(std::int32_t value,
                                                 std::span<char> buffer) noexcept {
        if (buffer.empty()) {
            return {};
        }

        // Handle zero case
        if (value == 0) {
            buffer[0] = '0';
            return {buffer.data(), 1};
        }

        // Handle negative values
        bool negative = value < 0;

        // Convert to unsigned, handling INT32_MIN correctly
        std::uint32_t abs_value = 0;
        if (value == std::numeric_limits<std::int32_t>::min()) {
            // Special case: -INT32_MIN would overflow
            abs_value = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) + 1U;
        } else {
            abs_value = negative ? static_cast<std::uint32_t>(-value)
                                : static_cast<std::uint32_t>(value);
        }

        // Format from right to left
        std::size_t pos = buffer.size();
        while (abs_value > 0 && pos > 0) {
            buffer[--pos] = static_cast<char>('0' + (abs_value % 10));
            abs_value /= 10;
        }

        // Check if we ran out of space
        if (abs_value > 0) {
            return {};  // Buffer too small
        }

        // Add minus sign if negative
        if (negative) {
            if (pos == 0) {
                return {};  // No space for minus sign
            }
            buffer[--pos] = '-';
        }

        return {buffer.data() + pos, buffer.size() - pos};
    }

    /**
     * @brief Format an integer using a thread_local buffer (convenience method).
     *
     * This is a convenience method that uses a thread_local buffer for formatting.
     * The returned string_view is only valid until the next call to format() in
     * the same thread.
     *
     * @tparam T The integer type (signed or unsigned).
     * @param value The value to format.
     * @return std::string_view pointing to the formatted string.
     *
     * @note The returned string_view is only valid until the next call to format()
     *       in the same thread. Copy the string if you need to keep it longer.
     *
     * @par Example
     * @code
     * auto str1 = NumericFormatter::format(42);
     * auto str2 = NumericFormatter::format(-100);  // str1 is now invalid!
     * @endcode
     */
    template <typename T>
    static std::string_view format(T value) noexcept {
        // Buffer large enough for any int32_t or uint32_t (11 bytes + 1 for safety)
        thread_local std::array<char, 12> buffer;

        if constexpr (std::is_signed_v<T>) {
            return format_int(static_cast<std::int32_t>(value), buffer);
        } else {
            return format_uint(static_cast<std::uint32_t>(value), buffer);
        }
    }
};

} // namespace hm11::util
