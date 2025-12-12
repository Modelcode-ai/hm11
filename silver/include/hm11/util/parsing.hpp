#pragma once

// HM11 Driver Parsing Utilities
// Implements zero-copy parsing utilities for AT command responses
// Resolves Design Decision #6 (Response Parsing Strategy)

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <system_error>

#include "../types/type_aliases.hpp"

// Forward declaration for VariableString
namespace hm11 {
template <std::size_t Capacity> class VariableString;
}

namespace hm11::util {

/// @brief Result type for parsing operations
/// @tparam T Type of the parsed value
template <typename T> struct ParseResult {
    T value{};                  ///< Parsed value (only valid if success is true)
    bool success{false};        ///< True if parsing succeeded
    std::string_view remaining; ///< Remaining unparsed portion of input

    /// @brief Check if parsing was successful
    constexpr bool has_value() const noexcept { return success; }

    /// @brief Get the parsed value (only call if has_value() returns true)
    constexpr const T& operator*() const noexcept { return value; }

    /// @brief Get pointer to the parsed value (only call if has_value() returns true)
    constexpr const T* operator->() const noexcept { return &value; }
};

// ============================================================================
// Basic parsing functions
// ============================================================================

/// @brief Parse integer from string view
/// @param sv String view to parse from
/// @param base Numeric base (10 for decimal, 16 for hex)
/// @return ParseResult with parsed integer
template <std::integral T> ParseResult<T> parse_int(std::string_view sv, int base = 10) noexcept {
    if (sv.empty()) {
        return {T{}, false, sv};
    }

    T value{};
    const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, base);

    const bool success = (ec == std::errc{});
    const std::size_t parsed_length = success ? static_cast<std::size_t>(ptr - sv.data()) : 0;
    const std::string_view remaining = sv.substr(parsed_length);

    return {value, success, remaining};
}

/// @brief Parse unsigned integer from string view
/// @param sv String view to parse from
/// @param base Numeric base (10 for decimal, 16 for hex)
/// @return ParseResult with parsed unsigned integer
template <std::unsigned_integral T> ParseResult<T> parse_uint(std::string_view sv, int base = 10) noexcept {
    return parse_int<T>(sv, base);
}

/// @brief Parse hex character to numeric value
/// @param c Hex character ('0'-'9', 'A'-'F', 'a'-'f')
/// @return ParseResult with numeric value (0-15)
inline ParseResult<uint8_t> parse_hex_char(char c) noexcept {
    if (c >= '0' && c <= '9') {
        return {static_cast<uint8_t>(c - '0'), true, {}};
    }
    if (c >= 'A' && c <= 'F') {
        return {static_cast<uint8_t>(c - 'A' + 10), true, {}};
    }
    if (c >= 'a' && c <= 'f') {
        return {static_cast<uint8_t>(c - 'a' + 10), true, {}};
    }
    return {0, false, {}};
}

/// @brief Parse boolean from string ("0" = false, "1" = true)
/// @param sv String view to parse from
/// @return ParseResult with parsed boolean
inline ParseResult<bool> parse_bool(std::string_view sv) noexcept {
    if (sv.empty()) {
        return {false, false, sv};
    }

    if (sv[0] == '0') {
        return {false, true, sv.substr(1)};
    }
    if (sv[0] == '1') {
        return {true, true, sv.substr(1)};
    }
    return {false, false, sv};
}

// ============================================================================
// String tokenization and searching
// ============================================================================

/// @brief Find substring in string view
/// @param haystack String to search in
/// @param needle Substring to find
/// @param start_pos Starting position for search
/// @return Position of first occurrence or std::string_view::npos
constexpr std::size_t find(std::string_view haystack, std::string_view needle, std::size_t start_pos = 0) noexcept {
    return haystack.find(needle, start_pos);
}

/// @brief Find character in string view
/// @param haystack String to search in
/// @param needle Character to find
/// @param start_pos Starting position for search
/// @return Position of first occurrence or std::string_view::npos
constexpr std::size_t find(std::string_view haystack, char needle, std::size_t start_pos = 0) noexcept {
    return haystack.find(needle, start_pos);
}

/// @brief Check if string view starts with given prefix
/// @param str String to check
/// @param prefix Prefix to look for
/// @return true if str starts with prefix
constexpr bool starts_with(std::string_view str, std::string_view prefix) noexcept {
    return str.starts_with(prefix);
}

/// @brief Check if string view ends with given suffix
/// @param str String to check
/// @param suffix Suffix to look for
/// @return true if str ends with suffix
constexpr bool ends_with(std::string_view str, std::string_view suffix) noexcept {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

/// @brief Trim whitespace from beginning and end of string view
/// @param str String to trim
/// @return Trimmed string view
constexpr std::string_view trim(std::string_view str) noexcept {
    const auto is_whitespace = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    };

    // Trim from start
    while (!str.empty() && is_whitespace(str.front())) {
        str.remove_prefix(1);
    }

    // Trim from end
    while (!str.empty() && is_whitespace(str.back())) {
        str.remove_suffix(1);
    }

    return str;
}

