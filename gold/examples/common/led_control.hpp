/**
 * @file led_control.hpp
 * @brief LED control abstraction for HM-11 driver examples
 *
 * This header provides portable LED control macros that work across different
 * build targets:
 * - **STM32 embedded target**: Controls physical hardware LEDs
 * - **Native/test builds**: No-ops or mockable for testing
 *
 * ## Usage Pattern
 *
 * Examples use these macros for visual feedback throughout their execution:
 * ```cpp
 * #include "examples/common/led_control.hpp"
 *
 * INIT_LEDS();              // Initialize LED hardware
 * LED_GREEN_ON();           // Success indication
 * LED_RED_TOGGLE();         // Error indication (blinking)
 * ```
 *
 * ## Platform Independence
 *
 * The macros abstract platform-specific LED operations:
 * - Define platform-specific implementations before including this header
 * - Default to no-ops if not defined (safe for native builds)
 * - Can be mocked in tests using preprocessor definitions
 *
 * ## STM32 Implementation
 *
 * For STM32F429 Discovery board, define LED operations based on your board's
 * LED configuration (typically Green LED on GPIO pin, Red LED on another).
 * The platform-specific CMake configuration should provide these definitions.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#ifndef HM11_EXAMPLES_COMMON_LED_CONTROL_HPP
#define HM11_EXAMPLES_COMMON_LED_CONTROL_HPP

// ============================================================================
// LED Initialization
// ============================================================================

/**
 * @brief Initialize LED hardware
 *
 * Must be called before using any LED control macros.
 * On STM32, configures GPIO pins for LED output.
 * On native builds, no-op by default.
 */
#ifndef INIT_LEDS
#define INIT_LEDS()                                                                                                        \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

// ============================================================================
// Green LED Control (Success Indication)
// ============================================================================

/**
 * @brief Turn on green LED
 *
 * Indicates successful operation or connected state.
 */
#ifndef LED_GREEN_ON
#define LED_GREEN_ON()                                                                                                     \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

/**
 * @brief Turn off green LED
 *
 * Indicates idle or waiting state.
 */
#ifndef LED_GREEN_OFF
#define LED_GREEN_OFF()                                                                                                    \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

/**
 * @brief Toggle green LED state
 *
 * Used for blinking patterns (waiting for connection, indicating activity).
 */
#ifndef LED_GREEN_TOGGLE
#define LED_GREEN_TOGGLE()                                                                                                 \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

// ============================================================================
// Red LED Control (Error Indication)
// ============================================================================

/**
 * @brief Turn on red LED
 *
 * Indicates error state or failed operation.
 */
#ifndef LED_RED_ON
#define LED_RED_ON()                                                                                                       \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

/**
 * @brief Turn off red LED
 *
 * Clears error indication.
 */
#ifndef LED_RED_OFF
#define LED_RED_OFF()                                                                                                      \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

/**
 * @brief Toggle red LED state
 *
 * Used for error blinking patterns (operation failed, entering error handler).
 */
#ifndef LED_RED_TOGGLE
#define LED_RED_TOGGLE()                                                                                                   \
    do {                                                                                                                   \
        /* no-op in default build */                                                                                       \
    } while (0)
#endif

// ============================================================================
// Common LED Patterns (Inline Utility Functions)
// ============================================================================

#include <chrono>
#include <thread>

namespace hm11::examples {

/**
 * @brief Blink green LED to indicate success
 *
 * Provides visual feedback that an operation completed successfully.
 * Blinks green LED 3 times with 200ms period.
 */
inline void indicate_success() noexcept {
    for (int i = 0; i < 3; ++i) {
        LED_GREEN_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Enter error state with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
 * In a real embedded system, this would be the error handler.
 *
 * @note This function never returns - it is the terminal error handler
 */
[[noreturn]] inline void indicate_error() noexcept {
    while (true) {
        LED_RED_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

} // namespace hm11::examples

#endif // HM11_EXAMPLES_COMMON_LED_CONTROL_HPP
