//===-------------- ATResponseParser.hpp ---------------------------===//
// Zero‑copy parsing utilities for AT command responses.
//===---------------------------------------------------------------===//

#ifndef HM11_AT_RESPONSE_PARSER_HPP
#define HM11_AT_RESPONSE_PARSER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace hm11 {
/**
 * \brief Zero-copy parsing utilities for AT command responses.
 *
 * All parsing functions are constexpr and operate on std::string_view to
 * avoid heap allocation. Parsed values are written to caller-provided
 * output parameters.
 *
 * \example
 * \code
 *   // Extract decimal integer
 *   unsigned int value = 0;
 *   if (extract_uint("12345", value) == ParseResult::Ok) {
 *     // value is now 12345
 *   }
 *
 *   // Parse MAC address
 *   std::array<std::uint8_t, 6> mac{};
 *   if (parse_mac("A1:B2:C3:D4:E5:F6", mac) == ParseResult::Ok) {
 *     // mac[0] is 0xA1, mac[5] is 0xF6
 *   }
 *
 *   // Parse sensor readings
 *   std::uint8_t temp = 0, hum = 0;
 *   if (parse_sensor_readings("25,60", temp, hum) == ParseResult::Ok) {
 *     // temp is 25, hum is 60
 *   }
 * \endcode
 */

/** \brief Result of a parsing operation. */
enum class ParseResult : std::uint8_t { Ok, Invalid, Overflow };

/** \brief Check if a character is a hex digit. */
constexpr bool is_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/** \brief Convert a hex digit to its numeric value. */
constexpr std::uint8_t hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
        return static_cast<std::uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<std::uint8_t>(c - 'a' + 10);
    }
    return static_cast<std::uint8_t>(c - 'A' + 10);
}

/** \brief Extract decimal unsigned integer. */
constexpr ParseResult extract_uint(std::string_view sv, unsigned int& out) noexcept {
    unsigned int value = 0U;
    if (sv.empty()) {
        return ParseResult::Invalid;
    }
    for (char c : sv) {
        if (c < '0' || c > '9') {
            return ParseResult::Invalid;
        }
        auto digit = static_cast<unsigned int>(c - '0');
        if (value > (std::numeric_limits<unsigned int>::max() - digit) / 10U) {
            return ParseResult::Overflow;
        }
        value = value * 10U + digit;
    }
    out = value;
    return ParseResult::Ok;
}

/** \brief Extract hexadecimal unsigned integer. */
constexpr ParseResult extract_hex(std::string_view sv, unsigned int& out) noexcept {
    unsigned int value = 0U;
    if (sv.empty()) {
        return ParseResult::Invalid;
    }
    for (char c : sv) {
        if (!is_hex_digit(c)) {
            return ParseResult::Invalid;
        }
        std::uint8_t digit = hex_value(c);
        if (value > (std::numeric_limits<unsigned int>::max() - digit) / 16U) {
            return ParseResult::Overflow;
        }
        value = (value << 4U) | digit;
    }
    out = value;
    return ParseResult::Ok;
}

/** \brief Split "key:value" view. */
constexpr ParseResult
split_key_value(std::string_view sv, char sep, std::string_view& key, std::string_view& value) noexcept {
    auto pos = sv.find(sep);
    if (pos == std::string_view::npos) {
        return ParseResult::Invalid;
    }
    key = sv.substr(0, pos);
    value = sv.substr(pos + 1);
    return ParseResult::Ok;
}

/** \brief Parse UUID (32 hex chars, optional dashes). */
constexpr ParseResult parse_uuid(std::string_view sv, std::array<std::uint8_t, 16>& out) noexcept {
    std::array<char, 32> buf{};
    std::size_t idx = 0U;
    for (char c : sv) {
        if (c == '-') {
            continue;
        }
        if (!is_hex_digit(c) || idx >= buf.size()) {
            return ParseResult::Invalid;
        }
        buf[idx++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    if (idx != buf.size()) {
        return ParseResult::Invalid;
    }
    // Parse using pointer iteration to avoid non‑constant index warnings
    const char* p = buf.data();
    for (auto& byte : out) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::uint8_t hi = hex_value(*p++);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::uint8_t lo = hex_value(*p++);
        byte = static_cast<std::uint8_t>((hi << 4U) | lo);
    }
    return ParseResult::Ok;
}

/** \brief Parse MAC address (12 hex chars, optional ':' or '-'). */
constexpr ParseResult parse_mac(std::string_view sv, std::array<std::uint8_t, 6>& out) noexcept {
    std::array<char, 12> buf{};
    std::size_t idx = 0U;
    for (char c : sv) {
        if (c == ':' || c == '-') {
            continue;
        }
        if (!is_hex_digit(c) || idx >= buf.size()) {
            return ParseResult::Invalid;
        }
        buf[idx++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    if (idx != buf.size()) {
        return ParseResult::Invalid;
    }
    // Parse using pointer iteration to avoid non‑constant index warnings
    const char* p = buf.data();
    for (auto& byte : out) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::uint8_t hi = hex_value(*p++);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::uint8_t lo = hex_value(*p++);
        byte = static_cast<std::uint8_t>((hi << 4U) | lo);
    }
    return ParseResult::Ok;
}

/** \brief Parse sensor readings "temp,hum". */
constexpr ParseResult
parse_sensor_readings(std::string_view sv, std::uint8_t& temperature, std::uint8_t& humidity) noexcept {
    std::string_view temp_str;
    std::string_view hum_str;
    if (split_key_value(sv, ',', temp_str, hum_str) != ParseResult::Ok) {
        return ParseResult::Invalid;
    }
    unsigned int temp = 0U;
    unsigned int hum = 0U;
    if (extract_uint(temp_str, temp) != ParseResult::Ok) {
        return ParseResult::Invalid;
    }
    if (extract_uint(hum_str, hum) != ParseResult::Ok) {
        return ParseResult::Invalid;
    }
    if (temp > 255U || hum > 255U) {
        return ParseResult::Overflow;
    }
    temperature = static_cast<std::uint8_t>(temp);
    humidity = static_cast<std::uint8_t>(hum);
    return ParseResult::Ok;
}

} // namespace hm11

#endif // HM11_AT_RESPONSE_PARSER_HPP