/// @brief Split string view at first occurrence of delimiter
/// @param str String to split
/// @param delimiter Character delimiter
/// @return Pair of {before_delimiter, after_delimiter}
constexpr std::pair<std::string_view, std::string_view> split_once(std::string_view str, char delimiter) noexcept {
    const std::size_t pos = str.find(delimiter);
    if (pos == std::string_view::npos) {
        return {str, {}};
    }
    return {str.substr(0, pos), str.substr(pos + 1)};
}

/// @brief Split string view at first occurrence of delimiter string
/// @param str String to split
/// @param delimiter String delimiter
/// @return Pair of {before_delimiter, after_delimiter}
constexpr std::pair<std::string_view, std::string_view> split_once(
    std::string_view str,
    std::string_view delimiter) noexcept {
    const std::size_t pos = str.find(delimiter);
    if (pos == std::string_view::npos) {
        return {str, {}};
    }
    return {str.substr(0, pos), str.substr(pos + delimiter.size())};
}

// ============================================================================
// HM11-specific parsing utilities
// ============================================================================

/// @brief Parse Role enum from character ('0' = Peripheral, '1' = Central)
/// @param c Character to parse
/// @return ParseResult with Role value
inline ParseResult<Role> parse_role(char c) noexcept {
    if (c == '0') {
        return {Role::Peripheral, true, {}};
    }
    if (c == '1') {
        return {Role::Central, true, {}};
    }
    return {Role::Peripheral, false, {}};
}

/// @brief Parse AdvertisingInterval from character
/// Maps characters to enum values using reverse of to_char() mapping
/// @param c Character to parse ('0'-'9', 'A'-'F')
/// @return ParseResult with AdvertisingInterval value
inline ParseResult<AdvertisingInterval> parse_advertising_interval(char c) noexcept {
    switch (c) {
        case '0':
            return {AdvertisingInterval::Ms100, true, {}};
        case '1':
            return {AdvertisingInterval::Ms211, true, {}};
        case '2':
            return {AdvertisingInterval::Ms252, true, {}};
        case '3':
            return {AdvertisingInterval::Ms318, true, {}};
        case '4':
            return {AdvertisingInterval::Ms417, true, {}};
        case '5':
            return {AdvertisingInterval::Ms546, true, {}};
        case '6':
            return {AdvertisingInterval::Ms760, true, {}};
        case '7':
            return {AdvertisingInterval::Ms852, true, {}};
        case '8':
            return {AdvertisingInterval::Ms1022, true, {}};
        case '9':
            return {AdvertisingInterval::Ms1285, true, {}};
        case 'A':
        case 'a':
            return {AdvertisingInterval::Ms2000, true, {}};
        case 'B':
        case 'b':
            return {AdvertisingInterval::Ms3000, true, {}};
        case 'C':
        case 'c':
            return {AdvertisingInterval::Ms4000, true, {}};
        case 'D':
        case 'd':
            return {AdvertisingInterval::Ms5000, true, {}};
        case 'E':
        case 'e':
            return {AdvertisingInterval::Ms6000, true, {}};
        case 'F':
        case 'f':
            return {AdvertisingInterval::Ms7000, true, {}};
        default:
            return {AdvertisingInterval::Ms100, false, {}};
    }
}

/// @brief Parse AdvertisingType from character
/// @param c Character to parse ('0'-'3')
/// @return ParseResult with AdvertisingType value
inline ParseResult<AdvertisingType> parse_advertising_type(char c) noexcept {
    switch (c) {
        case '0':
            return {AdvertisingType::AdvertisingScanResponseConnectable, true, {}};
        case '1':
            return {AdvertisingType::LastDeviceConnect, true, {}};
        case '2':
            return {AdvertisingType::AdvertisingScanResponse, true, {}};
        case '3':
            return {AdvertisingType::Advertising, true, {}};
        default:
            return {AdvertisingType::AdvertisingScanResponseConnectable, false, {}};
    }
}

/// @brief Extract "OK+Get:" response value
/// @param response Full response string
/// @return ParseResult with the value portion after "OK+Get:"
inline ParseResult<std::string_view> extract_get_response(std::string_view response) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";

    if (!starts_with(response, OK_GET_PREFIX)) {
        return {{}, false, response};
    }

    const std::string_view value = response.substr(OK_GET_PREFIX.size());
    return {value, true, {}};
}

/// @brief Extract "OK+Set:" response value
/// @param response Full response string
/// @return ParseResult with the value portion after "OK+Set:"
inline ParseResult<std::string_view> extract_set_response(std::string_view response) noexcept {
    constexpr std::string_view OK_SET_PREFIX = "OK+Set:";

    if (!starts_with(response, OK_SET_PREFIX)) {
        return {{}, false, response};
    }

    const std::string_view value = response.substr(OK_SET_PREFIX.size());
    return {value, true, {}};
}

