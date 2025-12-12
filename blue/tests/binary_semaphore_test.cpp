// Unit tests for hm11::BinarySemaphore
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "hm11/sync/binary_semaphore.hpp"

using namespace hm11;

TEST(BinarySemaphoreTest, GiveThenTakeSucceeds) {
    BinarySemaphore sem;
    // Signal from ISR context (simulated)
    sem.give_from_isr();
    // Should immediately succeed
    bool ok = sem.take_with_timeout(std::chrono::milliseconds(10));
    EXPECT_TRUE(ok);
    // Flag cleared – subsequent take should timeout
    bool timeout = sem.take_with_timeout(std::chrono::milliseconds(1));
    EXPECT_FALSE(timeout);
}

TEST(BinarySemaphoreTest, TimeoutWhenNotSignaled) {
    BinarySemaphore sem;
    // No signal – should timeout quickly
    bool result = sem.take_with_timeout(std::chrono::milliseconds(5));
    EXPECT_FALSE(result);
}

TEST(BinarySemaphoreTest, TryTakeBehavior) {
    BinarySemaphore sem;
    // Initially not signaled
    EXPECT_FALSE(sem.try_take());
    // Signal
    sem.give_from_isr();
    EXPECT_TRUE(sem.try_take());
    // Flag cleared again
    EXPECT_FALSE(sem.try_take());
}

TEST(BinarySemaphoreTest, ConcurrentSignalAndTake) {
    BinarySemaphore sem;
    // Start a thread that will wait on the semaphore
    std::thread waiter([&]() {
        bool ok = sem.take_with_timeout(std::chrono::seconds(1));
        EXPECT_TRUE(ok);
    });
    // Give some time for waiter to block, then signal
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sem.give_from_isr();
    waiter.join();
    // Ensure semaphore is cleared after take
    EXPECT_FALSE(sem.try_take());
}
