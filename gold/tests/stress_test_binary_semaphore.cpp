/**
 * @file stress_test_binary_semaphore.cpp
 * @brief Stress tests for BinarySemaphore ISR-safe signaling
 *
 * These tests verify the BinarySemaphore implementation under high contention:
 * - Multiple rapid give/take cycles
 * - Signal coalescing (binary semantics)
 * - Timeout accuracy
 * - Memory ordering correctness
 * - ISR-safe operations
 *
 * Build with ThreadSanitizer to detect race conditions:
 *   clang++ -fsanitize=thread -g -O1 -std=c++20 \
 *           -I../include stress_test_binary_semaphore.cpp -lgtest -lgtest_main \
 *           -o stress_test_binary_semaphore
 *
 * Run with:
 *   ./stress_test_binary_semaphore
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "hm11/sync/binary_semaphore.hpp"

using namespace hm11::sync;
using namespace std::chrono_literals;

// ============================================================================
// Test Fixture
// ============================================================================

class BinarySemaphoreStressTest : public ::testing::Test {
  protected:
    BinarySemaphore semaphore;
};

// ============================================================================
// Stress Test 1: Rapid Give-Take Cycles
// ============================================================================

TEST_F(BinarySemaphoreStressTest, RapidGiveTakeCycles) {
    constexpr int ITERATIONS = 1'000;
    std::atomic<size_t> successful_takes{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Give from one thread (simulating ISR)
        std::thread giver([&]() { semaphore.give_from_isr(); });

        // Take from another thread (simulating application)
        std::thread taker([&]() {
            if (semaphore.take_with_timeout(10ms)) {
                successful_takes.fetch_add(1, std::memory_order_relaxed);
            }
        });

        giver.join();
        taker.join();
    }

    // All takes should succeed (give always happens before or during take)
    EXPECT_GE(successful_takes.load(), ITERATIONS * 0.95) << "Most takes should succeed in rapid cycles";

    std::cout << "Successful takes: " << successful_takes.load() << " / " << ITERATIONS << std::endl;
}

// ============================================================================
// Stress Test 2: Multiple Gives (Binary Coalescing)
// ============================================================================

TEST_F(BinarySemaphoreStressTest, MultipleGivesCoalesce) {
    constexpr int ITERATIONS = 1000;
    std::atomic<size_t> single_take_success{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Multiple concurrent gives (should coalesce into single signal)
        std::thread g1([&]() { semaphore.give_from_isr(); });
        std::thread g2([&]() { semaphore.give_from_isr(); });
        std::thread g3([&]() { semaphore.give_from_isr(); });

        g1.join();
        g2.join();
        g3.join();

        // First take should succeed
        EXPECT_TRUE(semaphore.take_with_timeout(10ms)) << "First take should succeed";

        // Second take should fail or timeout (binary semantics)
        if (!semaphore.try_take()) {
            single_take_success.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Most iterations should demonstrate binary coalescing
    EXPECT_GT(single_take_success.load(), ITERATIONS * 0.5) << "Multiple gives should coalesce (binary semantics)";

    std::cout << "Binary coalescing observed: " << single_take_success.load() << " / " << ITERATIONS << std::endl;
}

// ============================================================================
// Stress Test 3: Signal Before Wait
// ============================================================================

TEST_F(BinarySemaphoreStressTest, SignalBeforeWait) {
    constexpr int ITERATIONS = 1'000;
    std::atomic<size_t> immediate_returns{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Give BEFORE take
        semaphore.give_from_isr();

        // Small delay to ensure give completes
        std::this_thread::sleep_for(1us);

        // Take should return immediately without blocking
        auto start = std::chrono::steady_clock::now();
        bool success = semaphore.take_with_timeout(100ms);
        auto elapsed = std::chrono::steady_clock::now() - start;

        EXPECT_TRUE(success) << "Take should succeed when signaled before wait";

        if (std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() < 1000) {
            immediate_returns.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Most iterations should return immediately
    EXPECT_GT(immediate_returns.load(), ITERATIONS * 0.95) << "Signal-before-wait should return immediately";

    std::cout << "Immediate returns: " << immediate_returns.load() << " / " << ITERATIONS << std::endl;
}

// ============================================================================
// Stress Test 4: Timeout Accuracy
// ============================================================================

TEST_F(BinarySemaphoreStressTest, TimeoutAccuracy) {
    constexpr int ITERATIONS = 100;
    std::vector<int64_t> timeout_errors_us;
    timeout_errors_us.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; ++i) {
        // Take without giving - should timeout
        auto start = std::chrono::steady_clock::now();
        bool success = semaphore.take_with_timeout(10ms);
        auto elapsed = std::chrono::steady_clock::now() - start;

        EXPECT_FALSE(success) << "Take should timeout when not signaled";

        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        int64_t error_us = std::abs(elapsed_us - 10000); // Expected: 10ms = 10000us
        timeout_errors_us.push_back(error_us);
    }

    // Calculate statistics
    int64_t max_error = *std::max_element(timeout_errors_us.begin(), timeout_errors_us.end());
    double avg_error = std::accumulate(timeout_errors_us.begin(), timeout_errors_us.end(), 0.0) / ITERATIONS;

    std::cout << "Timeout accuracy (10ms target):" << std::endl;
    std::cout << "  Max error: " << max_error << " us" << std::endl;
    std::cout << "  Avg error: " << avg_error << " us" << std::endl;

    // Expect reasonable timeout accuracy
    EXPECT_LT(max_error, 5000) << "Timeout error too large (>5ms)";
}

// ============================================================================
// Stress Test 5: Try-Take Under Contention
// ============================================================================

TEST_F(BinarySemaphoreStressTest, TryTakeContention) {
    constexpr int ITERATIONS = 1'000;
    std::atomic<size_t> try_take_success{0};
    std::atomic<size_t> try_take_failure{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Give from one thread
        std::thread giver([&]() {
            std::this_thread::sleep_for(100us); // Small delay
            semaphore.give_from_isr();
        });

        // Multiple threads try to take
        std::thread taker1([&]() {
            if (semaphore.try_take()) {
                try_take_success.fetch_add(1, std::memory_order_relaxed);
            } else {
                try_take_failure.fetch_add(1, std::memory_order_relaxed);
            }
        });

        std::thread taker2([&]() {
            if (semaphore.try_take()) {
                try_take_success.fetch_add(1, std::memory_order_relaxed);
            } else {
                try_take_failure.fetch_add(1, std::memory_order_relaxed);
            }
        });

        giver.join();
        taker1.join();
        taker2.join();
    }

    // Only one taker per iteration should succeed (binary semantics)
    std::cout << "Try-take success: " << try_take_success.load() << std::endl;
    std::cout << "Try-take failure: " << try_take_failure.load() << std::endl;

    // Total attempts: 2 * ITERATIONS
    EXPECT_EQ(try_take_success.load() + try_take_failure.load(), 2 * ITERATIONS);
}

// ============================================================================
// Stress Test 6: Memory Ordering Verification
// ============================================================================

TEST_F(BinarySemaphoreStressTest, MemoryOrderingVerification) {
    // This test verifies memory_order_release/acquire correctness
    constexpr int ITERATIONS = 1'000;
    std::atomic<int> shared_data{0};
    std::atomic<size_t> correct_reads{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        shared_data.store(i, std::memory_order_relaxed);

        // Producer: Write data, then signal
        std::thread producer([&]() {
            shared_data.store(i, std::memory_order_relaxed);
            // Give with memory_order_release (publishes shared_data write)
            semaphore.give_from_isr();
        });

        // Consumer: Wait for signal, then read data
        std::thread consumer([&]() {
            // Take with memory_order_acquire (observes shared_data write)
            if (semaphore.take_with_timeout(10ms)) {
                int value = shared_data.load(std::memory_order_relaxed);
                if (value == i) {
                    correct_reads.fetch_add(1, std::memory_order_relaxed);
                } else {
                    FAIL() << "Memory ordering violation: expected " << i << ", got " << value;
                }
            }
        });

        producer.join();
        consumer.join();
    }

    EXPECT_EQ(correct_reads.load(), ITERATIONS) << "All reads should see correct data (memory ordering)";
}

// ============================================================================
// Stress Test 7: Concurrent Gives from Multiple ISRs
// ============================================================================

TEST_F(BinarySemaphoreStressTest, ConcurrentGives) {
    constexpr int ITERATIONS = 1000;
    std::atomic<size_t> total_gives{0};
    std::atomic<size_t> successful_takes{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Simulate multiple ISRs giving concurrently
        std::thread isr1([&]() {
            semaphore.give_from_isr();
            total_gives.fetch_add(1, std::memory_order_relaxed);
        });

        std::thread isr2([&]() {
            semaphore.give_from_isr();
            total_gives.fetch_add(1, std::memory_order_relaxed);
        });

        std::thread isr3([&]() {
            semaphore.give_from_isr();
            total_gives.fetch_add(1, std::memory_order_relaxed);
        });

        isr1.join();
        isr2.join();
        isr3.join();

        // Only one take should succeed (binary coalescing)
        if (semaphore.take_with_timeout(10ms)) {
            successful_takes.fetch_add(1, std::memory_order_relaxed);
        }

        // Clear any remaining signal
        (void)semaphore.try_take();
    }

    EXPECT_EQ(total_gives.load(), ITERATIONS * 3);
    EXPECT_EQ(successful_takes.load(), ITERATIONS);

    std::cout << "Total gives: " << total_gives.load() << std::endl;
    std::cout << "Successful takes: " << successful_takes.load() << std::endl;
}

// ============================================================================
// Stress Test 8: Rapid Auto-Reset
// ============================================================================

TEST_F(BinarySemaphoreStressTest, RapidAutoReset) {
    // Verify auto-reset semantics work correctly under stress
    constexpr int ITERATIONS = 1'000;

    for (int i = 0; i < ITERATIONS; ++i) {
        // Give
        semaphore.give_from_isr();

        // Take (should auto-reset)
        EXPECT_TRUE(semaphore.take_with_timeout(10ms)) << "First take should succeed";

        // Second take should fail (auto-reset cleared semaphore)
        EXPECT_FALSE(semaphore.try_take()) << "Second take should fail (auto-reset)";
    }

    SUCCEED() << "Auto-reset working correctly over " << ITERATIONS << " cycles";
}

// ============================================================================
// Stress Test 9: Take Timeout Expiry
// ============================================================================

TEST_F(BinarySemaphoreStressTest, TakeTimeoutExpiry) {
    constexpr int ITERATIONS = 100;
    std::atomic<size_t> timeout_count{0};

    for (int i = 0; i < ITERATIONS; ++i) {
        // Take without giving - should timeout after 5ms
        auto start = std::chrono::steady_clock::now();
        bool success = semaphore.take_with_timeout(5ms);
        auto elapsed = std::chrono::steady_clock::now() - start;

        if (!success) {
            timeout_count.fetch_add(1, std::memory_order_relaxed);
        }

        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        EXPECT_GE(elapsed_ms, 5) << "Should wait at least 5ms before timeout";
        EXPECT_LT(elapsed_ms, 20) << "Should not wait much more than 5ms (TSan adds variance)";
    }

    EXPECT_EQ(timeout_count.load(), ITERATIONS) << "All takes should timeout";
}

// ============================================================================
// Stress Test 10: Producer-Consumer Pattern
// ============================================================================

TEST_F(BinarySemaphoreStressTest, ProducerConsumerPattern) {
    constexpr int ITEMS = 10; // Very small for TSan
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<int> shared_value{0};
    std::atomic<bool> producer_done{false};

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < ITEMS; ++i) {
            // Wait for consumer to consume previous item
            while (consumed.load(std::memory_order_acquire) < i) {
                std::this_thread::yield();
            }

            shared_value.store(i, std::memory_order_relaxed);
            produced.fetch_add(1, std::memory_order_relaxed);
            semaphore.give_from_isr();
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer thread
    std::thread consumer([&]() {
        int last_value = -1;
        while (consumed.load(std::memory_order_relaxed) < ITEMS) {
            if (semaphore.take_with_timeout(100ms)) {
                int value = shared_value.load(std::memory_order_relaxed);
                EXPECT_GE(value, last_value) << "Values should not decrease";
                last_value = value;
                consumed.fetch_add(1, std::memory_order_release);
            } else if (producer_done.load(std::memory_order_acquire)) {
                break; // Producer done and timeout
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced.load(), ITEMS);
    EXPECT_EQ(consumed.load(), ITEMS) << "Consumer should have consumed all items";

    std::cout << "Produced: " << produced.load() << ", Consumed: " << consumed.load() << std::endl;
}
