// HM11 Synchronization Primitives - Binary Semaphore Unit Tests
// Comprehensive testing with ThreadSanitizer validation for ISR-safe operations

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "hm11/sync/binary_semaphore.hpp"

using namespace hm11::sync;
using namespace hm11::hal;

class BinarySemaphoreTest : public ::testing::Test {
  protected:
    BinarySemaphore semaphore_;
};

/// Test basic signal/wait operation
TEST_F(BinarySemaphoreTest, BasicSignalWait) {
    // Initially not signaled
    EXPECT_FALSE(semaphore_.IsSignaled());

    // Signal from "ISR" context
    semaphore_.GiveFromISR();

    // Should be signaled
    EXPECT_TRUE(semaphore_.IsSignaled());

    // Take should return immediately with Ok status
    auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(100));
    EXPECT_EQ(status, UARTStatus::Ok);

    // Should be reset after take
    EXPECT_FALSE(semaphore_.IsSignaled());
}

/// Test signal with error status propagation
TEST_F(BinarySemaphoreTest, SignalWithErrorStatus) {
    // Signal with error status from "ISR"
    semaphore_.GiveFromISRWithStatus(UARTStatus::ErrError);

    // Take should return the error status
    auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(100));
    EXPECT_EQ(status, UARTStatus::ErrError);

    // Should be reset after take
    EXPECT_FALSE(semaphore_.IsSignaled());
}

/// Test timeout behavior when no signal occurs
TEST_F(BinarySemaphoreTest, TimeoutBehavior) {
    // Take with timeout when not signaled
    auto start = std::chrono::steady_clock::now();
    auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(100));
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(status, UARTStatus::ErrTimeout);
    EXPECT_GE(elapsed, std::chrono::milliseconds(90));  // Allow some tolerance
    EXPECT_LE(elapsed, std::chrono::milliseconds(150)); // But not too much
}

/// Test concurrent signal from mock ISR thread
TEST_F(BinarySemaphoreTest, ConcurrentISRSignaling) {
    std::atomic<bool> isr_should_signal{false};
    std::atomic<bool> isr_finished{false};

    // Mock ISR thread that waits for signal to fire
    std::thread mock_isr([&]() {
        while (!isr_should_signal.load()) {
            std::this_thread::yield();
        }

        // Simulate ISR delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Signal completion
        semaphore_.GiveFromISR();
        isr_finished.store(true);
    });

    // Start waiting before ISR signals
    auto wait_future = std::async(std::launch::async, [&]() {
        return semaphore_.TakeWithTimeout(std::chrono::milliseconds(200));
    });

    // Small delay to ensure wait starts
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Trigger "ISR" to signal
    isr_should_signal.store(true);

    // Wait should complete successfully
    auto status = wait_future.get();
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(isr_finished.load());

    mock_isr.join();
}

/// Test multiple waiters (should only wake one - binary semaphore)
TEST_F(BinarySemaphoreTest, SingleWakeup) {
    std::atomic<int> wakeup_count{0};
    std::vector<std::future<UARTStatus>> wait_futures;

    // Start multiple waiters
    for (int i = 0; i < 3; ++i) {
        wait_futures.push_back(std::async(std::launch::async, [&]() {
            auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(200));
            if (status == UARTStatus::Ok) {
                wakeup_count.fetch_add(1);
            }
            return status;
        }));
    }

    // Small delay to ensure all waiters start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Signal once
    semaphore_.GiveFromISR();

    // Wait for all futures
    std::vector<UARTStatus> results;
    for (auto& future : wait_futures) {
        results.push_back(future.get());
    }

    // Only one waiter should have been woken up with Ok status
    int ok_count = std::count(results.begin(), results.end(), UARTStatus::Ok);
    int timeout_count = std::count(results.begin(), results.end(), UARTStatus::ErrTimeout);

    EXPECT_EQ(ok_count, 1);            // Exactly one successful wake
    EXPECT_EQ(timeout_count, 2);       // Two timeouts
    EXPECT_EQ(wakeup_count.load(), 1); // Atomic counter confirms single wakeup
}

/// Test rapid signal/reset cycles (stress test for race conditions)
TEST_F(BinarySemaphoreTest, RapidSignalResetCycles) {
    constexpr int ITERATIONS = 1000;
    std::atomic<int> successful_takes{0};
    std::atomic<bool> should_stop{false};

    // Producer thread (mock ISR)
    std::thread producer([&]() {
        for (int i = 0; i < ITERATIONS && !should_stop.load(); ++i) {
            semaphore_.GiveFromISR();

            // Small random delay to create race conditions
            if (i % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < ITERATIONS && !should_stop.load(); ++i) {
            auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(10));
            if (status == UARTStatus::Ok) {
                successful_takes.fetch_add(1);
            } else if (status == UARTStatus::ErrTimeout) {
                // Acceptable - might timeout due to timing
                continue;
            } else {
                // Unexpected error
                should_stop.store(true);
                break;
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_FALSE(should_stop.load()); // No unexpected errors occurred
    EXPECT_GE(
        successful_takes.load(),
        ITERATIONS / 10); // At least 10% successful (realistic for binary semaphore under load)
    EXPECT_LT(successful_takes.load(), ITERATIONS); // Should not succeed 100% (would indicate race condition)
}

/// Test memory ordering correctness with data sharing
TEST_F(BinarySemaphoreTest, MemoryOrderingCorrectness) {
    constexpr int TEST_VALUE = 0x12345678;
    std::atomic<int> shared_data{0};

    // Producer writes data then signals
    std::thread producer([&]() {
        // Write data with release semantics
        shared_data.store(TEST_VALUE, std::memory_order_release);

        // Signal completion (BinarySemaphore uses release ordering)
        semaphore_.GiveFromISR();
    });

    // Consumer waits for signal then reads data
    std::thread consumer([&]() {
        // Wait for signal (BinarySemaphore uses acquire ordering)
        auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(1000));
        EXPECT_EQ(status, UARTStatus::Ok);

        // Read data with acquire semantics
        int value = shared_data.load(std::memory_order_acquire);
        EXPECT_EQ(value, TEST_VALUE);
    });

    producer.join();
    consumer.join();
}

/// Test reset operation
TEST_F(BinarySemaphoreTest, ResetOperation) {
    // Signal semaphore
    semaphore_.GiveFromISR();
    EXPECT_TRUE(semaphore_.IsSignaled());

    // Reset should clear signaled state
    semaphore_.Reset();
    EXPECT_FALSE(semaphore_.IsSignaled());

    // Take should timeout since reset cleared the signal
    auto status = semaphore_.TakeWithTimeout(std::chrono::milliseconds(10));
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

/// Benchmark test for ISR latency (informational)
TEST_F(BinarySemaphoreTest, ISRLatencyBenchmark) {
    constexpr int ITERATIONS = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        semaphore_.GiveFromISR();
        semaphore_.Reset();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto avg_ns_per_signal = total_ns / ITERATIONS;

    // Log performance information (not an assertion - just informational)
    std::cout << "Average ISR signal time: " << avg_ns_per_signal << " ns" << std::endl;

    // Sanity check - should be under 100µs even on debug builds
    EXPECT_LT(avg_ns_per_signal, 100000); // 100µs = 100,000ns
}