/// @brief Parse ConnectResult from character (Ada's To_Connect_Result)
/// Matches exactly the Ada implementation:
/// - 'L' = Connecting
/// - 'E' = Connect_Error
/// - 'F' = Connect_Fail
/// - 'N' = No_Address
/// - ' ', NUL, '0'-'5' = Connected
/// - others = Other_Error
/// @param c Character to parse
/// @return ParseResult with ConnectResult value
inline ParseResult<ConnectResult> parse_connect_result(char c) noexcept {
    switch (c) {
        case 'L':
            return {ConnectResult::Connecting, true, {}};
        case 'E':
            return {ConnectResult::ConnectError, true, {}};
        case 'F':
            return {ConnectResult::ConnectFail, true, {}};
        case 'N':
            return {ConnectResult::NoAddress, true, {}};
        case ' ':
        case '\0':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
            return {ConnectResult::Connected, true, {}};
        default:
            return {ConnectResult::OtherError, true, {}};
    }
}

/// @brief Parse SensorType from character ('0' = None, '1' = DHT11, '2' = DS18B20)
/// @param c Character to parse
/// @return ParseResult with SensorType value
inline ParseResult<SensorType> parse_sensor_type(char c) noexcept {
    switch (c) {
        case '0':
            return {SensorType::None, true, {}};
        case '1':
            return {SensorType::DHT11, true, {}};
        case '2':
            return {SensorType::DS18B20, true, {}};
        default:
            return {SensorType::None, false, {}};
    }
}

/// @brief Parse UartBaudRate from character ('0'-'8')
/// @param c Character to parse
/// @return ParseResult with UartBaudRate value
inline ParseResult<UartBaudRate> parse_uart_baud_rate(char c) noexcept {
    switch (c) {
        case '0':
            return {UartBaudRate::Br9600, true, {}};
        case '1':
            return {UartBaudRate::Br19200, true, {}};
        case '2':
            return {UartBaudRate::Br38400, true, {}};
        case '3':
            return {UartBaudRate::Br57600, true, {}};
        case '4':
            return {UartBaudRate::Br115200, true, {}};
        case '5':
            return {UartBaudRate::Br4800, true, {}};
        case '6':
            return {UartBaudRate::Br2400, true, {}};
        case '7':
            return {UartBaudRate::Br1200, true, {}};
        case '8':
            return {UartBaudRate::Br230400, true, {}};
        default:
            return {UartBaudRate::Br9600, false, {}};
    }
}

/// @brief Parse ModulePower from character ('0'-'3')
/// @param c Character to parse
/// @return ParseResult with ModulePower value
inline ParseResult<ModulePower> parse_module_power(char c) noexcept {
    switch (c) {
        case '0':
            return {ModulePower::DbmMinus23, true, {}};
        case '1':
            return {ModulePower::DbmMinus6, true, {}};
        case '2':
            return {ModulePower::Dbm0, true, {}};
        case '3':
            return {ModulePower::Dbm6, true, {}};
        default:
            return {ModulePower::Dbm0, false, {}};
    }
}

/// @brief Parse OutputPower from character ('0'-'1')
/// @param c Character to parse
/// @return ParseResult with OutputPower value
inline ParseResult<OutputPower> parse_output_power(char c) noexcept {
    switch (c) {
        case '0':
            return {OutputPower::NormalPower, true, {}};
        case '1':
            return {OutputPower::MaxPower, true, {}};
        default:
            return {OutputPower::MaxPower, false, {}};
    }
}

/// @brief Parse AdvertisingMode from character ('0'-'1')
/// @param c Character to parse
/// @return ParseResult with AdvertisingMode value
inline ParseResult<AdvertisingMode> parse_advertising_mode(char c) noexcept {
    switch (c) {
        case '0':
            return {AdvertisingMode::NormalAdvertising, true, {}};
        case '1':
            return {AdvertisingMode::ReliableAdvertising, true, {}};
        default:
            return {AdvertisingMode::NormalAdvertising, false, {}};
    }
}

/// @brief Parse ParityBit from character ('0'-'2')
/// @param c Character to parse
/// @return ParseResult with ParityBit value
inline ParseResult<ParityBit> parse_parity_bit(char c) noexcept {
    switch (c) {
        case '0':
            return {ParityBit::None, true, {}};
        case '1':
            return {ParityBit::Odd, true, {}};
        case '2':
            return {ParityBit::Even, true, {}};
        default:
            return {ParityBit::None, false, {}};
    }
}

/// @brief Parse StopBit from character ('0'-'1')
/// @param c Character to parse
/// @return ParseResult with StopBit value
inline ParseResult<StopBit> parse_stop_bit(char c) noexcept {
    switch (c) {
        case '0':
            return {StopBit::OneStopBit, true, {}};
        case '1':
            return {StopBit::TwoStopBit, true, {}};
        default:
            return {StopBit::OneStopBit, false, {}};
    }
}

