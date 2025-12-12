// HM11 Synchronization Primitives - Watchdog Timer Unit Tests
// Testing ISR-side deadline polling and timeout mechanisms

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "hm11/sync/watchdog.hpp"

using namespace hm11::sync;
using namespace hm11::hal;

class WatchdogTest : public ::testing::Test {
  protected:
    Watchdog watchdog_;
};

/// Test basic start/stop operations
TEST_F(WatchdogTest, BasicStartStop) {
    // Initially not started
    EXPECT_FALSE(watchdog_.IsStarted());

    // Start with timeout
    watchdog_.Start(std::chrono::milliseconds(100));
    EXPECT_TRUE(watchdog_.IsStarted());

    // Stop watchdog
    watchdog_.Stop();
    EXPECT_FALSE(watchdog_.IsStarted());
}

/// Test normal completion signaling from ISR
TEST_F(WatchdogTest, NormalCompletionSignaling) {
    // Start watchdog
    watchdog_.Start(std::chrono::milliseconds(200));

    // Signal completion from mock ISR after short delay
    std::thread mock_isr([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        watchdog_.SignalCompletionFromISR(UARTStatus::Ok);
    });

    // Wait for completion
    auto status = watchdog_.AwaitCompletion();
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_FALSE(watchdog_.IsStarted()); // Should be stopped after completion

    mock_isr.join();
}

/// Test error status propagation from ISR
TEST_F(WatchdogTest, ErrorStatusPropagation) {
    watchdog_.Start(std::chrono::milliseconds(200));

    // Signal error from mock ISR
    std::thread mock_isr([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        watchdog_.SignalCompletionFromISR(UARTStatus::ErrError);
    });

    // Should receive error status
    auto status = watchdog_.AwaitCompletion();
    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_FALSE(watchdog_.IsStarted());

    mock_isr.join();
}

/// Test ISR-side timeout checking
TEST_F(WatchdogTest, ISRSideTimeoutChecking) {
    // Start with short timeout
    watchdog_.Start(std::chrono::milliseconds(50));

    // Mock ISR that checks timeout periodically
    std::atomic<bool> timeout_detected{false};
    std::thread mock_isr([&]() {
        // Wait for timeout to occur
        std::this_thread::sleep_for(std::chrono::milliseconds(60));

        // Check timeout from ISR context
        bool timed_out = watchdog_.CheckTimeoutFromISR();
        timeout_detected.store(timed_out);
    });

    // Wait for completion (should be timeout)
    auto status = watchdog_.AwaitCompletion();
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
    EXPECT_FALSE(watchdog_.IsStarted());

    mock_isr.join();
    EXPECT_TRUE(timeout_detected.load());
}

/// Test that timeout check returns false when not started
TEST_F(WatchdogTest, TimeoutCheckWhenNotStarted) {
    // Should return false when not started
    EXPECT_FALSE(watchdog_.CheckTimeoutFromISR());

    // Start and immediately stop
    watchdog_.Start(std::chrono::milliseconds(100));
    watchdog_.Stop();

    // Should return false after stop
    EXPECT_FALSE(watchdog_.CheckTimeoutFromISR());
}

/// Test timeout check returns false when deadline not reached
TEST_F(WatchdogTest, TimeoutCheckBeforeDeadline) {
    // Start with long timeout
    watchdog_.Start(std::chrono::milliseconds(1000));

    // Check timeout immediately (should return false)
    EXPECT_FALSE(watchdog_.CheckTimeoutFromISR());
    EXPECT_TRUE(watchdog_.IsStarted()); // Should still be started
}

/// Test concurrent timeout checking from multiple mock ISRs
TEST_F(WatchdogTest, ConcurrentTimeoutChecking) {
    watchdog_.Start(std::chrono::milliseconds(100));

    std::atomic<int> timeout_count{0};
    std::vector<std::thread> mock_isrs;

    // Start multiple mock ISR threads
    for (int i = 0; i < 5; ++i) {
        mock_isrs.emplace_back([&]() {
            // Wait for timeout to occur
            std::this_thread::sleep_for(std::chrono::milliseconds(110));

            // Check timeout (only one should succeed in signaling)
            if (watchdog_.CheckTimeoutFromISR()) {
                timeout_count.fetch_add(1);
            }
        });
    }

    // Wait for completion
    auto status = watchdog_.AwaitCompletion();
    EXPECT_EQ(status, UARTStatus::ErrTimeout);

    // Wait for all mock ISRs
    for (auto& isr : mock_isrs) {
        isr.join();
    }

    // Only one ISR should have successfully signaled timeout
    EXPECT_EQ(timeout_count.load(), 1);
}

