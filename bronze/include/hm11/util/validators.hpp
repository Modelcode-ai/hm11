/**
 * @file validators.hpp
 * @brief Character validators for FixedString template
 *
 * This file contains validator classes for the FixedString template.
 * Validators are policy classes that determine whether a character
 * is valid for a particular string type.
 */

#ifndef HM11_UTIL_VALIDATORS_HPP
#define HM11_UTIL_VALIDATORS_HPP

#include <cctype>
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace hm11::util {

/**
 * @brief Concept for character validator policy classes
 *
 * Any class that satisfies this concept can be used as a validator
 * for FixedString. The class must provide a static is_valid method
 * that checks if a character is valid.
 */
template <typename T>
concept CharacterValidator = requires(char c) {
    { T::is_valid(c) } -> std::same_as<bool>;
    { T::description() } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Validator for hexadecimal characters
 *
 * Validates that a character is a valid hexadecimal digit (0-9, A-F).
 * Equivalent to the Ada Hex_Character type.
 */
struct HexCharValidator {
    /**
     * @brief Check if a character is a valid hexadecimal digit
     *
     * @param c Character to check
     * @return true if the character is a valid hexadecimal digit (0-9, A-F)
     * @return false otherwise
     */
    [[nodiscard]] static constexpr bool is_valid(char c) noexcept {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept {
        return "Hexadecimal character (0-9, A-F)";
    }
};

/**
 * @brief Validator for decimal digit characters
 *
 * Validates that a character is a decimal digit (0-9).
 * Equivalent to the Ada Digit_Character type.
 */
struct DigitValidator {
    /**
     * @brief Check if a character is a valid decimal digit
     *
     * @param c Character to check
     * @return true if the character is a valid decimal digit (0-9)
     * @return false otherwise
     */
    [[nodiscard]] static constexpr bool is_valid(char c) noexcept { return c >= '0' && c <= '9'; }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept { return "Decimal digit character (0-9)"; }
};

/**
 * @brief Validator that accepts any character
 *
 * This validator accepts any character without restrictions.
 */
struct AnyCharValidator {
    /**
     * @brief Check if a character is valid (always returns true)
     *
     * @param c Character to check (unused)
     * @return true always
     */
    [[nodiscard]] static constexpr bool is_valid(char /*c*/) noexcept { return true; }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept { return "Any character"; }
};

/**
 * @brief Validator for alphabetic characters
 *
 * Validates that a character is an alphabetic character (A-Z, a-z).
 */
struct AlphaCharValidator {
    /**
     * @brief Check if a character is a valid alphabetic character
     *
     * @param c Character to check
     * @return true if the character is a valid alphabetic character (A-Z, a-z)
     * @return false otherwise
     */
    [[nodiscard]] static constexpr bool is_valid(char c) noexcept {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept { return "Alphabetic character (A-Z, a-z)"; }
};

/**
 * @brief Validator for alphanumeric characters
 *
 * Validates that a character is an alphanumeric character (A-Z, a-z, 0-9).
 */
struct AlphaNumValidator {
    /**
     * @brief Check if a character is a valid alphanumeric character
     *
     * @param c Character to check
     * @return true if the character is a valid alphanumeric character (A-Z, a-z, 0-9)
     * @return false otherwise
     */
    [[nodiscard]] static constexpr bool is_valid(char c) noexcept {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept {
        return "Alphanumeric character (A-Z, a-z, 0-9)";
    }
};

/**
 * @brief Validator for printable characters
 *
 * Validates that a character is a printable character (ASCII 32-126).
 */
struct PrintableCharValidator {
    /**
     * @brief Check if a character is a valid printable character
     *
     * @param c Character to check
     * @return true if the character is a valid printable character (ASCII 32-126)
     * @return false otherwise
     */
    [[nodiscard]] static constexpr bool is_valid(char c) noexcept { return c >= ' ' && c <= '~'; }

    /**
     * @brief Get a description of the validator
     *
     * @return constexpr std::string_view Description
     */
    [[nodiscard]] static constexpr std::string_view description() noexcept {
        return "Printable character (ASCII 32-126)";
    }
};

} // namespace hm11::util

#endif // HM11_UTIL_VALIDATORS_HPP