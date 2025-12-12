/**
 * @file test_watchdog.cpp
 * @brief Unit tests for Watchdog timeout manager
 *
 * This file contains comprehensive unit tests for the Watchdog class that
 * coordinates timeout handling, event signaling, and error propagation between
 * ISR and application contexts.
 *
 * ## Test Coverage
 *
 * - Basic Start/Await/Release cycle with Ok status
 * - Start/Await/Release with error status (ErrError)
 * - Simulated timeout expiration (test ISR checks deadline and calls Release(ErrTimeout))
 * - Multiple sequential operations (reusability)
 * - Early Stop cancellation (Stop called before Release)
 * - Release from simulated ISR context (std::thread)
 * - Edge cases: Release called twice (second is no-op), Stop called twice (idempotent)
 * - ThreadSanitizer validation (run with -fsanitize=thread)
 *
 * ## Running Tests
 *
 * ```bash
 * # Standard build and test
 * cmake --build build --target test_watchdog
 * ./build/tests/test_watchdog
 *
 * # With ThreadSanitizer
 * cmake -DCMAKE_BUILD_TYPE=Debug \
 *       -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
 *       -B build-tsan
 * cmake --build build-tsan --target test_watchdog
 * ./build-tsan/tests/test_watchdog
 * ```
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <chrono>
#include <hm11/sync/watchdog.hpp>
#include <thread>

using namespace std::chrono_literals;
using hm11::hal::UartStatus;
using hm11::sync::Watchdog;

// Test fixture for Watchdog tests
class WatchdogTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // Test fixtures require protected members for test access
    Watchdog watchdog;
    alignas(32) std::array<uint8_t, 128> buffer{}; // Aligned buffer for future DMA use
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void SetUp() override {
        // Clear buffer before each test
        std::ranges::fill(buffer, 0);
    }
};

// ============================================================================
// Basic Operation Tests
// ============================================================================

/**
 * @brief Test basic Start/Await/Release cycle with Ok status
 *
 * This test verifies the fundamental operation sequence:
 * 1. Application calls start() to begin operation
 * 2. Simulated ISR calls release(Ok) to signal completion
 * 3. Application calls await_event() and receives Ok status
 */
TEST_F(WatchdogTest, BasicStartAwaitReleaseCycle) {
    // Start operation with 1 second timeout
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Simulate ISR signaling completion in a separate thread
    std::thread isr_thread([this]() {
        // Simulate some ISR processing time
        std::this_thread::sleep_for(10ms);

        // Signal successful completion
        watchdog.release(UartStatus::Ok);
    });

    // Wait for ISR to signal (should complete in ~10ms)
    auto status = watchdog.await_event(1s);

    // Verify we received Ok status
    EXPECT_EQ(status, UartStatus::Ok);

    // Cleanup
    isr_thread.join();
}

/**
 * @brief Test Start/Await/Release with error status
 *
 * Verifies that error status codes propagate correctly from ISR to application.
 */
TEST_F(WatchdogTest, ReleaseWithErrorStatus) {
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Simulate ISR detecting hardware error
    std::thread isr_thread([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::ErrError);
    });

    // Wait for ISR signal
    auto status = watchdog.await_event(1s);

    // Verify error status propagated correctly
    EXPECT_EQ(status, UartStatus::ErrError);

    isr_thread.join();
}

// ============================================================================
// Timeout Tests
// ============================================================================

/**
 * @brief Test simulated timeout expiration
 *
 * Simulates the ISR checking the deadline and releasing with ErrTimeout status.
 * This tests Decision 25 (ISR-side deadline polling).
 */
TEST_F(WatchdogTest, SimulatedTimeoutExpiration) {
    // Start operation with short timeout
    watchdog.start(buffer.data(), buffer.size(), 100ms);

    // Simulate ISR checking deadline and detecting timeout
    std::thread isr_thread([this]() {
        // Wait past the deadline
        std::this_thread::sleep_for(150ms);

        // ISR checks deadline and finds it expired
        EXPECT_TRUE(watchdog.check_deadline());

        // Signal timeout
        watchdog.release(UartStatus::ErrTimeout);
    });

    // Wait for ISR to signal timeout
    auto status = watchdog.await_event(200ms);

    // Verify timeout status propagated correctly
    EXPECT_EQ(status, UartStatus::ErrTimeout);

    isr_thread.join();
}

/**
 * @brief Test that check_deadline() returns false before deadline
 *
 * Verifies that the deadline check correctly identifies when timeout has not
 * yet occurred.
 */
