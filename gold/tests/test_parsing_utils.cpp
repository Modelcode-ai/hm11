#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hm11/util/parsing_utils.hpp"

using namespace hm11::util;
using testing::Optional;
using testing::SizeIs;

// ============================================================================
// String Search Tests (starts_with, ends_with, find)
// ============================================================================

TEST(ParsingUtils, StartsWithStringView) {
    EXPECT_TRUE(starts_with("OK+Set:1", "OK+"));
    EXPECT_TRUE(starts_with("OK+Set:1", "OK+Set:1"));
    EXPECT_TRUE(starts_with("OK+Set:1", ""));
    EXPECT_FALSE(starts_with("OK+Set:1", "OK+Set:12"));
    EXPECT_FALSE(starts_with("OK+Set:1", "Set"));
    EXPECT_FALSE(starts_with("", "OK"));
}

TEST(ParsingUtils, StartsWithByteSpan) {
    const std::array<uint8_t, 8> K_DATA = {'O', 'K', '+', 'S', 'e', 't', ':', '1'};
    const std::array<uint8_t, 3> K_PREFIX = {'O', 'K', '+'};
    const std::array<uint8_t, 3> K_WRONG = {'S', 'e', 't'};

    EXPECT_TRUE(starts_with(std::span(K_DATA), std::span(K_PREFIX)));
    EXPECT_FALSE(starts_with(std::span(K_DATA), std::span(K_WRONG)));
}

TEST(ParsingUtils, EndsWithStringView) {
    EXPECT_TRUE(ends_with("OK+Set:1", ":1"));
    EXPECT_TRUE(ends_with("OK+Set:1", "OK+Set:1"));
    EXPECT_TRUE(ends_with("OK+Set:1", ""));
    EXPECT_FALSE(ends_with("OK+Set:1", "Set:12"));
    EXPECT_FALSE(ends_with("OK+Set:1", "OK"));
    EXPECT_FALSE(ends_with("", ":1"));
}

TEST(ParsingUtils, EndsWithByteSpan) {
    const std::array<uint8_t, 8> K_DATA = {'O', 'K', '+', 'S', 'e', 't', ':', '1'};
    const std::array<uint8_t, 2> K_SUFFIX = {':', '1'};
    const std::array<uint8_t, 2> K_WRONG = {'O', 'K'};

    EXPECT_TRUE(ends_with(std::span(K_DATA), std::span(K_SUFFIX)));
    EXPECT_FALSE(ends_with(std::span(K_DATA), std::span(K_WRONG)));
}

TEST(ParsingUtils, FindStringView) {
    EXPECT_EQ(find("OK+DISC:001122334455", "DISC"), 3);
    EXPECT_EQ(find("OK+DISC:001122334455", ":"), 7);
    EXPECT_EQ(find("OK+DISC:001122334455", "999"), std::string_view::npos);
    EXPECT_EQ(find("OK+DISC:001122334455", ""), 0);
}

TEST(ParsingUtils, FindByteSpan) {
    const std::array<uint8_t, 15> K_DATA = {'O', 'K', '+', 'D', 'I', 'S', 'C', ':', '0', '0', '1', '1', '2', '2', '3'};
    const std::array<uint8_t, 4> K_PATTERN = {'D', 'I', 'S', 'C'};
    const std::array<uint8_t, 3> K_MISSING = {'9', '9', '9'};

    EXPECT_EQ(find(std::span(K_DATA), std::span(K_PATTERN)), 3);
    EXPECT_EQ(find(std::span(K_DATA), std::span(K_MISSING)), K_DATA.size());
}

// ============================================================================
// CRLF Detection and Trimming Tests
// ============================================================================

TEST(ParsingUtils, FindCRLFStringView) {
    EXPECT_EQ(find_crlf("OK+NAME:Device\r\n"), 14);
    EXPECT_EQ(find_crlf("OK+RSSI:-45\r\n"), 11);
    EXPECT_EQ(find_crlf("NoLineEnding"), std::string_view::npos);
    EXPECT_EQ(find_crlf("\r\n"), 0);
    EXPECT_EQ(find_crlf(""), std::string_view::npos);
}

TEST(ParsingUtils, FindCRLFByteSpan) {
    const std::array<uint8_t, 16> K_DATA1 =
        {'O', 'K', '+', 'N', 'A', 'M', 'E', ':', 'D', 'e', 'v', 'i', 'c', 'e', '\r', '\n'};
    const std::array<uint8_t, 12> K_DATA2 = {'N', 'o', 'L', 'i', 'n', 'e', 'E', 'n', 'd', 'i', 'n', 'g'};

    EXPECT_EQ(find_crlf(std::span(K_DATA1)), 14);
    EXPECT_EQ(find_crlf(std::span(K_DATA2)), K_DATA2.size());
}

