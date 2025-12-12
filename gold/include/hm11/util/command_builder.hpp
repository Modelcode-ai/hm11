/**
 * @file command_builder.hpp
 * @brief Stack-based AT command builder utilities for HM-11 driver
 *
 * This header provides utilities for constructing AT command strings entirely
 * on the stack without heap allocation. Commands are built using a fluent
 * interface that concatenates command prefixes, parameters from enums, and
 * suffixes into fixed-size char arrays.
 *
 * ## Overview
 *
 * Every HM-11 driver method needs to construct AT commands like:
 * - "AT+ROLE1" (set central role)
 * - "AT+NAME012345ABCDEF" (set module name)
 * - "AT+CON0001122334455" (connect to MAC address)
 *
 * Ada concatenates strings directly using the "&" operator. C++ requires
 * explicit buffer management. This module provides safe, efficient command
 * construction with compile-time and runtime bounds checking.
 *
 * ## Design (Decision: Architecture Question 3)
 *
 * **Chosen Approach:** Lightweight class-based builder with fixed-size internal
 * buffer and fluent interface.
 *
 * **Rationale:**
 * - Provides clear, readable API: builder.append("AT+").append(role).get()
 * - Encapsulates bounds checking logic in single class
 * - Buffer lifetime tied to builder object (RAII)
 * - Fluent interface enables method chaining
 * - Single implementation point for overflow handling
 * - Easier to extend with additional append overloads
 *
 * **Rejected Alternatives:**
 * - Template functions writing to output parameters: More complex API, caller
 *   manages buffer, harder to chain operations, duplicated bounds checking
 * - Returning std::array: Can't have variable-length commands, wastes stack
 *   space for short commands, awkward API
 * - Separate utilities for command patterns: Code duplication, inconsistent
 *   bounds checking, harder to maintain
 *
 * ## Buffer Sizing
 *
 * Maximum HM-11 AT command analysis:
 * - Longest command prefix: "AT+NOTIFYOFF" (12 chars)
 * - Longest parameter: iBeacon UUID (32 hex chars) + "0x" prefix = 34 chars
 * - Module name: up to 12 characters
 * - Total worst case: ~50 chars
 * - Safety margin: 128 bytes chosen for generous headroom and alignment
 *
 * ## Architecture
 *
 * CommandBuilder uses a fixed std::array<char, 128> internal buffer with:
 * - Current write position tracked via size_t index
 * - Fluent interface: all append() methods return *this
 * - Bounds checking in debug builds (assertions)
 * - Optional overflow handling in release builds (saturation or error flag)
 * - Zero-copy get() method returns string_view into internal buffer
 * - Final c_str() method null-terminates and returns pointer
 *
 * Supported append types:
 * - String literals (const char*)
 * - std::string_view
 * - Enum values (via enum_to_string)
 * - FixedString types (MAC addresses, UUIDs, etc.)
 * - RangedInteger types (converted to decimal string)
 * - Single characters
 *
 * ## Usage Examples
 *
 * ### Basic Command Construction
 * @code
 * using namespace hm11::util;
 * using namespace hm11::types;
 *
 * // Simple enum-based command
 * CommandBuilder builder;
 * builder.append("AT+ROLE").append(Role::Central);
 * // Result: "AT+ROLE1"
 *
 * // Command with MAC address
 * MacAddress mac{"B4994C123456"};
 * CommandBuilder cmd;
 * cmd.append("AT+CON").append(mac);
 * // Result: "AT+CONB4994C123456"
 *
 * // Complex command with multiple parameters
 * CommandBuilder cb;
 * cb.append("AT+MARJ0x").append(VersionType{"0001"});
 * // Result: "AT+MARJ0x0001"
 * @endcode
 *
 * ### Method Chaining
 * @code
 * auto cmd = CommandBuilder()
 *     .append("AT+ROLE")
 *     .append(Role::Peripheral)
 *     .get(); // Returns std::string_view
 *
 * // Use in transmission
 * const char* cmd_str = CommandBuilder()
 *     .append("AT+NAME")
 *     .append(module_name)
 *     .c_str(); // Null-terminated C string
 * @endcode
 *
 * ### Query Commands
 * @code
 * auto query = CommandBuilder()
 *     .append("AT+ROLE?")
 *     .get();
 * // Result: "AT+ROLE?"
 * @endcode
 *
 * ### Error Handling
 * @code
 * CommandBuilder builder;
 * builder.append("AT+")
 *        .append(very_long_string); // Would overflow
 *
 * if (builder.overflow()) {
 *     // Handle error: command too long
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * CommandBuilder is NOT thread-safe. Each thread must use its own instance.
 * Builders are intended to be stack-allocated, used to build a single command,
 * then discarded.
 *
 * ## Performance
 *
 * - All operations are inline and constexpr where possible
 * - Zero heap allocation
 * - Stack buffer size: 128 bytes
 * - Typical command: 10-30 chars, fits easily in L1 cache
 * - Append operations: simple memcpy, O(n) in appended string length
 * - Bounds checking: single comparison per append (debug only)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include "hm11/types/enums.hpp"
#include "hm11/types/fixed_string.hpp"
#include "hm11/types/ranged_integer.hpp"
#include "hm11/util/enum_mapping.hpp"

namespace hm11::util {

// Bring types into scope for template parameters
using hm11::FixedString;
using hm11::types::RangedInteger;

/**
 * @brief Maximum size for AT command buffer
 *
 * Sized to handle the longest possible HM-11 AT command with generous margin:
 * - Longest prefix: "AT+NOTIFYOFF" (12 chars)
 * - Longest parameter: iBeacon UUID (32 hex chars) with "0x" prefix
 * - Safety margin for multiple parameters
 * - Total: 128 bytes provides 2x safety margin
 */
