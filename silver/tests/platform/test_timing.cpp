// Tests for timing utilities and deadline calculations
// Validates timeout and timestamp operations across platforms

#include <gtest/gtest.h>

#include <thread>

#include "hm11/platform/timing.hpp"

namespace hm11::platform::test {

class TimingTest : public ::testing::Test {
  protected:
    void SetUp() override { start_time_ = Timing::Now(); }

    Timing::Timestamp start_time_;
};

// Test basic timing operations
TEST_F(TimingTest, BasicTimingOperations) {
    auto now1 = Timing::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto now2 = Timing::Now();

    // Time should advance
    EXPECT_LT(now1, now2);

    // Difference should be approximately 10ms (with some tolerance)
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now2 - now1);
    EXPECT_GE(diff.count(), 8);  // At least 8ms (account for scheduling)
    EXPECT_LE(diff.count(), 50); // No more than 50ms (reasonable upper bound)
}

// Test deadline calculations
TEST_F(TimingTest, DeadlineCalculations) {
    auto timeout = Timing::Duration{100}; // 100ms timeout
    auto deadline = Timing::DeadlineAfter(timeout);

    // Deadline should be in the future
    EXPECT_GT(deadline, Timing::Now());

    // Deadline should be approximately timeout milliseconds from now
    auto expected_deadline = Timing::Now() + timeout;
    auto duration_diff = deadline - expected_deadline;
    auto deadline_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        duration_diff >= decltype(duration_diff)::zero() ? duration_diff : -duration_diff);
    EXPECT_LE(deadline_diff.count(), 5); // Within 5ms tolerance
}

// Test deadline expiration checking
TEST_F(TimingTest, DeadlineExpirationChecking) {
    // Create deadline 50ms in the future
    auto deadline = Timing::DeadlineAfter(Timing::Duration{50});

    // Should not be exceeded initially
    EXPECT_FALSE(Timing::IsDeadlineExceeded(deadline));

    // Sleep for 60ms to exceed deadline
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Should now be exceeded
    EXPECT_TRUE(Timing::IsDeadlineExceeded(deadline));
}

// Test remaining time calculation
TEST_F(TimingTest, RemainingTimeCalculation) {
    auto deadline = Timing::DeadlineAfter(Timing::Duration{100});

    // Initially should have close to full timeout remaining
    auto remaining = Timing::TimeUntilDeadline(deadline);
    EXPECT_GE(remaining.count(), 90);  // At least 90ms remaining
    EXPECT_LE(remaining.count(), 105); // No more than 105ms remaining

    // Sleep for 50ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should have about 50ms remaining (with tolerance)
    remaining = Timing::TimeUntilDeadline(deadline);
    EXPECT_GE(remaining.count(), 30); // At least 30ms (scheduling tolerance)
    EXPECT_LE(remaining.count(), 60); // No more than 60ms

    // Sleep until deadline is exceeded
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Should return zero when deadline exceeded
    remaining = Timing::TimeUntilDeadline(deadline);
    EXPECT_EQ(remaining.count(), 0);
}

// Test time unit conversions
TEST_F(TimingTest, TimeUnitConversions) {
    auto timeout = Timing::Duration{100}; // 100ms

    // Convert to microseconds
    auto microseconds = Timing::ToMicroseconds(timeout);
    EXPECT_EQ(microseconds.count(), 100000); // 100ms = 100,000μs

    // Convert to nanoseconds
    auto nanoseconds = Timing::ToNanoseconds(timeout);
    EXPECT_EQ(nanoseconds.count(), 100000000); // 100ms = 100,000,000ns

    // Test zero duration
    auto zero_timeout = Timing::Duration{0};
    EXPECT_EQ(Timing::ToMicroseconds(zero_timeout).count(), 0);
    EXPECT_EQ(Timing::ToNanoseconds(zero_timeout).count(), 0);
}

// Test timing constants
TEST_F(TimingTest, TimingConstants) {
    // Verify constant values are reasonable
    EXPECT_GT(Timing::Constants::INTERRUPT_LATENCY.count(), 0);
    EXPECT_LT(Timing::Constants::INTERRUPT_LATENCY.count(), 1000); // Less than 1ms

    EXPECT_GT(Timing::Constants::CONTEXT_SWITCH_TIME.count(), 0);
    EXPECT_LT(Timing::Constants::CONTEXT_SWITCH_TIME.count(), 10000); // Less than 10ms

    EXPECT_GT(Timing::Constants::MINIMUM_TIMEOUT.count(), 0);
    EXPECT_LT(Timing::Constants::MINIMUM_TIMEOUT.count(), 100); // Less than 100ms

    EXPECT_GT(Timing::Constants::DEFAULT_TIMEOUT.count(), 100);
    EXPECT_LT(Timing::Constants::DEFAULT_TIMEOUT.count(), 10000); // Between 100ms and 10s

    EXPECT_GT(Timing::Constants::MAXIMUM_TIMEOUT.count(), 10000);
    EXPECT_LT(Timing::Constants::MAXIMUM_TIMEOUT.count(), 60000); // Between 10s and 60s

    // Verify ordering of timeout constants
    EXPECT_LT(Timing::Constants::MINIMUM_TIMEOUT, Timing::Constants::DEFAULT_TIMEOUT);
    EXPECT_LT(Timing::Constants::DEFAULT_TIMEOUT, Timing::Constants::MAXIMUM_TIMEOUT);
}

// Test steady clock behavior (monotonic)
TEST_F(TimingTest, SteadyClockMonotonic) {
    std::vector<Timing::Timestamp> timestamps;

    // Collect multiple timestamps
    for (int i = 0; i < 10; ++i) {
        timestamps.push_back(Timing::Now());
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Verify timestamps are monotonically increasing
    for (size_t i = 1; i < timestamps.size(); ++i) {
        EXPECT_GE(timestamps[i], timestamps[i - 1]) << "Timestamp " << i << " went backwards";
    }
}

// Performance test - timing operations should be fast
TEST_F(TimingTest, PerformanceBaseline) {
    const size_t iterations = 1000; // Reduced for CI stability

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        auto now = Timing::Now();
        auto deadline = Timing::DeadlineAfter(Timing::Duration{100});
        volatile bool exceeded = Timing::IsDeadlineExceeded(deadline);
        (void)exceeded; // Suppress unused variable warning
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Timing operations should be reasonably fast
    // Note: Debug builds with sanitizers can be much slower
    // CI/test environments may have high variability
    // Very generous timeout to avoid CI infrastructure issues
    EXPECT_LT(duration.count(), 10000000) << "Timing operations too slow: " << duration.count() << " μs";
}

// Test deadline calculation edge cases
TEST_F(TimingTest, DeadlineEdgeCases) {
    // Zero timeout should create deadline in immediate past/present
    auto zero_deadline = Timing::DeadlineAfter(Timing::Duration{0});
    auto remaining = Timing::TimeUntilDeadline(zero_deadline);
    EXPECT_EQ(remaining.count(), 0);

    // Very large timeout should work without overflow
    auto large_deadline = Timing::DeadlineAfter(Timing::Constants::MAXIMUM_TIMEOUT);
    EXPECT_FALSE(Timing::IsDeadlineExceeded(large_deadline));
    auto large_remaining = Timing::TimeUntilDeadline(large_deadline);
    EXPECT_GT(large_remaining.count(), 25000); // At least 25 seconds remaining
}

} // namespace hm11::platform::test