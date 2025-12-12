/**
 * @file variable_string.hpp
 * @brief Variable-length string with fixed capacity
 * @details Provides a C++ equivalent of Ada's Variable_String type for string buffer management
 *
 * This file is part of the HM11 driver C++ migration project.
 * Implements Decision 27 regarding discovery data lifetime and storage.
 */

#ifndef HM11_UTIL_VARIABLE_STRING_HPP
#define HM11_UTIL_VARIABLE_STRING_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <string>
#include <string_view>

namespace hm11::util {

/**
 * @brief Variable-length string with fixed capacity
 *
 * This class implements a string with fixed capacity and tracked length,
 * similar to Ada's Variable_String type. It provides a stack-allocated
 * string buffer with bounds checking and length tracking.
 *
 * Following Decision 27, this is used for temporary discovery data storage
 * during parsing of streaming data, matching Ada's approach exactly.
 *
 * @tparam Capacity Maximum number of characters that can be stored
 */
template <std::size_t Capacity> class VariableString {
  public:
    /**
     * @brief Construct an empty VariableString
     */
    VariableString() = default;

    /**
     * @brief Construct a VariableString from a C-string
     *
     * @param str Null-terminated C-string
     * @throws std::length_error if str is longer than Capacity
     */
    explicit VariableString(const char* str) {
        std::size_t len = std::strlen(str);
        if (len > Capacity) {
            throw std::length_error("String exceeds capacity");
        }
        std::copy_n(str, len, value_.begin());
        length_ = len;
    }

    /**
     * @brief Construct a VariableString from a std::string
     *
     * @param str String to copy
     * @throws std::length_error if str is longer than Capacity
     */
    explicit VariableString(const std::string& str) {
        if (str.length() > Capacity) {
            throw std::length_error("String exceeds capacity");
        }
        std::copy(str.begin(), str.end(), value_.begin());
        length_ = str.length();
    }

    /**
     * @brief Construct a VariableString from a string_view
     *
     * @param sv String view to copy
     * @throws std::length_error if sv is longer than Capacity
     */
    explicit VariableString(std::string_view sv) {
        if (sv.length() > Capacity) {
            throw std::length_error("String exceeds capacity");
        }
        std::copy(sv.begin(), sv.end(), value_.begin());
        length_ = sv.length();
    }

    /**
     * @brief Reset the string to empty
     */
    void clear() noexcept { length_ = 0; }

    /**
     * @brief Get the current length of the string
     *
     * @return Current number of characters in the string
     */
    [[nodiscard]] std::size_t length() const noexcept { return length_; }

    /**
     * @brief Check if the string is empty
     *
     * @return true if the string is empty, false otherwise
     */
    [[nodiscard]] bool empty() const noexcept { return length_ == 0; }

    /**
     * @brief Get the maximum capacity of the string
     *
     * @return Maximum number of characters that can be stored
     */
    [[nodiscard]] constexpr std::size_t capacity() const noexcept { return Capacity; }

    /**
     * @brief Get the remaining space in the string
     *
     * @return Number of additional characters that can be appended
     */
    [[nodiscard]] std::size_t remaining() const noexcept { return Capacity - length_; }

    /**
     * @brief Get a pointer to the underlying character array
     *
     * @return Pointer to the character array (not null-terminated)
     */
    [[nodiscard]] const char* data() const noexcept { return value_.data(); }

    /**
     * @brief Get a pointer to the underlying character array
     *
     * @return Pointer to the character array (not null-terminated)
     */
    char* data() noexcept { return value_.data(); }

    /**
     * @brief Get a string_view of the current content
     *
     * @return std::string_view representing the current content
     */
    [[nodiscard]] std::string_view view() const noexcept { return std::string_view(value_.data(), length_); }

    /**
     * @brief Convert to std::string
     *
     * @return std::string copy of the current content
     */
    [[nodiscard]] std::string str() const { return std::string(value_.data(), length_); }

    /**
     * @brief Append a single character
     *
     * @param ch Character to append
     * @return Reference to this object for method chaining
     * @throws std::length_error if appending would exceed capacity
     */
    VariableString& append(char ch) {
        if (length_ >= Capacity) {
            throw std::length_error("String capacity exceeded");
        }
        value_[length_++] = ch;
        return *this;
    }

    /**
     * @brief Append a C-string
     *
     * @param str Null-terminated C-string to append
     * @return Reference to this object for method chaining
     * @throws std::length_error if appending would exceed capacity
     */
    VariableString& append(const char* str) {
        std::size_t len = std::strlen(str);
        if (length_ + len > Capacity) {
            throw std::length_error("String capacity exceeded");
        }
        std::copy_n(str, len, value_.begin() + length_);
        length_ += len;
        return *this;
    }

    /**
     * @brief Append a std::string
     *
     * @param str String to append
     * @return Reference to this object for method chaining
     * @throws std::length_error if appending would exceed capacity
     */
    VariableString& append(const std::string& str) {
        if (length_ + str.length() > Capacity) {
            throw std::length_error("String capacity exceeded");
        }
        std::copy(str.begin(), str.end(), value_.begin() + length_);
        length_ += str.length();
        return *this;
    }

    /**
     * @brief Append a string_view
     *
     * @param sv String view to append
     * @return Reference to this object for method chaining
     * @throws std::length_error if appending would exceed capacity
     */
    VariableString& append(std::string_view sv) {
        if (length_ + sv.length() > Capacity) {
            throw std::length_error("String capacity exceeded");
        }
        std::copy(sv.begin(), sv.end(), value_.begin() + length_);
        length_ += sv.length();
        return *this;
    }

    /**
     * @brief Append a range of characters from a string_view
     *
     * @param from Starting position (inclusive)
     * @param to Ending position (inclusive)
     * @param sv String view containing the range
     * @return Reference to this object for method chaining
     * @throws std::length_error if appending would exceed capacity
     * @throws std::out_of_range if the range is invalid
     */
    VariableString& append(std::size_t from, std::size_t to, std::string_view sv) {
        if (from > to || to >= sv.length()) {
            throw std::out_of_range("Invalid range");
        }

        std::size_t len = to - from + 1;
        if (length_ + len > Capacity) {
            throw std::length_error("String capacity exceeded");
        }

        std::copy(sv.begin() + from, sv.begin() + to + 1, value_.begin() + length_);
        length_ += len;
        return *this;
    }

    /**
     * @brief Overwrite with a C-string
     *
     * @param str Null-terminated C-string
     * @return Reference to this object for method chaining
     * @throws std::length_error if str is longer than Capacity
     */
    VariableString& assign(const char* str) {
        clear();
        return append(str);
    }

    /**
     * @brief Overwrite with a std::string
     *
     * @param str String to copy
     * @return Reference to this object for method chaining
     * @throws std::length_error if str is longer than Capacity
     */
    VariableString& assign(const std::string& str) {
        clear();
        return append(str);
    }

    /**
     * @brief Overwrite with a string_view
     *
     * @param sv String view to copy
     * @return Reference to this object for method chaining
     * @throws std::length_error if sv is longer than Capacity
     */
    VariableString& assign(std::string_view sv) {
        clear();
        return append(sv);
    }

    /**
     * @brief Get a character at a specific position
     *
     * @param pos Position (0-based)
     * @return Character at the position
     * @throws std::out_of_range if pos is out of range
     */
    char operator[](std::size_t pos) const {
        if (pos >= length_) {
            throw std::out_of_range("Index out of range");
        }
        return value_[pos];
    }

    /**
     * @brief Get a reference to a character at a specific position
     *
     * @param pos Position (0-based)
     * @return Reference to the character at the position
     * @throws std::out_of_range if pos is out of range
     */
    char& operator[](std::size_t pos) {
        if (pos >= length_) {
            throw std::out_of_range("Index out of range");
        }
        return value_[pos];
    }

    /**
     * @brief Compare with another VariableString
     *
     * @tparam OtherCapacity Capacity of the other VariableString
     * @param other VariableString to compare with
     * @return true if the content is the same, false otherwise
     */
    template <std::size_t OtherCapacity> bool operator==(const VariableString<OtherCapacity>& other) const noexcept {
        if (length_ != other.length()) {
            return false;
        }
        return std::equal(value_.begin(), value_.begin() + length_, other.data(), other.data() + other.length());
    }

    /**
     * @brief Compare with a C-string
     *
     * @param str Null-terminated C-string to compare with
     * @return true if the content is the same, false otherwise
     */
    bool operator==(const char* str) const noexcept {
        std::size_t str_len = std::strlen(str);
        if (length_ != str_len) {
            return false;
        }
        return std::equal(value_.begin(), value_.begin() + length_, str, str + str_len);
    }

    /**
     * @brief Compare with a std::string
     *
     * @param str String to compare with
     * @return true if the content is the same, false otherwise
     */
    bool operator==(const std::string& str) const noexcept {
        if (length_ != str.length()) {
            return false;
        }
        return std::equal(value_.begin(), value_.begin() + length_, str.begin(), str.end());
    }

    /**
     * @brief Compare with a string_view
     *
     * @param sv String view to compare with
     * @return true if the content is the same, false otherwise
     */
    bool operator==(std::string_view sv) const noexcept {
        if (length_ != sv.length()) {
            return false;
        }
        return std::equal(value_.begin(), value_.begin() + length_, sv.begin(), sv.end());
    }

  private:
    std::array<char, Capacity> value_{};
    std::size_t length_{0};
};

/**
 * @brief Output operator for VariableString
 *
 * @tparam Capacity Capacity of the VariableString
 * @param os Output stream
 * @param vs VariableString to output
 * @return Reference to the output stream
 */
template <std::size_t Capacity> std::ostream& operator<<(std::ostream& os, const VariableString<Capacity>& vs) {
    os.write(vs.data(), static_cast<std::streamsize>(vs.length()));
    return os;
}

} // namespace hm11::util

#endif // HM11_UTIL_VARIABLE_STRING_HPP