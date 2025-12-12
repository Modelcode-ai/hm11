#ifndef HM11_UTIL_PARSING_UTILS_HPP
#define HM11_UTIL_PARSING_UTILS_HPP

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>

namespace hm11::util {

/// @brief Convert UART byte buffer to string_view for zero-copy parsing (Decision 6)
///
/// This function provides a type-safe conversion from uint8_t buffers (UART HAL standard)
/// to string_view for parsing operations. The conversion is safe and compliant with
/// embedded requirements:
///
/// **C++ Standard Guarantees:**
/// - C++20 [basic.types.general]/2: char can alias any object type
/// - C++20 [basic.lval]/11.8: Accessing uint8_t through const char* is explicitly allowed
/// - uint8_t and char are both 8-bit byte-compatible types with identical representation
///
/// **Embedded Compliance:**
/// - Zero heap allocation (no malloc/new)
/// - Zero data copying (just pointer cast)
/// - Deterministic performance (O(1) constant time)
/// - ISR-safe (read-only access, no locks)
/// - Cache-friendly (16-byte string_view = 2 pointers)
/// - Exception-free (all operations noexcept)
///
/// **Memory Safety:**
/// - Only const access (read-only), preventing modification through wrong type
/// - ARM Cortex-M: char = 8 bits, uint8_t = 8 bits (byte-compatible at hardware level)
/// - No alignment issues (both 1-byte aligned)
/// - No undefined behavior per C++ standard
///
/// @param bytes Byte buffer from UART receive operation
/// @return string_view for zero-copy parsing (no heap allocation, no data copy)
///
/// @note This is a zero-cost abstraction - compiler inlines to single pointer cast
/// @note Returned view is valid only while source buffer remains valid
/// @note Replaces 27 scattered reinterpret_cast calls with centralized, documented conversion
///
/// **Usage Example:**
/// @code
/// hal::UartData8b buffer(response_buffer_.data(), 256);
/// uart_port_.receive(buffer, status);
/// std::string_view response = util::to_string_view(buffer);  // Zero-copy
/// std::string_view value = response.substr(7, 3);            // Zero-copy substring
/// @endcode
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
// Required for byte/char type conversion - uint8_t and char are byte-compatible per C++20 standard
// This is the ONLY location where this cast should appear - all other code should use this helper
[[nodiscard]] inline std::string_view to_string_view(std::span<const uint8_t> bytes) noexcept {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

/// @brief Check if a string_view starts with a given prefix
/// @param view The string_view to check
/// @param prefix The prefix to search for
/// @return true if view starts with prefix, false otherwise
/// @note This is a constexpr zero-copy operation
constexpr bool starts_with(std::string_view view, std::string_view prefix) noexcept {
    return view.starts_with(prefix);
}

/// @brief Check if a byte span starts with a given prefix
/// @param view The byte span to check
/// @param prefix The prefix to search for
/// @return true if view starts with prefix, false otherwise
/// @note This is a constexpr zero-copy operation
constexpr bool starts_with(std::span<const uint8_t> view, std::span<const uint8_t> prefix) noexcept {
    if (view.size() < prefix.size()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), view.begin());
}

/// @brief Check if a string_view ends with a given suffix
/// @param view The string_view to check
/// @param suffix The suffix to search for
/// @return true if view ends with suffix, false otherwise
/// @note This is a constexpr zero-copy operation
constexpr bool ends_with(std::string_view view, std::string_view suffix) noexcept {
    return view.size() >= suffix.size() && view.substr(view.size() - suffix.size()) == suffix;
}

/// @brief Check if a byte span ends with a given suffix
/// @param view The byte span to check
/// @param suffix The suffix to search for
/// @return true if view ends with suffix, false otherwise
/// @note This is a constexpr zero-copy operation
constexpr bool ends_with(std::span<const uint8_t> view, std::span<const uint8_t> suffix) noexcept {
    if (view.size() < suffix.size()) {
        return false;
    }
    return std::equal(suffix.begin(), suffix.end(), view.end() - static_cast<std::ptrdiff_t>(suffix.size()));
}

/// @brief Find the first occurrence of a substring in a string_view
/// @param view The string_view to search in
/// @param pattern The pattern to search for
/// @return The position of the first occurrence, or std::string_view::npos if
/// not found
/// @note This is a constexpr zero-copy operation
constexpr std::size_t find(std::string_view view, std::string_view pattern) noexcept {
    return view.find(pattern);
}

/// @brief Find the first occurrence of a byte pattern in a byte span
/// @param view The byte span to search in
/// @param pattern The pattern to search for
/// @return The position of the first occurrence, or view.size() if not found
/// @note This is a zero-copy operation
constexpr std::size_t find(std::span<const uint8_t> view, std::span<const uint8_t> pattern) noexcept {
    if (pattern.empty() || view.size() < pattern.size()) {
        return view.size();
    }

    auto it = std::ranges::search(view, pattern);
    if (it.empty()) {
        return view.size();
    }
    return static_cast<std::size_t>(std::distance(view.begin(), it.begin()));
}

/// @brief CRLF sequence as a string_view constant
inline constexpr std::string_view K_CRLF = "\r\n";

/// @brief CRLF sequence as byte array
inline constexpr std::array<uint8_t, 2> K_CRLF_BYTES = {'\r', '\n'};

/// @brief Find the first occurrence of CRLF in a string_view
/// @param view The string_view to search in
/// @return The position of CRLF, or std::string_view::npos if not found
constexpr std::size_t find_crlf(std::string_view view) noexcept {
    return view.find(K_CRLF);
}

/// @brief Find the first occurrence of CRLF in a byte span
/// @param view The byte span to search in
/// @return The position of CRLF, or view.size() if not found
constexpr std::size_t find_crlf(std::span<const uint8_t> view) noexcept {
    return find(view, std::span<const uint8_t>(K_CRLF_BYTES));
}

/// @brief Trim CRLF from the end of a string_view
/// @param view The string_view to trim
/// @return A new string_view without trailing CRLF
constexpr std::string_view trim_crlf(std::string_view view) noexcept {
    if (ends_with(view, K_CRLF)) {
        return view.substr(0, view.size() - K_CRLF.size());
    }
    return view;
}

/// @brief Trim CRLF from the end of a byte span
/// @param view The byte span to trim
/// @return A new span without trailing CRLF
constexpr std::span<const uint8_t> trim_crlf(std::span<const uint8_t> view) noexcept {
    if (ends_with(view, std::span<const uint8_t>(K_CRLF_BYTES))) {
        return view.subspan(0, view.size() - K_CRLF_BYTES.size());
    }
    return view;
}

/// @brief Check if a character is a valid hexadecimal digit
/// @param c The character to check
/// @return true if c is in [0-9A-Fa-f], false otherwise
constexpr bool is_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/// @brief Check if a byte is a valid hexadecimal digit
/// @param byte The byte to check
/// @return true if byte represents a hex digit, false otherwise
constexpr bool is_hex_digit(uint8_t byte) noexcept {
    return is_hex_digit(static_cast<char>(byte));
}

/// @brief Convert a hexadecimal character to its integer value
/// @param c The hex character to convert
/// @return The integer value (0-15), or std::nullopt if invalid
constexpr std::optional<uint8_t> hex_char_to_int(char c) noexcept {
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(c - 'a' + 10);
    }
    return std::nullopt;
}