inline constexpr std::size_t MAX_COMMAND_SIZE = 128;

/**
 * @brief Stack-based AT command builder with fluent interface
 *
 * Provides safe, efficient construction of AT command strings without heap
 * allocation. Commands are built incrementally via append() methods and
 * retrieved via get() or c_str().
 *
 * ## Lifecycle
 *
 * 1. Construct builder (on stack)
 * 2. Chain append() calls to build command
 * 3. Check overflow() if concerned about buffer size
 * 4. Get result via get() (string_view) or c_str() (null-terminated)
 * 5. Builder automatically destroyed when out of scope
 *
 * ## Error Handling
 *
 * - Debug builds: Assertion failure on overflow
 * - Release builds: Overflow flag set, subsequent appends ignored
 * - Check overflow() to detect errors
 * - get() and c_str() return partial command if overflow occurred
 *
 * @note This class is NOT thread-safe. Use one builder per thread.
 * @note Returned string_view/pointer valid only while builder exists.
 */
class CommandBuilder {
  public:
    /**
     * @brief Construct empty command builder
     */
    constexpr CommandBuilder() noexcept = default;

    /**
     * @brief Append a string literal or C-string
     *
     * @param str Null-terminated string to append
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * builder.append("AT+ROLE");
     * @endcode
     */
    constexpr CommandBuilder& append(const char* str) noexcept {
        if (overflow_flag) {
            return *this; // Already overflowed, ignore further appends
        }

        const std::size_t LEN = std::char_traits<char>::length(str);
        if (current_size + LEN >= MAX_COMMAND_SIZE) {
            overflow_flag = true;
            assert(false && "CommandBuilder overflow: command too long");
            return *this;
        }

        // Copy string to buffer using std::copy_n (constexpr in C++20)
        // At this point: current_size + LEN < MAX_COMMAND_SIZE (proven by check above)
        // Note: GCC 14 -Wstringop-overflow and -Warray-bounds false positives suppressed below
        // The overflow check above guarantees this copy is safe, but GCC's
        // interprocedural analysis cannot prove the relationship at call sites
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        std::copy_n(str, LEN, buffer.data() + current_size);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
        current_size += LEN;

        return *this;
    }

