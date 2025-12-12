/**
 * @file error_handling.hpp
 * @brief Error handling utilities for HM-11 driver examples
 *
 * This header provides consistent error handling patterns used across all
 * HM-11 driver examples. It matches Ada's Check_Status pattern from
 * hm11_example.adb.
 *
 * ## Usage Pattern
 *
 * Examples use CHECK_STATUS to validate each driver operation:
 * ```cpp
 * #include "examples/common/error_handling.hpp"
 * #include "examples/common/led_control.hpp"
 *
 * UartStatus status;
 * driver.test(status);
 * CHECK_STATUS(status);  // Enters error state if status != Ok
 * ```
 *
 * ## Design Principles
 *
 * - **Visual Feedback**: Uses LED patterns to indicate errors (red LED blinking)
 * - **Terminal Error Handler**: CHECK_STATUS never returns on error
 * - **Zero Overhead**: Inline functions and macros, optimized away in release builds
 * - **Ada Pattern Match**: Directly mirrors Ada's Check_Status procedure
 *
 * ## Ada Source Reference
 *
 * This header implements the C++ equivalent of Ada's error handling:
 * - Ada's `Check_Status` procedure (hm11_example.adb:88-98)
 * - Ada's `On_Error` procedure (hm11_example.adb:76-85)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#ifndef HM11_EXAMPLES_COMMON_ERROR_HANDLING_HPP
#define HM11_EXAMPLES_COMMON_ERROR_HANDLING_HPP

#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"

namespace hm11::examples {

/**
 * @brief Check UART operation status and enter error state if failed
 *
 * Provides consistent error handling pattern matching Ada's Check_Status.
 * If status is Ok, returns normally. Otherwise, enters infinite error loop
 * with red LED blinking.
 *
 * @param status Operation status to check
 *
 * Ada source: hm11_example.adb:88-98
 */
[[noreturn]] inline void on_error() noexcept {
    // Enter infinite error loop with red LED blinking
    // In Ada: STM32.Board.Toggle (Red_LED) in loop
    indicate_error();
}

/**
 * @brief Check status and call error handler if not Ok
 *
 * @param status UART operation status to validate
 */
inline void check_status(hal::UartStatus status) noexcept {
    if (status != hal::UartStatus::Ok) {
        on_error();
    }
}

} // namespace hm11::examples

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * @brief Check UART status and enter error state if failed
 *
 * Provides a terse way to check operation status at call sites.
 * Equivalent to calling hm11::examples::check_status(status).
 *
 * @param status Expression that evaluates to UartStatus
 *
 * Example:
 * ```cpp
 * UartStatus status;
 * driver.test(status);
 * CHECK_STATUS(status);  // Enters error loop if status != Ok
 * ```
 */
#define CHECK_STATUS(status) ::hm11::examples::check_status((status))

#endif // HM11_EXAMPLES_COMMON_ERROR_HANDLING_HPP