/// @brief Convert a hexadecimal byte to its integer value
/// @param byte The hex byte to convert
/// @return The integer value (0-15), or std::nullopt if invalid
constexpr std::optional<uint8_t> hex_char_to_int(uint8_t byte) noexcept {
    return hex_char_to_int(static_cast<char>(byte));
}

/// @brief Validate that a string_view contains only hexadecimal digits
/// @param view The string_view to validate
/// @return true if all characters are hex digits, false otherwise
constexpr bool is_all_hex(std::string_view view) noexcept {
    return std::ranges::all_of(view, [](char c) { return is_hex_digit(c); });
}

/// @brief Validate that a byte span contains only hexadecimal digits
/// @param view The byte span to validate
/// @return true if all bytes are hex digits, false otherwise
constexpr bool is_all_hex(std::span<const uint8_t> view) noexcept {
    return std::ranges::all_of(view, [](uint8_t b) { return is_hex_digit(b); });
}

/// @brief Check if a string_view contains only decimal digits
/// @param view The string_view to check
/// @return true if all characters are decimal digits, false otherwise
constexpr bool is_all_digits(std::string_view view) noexcept {
    return std::ranges::all_of(view, [](char c) { return c >= '0' && c <= '9'; });
}

/// @brief Convert a decimal digit character to its integer value
/// @param c The digit character to convert
/// @return The integer value (0-9)
/// @note Assumes c is a valid decimal digit ('0'-'9'). Behavior is undefined for other characters.
constexpr uint8_t char_to_digit(char c) noexcept {
    return static_cast<uint8_t>(c - '0');
}

/// @brief Parse a fixed-length hexadecimal field from a string_view
/// @tparam N The expected number of hex characters
/// @param view The string_view to parse from (must be at least N characters)
/// @return A string_view of exactly N characters if valid, std::nullopt
/// otherwise
/// @note The returned view is a zero-copy slice of the input view
template <std::size_t N> constexpr std::optional<std::string_view> parse_hex_field(std::string_view view) noexcept {
    if (view.size() < N) {
        return std::nullopt;
    }
    std::string_view field = view.substr(0, N);
    if (!is_all_hex(field)) {
        return std::nullopt;
    }
    return field;
}

