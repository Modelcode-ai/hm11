/**
 * @file mock_led.hpp
 * @brief Mock LED for simulating visual feedback in tests
 *
 * This header provides a mock LED implementation for testing example applications
 * that use LED indicators for status feedback. Records state changes for validation.
 *
 * ## Features
 *
 * - Record LED state changes (on/off, toggle)
 * - Query current state
 * - Verify state change history
 * - Clear history for multi-step tests
 *
 * ## Usage Example
 *
 * @code
 * #include "tests/util/mock_led.hpp"
 *
 * MockLED status_led;
 *
 * // Control LED
 * status_led.on();
 * EXPECT_TRUE(status_led.is_on());
 *
 * status_led.off();
 * EXPECT_FALSE(status_led.is_on());
 *
 * status_led.toggle();
 * EXPECT_TRUE(status_led.is_on());
 *
 * // Verify state change history
 * const auto& history = status_led.get_state_history();
 * EXPECT_EQ(history.size(), 3);
 * EXPECT_TRUE(history[0]);   // on()
 * EXPECT_FALSE(history[1]);  // off()
 * EXPECT_TRUE(history[2]);   // toggle()
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <cstddef>
#include <vector>

namespace hm11::test {

/**
 * @brief Mock LED for testing visual feedback
 *
 * Simulates an LED indicator by recording state changes.
 * Compatible with example application code patterns.
 */
class MockLED {
  public:
    /**
     * @brief Turn LED on
     */
    void on() noexcept;

    /**
     * @brief Turn LED off
     */
    void off() noexcept;

    /**
     * @brief Toggle LED state
     */
    void toggle() noexcept;

    /**
     * @brief Set LED state explicitly
     * @param state true for on, false for off
     */
    void set(bool state) noexcept;

    /**
     * @brief Get current LED state
     * @return true if LED is on, false if off
     */
    [[nodiscard]] bool is_on() const noexcept { return current_state_; }

    /**
     * @brief Get LED state change history
     * @return Vector of state changes (true=on, false=off) in chronological order
     */
    [[nodiscard]] const std::vector<bool>& get_state_history() const noexcept { return state_history_; }

    /**
     * @brief Get number of times LED was turned on
     * @return Count of on() and set(true) calls
     */
    [[nodiscard]] std::size_t get_on_count() const noexcept { return on_count_; }

    /**
     * @brief Get number of times LED was turned off
     * @return Count of off() and set(false) calls
     */
    [[nodiscard]] std::size_t get_off_count() const noexcept { return off_count_; }

    /**
     * @brief Get number of times LED was toggled
     * @return Count of toggle() calls
     */
    [[nodiscard]] std::size_t get_toggle_count() const noexcept { return toggle_count_; }

    /**
     * @brief Clear all state and history
     *
     * Resets LED to off state and clears all recorded history.
     */
    void clear() noexcept;

    /**
     * @brief Clear only the history, keep current state
     *
     * Useful for multi-step tests where current state matters
     * but history should be reset between test phases.
     */
    void clear_history() noexcept;

  private:
    bool current_state_{false};        ///< Current LED state (false=off, true=on)
    std::vector<bool> state_history_;  ///< History of state changes
    std::size_t on_count_{0};          ///< Count of on transitions
    std::size_t off_count_{0};         ///< Count of off transitions
    std::size_t toggle_count_{0};      ///< Count of toggle calls
};

} // namespace hm11::test