/// @brief Parse LinkLayerConnectionInterval from character ('0'-'9')
/// @param c Character to parse
/// @return ParseResult with LinkLayerConnectionInterval value
inline ParseResult<LinkLayerConnectionInterval> parse_link_layer_connection_interval(char c) noexcept {
    switch (c) {
        case '0':
            return {LinkLayerConnectionInterval::Ms7, true, {}};
        case '1':
            return {LinkLayerConnectionInterval::Ms10, true, {}};
        case '2':
            return {LinkLayerConnectionInterval::Ms15, true, {}};
        case '3':
            return {LinkLayerConnectionInterval::Ms20, true, {}};
        case '4':
            return {LinkLayerConnectionInterval::Ms25, true, {}};
        case '5':
            return {LinkLayerConnectionInterval::Ms30, true, {}};
        case '6':
            return {LinkLayerConnectionInterval::Ms35, true, {}};
        case '7':
            return {LinkLayerConnectionInterval::Ms40, true, {}};
        case '8':
            return {LinkLayerConnectionInterval::Ms45, true, {}};
        case '9':
            return {LinkLayerConnectionInterval::Ms4000, true, {}};
        default:
            return {LinkLayerConnectionInterval::Ms20, false, {}};
    }
}

/// @brief Parse ConnectionSupervisionTimeout from character ('0'-'6')
/// @param c Character to parse
/// @return ParseResult with ConnectionSupervisionTimeout value
inline ParseResult<ConnectionSupervisionTimeout> parse_connection_supervision_timeout(char c) noexcept {
    switch (c) {
        case '0':
            return {ConnectionSupervisionTimeout::Ms100, true, {}};
        case '1':
            return {ConnectionSupervisionTimeout::Ms1000, true, {}};
        case '2':
            return {ConnectionSupervisionTimeout::Ms2000, true, {}};
        case '3':
            return {ConnectionSupervisionTimeout::Ms3000, true, {}};
        case '4':
            return {ConnectionSupervisionTimeout::Ms4000, true, {}};
        case '5':
            return {ConnectionSupervisionTimeout::Ms5000, true, {}};
        case '6':
            return {ConnectionSupervisionTimeout::Ms6000, true, {}};
        default:
            return {ConnectionSupervisionTimeout::Ms6000, false, {}};
    }
}

/// @brief Parse PioOutput from character ('0'-'1')
/// @param c Character to parse
/// @return ParseResult with PioOutput value
inline ParseResult<PioOutput> parse_pio_output(char c) noexcept {
    switch (c) {
        case '0':
            return {PioOutput::Low, true, {}};
        case '1':
            return {PioOutput::High, true, {}};
        default:
            return {PioOutput::Low, false, {}};
    }
}

/// @brief Parse MAC address from 12-character hex string
/// @param hex_str Hex string (exactly 12 characters)
/// @return ParseResult with MacAddress value
inline ParseResult<MacAddress> parse_mac_address(std::string_view hex_str) noexcept {
    if (hex_str.size() != 12) {
        return {MacAddress{}, false, hex_str};
    }

    // Validate all characters are hex
    for (char c : hex_str) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return {MacAddress{}, false, hex_str};
        }
    }

    try {
        MacAddress mac(hex_str);
        return {mac, true, {}};
    } catch (...) {
        return {MacAddress{}, false, hex_str};
    }
}

/// @brief Parse UUID from 4-character hex string
/// @param hex_str Hex string (exactly 4 characters)
/// @return ParseResult with UUID value
inline ParseResult<UUID> parse_uuid(std::string_view hex_str) noexcept {
    if (hex_str.size() != 4) {
        return {UUID{}, false, hex_str};
    }

    // Validate all characters are hex
    for (char c : hex_str) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return {UUID{}, false, hex_str};
        }
    }

    try {
        UUID uuid(hex_str);
        return {uuid, true, {}};
    } catch (...) {
        return {UUID{}, false, hex_str};
    }
}

// ============================================================================
// Discovery response parsing utilities
// ============================================================================

/// @brief Discovery command response prefixes
/// These constants match the Ada implementation exactly
namespace discovery_prefixes {
constexpr std::string_view OK_DISCS = "OK+DISCS"; ///< Discovery start
constexpr std::string_view OK_DISC = "OK+DISC:";  ///< Device MAC address
constexpr std::string_view OK_NAME = "OK+NAME:";  ///< Device name
constexpr std::string_view OK_RSSI = "OK+RSSI:";  ///< Device RSSI
constexpr std::string_view OK_DISCE = "OK+DISCE"; ///< Discovery end
constexpr std::string_view CRLF = "\r\n";         ///< Line terminator
constexpr std::size_t MIN_PREFIX = 8;             ///< Minimum prefix length for parsing
} // namespace discovery_prefixes

