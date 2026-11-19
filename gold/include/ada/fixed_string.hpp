// fixed_string.hpp - Ada constrained character arrays for C++20.
//
// Maps `type X is array (1 .. N) of C` (C a constrained character type) to
// `using X = ada::FixedString<N, Validator, struct XTag>;`, and Ada `String (1 .. N)` to `ada::String<N>`.
// Indexing is 0-based; FIRST and LAST mirror Ada's X'First and X'Last, so `X (X'Last)` is `x[X::LAST]`.
// Every write is validated: construction, `x[i] = c` (through a proxy) and `x.set(i, c)`.
//
// Invalid characters, wrong lengths and out-of-range indexes:
// - Constant evaluation: always a compile error, including with NDEBUG.
// - String literal of the wrong length: always a compile error.
// - Runtime: assert (debug builds only); release never reads past the input, but stores unchecked characters.
// Adjust out-of-range behavior in release builds to match the project's goals.
// Untrusted input (e.g. device replies) should go through try_make() or is_valid() instead.

#ifndef ADA_FIXED_STRING_HPP
#define ADA_FIXED_STRING_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "ada/char_validators.hpp"
#include "ada/detail/constraint_check.hpp"

namespace ada {

/// Fixed-length character array whose elements all satisfy Validator.
///
/// @tparam N         Number of characters
/// @tparam Validator ada::CharValidator constraining each character
/// @tparam Tag       Distinct empty type per Ada type; prevents mixing types with equal N and Validator
template <std::size_t N, CharValidator Validator, typename Tag> class FixedString {
    static_assert(N >= 1, "FixedString must hold at least one character");

  public:
    using ValidatorType = Validator;
    using TagType = Tag;

    static constexpr std::size_t LENGTH = N;   ///< Ada X'Length
    static constexpr std::size_t FIRST = 0;    ///< Ada X'First, as a 0-based index
    static constexpr std::size_t LAST = N - 1; ///< Ada X'Last, as a 0-based index

    /// Proxy returned by the non-const operator[] so that `x[i] = c` is validated.
    class Reference {
      public:
        Reference(const Reference&) noexcept = default;
        ~Reference() = default;

        constexpr Reference& operator=(char c) noexcept {
            owner_->set(index_, c);
            return *this;
        }

        // Assigns the referenced character, so `a[0] = b[1]` copies a value rather than rebinding.
        constexpr Reference& operator=(const Reference& other) noexcept {
            if (this != &other) {
                *this = static_cast<char>(other);
            }
            return *this;
        }

        constexpr operator char() const noexcept { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            return std::as_const(*owner_)[index_];
        }

      private:
        friend class FixedString;
        constexpr Reference(FixedString& owner_string, std::size_t element_index) noexcept
            : owner_{&owner_string}, index_{element_index} {}

        FixedString* owner_;
        std::size_t index_;
    };

    /// Fills every element with Validator::DEFAULT_CHAR.
    constexpr FixedString() noexcept : chars_{} { std::fill_n(chars_.begin(), N, Validator::DEFAULT_CHAR); }

    /// From a string literal of exactly N characters.
    template <std::size_t M>
        requires(M == N + 1)
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays): array reference carries the length
    explicit constexpr FixedString(const char (&str)[M]) noexcept : FixedString(std::string_view{str, N}) {
        check_length(str[N] == '\0');
    }

    /// A literal of the wrong length does not compile. Explicit so it is never an implicit conversion candidate.
    template <std::size_t M>
        requires(M != N + 1)
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays): array reference carries the length
    explicit FixedString(const char (&str)[M]) = delete;

    /// From runtime text; checked like a literal. Never reads past str, even when the length is wrong.
    explicit constexpr FixedString(std::string_view str) noexcept : chars_{} {
        check_length(str.size() == N);
        for (std::size_t i = 0; i < N; ++i) {
            char c = i < str.size() ? str[i] : Validator::DEFAULT_CHAR;
            check_char(c);
            chars_[i] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index): i < N
        }
    }

    /// True if str has exactly N characters, all valid (Ada 'Valid).
    [[nodiscard]] static constexpr bool is_valid(std::string_view str) noexcept {
        return str.size() == N && std::all_of(str.begin(), str.end(), [](char c) { return Validator::is_valid(c); });
    }

    /// Checked construction for untrusted input: std::nullopt instead of an assert.
    [[nodiscard]] static constexpr std::optional<FixedString> try_make(std::string_view str) noexcept {
        if (!is_valid(str)) {
            return std::nullopt;
        }
        return FixedString{str};
    }

    [[nodiscard]] constexpr char operator[](std::size_t i) const noexcept {
        check_index(i);
        return chars_[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index): checked above
    }

    [[nodiscard]] constexpr Reference operator[](std::size_t i) noexcept {
        check_index(i);
        return Reference{*this, i};
    }

    constexpr void set(std::size_t i, char c) noexcept {
        check_index(i);
        check_char(c);
        chars_[i] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index): checked above
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {chars_.data(), N}; }
    [[nodiscard]] constexpr const char* c_str() const noexcept { return chars_.data(); }

    [[nodiscard]] constexpr auto begin() const noexcept { return chars_.cbegin(); }
    [[nodiscard]] constexpr auto end() const noexcept { return chars_.cbegin() + N; }

    [[nodiscard]] friend constexpr bool operator==(const FixedString&, const FixedString&) noexcept = default;
    // Ada orders Characters by position (0 .. 255), so compare as unsigned char whether or not char is signed.
    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(
        const FixedString& lhs,
        const FixedString& rhs) noexcept {
        return std::lexicographical_compare_three_way(
            lhs.begin(),
            lhs.end(),
            rhs.begin(),
            rhs.end(),
            [](char a, char b) { return static_cast<unsigned char>(a) <=> static_cast<unsigned char>(b); });
    }

    [[nodiscard]] friend constexpr bool operator==(const FixedString& lhs, std::string_view rhs) noexcept {
        return lhs.view() == rhs;
    }

  private:
    static constexpr void check_length(bool ok) noexcept {
        if (std::is_constant_evaluated() && !ok) {
            detail::constraint_error_in_constant_expression();
        }
        assert(ok && "String length does not match FixedString length");
    }

    static constexpr void check_char(char c) noexcept {
        bool ok = Validator::is_valid(c);
        if (std::is_constant_evaluated() && !ok) {
            detail::constraint_error_in_constant_expression();
        }
        assert(ok && "Invalid character for FixedString validator");
    }

    static constexpr void check_index(std::size_t i) noexcept {
        bool ok = i < N;
        if (std::is_constant_evaluated() && !ok) {
            detail::constraint_error_in_constant_expression();
        }
        assert(ok && "FixedString index out of range");
    }

    std::array<char, N + 1> chars_; ///< N characters plus a '\0' terminator for c_str()
};

struct StringTag {};

/// Ada `String (1 .. N)`: any characters. All Strings of one length share a type, like Ada subtypes of String.
template <std::size_t N> using String = FixedString<N, AnyChar, StringTag>;

} // namespace ada

#endif // ADA_FIXED_STRING_HPP
