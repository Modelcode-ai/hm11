/**
 * @file mock_timer.hpp
 * @brief Mock timer for controllable time simulation in tests
 *
 * This header provides a mock timer implementation for testing timeout behavior
 * and timing-dependent operations without real time delays.
 *
 * ## Features
 *
 * - Controllable time advancement
 * - Query elapsed time
 * - Reset timer state
 * - Simulate timeout conditions
 *
 * ## Usage Example
 *
 * @code
 * #include "tests/util/mock_timer.hpp"
 *
 * MockTimer timer;
 *
 * // Start timing
 * timer.start();
 *
 * // Simulate passage of time
 * timer.advance(std::chrono::milliseconds(500));
 *
 * // Check elapsed time
 * EXPECT_EQ(timer.elapsed_ms(), 500);
 *
 * // Test timeout conditions
 * EXPECT_FALSE(timer.has_elapsed(std::chrono::milliseconds(1000)));
 * timer.advance(std::chrono::milliseconds(500));
 * EXPECT_TRUE(timer.has_elapsed(std::chrono::milliseconds(1000)));
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace hm11::test {

/**
 * @brief Mock timer for controllable time simulation
 *
 * Provides deterministic timing for tests without real delays.
 * Useful for testing timeout behavior, retry logic, and timing-dependent operations.
 */
class MockTimer {
  public:
    /**
     * @brief Start or restart the timer
     *
     * Sets the timer to zero and marks it as running.
     */
    void start() noexcept;

    /**
     * @brief Stop the timer
     *
     * Stops time advancement. Elapsed time remains accessible.
     */
    void stop() noexcept;

    /**
     * @brief Advance simulated time
     *
     * @param duration Time to advance in milliseconds
     */
    void advance(std::chrono::milliseconds duration) noexcept;

    /**
     * @brief Advance simulated time by milliseconds
     *
     * @param milliseconds Time to advance
     */
    void advance_ms(uint32_t milliseconds) noexcept;

    /**
     * @brief Get elapsed time since start
     *
     * @return Elapsed time in milliseconds
     */
    [[nodiscard]] std::chrono::milliseconds elapsed() const noexcept { return elapsed_time_; }

    /**
     * @brief Get elapsed time in milliseconds
     *
     * @return Elapsed milliseconds as uint32_t
     */
    [[nodiscard]] uint32_t elapsed_ms() const noexcept { return static_cast<uint32_t>(elapsed_time_.count()); }

    /**
     * @brief Check if a duration has elapsed
     *
     * @param duration Duration to check against
     * @return true if elapsed time >= duration
     */
    [[nodiscard]] bool has_elapsed(std::chrono::milliseconds duration) const noexcept { return elapsed_time_ >= duration; }

    /**
     * @brief Check if timer is running
     *
     * @return true if timer is running
     */
    [[nodiscard]] bool is_running() const noexcept { return running_; }

    /**
     * @brief Reset timer to initial state
     *
     * Sets elapsed time to zero and stops the timer.
     */
    void reset() noexcept;

    /**
     * @brief Set elapsed time directly
     *
     * Useful for setting up specific test conditions.
     *
     * @param duration Time to set
     */
    void set_elapsed(std::chrono::milliseconds duration) noexcept { elapsed_time_ = duration; }

    /**
     * @brief Get remaining time until timeout
     *
     * @param timeout Total timeout duration
     * @return Remaining time, or zero if timeout has elapsed
     */
    [[nodiscard]] std::chrono::milliseconds remaining(std::chrono::milliseconds timeout) const noexcept;

  private:
    std::chrono::milliseconds elapsed_time_{0}; ///< Simulated elapsed time
    bool running_{false};                       ///< Whether timer is running
};

/**
 * @brief Scoped timer that automatically starts on construction
 *
 * RAII wrapper around MockTimer for convenient timing blocks.
 *
 * @code
 * {
 *     ScopedMockTimer timer;
 *     // Simulate operation
 *     timer.advance(std::chrono::milliseconds(100));
 *     EXPECT_EQ(timer.elapsed_ms(), 100);
 * } // Timer stops automatically
 * @endcode
 */
class ScopedMockTimer : public MockTimer {
  public:
    /**
     * @brief Construct and start timer
     */
    ScopedMockTimer() noexcept { start(); }

    /**
     * @brief Stop timer on destruction
     */
    ~ScopedMockTimer() noexcept { stop(); }

    // Delete copy and move to enforce RAII semantics
    ScopedMockTimer(const ScopedMockTimer&) = delete;
    ScopedMockTimer& operator=(const ScopedMockTimer&) = delete;
    ScopedMockTimer(ScopedMockTimer&&) = delete;
    ScopedMockTimer& operator=(ScopedMockTimer&&) = delete;
};

} // namespace hm11::test
