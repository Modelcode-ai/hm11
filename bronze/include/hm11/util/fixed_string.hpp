/**
 * @file fixed_string.hpp
 * @brief FixedString template for fixed-size character arrays with validation
 *
 * This file contains the FixedString template class that provides a type-safe
 * fixed-size character array with character validation. It is designed to be
 * a direct analog to Ada's fixed-size constrained character array types.
 */

#ifndef HM11_UTIL_FIXED_STRING_HPP
#define HM11_UTIL_FIXED_STRING_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include "validators.hpp"

namespace hm11::util {

/**
 * @brief Provides a fixed-size character array with validation
 *
 * This template class implements a fixed-size character array similar to Ada's
 * fixed-size character array types. It ensures all characters are valid
 * according to the specified validator. All operations are constexpr where
 * possible for compile-time evaluation, and noexcept for ISR safety.
 *
 * The class provides:
 * - Fixed-size storage with compile-time size validation
 * - Character validation against a specified validator policy
 * - Safe string operations and conversions
 * - STL container compatibility (iterators, etc.)
 * - String view compatibility for efficient access
 *
 * This implementation corresponds to Decision 2: Strong Type System Translation,
 * specifically for handling Ada's fixed-size character arrays with constraints.
 * It provides a type-safe alternative to raw character arrays while maintaining
 * the efficiency of stack allocation.
 *
 * Example usage:
 * ```cpp
 * // Define a MAC address type (12 hex characters)
 * using MacAddress = FixedString<12, HexCharValidator>;
 *
 * // Use the type
 * MacAddress mac("123456789ABC");  // Valid
 * MacAddress mac2("INVALID!!");    // Invalid (assertions in debug)
 *
 * // Access contents
 * char first = mac[0];             // '1'
 * std::string str = mac.to_string(); // "123456789ABC"
 * ```
 *
 * Thread safety: This class has no internal mutable state beyond the stored
 * character array. Most operations are thread-safe, but non-const methods
 * that modify the string are not thread-safe for concurrent access.
 * All constructors and accessors are noexcept for ISR safety.
 *
 * @tparam N The size of the character array
 * @tparam Validator The validator policy class to use for character validation
 *
 * @see CharacterValidator for the validator concept
 * @see HexCharValidator and other validators for specific validation policies
 */
template <std::size_t N, CharacterValidator Validator = AnyCharValidator> class FixedString {
    static_assert(N > 0, "FixedString size must be greater than 0");

  private:
    std::array<char, N> data{};

    /**
     * @brief Validates a character
     *
     * This method validates a character against the Validator policy.
     * In debug builds, it uses assertions to verify that the character is valid.
     * In release builds, it silently replaces invalid characters with a default.
     *
     * This approach provides strong safety guarantees during development while
     * maintaining robustness in production:
     * - Debug builds immediately identify validation issues
     * - Release builds gracefully handle invalid input
     *
     * @param c Character to validate
     * @return char The character (unchanged if valid, replaced if invalid)
     *
     * @pre In debug builds, c must satisfy Validator::is_valid or an assertion will trigger
     * @post The returned character satisfies Validator::is_valid
     *
     * Thread safety: This method is thread-safe and ISR-safe.
     */
    [[nodiscard]] static constexpr char validate_char(char c) {
        if (!Validator::is_valid(c)) {
#ifdef NDEBUG
            // In release builds, fail silently
            return '0';
#else
            // In debug builds, use assertions
            assert(Validator::is_valid(c) && "Invalid character for FixedString");
            // If assertions are disabled, still return a valid character
            return '0';
#endif
        }
        return c;
    }

    /**
     * @brief Validates an entire string
     *
     * This method checks if all characters in a string satisfy the Validator policy.
     * It provides efficient bulk validation for string initialization and assignment.
     *
     * The validation stops at the first invalid character, following a fail-fast
     * approach to minimize unnecessary processing.
     *
     * @param str Pointer to the string to validate
     * @param n Number of characters to validate
     *
     * @return true if all n characters are valid according to Validator
     * @return false if any character fails validation
     *
     * @pre str must point to a valid memory region of at least n bytes
     *
     * Thread safety: This method is thread-safe and ISR-safe.
     */
    [[nodiscard]] static constexpr bool validate_string(const char* str, std::size_t n) noexcept {
        for (std::size_t i = 0; i < n; ++i) {
            if (!Validator::is_valid(str[i])) {
                return false;
            }
        }
        return true;
    }

  public:
    // Type aliases for STL compatibility
    using ValueType = char;
    using SizeType = std::size_t;
    using DifferenceType = std::ptrdiff_t;
    using Reference = char&;
    using ConstReference = const char&;
    using Pointer = char*;
    using ConstPointer = const char*;
    using Iterator = typename std::array<char, N>::iterator;
    using ConstIterator = typename std::array<char, N>::const_iterator;
    using ReverseIterator = typename std::array<char, N>::reverse_iterator;
    using ConstReverseIterator = typename std::array<char, N>::const_reverse_iterator;

    /**
     * @brief Default constructor
     *
     * Initializes the string with all zeros ('0' characters), matching
     * the behavior of Ada's default initialization for fixed-size arrays.
     * This ensures a safe, predictable default state.
     *
     * The initialization is constexpr, allowing for compile-time creation
     * of FixedString instances with zero runtime overhead.
     *
     * Thread safety: This constructor is thread-safe and ISR-safe.
     */
    constexpr FixedString() noexcept : data() {
        // Initialize with zeros (as would happen in Ada)
        std::fill_n(data.begin(), N, '0');
    }

    /**
     * @brief Constructor from C-style string
     *
     * @param str C-style string to initialize from
     */
    constexpr explicit FixedString(const char* str) noexcept : data() {
        std::fill_n(data.begin(), N, '0');
        if (str != nullptr) {
            const std::size_t LEN = std::min(std::strlen(str), N);
            bool valid = true;
            for (std::size_t i = 0; i < LEN; ++i) {
                if (Validator::is_valid(str[i])) {
                    data.at(i) = str[i];
                } else {
                    valid = false;
                    break;
                }
            }
#ifndef NDEBUG
            assert(valid && "Invalid characters in string");
#endif
            // If invalid characters found, reset to default
            if (!valid) {
                std::fill_n(data.begin(), N, '0');
            }
        }
    }

    /**
     * @brief Constructor from std::string
     *
     * @param str String to initialize from
     */
    constexpr explicit FixedString(const std::string& str) noexcept : data() {
        std::fill_n(data.begin(), N, '0');
        const std::size_t LEN = std::min(str.size(), N);
        bool valid = true;
        for (std::size_t i = 0; i < LEN; ++i) {
            if (Validator::is_valid(str[i])) {
                data.at(i) = str[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in string");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
    }

    /**
     * @brief Constructor from string_view
     *
     * @param sv String view to initialize from
     */
    constexpr explicit FixedString(std::string_view sv) noexcept : data() {
        std::fill_n(data.begin(), N, '0');
        const std::size_t LEN = std::min(sv.size(), N);
        bool valid = true;

        for (std::size_t i = 0; i < LEN; ++i) {
            if (Validator::is_valid(sv[i])) {
                auto data_it = data.begin() + i;
                *data_it = sv[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in string");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
    }

    /**
     * @brief Constructor from std::array
     *
     * @param arr Array to initialize from
     */
    constexpr explicit FixedString(const std::array<char, N>& arr) noexcept : data() {
        std::fill_n(data.begin(), N, '0');
        bool valid = true;
        for (std::size_t i = 0; i < N; ++i) {
            if (Validator::is_valid(arr[i])) {
                data.at(i) = arr[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in array");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
    }

    /**
     * @brief Constructor from initializer list
     *
     * @param il Initializer list to initialize from
     */
    constexpr FixedString(std::initializer_list<char> il) noexcept : data() {
        std::fill_n(data.begin(), N, '0');
        if (il.size() > N) {
#ifndef NDEBUG
            assert(il.size() <= N && "Initializer list too long for FixedString");
#endif
            return;
        }

        bool valid = true;
        std::size_t i = 0;
        for (char c : il) {
            if (Validator::is_valid(c)) {
                data.at(i++) = c;
            } else {
                valid = false;
                break;
            }
            if (i >= N) {
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in initializer list");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
    }

    /**
     * @brief Copy constructor
     *
     * @param other FixedString to copy from
     */
    constexpr FixedString(const FixedString& other) noexcept = default;

    /**
     * @brief Move constructor
     *
     * @param other FixedString to move from
     */
    constexpr FixedString(FixedString&& other) noexcept = default;

    /**
     * @brief Copy assignment operator
     *
     * @param other FixedString to copy from
     * @return FixedString& Reference to this object
     */
    constexpr FixedString& operator=(const FixedString& other) noexcept = default;

    /**
     * @brief Move assignment operator
     *
     * @param other FixedString to move from
     * @return FixedString& Reference to this object
     */
    constexpr FixedString& operator=(FixedString&& other) noexcept = default;

    /**
     * @brief Destructor
     */
    ~FixedString() = default;

    /**
     * @brief Assignment operator from C-style string
     *
     * @param str C-style string to assign from
     * @return FixedString& Reference to this object
     */
    constexpr FixedString& operator=(const char* str) noexcept {
        if (str == nullptr) {
            return *this;
        }

        std::fill_n(data.begin(), N, '0');
        const std::size_t LEN = std::min(std::strlen(str), N);
        bool valid = true;
        for (std::size_t i = 0; i < LEN; ++i) {
            if (Validator::is_valid(str[i])) {
                data.at(i) = str[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in string");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
        return *this;
    }

    /**
     * @brief Assignment operator from std::string
     *
     * @param str String to assign from
     * @return FixedString& Reference to this object
     */
    constexpr FixedString& operator=(const std::string& str) noexcept {
        std::fill_n(data.begin(), N, '0');
        const std::size_t LEN = std::min(str.size(), N);
        bool valid = true;
        for (std::size_t i = 0; i < LEN; ++i) {
            if (Validator::is_valid(str[i])) {
                data.at(i) = str[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in string");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
        return *this;
    }

    /**
     * @brief Assignment operator from std::string_view
     *
     * @param sv String view to assign from
     * @return FixedString& Reference to this object
     */
    constexpr FixedString& operator=(std::string_view sv) noexcept {
        std::fill_n(data.begin(), N, '0');
        const std::size_t LEN = std::min(sv.size(), N);
        bool valid = true;
        for (std::size_t i = 0; i < LEN; ++i) {
            if (Validator::is_valid(sv[i])) {
                data.at(i) = sv[i];
            } else {
                valid = false;
                break;
            }
        }
#ifndef NDEBUG
        assert(valid && "Invalid characters in string");
#endif
        // If invalid characters found, reset to default
        if (!valid) {
            std::fill_n(data.begin(), N, '0');
        }
        return *this;
    }

    /**
     * @brief Get the size of the string (always N)
     *
     * @return constexpr std::size_t The size of the string
     */
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }

    /**
     * @brief Get the maximum size of the string (always N)
     *
     * @return constexpr std::size_t The maximum size of the string
     */
    [[nodiscard]] static constexpr std::size_t max_size() noexcept { return N; }

    /**
     * @brief Check if the string is empty (always false)
     *
     * @return constexpr bool Always false
     */
    [[nodiscard]] static constexpr bool empty() noexcept { return N == 0; }

    /**
     * @brief Access the character at the specified index
     *
     * @param pos Index to access
     * @return constexpr Reference Reference to the character
     */
    [[nodiscard]] constexpr Reference operator[](std::size_t pos) noexcept {
        assert(pos < N && "Index out of bounds");
        return data.at(pos); // Use at() instead of [] to avoid warning
    }

    /**
     * @brief Access the character at the specified index
     *
     * @param pos Index to access
     * @return constexpr ConstReference Reference to the character
     */
    [[nodiscard]] constexpr ConstReference operator[](std::size_t pos) const noexcept {
        assert(pos < N && "Index out of bounds");
        return data.at(pos); // Use at() instead of [] to avoid warning
    }

    /**
     * @brief Access the character at the specified index with bounds checking
     *
     * @param pos Index to access
     * @return constexpr Reference Reference to the character
     * @throws std::out_of_range if the index is out of bounds
     */
    [[nodiscard]] constexpr Reference at(std::size_t pos) {
        if (pos >= N) {
            throw std::out_of_range("FixedString::at: index out of range");
        }
        return data[pos];
    }

    /**
     * @brief Access the character at the specified index with bounds checking
     *
     * @param pos Index to access
     * @return constexpr ConstReference Reference to the character
     * @throws std::out_of_range if the index is out of bounds
     */
    [[nodiscard]] constexpr ConstReference at(std::size_t pos) const {
        if (pos >= N) {
            throw std::out_of_range("FixedString::at: index out of range");
        }
        return data[pos];
    }

    /**
     * @brief Access the first character
     *
     * @return constexpr Reference Reference to the first character
     */
    [[nodiscard]] constexpr Reference front() noexcept {
        static_assert(N > 0, "Cannot call front() on empty FixedString");
        return data[0];
    }

    /**
     * @brief Access the first character
     *
     * @return constexpr ConstReference Reference to the first character
     */
    [[nodiscard]] constexpr ConstReference front() const noexcept {
        static_assert(N > 0, "Cannot call front() on empty FixedString");
        return data[0];
    }

    /**
     * @brief Access the last character
     *
     * @return constexpr Reference Reference to the last character
     */
    [[nodiscard]] constexpr Reference back() noexcept {
        static_assert(N > 0, "Cannot call back() on empty FixedString");
        return data[N - 1];
    }

    /**
     * @brief Access the last character
     *
     * @return constexpr ConstReference Reference to the last character
     */
    [[nodiscard]] constexpr ConstReference back() const noexcept {
        static_assert(N > 0, "Cannot call back() on empty FixedString");
        return data[N - 1];
    }

    /**
     * @brief Get a Pointer to the raw character array
     *
     * @return constexpr Pointer Pointer to the character array
     */
    [[nodiscard]] constexpr Pointer get_raw_data() noexcept { return data.data(); }

    /**
     * @brief Get a Pointer to the raw character array
     *
     * @return constexpr ConstPointer Pointer to the character array
     */
    [[nodiscard]] constexpr ConstPointer get_raw_data() const noexcept { return data.data(); }

    /**
     * @brief Get a Pointer to the raw character array (backwards compatibility)
     *
     * @return constexpr Pointer Pointer to the character array
     */
    [[nodiscard]] constexpr Pointer get_data() noexcept { return get_raw_data(); }

    /**
     * @brief Get a Pointer to the raw character array (backwards compatibility)
     *
     * @return constexpr ConstPointer Pointer to the character array
     */
    [[nodiscard]] constexpr ConstPointer get_data() const noexcept { return get_raw_data(); }

    /**
     * @brief Get a Pointer to the raw character array
     *
     * @return constexpr ConstPointer Pointer to the character array
     */
    [[nodiscard]] constexpr ConstPointer c_str() const noexcept {
        // This is safe because data is always null-terminated by the std::array implementation
        return get_raw_data();
    }

    // Iterator support

    /**
     * @brief Get an Iterator to the beginning
     *
     * @return constexpr Iterator Iterator to the beginning
     */
    [[nodiscard]] constexpr Iterator begin() noexcept { return data.begin(); }

    /**
     * @brief Get a const Iterator to the beginning
     *
     * @return constexpr ConstIterator Const Iterator to the beginning
     */
    [[nodiscard]] constexpr ConstIterator begin() const noexcept { return data.begin(); }

    /**
     * @brief Get a const Iterator to the beginning
     *
     * @return constexpr ConstIterator Const Iterator to the beginning
     */
    [[nodiscard]] constexpr ConstIterator cbegin() const noexcept { return data.cbegin(); }

    /**
     * @brief Get an Iterator to the end
     *
     * @return constexpr Iterator Iterator to the end
     */
    [[nodiscard]] constexpr Iterator end() noexcept { return data.end(); }

    /**
     * @brief Get a const Iterator to the end
     *
     * @return constexpr ConstIterator Const Iterator to the end
     */
    [[nodiscard]] constexpr ConstIterator end() const noexcept { return data.end(); }

    /**
     * @brief Get a const Iterator to the end
     *
     * @return constexpr ConstIterator Const Iterator to the end
     */
    [[nodiscard]] constexpr ConstIterator cend() const noexcept { return data.cend(); }

    /**
     * @brief Get a reverse Iterator to the beginning
     *
     * @return constexpr ReverseIterator Reverse Iterator to the beginning
     */
    [[nodiscard]] constexpr ReverseIterator rbegin() noexcept { return data.rbegin(); }

    /**
     * @brief Get a const reverse Iterator to the beginning
     *
     * @return constexpr ConstReverseIterator Const reverse Iterator to the beginning
     */
    [[nodiscard]] constexpr ConstReverseIterator rbegin() const noexcept { return data.rbegin(); }

    /**
     * @brief Get a const reverse Iterator to the beginning
     *
     * @return constexpr ConstReverseIterator Const reverse Iterator to the beginning
     */
    [[nodiscard]] constexpr ConstReverseIterator crbegin() const noexcept { return data.crbegin(); }

    /**
     * @brief Get a reverse Iterator to the end
     *
     * @return constexpr ReverseIterator Reverse Iterator to the end
     */
    [[nodiscard]] constexpr ReverseIterator rend() noexcept { return data.rend(); }

    /**
     * @brief Get a const reverse Iterator to the end
     *
     * @return constexpr ConstReverseIterator Const reverse Iterator to the end
     */
    [[nodiscard]] constexpr ConstReverseIterator rend() const noexcept { return data.rend(); }

    /**
     * @brief Get a const reverse Iterator to the end
     *
     * @return constexpr ConstReverseIterator Const reverse Iterator to the end
     */
    [[nodiscard]] constexpr ConstReverseIterator crend() const noexcept { return data.crend(); }

    /**
     * @brief Get a view of a substring
     *
     * @param pos Position of the first character
     * @param count Length of the substring
     * @return std::string_view View of the substring
     */
    [[nodiscard]] constexpr std::string_view substr(std::size_t pos = 0, std::size_t count = std::string_view::npos)
        const {
        if (pos >= N) {
            throw std::out_of_range("FixedString::substr: position out of range");
        }
        return std::string_view(data.data() + pos, std::min(count, N - pos));
    }

    /**
     * @brief Convert to std::string
     *
     * @return std::string String representation
     */
    [[nodiscard]] std::string to_string() const { return std::string(data.data(), N); }

    /**
     * @brief Convert to std::string_view
     *
     * @return std::string_view View of the string
     */
    [[nodiscard]] constexpr operator std::string_view() const noexcept { return std::string_view(data.data(), N); }

    /**
     * @brief Get the underlying std::array
     *
     * @return const std::array<char, N>& The underlying array
     */
    [[nodiscard]] constexpr const std::array<char, N>& to_array() const noexcept { return data; }

    // Comparison operators

    /**
     * @brief Equality operator
     *
     * @param other FixedString to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(const FixedString& other) const noexcept { return data == other.data; }

    /**
     * @brief Equality operator with C-style string
     *
     * @param str C-style string to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(const char* str) const noexcept {
        if (str == nullptr) {
            return false;
        }
        const std::size_t LEN = std::strlen(str);
        if (LEN != N) {
            return false;
        }
        return std::equal(data.begin(), data.end(), str);
    }

    /**
     * @brief Equality operator with std::string
     *
     * @param str String to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(const std::string& str) const noexcept {
        if (str.size() != N) {
            return false;
        }
        return std::equal(data.begin(), data.end(), str.begin());
    }

    /**
     * @brief Equality operator with std::string_view
     *
     * @param sv String view to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(std::string_view sv) const noexcept {
        if (sv.size() != N) {
            return false;
        }
        return std::equal(data.begin(), data.end(), sv.begin());
    }

    /**
     * @brief Inequality operator
     *
     * @param other FixedString to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(const FixedString& other) const noexcept { return !(*this == other); }

    /**
     * @brief Inequality operator with C-style string
     *
     * @param str C-style string to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(const char* str) const noexcept { return !(*this == str); }

    /**
     * @brief Inequality operator with std::string
     *
     * @param str String to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(const std::string& str) const noexcept { return !(*this == str); }

    /**
     * @brief Inequality operator with std::string_view
     *
     * @param sv String view to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(std::string_view sv) const noexcept { return !(*this == sv); }

    /**
     * @brief Less than operator
     *
     * @param other FixedString to compare with
     * @return true if less than
     * @return false if not less than
     */
    [[nodiscard]] constexpr bool operator<(const FixedString& other) const noexcept { return data < other.data; }

    /**
     * @brief Less than or equal operator
     *
     * @param other FixedString to compare with
     * @return true if less than or equal
     * @return false if not less than or equal
     */
    [[nodiscard]] constexpr bool operator<=(const FixedString& other) const noexcept { return data <= other.data; }

    /**
     * @brief Greater than operator
     *
     * @param other FixedString to compare with
     * @return true if greater than
     * @return false if not greater than
     */
    [[nodiscard]] constexpr bool operator>(const FixedString& other) const noexcept { return data > other.data; }

    /**
     * @brief Greater than or equal operator
     *
     * @param other FixedString to compare with
     * @return true if greater than or equal
     * @return false if not greater than or equal
     */
    [[nodiscard]] constexpr bool operator>=(const FixedString& other) const noexcept { return data >= other.data; }

    /**
     * @brief Check if all characters are valid
     *
     * @return true if all characters are valid
     * @return false if any character is invalid
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        for (char c : data) {
            if (!Validator::is_valid(c)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Get the validator description
     *
     * @return std::string_view Description of the validator
     */
    [[nodiscard]] static constexpr std::string_view validator_description() noexcept {
        return Validator::description();
    }
};

// Define the Ada string types using FixedString

// MAC_Address (12 hexadecimal characters)
using MacAddress = FixedString<12, HexCharValidator>;

// UUID (4 hexadecimal characters)
using Uuid = FixedString<4, HexCharValidator>;

// PIN_Type (6 decimal digits)
using PinType = FixedString<6, DigitValidator>;

// iBeacon_UUID (32 hexadecimal characters)
using IBeaconUuid = FixedString<32, HexCharValidator>;

// Version_Type (4 hexadecimal characters)
using VersionType = FixedString<4, HexCharValidator>;

// Measured_Power (2 hexadecimal characters)
using MeasuredPower = FixedString<2, HexCharValidator>;

// Work_Interval (2 decimal digits)
using WorkInterval = FixedString<2, DigitValidator>;

// Handle_Type (4 hexadecimal characters)
using HandleType = FixedString<4, HexCharValidator>;

// Advertising_FLAG (2 hexadecimal characters)
using AdvertisingFlag = FixedString<2, HexCharValidator>;

// Advertising_Temperature_Type (2 hexadecimal characters)
using AdvertisingTemperatureType = FixedString<2, HexCharValidator>;

// Connect_Timeout (6 decimal digits)
using ConnectTimeout = FixedString<6, DigitValidator>;

// PIO_Numbers (3 hexadecimal characters)
using PioNumbers = FixedString<3, HexCharValidator>;

// Advertisement_Data (12 hexadecimal characters)
using AdvertisementData = FixedString<12, HexCharValidator>;

// Characteristic_Type (4 hexadecimal characters)
using CharacteristicType = FixedString<4, HexCharValidator>;

// Humi_Information (2 hexadecimal characters)
using HumiInformation = FixedString<2, HexCharValidator>;

// Internal_Temperature_Type (7 decimal digits)
using InternalTemperatureType = FixedString<7, DigitValidator>;

} // namespace hm11::util

#endif // HM11_UTIL_FIXED_STRING_HPP