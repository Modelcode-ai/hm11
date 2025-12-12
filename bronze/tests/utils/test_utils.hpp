/**
 * @file test_utils.hpp
 * @brief Common utilities for HM11 driver tests
 *
 * This file provides common utility functions and macros that are used
 * throughout the HM11 driver test suite.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_TESTS_UTILS_TEST_UTILS_HPP
#define HM11_TESTS_UTILS_TEST_UTILS_HPP

#include <algorithm>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "hm11/hal/types.hpp"

namespace hm11::tests::utils {

/**
 * @brief Generate a random vector of UInt8 values
 *
 * @param size The size of the vector to generate
 * @return std::vector<hal::UInt8> The generated vector
 */
inline std::vector<hal::UInt8> random_uint8_vector(std::size_t size) {
    std::vector<hal::UInt8> result(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    std::generate(result.begin(), result.end(), [&]() { return static_cast<hal::UInt8>(dist(gen)); });

    return result;
}

/**
 * @brief Generate a random vector of UInt16 values
 *
 * @param size The size of the vector to generate
 * @return std::vector<hal::UInt16> The generated vector
 */
inline std::vector<hal::UInt16> random_uint16_vector(std::size_t size) {
    std::vector<hal::UInt16> result(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 511); // 9-bit values max

    std::generate(result.begin(), result.end(), [&]() { return static_cast<hal::UInt16>(dist(gen)); });

    return result;
}

/**
 * @brief Generate a random string
 *
 * @param size The size of the string to generate
 * @return std::string The generated string
 */
inline std::string random_string(std::size_t size) {
    std::string result(size, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist('a', 'z');

    std::generate(result.begin(), result.end(), [&]() { return static_cast<char>(dist(gen)); });

    return result;
}

/**
 * @brief Generate a random hex string
 *
 * @param size The size of the string to generate
 * @return std::string The generated hex string
 */
inline std::string random_hex_string(std::size_t size) {
    std::string result(size, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    const char* hex_chars = "0123456789ABCDEF";

    std::generate(result.begin(), result.end(),
                [&]() { return hex_chars[dist(gen)]; });

    return result;
}

/**
 * @brief Convert a vector of UInt8 values to a string
 *
 * @param data The vector of UInt8 values
 * @return std::string The string representation
 */
inline std::string uint8_vector_to_string(const std::vector<hal::UInt8>& data) {
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

/**
 * @brief Convert a string to a vector of UInt8 values
 *
 * @param data The string
 * @return std::vector<hal::UInt8> The vector of UInt8 values
 */
inline std::vector<hal::UInt8> string_to_uint8_vector(const std::string& data) {
    return std::vector<hal::UInt8>(data.begin(), data.end());
}

/**
 * @brief Execute a function with a timeout
 *
 * @tparam Func Type of the function to execute
 * @param func The function to execute
 * @param timeout_ms The timeout in milliseconds
 * @return bool True if the function completed before the timeout, false otherwise
 */
template <typename Func>
bool execute_with_timeout(Func&& func, std::chrono::milliseconds timeout_ms) {
    auto future = std::async(std::launch::async, std::forward<Func>(func));
    auto status = future.wait_for(timeout_ms);
    return status == std::future_status::ready;
}

/**
 * @brief Sleep for a specified duration
 *
 * @param ms The duration in milliseconds
 */
inline void sleep_for(std::chrono::milliseconds ms) {
    std::this_thread::sleep_for(ms);
}

} // namespace hm11::tests::utils

// Custom Google Test assertions

/**
 * @brief Assert that a UartStatus is equal to an expected value
 */
#define ASSERT_UART_STATUS_EQ(expected, actual) \
    ASSERT_EQ(expected, actual) << "Expected status: " << static_cast<int>(expected) \
                                << ", Actual status: " << static_cast<int>(actual)

/**
 * @brief Expect that a UartStatus is equal to an expected value
 */
#define EXPECT_UART_STATUS_EQ(expected, actual) \
    EXPECT_EQ(expected, actual) << "Expected status: " << static_cast<int>(expected) \
                                << ", Actual status: " << static_cast<int>(actual)

#endif // HM11_TESTS_UTILS_TEST_UTILS_HPP