/// @brief Check if buffer starts with specific prefix at given position
/// Equivalent to Ada's Start_With function for circular buffer parsing
/// @param buffer Buffer to search in
/// @param buffer_size Total buffer size
/// @param prefix Prefix to look for
/// @param start_pos Starting position in circular buffer
/// @param end_pos End position in circular buffer (exclusive)
/// @return true if prefix matches at start_pos
inline bool starts_with_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::string_view prefix,
    std::size_t start_pos,
    std::size_t end_pos) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    // Calculate available length handling circular wrapping
    std::size_t available_length = 0;
    if (end_pos >= start_pos) {
        available_length = end_pos - start_pos;
    } else {
        available_length = (buffer_size - start_pos) + end_pos;
    }

    if (available_length < prefix.size()) {
        return false;
    }

    // Compare characters with circular buffer wrapping
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        if (static_cast<char>(buffer[pos]) != prefix[i]) {
            return false;
        }
    }

    return true;
}

/// @brief Find CRLF terminator in circular buffer
/// Equivalent to Ada's Find function searching for ASCII.CR & ASCII.LF
/// @param buffer Buffer to search in
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param end_pos End position in circular buffer (exclusive)
/// @return Position of first CR character, or SIZE_MAX if not found
inline std::size_t find_crlf_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::size_t end_pos) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return SIZE_MAX;
    }

    // Calculate available length handling circular wrapping
    std::size_t available_length = 0;
    if (end_pos >= start_pos) {
        available_length = end_pos - start_pos;
    } else {
        available_length = (buffer_size - start_pos) + end_pos;
    }

    // Need at least 2 characters for CRLF
    if (available_length < 2) {
        return SIZE_MAX;
    }

    // Search for CR+LF sequence
    for (std::size_t i = 0; i < available_length - 1; ++i) {
        const std::size_t pos1 = (start_pos + i) % buffer_size;
        const std::size_t pos2 = (start_pos + i + 1) % buffer_size;

        if (buffer[pos1] == '\r' && buffer[pos2] == '\n') {
            return (start_pos + i) % buffer_size;
        }
    }

    return SIZE_MAX;
}

/// @brief Extract MAC address from circular buffer
/// Equivalent to Ada's MAC address extraction with import/address overlay
/// @param buffer Buffer to read from
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param mac Output MAC address (12 hex characters)
/// @return true if extraction successful
inline bool extract_mac_address_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    MacAddress& mac) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    constexpr std::size_t MAC_LENGTH = 12;

    // Extract 12 hex characters with circular wrapping
    std::array<char, MAC_LENGTH> mac_chars{};
    for (std::size_t i = 0; i < MAC_LENGTH; ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        const char c = static_cast<char>(buffer[pos]);

        // Validate hex character
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return false;
        }

        mac_chars[i] = c;
    }

    // Create MAC address from extracted characters
    try {
        mac = MacAddress(std::string_view(mac_chars.data(), MAC_LENGTH));
        return true;
    } catch (...) {
        return false;
    }
}

/// @brief Extract variable-length string data from circular buffer
/// Equivalent to Ada's Append function for Variable_String accumulation
/// @param buffer Buffer to read from
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param length Number of characters to extract
/// @param dest Destination VariableString for accumulation
/// @return true if extraction successful
template <std::size_t Capacity>
inline bool extract_string_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::size_t length,
    VariableString<Capacity>& dest) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    // Check if we have enough capacity
    if (dest.remaining_capacity() < length) {
        return false;
    }

    // Extract characters with circular wrapping
    for (std::size_t i = 0; i < length; ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        const char c = static_cast<char>(buffer[pos]);

        try {
            dest.append(c);
        } catch (...) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// iBeacon discovery parsing utilities
// ============================================================================

/// @brief iBeacon discovery response prefixes
/// These constants match the Ada implementation exactly for iBeacon scanning
namespace ibeacon_discovery_prefixes {
constexpr std::string_view OK_DISCS = "OK+DISCS"; ///< iBeacon discovery start
constexpr std::string_view OK_DISC = "OK+DISC";   ///< iBeacon device data
constexpr std::string_view OK_DISCE = "OK+DISCE"; ///< iBeacon discovery end
constexpr std::size_t IBEACON_DATA_LENGTH = 66;   ///< Fixed iBeacon data length (8+32+10+12+4)
} // namespace ibeacon_discovery_prefixes

/// @brief Parse iBeacon device string into components
/// Parses Device string with format: P0:P1:P2:P3:P4 where:
/// - Factory ID: P0 length is 8 bytes
/// - iBeacon UUID: P1 length is 32 bytes
/// - P2 length is 10 bytes (Major 4 + Minor 4 + Measured Power 2)
/// - MAC: P3 length is 12 bytes
/// - RSSI: P4 length is 4 bytes
/// All values are in ASCII mode.
/// @param device_str Full iBeacon device string
/// @param factory_id Output factory ID (8 characters)
/// @param uuid Output iBeacon UUID (32 characters)
/// @param major Output major value (4 characters)
/// @param minor Output minor value (4 characters)
/// @param measured_power Output measured power (2 characters)
/// @param mac Output MAC address (12 characters)
/// @param rssi Output RSSI string (4 characters)
/// @return true if parsing successful
inline bool parse_ibeacon_device(
    std::string_view device_str,
    std::string_view& factory_id,
    std::string_view& uuid,
    std::string_view& major,
    std::string_view& minor,
    std::string_view& measured_power,
    std::string_view& mac,
    std::string_view& rssi) noexcept {
    // Expected total length: 8 + 32 + 10 + 12 + 4 = 66 characters
    if (device_str.size() != ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH) {
        return false;
    }

    std::size_t pos = 0;

    // Extract Factory ID (8 bytes)
    factory_id = device_str.substr(pos, 8);
    pos += 8;

    // Extract iBeacon UUID (32 bytes)
    uuid = device_str.substr(pos, 32);
    pos += 32;

    // Extract P2 data (10 bytes total)
    // Major Value (4 bytes)
    major = device_str.substr(pos, 4);
    pos += 4;

    // Minor Value (4 bytes)
    minor = device_str.substr(pos, 4);
    pos += 4;

    // Measured Power (2 bytes)
    measured_power = device_str.substr(pos, 2);
    pos += 2;

    // Extract MAC (12 bytes)
    mac = device_str.substr(pos, 12);
    pos += 12;

    // Extract RSSI (4 bytes)
    rssi = device_str.substr(pos, 4);

    return true;
}

/// @brief Extract iBeacon device data from circular buffer
/// Equivalent to Ada's iBeacon data extraction with fixed length
/// @param buffer Buffer to read from
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param device_data Output array for 66 characters of iBeacon data
/// @return true if extraction successful
inline bool extract_ibeacon_data_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH>& device_data) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    // Extract 66 characters with circular wrapping
    for (std::size_t i = 0; i < ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH; ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        device_data[i] = static_cast<char>(buffer[pos]);
    }

    return true;
}

