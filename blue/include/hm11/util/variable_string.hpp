// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file variable_string.hpp
 * @brief Zero-allocation variable-length string for embedded systems.
 *
 * Provides stack-allocated variable-length string storage without heap allocation,
 * suitable for embedded systems with constrained memory and real-time requirements.
 */

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace hm11::util {

/**
 * @brief Fixed-capacity variable-length string with zero heap allocation.
 *
 * This class provides a stack-allocated string container that can hold strings
 * up to Capacity bytes. It uses a fixed std::array for storage and tracks the
 * actual length separately. All operations are noexcept and do not allocate
 * from the heap.
 *
 * @tparam Capacity Maximum number of characters that can be stored.
 *
 * @par Thread Safety
 * Not thread-safe. Concurrent access to the same instance requires external
 * synchronization.
 *
 * @par Performance
 * All operations are O(1) except append(std::string_view) which is O(n) where
 * n is the length of the appended string. No dynamic memory allocation.
 *
 * @par Memory Layout
 * The class stores a fixed std::array<char, Capacity> plus a size_t for length,
 * resulting in sizeof(VariableString<N>) = N + sizeof(size_t) bytes.
 *
 * @par Example Usage
 * @code
 * VariableString<32> name;
 * name.append("Hello");
 * name.append(' ');
 * name.append("World");
 * uart.transmit(name.view());  // Transmit "Hello World"
 * name.clear();                // Reuse the buffer
 * @endcode
 */
template <std::size_t Capacity>
struct VariableString {
    std::array<char, Capacity> value{};
    std::size_t last{0};

    /**
     * @brief Clear the string, resetting length to zero.
     *
     * Does not zero the underlying array, only resets the length counter.
     * This is safe because view() only exposes [0, last).
     */
    constexpr void clear() noexcept {
        last = 0;
    }

    /**
     * @brief Append a single character to the string.
     *
     * @param c Character to append.
     * @return true if character was appended, false if capacity exceeded.
     */
    constexpr bool append(char c) noexcept {
        if (last >= Capacity) {
            return false;
        }
        value[last++] = c;
        return true;
    }

    /**
     * @brief Append a string_view to the string.
     *
     * Appends as many characters as possible without exceeding capacity.
     *
     * @param str String view to append.
     * @return true if all characters were appended, false if capacity exceeded.
     */
    constexpr bool append(std::string_view str) noexcept {
        if (last + str.size() > Capacity) {
            return false;
        }
        for (char c : str) {
            value[last++] = c;
        }
        return true;
    }

    /**
     * @brief Get a string_view of the current string content.
     *
     * The returned view is valid as long as this VariableString object exists
     * and is not modified.
     *
     * @return std::string_view pointing to [0, last) bytes in the buffer.
     */
    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return {value.data(), last};
    }

    /**
     * @brief Get the current length of the string.
     *
     * @return Number of characters currently stored.
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return last;
    }

    /**
     * @brief Get the maximum capacity of the string.
     *
     * @return Maximum number of characters that can be stored.
     */
    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return Capacity;
    }

    /**
     * @brief Check if the string is empty.
     *
     * @return true if length is zero, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return last == 0;
    }
};

} // namespace hm11::util
