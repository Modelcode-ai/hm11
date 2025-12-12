/**
 * @file error.hpp
 * @brief Error handling utilities for the HM11 driver
 *
 * This file provides utilities for error handling and error propagation in the HM11 driver.
 * It implements the error handling strategy defined in Design Decision 4, using out
 * parameters via references for status reporting and propagation.
 *
 * The utilities in this file help maintain consistent error handling semantics across
 * the codebase and provide debugging support for error conditions.
 */

#ifndef HM11_UTIL_ERROR_HPP
#define HM11_UTIL_ERROR_HPP

#include <cassert>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "hm11/util/status.hpp"

namespace hm11::util {

/**
 * @brief Propagate an error status from one operation to another
 *
 * This function propagates an error status from one operation to another.
 * If the source status indicates an error, it sets the destination status
 * to the same error and returns true. If the source status is Ok, it leaves
 * the destination status unchanged and returns false.
 *
 * @tparam SourceStatusType Type of the source status
 * @tparam DestStatusType Type of the destination status
 * @param source_status Source status to check
 * @param dest_status Destination status to update if source has error
 * @return true If an error was propagated
 * @return false If no error was propagated (source status is Ok)
 */
template <typename SourceStatusType, typename DestStatusType>
constexpr inline bool propagate_error(const SourceStatusType& source_status, DestStatusType& dest_status) noexcept {
    if (is_error(source_status)) {
        // Convert between status types if necessary
        if constexpr (std::is_same_v<SourceStatusType, DestStatusType>) {
            dest_status = source_status;
        } else if constexpr (
            std::is_same_v<SourceStatusType, hal::UartStatus> && std::is_same_v<DestStatusType, Status>) {
            dest_status = to_status(source_status);
        } else if constexpr (
            std::is_same_v<SourceStatusType, Status> && std::is_same_v<DestStatusType, hal::UartStatus>) {
            dest_status = to_uart_status(source_status);
        } else {
            static_assert(
                std::is_same_v<SourceStatusType, hal::UartStatus> || std::is_same_v<SourceStatusType, Status>,
                "propagate_error only supports Status and UartStatus types");
            static_assert(
                std::is_same_v<DestStatusType, hal::UartStatus> || std::is_same_v<DestStatusType, Status>,
                "propagate_error only supports Status and UartStatus types");
        }
        return true;
    }
    return false;
}

/**
 * @brief Execute an operation only if a status indicates success
 *
 * This function executes a provided operation only if the status indicates success.
 * If the status indicates an error, it skips the operation. This is useful for
 * conditionally executing operations that depend on previous operations succeeding.
 *
 * @tparam StatusType Type of status to check
 * @tparam Func Type of function to execute
 * @param status Status to check
 * @param operation Operation to execute if status is Ok
 * @return true If the operation was executed (status was Ok)
 * @return false If the operation was skipped (status was an error)
 */
template <typename StatusType, typename Func>
constexpr inline bool execute_if_ok(const StatusType& status, Func&& operation) noexcept {
    if (is_ok(status)) {
        std::forward<Func>(operation)();
        return true;
    }
    return false;
}

/**
 * @brief Check if a condition is true and set an error status if not
 *
 * This function checks if a condition is true and sets an error status if not.
 * It is useful for parameter validation and other precondition checks.
 *
 * @tparam StatusType Type of status to set
 * @param condition Condition to check
 * @param status Status to set if condition is false
 * @param error_status Error status to set if condition is false
 * @return true If the condition is true
 * @return false If the condition is false and status was set
 */
template <typename StatusType>
constexpr inline bool check_condition(bool condition, StatusType& status, const StatusType& error_status) noexcept {
    if (!condition) {
        status = error_status;
        return false;
    }
    return true;
}

/**
 * @brief Check if a response matches an expected value
 *
 * This function checks if a response string starts with an expected prefix.
 * If not, it sets the status to an error value. This is similar to the
 * Check_Response function in the Ada implementation.
 *
 * @tparam StatusType Type of status to set
 * @param response Response string to check
 * @param expected Expected prefix for the response
 * @param status Status to update if response doesn't match
 * @param error_status Error status to set if response doesn't match
 * @return true If the response matches the expected prefix
 * @return false If the response doesn't match and status was set
 */
template <typename StatusType>
inline bool check_response(
    std::string_view response,
    std::string_view expected,
    StatusType& status,
    const StatusType& error_status) noexcept {
    // Check if the response starts with the expected prefix
    if (response.size() < expected.size() || response.substr(0, expected.size()) != expected) {
        status = error_status;
        return false;
    }
    return true;
}

/**
 * @brief Validate a parameter and set an error status if invalid
 *
 * This utility function validates a parameter against a validation function
 * and sets an error status if the validation fails.
 *
 * @tparam T Parameter type
 * @tparam StatusType Status type
 * @tparam Validator Validator function type
 * @param value Parameter value to validate
 * @param status Status to set if validation fails
 * @param validator Validation function
 * @return true If validation succeeded
 * @return false If validation failed and status was set
 */
template <typename T, typename StatusType, typename Validator>
constexpr inline bool validate_param(const T& value, StatusType& status, Validator&& validator) noexcept {
    if (!std::forward<Validator>(validator)(value)) {
        if constexpr (std::is_same_v<StatusType, Status>) {
            status = Status::Err_Invalid_Param;
        } else if constexpr (std::is_same_v<StatusType, hal::UartStatus>) {
            status = hal::UartStatus::ErrError;
        } else {
            static_assert(
                std::is_same_v<StatusType, Status> || std::is_same_v<StatusType, hal::UartStatus>,
                "validate_param only supports Status and UartStatus types");
        }
        return false;
    }
    return true;
}

/**
 * @brief Set a status to an error value if a condition is met
 *
 * This utility sets a status to an error value if a condition is met.
 * It is the inverse of check_condition.
 *
 * @tparam StatusType Type of status to set
 * @param condition Condition to check
 * @param status Status to update
 * @param error_status Error status to set if condition is true
 * @return true If the condition is true and status was set
 * @return false If the condition is false
 */
template <typename StatusType>
constexpr inline bool set_error_if(bool condition, StatusType& status, const StatusType& error_status) noexcept {
    if (condition) {
        status = error_status;
        return true;
    }
    return false;
}

/**
 * @brief Initialize a status to Ok
 *
 * This utility initializes a status variable to its Ok value.
 * It's a convenience function for initializing status out parameters.
 *
 * @tparam StatusType Type of status to initialize
 * @param status Status to initialize to Ok
 */
template <typename StatusType> constexpr inline void initialize_status(StatusType& status) noexcept {
    if constexpr (std::is_same_v<StatusType, hal::UartStatus>) {
        status = hal::UartStatus::Ok;
    } else if constexpr (std::is_same_v<StatusType, Status>) {
        status = Status::Ok;
    } else {
        static_assert(
            std::is_same_v<StatusType, hal::UartStatus> || std::is_same_v<StatusType, Status>,
            "initialize_status only supports Status and UartStatus types");
    }
}

/**
 * @brief Assert that a condition is true, for debugging
 *
 * This utility function asserts that a condition is true. It is used for
 * debugging and is only active in debug builds.
 *
 * @param condition Condition to check
 * @param message Message to display if condition is false
 */
inline void debug_assert(bool condition, const char* message) {
#if !defined(NDEBUG) && !defined(GTEST_INCLUDE_GTEST_GTEST_H_)
    // Only assert in debug builds and not in test builds
    assert(condition && message);
#else
    (void)condition; // Avoid unused parameter warning in release or test builds
    (void)message;   // Avoid unused parameter warning in release or test builds
#endif
}

/**
 * @brief Log an error status, for debugging
 *
 * This utility function logs an error status. It is used for debugging
 * and is only active in debug builds.
 *
 * @tparam StatusType Type of status to log
 * @param status Status to log
 * @param function_name Name of the function where the error occurred
 */
template <typename StatusType> inline void debug_log_error(const StatusType& status, const char* function_name) {
#if !defined(NDEBUG) && !defined(GTEST_INCLUDE_GTEST_GTEST_H_)
    // Only assert in debug builds and not in test builds
    if (is_error(status)) {
        // In a real implementation, this would log to a debug output
        // For now, we just use assert with a message
        debug_assert(
            false,
            (std::string("Error in ") + function_name + ": " + std::string(status_to_string(status))).c_str());
    }
#else
    (void)status;        // Avoid unused parameter warning in release or test builds
    (void)function_name; // Avoid unused parameter warning in release or test builds
#endif
}

} // namespace hm11::util

#endif // HM11_UTIL_ERROR_HPP