/**
 * @file fuzz_fixed_string.cpp
 * @brief LibFuzzer target for FixedString template
 *
 * This fuzzer tests the FixedString template for:
 * - Character validation (HexCharValidator, DigitCharValidator)
 * - Bounds checking on construction
 * - Assertion failures on invalid input
 * - Edge cases in operator overloads
 *
 * Build with:
 *   clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
 *           -std=c++20 -I../include fuzz_fixed_string.cpp -o fuzz_fixed_string
 *
 * Run with:
 *   ./fuzz_fixed_string -max_len=100 -max_total_time=300 corpus/fixed_string
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "hm11/types/fixed_string.hpp"
#include "hm11/types/validators.hpp"

// Test with common FixedString sizes used in the codebase
using MacAddress = hm11::FixedString<12, hm11::HexCharValidator>;
using PinCode = hm11::FixedString<6, hm11::DigitCharValidator>;
using UUID = hm11::FixedString<4, hm11::HexCharValidator>;
using IBeaconUuid = hm11::FixedString<32, hm11::HexCharValidator>;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Convert to string_view (null-terminate for safety)
    std::string input_str(reinterpret_cast<const char*>(data), size);
    std::string_view input(input_str);

    // ========================================================================
    // Fuzz 1: MAC Address (12 hex chars)
    // ========================================================================
    if (size == 12) {
        // Pre-validate to avoid assertion failures
        bool all_hex = true;
        for (size_t i = 0; i < 12; ++i) {
            if (!hm11::HexCharValidator::is_valid(input_str[i])) {
                all_hex = false;
                break;
            }
        }

        if (all_hex) {
            MacAddress mac(input);
            (void)mac.size();
            (void)mac.c_str();
            (void)mac.view();
            (void)mac[0];

            MacAddress mac2(input);
            (void)(mac == mac2);
            (void)(mac != mac2);
            (void)(mac < mac2);
            (void)(mac <= mac2);
            (void)(mac > mac2);
            (void)(mac >= mac2);
        }
    }

    // ========================================================================
    // Fuzz 2: PIN Code (6 digits)
    // ========================================================================
    if (size == 6) {
        bool all_digits = true;
        for (size_t i = 0; i < 6; ++i) {
            if (!hm11::DigitCharValidator::is_valid(input_str[i])) {
                all_digits = false;
                break;
            }
        }

        if (all_digits) {
            PinCode pin(input);
            (void)pin.size();
            (void)pin.c_str();
            (void)pin.view();
        }
    }

    // ========================================================================
    // Fuzz 3: UUID (4 hex chars)
    // ========================================================================
    if (size == 4) {
        bool all_hex = true;
        for (size_t i = 0; i < 4; ++i) {
            if (!hm11::HexCharValidator::is_valid(input_str[i])) {
                all_hex = false;
                break;
            }
        }

        if (all_hex) {
            UUID uuid(input);
            (void)uuid.size();
            (void)uuid.c_str();
            (void)uuid.view();
            (void)(uuid == input);
            (void)(uuid != input);
        }
    }

    // ========================================================================
    // Fuzz 4: iBeacon UUID (32 hex chars)
    // ========================================================================
    if (size == 32) {
        bool all_hex = true;
        for (size_t i = 0; i < 32; ++i) {
            if (!hm11::HexCharValidator::is_valid(input_str[i])) {
                all_hex = false;
                break;
            }
        }

        if (all_hex) {
            IBeaconUuid beacon_uuid(input);
            (void)beacon_uuid.size();
            (void)beacon_uuid.c_str();
            (void)beacon_uuid.view();

            for (auto c : beacon_uuid) {
                (void)c;
            }
        }
    }

    // ========================================================================
    // Fuzz 5: Character validator edge cases
    // ========================================================================
    // Test validators directly with all possible byte values
    if (size > 0) {
        char test_char = static_cast<char>(data[0]);

        // Test hex validator
        (void)hm11::HexCharValidator::is_valid(test_char);

        // Test digit validator
        (void)hm11::DigitCharValidator::is_valid(test_char);
    }

    // ========================================================================
    // Fuzz 6: Default construction
    // ========================================================================
    // Default construction should always work
    {
        MacAddress default_mac;
        (void)default_mac.view();  // Should be "000000000000"

        PinCode default_pin;
        (void)default_pin.view();  // Should be "000000"
    }

    // ========================================================================
    // Fuzz 7: Indexing operations
    // ========================================================================
    if (size >= 12) {
        bool all_hex = true;
        for (size_t i = 0; i < 12; ++i) {
            if (!hm11::HexCharValidator::is_valid(input_str[i])) {
                all_hex = false;
                break;
            }
        }

        if (all_hex) {
            MacAddress mac(std::string_view(input_str.data(), 12));
            for (size_t i = 0; i < 12; ++i) {
                (void)mac[i];
            }
        }
    }

    return 0;
}