/// Test race condition between completion and timeout
TEST_F(WatchdogTest, CompletionVsTimeoutRace) {
    // Use very short timeout to create race condition
    watchdog_.Start(std::chrono::milliseconds(10));

    std::atomic<int> completion_attempts{0};
    std::atomic<int> timeout_attempts{0};

    // Mock ISR attempting completion
    std::thread completion_isr([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(8)); // Just before timeout
        watchdog_.SignalCompletionFromISR(UARTStatus::Ok);
        completion_attempts.fetch_add(1);
    });

    // Mock ISR attempting timeout
    std::thread timeout_isr([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(12)); // Just after timeout
        if (watchdog_.CheckTimeoutFromISR()) {
            timeout_attempts.fetch_add(1);
        }
    });

    // Wait for result (should be either completion or timeout)
    auto status = watchdog_.AwaitCompletion();
    EXPECT_TRUE(status == UARTStatus::Ok || status == UARTStatus::ErrTimeout);

    completion_isr.join();
    timeout_isr.join();

    // Both ISRs should have attempted their operations
    EXPECT_EQ(completion_attempts.load(), 1);
    // Timeout attempt depends on timing - may or may not execute depending on which wins
}

/// Test multiple start/stop cycles
TEST_F(WatchdogTest, MultipleStartStopCycles) {
    constexpr int CYCLES = 100;

    for (int i = 0; i < CYCLES; ++i) {
        // Start watchdog
        watchdog_.Start(std::chrono::milliseconds(50));
        EXPECT_TRUE(watchdog_.IsStarted());

        // Stop immediately
        watchdog_.Stop();
        EXPECT_FALSE(watchdog_.IsStarted());

        // Verify timeout check returns false after stop
        EXPECT_FALSE(watchdog_.CheckTimeoutFromISR());
    }
}

/// Test restart after timeout
TEST_F(WatchdogTest, RestartAfterTimeout) {
    // First timeout cycle
    watchdog_.Start(std::chrono::milliseconds(10));

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    watchdog_.CheckTimeoutFromISR();

    auto first_status = watchdog_.AwaitCompletion();
    EXPECT_EQ(first_status, UARTStatus::ErrTimeout);

    // Second successful cycle
    watchdog_.Start(std::chrono::milliseconds(100));

    std::thread completion([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        watchdog_.SignalCompletionFromISR(UARTStatus::Ok);
    });

    auto second_status = watchdog_.AwaitCompletion();
    EXPECT_EQ(second_status, UARTStatus::Ok);

    completion.join();
}

/// Test deadline accuracy
TEST_F(WatchdogTest, DeadlineAccuracy) {
    constexpr auto TIMEOUT_MS = 100;

    auto start_time = std::chrono::steady_clock::now();
    watchdog_.Start(std::chrono::milliseconds(TIMEOUT_MS));

    // Wait for deadline to pass
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS + 10));

    // Check timeout should detect deadline exceeded
    EXPECT_TRUE(watchdog_.CheckTimeoutFromISR());

    auto status = watchdog_.AwaitCompletion();
    EXPECT_EQ(status, UARTStatus::ErrTimeout);

    auto elapsed = std::chrono::steady_clock::now() - start_time;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    // Should be approximately the timeout duration (with some tolerance)
    EXPECT_GE(elapsed_ms.count(), TIMEOUT_MS);
    EXPECT_LE(elapsed_ms.count(), TIMEOUT_MS + 50); // 50ms tolerance
}

/// Performance test for ISR operations
TEST_F(WatchdogTest, ISRPerformanceBenchmark) {
    constexpr int ITERATIONS = 10000;

    // Benchmark CheckTimeoutFromISR performance
    watchdog_.Start(std::chrono::milliseconds(1000)); // Long timeout to avoid triggering

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog_.CheckTimeoutFromISR();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto avg_ns = total_ns / ITERATIONS;

    std::cout << "Average CheckTimeoutFromISR time: " << avg_ns << " ns" << std::endl;

    // Should be reasonable even in debug builds (<100µs per check)
    EXPECT_LT(avg_ns, 100000); // 100µs = 100,000ns

    watchdog_.Stop();
}