TEST_F(WatchdogTest, CheckDeadlineReturnsFalseBeforeExpiration) {
    // Start operation with long timeout
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Check deadline immediately (should not be expired)
    EXPECT_FALSE(watchdog.check_deadline());

    // Cleanup: signal to avoid blocking
    watchdog.release(UartStatus::Ok);
    (void)watchdog.await_event(100ms);
}

/**
 * @brief Test that check_deadline() returns false when not started
 *
 * Verifies defensive behavior when checking deadline without active operation.
 */
TEST_F(WatchdogTest, CheckDeadlineReturnsFalseWhenNotStarted) {
    // Check deadline without starting operation
    EXPECT_FALSE(watchdog.check_deadline());
}

// ============================================================================
// Multiple Operations Tests
// ============================================================================

/**
 * @brief Test multiple sequential operations (reusability)
 *
 * Verifies that the Watchdog can be reused for multiple operations without
 * state leakage between operations.
 */
TEST_F(WatchdogTest, MultipleSequentialOperations) {
    // First operation
    watchdog.start(buffer.data(), buffer.size(), 1s);
    std::thread isr_thread1([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });
    auto status1 = watchdog.await_event(1s);
    EXPECT_EQ(status1, UartStatus::Ok);
    isr_thread1.join();

    // Second operation with different status
    watchdog.start(buffer.data(), buffer.size(), 1s);
    std::thread isr_thread2([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::ErrError);
    });
    auto status2 = watchdog.await_event(1s);
    EXPECT_EQ(status2, UartStatus::ErrError);
    isr_thread2.join();

    // Third operation back to Ok
    watchdog.start(buffer.data(), buffer.size(), 1s);
    std::thread isr_thread3([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });
    auto status3 = watchdog.await_event(1s);
    EXPECT_EQ(status3, UartStatus::Ok);
    isr_thread3.join();
}

// ============================================================================
// Early Stop Tests
// ============================================================================

/**
 * @brief Test early Stop cancellation (Stop called before Release)
 *
 * Verifies that the application can cancel an operation early by calling stop().
 */
TEST_F(WatchdogTest, EarlyStopCancellation) {
    // Start operation
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Cancel operation early (before ISR signals)
    watchdog.stop();

    // Simulate ISR trying to signal after stop (should still work)
    std::thread isr_thread([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });

    // Wait for ISR signal (should still receive it)
    auto status = watchdog.await_event(100ms);

    // The status should be Ok because release() still stores it
    EXPECT_EQ(status, UartStatus::Ok);

    isr_thread.join();
}

// ============================================================================
// Idempotency Tests
// ============================================================================

/**
 * @brief Test that start() is idempotent (second call is no-op)
 *
 * Verifies that calling start() while already started is safe and returns
 * immediately without side effects. This matches Ada's behavior (drivers.adb:85-87).
 */
TEST_F(WatchdogTest, StartIsIdempotent) {
    // First start
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Second start should be no-op
    watchdog.start(buffer.data(), buffer.size(), 500ms);

    // Simulate ISR signaling
    std::thread isr_thread([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });

    // The original timeout (1s) should still be in effect, not the second one (500ms)
    // We verify this indirectly by checking that await_event succeeds
    auto status = watchdog.await_event(1s);
    EXPECT_EQ(status, UartStatus::Ok);

    isr_thread.join();
}

/**
 * @brief Test that stop() is idempotent (multiple calls are safe)
 *
 * Verifies that calling stop() multiple times is safe and has no side effects
 * after the first call. This matches Ada's behavior (drivers.adb:145-147).
 */
TEST_F(WatchdogTest, StopIsIdempotent) {
    // Start operation
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Call stop multiple times
    watchdog.stop();
    watchdog.stop();
    watchdog.stop();

    // Should still be able to signal and complete
    std::thread isr_thread([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });

    auto status = watchdog.await_event(100ms);
    EXPECT_EQ(status, UartStatus::Ok);

    isr_thread.join();
}

/**
 * @brief Test that stop() without start() is safe
 *
 * Verifies defensive behavior when stop() is called without a prior start().
 */
TEST_F(WatchdogTest, StopWithoutStartIsSafe) {
    // Call stop without starting
    watchdog.stop();

    // Should be no-op, no crash or undefined behavior
    // If we got here, the test passed
    SUCCEED();
}

/**
 * @brief Test that release() called twice is idempotent (second call is no-op)
 *
 * Verifies that only the first release() call has effect, preventing status
 * overwrite. This matches Ada's behavior (drivers.adb:65):
 *   if not Event_Occurred then
 *      Read_Status := Status;
 *      Event_Occurred := True;
 *   end if;
 */
