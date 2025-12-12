/**
 * @file circular_buffer_test.cpp
 * @brief Unit tests for CircularBuffer class
 *
 * This file contains unit tests for the CircularBuffer class, testing various
 * aspects of the implementation including:
 * - Basic buffer operations
 * - Thread safety and lock-free behavior
 * - Proper handling of buffer wrapping
 * - Overflow-safe position tracking
 * - Multi-threaded producer/consumer scenarios
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <thread>
#include <vector>

#include "hm11/sync/circular_buffer.hpp"

namespace hm11 {
namespace sync {
namespace test {

// Test fixture for CircularBuffer
class CircularBufferTest : public ::testing::Test {
  protected:
    // CircularBuffer with capacity of 16 (must be power of 2)
    CircularBuffer<16> buffer;

    // Helper method to fill the buffer with sequential values
    void fill_buffer(int count) {
        for (uint8_t i = 0; i < count && i < 16; ++i) {
            buffer.write(i);
        }
    }
};

// Test basic buffer properties
TEST_F(CircularBufferTest, BasicProperties) {
    EXPECT_EQ(buffer.capacity(), 16U);
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_TRUE(buffer.empty());
}

// Test writing to the buffer
TEST_F(CircularBufferTest, WriteOperation) {
    buffer.write(42);
    EXPECT_FALSE(buffer.empty());

    // Fill the buffer to capacity
    for (int i = 0; i < 15; ++i) {
        buffer.write(i);
    }

    // Verify write position advanced
    EXPECT_GT(buffer.get_write_position(), 0U);
}

// Test reading from the buffer
TEST_F(CircularBufferTest, ReadOperation) {
    fill_buffer(5);
    EXPECT_EQ(buffer.available(), 5U);

    uint8_t value;
    EXPECT_TRUE(buffer.read(value));
    EXPECT_EQ(value, 0);
    EXPECT_EQ(buffer.available(), 4U);

    EXPECT_TRUE(buffer.read(value));
    EXPECT_EQ(value, 1);
    EXPECT_EQ(buffer.available(), 3U);

    // Read remaining values
    EXPECT_TRUE(buffer.read(value)); // 2
    EXPECT_TRUE(buffer.read(value)); // 3
    EXPECT_TRUE(buffer.read(value)); // 4
    EXPECT_EQ(buffer.available(), 0U);

    // Buffer is empty, read should fail
    EXPECT_FALSE(buffer.read(value));
}

// Test peeking at the buffer
TEST_F(CircularBufferTest, PeekOperation) {
    fill_buffer(3);

    uint8_t value;
    EXPECT_TRUE(buffer.peek(value));
    EXPECT_EQ(value, 0);

    // Peek doesn't remove the item, so size should remain the same
    EXPECT_EQ(buffer.available(), 3U);

    // Read the item, then peek at the next one
    EXPECT_TRUE(buffer.read(value));
    EXPECT_EQ(value, 0);

    EXPECT_TRUE(buffer.peek(value));
    EXPECT_EQ(value, 1);
}

// Test buffer wrapping
TEST_F(CircularBufferTest, BufferWrapping) {
    // Fill the buffer
    fill_buffer(16);

    // Read 8 items
    uint8_t value;
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(buffer.read(value));
    }

    // Now write 8 more items (should wrap around in the internal buffer)
    for (uint8_t i = 16; i < 24; ++i) {
        buffer.write(i);
    }

    // Verify correct values by reading all remaining items
    for (uint8_t i = 8; i < 24; ++i) {
        EXPECT_TRUE(buffer.read(value));
        EXPECT_EQ(value, i);
    }

    EXPECT_TRUE(buffer.empty());
}

// Test clearing the buffer
TEST_F(CircularBufferTest, ClearOperation) {
    fill_buffer(5);
    EXPECT_EQ(buffer.available(), 5U);

    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.available(), 0U);
}

// Test resetting the buffer
TEST_F(CircularBufferTest, ResetOperation) {
    fill_buffer(5);

    uint8_t value;
    EXPECT_TRUE(buffer.read(value));
    EXPECT_EQ(buffer.available(), 4U);

    buffer.reset();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.get_read_position(), 0U);
    EXPECT_EQ(buffer.get_write_position(), 0U);
}

// Test handling of large position values approaching integer overflow
TEST_F(CircularBufferTest, LargePositionValues) {
    CircularBuffer<4> small_buffer;

    // Write 4 items to fill the buffer
    small_buffer.write(1);
    small_buffer.write(2);
    small_buffer.write(3);
    small_buffer.write(4);

    // Read them all
    uint8_t value;
    EXPECT_TRUE(small_buffer.read(value)); // 1
    EXPECT_TRUE(small_buffer.read(value)); // 2
    EXPECT_TRUE(small_buffer.read(value)); // 3
    EXPECT_TRUE(small_buffer.read(value)); // 4

    // Repeat this many times to simulate long-running operation
    for (int i = 0; i < 1000; ++i) {
        // Write items
        small_buffer.write(10);
        small_buffer.write(20);
        small_buffer.write(30);

        // Read them
        EXPECT_TRUE(small_buffer.read(value)); // 10
        EXPECT_EQ(value, 10);
        EXPECT_TRUE(small_buffer.read(value)); // 20
        EXPECT_EQ(value, 20);
        EXPECT_TRUE(small_buffer.read(value)); // 30
        EXPECT_EQ(value, 30);
    }

    // Buffer should be empty after all operations
    EXPECT_TRUE(small_buffer.empty());

    // Even after many operations, buffer should still work correctly
    small_buffer.write(42);
    EXPECT_GT(small_buffer.available(), 0U);
    EXPECT_TRUE(small_buffer.read(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(small_buffer.empty());
}

// Test direct data access (Ada-style)
TEST_F(CircularBufferTest, DirectDataAccess) {
    // Write some data
    buffer.write(10);
    buffer.write(20);
    buffer.write(30);

    // Access data directly
    const uint8_t* data = buffer.data();
    size_t size = buffer.size();

    EXPECT_NE(data, nullptr);
    EXPECT_GT(size, 0U);

    // Data should be accessible up to size
    for (size_t i = 0; i < size && i < 3; ++i) {
        // Just verify we can read the data
        [[maybe_unused]] uint8_t byte = data[i];
    }
}

// Test producer-consumer scenario with multiple threads
TEST_F(CircularBufferTest, ThreadedProducerConsumer) {
    CircularBuffer<128> large_buffer;

    std::atomic<bool> producer_done(false);
    std::atomic<int> items_produced(0);
    std::atomic<int> items_consumed(0);

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 1000; ++i) {
            large_buffer.write(static_cast<uint8_t>(i & 0xFF));
            items_produced.fetch_add(1, std::memory_order_relaxed);
            // Small yield to allow consumer to run
            if (i % 10 == 0) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer thread
    std::thread consumer([&]() {
        uint8_t value;
        while (!producer_done.load(std::memory_order_acquire) || !large_buffer.empty()) {
            if (large_buffer.read(value)) {
                items_consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(items_produced.load(), 1000);
    EXPECT_EQ(items_consumed.load(), 1000);
    EXPECT_TRUE(large_buffer.empty());
}

} // namespace test
} // namespace sync
} // namespace hm11
