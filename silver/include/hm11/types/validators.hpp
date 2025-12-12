#pragma once

#include <cctype>
#include <type_traits>

namespace hm11 {

/// @brief Character validation concepts and validator classes
///
/// These validators provide compile-time and runtime character validation
/// for FixedString templates, equivalent to Ada's character constraints
/// like `type Hex_Character is new Character range '0' .. 'F';`

/// @brief Concept for character validators
/// A validator must have a static constexpr validate function
template <typename T>
concept CharacterValidator = requires(char c) {
    { T::validate(c) } -> std::convertible_to<bool>;
};

/// @brief Validator for hexadecimal characters (0-9, A-F, a-f)
/// Maps to Ada: `type Hex_Character is new Character range '0' .. 'F';`
/// Note: Ada version only accepts uppercase, but we allow both for flexibility
struct HexCharValidator {
    /// @brief Validate that character is a valid hexadecimal digit
    /// @param c Character to validate
    /// @return true if character is 0-9, A-F, or a-f
    static constexpr bool validate(char c) noexcept {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "HexChar"; }
};

/// @brief Validator for decimal digit characters (0-9)
/// Maps to Ada: `type Digit_Character is new Character range '0' .. '9';`
struct DigitCharValidator {
    /// @brief Validate that character is a decimal digit
    /// @param c Character to validate
    /// @return true if character is 0-9
    static constexpr bool validate(char c) noexcept { return c >= '0' && c <= '9'; }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "DigitChar"; }
};

/// @brief Validator that accepts any ASCII character
/// Used as a default or for unconstrained strings
struct AnyCharValidator {
    /// @brief Validate that character is a valid ASCII character
    /// @param c Character to validate
    /// @return true if character is printable ASCII or common whitespace
    static constexpr bool validate(char c) noexcept {
        // Accept printable ASCII and common whitespace, plus extended ASCII for international characters
        return (c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r' || (static_cast<unsigned char>(c) >= 128);
    }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "AnyChar"; }
};

/// @brief Validator for uppercase alphabetic characters (A-Z)
struct UpperAlphaValidator {
    /// @brief Validate that character is an uppercase letter
    /// @param c Character to validate
    /// @return true if character is A-Z
    static constexpr bool validate(char c) noexcept { return c >= 'A' && c <= 'Z'; }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "UpperAlpha"; }
};

/// @brief Validator for lowercase alphabetic characters (a-z)
struct LowerAlphaValidator {
    /// @brief Validate that character is a lowercase letter
    /// @param c Character to validate
    /// @return true if character is a-z
    static constexpr bool validate(char c) noexcept { return c >= 'a' && c <= 'z'; }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "LowerAlpha"; }
};

/// @brief Validator for alphanumeric characters (0-9, A-Z, a-z)
struct AlphaNumValidator {
    /// @brief Validate that character is alphanumeric
    /// @param c Character to validate
    /// @return true if character is 0-9, A-Z, or a-z
    static constexpr bool validate(char c) noexcept {
        return DigitCharValidator::validate(c) || UpperAlphaValidator::validate(c) || LowerAlphaValidator::validate(c);
    }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "AlphaNum"; }
};

/// @brief Custom validator for MAC address characters (hex digits and colons)
/// Used specifically for MAC address parsing
struct MacCharValidator {
    /// @brief Validate character for MAC address format
    /// @param c Character to validate
    /// @return true if character is hex digit or colon
    static constexpr bool validate(char c) noexcept { return HexCharValidator::validate(c) || c == ':'; }

    /// @brief Get a descriptive name for this validator
    static constexpr const char* name() noexcept { return "MacChar"; }
};

// Static assertions to ensure validators meet the concept requirements
static_assert(CharacterValidator<HexCharValidator>);
static_assert(CharacterValidator<DigitCharValidator>);
static_assert(CharacterValidator<AnyCharValidator>);
static_assert(CharacterValidator<UpperAlphaValidator>);
static_assert(CharacterValidator<LowerAlphaValidator>);
static_assert(CharacterValidator<AlphaNumValidator>);
static_assert(CharacterValidator<MacCharValidator>);

} // namespace hm11