    /**
     * @brief Append a string_view
     *
     * @param sv String view to append
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * std::string_view prefix = "AT+";
     * builder.append(prefix);
     * @endcode
     */
    constexpr CommandBuilder& append(std::string_view sv) noexcept {
        if (overflow_flag) {
            return *this;
        }

        const std::size_t LEN = sv.size();
        if (current_size + LEN >= MAX_COMMAND_SIZE) {
            overflow_flag = true;
            assert(false && "CommandBuilder overflow: command too long");
            return *this;
        }

        // Copy string_view to buffer using std::copy_n (constexpr in C++20)
        // At this point: current_size + LEN < MAX_COMMAND_SIZE (proven by check above)
        // Note: GCC 14 -Wstringop-overflow and -Warray-bounds false positives suppressed below
        // The overflow check above guarantees this copy is safe, but GCC's
        // interprocedural analysis cannot prove the relationship at call sites
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        std::ranges::copy(sv, buffer.begin() + current_size);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
        current_size += LEN;

        return *this;
    }

    /**
     * @brief Append a single character
     *
     * @param c Character to append
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * builder.append("AT+ROLE").append('?');
     * @endcode
     */
    constexpr CommandBuilder& append(char c) noexcept {
        if (overflow_flag) {
            return *this;
        }

        if (current_size >= MAX_COMMAND_SIZE - 1) {
            overflow_flag = true;
            assert(false && "CommandBuilder overflow: command too long");
            return *this;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Index checked by overflow guard above
        buffer[current_size++] = c;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        return *this;
    }

    /**
     * @brief Append a Role enum value
     *
     * Converts Role to its AT command parameter string ("0" or "1").
     *
     * @param role Role enum value
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * builder.append("AT+ROLE").append(Role::Central); // "AT+ROLE1"
     * @endcode
     */
    constexpr CommandBuilder& append(types::Role role) noexcept { return append(enum_to_string(role)); }

    /**
     * @brief Append a BondMode enum value
     *
     * @param mode BondMode enum value
     * @return Reference to this builder for method chaining
     */
    constexpr CommandBuilder& append(types::BondMode mode) noexcept { return append(enum_to_string(mode)); }

    /**
     * @brief Append an AdvertisingInterval enum value
     *
     * @param interval AdvertisingInterval enum value
     * @return Reference to this builder for method chaining
     */
    constexpr CommandBuilder& append(types::AdvertisingInterval interval) noexcept {
        return append(enum_to_string(interval));
    }

    // NOTE: AdvertisingType, UartBaudRate, SensorType enum mappings
    // are not yet implemented in enum_mapping.hpp.
    // These may be added when those enums need AT command conversion.

    /**
     * @brief Append any FixedString type (MAC, UUID, Handle, etc.)
     *
     * FixedString types include MacAddress, UUID, HandleType, IBeaconUuid,
     * VersionType, PinType, WorkInterval, etc. All are appended as their
     * string representation.
     *
     * @tparam N Size of the fixed string
     * @tparam Validator Character validator type
     * @param fs FixedString to append
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * MacAddress mac{"B4994C123456"};
     * builder.append("AT+CON").append(mac); // "AT+CONB4994C123456"
     * @endcode
     */
    template <std::size_t N, typename Validator>
    constexpr CommandBuilder& append(const FixedString<N, Validator>& fs) noexcept {
        return append(fs.view());
    }

    /**
     * @brief Append any RangedInteger type as decimal string
     *
     * RangedInteger types include Percent, TemperatureType, LayerConnectionLatency,
     * MacWhiteListIndex, etc. Values are converted to decimal string representation.
     *
     * @tparam T Underlying integer type
     * @tparam Min Minimum value
     * @tparam Max Maximum value
     * @tparam Tag Tag type for strong typing
     * @param ri RangedInteger to append
     * @return Reference to this builder for method chaining
     *
     * Example:
     * @code
     * MacWhiteListIndex idx{2};
     * builder.append("AT+AD").append(idx); // "AT+AD2"
     * @endcode
     */
    template <typename T, T Min, T Max, typename Tag>
    constexpr CommandBuilder& append(const RangedInteger<T, Min, Max, Tag>& ri) noexcept {
        T value = ri.get();

        // Handle negative values (though most HM-11 types are unsigned)
        if constexpr (std::is_signed_v<T>) {
            if (value < 0) {
                append('-');
                // NOTE: This assumes Min > std::numeric_limits<T>::min()
                // so that -value is well-defined.
                value = -value;
            }
        }

        // Zero is a special case
        if (value == 0) {
            append('0');
            return *this;
        }

        // Enough for 64-bit integer (19 digits) + some slack
        std::array<char, 20> temp{};
        char* it = temp.data();

        // Write digits in reverse order into temp
        T tmp = value;
        while (tmp > 0) {
            *it++ = static_cast<char>('0' + static_cast<int>(tmp % 10));
            tmp /= 10;
        }

        // Now [temp.data(), it) holds the digits in reverse.
        // Walk backwards without indexing, so no bounds/index warnings.
        while (it != temp.data()) {
            --it;
            append(*it);
        }

        return *this;
    }

    /**
     * @brief Get the constructed command as a string_view
     *
     * Returns a view into the internal buffer. The view is valid only while
     * this builder exists and is not modified.
     *
     * @return std::string_view of the constructed command
     *
     * Example:
     * @code
     * auto cmd = builder.append("AT+ROLE").append(Role::Central).get();
     * // cmd is "AT+ROLE1"
     * @endcode
     */
    [[nodiscard]] constexpr std::string_view get() const noexcept { return {buffer.data(), current_size}; }

    /**
     * @brief Get the constructed command as a null-terminated C string
     *
     * Adds null terminator to the buffer and returns pointer. The pointer is
     * valid only while this builder exists and is not modified.
     *
     * @return const char* pointer to null-terminated command string
     *
     * @warning This method modifies the internal buffer by adding null terminator.
     *          Do not use get() after calling c_str() unless you re-build the command.
     *
     * Example:
     * @code
     * const char* cmd = builder.append("AT+ROLE0").c_str();
     * transmit(uart, cmd);
     * @endcode
     */
    [[nodiscard]] constexpr const char* c_str() noexcept {
        if (current_size < MAX_COMMAND_SIZE) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
            // Index checked by conditional above
            buffer[current_size] = '\0';
            // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        } else {
            // Overflow - ensure last position is null terminator
            buffer[MAX_COMMAND_SIZE - 1] = '\0';
        }
        return buffer.data();
    }