TEST(ParsingUtils, TrimCRLFStringView) {
    EXPECT_EQ(trim_crlf("OK+Set:1\r\n"), "OK+Set:1");
    EXPECT_EQ(trim_crlf("OK+Set:1"), "OK+Set:1");
    EXPECT_EQ(trim_crlf("\r\n"), "");
    EXPECT_EQ(trim_crlf(""), "");
}

TEST(ParsingUtils, TrimCRLFByteSpan) {
    const std::array<uint8_t, 10> K_DATA1 = {'O', 'K', '+', 'S', 'e', 't', ':', '1', '\r', '\n'};
    const std::array<uint8_t, 8> K_DATA2 = {'O', 'K', '+', 'S', 'e', 't', ':', '1'};

    auto result1 = trim_crlf(std::span(K_DATA1));
    EXPECT_EQ(result1.size(), 8);

    auto result2 = trim_crlf(std::span(K_DATA2));
    EXPECT_EQ(result2.size(), 8);
}

TEST(ParsingUtils, SplitAtCRLFStringView) {
    auto [before, after] = split_at_crlf("OK+NAME:Device\r\nOK+RSSI:-45");
    EXPECT_EQ(before, "OK+NAME:Device");
    EXPECT_EQ(after, "OK+RSSI:-45");

    auto [single, empty] = split_at_crlf("NoLineEnding");
    EXPECT_EQ(single, "NoLineEnding");
    EXPECT_TRUE(empty.empty());

    auto [first, rest] = split_at_crlf("\r\nRest");
    EXPECT_TRUE(first.empty());
    EXPECT_EQ(rest, "Rest");
}

TEST(ParsingUtils, SplitAtCRLFByteSpan) {
    const std::array<uint8_t, 27> K_DATA = {'O',  'K',  '+', 'N', 'A', 'M', 'E', ':', 'D', 'e', 'v', 'i', 'c', 'e',
                                            '\r', '\n', 'O', 'K', '+', 'R', 'S', 'S', 'I', ':', '-', '4', '5'};

    auto [before, after] = split_at_crlf(std::span(K_DATA));
    EXPECT_EQ(before.size(), 14);
    EXPECT_EQ(after.size(), 11);
}

// ============================================================================
// Hexadecimal Character Tests
// ============================================================================

TEST(ParsingUtils, IsHexDigit) {
    // Valid hex digits
    EXPECT_TRUE(is_hex_digit('0'));
    EXPECT_TRUE(is_hex_digit('9'));
    EXPECT_TRUE(is_hex_digit('A'));
    EXPECT_TRUE(is_hex_digit('F'));
    EXPECT_TRUE(is_hex_digit('a'));
    EXPECT_TRUE(is_hex_digit('f'));

    // Invalid characters
    EXPECT_FALSE(is_hex_digit('G'));
    EXPECT_FALSE(is_hex_digit('g'));
    EXPECT_FALSE(is_hex_digit(' '));
    EXPECT_FALSE(is_hex_digit(':'));
    EXPECT_FALSE(is_hex_digit('\n'));
}

TEST(ParsingUtils, IsHexDigitByte) {
    EXPECT_TRUE(is_hex_digit(static_cast<uint8_t>('0')));
    EXPECT_TRUE(is_hex_digit(static_cast<uint8_t>('A')));
    EXPECT_FALSE(is_hex_digit(static_cast<uint8_t>('G')));
}

TEST(ParsingUtils, HexCharToInt) {
    EXPECT_EQ(hex_char_to_int('0'), 0);
    EXPECT_EQ(hex_char_to_int('9'), 9);
    EXPECT_EQ(hex_char_to_int('A'), 10);
    EXPECT_EQ(hex_char_to_int('F'), 15);
    EXPECT_EQ(hex_char_to_int('a'), 10);
    EXPECT_EQ(hex_char_to_int('f'), 15);

    EXPECT_FALSE(hex_char_to_int('G').has_value());
    EXPECT_FALSE(hex_char_to_int('g').has_value());
    EXPECT_FALSE(hex_char_to_int(' ').has_value());
    EXPECT_FALSE(hex_char_to_int(':').has_value());
}

