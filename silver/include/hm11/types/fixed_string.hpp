#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include "validators.hpp"

namespace hm11 {

/// @brief Template class for fixed-size strings with character validation
///
/// This template provides compile-time size checking and character validation
/// equivalent to Ada's constrained array types like:
/// `type MAC_Address is array (1 .. 12) of Hex_Character;`
///
/// @tparam N Maximum number of characters (compile-time constant)
/// @tparam Validator Character validator class (must satisfy CharacterValidator concept)
///
/// Features:
/// - Fixed maximum capacity with efficient storage
/// - Character validation on every operation
/// - Zero-overhead abstraction (constexpr/noexcept where possible)
/// - Compatible with std::string_view for zero-copy parsing
/// - Ada-equivalent behavior with C++ convenience methods
template <std::size_t N, CharacterValidator Validator = AnyCharValidator> class FixedString {
    static_assert(N > 0, "FixedString size must be greater than 0");
    static_assert(N <= 1024, "FixedString size should be reasonable (max 1024 chars)");

  public:
    // Type aliases
    using ValueType = char;
    using SizeType = std::size_t;
    using DifferenceType = std::ptrdiff_t;
    using Reference = char&;
    using ConstReference = const char&;
    using Pointer = char*;
    using ConstPointer = const char*;
    using Iterator = Pointer;
    using ConstIterator = ConstPointer;
    using ValidatorType = Validator;

    // Constants
    static constexpr SizeType MAX_SIZE = N;

    /// @brief Default constructor - creates empty string
    constexpr FixedString() noexcept : data_array{}, current_length(0) {}

    /// @brief Constructor from C string with validation
    /// @param str C string to copy (must be null-terminated)
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is too long
    explicit FixedString(const char* str) : data_array{}, current_length(0) {
        if (str == nullptr) {
            return; // Empty string
        }

        const SizeType LEN = std::strlen(str);
        if (LEN > N) {
            throw std::length_error("String too long for FixedString capacity");
        }

        for (SizeType i = 0; i < LEN; ++i) {
            if (!Validator::validate(str[i])) {
                throw std::invalid_argument("Invalid character in string for FixedString validator");
            }
            data_array[i] = str[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        current_length = LEN;
    }

    /// @brief Constructor from std::string_view with validation
    /// @param sv String view to copy from
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is too long
    explicit FixedString(std::string_view sv) : data_array{}, current_length(0) {
        if (sv.size() > N) {
            throw std::length_error("String too long for FixedString capacity");
        }

        for (SizeType i = 0; i < sv.size(); ++i) {
            if (!Validator::validate(sv[i])) {
                throw std::invalid_argument("Invalid character in string for FixedString validator");
            }
            data_array[i] = sv[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        current_length = sv.size();
    }

    /// @brief Constructor from std::array with validation
    /// @param arr Array of characters to copy
    /// @throws std::invalid_argument if any character fails validation
    template <SizeType ArraySize>
    explicit FixedString(const std::array<char, ArraySize>& arr) : data_array{}, current_length(0) {
        static_assert(ArraySize <= N, "Source array too large for FixedString");

        for (SizeType i = 0; i < ArraySize; ++i) {
            if (!Validator::validate(arr[i])) { // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                throw std::invalid_argument("Invalid character in array for FixedString validator");
            }
            data_array[i] = arr[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        current_length = ArraySize;
    }

    /// @brief Copy constructor
    constexpr FixedString(const FixedString&) = default;

    /// @brief Copy assignment operator
    constexpr FixedString& operator=(const FixedString&) = default;

    /// @brief Move constructor
    constexpr FixedString(FixedString&&) noexcept = default;

    /// @brief Move assignment operator
    constexpr FixedString& operator=(FixedString&&) noexcept = default;

    /// @brief Destructor
    ~FixedString() = default;

    /// @brief Assignment from C string
    FixedString& operator=(const char* str) {
        *this = FixedString(str);
        return *this;
    }

    /// @brief Assignment from string view
    FixedString& operator=(std::string_view sv) {
        *this = FixedString(sv);
        return *this;
    }

    // Element access

    /// @brief Access character at index with bounds checking
    /// @param pos Index of character
    /// @return Reference to character at position
    /// @throws std::out_of_range if pos >= size()
    constexpr ConstReference at(SizeType pos) const {
        if (pos >= current_length) {
            throw std::out_of_range("FixedString index out of range");
        }
        return data_array[pos];
    }

    /// @brief Access character at index with bounds checking
    constexpr Reference at(SizeType pos) {
        if (pos >= current_length) {
            throw std::out_of_range("FixedString index out of range");
        }
        return data_array[pos]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Access character at index without bounds checking
    /// @param pos Index of character
    /// @return Reference to character at position
    constexpr ConstReference operator[](SizeType pos) const noexcept {
        assert(pos < current_length && "FixedString index out of range");
        return data_array[pos]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Access character at index without bounds checking
    constexpr Reference operator[](SizeType pos) noexcept {
        assert(pos < current_length && "FixedString index out of range");
        return data_array[pos]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Access first character
    /// @return Reference to first character
    /// @throws std::out_of_range if string is empty
    constexpr ConstReference front() const {
        if (empty()) {
            throw std::out_of_range("FixedString is empty");
        }
        return data_array[0];
    }

    /// @brief Access first character
    constexpr Reference front() {
        if (empty()) {
            throw std::out_of_range("FixedString is empty");
        }
        return data_array[0];
    }

    /// @brief Access last character
    /// @return Reference to last character
    /// @throws std::out_of_range if string is empty
    constexpr ConstReference back() const {
        if (empty()) {
            throw std::out_of_range("FixedString is empty");
        }
        return data_array[current_length - 1]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Access last character
    constexpr Reference back() {
        if (empty()) {
            throw std::out_of_range("FixedString is empty");
        }
        return data_array[current_length - 1]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Get pointer to underlying data
    /// @return Pointer to character array (not null-terminated unless explicitly added)
    constexpr ConstPointer data() const noexcept { return data_array.data(); }

    /// @brief Get pointer to underlying data
    constexpr Pointer data() noexcept { return data_array.data(); }

    /// @brief Get C string representation (null-terminated)
    /// @return Null-terminated C string
    /// Note: This method ensures null-termination by temporarily modifying internal buffer
    const char* c_str() const {
        // We need to ensure null termination, but data might be full
        // This is a const method, so we use mutable or const_cast for implementation
        if (current_length < N) {
            // Safe to add null terminator - we need to avoid const_cast and use a different approach
            static thread_local std::array<char, N + 1> temp_buffer{};
            std::copy_n(data_array.data(), current_length, temp_buffer.data());
            temp_buffer[current_length] = '\0'; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            return temp_buffer.data();
        }
        // Buffer is full, need to use a static buffer for return
        static thread_local std::array<char, N + 1> temp_buffer{};
        std::copy_n(data_array.data(), N, temp_buffer.data());
        temp_buffer[N] = '\0';
        return temp_buffer.data();
    }

    // Iterators

    constexpr Iterator begin() noexcept { return data_array.data(); }
    constexpr ConstIterator begin() const noexcept { return data_array.data(); }
    constexpr ConstIterator cbegin() const noexcept { return data_array.data(); }

    constexpr Iterator end() noexcept { return data_array.data() + current_length; }
    constexpr ConstIterator end() const noexcept { return data_array.data() + current_length; }
    constexpr ConstIterator cend() const noexcept { return data_array.data() + current_length; }

    // Capacity

    /// @brief Check if string is empty
    constexpr bool empty() const noexcept { return current_length == 0; }

    /// @brief Get current string length
    constexpr SizeType size() const noexcept { return current_length; }

    /// @brief Get current string length (alias for size())
    constexpr SizeType length() const noexcept { return current_length; }

    /// @brief Get maximum possible size
    static constexpr SizeType capacity() noexcept { return N; }

    /// @brief Get remaining capacity
    constexpr SizeType remaining_capacity() const noexcept { return N - current_length; }

    /// @brief Check if string is at maximum capacity
    constexpr bool full() const noexcept { return current_length == N; }

    // Modifiers

    /// @brief Clear the string (set length to 0)
    constexpr void clear() noexcept {
        current_length = 0;
        // Optional: clear data for security (comment out for performance)
        // data_array.fill('\0');
    }

    /// @brief Append a character with validation
    /// @param c Character to append
    /// @throws std::invalid_argument if character fails validation
    /// @throws std::length_error if string is already at capacity
    void push_back(char c) {
        if (current_length >= N) {
            throw std::length_error("FixedString at capacity, cannot append");
        }
        if (!Validator::validate(c)) {
            throw std::invalid_argument("Character fails FixedString validation");
        }
        data_array[current_length++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// @brief Remove last character
    /// @throws std::out_of_range if string is empty
    void pop_back() {
        if (empty()) {
            throw std::out_of_range("Cannot pop from empty FixedString");
        }
        --current_length;
    }

    /// @brief Append another FixedString
    /// @param other FixedString to append
    /// @throws std::length_error if combined length exceeds capacity
    template <SizeType OtherN, CharacterValidator OtherValidator>
    void append(const FixedString<OtherN, OtherValidator>& other) {
        if (current_length + other.size() > N) {
            throw std::length_error("Cannot append: would exceed FixedString capacity");
        }

        // If validators are different, we need to validate each character
        if constexpr (!std::is_same_v<Validator, OtherValidator>) {
            for (SizeType i = 0; i < other.size(); ++i) {
                if (!Validator::validate(other[i])) {
                    throw std::invalid_argument("Character from other FixedString fails validation");
                }
                data_array[current_length + i] = other[i];
            }
        } else {
            // Same validator, can copy directly
            std::copy_n(other.data(), other.size(), data_array.data() + current_length);
        }
        current_length += other.size();
    }

    /// @brief Append string view with validation
    /// @param sv String view to append
    /// @throws std::length_error if combined length exceeds capacity
    /// @throws std::invalid_argument if any character fails validation
    void append(std::string_view sv) {
        if (current_length + sv.size() > N) {
            throw std::length_error("Cannot append: would exceed FixedString capacity");
        }

        for (SizeType i = 0; i < sv.size(); ++i) {
            if (!Validator::validate(sv[i])) {
                throw std::invalid_argument("Character fails FixedString validation");
            }
            data_array[current_length + i] = sv[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        current_length += sv.size();
    }

    /// @brief Resize string to new length
    /// @param new_size New size for the string
    /// @param fill_char Character to use for padding if expanding
    /// @throws std::length_error if new_size > capacity
    /// @throws std::invalid_argument if fill_char fails validation
    void resize(SizeType new_size, char fill_char = '\0') {
        if (new_size > N) {
            throw std::length_error("Cannot resize beyond FixedString capacity");
        }

        if (new_size > current_length) {
            // Expanding - validate fill character
            if (!Validator::validate(fill_char)) {
                throw std::invalid_argument("Fill character fails FixedString validation");
            }
            // Fill new space with fill_char
            std::fill_n(data_array.data() + current_length, new_size - current_length, fill_char);
        }
        current_length = new_size;
    }

    // String operations

    /// @brief Get substring as string_view
    /// @param pos Starting position
    /// @param len Length of substring (default: to end of string)
    /// @return String view of the substring
    constexpr std::string_view substr(SizeType pos = 0, SizeType len = std::string_view::npos) const {
        if (pos > current_length) {
            throw std::out_of_range("Substring starting position out of range");
        }
        const SizeType ACTUAL_LEN = std::min(len, current_length - pos);
        return std::string_view(data_array.data() + pos, ACTUAL_LEN);
    }

    /// @brief Convert to string_view for zero-copy operations
    constexpr operator std::string_view() const noexcept { return std::string_view(data_array.data(), current_length); }

    /// @brief Get string_view representation (explicit method)
    /// @return String view of current content
    constexpr std::string_view view() const noexcept { return std::string_view(data_array.data(), current_length); }

    /// @brief Find first occurrence of character
    /// @param c Character to find
    /// @param pos Starting position for search
    /// @return Position of first occurrence or std::string_view::npos
    constexpr SizeType find(char c, SizeType pos = 0) const noexcept {
        if (pos >= current_length) {
            return std::string_view::npos;
        }

        for (SizeType i = pos; i < current_length; ++i) {
            if (data_array[i] == c) { // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                return i;
            }
        }
        return std::string_view::npos;
    }

    /// @brief Check if string starts with given prefix
    /// @param prefix Prefix to check
    /// @return true if string starts with prefix
    constexpr bool starts_with(std::string_view prefix) const noexcept {
        if (prefix.size() > current_length) {
            return false;
        }
        return std::string_view(*this).starts_with(prefix);
    }

    /// @brief Check if string ends with given suffix
    /// @param suffix Suffix to check
    /// @return true if string ends with suffix
    constexpr bool ends_with(std::string_view suffix) const noexcept {
        if (suffix.size() > current_length) {
            return false;
        }
        return std::string_view(*this).substr(current_length - suffix.size()) == suffix;
    }

    // Validation utilities

    /// @brief Check if all characters in string are valid for this validator
    /// @return true if all characters pass validation
    constexpr bool is_valid() const noexcept {
        for (SizeType i = 0; i < current_length; ++i) {
            if (!Validator::validate(data_array[i])) { // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                return false;
            }
        }
        return true;
    }

    /// @brief Get validator name for debugging
    static constexpr const char* validator_name() noexcept { return Validator::name(); }

    // Comparison operators

    constexpr bool operator==(const FixedString& other) const noexcept {
        return std::string_view(*this) == std::string_view(other);
    }

    constexpr bool operator!=(const FixedString& other) const noexcept { return !(*this == other); }

    constexpr bool operator<(const FixedString& other) const noexcept {
        return std::string_view(*this) < std::string_view(other);
    }

    constexpr bool operator<=(const FixedString& other) const noexcept {
        return std::string_view(*this) <= std::string_view(other);
    }

    constexpr bool operator>(const FixedString& other) const noexcept {
        return std::string_view(*this) > std::string_view(other);
    }

    constexpr bool operator>=(const FixedString& other) const noexcept {
        return std::string_view(*this) >= std::string_view(other);
    }

    // Comparison with string_view

    constexpr bool operator==(std::string_view sv) const noexcept { return std::string_view(*this) == sv; }

    constexpr bool operator!=(std::string_view sv) const noexcept { return std::string_view(*this) != sv; }

    constexpr bool operator<(std::string_view sv) const noexcept { return std::string_view(*this) < sv; }

    constexpr bool operator<=(std::string_view sv) const noexcept { return std::string_view(*this) <= sv; }

    constexpr bool operator>(std::string_view sv) const noexcept { return std::string_view(*this) > sv; }

    constexpr bool operator>=(std::string_view sv) const noexcept { return std::string_view(*this) >= sv; }

  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // Protected access needed for ExactFixedString derived class
    std::array<char, N + 1> data_array; // +1 for potential null terminator in c_str()
    SizeType current_length;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

// Free function operators for symmetry

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator==(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs == std::string_view(rhs);
}

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator!=(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs != std::string_view(rhs);
}

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator<(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs < std::string_view(rhs);
}

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator<=(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs <= std::string_view(rhs);
}

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator>(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs > std::string_view(rhs);
}

template <std::size_t N, CharacterValidator Validator>
constexpr bool operator>=(std::string_view lhs, const FixedString<N, Validator>& rhs) noexcept {
    return lhs >= std::string_view(rhs);
}

// Concatenation operators

template <std::size_t N1, std::size_t N2, CharacterValidator V1, CharacterValidator V2>
auto operator+(const FixedString<N1, V1>& lhs, const FixedString<N2, V2>& rhs) {
    // Result uses the more restrictive validator if they're different
    using ResultValidator = std::conditional_t<std::is_same_v<V1, V2>, V1, AnyCharValidator>;
    constexpr std::size_t RESULT_SIZE = N1 + N2;

    FixedString<RESULT_SIZE, ResultValidator> result(lhs);
    result.append(rhs);
    return result;
}

/// @brief Template class for exact-length strings with character validation
///
/// Unlike FixedString, this template enforces that the input string must be
/// exactly N characters long. This is useful for types like MAC addresses,
/// UUIDs, and other fixed-format identifiers that must have a specific length.
///
/// @tparam N Required number of characters (compile-time constant)
/// @tparam Validator Character validator class (must satisfy CharacterValidator concept)
///
/// Features:
/// - Enforces exact length requirement (throws std::length_error if not exactly N chars)
/// - Character validation on every operation
/// - Inherits all functionality from FixedString
/// - Ada-equivalent behavior for exact-length array types
template <std::size_t N, CharacterValidator Validator = AnyCharValidator>
class ExactFixedString : public FixedString<N, Validator> {
  public:
    using BaseType = FixedString<N, Validator>;
    using SizeType = typename BaseType::SizeType;

    /// @brief Default constructor - creates empty string (invalid for ExactFixedString)
    /// This will be invalid until a valid exact-length string is assigned
    constexpr ExactFixedString() noexcept : BaseType() {}

    /// @brief Constructor from C string with exact length validation
    /// @param str C string to copy (must be null-terminated and exactly N characters)
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is not exactly N characters
    explicit ExactFixedString(const char* str) : BaseType() {
        if (str == nullptr) {
            throw std::length_error("String cannot be null for ExactFixedString");
        }
        const SizeType len = std::strlen(str);
        if (len != N) {
            throw std::length_error("String must be exactly " + std::to_string(N) + " characters long");
        }

        // Use the base class constructor which already validates characters
        *this = ExactFixedString(std::string_view(str, len));
    }

    /// @brief Constructor from std::string_view with exact length validation
    /// @param sv String view to copy from (must be exactly N characters)
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is not exactly N characters
    explicit ExactFixedString(std::string_view sv) : BaseType() {
        if (sv.size() != N) {
            throw std::length_error("String must be exactly " + std::to_string(N) + " characters long");
        }

        // Manually validate and copy since base constructor would accept shorter strings
        for (SizeType i = 0; i < sv.size(); ++i) {
            if (!Validator::validate(sv[i])) {
                throw std::invalid_argument("Invalid character in string for ExactFixedString validator");
            }
            this->data_array[i] = sv[i];
        }
        this->current_length = N;
    }

    /// @brief Constructor from std::array with exact length validation
    /// @param arr Array of characters to copy (ArraySize must equal N)
    /// @throws std::invalid_argument if any character fails validation
    template <SizeType ArraySize> explicit ExactFixedString(const std::array<char, ArraySize>& arr) : BaseType() {
        static_assert(ArraySize == N, "Array size must exactly match ExactFixedString size");

        for (SizeType i = 0; i < ArraySize; ++i) {
            if (!Validator::validate(arr[i])) {
                throw std::invalid_argument("Invalid character in array for ExactFixedString validator");
            }
            this->data_array[i] = arr[i];
        }
        this->current_length = N;
    }

    /// @brief Copy constructor
    constexpr ExactFixedString(const ExactFixedString&) = default;

    /// @brief Move constructor
    constexpr ExactFixedString(ExactFixedString&&) noexcept = default;

    /// @brief Destructor
    ~ExactFixedString() = default;

    /// @brief Copy assignment with exact length validation
    /// @param str C string to assign (must be exactly N characters)
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is not exactly N characters
    ExactFixedString& operator=(const char* str) {
        if (str == nullptr) {
            throw std::length_error("String cannot be null for ExactFixedString");
        }
        const SizeType len = std::strlen(str);
        if (len != N) {
            throw std::length_error("String must be exactly " + std::to_string(N) + " characters long");
        }

        *this = ExactFixedString(std::string_view(str, len));
        return *this;
    }

    /// @brief Copy assignment with exact length validation
    /// @param sv String view to assign (must be exactly N characters)
    /// @throws std::invalid_argument if any character fails validation
    /// @throws std::length_error if string is not exactly N characters
    ExactFixedString& operator=(std::string_view sv) {
        if (sv.size() != N) {
            throw std::length_error("String must be exactly " + std::to_string(N) + " characters long");
        }

        // Clear and rebuild to ensure exact length
        this->current_length = 0;
        for (SizeType i = 0; i < sv.size(); ++i) {
            if (!Validator::validate(sv[i])) {
                throw std::invalid_argument("Invalid character in string for ExactFixedString validator");
            }
            this->data_array[i] = sv[i];
        }
        this->current_length = N;
        return *this;
    }

    /// @brief Copy assignment
    constexpr ExactFixedString& operator=(const ExactFixedString&) = default;

    /// @brief Move assignment
    constexpr ExactFixedString& operator=(ExactFixedString&&) noexcept = default;

    /// @brief Check if this ExactFixedString is valid (has exactly N characters)
    /// @return True if the string has exactly N characters
    constexpr bool is_valid() const noexcept { return this->size() == N; }

    /// @brief Disable operations that would change the length
    /// These are deleted to maintain the exact length invariant
    void push_back(char) = delete;
    void pop_back() = delete;
    void resize(SizeType) = delete;
    void clear() = delete;
    bool append(std::string_view) = delete;
    bool append(const BaseType&) = delete;
    template <std::size_t M, CharacterValidator V> bool append(const FixedString<M, V>&) = delete;
};

} // namespace hm11