// ============================================================================
// GATT service discovery parsing utilities
// ============================================================================

/// @brief GATT service discovery response prefixes and constants
/// These constants match the Ada implementation exactly for service discovery
namespace gatt_service_discovery {
constexpr std::string_view SERVICE_HEADER =
    "********************************************************"; ///< Service discovery header/end marker (56 stars)
constexpr std::size_t SERVICE_HEADER_LENGTH = 56;               ///< Length of service header/end marker
constexpr std::size_t SERVICE_UUID_LENGTH = 14;                 ///< Service UUID data length (4:4:4 format)
} // namespace gatt_service_discovery

/// @brief Parse service UUID string into components
/// Parses Service string with format: [P1]:[P2]:[P3] where:
/// - P1: 4 bytes, Services start handle
/// - P2: 4 bytes, Services end handle
/// - P3: 4 bytes, Services UUID
/// @param service_str Full service UUID string (14 characters)
/// @param start_handle Output start handle (4 hex characters)
/// @param end_handle Output end handle (4 hex characters)
/// @param service_uuid Output service UUID (4 hex characters)
/// @return true if parsing successful
inline bool parse_service_uuid_components(
    std::string_view service_str,
    std::string_view& start_handle,
    std::string_view& end_handle,
    std::string_view& service_uuid) noexcept {
    // Expected format: 4:4:4 = 14 total characters
    if (service_str.size() != gatt_service_discovery::SERVICE_UUID_LENGTH) {
        return false;
    }

    // Validate format has colons in the right places
    if (service_str[4] != ':' || service_str[9] != ':') {
        return false;
    }

    // Extract components
    start_handle = service_str.substr(0, 4);
    end_handle = service_str.substr(5, 4);
    service_uuid = service_str.substr(10, 4);

    // Validate all components are hex characters
    for (char c : start_handle) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return false;
        }
    }
    for (char c : end_handle) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return false;
        }
    }
    return std::ranges::all_of(service_uuid, [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    });
}

/// @brief Extract service UUID data from circular buffer
/// Equivalent to Ada's Copy procedure for UUID_Data extraction
/// @param buffer Buffer to read from
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param service_data Output array for 14 characters of service UUID
/// @return true if extraction successful
inline bool extract_service_uuid_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH>& service_data) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    // Extract 14 characters with circular wrapping
    for (std::size_t i = 0; i < gatt_service_discovery::SERVICE_UUID_LENGTH; ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        service_data[i] = static_cast<char>(buffer[pos]);
    }

    return true;
}

/// @brief Check if buffer contains service discovery header/end at position
/// Equivalent to Ada's Start_With function for Header pattern matching
/// @param buffer Buffer to search in
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param end_pos End position in circular buffer (exclusive)
/// @return true if header pattern matches at start_pos
inline bool has_service_header_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::size_t end_pos) noexcept {
    return starts_with_circular(buffer, buffer_size, gatt_service_discovery::SERVICE_HEADER, start_pos, end_pos);
}

// ============================================================================
// GATT characteristic discovery parsing utilities
// ============================================================================

