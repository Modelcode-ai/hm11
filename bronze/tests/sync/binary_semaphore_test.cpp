/**
 * @file binary_semaphore_test.cpp
 * @brief Unit tests for the binary semaphore implementation
 *
 * This file is part of the HM11 driver C++ migration project.
 * It tests the binary semaphore implementation.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

#include "hm11/sync/binary_semaphore.hpp"

// Test fixture for binary semaphore tests
class BinarySemaphoreTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test basic semaphore functionality
TEST_F(BinarySemaphoreTest, BasicFunctionality) {
    // Create a semaphore
    hm11::sync::BinarySemaphore sem(false);

    // Semaphore should not be signaled initially
    EXPECT_FALSE(sem.is_signaled());

    // Try to take the semaphore with a short timeout
    hm11::sync::UartStatus status;
    auto result = sem.take_with_timeout(status, std::chrono::milliseconds(10));
    EXPECT_FALSE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrTimeout);

    // Signal the semaphore
    sem.give_from_isr(hm11::sync::UartStatus::Ok);

    // Verify the semaphore is now signaled
    EXPECT_TRUE(sem.is_signaled());

    // Take the semaphore
    result = sem.take_with_timeout(status, std::chrono::milliseconds(10));
    EXPECT_TRUE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::Ok);

    // Semaphore should be reset after taking
    EXPECT_FALSE(sem.is_signaled());
}

// Test asynchronous signaling
TEST_F(BinarySemaphoreTest, AsyncSignaling) {
    hm11::sync::BinarySemaphore sem(false);
    hm11::sync::UartStatus status;

    // Spawn a thread that will signal the semaphore after a delay
    auto future = std::async(std::launch::async, [&sem]() {
        // Wait a bit before signaling
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // Signal with error status to test status passing
        sem.give(hm11::sync::UartStatus::ErrError);
        return true;
    });

    // Wait for the semaphore, this should succeed after the thread signals it
    bool result = sem.take_with_timeout(status, std::chrono::milliseconds(100));
    EXPECT_TRUE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrError);

    // Make sure the async thread completes
    future.wait();
}

// Test timeout behavior
TEST_F(BinarySemaphoreTest, TimeoutBehavior) {
    hm11::sync::BinarySemaphore sem(false);
    hm11::sync::UartStatus status;

    // Measure the actual time taken by a timeout
    auto start = std::chrono::steady_clock::now();
    bool result = sem.take_with_timeout(status, std::chrono::milliseconds(50));
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Verify timeout behavior
    EXPECT_FALSE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrTimeout);

    // Verify that we waited approximately the right amount of time
    // Allow some slack for thread scheduling
    EXPECT_GE(elapsed.count(), 45);
}

// Test reset functionality
TEST_F(BinarySemaphoreTest, ResetFunctionality) {
    hm11::sync::BinarySemaphore sem(false);

    // Signal the semaphore
    sem.give();
    EXPECT_TRUE(sem.is_signaled());

    // Reset the semaphore
    sem.reset();
    EXPECT_FALSE(sem.is_signaled());

    // Verify that taking now times out
    hm11::sync::UartStatus status;
    bool result = sem.take_with_timeout(status, std::chrono::milliseconds(10));
    EXPECT_FALSE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrTimeout);
}

// Test ISR-like behavior using threads
TEST_F(BinarySemaphoreTest, IsrSimulation) {
    hm11::sync::BinarySemaphore sem(false);
    hm11::sync::UartStatus status;

    // Launch a thread that will simulate an ISR
    std::thread isr_thread([&sem]() {
        // Simulate some ISR processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Signal the semaphore as if from an ISR
        sem.give_from_isr(hm11::sync::UartStatus::ErrError);
    });

    // Wait for the "ISR" to signal
    bool result = sem.take_with_timeout(status, std::chrono::milliseconds(100));
    EXPECT_TRUE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrError);

    // Ensure the ISR thread completes
    if (isr_thread.joinable()) {
        isr_thread.join();
    }
}

// Test multiple waiters behavior
TEST_F(BinarySemaphoreTest, MultipleWaiters) {
    hm11::sync::BinarySemaphore sem(false);

    // Launch two threads that will wait on the semaphore
    auto future1 = std::async(std::launch::async, [&sem]() {
        hm11::sync::UartStatus status;
        bool result = sem.take_with_timeout(status, std::chrono::milliseconds(100));
        return std::make_pair(result, status);
    });

    auto future2 = std::async(std::launch::async, [&sem]() {
        hm11::sync::UartStatus status;
        bool result = sem.take_with_timeout(status, std::chrono::milliseconds(100));
        return std::make_pair(result, status);
    });

    // Wait a bit to ensure both threads are waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Signal the semaphore once
    sem.give_from_isr();

    // Only one waiter should succeed, the other should timeout
    auto result1 = future1.get();
    auto result2 = future2.get();

    // At least one should succeed
    EXPECT_TRUE(result1.first || result2.first);

    // And at least one should fail with timeout
    EXPECT_TRUE(!result1.first || !result2.first);

    // The successful one should have OK status
    if (result1.first) {
        EXPECT_EQ(result1.second, hm11::sync::UartStatus::Ok);
    } else {
        EXPECT_EQ(result2.second, hm11::sync::UartStatus::Ok);
    }

    // The failed one should have timeout status
    if (!result1.first) {
        EXPECT_EQ(result1.second, hm11::sync::UartStatus::ErrTimeout);
    } else {
        EXPECT_EQ(result2.second, hm11::sync::UartStatus::ErrTimeout);
    }
}