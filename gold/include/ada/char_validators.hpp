// char_validators.hpp - Character constraints for ada::FixedString.
//
// A validator provides `static constexpr bool is_valid(char)` and `static constexpr char DEFAULT_CHAR`,
// the value of a default-constructed element (which must itself be valid).

#ifndef ADA_CHAR_VALIDATORS_HPP
#define ADA_CHAR_VALIDATORS_HPP

#include <concepts>

namespace ada {

template <typename V>
concept CharValidator = requires(char c) {
    { V::is_valid(c) } -> std::same_as<bool>;
    { V::DEFAULT_CHAR } -> std::convertible_to<char>;
} && V::is_valid(V::DEFAULT_CHAR);

/// Ada `type T is new Character range Lo .. Hi`, exactly.
/// Note: Ada's `Hex_Character is new Character range '0' .. 'F'` is CharRange<'0', 'F'>, which also accepts
/// ":;<=>?@"; use HexDigit for hexadecimal digits only.
template <char Lo, char Hi> struct CharRange {
    static_assert(static_cast<unsigned char>(Lo) <= static_cast<unsigned char>(Hi), "Lo must be <= Hi");

    static constexpr char DEFAULT_CHAR = Lo;

    // Compared as unsigned so ranges above 0x7F work where char is signed (Ada Character is 0 .. 255).
    static constexpr bool is_valid(char c) noexcept {
        auto u = static_cast<unsigned char>(c);
        return u >= static_cast<unsigned char>(Lo) && u <= static_cast<unsigned char>(Hi);
    }
};

/// Uppercase hexadecimal digits '0'-'9', 'A'-'F'.
struct HexDigit {
    static constexpr char DEFAULT_CHAR = '0';

    static constexpr bool is_valid(char c) noexcept { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); }
};

/// Ada `type Digit_Character is new Character range '0' .. '9'`.
using DecimalDigit = CharRange<'0', '9'>;

/// Any character, as in Ada String.
struct AnyChar {
    static constexpr char DEFAULT_CHAR = ' ';

    static constexpr bool is_valid(char /*c*/) noexcept { return true; }
};

} // namespace ada

#endif // ADA_CHAR_VALIDATORS_HPP
