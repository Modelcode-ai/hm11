/**
 * @file fuzz_parsing_utils.cpp
 * @brief LibFuzzer target for parsing utilities
 *
 * This fuzzer tests the parsing utilities in hm11::util for:
 * - Crashes on malformed input
 * - Memory safety violations
 * - Assertion failures
 * - Edge cases in hex/digit validation
 * - Integer parsing overflow
 * - Temperature/humidity range validation
 *
 * Build with:
 *   clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
 *           -std=c++20 -I../include fuzz_parsing_utils.cpp -o fuzz_parsing_utils
 *
 * Run with:
 *   ./fuzz_parsing_utils -max_len=1000 -max_total_time=300 corpus/parsing
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hm11/util/parsing_utils.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Create span for byte-based operations
    std::span<const uint8_t> input(data, size);

    // ========================================================================
    // Fuzz 1: Hex digit validation
    // ========================================================================
    // Tests: is_hex_digit, is_all_hex
    // Goal: Find inputs that cause incorrect validation
    (void)hm11::util::is_all_hex(input);

    // Test individual hex character validation
    if (size > 0) {
        (void)hm11::util::is_hex_digit(data[0]);
        (void)hm11::util::hex_char_to_int(data[0]);
    }

    // ========================================================================
    // Fuzz 2: MAC address parsing
    // ========================================================================
    // Tests: parse_mac_address (expects exactly 12 hex chars)
    // Goal: Find crashes on malformed MAC addresses
    if (size >= 12) {
        (void)hm11::util::parse_mac_address(input);
    }

    // ========================================================================
    // Fuzz 3: Hex field parsing
    // ========================================================================
    // Tests: parse_hex_field<N> template
    // Goal: Verify bounds checking and validation
    if (size >= 4) {
        (void)hm11::util::parse_hex_field<4>(input);  // UUID
    }
    if (size >= 32) {
        (void)hm11::util::parse_hex_field<32>(input);  // iBeacon UUID
    }

    // ========================================================================
    // Fuzz 4: String view conversion and operations
    // ========================================================================
    // Tests: to_string_view, starts_with, ends_with, find
    // Goal: Find crashes in string operations
    auto str_view = hm11::util::to_string_view(input);

    // Test string operations
    (void)hm11::util::starts_with(str_view, "OK+");
    (void)hm11::util::ends_with(str_view, "\r\n");
    (void)hm11::util::find(str_view, ":");
    (void)hm11::util::find_crlf(str_view);
    (void)hm11::util::trim_crlf(str_view);

    // Byte-based operations
    constexpr std::array<uint8_t, 3> prefix = {'O', 'K', '+'};
    (void)hm11::util::starts_with(input, std::span<const uint8_t>(prefix));
    (void)hm11::util::ends_with(input, std::span<const uint8_t>(hm11::util::K_CRLF_BYTES));
    (void)hm11::util::find_crlf(input);
    (void)hm11::util::trim_crlf(input);

    // ========================================================================
    // Fuzz 5: Integer parsing
    // ========================================================================
    // Tests: parse_int<T> using std::from_chars
    // Goal: Find overflow, underflow, malformed number crashes
    (void)hm11::util::parse_int<uint8_t>(str_view);
    (void)hm11::util::parse_int<uint16_t>(str_view);
    (void)hm11::util::parse_int<uint32_t>(str_view);
    (void)hm11::util::parse_int<int8_t>(str_view);
    (void)hm11::util::parse_int<int16_t>(str_view);
    (void)hm11::util::parse_int<int32_t>(str_view);

    // ========================================================================
    // Fuzz 6: Temperature parsing
    // ========================================================================
    // Tests: parse_temperature (expects 0-120)
    // Goal: Find inputs that bypass range validation
    (void)hm11::util::parse_temperature(str_view);

    // ========================================================================
    // Fuzz 7: Humidity parsing
    // ========================================================================
    // Tests: parse_humidity (expects 0-100)
    // Goal: Find inputs that bypass range validation
    (void)hm11::util::parse_humidity(str_view);

    // ========================================================================
    // Fuzz 8: Digit validation
    // ========================================================================
    // Tests: is_all_digits, char_to_digit
    // Goal: Find incorrect digit validation
    (void)hm11::util::is_all_digits(str_view);
    if (size > 0) {
        if (data[0] >= '0' && data[0] <= '9') {
            (void)hm11::util::char_to_digit(static_cast<char>(data[0]));
        }
    }

    // ========================================================================
    // Fuzz 9: CRLF splitting
    // ========================================================================
    // Tests: split_at_crlf
    // Goal: Find crashes in string splitting logic
    (void)hm11::util::split_at_crlf(str_view);
    (void)hm11::util::split_at_crlf(input);

    // ========================================================================
    // Fuzz 10: UUID component parsing
    // ========================================================================
    // Tests: parse_uuid_component (4 hex chars)
    // Goal: Verify 4-character hex validation
    if (size >= 4) {
        (void)hm11::util::parse_uuid_component(str_view);
    }

    return 0;  // Non-zero return values are reserved for future use
}
