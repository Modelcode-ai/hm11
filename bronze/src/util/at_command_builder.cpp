/**
 * @file at_command_builder.cpp
 * @brief Implementation of AT command builder utilities
 */

#include "../../include/hm11/util/at_command_builder.hpp"

namespace hm11::util::at {

[[nodiscard]] std::string build_query_command(std::string_view command) {
    std::string result;
    result.reserve(command.size() + 1); // Reserve space for command + "?"
    result.append(command);
    result.push_back('?');
    return result;
}

[[nodiscard]] std::string build_set_command(std::string_view command, std::string_view value) {
    std::string result;
    result.reserve(command.size() + value.size()); // Reserve space for command + value
    result.append(command);
    result.append(value);
    return result;
}

[[nodiscard]] std::string build_set_command(std::string_view command, const std::string& value) {
    return build_set_command(command, std::string_view(value));
}

[[nodiscard]] std::string build_hex_command(std::string_view command, std::string_view value) {
    std::string result;
    constexpr std::string_view HEX_PREFIX = "0x";
    result.reserve(command.size() + HEX_PREFIX.size() + value.size()); // Reserve space for command + "0x" + value
    result.append(command);
    result.append(HEX_PREFIX);
    result.append(value);
    return result;
}

[[nodiscard]] std::string build_connect_command(const MacAddress& mac) {
    return build_set_command(cmd::CON, std::string_view(mac.get_data(), MacAddress::size()));
}

[[nodiscard]] std::string build_scan_command(uint8_t seconds) {
    if (seconds == 0) {
        return std::string(cmd::SCAN); // Indefinite scan
    }

    // Convert seconds to a string
    std::string seconds_str = std::to_string(seconds);
    return build_set_command(cmd::SCAN, seconds_str);
}

} // namespace hm11::util::at