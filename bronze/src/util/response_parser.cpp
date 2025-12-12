/**
 * @file response_parser.cpp
 * @brief Implementation of response parsing utilities
 */

#include "../../include/hm11/util/response_parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace hm11::util::parser {

[[nodiscard]] bool contains(std::string_view response, std::string_view expected) {
    return response.find(expected) != std::string_view::npos;
}

[[nodiscard]] bool starts_with(std::string_view response, std::string_view expected) {
    return response.size() >= expected.size() && response.starts_with(expected);
}

[[nodiscard]] std::optional<size_t> find(std::string_view response, std::string_view substring) {
    auto pos = response.find(substring);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    return pos;
}

[[nodiscard]] std::string_view extract(std::string_view response, size_t start_pos, size_t end_pos) {
    if (start_pos >= response.size()) {
        return {};
    }
    return response.substr(
        start_pos,
        (end_pos == std::string_view::npos) ? end_pos : std::min(end_pos - start_pos, response.size() - start_pos));
}

[[nodiscard]] std::optional<std::string_view>
extract_after(std::string_view response, std::string_view prefix, char end_delimiter) {
    auto prefix_pos = find(response, prefix);
    if (!prefix_pos) {
        return std::nullopt;
    }

    size_t start_pos = *prefix_pos + prefix.size();
    // Allow empty values - start_pos == response.size() is valid (empty string after prefix)
    if (start_pos > response.size()) {
        return std::nullopt;
    }

    size_t end_pos = response.size();
    if (end_delimiter != '\0') {
        auto delimiter_pos = response.find(end_delimiter, start_pos);
        if (delimiter_pos != std::string_view::npos) {
            end_pos = delimiter_pos;
        }
    }

    return extract(response, start_pos, end_pos);
}

[[nodiscard]] Role extract_role(std::string_view response, hal::UartStatus& status) {
    auto value_opt = extract_after(response, at::response::OK_GET_PREFIX);
    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return Role::Peripheral; // Default to peripheral on error
    }

    char role_char = (*value_opt)[0];
    if (role_char == '0') {
        status = hal::UartStatus::Ok;
        return Role::Peripheral;
    }

    if (role_char == '1') {
        status = hal::UartStatus::Ok;
        return Role::Central;
    }

    status = hal::UartStatus::ErrError;
    return Role::Peripheral; // Default to peripheral on error
}

bool extract_mac_address(std::string_view response, std::string_view prefix, MacAddress& mac, hal::UartStatus& status) {
    auto value_opt = extract_after(response, prefix);
    if (!value_opt || value_opt->empty() || value_opt->size() < MacAddress::size()) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Get the MAC address substring
    auto mac_str = value_opt->substr(0, MacAddress::size());

    // Validate the MAC address BEFORE copying (all characters should be hex)
    for (char c : mac_str) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F')) {
            status = hal::UartStatus::ErrError;
            return false;
        }
    }

    // Only copy if validation passed
    std::ranges::copy(mac_str, mac.begin());

    status = hal::UartStatus::Ok;
    return true;
}

bool extract_version(std::string_view response, VersionType& version, hal::UartStatus& status) {
    // Look for "V" followed by version number (e.g., "V1.0", "V5.6")
    auto v_pos = response.find('V');
    if (v_pos == std::string_view::npos) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Extract starting from 'V' to the end or until we find a non-version character
    // Version format is typically "V" followed by characters until whitespace/end
    size_t version_start = v_pos;
    size_t version_end = v_pos + 1;

    // Find the end of the version string (whitespace, null, or end of string)
    while (version_end < response.size() && std::isspace(static_cast<unsigned char>(response[version_end])) == 0 &&
           response[version_end] != '\0') {
        version_end++;
    }

    auto version_str = response.substr(version_start, version_end - version_start);
    if (version_str.empty() || version_str.size() > 4) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Pad with spaces if needed to make it exactly 4 characters
    std::string padded_version(version_str);
    while (padded_version.size() < 4) {
        padded_version += ' ';
    }

    // Direct assignment from string to VersionType
    version = VersionType(padded_version.c_str());
    status = hal::UartStatus::Ok;
    return true;
}

bool extract_name(std::string_view response, char* name, std::size_t max_length, hal::UartStatus& status) {
    auto value_opt = extract_after(response, at::response::OK_GET_PREFIX);
    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Copy the name characters, limited by max_length
    auto name_length = std::min(value_opt->size(), max_length - 1);
    std::copy_n(value_opt->begin(), name_length, name);
    name[name_length] = '\0'; // Ensure null-termination

    status = hal::UartStatus::Ok;
    return true;
}

[[nodiscard]] bool is_success(std::string_view response) {
    return contains(response, at::response::OK);
}

[[nodiscard]] bool is_error(std::string_view response) {
    return contains(response, at::response::ERROR);
}

[[nodiscard]] std::string_view to_string_view(const hal::UInt8* buffer, std::size_t max_size) {
    // Find the null terminator if present
    const hal::UInt8* null_terminator = std::find(buffer, buffer + max_size, 0);
    auto length = static_cast<std::size_t>(null_terminator - buffer);

    // Create a string_view from the buffer (up to the null terminator if found)
    // Using reinterpret_cast is necessary when converting between unsigned char* and char*
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char*>(buffer), length};
}

[[nodiscard]] std::string_view to_string_view(const char* buffer, std::size_t max_size) {
    // Find the null terminator if present
    const char* null_terminator = std::find(buffer, buffer + max_size, '\0');
    auto length = static_cast<std::size_t>(null_terminator - buffer);

    // Create a string_view from the buffer (up to the null terminator if found)
    return {buffer, length};
}

} // namespace hm11::util::parser