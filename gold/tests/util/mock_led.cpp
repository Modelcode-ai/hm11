/**
 * @file mock_led.cpp
 * @brief Implementation of mock LED for testing
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "mock_led.hpp"

namespace hm11::test {

void MockLED::on() noexcept {
    current_state_ = true;
    state_history_.push_back(true);
    ++on_count_;
}

void MockLED::off() noexcept {
    current_state_ = false;
    state_history_.push_back(false);
    ++off_count_;
}

void MockLED::toggle() noexcept {
    current_state_ = !current_state_;
    state_history_.push_back(current_state_);
    ++toggle_count_;
}

void MockLED::set(bool state) noexcept {
    current_state_ = state;
    state_history_.push_back(state);
    if (state) {
        ++on_count_;
    } else {
        ++off_count_;
    }
}

void MockLED::clear() noexcept {
    current_state_ = false;
    state_history_.clear();
    on_count_ = 0;
    off_count_ = 0;
    toggle_count_ = 0;
}

void MockLED::clear_history() noexcept {
    state_history_.clear();
    on_count_ = 0;
    off_count_ = 0;
    toggle_count_ = 0;
}

} // namespace hm11::test
