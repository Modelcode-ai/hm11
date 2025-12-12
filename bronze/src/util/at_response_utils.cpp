/**
 * @file at_response_utils.cpp
 * @brief Implementation of utility functions for parsing AT command responses
 */

#include "../../include/hm11/util/at_response_utils.hpp"

#include "../../include/hm11/hal/uart.hpp"

namespace hm11::util {

bool check_response(const std::string& response, const std::string& expected) {
    // Convert to string_view to avoid allocations
    std::string_view response_view(response);
    std::string_view expected_view(expected);

    return parser::starts_with(response_view, expected_view);
}

bool parse_mac_address(const std::string& response, const std::string& prefix, MacAddress& mac) {
    // Create dummy status for parser functions
    hal::UartStatus status = hal::UartStatus::Ok;

    // Convert to string_view to avoid allocations
    std::string_view response_view(response);
    std::string_view prefix_view(prefix);

    return parser::extract_mac_address(response_view, prefix_view, mac, status);
}

bool parse_version(const std::string& response, std::string& version) {
    // Look for version patterns: "V" (e.g., "V1.0") or "Version" (e.g., "Version 2.1")
    size_t version_start = std::string::npos;

    // First try to find "Version " (with space)
    auto version_pos = response.find("Version ");
    if (version_pos != std::string::npos) {
        version_start = version_pos;
    } else {
        // Try to find "V" followed by a digit or dot
        auto v_pos = response.find('V');
        if (v_pos != std::string::npos && v_pos + 1 < response.size()) {
            char next_char = response[v_pos + 1];
            // Check if it's likely a version (followed by digit or dot)
            if (std::isdigit(static_cast<unsigned char>(next_char)) || next_char == '.') {
                version_start = v_pos;
            }
        }
    }

    if (version_start == std::string::npos) {
        return false;
    }

    // Extract starting from version_start to the end or until we find whitespace/null
    size_t version_end = version_start + 1;
    while (version_end < response.size() && !std::isspace(static_cast<unsigned char>(response[version_end])) &&
           response[version_end] != '\0') {
        version_end++;
    }

    // For "Version " pattern, include the space and what follows
    if (response.substr(version_start, 8) == "Version ") {
        version_end = version_start + 8; // Start after "Version "
        while (version_end < response.size() && !std::isspace(static_cast<unsigned char>(response[version_end])) &&
               response[version_end] != '\0') {
            version_end++;
        }
        // Extract the full "Version X.Y" string
        version = response.substr(version_start, version_end - version_start);
    } else {
        version = response.substr(version_start, version_end - version_start);
    }

    return !version.empty();
}

} // namespace hm11::util

// The build_query_command implementation is already in at_command_builder.cpp