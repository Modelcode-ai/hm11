//
// fixed_string.hpp - Fixed-size string template with character validation
//
// Part of the HM-11 BLE driver C++ migration from Ada.
// This template provides compile-time sized strings with character validation,
// directly translating Ada's fixed-size character array types.
//
// Ada source: hm11.ads lines 91-270 showing various fixed-size arrays:
//   type MAC_Address is array (1 .. 12) of Hex_Character;
//   type PIN_Type is array (1 .. 6) of Digit_Character;
//   type iBeacon_UUID is array (1 .. 32) of Hex_Character;
//   etc.
//
// Design Decisions (from Migration Spec Decision 2):
// - Fixed capacity known at compile time (N template parameter)
// - No dynamic allocation, suitable for embedded/ISR contexts
// - Validation on every character insertion
// - Class encapsulation with private data
// - Direct mapping: Ada array type → FixedString<N, Validator>
//

#ifndef HM11_TYPES_FIXED_STRING_HPP
#define HM11_TYPES_FIXED_STRING_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace hm11 {

/// @brief Fixed-size string with compile-time character validation
///
/// This template provides Ada-equivalent semantics for fixed-size character
/// arrays with character type constraints. The string always contains exactly
/// N characters (fixed size, not variable length).
///
/// @tparam N Number of characters (excluding null terminator)
/// @tparam Validator Type with static is_valid(char) and validate(char, size_t) methods
///
/// Example usage:
/// @code
/// using MAC_Address = FixedString<12, HexCharValidator>;
/// constexpr MAC_Address mac{"B499CF123456"};  // OK
/// // MAC_Address invalid{"XYZ"};                // Assertion failure in debug
/// @endcode
///
/// Memory layout:
/// - Storage: std::array<char, N+1> with null terminator for C compatibility
/// - Always contains exactly N valid characters plus '\0'
/// - Stack-allocated, no heap usage
///
/// Validation:
/// - Compile-time: constexpr construction validates at compile time
/// - Runtime: Assertions in debug builds, no overhead in release
///
template <std::size_t N, typename Validator> class FixedString {
  public:
    /// @brief Size of the string (number of characters, excluding null terminator)
    static constexpr std::size_t CAPACITY = N;

    /// @brief Default constructor - initializes all characters to '0'
    ///
    /// This matches Ada's default initialization behavior where uninitialized
    /// arrays get default values. We use '0' as the most neutral character
    /// that passes both hex and digit validators.
    constexpr FixedString() noexcept : data{} {
        data.fill('0');
        data[N] = '\0'; // Ensure null termination

        // Validate default initialization
        for (std::size_t i = 0; i < N; ++i) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
            // Loop bounds guarantee index < N+1
            Validator::validate(data[i], i);
            // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    }

    /// @brief Construct from string literal or string_view
    /// @param str Source string (must be exactly N characters)
    ///
    /// This constructor validates that the input is exactly N characters
    /// and that each character passes the Validator.
    ///
    /// @note In constexpr context, validation happens at compile time.
    /// @note In runtime context, assertions fire in debug builds only.
    constexpr explicit FixedString(std::string_view str) noexcept : data{} {
        assert(str.size() == N && "String length must match FixedString capacity");

        for (std::size_t i = 0; i < N; ++i) {
            Validator::validate(str[i], i);
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
            // Loop bounds guarantee index < N+1
            data[i] = str[i];
            // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        data[N] = '\0'; // Null terminator
    }

    /// @brief Construct from C-string
    /// @param str Null-terminated C-string (must be exactly N characters)
    constexpr explicit FixedString(const char* str) noexcept : FixedString(std::string_view(str)) {}

    /// @brief Access character at index with bounds checking
    /// @param index Character position (0 to N-1)
    /// @return Reference to character at index
    constexpr char& operator[](std::size_t index) noexcept {
        assert(index < N && "Index out of bounds");
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Assert checks bounds
        return data[index];
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Access character at index with bounds checking (const version)
    /// @param index Character position (0 to N-1)
    /// @return Const reference to character at index
    constexpr const char& operator[](std::size_t index) const noexcept {
        assert(index < N && "Index out of bounds");
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Assert checks bounds
        return data[index];
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Get pointer to underlying null-terminated C-string
    /// @return Pointer to internal character array
    constexpr const char* c_str() const noexcept { return data.data(); }

    /// @brief Get pointer to underlying character data
    /// @return Pointer to internal character array
    constexpr const char* get_data() const noexcept { return data.data(); }

    /// @brief Get non-const pointer to underlying character data
    /// @return Pointer to internal character array
    ///
    /// @warning Caller must ensure modified characters still pass validation
    constexpr char* get_data() noexcept { return data.data(); }

    /// @brief Get the number of characters in the string
    /// @return Always returns N (fixed size)
    constexpr std::size_t size() const noexcept { return N; }

    /// @brief Get the capacity of the string
    /// @return Always returns N (fixed size)
    constexpr std::size_t length() const noexcept { return N; }

    /// @brief Convert to string_view for zero-copy parsing
    /// @return string_view over the character data (excluding null terminator)
    constexpr operator std::string_view() const noexcept { return std::string_view(data.data(), N); }

    /// @brief Explicit conversion to string_view
    /// @return string_view over the character data
    constexpr std::string_view view() const noexcept { return std::string_view(data.data(), N); }

    /// @brief Equality comparison
    /// @param other FixedString to compare with
    /// @return true if all characters match
    constexpr bool operator==(const FixedString& other) const noexcept {
        return std::equal(data.begin(), data.begin() + N, other.data.begin());
    }

    /// @brief Inequality comparison
    /// @param other FixedString to compare with
    /// @return true if any character differs
    constexpr bool operator!=(const FixedString& other) const noexcept { return !(*this == other); }

    /// @brief Compare with string_view
    /// @param str string_view to compare with
    /// @return true if contents match
    constexpr bool operator==(std::string_view str) const noexcept {
        return str.size() == N && std::equal(data.begin(), data.begin() + N, str.begin());
    }

    /// @brief Compare with string_view (inequality)
    /// @param str string_view to compare with
    /// @return true if contents differ
    constexpr bool operator!=(std::string_view str) const noexcept { return !(*this == str); }

    /// @brief Lexicographic less-than comparison
    /// @param other FixedString to compare with
    /// @return true if this string is lexicographically less than other
    constexpr bool operator<(const FixedString& other) const noexcept {
        return std::lexicographical_compare(data.begin(), data.begin() + N, other.data.begin(), other.data.begin() + N);
    }

    /// @brief Lexicographic less-than-or-equal comparison
    /// @param other FixedString to compare with
    /// @return true if this string is lexicographically <= other
    constexpr bool operator<=(const FixedString& other) const noexcept { return !(other < *this); }

    /// @brief Lexicographic greater-than comparison
    /// @param other FixedString to compare with
    /// @return true if this string is lexicographically greater than other
    constexpr bool operator>(const FixedString& other) const noexcept { return other < *this; }

    /// @brief Lexicographic greater-than-or-equal comparison
    /// @param other FixedString to compare with
    /// @return true if this string is lexicographically >= other
    constexpr bool operator>=(const FixedString& other) const noexcept { return !(*this < other); }

    /// @brief Get iterator to beginning
    constexpr auto begin() noexcept { return data.begin(); }
    constexpr auto begin() const noexcept { return data.begin(); }
    constexpr auto cbegin() const noexcept { return data.cbegin(); }

    /// @brief Get iterator to end (excluding null terminator)
    constexpr auto end() noexcept { return data.begin() + N; }
    constexpr auto end() const noexcept { return data.begin() + N; }
    constexpr auto cend() const noexcept { return data.cbegin() + N; }

  private:
    // Storage: N characters + 1 null terminator
    std::array<char, N + 1> data;
};

} // namespace hm11

#endif // HM11_TYPES_FIXED_STRING_HPP