/// @brief Parse a fixed-length hexadecimal field from a byte span
/// @tparam N The expected number of hex bytes
/// @param view The byte span to parse from (must be at least N bytes)
/// @return A span of exactly N bytes if valid, std::nullopt otherwise
/// @note The returned span is a zero-copy slice of the input span
template <std::size_t N>
constexpr std::optional<std::span<const uint8_t>> parse_hex_field(std::span<const uint8_t> view) noexcept {
    if (view.size() < N) {
        return std::nullopt;
    }
    std::span<const uint8_t> field = view.subspan(0, N);
    if (!is_all_hex(field)) {
        return std::nullopt;
    }
    return field;
}

/// @brief Parse a MAC address (12 hex characters) from a string_view
/// @param view The string_view to parse from
/// @return A string_view of the MAC address if valid, std::nullopt otherwise
constexpr std::optional<std::string_view> parse_mac_address(std::string_view view) noexcept {
    return parse_hex_field<12>(view);
}

/// @brief Parse a MAC address (12 hex bytes) from a byte span
/// @param view The byte span to parse from
/// @return A span of the MAC address if valid, std::nullopt otherwise
constexpr std::optional<std::span<const uint8_t>> parse_mac_address(std::span<const uint8_t> view) noexcept {
    return parse_hex_field<12>(view);
}

/// @brief Parse an iBeacon UUID component (4 hex characters) from a
/// string_view
/// @param view The string_view to parse from
/// @return A string_view of the UUID component if valid, std::nullopt otherwise
constexpr std::optional<std::string_view> parse_uuid_component(std::string_view view) noexcept {
    return parse_hex_field<4>(view);
}

/// @brief Parse an integer from a string_view
/// @tparam T The integer type to parse into (must be integral)
/// @param view The string_view to parse from
/// @return The parsed integer value, or std::nullopt if parsing fails
/// @note Uses std::from_chars for efficient parsing
template <typename T>
    requires std::is_integral_v<T>
constexpr std::optional<T> parse_int(std::string_view view) noexcept {
    T value{};
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), value);
    if (ec == std::errc{} && ptr == view.data() + view.size()) {
        return value;
    }
    return std::nullopt;
}

/// @brief Parse a temperature value from a string_view
/// @param view The string_view containing the temperature string
/// @return The parsed temperature (0-120), or std::nullopt if invalid
/// @note Temperature is expected to be in range 0-120
constexpr std::optional<uint8_t> parse_temperature(std::string_view view) noexcept {
    auto value = parse_int<int>(view);
    if (!value || *value < 0 || *value > 120) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(*value);
}

/// @brief Parse a humidity value from a string_view
/// @param view The string_view containing the humidity string
/// @return The parsed humidity (0-100), or std::nullopt if invalid
/// @note Humidity is expected to be in range 0-100
constexpr std::optional<uint8_t> parse_humidity(std::string_view view) noexcept {
    auto value = parse_int<int>(view);
    if (!value || *value < 0 || *value > 100) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(*value);
}

/// @brief Split a string_view on CRLF, returning the part before CRLF
/// @param view The string_view to split
/// @return A pair of (before_crlf, remaining_after_crlf)
/// @note If CRLF not found, returns (view, empty_view)
constexpr std::pair<std::string_view, std::string_view> split_at_crlf(std::string_view view) noexcept {
    std::size_t pos = find_crlf(view);
    if (pos == std::string_view::npos) {
        return {view, std::string_view{}};
    }
    return {view.substr(0, pos), view.substr(pos + K_CRLF.size())};
}

/// @brief Split a byte span on CRLF, returning the part before CRLF
/// @param view The byte span to split
/// @return A pair of (before_crlf, remaining_after_crlf)
/// @note If CRLF not found, returns (view, empty_span)
constexpr std::pair<std::span<const uint8_t>, std::span<const uint8_t>> split_at_crlf(
    std::span<const uint8_t> view) noexcept {
    std::size_t pos = find_crlf(view);
    if (pos == view.size()) {
        return {view, std::span<const uint8_t>{}};
    }
    return {view.subspan(0, pos), view.subspan(pos + K_CRLF_BYTES.size())};
}

/// @brief Convert a string_view to a byte span (zero-copy)
/// @param str The string_view to convert
/// @return A byte span of the same data
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
// Required for byte/char type conversions
inline std::span<const uint8_t> to_byte_span(std::string_view str) noexcept {
    return {reinterpret_cast<const uint8_t*>(str.data()), str.size()};
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

} // namespace hm11::util

#endif // HM11_UTIL_PARSING_UTILS_HPP