TEST(ParsingUtils, HexCharToIntByte) {
    EXPECT_EQ(hex_char_to_int(static_cast<uint8_t>('A')), 10);
    EXPECT_FALSE(hex_char_to_int(static_cast<uint8_t>('G')).has_value());
}

TEST(ParsingUtils, IsAllHex) {
    EXPECT_TRUE(is_all_hex("0123456789ABCDEF"));
    EXPECT_TRUE(is_all_hex("abcdef"));
    EXPECT_TRUE(is_all_hex("A1B2C3"));
    EXPECT_TRUE(is_all_hex(""));

    EXPECT_FALSE(is_all_hex("0123456789ABCDEFG"));
    EXPECT_FALSE(is_all_hex("12:34:56"));
    EXPECT_FALSE(is_all_hex("Hello"));
    EXPECT_FALSE(is_all_hex("12 34"));
}

TEST(ParsingUtils, IsAllHexByteSpan) {
    const std::array<uint8_t, 6> K_VALID = {'A', '1', 'B', '2', 'C', '3'};
    const std::array<uint8_t, 5> K_INVALID = {'1', '2', 'G', '4', '5'};

    EXPECT_TRUE(is_all_hex(std::span(K_VALID)));
    EXPECT_FALSE(is_all_hex(std::span(K_INVALID)));
}

TEST(ParsingUtils, IsAllDigits) {
    EXPECT_TRUE(is_all_digits("0123456789"));
    EXPECT_TRUE(is_all_digits("42"));
    EXPECT_TRUE(is_all_digits(""));

    EXPECT_FALSE(is_all_digits("12A34"));
    EXPECT_FALSE(is_all_digits("Hello"));
    EXPECT_FALSE(is_all_digits("12.34"));
}

// ============================================================================
// Fixed-Field Parsing Tests
// ============================================================================

TEST(ParsingUtils, ParseHexField) {
    // Valid hex field
    auto result = parse_hex_field<4>("ABCD123456");
    EXPECT_THAT(result, Optional(std::string_view("ABCD")));

    // Exact length
    auto exact = parse_hex_field<4>("ABCD");
    EXPECT_THAT(exact, Optional(std::string_view("ABCD")));

    // Too short
    auto too_short = parse_hex_field<4>("ABC");
    EXPECT_FALSE(too_short.has_value());

    // Invalid hex characters
    auto invalid = parse_hex_field<4>("ABCG");
    EXPECT_FALSE(invalid.has_value());
}

TEST(ParsingUtils, ParseHexFieldByteSpan) {
    const std::array<uint8_t, 10> K_DATA = {'A', 'B', 'C', 'D', '1', '2', '3', '4', '5', '6'};

    auto result = parse_hex_field<4>(std::span(K_DATA));
    EXPECT_THAT(result, Optional(SizeIs(4)));
}

TEST(ParsingUtils, ParseMACAddress) {
    // Valid MAC address
    auto mac1 = parse_mac_address("001122334455AABBCC");
    EXPECT_THAT(mac1, Optional(std::string_view("001122334455")));

    // Exact length
    auto mac2 = parse_mac_address("AABBCCDDEEFF");
    EXPECT_THAT(mac2, Optional(std::string_view("AABBCCDDEEFF")));

    // Too short
    auto too_short = parse_mac_address("00112233");
    EXPECT_FALSE(too_short.has_value());

    // Invalid characters
    auto invalid = parse_mac_address("00:11:22:33:44:55");
    EXPECT_FALSE(invalid.has_value());
}

TEST(ParsingUtils, ParseMACAddressByteSpan) {
    const std::array<uint8_t, 12> K_MAC = {'0', '0', '1', '1', '2', '2', '3', '3', '4', '4', '5', '5'};

    auto result = parse_mac_address(std::span(K_MAC));
    EXPECT_THAT(result, Optional(SizeIs(12)));
}

TEST(ParsingUtils, ParseUUIDComponent) {
    // Valid UUID component (4 hex chars)
    auto uuid1 = parse_uuid_component("ABCD1234");
    EXPECT_THAT(uuid1, Optional(std::string_view("ABCD")));

    // Exact length
    auto uuid2 = parse_uuid_component("1234");
    EXPECT_THAT(uuid2, Optional(std::string_view("1234")));

    // Too short
    auto too_short = parse_uuid_component("ABC");
    EXPECT_FALSE(too_short.has_value());

    // Invalid characters
    auto invalid = parse_uuid_component("ABCG");
    EXPECT_FALSE(invalid.has_value());
}

// ============================================================================
// Numeric Parsing Tests
// ============================================================================