    /**
     * @brief Check if overflow occurred during command construction
     *
     * @return true if buffer overflowed, false otherwise
     *
     * Example:
     * @code
     * builder.append("AT+").append(very_long_parameter);
     * if (builder.overflow()) {
     *     handle_error();
     * }
     * @endcode
     */
    [[nodiscard]] constexpr bool overflow() const noexcept { return overflow_flag; }

    /**
     * @brief Get current command length
     *
     * @return Number of characters in the command
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept { return current_size; }

    /**
     * @brief Check if command is empty
     *
     * @return true if no characters appended, false otherwise
     */
    [[nodiscard]] constexpr bool empty() const noexcept { return current_size == 0; }

    /**
     * @brief Clear the command buffer and reset state
     *
     * Resets the builder to initial empty state. Useful for reusing the same
     * builder object for multiple commands.
     *
     * Example:
     * @code
     * CommandBuilder builder;
     * builder.append("AT+ROLE0");
     * send_command(builder.get());
     * builder.clear();
     * builder.append("AT+NAME").append(name);
     * send_command(builder.get());
     * @endcode
     */
    constexpr void clear() noexcept {
        current_size = 0;
        overflow_flag = false;
    }

  private:
    std::array<char, MAX_COMMAND_SIZE> buffer{}; ///< Internal command buffer
    std::size_t current_size{0};                 ///< Current command length
    bool overflow_flag{false};                   ///< True if overflow occurred
};

} // namespace hm11::util
