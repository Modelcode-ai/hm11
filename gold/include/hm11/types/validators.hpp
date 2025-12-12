//
// validators.hpp - Character validators for FixedString template
//
// Part of the HM-11 BLE driver C++ migration from Ada.
// This file provides validator types for compile-time and runtime validation
// of character constraints in fixed-size strings.
//
// Ada source: hm11.ads lines 78-79
//   type Hex_Character   is new Character range '0' .. 'F';
//   type Digit_Character is new Character range '0' .. '9';
//

#ifndef HM11_TYPES_VALIDATORS_HPP
#define HM11_TYPES_VALIDATORS_HPP

#include <cassert>
#include <cstddef>

namespace hm11 {

/// @brief Validator for hexadecimal characters (uppercase only)
///
/// Validates that characters are in the ranges:
///   - '0' to '9' (digits)
///   - 'A' to 'F' (uppercase hex letters)
///
/// This matches Ada's Hex_Character type definition:
///   type Hex_Character is new Character range '0' .. 'F';
///
/// Note: Ada's range '0'..'F' includes some non-hex characters
/// (e.g., ':', ';', '<', '=', '>', '?', '@'), but actual usage
/// in the codebase expects only valid hex digits. This validator
/// implements the intended semantic behavior.
struct HexCharValidator {
    /// @brief Check if a character is a valid hexadecimal character
    /// @param c Character to validate
    /// @return true if c is '0'-'9' or 'A'-'F', false otherwise
    static constexpr bool is_valid(char c) noexcept { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); }

    /// @brief Validate a character, asserting if invalid
    /// @param c Character to validate
    /// @param index Position in string (for error reporting)
    ///
    /// In debug builds, this asserts if the character is invalid.
    /// In release builds, the assertion is optimized away.
    static constexpr void validate([[maybe_unused]] char c, [[maybe_unused]] std::size_t index) noexcept {
        assert(is_valid(c) && "Invalid hexadecimal character (expected '0'-'9' or 'A'-'F')");
    }

    /// @brief Get validator name for debugging
    static constexpr const char* name() noexcept { return "HexCharValidator"; }
};

/// @brief Validator for decimal digit characters
///
/// Validates that characters are in the range:
///   - '0' to '9' (digits only)
///
/// This matches Ada's Digit_Character type definition:
///   type Digit_Character is new Character range '0' .. '9';
struct DigitCharValidator {
    /// @brief Check if a character is a valid decimal digit
    /// @param c Character to validate
    /// @return true if c is '0'-'9', false otherwise
    static constexpr bool is_valid(char c) noexcept { return c >= '0' && c <= '9'; }

    /// @brief Validate a character, asserting if invalid
    /// @param c Character to validate
    /// @param index Position in string (for error reporting)
    ///
    /// In debug builds, this asserts if the character is invalid.
    /// In release builds, the assertion is optimized away.
    static constexpr void validate([[maybe_unused]] char c, [[maybe_unused]] std::size_t index) noexcept {
        assert(is_valid(c) && "Invalid digit character (expected '0'-'9')");
    }

    /// @brief Get validator name for debugging
    static constexpr const char* name() noexcept { return "DigitCharValidator"; }
};

} // namespace hm11

#endif // HM11_TYPES_VALIDATORS_HPP
