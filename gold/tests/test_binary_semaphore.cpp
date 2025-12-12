/**
 * @file test_binary_semaphore.cpp
 * @brief Unit tests for BinarySemaphore ISR-to-thread signaling primitive
 *
 * This test suite validates the BinarySemaphore implementation. Tests cover
 * basic functionality, timeout behavior, signal-before-wait scenarios,
 * concurrent ISR signaling, and memory ordering.
 *
 * ## Test Strategy (Decision 21)
 *
 * Uses mock-based unit tests with std::thread to simulate interrupt context
 * on x86 Linux. This provides ~60% confidence in ISR safety (Phase 1 testing).
 * Later phases will add QEMU ARM emulation (~85% confidence) and hardware
 * testing (~99% confidence).
 *
 * ## ThreadSanitizer Validation
 *
 * All tests should pass with -fsanitize=thread to verify:
 * - No data races in atomic operations
 * - Proper memory ordering (release/acquire)
 * - Correct synchronization patterns
 *
 * Build with: cmake -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <hm11/sync/binary_semaphore.hpp>
#include <thread>

using namespace std::chrono_literals;

namespace hm11::sync::test {

/**
 * @brief Test fixture for BinarySemaphore tests
 *
 * Provides common setup and utilities for testing semaphore behavior.
 */
class BinarySemaphoreTest : public ::testing::Test {
  protected:
    BinarySemaphore semaphore;
};

/**
 * @brief Test basic give/take functionality
 *
 * Verifies that:
 * - take_with_timeout() blocks until give_from_isr() is called
 * - Signal is properly delivered from simulated ISR to thread
 * - Return value indicates successful take
 */
TEST_F(BinarySemaphoreTest, BasicGiveTake) {
    // Flag to verify ISR thread executed
    std::atomic<bool> isr_executed{false};

    // Simulate ISR signaling after short delay
    std::thread simulated_isr([&] {
        std::this_thread::sleep_for(50ms);
        isr_executed.store(true, std::memory_order_release);
        semaphore.give_from_isr();
    });

    // Should block until ISR signals
    const bool TAKEN = semaphore.take_with_timeout(1s);

    // Verify successful take
    EXPECT_TRUE(TAKEN);

    // Verify ISR executed before we woke up
    EXPECT_TRUE(isr_executed.load(std::memory_order_acquire));

    simulated_isr.join();
}

/**
 * @brief Test timeout behavior when no signal arrives
 *
 * Verifies that:
 * - take_with_timeout() returns false when timeout expires
 * - Thread unblocks after timeout period
 * - No signal was received
 */
TEST_F(BinarySemaphoreTest, TimeoutNoSignal) {
    const auto START = std::chrono::steady_clock::now();

    // No ISR thread - should timeout
    const bool TAKEN = semaphore.take_with_timeout(100ms);

    const auto ELAPSED = std::chrono::steady_clock::now() - START;

    // Should return false (timeout)
    EXPECT_FALSE(TAKEN);

    // Should have waited approximately the timeout duration
    // Allow 50ms tolerance for scheduling and system overhead
    EXPECT_GE(ELAPSED, 100ms);
    EXPECT_LE(ELAPSED, 150ms);
}

/**
 * @brief Test successful take before timeout expires
 *
 * Verifies that:
 * - take_with_timeout() returns true when signal arrives before timeout
 * - Thread wakes up promptly after signal
 * - Actual wait time is less than timeout
 */
TEST_F(BinarySemaphoreTest, SignalBeforeTimeout) {
    const auto START = std::chrono::steady_clock::now();

    // Simulate ISR signaling after short delay (well before timeout)
    std::thread simulated_isr([&] {
        std::this_thread::sleep_for(50ms);
        semaphore.give_from_isr();
    });

    // Should receive signal before timeout
    const bool TAKEN = semaphore.take_with_timeout(5s);

    const auto ELAPSED = std::chrono::steady_clock::now() - START;

    // Should return true (signaled)
    EXPECT_TRUE(TAKEN);

    // Should have waited approximately 50ms (not full timeout)
    // Allow 50ms tolerance for scheduling overhead
    EXPECT_GE(ELAPSED, 50ms);
    EXPECT_LE(ELAPSED, 150ms);

    simulated_isr.join();
}

/**
 * @brief Test signal-before-wait behavior (give before take)
 *
 * Verifies that:
 * - give_from_isr() called before take_with_timeout() stores the signal
 * - Subsequent take_with_timeout() returns immediately without blocking
 * - No race conditions in signal-before-wait scenario
 */