TEST(ParsingUtils, ParseInt) {
    // Valid integers
    EXPECT_EQ(parse_int<int>("42"), 42);
    EXPECT_EQ(parse_int<int>("0"), 0);
    EXPECT_EQ(parse_int<int>("12345"), 12345);
    EXPECT_EQ(parse_int<int>("-45"), -45);

    // unsigned
    EXPECT_EQ(parse_int<unsigned>("100"), 100U);

    // Invalid inputs
    EXPECT_FALSE(parse_int<int>("").has_value());
    EXPECT_FALSE(parse_int<int>("abc").has_value());
    EXPECT_FALSE(parse_int<int>("12.5").has_value());
    EXPECT_FALSE(parse_int<int>("12 ").has_value());
    EXPECT_FALSE(parse_int<int>(" 12").has_value());

    // Partial parse not accepted (must consume entire string)
    EXPECT_FALSE(parse_int<int>("12abc").has_value());
}

TEST(ParsingUtils, ParseTemperature) {
    // Valid temperatures
    EXPECT_EQ(parse_temperature("25"), 25);
    EXPECT_EQ(parse_temperature("0"), 0);
    EXPECT_EQ(parse_temperature("120"), 120);

    // Out of range
    EXPECT_FALSE(parse_temperature("-1").has_value());
    EXPECT_FALSE(parse_temperature("121").has_value());
    EXPECT_FALSE(parse_temperature("255").has_value());

    // Invalid format
    EXPECT_FALSE(parse_temperature("").has_value());
    EXPECT_FALSE(parse_temperature("25.5").has_value());
    EXPECT_FALSE(parse_temperature("abc").has_value());
}

TEST(ParsingUtils, ParseHumidity) {
    // Valid humidity
    EXPECT_EQ(parse_humidity("60"), 60);
    EXPECT_EQ(parse_humidity("0"), 0);
    EXPECT_EQ(parse_humidity("100"), 100);

    // Out of range
    EXPECT_FALSE(parse_humidity("-1").has_value());
    EXPECT_FALSE(parse_humidity("101").has_value());
    EXPECT_FALSE(parse_humidity("255").has_value());

    // Invalid format
    EXPECT_FALSE(parse_humidity("").has_value());
    EXPECT_FALSE(parse_humidity("60.5").has_value());
    EXPECT_FALSE(parse_humidity("abc").has_value());
}

// ============================================================================
// Conversion Tests
// ============================================================================

TEST(ParsingUtils, ToStringView) {
    const std::array<uint8_t, 5> K_DATA = {'H', 'e', 'l', 'l', 'o'};
    auto str = to_string_view(std::span(K_DATA));
    EXPECT_EQ(str, "Hello");
    EXPECT_EQ(str.size(), 5);
}

TEST(ParsingUtils, ToByteSpan) {
    std::string_view str = "Hello";
    auto bytes = to_byte_span(str);
    EXPECT_EQ(bytes.size(), 5);
    EXPECT_EQ(bytes[0], static_cast<uint8_t>('H'));
    EXPECT_EQ(bytes[4], static_cast<uint8_t>('o'));
}

// ============================================================================
// Real-World Response Parsing Tests
// ============================================================================

TEST(ParsingUtils, ParseSimpleAcknowledgment) {
    std::string_view response = "OK+Set:1\r\n";

    // Check it starts with expected prefix
    EXPECT_TRUE(starts_with(response, "OK+Set:"));

    // Trim CRLF
    auto trimmed = trim_crlf(response);
    EXPECT_EQ(trimmed, "OK+Set:1");

    // Extract the value part after colon
    auto colon_pos = find(trimmed, ":");
    ASSERT_NE(colon_pos, std::string_view::npos);
    auto value_part = trimmed.substr(colon_pos + 1);
    EXPECT_EQ(value_part, "1");
}

TEST(ParsingUtils, ParseDiscoveryResponse) {
    std::string_view response = "OK+DISC:001122334455\r\nOK+NAME:HM-11\r\nOK+RSSI:-45\r\n";

    // Parse MAC address line
    auto [mac_line, rest1] = split_at_crlf(response);
    EXPECT_TRUE(starts_with(mac_line, "OK+DISC:"));

    auto mac_start = mac_line.substr(8); // Skip "OK+DISC:"
    auto mac = parse_mac_address(mac_start);
    EXPECT_THAT(mac, Optional(std::string_view("001122334455")));

    // Parse name line
    auto [name_line, rest2] = split_at_crlf(rest1);
    EXPECT_TRUE(starts_with(name_line, "OK+NAME:"));
    auto name = name_line.substr(8); // Skip "OK+NAME:"
    EXPECT_EQ(name, "HM-11");

    // Parse RSSI line
    auto [rssi_line, rest3] = split_at_crlf(rest2);
    EXPECT_TRUE(starts_with(rssi_line, "OK+RSSI:"));
    auto rssi_str = rssi_line.substr(8); // Skip "OK+RSSI:"
    auto rssi = parse_int<int>(rssi_str);
    EXPECT_THAT(rssi, Optional(-45));

    EXPECT_TRUE(rest3.empty());
}

