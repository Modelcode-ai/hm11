/**
 * @file mock_timer.cpp
 * @brief Implementation of mock timer for testing
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "mock_timer.hpp"

#include <algorithm>

namespace hm11::test {

void MockTimer::start() noexcept {
    elapsed_time_ = std::chrono::milliseconds(0);
    running_ = true;
}

void MockTimer::stop() noexcept {
    running_ = false;
}

void MockTimer::advance(std::chrono::milliseconds duration) noexcept {
    if (running_) {
        elapsed_time_ += duration;
    }
}

void MockTimer::advance_ms(uint32_t milliseconds) noexcept {
    advance(std::chrono::milliseconds(milliseconds));
}

void MockTimer::reset() noexcept {
    elapsed_time_ = std::chrono::milliseconds(0);
    running_ = false;
}

std::chrono::milliseconds MockTimer::remaining(std::chrono::milliseconds timeout) const noexcept {
    if (elapsed_time_ >= timeout) {
        return std::chrono::milliseconds(0);
    }
    return timeout - elapsed_time_;
}

} // namespace hm11::test