/// @brief Characteristic properties bitfield enumeration
/// Implements Design Decision #3 (Characteristic Data Representation)
/// Provides type-safe property combinations for "RD|WR|WN|NO|IN" format parsing
///
/// Maps to Ada's characteristic properties specification:
/// "RD|WR|WN|NO|IN"; WR - Write; WN - Write_Without_Response;
/// IN - Indicate; NO - Notify; RD - Read;
/// Usually, a characteristic maybe only has one or two properties.
/// Unsupported property part replaced by "--"
enum class CharacteristicProperty : uint8_t {
    None = 0x00,
    Read = 0x01,                 ///< RD - Read property
    Write = 0x02,                ///< WR - Write property
    WriteWithoutResponse = 0x04, ///< WN - Write_Without_Response property
    Notify = 0x08,               ///< NO - Notify property
    Indicate = 0x10,             ///< IN - Indicate property

    // Common combinations to avoid enum cast warnings
    ReadWrite = Read | Write,                                                                   ///< 0x03
    ReadWriteWithoutResponse = Read | WriteWithoutResponse,                                     ///< 0x05
    WriteWriteWithoutResponse = Write | WriteWithoutResponse,                                   ///< 0x06
    ReadWriteWriteWithoutResponse = Read | Write | WriteWithoutResponse,                        ///< 0x07
    ReadNotify = Read | Notify,                                                                 ///< 0x09
    WriteNotify = Write | Notify,                                                               ///< 0x0A
    WriteWithoutResponseNotify = WriteWithoutResponse | Notify,                                 ///< 0x0C
    WriteWriteWithoutResponseNotify = Write | WriteWithoutResponse | Notify,                    ///< 0x0E
    ReadWriteWriteWithoutResponseNotify = Read | Write | WriteWithoutResponse | Notify,         ///< 0x0F
    ReadIndicate = Read | Indicate,                                                             ///< 0x11
    WriteIndicate = Write | Indicate,                                                           ///< 0x12
    WriteWithoutResponseIndicate = WriteWithoutResponse | Indicate,                             ///< 0x14
    WriteWriteWithoutResponseIndicate = Write | WriteWithoutResponse | Indicate,                ///< 0x16
    ReadWriteWriteWithoutResponseIndicate = Read | Write | WriteWithoutResponse | Indicate,     ///< 0x17
    NotifyIndicate = Notify | Indicate,                                                         ///< 0x18
    WriteWriteWithoutResponseNotifyIndicate = Write | WriteWithoutResponse | Notify | Indicate, ///< 0x1E

    // Three-way combinations
    ReadWriteNotify = Read | Write | Notify,                                       ///< 0x0B
    ReadWriteIndicate = Read | Write | Indicate,                                   ///< 0x13
    ReadWriteWithoutResponseNotify = Read | WriteWithoutResponse | Notify,         ///< 0x0D
    ReadWriteWithoutResponseIndicate = Read | WriteWithoutResponse | Indicate,     ///< 0x15
    ReadNotifyIndicate = Read | Notify | Indicate,                                 ///< 0x19
    WriteNotifyIndicate = Write | Notify | Indicate,                               ///< 0x1A
    WriteWithoutResponseNotifyIndicate = WriteWithoutResponse | Notify | Indicate, ///< 0x1C

    // Four-way combinations
    ReadWriteNotifyIndicate = Read | Write | Notify | Indicate,                               ///< 0x1B
    ReadWriteWithoutResponseNotifyIndicate = Read | WriteWithoutResponse | Notify | Indicate, ///< 0x1D

    // Five-way combination (all properties)
    All = Read | Write | WriteWithoutResponse | Notify | Indicate ///< 0x1F
};

