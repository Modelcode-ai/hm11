/**
 * @file watchdog_test.cpp
 * @brief Unit tests for the watchdog implementation
 *
 * This file is part of the HM11 driver C++ migration project.
 * It tests the watchdog implementation.
 */

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <future>
#include <thread>

#include "hm11/sync/watchdog.hpp"

// Test fixture for watchdog tests
class WatchdogTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test basic watchdog functionality
TEST_F(WatchdogTest, BasicFunctionality) {
    // Create a watchdog
    hm11::sync::Watchdog watchdog;

    // Watchdog should not be started initially
    EXPECT_FALSE(watchdog.is_started());

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog
    bool result = watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000));
    EXPECT_TRUE(result);
    EXPECT_TRUE(watchdog.is_started());

    // Check that the position is initialized to 1 (1-based indexing like Ada)
    bool is_stopped;
    std::size_t position;
    watchdog.get_position(is_stopped, position);
    EXPECT_FALSE(is_stopped);
    EXPECT_EQ(position, 1);

    // Update the position
    watchdog.set_position(5);
    watchdog.get_position(is_stopped, position);
    EXPECT_EQ(position, 5);

    // Stop the watchdog
    watchdog.stop();
    EXPECT_FALSE(watchdog.is_started());

    // Verify the position is preserved after stopping
    watchdog.get_position(is_stopped, position);
    EXPECT_TRUE(is_stopped);
    EXPECT_EQ(position, 5);
}

// Test await_event functionality
TEST_F(WatchdogTest, AwaitEvent) {
    hm11::sync::Watchdog watchdog;

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog
    watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000));

    // Launch a thread that will release the watchdog after a delay
    auto future = std::async(std::launch::async, [&watchdog]() {
        // Wait a bit before signaling
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // Release with error status to test status passing
        watchdog.release(hm11::sync::UartStatus::ErrError);
        return true;
    });

    // Wait for the event
    hm11::sync::UartStatus status;
    bool result = watchdog.await_event(status);

    // Verify the result
    EXPECT_TRUE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrError);

    // Watchdog should be automatically stopped after release
    EXPECT_FALSE(watchdog.is_started());

    // Make sure the async thread completes
    future.wait();
}

// Test deadline exceeded functionality
TEST_F(WatchdogTest, DeadlineExceeded) {
    hm11::sync::Watchdog watchdog;

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog with a very short timeout
    watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(10));

    // Wait long enough for the deadline to be exceeded
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Check if deadline is exceeded
    EXPECT_TRUE(watchdog.is_deadline_exceeded());
}

// Test ISR-like behavior using threads
TEST_F(WatchdogTest, IsrSimulation) {
    hm11::sync::Watchdog watchdog;

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog
    watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000));

    // Launch a thread that will simulate an ISR
    std::thread isr_thread([&watchdog]() {
        // Simulate some ISR processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Release the watchdog as if from an ISR
        watchdog.release(hm11::sync::UartStatus::Ok);
    });

    // Wait for the event
    hm11::sync::UartStatus status;
    bool result = watchdog.await_event(status);

    // Verify the result
    EXPECT_TRUE(result);
    EXPECT_EQ(status, hm11::sync::UartStatus::Ok);

    // Ensure the ISR thread completes
    if (isr_thread.joinable()) {
        isr_thread.join();
    }
}

// Test starting a watchdog multiple times
TEST_F(WatchdogTest, MultipleStarts) {
    hm11::sync::Watchdog watchdog;

    // Create buffers for testing
    std::array<uint8_t, 10> buffer1 = {0};
    std::array<uint8_t, 20> buffer2 = {0};

    // First start should succeed
    bool result1 = watchdog.start(buffer1.data(), buffer1.size(), std::chrono::milliseconds(1000));
    EXPECT_TRUE(result1);
    EXPECT_TRUE(watchdog.is_started());

    // Second start should fail because watchdog is already started
    bool result2 = watchdog.start(buffer2.data(), buffer2.size(), std::chrono::milliseconds(1000));
    EXPECT_FALSE(result2);

    // Stop the watchdog
    watchdog.stop();
    EXPECT_FALSE(watchdog.is_started());

    // After stopping, should be able to start again
    bool result3 = watchdog.start(buffer2.data(), buffer2.size(), std::chrono::milliseconds(1000));
    EXPECT_TRUE(result3);
    EXPECT_TRUE(watchdog.is_started());
}

// Test position management
TEST_F(WatchdogTest, PositionManagement) {
    hm11::sync::Watchdog watchdog;

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog
    watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000));

    // Position should start at 1
    bool is_stopped;
    std::size_t position;
    watchdog.get_position(is_stopped, position);
    EXPECT_FALSE(is_stopped);
    EXPECT_EQ(position, 1);

    // Update position
    watchdog.set_position(5);
    watchdog.get_position(is_stopped, position);
    EXPECT_EQ(position, 5);

    // Update to beyond buffer size - should allow this as the caller
    // is responsible for bounds checking
    watchdog.set_position(20);
    watchdog.get_position(is_stopped, position);
    EXPECT_EQ(position, 20);

    // Stop the watchdog
    watchdog.stop();

    // Position should be preserved after stopping
    watchdog.get_position(is_stopped, position);
    EXPECT_TRUE(is_stopped);
    EXPECT_EQ(position, 20);
}

// Test timeout handling
TEST_F(WatchdogTest, TimeoutHandling) {
    hm11::sync::Watchdog watchdog;

    // Create a buffer for testing
    std::array<uint8_t, 10> buffer = {0};

    // Start the watchdog with a very short timeout
    watchdog.start(buffer.data(), buffer.size(), std::chrono::milliseconds(10));

    // Try to wait for an event
    hm11::sync::UartStatus status;

    // Release after timeout
    std::thread releaser([&watchdog]() {
        // Wait for the deadline to be exceeded
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Check if deadline exceeded and release with timeout status
        if (watchdog.is_deadline_exceeded()) {
            watchdog.release(hm11::sync::UartStatus::ErrTimeout);
        }
    });

    // Wait for the event
    bool result = watchdog.await_event(status);

    // Verify the result
    EXPECT_TRUE(result); // The result should be true because release was called
    EXPECT_EQ(status, hm11::sync::UartStatus::ErrTimeout);

    // Ensure the releaser thread completes
    if (releaser.joinable()) {
        releaser.join();
    }
}