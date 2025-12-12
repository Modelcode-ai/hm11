/**
 * @file circular_buffer_test.cpp
 * @brief Unit tests for the circular buffer implementation
 *
 * This file contains tests for the circular buffer implementation,
 * ensuring it works correctly for the HM11 driver.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <thread>
#include <vector>

#include "hm11/hal/types.hpp"
#include "hm11/util/circular_buffer.hpp"

// Test fixture for CircularBuffer tests
class CircularBufferTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test basic operations
TEST_F(CircularBufferTest, BasicOperations) {
    hm11::util::CircularBuffer<int, 5> buffer;

    // Initial state
    EXPECT_EQ(buffer.capacity(), 5);
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    // Write some data
    EXPECT_TRUE(buffer.write(10));
    EXPECT_TRUE(buffer.write(20));
    EXPECT_TRUE(buffer.write(30));

    // Check state
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_FALSE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    // Read data
    int item;
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 10);
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 20);
    EXPECT_EQ(buffer.size(), 1);

    // Peek data
    EXPECT_TRUE(buffer.peek(0, item));
    EXPECT_EQ(item, 30);
    EXPECT_EQ(buffer.size(), 1); // Peek doesn't change size

    // Read the last item
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 30);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.read(item)); // Nothing to read
}

// Test filling the buffer
TEST_F(CircularBufferTest, FullBuffer) {
    hm11::util::CircularBuffer<int, 4> buffer;

    // Fill the buffer
    EXPECT_TRUE(buffer.write(10));
    EXPECT_TRUE(buffer.write(20));
    EXPECT_FALSE(buffer.full()); // Not full with 2 elements in a buffer of capacity 4
    EXPECT_TRUE(buffer.write(30));
    EXPECT_TRUE(buffer.full());     // Full after 3 elements because one slot is always kept empty
    EXPECT_FALSE(buffer.write(40)); // Can't write any more

    // Read one item
    int item;
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 10);
    EXPECT_FALSE(buffer.full());

    // Now we can write again
    EXPECT_TRUE(buffer.write(60));
    EXPECT_TRUE(buffer.full()); // Full again
}

// Test wrapping behavior
TEST_F(CircularBufferTest, WrappingBehavior) {
    hm11::util::CircularBuffer<int, 4> buffer;

    // Fill the buffer
    EXPECT_TRUE(buffer.write(10));
    EXPECT_TRUE(buffer.write(20));
    EXPECT_TRUE(buffer.write(30));

    // Read data
    int item;
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 10);
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 20);

    // Write more data (will wrap around)
    EXPECT_TRUE(buffer.write(40));
    EXPECT_TRUE(buffer.write(50));

    // Verify the contents
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 30);
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 40);
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 50);
    EXPECT_TRUE(buffer.empty());
}

// Test batch operations
TEST_F(CircularBufferTest, BatchOperations) {
    hm11::util::CircularBuffer<int, 10> buffer;

    // Write multiple items at once
    std::array<int, 5> write_data = {10, 20, 30, 40, 50};
    EXPECT_EQ(buffer.write(write_data), 5);

    // Read multiple items at once
    std::array<int, 3> read_data{};
    EXPECT_EQ(buffer.read(read_data), 3);
    EXPECT_EQ(read_data[0], 10);
    EXPECT_EQ(read_data[1], 20);
    EXPECT_EQ(read_data[2], 30);

    // Check state
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_FALSE(buffer.empty());
}

// Test thread safety with producer-consumer pattern
TEST_F(CircularBufferTest, ThreadSafety) {
    hm11::util::CircularBuffer<int, 100> buffer;
    constexpr int NUM_ITEMS = 1000;

    std::atomic<bool> producer_done(false);
    std::atomic<int> consumed_sum(0);

    // Producer thread
    std::thread producer([&]() {
        for (int i = 1; i <= NUM_ITEMS; ++i) {
            while (!buffer.write(i)) {
                std::this_thread::yield(); // Buffer full, yield and retry
            }
        }
        producer_done = true;
    });

    // Consumer thread
    std::thread consumer([&]() {
        int sum = 0;
        int count = 0;
        int item;

        while (count < NUM_ITEMS) {
            if (buffer.read(item)) {
                sum += item;
                ++count;
            } else if (!producer_done) {
                std::this_thread::yield(); // Buffer empty but producer not done, yield
            } else {
                // Producer done and buffer empty, but we haven't consumed all items
                // This shouldn't happen if the buffer is working correctly
                if (count < NUM_ITEMS) {
                    FAIL() << "Consumer couldn't read all items";
                }
                break;
            }
        }

        consumed_sum = sum;
    });

    // Wait for threads to finish
    producer.join();
    consumer.join();

    // Calculate expected sum (1+2+...+NUM_ITEMS)
    const int expected_sum = (NUM_ITEMS * (NUM_ITEMS + 1)) / 2;
    EXPECT_EQ(consumed_sum.load(), expected_sum);
}

// Test clear operation
TEST_F(CircularBufferTest, Clear) {
    hm11::util::CircularBuffer<int, 5> buffer;

    // Fill the buffer
    EXPECT_TRUE(buffer.write(10));
    EXPECT_TRUE(buffer.write(20));
    EXPECT_TRUE(buffer.write(30));
    EXPECT_EQ(buffer.size(), 3);

    // Clear the buffer
    buffer.clear();
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    // Write after clear
    EXPECT_TRUE(buffer.write(40));
    EXPECT_EQ(buffer.size(), 1);

    // Read after clear
    int item;
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(item, 40);
}

// Test positions
TEST_F(CircularBufferTest, Positions) {
    hm11::util::CircularBuffer<int, 5> buffer;

    // Initial positions
    EXPECT_EQ(buffer.get_read_position(), 0);
    EXPECT_EQ(buffer.get_write_position(), 0);

    // Write and check positions
    EXPECT_TRUE(buffer.write(10));
    EXPECT_TRUE(buffer.write(20));
    EXPECT_EQ(buffer.get_read_position(), 0);
    EXPECT_EQ(buffer.get_write_position(), 2);

    // Read and check positions
    int item;
    EXPECT_TRUE(buffer.read(item));
    EXPECT_EQ(buffer.get_read_position(), 1);
    EXPECT_EQ(buffer.get_write_position(), 2);

    // Test wrapping
    EXPECT_TRUE(buffer.write(30));
    EXPECT_TRUE(buffer.write(40));
    EXPECT_TRUE(buffer.read(item)); // read 20
    EXPECT_TRUE(buffer.read(item)); // read 30
    EXPECT_TRUE(buffer.write(50));
    EXPECT_TRUE(buffer.write(60)); // Should wrap

    // The actual positions depend on the implementation, but we can test the size
    EXPECT_EQ(buffer.size(), 3); // 40, 50, 60

    // Reading all items should advance read_pos to write_pos
    EXPECT_TRUE(buffer.read(item)); // read 40
    EXPECT_TRUE(buffer.read(item)); // read 50
    EXPECT_TRUE(buffer.read(item)); // read 60
    EXPECT_EQ(buffer.get_read_position(), buffer.get_write_position());
}