/// @brief Bitwise OR operator for characteristic properties
constexpr CharacteristicProperty operator|(CharacteristicProperty lhs, CharacteristicProperty rhs) noexcept {
    return static_cast<CharacteristicProperty>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/// @brief Bitwise AND operator for characteristic properties
constexpr CharacteristicProperty operator&(CharacteristicProperty lhs, CharacteristicProperty rhs) noexcept {
    return static_cast<CharacteristicProperty>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

/// @brief Bitwise OR assignment operator for characteristic properties
constexpr CharacteristicProperty& operator|=(CharacteristicProperty& lhs, CharacteristicProperty rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

/// @brief Check if characteristic has specific property
/// @param properties Properties bitfield to check
/// @param property Property to test for
/// @return true if property is set in the bitfield
constexpr bool has_property(CharacteristicProperty properties, CharacteristicProperty property) noexcept {
    return (properties & property) != CharacteristicProperty::None;
}

/// @brief GATT characteristic discovery response parsing constants
/// These constants match the Ada implementation exactly for characteristic discovery
namespace gatt_characteristic_discovery {
constexpr std::string_view CHARACTERISTIC_HEADER =
    "********************************************************"; ///< Characteristic discovery header/end marker (56
                                                                ///< stars)
constexpr std::size_t CHARACTERISTIC_HEADER_LENGTH = 56;        ///< Length of characteristic header/end marker
constexpr std::size_t CHARACTERISTIC_UUID_LENGTH = 24;          ///< Characteristic UUID data length (4:14:4 format)
constexpr std::size_t CHARACTERISTIC_PROPERTIES_LENGTH =
    14; ///< Characteristic properties string length ("RD|WR|WN|NO|IN")
} // namespace gatt_characteristic_discovery

/// @brief Parse characteristic properties string into bitfield
/// Parses properties string with format: "RD|WR|WN|NO|IN" where:
/// - RD: Read property (position 0-1)
/// - WR: Write property (position 3-4)
/// - WN: Write_Without_Response property (position 6-7)
/// - NO: Notify property (position 9-10)
/// - IN: Indicate property (position 12-13)
/// - Unsupported properties replaced with "--"
/// @param properties_str Properties string (exactly 14 characters)
/// @return ParseResult with CharacteristicProperty bitfield
inline ParseResult<CharacteristicProperty> parse_characteristic_properties(std::string_view properties_str) noexcept {
    if (properties_str.size() != gatt_characteristic_discovery::CHARACTERISTIC_PROPERTIES_LENGTH) {
        return {CharacteristicProperty::None, false, properties_str};
    }

    // Validate separators are in correct positions
    if (properties_str[2] != '|' || properties_str[5] != '|' || properties_str[8] != '|' || properties_str[11] != '|') {
        return {CharacteristicProperty::None, false, properties_str};
    }

    CharacteristicProperty result = CharacteristicProperty::None;

    // Parse RD (Read) at positions 0-1
    if (properties_str.starts_with("RD")) {
        result |= CharacteristicProperty::Read;
    }

    // Parse WR (Write) at positions 3-4
    if (properties_str.substr(3, 2) == "WR") {
        result |= CharacteristicProperty::Write;
    }

    // Parse WN (Write_Without_Response) at positions 6-7
    if (properties_str.substr(6, 2) == "WN") {
        result |= CharacteristicProperty::WriteWithoutResponse;
    }

    // Parse NO (Notify) at positions 9-10
    if (properties_str.substr(9, 2) == "NO") {
        result |= CharacteristicProperty::Notify;
    }

    // Parse IN (Indicate) at positions 12-13
    if (properties_str.substr(12, 2) == "IN") {
        result |= CharacteristicProperty::Indicate;
    }

    return {result, true, {}};
}

/// @brief Parse characteristic UUID string into components
/// Parses Characteristic string with format: [P1]:[P2]:[P3] where:
/// - P1: 4 bytes, Characteristic handle
/// - P2: 14 bytes, "RD|WR|WN|NO|IN" property string
/// - P3: 4 bytes, Characteristic UUID
/// @param characteristic_str Full characteristic UUID string (24 characters)
/// @param handle Output handle (4 hex characters)
/// @param properties Output properties bitfield
/// @param characteristic_uuid Output characteristic UUID (4 hex characters)
/// @return true if parsing successful
inline bool parse_characteristic_uuid_components(
    std::string_view characteristic_str,
    std::string_view& handle,
    CharacteristicProperty& properties,
    std::string_view& characteristic_uuid) noexcept {
    // Expected format: 4:14:4 = 24 total characters (including 2 colons)
    if (characteristic_str.size() != gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH) {
        return false;
    }

    // Validate format has colons in the right places
    if (characteristic_str[4] != ':' || characteristic_str[19] != ':') {
        return false;
    }

    // Extract components
    handle = characteristic_str.substr(0, 4);
    std::string_view properties_str = characteristic_str.substr(5, 14);
    characteristic_uuid = characteristic_str.substr(20, 4);

    // Validate handle is hex characters
    for (char c : handle) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return false;
        }
    }

    // Validate characteristic UUID is hex characters
    for (char c : characteristic_uuid) {
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f')) {
            return false;
        }
    }

    // Parse properties
    auto properties_result = parse_characteristic_properties(properties_str);
    if (!properties_result.has_value()) {
        return false;
    }

    properties = *properties_result;
    return true;
}

/// @brief Extract characteristic UUID data from circular buffer
/// Equivalent to Ada's Copy procedure for UUID_Data extraction
/// @param buffer Buffer to read from
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param characteristic_data Output array for 24 characters of characteristic UUID
/// @return true if extraction successful
inline bool extract_characteristic_uuid_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::array<char, gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH>& characteristic_data) noexcept {
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    // Extract 24 characters with circular wrapping
    for (std::size_t i = 0; i < gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH; ++i) {
        const std::size_t pos = (start_pos + i) % buffer_size;
        characteristic_data[i] = static_cast<char>(buffer[pos]);
    }

    return true;
}

/// @brief Check if buffer contains characteristic discovery header/end at position
/// Equivalent to Ada's Start_With function for Header pattern matching
/// @param buffer Buffer to search in
/// @param buffer_size Total buffer size
/// @param start_pos Starting position in circular buffer
/// @param end_pos End position in circular buffer (exclusive)
/// @return true if header pattern matches at start_pos
inline bool has_characteristic_header_circular(
    const uint8_t* buffer,
    std::size_t buffer_size,
    std::size_t start_pos,
    std::size_t end_pos) noexcept {
    return starts_with_circular(
        buffer,
        buffer_size,
        gatt_characteristic_discovery::CHARACTERISTIC_HEADER,
        start_pos,
        end_pos);
}

} // namespace hm11::util