/**
 * @file response_parser.hpp
 * @brief Utilities for parsing AT command responses
 *
 * This file provides utilities for parsing AT command responses from the HM-11 module.
 * It implements Decision 6 (Response Parsing) from the architecture specification,
 * using a zero-copy approach with std::string_view for efficient parsing.
 */

#ifndef HM11_UTIL_RESPONSE_PARSER_HPP
#define HM11_UTIL_RESPONSE_PARSER_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "../driver.hpp"
#include "../enum_types.hpp"
#include "../hal/types.hpp"
#include "../hal/uart.hpp"
#include "../hm11.hpp"
#include "at_command_builder.hpp"
#include "fixed_string.hpp"

/**
 * @brief Namespace for response parsing utilities
 */
namespace hm11::util::parser {

/**
 * @brief Check if the response contains the expected string
 *
 * @param response The response to check
 * @param expected The expected string
 * @return true if the response contains the expected string
 * @return false otherwise
 */
[[nodiscard]] bool contains(std::string_view response, std::string_view expected);

/**
 * @brief Check if the response starts with the expected string
 *
 * @param response The response to check
 * @param expected The expected string
 * @return true if the response starts with the expected string
 * @return false otherwise
 */
[[nodiscard]] bool starts_with(std::string_view response, std::string_view expected);

/**
 * @brief Find the position of a substring in the response
 *
 * This is a wrapper around string_view::find that handles the not-found case
 * in a more explicit way.
 *
 * @param response The response to search in
 * @param substring The substring to search for
 * @return std::optional<size_t> The position of the substring, or nullopt if not found
 */
[[nodiscard]] std::optional<size_t> find(std::string_view response, std::string_view substring);

/**
 * @brief Extract a substring from the response
 *
 * This function extracts a substring from the response between the start and end
 * positions.
 *
 * @param response The response to extract from
 * @param start_pos The start position
 * @param end_pos The end position (optional, defaults to the end of the string)
 * @return std::string_view The extracted substring
 */
[[nodiscard]] std::string_view
extract(std::string_view response, size_t start_pos, size_t end_pos = std::string_view::npos);

/**
 * @brief Extract a value from a response with the specified prefix
 *
 * This function extracts a value from a response that follows the specified prefix.
 * For example, if the response is "OK+Get:1" and the prefix is "OK+Get:", it will
 * return "1".
 *
 * @param response The response to extract from
 * @param prefix The prefix to look for
 * @param end_delimiter An optional delimiter that marks the end of the value
 * @return std::optional<std::string_view> The extracted value, or nullopt if not found
 */
[[nodiscard]] std::optional<std::string_view>
extract_after(std::string_view response, std::string_view prefix, char end_delimiter = '\0');

/**
 * @brief Extract and convert an enum value from a response with the specified prefix
 *
 * This function extracts an enum value from a response that follows the specified prefix.
 * For example, if the response is "OK+Get:1" and the prefix is "OK+Get:", it will
 * extract "1" and convert it to the corresponding enum value.
 *
 * @tparam EnumType The enum type to convert to
 * @param response The response to extract from
 * @param prefix The prefix to look for
 * @param end_delimiter An optional delimiter that marks the end of the value
 * @param success Output parameter indicating if the extraction was successful
 * @return EnumType The extracted enum value, or a default value if extraction failed
 */
template <typename EnumType>
[[nodiscard]] EnumType
extract_enum(std::string_view response, std::string_view prefix, char end_delimiter, bool& success) {
    auto value_opt = extract_after(response, prefix, end_delimiter);
    if (!value_opt) {
        success = false;
        return static_cast<EnumType>(0);
    }

    std::string_view value = *value_opt;
    if (value.empty()) {
        success = false;
        return static_cast<EnumType>(0);
    }

    // Convert the string value to an integer manually to avoid allocation
    int int_value = 0;
    bool found_digit = false;

    // Simple manual string to int conversion - stop at first non-digit
    for (char c : value) {
        if (c >= '0' && c <= '9') {
            found_digit = true;
            int_value = int_value * 10 + (c - '0');
        } else {
            // Stop at first non-digit character (could be \r, \n, space, etc.)
            break;
        }
    }

    if (!found_digit) {
        success = false;
        return static_cast<EnumType>(0);
    }

    // Validate the enum value is in valid range
    // For Role enum: only 0 (Peripheral) and 1 (Central) are valid
    if constexpr (std::is_same_v<EnumType, Role>) {
        if (int_value != 0 && int_value != 1) {
            success = false;
            return static_cast<EnumType>(0);
        }
    }

    // Convert the integer to the enum type
    success = true;
    return static_cast<EnumType>(int_value);
}

/**
 * @brief Extract a Role from a response
 *
 * @param response The response to extract from
 * @param status Output parameter for status
 * @return Role The extracted role
 */
[[nodiscard]] Role extract_role(std::string_view response, hal::UartStatus& status);

/**
 * @brief Extract a MAC address from a response
 *
 * @param response The response to extract from
 * @param prefix The prefix to look for (e.g., "OK+Get:")
 * @param mac Output parameter for the extracted MAC address
 * @param status Output parameter for status
 * @return true if extraction was successful
 * @return false otherwise
 */
bool extract_mac_address(std::string_view response, std::string_view prefix, MacAddress& mac, hal::UartStatus& status);

/**
 * @brief Extract a version string from a response
 *
 * @param response The response to extract from
 * @param version Output parameter for the extracted version
 * @param status Output parameter for status
 * @return true if extraction was successful
 * @return false otherwise
 */
bool extract_version(std::string_view response, VersionType& version, hal::UartStatus& status);

/**
 * @brief Extract a name string from a response
 *
 * @param response The response to extract from
 * @param name Output parameter for the extracted name
 * @param max_length Maximum length of the name
 * @param status Output parameter for status
 * @return true if extraction was successful
 * @return false otherwise
 */
bool extract_name(std::string_view response, char* name, std::size_t max_length, hal::UartStatus& status);

/**
 * @brief Check if a response indicates success
 *
 * @param response The response to check
 * @return true if the response indicates success
 * @return false otherwise
 */
[[nodiscard]] bool is_success(std::string_view response);

/**
 * @brief Check if a response indicates an error
 *
 * @param response The response to check
 * @return true if the response indicates an error
 * @return false otherwise
 */
[[nodiscard]] bool is_error(std::string_view response);

/**
 * @brief Extract a numerical value from a response
 *
 * @tparam T The numerical type to convert to
 * @param response The response to extract from
 * @param prefix The prefix to look for
 * @param end_delimiter An optional delimiter that marks the end of the value
 * @param success Output parameter indicating if the extraction was successful
 * @return T The extracted numerical value, or a default value if extraction failed
 */
template <typename T>
[[nodiscard]] T extract_number(std::string_view response, std::string_view prefix, char end_delimiter, bool& success) {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    auto value_opt = extract_after(response, prefix, end_delimiter);
    if (!value_opt) {
        success = false;
        return T{};
    }

    std::string_view value = *value_opt;
    if (value.empty()) {
        success = false;
        return T{};
    }

    // Convert the string value to a number without allocation
    T result{};
    bool valid = true;

    if constexpr (std::is_integral_v<T>) {
        // Handle sign for signed types
        bool is_negative = false;
        auto start_idx = std::size_t{0};

        if constexpr (std::is_signed_v<T>) {
            if (!value.empty() && value[0] == '-') {
                is_negative = true;
                start_idx = 1;
            }
        }

        // Parse the digits - stop at first non-digit (e.g., \r, \n, whitespace)
        bool found_digit = false;
        for (std::size_t i = start_idx; i < value.size(); ++i) {
            if (value[i] >= '0' && value[i] <= '9') {
                found_digit = true;
                result = static_cast<T>((result * 10) + (value[i] - '0'));
            } else {
                // Stop at first non-digit character (could be \r, \n, space, etc.)
                break;
            }
        }

        // Only invalid if we didn't find any digits
        if (!found_digit) {
            valid = false;
        }

        // Apply sign
        if constexpr (std::is_signed_v<T>) {
            if (is_negative) {
                result = -result;
            }
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        // Simple approximation for floating point - in a real implementation you would
        // want a more robust parsing function, but this avoids allocation
        bool is_negative = false;
        auto start_idx = std::size_t{0};

        if (!value.empty() && value[0] == '-') {
            is_negative = true;
            start_idx = 1;
        }

        bool after_decimal = false;
        T decimal_factor = 1.0;

        for (std::size_t i = start_idx; i < value.size(); ++i) {
            if (value[i] == '.') {
                if (after_decimal) {
                    valid = false; // Multiple decimal points
                    break;
                }
                after_decimal = true;
            } else if (value[i] >= '0' && value[i] <= '9') {
                if (after_decimal) {
                    decimal_factor *= 10.0;
                    result += static_cast<T>(value[i] - '0') / decimal_factor;
                } else {
                    result = result * 10.0 + static_cast<T>(value[i] - '0');
                }
            } else {
                valid = false;
                break;
            }
        }

        if (is_negative) {
            result = -result;
        }
    }

    if (!valid) {
        success = false;
        return T{};
    }

    success = true;
    return result;
}

/**
 * @brief Create a string_view from a potentially null-terminated buffer
 *
 * @param buffer The buffer containing the data
 * @param max_size The maximum size of the buffer
 * @return std::string_view A string_view of the data (up to the null terminator if present)
 */
[[nodiscard]] std::string_view to_string_view(const hal::UInt8* buffer, std::size_t max_size);

/**
 * @brief Create a string_view from a potentially null-terminated buffer
 *
 * @param buffer The buffer containing the data
 * @param max_size The maximum size of the buffer
 * @return std::string_view A string_view of the data (up to the null terminator if present)
 */
[[nodiscard]] std::string_view to_string_view(const char* buffer, std::size_t max_size);

/**
 * @brief Create a string_view from a potentially null-terminated buffer
 *
 * @tparam N The size of the buffer
 * @param buffer The buffer containing the data
 * @return std::string_view A string_view of the data (up to the null terminator if present)
 */
template <std::size_t N> [[nodiscard]] std::string_view to_string_view(const std::array<hal::UInt8, N>& buffer) {
    return to_string_view(buffer.data(), buffer.size());
}

/**
 * @brief Create a string_view from a potentially null-terminated buffer
 *
 * @tparam N The size of the buffer
 * @param buffer The buffer containing the data
 * @return std::string_view A string_view of the data (up to the null terminator if present)
 */
template <std::size_t N> [[nodiscard]] std::string_view to_string_view(const std::array<char, N>& buffer) {
    return to_string_view(buffer.data(), buffer.size());
}

} // namespace hm11::util::parser

#endif // HM11_UTIL_RESPONSE_PARSER_HPP