TEST(ParsingUtils, ParseSensorResponse) {
    std::string_view response = "OK+Get:25,60\r\n";

    // Trim CRLF
    auto trimmed = trim_crlf(response);

    // Extract value part after colon
    auto colon_pos = find(trimmed, ":");
    ASSERT_NE(colon_pos, std::string_view::npos);
    auto values = trimmed.substr(colon_pos + 1);
    EXPECT_EQ(values, "25,60");

    // Split on comma
    auto comma_pos = find(values, ",");
    ASSERT_NE(comma_pos, std::string_view::npos);

    auto temp_str = values.substr(0, comma_pos);
    auto humidity_str = values.substr(comma_pos + 1);

    auto temp = parse_temperature(temp_str);
    EXPECT_THAT(temp, Optional(25));

    auto humidity = parse_humidity(humidity_str);
    EXPECT_THAT(humidity, Optional(60));
}

TEST(ParsingUtils, ParseiBeaconUUID) {
    std::string_view response = "OK+Get:ABCD1234EFGH5678";

    // Extract UUID components (4 hex chars each)
    auto value_start = response.substr(7); // Skip "OK+Get:"

    auto part1 = parse_uuid_component(value_start);
    EXPECT_THAT(part1, Optional(std::string_view("ABCD")));

    auto part2 = parse_uuid_component(value_start.substr(4));
    EXPECT_THAT(part2, Optional(std::string_view("1234")));

    // This should fail (contains 'G' and 'H')
    auto part3 = parse_uuid_component(value_start.substr(8));
    EXPECT_FALSE(part3.has_value());
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST(ParsingUtils, EmptyInputs) {
    EXPECT_TRUE(starts_with("", ""));
    EXPECT_FALSE(starts_with("", "OK"));
    EXPECT_TRUE(ends_with("", ""));
    EXPECT_FALSE(ends_with("", "OK"));
    EXPECT_EQ(find("", "OK"), std::string_view::npos);
    EXPECT_EQ(find_crlf(""), std::string_view::npos);
    EXPECT_TRUE(trim_crlf("").empty());
    EXPECT_TRUE(is_all_hex(""));
    EXPECT_TRUE(is_all_digits(""));
}

TEST(ParsingUtils, SingleCharacterInputs) {
    EXPECT_TRUE(starts_with("A", "A"));
    EXPECT_TRUE(ends_with("A", "A"));
    EXPECT_TRUE(is_hex_digit('A'));
    EXPECT_TRUE(is_all_hex("A"));
    EXPECT_FALSE(is_all_digits("A"));
}

TEST(ParsingUtils, ZeroCopySemantics) {
    // Verify that parsing operations return views into the original data
    std::string original = "OK+DISC:001122334455";
    std::string_view view = original;

    auto mac = parse_mac_address(view.substr(8));

    // Explicit control-flow guard so clang-tidy sees that we don't use mac when it's empty
    if (!mac.has_value()) {
        FAIL() << "parse_mac_address(view.substr(8)) returned empty optional";
        return; // Important so the analyzer knows we stop here
    }

    // Now it's safe to access mac - verify zero-copy (points into original string)
    EXPECT_EQ(mac->data(), original.data() + 8);
}

TEST(ParsingUtils, ConstexprEvaluation) {
    // Verify that key functions can be evaluated at compile time
    constexpr bool K_TEST1 = starts_with("OK+Set:1", "OK+");
    static_assert(K_TEST1);

    constexpr bool K_TEST2 = is_hex_digit('A');
    static_assert(K_TEST2);

    constexpr bool K_TEST3 = is_all_hex("ABCDEF");
    static_assert(K_TEST3);

    constexpr auto K_TEST4 = hex_char_to_int('F');
    static_assert(K_TEST4.has_value() && *K_TEST4 == 15);

    constexpr auto K_TEST5 = trim_crlf("OK\r\n");
    static_assert(K_TEST5 == "OK");
}