TEST_F(BinarySemaphoreTest, GiveBeforeTake) {
    // Signal semaphore before any wait
    semaphore.give_from_isr();

    const auto START = std::chrono::steady_clock::now();

    // Should succeed immediately without blocking
    const bool TAKEN = semaphore.take_with_timeout(1s);

    const auto ELAPSED = std::chrono::steady_clock::now() - START;

    // Should return true immediately
    EXPECT_TRUE(TAKEN);

    // Should complete very quickly (within 10ms)
    EXPECT_LT(ELAPSED, 10ms);
}

/**
 * @brief Test multiple give/take cycles (reusability)
 *
 * Verifies that:
 * - Semaphore can be reused for multiple signal/wait cycles
 * - Auto-reset behavior works correctly (semaphore clears after take)
 * - No state corruption across multiple uses
 */
TEST_F(BinarySemaphoreTest, MultipleGiveTakeCycles) {
    // Perform multiple cycles to verify reusability
    for (int cycle = 0; cycle < 5; ++cycle) {
        std::thread simulated_isr([&] {
            std::this_thread::sleep_for(20ms);
            semaphore.give_from_isr();
        });

        const bool TAKEN = semaphore.take_with_timeout(1s);

        EXPECT_TRUE(TAKEN) << "Cycle " << cycle << " failed";

        simulated_isr.join();
    }
}

/**
 * @brief Test multiple Give calls before Take (binary semantics)
 *
 * Verifies that:
 * - Multiple give_from_isr() calls coalesce into single signal
 * - Only one take_with_timeout() succeeds
 * - Subsequent take_with_timeout() blocks (semaphore properly cleared)
 */
TEST_F(BinarySemaphoreTest, MultipleGivesCoalesce) {
    // Give multiple times
    semaphore.give_from_isr();
    semaphore.give_from_isr();
    semaphore.give_from_isr();

    // First take should succeed immediately
    EXPECT_TRUE(semaphore.take_with_timeout(10ms));

    // Second take should timeout (semaphore cleared after first take)
    EXPECT_FALSE(semaphore.take_with_timeout(50ms));
}

/**
 * @brief Test try_take() non-blocking operation
 *
 * Verifies that:
 * - try_take() returns false when not signaled (doesn't block)
 * - try_take() returns true when signaled
 * - try_take() clears semaphore on success
 */
TEST_F(BinarySemaphoreTest, try_takeNonBlocking) {
    // Should return false immediately (not signaled)
    EXPECT_FALSE(semaphore.try_take());

    // Signal semaphore
    semaphore.give_from_isr();

    // Should return true immediately (signaled)
    EXPECT_TRUE(semaphore.try_take());

    // Should return false again (cleared after successful take)
    EXPECT_FALSE(semaphore.try_take());
}

/**
 * @brief Test concurrent Give from simulated ISR while thread waits
 *
 * Verifies that:
 * - give_from_isr() can be called concurrently with take_with_timeout()
 * - Thread wakes up correctly when ISR signals
 * - No race conditions between producer (ISR) and consumer (thread)
 *
 * This test is critical for ThreadSanitizer validation of memory ordering.
 */
TEST_F(BinarySemaphoreTest, ConcurrentISRSignaling) {
    std::atomic<int> isr_call_count{0};

    // Simulate ISR being called multiple times (only first should signal)
    std::thread simulated_isr1([&] {
        std::this_thread::sleep_for(30ms);
        semaphore.give_from_isr();
        isr_call_count.fetch_add(1, std::memory_order_release);
    });

    std::thread simulated_isr2([&] {
        std::this_thread::sleep_for(40ms);
        semaphore.give_from_isr(); // Should be idempotent
        isr_call_count.fetch_add(1, std::memory_order_release);
    });

    // Should wake up after first ISR signals
    const bool TAKEN = semaphore.take_with_timeout(1s);

    EXPECT_TRUE(TAKEN);

    simulated_isr1.join();
    simulated_isr2.join();

    // Both ISR threads should have executed
    EXPECT_EQ(isr_call_count.load(std::memory_order_acquire), 2);
}

/**
 * @brief Test memory ordering: ISR writes visible after Take
 *
 * Verifies that:
 * - Memory operations before give_from_isr() are visible after take_with_timeout()
 * - Release-acquire semantics provide happens-before relationship
 * - Data written by ISR is correctly observed by thread
 *
 * This test validates Decision 23 (memory ordering strategy).
 */
