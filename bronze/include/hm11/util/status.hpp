/**
 * @file status.hpp
 * @brief Status types and utilities for the HM11 driver
 *
 * This file defines status types and utilities for error handling and status reporting
 * in the HM11 driver. It provides a consistent approach to status handling across
 * the driver implementation, matching the Ada pattern of using out parameters for
 * status reporting.
 *
 * Implements Design Decision 4 (Error Handling) from the modernization specifications,
 * using out parameters via references for status reporting.
 */

#ifndef HM11_UTIL_STATUS_HPP
#define HM11_UTIL_STATUS_HPP

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "hm11/hal/uart.hpp"

namespace hm11::util {

/**
 * @brief General status codes for HM11 operations
 *
 * These status codes indicate the result of HM11 operations beyond just UART
 * communication. This provides a more comprehensive set of status codes for
 * the driver while maintaining compatibility with UART_Status.
 */
enum class Status {
    Ok,                ///< Operation completed successfully
    Err_Error,         ///< Generic error occurred
    Err_Timeout,       ///< Operation timed out
    Err_Busy,          ///< Device is busy
    Err_Invalid_Param, ///< Invalid parameter provided
    Err_Invalid_State, ///< Operation invalid in current state
    Err_Not_Connected, ///< Not connected to a device
    Err_Not_Supported, ///< Operation not supported by the device
    Err_Parse_Error,   ///< Error parsing device response
    Err_Buffer_Full    ///< Buffer is full
};

/**
 * @brief Check if a status indicates success
 *
 * @tparam StatusType Type of status to check
 * @param status Status to check
 * @return true If the status indicates success (Ok)
 * @return false If the status indicates an error
 */
template <typename StatusType> constexpr inline bool is_ok(const StatusType& status) noexcept {
    if constexpr (std::is_same_v<StatusType, hal::UartStatus>) {
        return status == hal::UartStatus::Ok;
    } else if constexpr (std::is_same_v<StatusType, Status>) {
        return status == Status::Ok;
    } else {
        static_assert(
            std::is_same_v<StatusType, hal::UartStatus> || std::is_same_v<StatusType, Status>,
            "is_ok only supports Status and UartStatus types");
        return false;
    }
}

/**
 * @brief Check if a status indicates an error
 *
 * @tparam StatusType Type of status to check
 * @param status Status to check
 * @return true If the status indicates an error
 * @return false If the status indicates success
 */
template <typename StatusType> constexpr inline bool is_error(const StatusType& status) noexcept {
    return !is_ok(status);
}

/**
 * @brief Check if a status indicates a timeout
 *
 * @tparam StatusType Type of status to check
 * @param status Status to check
 * @return true If the status indicates a timeout
 * @return false Otherwise
 */
template <typename StatusType> constexpr inline bool is_timeout(const StatusType& status) noexcept {
    if constexpr (std::is_same_v<StatusType, hal::UartStatus>) {
        return status == hal::UartStatus::ErrTimeout;
    } else if constexpr (std::is_same_v<StatusType, Status>) {
        return status == Status::Err_Timeout;
    } else {
        static_assert(
            std::is_same_v<StatusType, hal::UartStatus> || std::is_same_v<StatusType, Status>,
            "is_timeout only supports Status and UartStatus types");
        return false;
    }
}

/**
 * @brief Check if a status indicates the device is busy
 *
 * @tparam StatusType Type of status to check
 * @param status Status to check
 * @return true If the status indicates the device is busy
 * @return false Otherwise
 */
template <typename StatusType> constexpr inline bool is_busy(const StatusType& status) noexcept {
    if constexpr (std::is_same_v<StatusType, hal::UartStatus>) {
        return status == hal::UartStatus::Busy;
    } else if constexpr (std::is_same_v<StatusType, Status>) {
        return status == Status::Err_Busy;
    } else {
        static_assert(
            std::is_same_v<StatusType, hal::UartStatus> || std::is_same_v<StatusType, Status>,
            "is_busy only supports Status and UartStatus types");
        return false;
    }
}

/**
 * @brief Convert a UART_Status to a general Status
 *
 * @param uart_status UART status to convert
 * @return Status Equivalent general status
 */
constexpr inline Status to_status(hal::UartStatus uart_status) noexcept {
    switch (uart_status) {
        case hal::UartStatus::Ok:
            return Status::Ok;
        case hal::UartStatus::ErrError:
            return Status::Err_Error;
        case hal::UartStatus::ErrTimeout:
            return Status::Err_Timeout;
        case hal::UartStatus::Busy:
            return Status::Err_Busy;
        default:
            return Status::Err_Error; // Fallback for unexpected values
    }
}

/**
 * @brief Convert a general Status to a UART_Status
 *
 * @param status General status to convert
 * @return hal::UART_Status Nearest equivalent UART status
 */
constexpr inline hal::UartStatus to_uart_status(Status status) noexcept {
    switch (status) {
        case Status::Ok:
            return hal::UartStatus::Ok;
        case Status::Err_Timeout:
            return hal::UartStatus::ErrTimeout;
        case Status::Err_Busy:
            return hal::UartStatus::Busy;
        case Status::Err_Error:
        case Status::Err_Invalid_Param:
        case Status::Err_Invalid_State:
        case Status::Err_Not_Connected:
        case Status::Err_Not_Supported:
        case Status::Err_Parse_Error:
        case Status::Err_Buffer_Full:
        default:
            return hal::UartStatus::ErrError;
    }
}

/**
 * @brief Get a string representation of a status
 *
 * This function returns a human-readable string for each status value,
 * useful for debugging and logging.
 *
 * @param status The status to convert to string
 * @return std::string_view String representation of the status
 */
constexpr inline std::string_view status_to_string(Status status) noexcept {
    switch (status) {
        case Status::Ok:
            return "Ok";
        case Status::Err_Error:
            return "Error";
        case Status::Err_Timeout:
            return "Timeout";
        case Status::Err_Busy:
            return "Busy";
        case Status::Err_Invalid_Param:
            return "Invalid Parameter";
        case Status::Err_Invalid_State:
            return "Invalid State";
        case Status::Err_Not_Connected:
            return "Not Connected";
        case Status::Err_Not_Supported:
            return "Not Supported";
        case Status::Err_Parse_Error:
            return "Parse Error";
        case Status::Err_Buffer_Full:
            return "Buffer Full";
        default:
            return "Unknown Status";
    }
}

/**
 * @brief Get a string representation of a UART status
 *
 * This function returns a human-readable string for each UART status value,
 * useful for debugging and logging.
 *
 * @param status The UART status to convert to string
 * @return std::string_view String representation of the status
 */
constexpr inline std::string_view status_to_string(hal::UartStatus status) noexcept {
    switch (status) {
        case hal::UartStatus::Ok:
            return "Ok";
        case hal::UartStatus::ErrError:
            return "Error";
        case hal::UartStatus::ErrTimeout:
            return "Timeout";
        case hal::UartStatus::Busy:
            return "Busy";
        default:
            return "Unknown Status";
    }
}

} // namespace hm11::util

#endif // HM11_UTIL_STATUS_HPP