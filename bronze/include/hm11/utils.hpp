#ifndef HM11_UTILS_HPP
#define HM11_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace hm11::utils {

/**
 * @brief Convert a numeric value to a string
 *
 * This function converts a numeric value to a string, similar to
 * Ada's Image attribute.
 *
 * @param value The value to convert
 * @return The string representation of the value
 */
template <typename T> std::string to_string(T value) {
    return std::to_string(value);
}

/**
 * @brief Convert a boolean value to a string
 *
 * This function converts a boolean value to a string, returning "1" for
 * true and "0" for false, matching the Ada Image function for booleans.
 *
 * @param value The boolean value to convert
 * @return "1" for true, "0" for false
 */
inline std::string to_string(bool value) {
    return value ? "1" : "0";
}

/**
 * @brief Find a substring within a buffer
 *
 * This function searches for a substring within a buffer of bytes, similar to
 * Ada's Find function.
 *
 * @param buffer The buffer to search in
 * @param size The size of the buffer
 * @param value The string to search for
 * @param from The starting index (0-based)
 * @return The index where the substring was found, or std::nullopt if not found
 */
std::optional<std::size_t>
find(const std::uint8_t* buffer, std::size_t size, std::string_view value, std::size_t from = 0);

/**
 * @brief Check if a buffer starts with a specific string
 *
 * This function checks if a buffer starts with a specific string at the given
 * position, similar to Ada's Start_With function.
 *
 * @param buffer The buffer to check
 * @param size The size of the buffer
 * @param value The string to check for
 * @param from The starting index (0-based)
 * @return true if the buffer starts with the string at the given position
 */
bool starts_with(const std::uint8_t* buffer, std::size_t size, std::string_view value, std::size_t from = 0);

/**
 * @brief Find the first zero byte in a buffer
 *
 * This function searches for the first zero byte in a buffer, similar to
 * Ada's Find_Zero function.
 *
 * @param buffer The buffer to search in
 * @param size The size of the buffer
 * @param from The starting index (0-based)
 * @return The index of the first zero byte, or std::nullopt if not found
 */
std::optional<std::size_t> find_zero(const std::uint8_t* buffer, std::size_t size, std::size_t from = 0);

/**
 * @brief Convert a byte to a numeric value
 *
 * This function converts a byte to a numeric value, similar to Ada's Value function.
 *
 * @param byte The byte to convert
 * @return The numeric value
 */
inline int value(std::uint8_t byte) {
    return static_cast<int>(byte) - '0';
}

} // namespace hm11::utils

#endif // HM11_UTILS_HPP