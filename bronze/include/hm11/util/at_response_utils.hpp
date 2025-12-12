/**
 * @file at_response_utils.hpp
 * @brief Utility functions for parsing AT command responses
 *
 * This file provides utility functions for parsing AT command responses
 * and extracting data from them.
 */

#ifndef HM11_UTIL_AT_RESPONSE_UTILS_HPP
#define HM11_UTIL_AT_RESPONSE_UTILS_HPP

#include <string>
#include <string_view>

#include "../driver.hpp"
#include "../enum_types.hpp"
#include "../hm11.hpp"
#include "at_command_builder.hpp"
#include "fixed_string.hpp"
#include "response_parser.hpp"

namespace hm11::util {

/**
 * @brief Parse a response and extract a value
 *
 * @tparam T The type of value to extract
 * @param response The response string
 * @param prefix The prefix to search for
 * @param value Output parameter for the extracted value
 * @return true if parsing was successful
 * @return false otherwise
 */
template <typename T> bool parse_response(const std::string& response, const std::string& prefix, T& value) {
    bool success = false;

    // Convert string to string_view to avoid allocations
    std::string_view response_view(response);
    std::string_view prefix_view(prefix);

    if constexpr (std::is_integral_v<T>) {
        // For integral types, use extract_number
        value = parser::extract_number<T>(response_view, prefix_view, '\0', success);
    } else if constexpr (std::is_same_v<T, bool>) {
        // For boolean, extract as int and convert
        int int_value = parser::extract_number<int>(response_view, prefix_view, '\0', success);
        if (success) {
            value = (int_value != 0);
        }
    } else if constexpr (std::is_enum_v<T>) {
        // For enums, use extract_enum which validates the range
        value = parser::extract_enum<T>(response_view, prefix_view, '\0', success);
    } else if constexpr (std::is_same_v<T, std::string>) {
        // For strings, extract directly (empty string is valid)
        auto value_opt = parser::extract_after(response_view, prefix_view);
        if (value_opt.has_value()) {
            value = std::string(*value_opt);
            success = true;
        } else {
            // Prefix not found
            success = false;
        }
    } else if constexpr (
        std::is_same_v<T, MacAddress> || std::is_same_v<T, Uuid> || std::is_same_v<T, IBeaconUuid> ||
        std::is_same_v<T, VersionType>) {
        // For fixed strings, validate BEFORE constructing to avoid assertion failures
        auto value_opt = parser::extract_after(response_view, prefix_view);
        if (value_opt && value_opt->size() >= T::size()) {
            // Manually validate hex characters (all these types use HexCharValidator)
            bool valid = true;
            for (size_t i = 0; i < T::size(); ++i) {
                char c = (*value_opt)[i];
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                T temp(*value_opt);
                value = temp;
                success = true;
            } else {
                // Invalid characters - don't construct
                success = false;
            }
        } else {
            // Prefix not found or value too short
            success = false;
        }
    }

    return success;
}

/**
 * @brief Check if a response starts with the expected value
 *
 * @param response The response string
 * @param expected The expected string
 * @return true if the response starts with the expected string
 * @return false otherwise
 */
bool check_response(const std::string& response, const std::string& expected);

/**
 * @brief Parse a MAC address from a response
 *
 * @param response The response string
 * @param prefix The prefix to search for
 * @param mac Output parameter for the extracted MAC address
 * @return true if parsing was successful
 * @return false otherwise
 */
bool parse_mac_address(const std::string& response, const std::string& prefix, MacAddress& mac);

/**
 * @brief Parse a version string from a response
 *
 * @param response The response string
 * @param version Output parameter for the extracted version
 * @return true if parsing was successful
 * @return false otherwise
 */
bool parse_version(const std::string& response, std::string& version);

/**
 * @brief Build an AT command with zero or more parameters
 *
 * @tparam Args Types of the parameters
 * @param command The base command
 * @param args The parameters
 * @return std::string The complete AT command
 */
template <typename... Args> std::string build_command(std::string_view command, Args&&... args) {
    std::string result(command);

    // Helper function to append each argument
    [[maybe_unused]] auto append_arg = [&result](const auto& arg) {
        if constexpr (
            std::is_same_v<std::decay_t<decltype(arg)>, std::string> ||
            std::is_same_v<std::decay_t<decltype(arg)>, std::string_view>) {
            result += arg;
        } else if constexpr (std::is_enum_v<std::decay_t<decltype(arg)>>) {
            // Convert enum to string using at::to_string
            result += at::to_string(arg);
        } else if constexpr (std::is_arithmetic_v<std::decay_t<decltype(arg)>>) {
            // Convert numeric values to string
            result += std::to_string(arg);
        } else {
            // For other types (like MacAddress), use string constructor
            result += std::string(arg);
        }
    };

    // Fold expression to append all arguments
    (append_arg(std::forward<Args>(args)), ...);

    return result;
}

} // namespace hm11::util

#endif // HM11_UTIL_AT_RESPONSE_UTILS_HPP