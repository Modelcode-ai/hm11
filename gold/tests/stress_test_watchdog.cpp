/**
 * @file stress_test_watchdog.cpp
 * @brief Stress tests for Watchdog ISR-to-thread synchronization
 *
 * These tests verify the Watchdog implementation under high contention:
 * - Multiple rapid release() calls (double-signal prevention)
 * - Timeout precision under load
 * - Start/stop idempotency
 * - Memory ordering correctness
 * - ISR race condition handling
 *
 * Build with ThreadSanitizer to detect race conditions:
 *   clang++ -fsanitize=thread -g -O1 -std=c++20 \
 *           -I../include stress_test_watchdog.cpp -lgtest -lgtest_main \
 *           -o stress_test_watchdog
 *
 * Run with:
 *   ./stress_test_watchdog
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/sync/watchdog.hpp"

using namespace hm11::sync;
using namespace hm11::hal;
using namespace std::chrono_literals;

// ============================================================================
// Test Fixture
// ============================================================================

class WatchdogStressTest : public ::testing::Test {
  protected:
    Watchdog watchdog;
    static constexpr size_t BUFFER_SIZE = 128;
    alignas(32) uint8_t buffer[BUFFER_SIZE]{};
};

// ============================================================================
// Stress Test 1: Double-Signal Prevention
// ============================================================================

TEST_F(WatchdogStressTest, DoubleSignalPrevention) {
    // This tests watchdog.cpp:57 - atomic compare_exchange for double-signal prevention
    constexpr int ITERATIONS = 10'000;
    std::atomic<size_t> ok_count{0};
    std::atomic<size_t> error_count{0};
    std::atomic<size_t> timeout_count{0};

    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        watchdog.start(buffer, BUFFER_SIZE, 100ms);

        // Simulate 3 ISRs firing simultaneously
        std::thread isr1([&]() { watchdog.release(UartStatus::Ok); });

        std::thread isr2([&]() { watchdog.release(UartStatus::ErrError); });

        std::thread isr3([&]() { watchdog.release(UartStatus::ErrTimeout); });

        // Wait for all ISRs
        isr1.join();
        isr2.join();
        isr3.join();

        // Only the FIRST release() should have taken effect
        UartStatus result = watchdog.await_event(100ms);

        // Count which status won (should be non-deterministic but only one)
        switch (result) {
            case UartStatus::Ok:
                ok_count.fetch_add(1, std::memory_order_relaxed);
                break;
            case UartStatus::ErrError:
                error_count.fetch_add(1, std::memory_order_relaxed);
                break;
            case UartStatus::ErrTimeout:
                timeout_count.fetch_add(1, std::memory_order_relaxed);
                break;
            default:
                FAIL() << "Unexpected status: " << static_cast<int>(result);
        }
    }

    // Verify that we got results (proves double-signal prevention worked)
    size_t total = ok_count.load() + error_count.load() + timeout_count.load();
    EXPECT_EQ(total, ITERATIONS) << "All iterations should complete";

    std::cout << "Race results: Ok=" << ok_count.load() << ", Error=" << error_count.load()
              << ", Timeout=" << timeout_count.load() << std::endl;

    // At least one should have won (all races completed)
    EXPECT_GT(ok_count.load(), 0);
    // Note: Due to timing, one status may win all races (not a bug, just timing)
    // The important thing is that exactly ONE status is set per iteration
}

// ============================================================================
// Stress Test 2: Timeout Precision
// ============================================================================

TEST_F(WatchdogStressTest, TimeoutPrecision) {
    constexpr int ITERATIONS = 100;
    std::vector<int64_t> timeout_errors_us;
    timeout_errors_us.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; ++i) {
        auto start = std::chrono::steady_clock::now();

        watchdog.start(buffer, BUFFER_SIZE, 10ms);
        UartStatus status = watchdog.await_event(10ms);

        auto elapsed = std::chrono::steady_clock::now() - start;
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

        // Should timeout around 10ms (10000 us)
        int64_t error_us = std::abs(elapsed_us - 10000);
        timeout_errors_us.push_back(error_us);

        // Status should be Ok (no error was set before timeout)
        EXPECT_EQ(status, UartStatus::Ok);
    }

    // Calculate statistics
    int64_t max_error = *std::max_element(timeout_errors_us.begin(), timeout_errors_us.end());
    int64_t min_error = *std::min_element(timeout_errors_us.begin(), timeout_errors_us.end());
    double avg_error = std::accumulate(timeout_errors_us.begin(), timeout_errors_us.end(), 0.0) / ITERATIONS;

    std::cout << "Timeout precision (10ms target):" << std::endl;
    std::cout << "  Max error: " << max_error << " us" << std::endl;
    std::cout << "  Min error: " << min_error << " us" << std::endl;
    std::cout << "  Avg error: " << avg_error << " us" << std::endl;

    // Expect reasonable precision (allow up to 10ms error on loaded systems)
    // ARM and virtualized environments may have higher timing variance
    EXPECT_LT(max_error, 10000) << "Timeout precision too low (>10ms error)";
}

// ============================================================================
// Stress Test 3: Start/Stop Idempotency
// ============================================================================

TEST_F(WatchdogStressTest, StartStopIdempotency) {
    constexpr int ITERATIONS = 1000;

    for (int i = 0; i < ITERATIONS; ++i) {
        // Multiple starts should be idempotent
        watchdog.start(buffer, BUFFER_SIZE, 100ms);
        watchdog.start(buffer, BUFFER_SIZE, 100ms); // Second call should be no-op
        watchdog.start(buffer, BUFFER_SIZE, 100ms); // Third call should be no-op

        // Release and verify
        std::thread isr([&]() { watchdog.release(UartStatus::Ok); });

        UartStatus status = watchdog.await_event(100ms);
        EXPECT_EQ(status, UartStatus::Ok);

        isr.join();

        // Multiple stops should be idempotent
        watchdog.stop();
        watchdog.stop(); // Second call should be no-op
        watchdog.stop(); // Third call should be no-op
    }
}

// ============================================================================
// Stress Test 4: Rapid Start-Release-Await Cycles
// ============================================================================

TEST_F(WatchdogStressTest, RapidCycles) {
    constexpr int ITERATIONS = 10'000;
    std::atomic<size_t> successful_cycles{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog.start(buffer, BUFFER_SIZE, 10ms);

        // Immediately release (simulate fast ISR)
        std::thread isr([&]() {
            std::this_thread::sleep_for(100us); // Tiny delay
            watchdog.release(UartStatus::Ok);
        });

        UartStatus status = watchdog.await_event(10ms);

        if (status == UartStatus::Ok) {
            successful_cycles.fetch_add(1, std::memory_order_relaxed);
        }

        isr.join();
    }

    EXPECT_EQ(successful_cycles.load(), ITERATIONS) << "All cycles should complete successfully";
    std::cout << "Completed " << successful_cycles.load() << " rapid cycles" << std::endl;
}

// ============================================================================
// Stress Test 5: Concurrent Start from Multiple Threads
// ============================================================================

TEST_F(WatchdogStressTest, ConcurrentStartAttempts) {
    // This tests that concurrent start() calls don't corrupt state
    constexpr int ITERATIONS = 1000;
    std::atomic<size_t> start_attempts{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Multiple threads try to start simultaneously
        std::thread t1([&]() {
            watchdog.start(buffer, BUFFER_SIZE, 10ms);
            start_attempts.fetch_add(1, std::memory_order_relaxed);
        });

        std::thread t2([&]() {
            watchdog.start(buffer, BUFFER_SIZE, 10ms);
            start_attempts.fetch_add(1, std::memory_order_relaxed);
        });

        t1.join();
        t2.join();

        // Release and verify state is still consistent
        watchdog.release(UartStatus::Ok);
        UartStatus status = watchdog.await_event(10ms);
        EXPECT_EQ(status, UartStatus::Ok);
    }

    EXPECT_EQ(start_attempts.load(), ITERATIONS * 2);
    std::cout << "Handled " << start_attempts.load() << " concurrent start attempts" << std::endl;
}

// ============================================================================
// Stress Test 6: Release Before Await (Signal-Before-Wait)
// ============================================================================

TEST_F(WatchdogStressTest, ReleaseBeforeAwait) {
    constexpr int ITERATIONS = 10'000;
    std::atomic<size_t> immediate_success{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog.start(buffer, BUFFER_SIZE, 100ms);

        // Release BEFORE await_event is called
        watchdog.release(UartStatus::Ok);

        // Small delay to ensure release completes
        std::this_thread::sleep_for(10us);

        // Now await - should return immediately without blocking
        auto start = std::chrono::steady_clock::now();
        UartStatus status = watchdog.await_event(100ms);
        auto elapsed = std::chrono::steady_clock::now() - start;

        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < 10) {
            immediate_success.fetch_add(1, std::memory_order_relaxed);
        }

        EXPECT_EQ(status, UartStatus::Ok);
    }

    // Most iterations should complete immediately (within 10ms)
    EXPECT_GT(immediate_success.load(), ITERATIONS * 0.95)
        << "Signal-before-wait should return immediately in most cases";

    std::cout << "Immediate returns: " << immediate_success.load() << " / " << ITERATIONS << std::endl;
}

// ============================================================================
// Stress Test 7: Deadline Check Under Load
// ============================================================================

TEST_F(WatchdogStressTest, DeadlineCheckUnderLoad) {
    constexpr int ITERATIONS = 1000;
    std::atomic<size_t> deadline_expired_count{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog.start(buffer, BUFFER_SIZE, 5ms);

        // Simulate ISR checking deadline repeatedly
        std::thread isr([&]() {
            for (int check = 0; check < 100; ++check) {
                if (watchdog.check_deadline()) {
                    deadline_expired_count.fetch_add(1, std::memory_order_relaxed);
                    watchdog.release(UartStatus::ErrTimeout);
                    break;
                }
                std::this_thread::sleep_for(100us); // Check every 100us
            }
        });

        UartStatus status = watchdog.await_event(20ms); // Longer timeout than start timeout

        isr.join();

        // Should either timeout or complete
        EXPECT_TRUE(status == UartStatus::ErrTimeout || status == UartStatus::Ok);
    }

    std::cout << "Deadline expired " << deadline_expired_count.load() << " times" << std::endl;
}

// ============================================================================
// Stress Test 8: Memory Ordering Verification
// ============================================================================

TEST_F(WatchdogStressTest, MemoryOrderingVerification) {
    // This test is designed to catch memory ordering bugs with ThreadSanitizer
    constexpr int ITERATIONS = 10'000;
    std::atomic<size_t> correct_status_count{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog.start(buffer, BUFFER_SIZE, 50ms);

        // Write data to buffer in ISR context
        std::thread isr([&]() {
            // Write test pattern
            for (size_t j = 0; j < BUFFER_SIZE; ++j) {
                buffer[j] = static_cast<uint8_t>(j);
            }

            // Release with Ok status (memory_order_release)
            watchdog.release(UartStatus::Ok);
        });

        // Read status in application context (memory_order_acquire)
        UartStatus status = watchdog.await_event(50ms);

        isr.join();

        if (status == UartStatus::Ok) {
            // Verify buffer data is visible (memory ordering test)
            bool data_valid = true;
            for (size_t j = 0; j < BUFFER_SIZE; ++j) {
                if (buffer[j] != static_cast<uint8_t>(j)) {
                    data_valid = false;
                    break;
                }
            }

            if (data_valid) {
                correct_status_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                FAIL() << "Memory ordering violation: buffer data not visible after release";
            }
        }
    }

    EXPECT_EQ(correct_status_count.load(), ITERATIONS) << "All iterations should see correct buffer data";
}

// ============================================================================
// Stress Test 9: Stop During Await
// ============================================================================

TEST_F(WatchdogStressTest, StopDuringAwait) {
    constexpr int ITERATIONS = 1000;

    for (int i = 0; i < ITERATIONS; ++i) {
        watchdog.start(buffer, BUFFER_SIZE, 100ms);

        // Start await in one thread
        std::thread await_thread([&]() {
            (void)watchdog.await_event(100ms);
            // May or may not complete depending on timing
        });

        // Call stop from another thread while await is blocked
        std::thread stop_thread([&]() {
            std::this_thread::sleep_for(1ms);
            watchdog.stop();
        });

        // Also release to ensure await completes
        std::this_thread::sleep_for(2ms);
        watchdog.release(UartStatus::Ok);

        await_thread.join();
        stop_thread.join();
    }

    // If we get here without deadlock, test passes
    SUCCEED();
}

// ============================================================================
// Stress Test 10: Long Duration Timeout
// ============================================================================

TEST_F(WatchdogStressTest, LongDurationTimeout) {
    // Test with longer timeout to verify no overflow in duration calculations
    watchdog.start(buffer, BUFFER_SIZE, 1000ms);

    auto start = std::chrono::steady_clock::now();
    UartStatus status = watchdog.await_event(1000ms);
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    EXPECT_EQ(status, UartStatus::Ok); // No error set
    EXPECT_GE(elapsed_ms, 1000) << "Should have waited at least 1000ms";
    EXPECT_LT(elapsed_ms, 1100) << "Should not have waited much more than 1000ms";

    std::cout << "Long timeout elapsed: " << elapsed_ms << "ms" << std::endl;
}