TEST_F(BinarySemaphoreTest, MemoryOrderingVisibility) {
    // Shared data modified by simulated ISR
    std::atomic<int> shared_counter{0};
    std::atomic<bool> shared_flag{false};

    std::thread simulated_isr([&] {
        std::this_thread::sleep_for(30ms);

        // ISR writes to shared memory
        // These stores use release ordering to synchronize with semaphore
        shared_counter.store(42, std::memory_order_release);
        shared_flag.store(true, std::memory_order_release);

        // Signal completion - publishes above writes
        semaphore.give_from_isr();
    });

    // Wait for ISR signal
    const bool TAKEN = semaphore.take_with_timeout(1s);
    EXPECT_TRUE(TAKEN);

    // After successful take, ISR writes must be visible
    // Semaphore's acquire ensures happens-before relationship
    EXPECT_EQ(shared_counter.load(std::memory_order_acquire), 42);
    EXPECT_TRUE(shared_flag.load(std::memory_order_acquire));

    simulated_isr.join();
}

/**
 * @brief Test rapid give/take cycles (stress test)
 *
 * Verifies that:
 * - Semaphore handles rapid signaling without corruption
 * - No race conditions under stress
 * - Consistent behavior across many iterations
 */
TEST_F(BinarySemaphoreTest, RapidGiveTakeCycles) {
    constexpr int ITERATIONS = 100;
    int successful_takes = 0;

    for (int i = 0; i < ITERATIONS; ++i) {
        // Rapid give/take without delay
        semaphore.give_from_isr();

        if (semaphore.take_with_timeout(100ms)) {
            ++successful_takes;
        }
    }

    // All takes should succeed (signaled before each take)
    EXPECT_EQ(successful_takes, ITERATIONS);
}

/**
 * @brief Test timeout precision
 *
 * Verifies that:
 * - Timeout duration is reasonably accurate
 * - Thread doesn't wake up significantly earlier than timeout
 * - Thread wakes up within reasonable tolerance after timeout
 */
TEST_F(BinarySemaphoreTest, TimeoutPrecision) {
    constexpr auto TIMEOUT_DURATION = 200ms;
    const auto START = std::chrono::steady_clock::now();

    const bool TAKEN = semaphore.take_with_timeout(TIMEOUT_DURATION);

    const auto ELAPSED = std::chrono::steady_clock::now() - START;

    EXPECT_FALSE(TAKEN);

    // Should not wake significantly early (allow 10ms early tolerance)
    EXPECT_GE(ELAPSED, TIMEOUT_DURATION - 10ms);

    // Should not wake significantly late (allow 50ms late tolerance for scheduling)
    EXPECT_LE(ELAPSED, TIMEOUT_DURATION + 50ms);
}

/**
 * @brief Test zero timeout behavior
 *
 * Verifies that:
 * - Zero timeout is handled correctly
 * - Returns immediately if not signaled
 * - Returns immediately with true if already signaled
 */
TEST_F(BinarySemaphoreTest, ZeroTimeout) {
    // Not signaled - should return false immediately
    const auto START1 = std::chrono::steady_clock::now();
    const bool TAKEN1 = semaphore.take_with_timeout(0ms);
    const auto ELAPSED1 = std::chrono::steady_clock::now() - START1;

    EXPECT_FALSE(TAKEN1);
    EXPECT_LT(ELAPSED1, 10ms); // Should be very fast

    // Signal then take with zero timeout
    semaphore.give_from_isr();

    const auto START2 = std::chrono::steady_clock::now();
    const bool TAKEN2 = semaphore.take_with_timeout(0ms);
    const auto ELAPSED2 = std::chrono::steady_clock::now() - START2;

    EXPECT_TRUE(TAKEN2);
    EXPECT_LT(ELAPSED2, 10ms); // Should be very fast
}

/**
 * @brief Test try_take after timeout
 *
 * Verifies that:
 * - After timeout, semaphore is in correct state
 * - try_take correctly reports not signaled after timeout
 * - Subsequent Give/Take cycle works correctly
 */
TEST_F(BinarySemaphoreTest, try_takeAfterTimeout) {
    // Timeout without signal
    EXPECT_FALSE(semaphore.take_with_timeout(50ms));

    // try_take should still return false
    EXPECT_FALSE(semaphore.try_take());

    // Now signal and verify try_take succeeds
    semaphore.give_from_isr();
    EXPECT_TRUE(semaphore.try_take());
}

/**
 * @brief Test ISR signaling during timeout check
 *
 * Verifies that:
 * - Signal arriving just before timeout is handled correctly
 * - No race between timeout check and signal delivery
 * - Thread wakes up even if signal arrives late
 */
TEST_F(BinarySemaphoreTest, SignalDuringTimeoutWindow) {
    // Signal arrives near the timeout boundary
    std::thread simulated_isr([&] {
        // Sleep for most of timeout duration
        std::this_thread::sleep_for(80ms);
        semaphore.give_from_isr();
    });

    // Should succeed (signal arrives before timeout)
    const bool TAKEN = semaphore.take_with_timeout(100ms);

    EXPECT_TRUE(TAKEN);

    simulated_isr.join();
}

} // namespace hm11::sync::test