TEST_F(WatchdogTest, ReleaseIsIdempotent) {
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Simulate multiple ISR calls (first with Ok, second with Error)
    std::thread isr_thread([this]() {
        std::this_thread::sleep_for(10ms);

        // First release with Ok
        watchdog.release(UartStatus::Ok);

        // Second release with Error (should be no-op)
        watchdog.release(UartStatus::ErrError);
    });

    // Wait for ISR signals
    auto status = watchdog.await_event(1s);

    // Should receive Ok (first status), not ErrError (second status)
    EXPECT_EQ(status, UartStatus::Ok);

    isr_thread.join();
}

/**
 * @brief Test concurrent release() calls from multiple threads
 *
 * Verifies that release() is thread-safe and only the first call has effect
 * when multiple threads try to signal simultaneously.
 */
TEST_F(WatchdogTest, ConcurrentReleaseCallsOnlyFirstSucceeds) {
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Spawn multiple threads all trying to release with different statuses
    std::thread isr_thread1([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::Ok);
    });

    std::thread isr_thread2([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::ErrError);
    });

    std::thread isr_thread3([this]() {
        std::this_thread::sleep_for(10ms);
        watchdog.release(UartStatus::ErrTimeout);
    });

    // Wait for ISR signals
    auto status = watchdog.await_event(1s);

    // Should receive exactly one status (whichever thread won the race)
    // The important thing is we got a valid status and no crash occurred
    EXPECT_TRUE(status == UartStatus::Ok || status == UartStatus::ErrError || status == UartStatus::ErrTimeout);

    isr_thread1.join();
    isr_thread2.join();
    isr_thread3.join();
}

// ============================================================================
// Signal-Before-Wait Tests
// ============================================================================

/**
 * @brief Test that release() before await_event() causes immediate return
 *
 * Verifies signal-before-wait semantics: if the ISR signals before the
 * application calls await_event(), the await_event() should return
 * immediately without blocking.
 */
TEST_F(WatchdogTest, SignalBeforeWait) {
    watchdog.start(buffer.data(), buffer.size(), 1s);

    // Signal immediately (before await_event)
    watchdog.release(UartStatus::Ok);

    // Give time for release to complete
    std::this_thread::sleep_for(10ms);

    // Now call await_event - should return immediately
    auto start_time = std::chrono::steady_clock::now();
    auto status = watchdog.await_event(1s);
    auto elapsed = std::chrono::steady_clock::now() - start_time;

    // Verify immediate return (should be much less than timeout)
    EXPECT_LT(elapsed, 100ms);
    EXPECT_EQ(status, UartStatus::Ok);
}

// ============================================================================
// ThreadSanitizer Stress Tests
// ============================================================================

/**
 * @brief Stress test with many rapid operations
 *
 * This test performs many rapid Start/Release/Await cycles to stress-test
 * the atomic operations and memory ordering. It should be run with
 * ThreadSanitizer (-fsanitize=thread) to detect any data races.
 */
TEST_F(WatchdogTest, StressTestRapidOperations) {
    constexpr int NUM_ITERATIONS = 100;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        watchdog.start(buffer.data(), buffer.size(), 100ms);

        // Simulate ISR signaling with minimal delay
        std::thread isr_thread([this, i]() {
            // Alternate between success and error to test status handling
            auto status = (i % 2 == 0) ? UartStatus::Ok : UartStatus::ErrError;
            watchdog.release(status);
        });

        auto status = watchdog.await_event(100ms);

        // Verify expected status for this iteration
        auto expected = (i % 2 == 0) ? UartStatus::Ok : UartStatus::ErrError;
        EXPECT_EQ(status, expected) << "Iteration " << i;

        isr_thread.join();
    }
}

/**
 * @brief Stress test with concurrent operations
 *
 * Tests multiple concurrent threads performing operations to stress-test
 * thread safety. Should be run with ThreadSanitizer to detect races.
 */
TEST_F(WatchdogTest, StressTestConcurrentAccess) {
    constexpr int NUM_OPERATIONS = 50;
    std::atomic<int> completed_operations{0};

    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        watchdog.start(buffer.data(), buffer.size(), 100ms);

        // Spawn ISR thread
        std::thread isr_thread([this]() { watchdog.release(UartStatus::Ok); });

        // Main thread waits
        auto status = watchdog.await_event(200ms);
        EXPECT_EQ(status, UartStatus::Ok);

        isr_thread.join();
        completed_operations++;
    }

    EXPECT_EQ(completed_operations.load(), NUM_OPERATIONS);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
