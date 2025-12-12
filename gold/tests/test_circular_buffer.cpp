#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "hm11/sync/circular_buffer.hpp"

using namespace hm11::sync;

// Test basic write and read operations
TEST(CircularBufferTest, BasicWriteRead) {
    CircularBuffer<8> buffer;

    // Initially empty
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.available(), 0);

    // Write single byte
    constexpr uint8_t WRITE_DATA = 42;
    buffer.write(WRITE_DATA); // Always succeeds (Ada-style)
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.available(), 1);

    // Read single byte
    uint8_t read_data = 0;
    bool read_success = buffer.read(read_data);
    EXPECT_TRUE(read_success);
    EXPECT_EQ(read_data, 42);
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.available(), 0);
}

// Test sequential writes and reads
TEST(CircularBufferTest, SequentialWriteRead) {
    CircularBuffer<8> buffer;

    // Write 5 bytes
    for (uint8_t i = 0; i < 5; i++) {
        buffer.write(i); // Always succeeds
    }
    EXPECT_EQ(buffer.available(), 5);
    EXPECT_FALSE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    // Read 5 bytes and verify
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t data = 0;
        bool success = buffer.read(data);
        EXPECT_TRUE(success);
        EXPECT_EQ(data, i);
    }
    EXPECT_TRUE(buffer.empty());
}

// Test filling buffer to capacity
TEST(CircularBufferTest, FillToCapacity) {
    CircularBuffer<8> buffer;

    // Write 7 bytes (capacity - 1, since we use one slot to distinguish full from empty)
    for (uint8_t i = 0; i < 7; i++) {
        buffer.write(i); // Always succeeds
    }

    EXPECT_EQ(buffer.available(), 7);
    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.empty());

    // Next write always succeeds (Ada-style overwrite)
    buffer.write(99);

    // Position wraps, so we have 8 bytes total written (but capacity is 8)
    // This matches Ada's wraparound behavior
    EXPECT_EQ(buffer.get_write_position(), 0); // Wrapped to 0
}

// Test wraparound behavior
TEST(CircularBufferTest, Wraparound) {
    CircularBuffer<8> buffer;

    // Write and read to advance positions
    for (uint8_t i = 0; i < 5; i++) {
        (void)buffer.write(i);
    }
    for (int i = 0; i < 5; i++) {
        uint8_t data = 0;
        (void)buffer.read(data);
    }

    // Now positions are at index 5 (wrapped positions)
    // Write 7 more bytes (will fill buffer and wrap around)
    for (uint8_t i = 10; i < 17; i++) {
        (void)buffer.write(i);
    }

    // We should be able to read 7 bytes (10-16)
    EXPECT_EQ(buffer.available(), 7U);
    for (uint8_t expected = 10; expected < 17; expected++) {
        uint8_t data = 0;
        bool success = buffer.read(data);
        EXPECT_TRUE(success);
        EXPECT_EQ(data, expected);
    }

    // Buffer should now be empty
    EXPECT_TRUE(buffer.empty());
}

// Test peek operation
TEST(CircularBufferTest, Peek) {
    CircularBuffer<8> buffer;

    // Write data
    (void)buffer.write(42);
    (void)buffer.write(43);

    // Peek should return first byte without consuming
    uint8_t data = 0;
    bool peek_success = buffer.peek(data);
    EXPECT_TRUE(peek_success);
    EXPECT_EQ(data, 42);
    EXPECT_EQ(buffer.available(), 2); // Still 2 bytes available

    // Peek again should return same byte
    peek_success = buffer.peek(data);
    EXPECT_TRUE(peek_success);
    EXPECT_EQ(data, 42);

    // Now read should consume the byte
    bool read_success = buffer.read(data);
    EXPECT_TRUE(read_success);
    EXPECT_EQ(data, 42);
    EXPECT_EQ(buffer.available(), 1);

    // Peek should now return next byte
    peek_success = buffer.peek(data);
    EXPECT_TRUE(peek_success);
    EXPECT_EQ(data, 43);
}

// Test reading from empty buffer
TEST(CircularBufferTest, ReadEmpty) {
    CircularBuffer<8> buffer;

    uint8_t data = 0;
    bool read_success = buffer.read(data);
    EXPECT_FALSE(read_success);
    bool peek_success = buffer.peek(data);
    EXPECT_FALSE(peek_success);
}

// Test clear operation
TEST(CircularBufferTest, Clear) {
    CircularBuffer<8> buffer;

    // Write some data
    for (uint8_t i = 0; i < 5; i++) {
        (void)buffer.write(i);
    }
    EXPECT_EQ(buffer.available(), 5);

    // Clear buffer
    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.available(), 0);

    // Should not be able to read
    uint8_t data = 0;
    bool read_success = buffer.read(data);
    EXPECT_FALSE(read_success);
}

// Test available count across wraparound
TEST(CircularBufferTest, AvailableWithWraparound) {
    CircularBuffer<8> buffer;

    // Advance positions past buffer end
    for (int i = 0; i < 10; i++) {
        (void)buffer.write(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 5; i++) {
        uint8_t data = 0;
        (void)buffer.read(data);
    }

    // Write more to cause write position to wrap
    for (int i = 0; i < 4; i++) {
        (void)buffer.write(static_cast<uint8_t>(i + 100));
    }

    // Available should be calculated correctly despite wraparound
    size_t avail = buffer.available();
    EXPECT_GT(avail, 0U);
    EXPECT_LE(avail, 7U); // Max available is capacity - 1
}

// Test buffer full handling with overwrite
TEST(CircularBufferTest, OverwriteWhenFull) {
    CircularBuffer<8> buffer;

    // Fill buffer to capacity (7 bytes)
    for (uint8_t i = 0; i < 7; i++) {
        buffer.write(i);
    }
    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.get_write_position(), 7);

    // Write 8th byte - wraps position to 0 (Ada-style)
    buffer.write(99);
    EXPECT_EQ(buffer.get_write_position(), 0); // Wrapped to 0
    EXPECT_EQ(buffer.get_read_position(), 0);  // Consumer hasn't read yet

    // Buffer appears empty because write_position == read_position
    // But it's actually full with wrapped data
    EXPECT_TRUE(buffer.empty()); // Wraparound makes it appear empty

    // Write another byte - this overwrites the data
    buffer.write(100);
    EXPECT_EQ(buffer.get_write_position(), 1);

    // Now we can read
    uint8_t data = 0;
    bool read_success = buffer.read(data);
    EXPECT_TRUE(read_success);
    EXPECT_EQ(data, 100); // The byte we just wrote at position 0
}

// Test concurrent producer/consumer (simulated ISR with std::thread)
TEST(CircularBufferTest, ConcurrentProducerConsumer) {
    CircularBuffer<256> buffer;
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> bytes_written{0};
    std::atomic<size_t> bytes_read{0};
    constexpr size_t TOTAL_BYTES = 10000;

    // Producer thread (simulates ISR)
    std::thread producer([&]() {
        for (size_t i = 0; i < TOTAL_BYTES; i++) {
            // Write with small delays to simulate realistic timing
            (void)buffer.write(static_cast<uint8_t>(i & 0xFF));
            bytes_written++;

            // Small delay to allow consumer to catch up sometimes
            if (i % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        producer_done.store(true);
    });

    // Consumer thread (simulates application)
    std::thread consumer([&]() {
        std::vector<uint8_t> read_data;
        read_data.reserve(TOTAL_BYTES);

        while (!producer_done.load() || buffer.available() > 0) {
            uint8_t data = 0;
            if (buffer.read(data)) {
                read_data.push_back(data);
                bytes_read++;
            } else {
                // Buffer empty, yield to producer
                std::this_thread::yield();
            }
        }

        // Verify we got data (may have lost some due to overwrite, but should have recent data)
        EXPECT_GT(read_data.size(), 0U);
    });

    producer.join();
    consumer.join();

    // Verify reasonable throughput
    EXPECT_EQ(bytes_written.load(), TOTAL_BYTES);
    EXPECT_GT(bytes_read.load(), 0U);

    // In overwrite mode, we might not read all bytes if consumer is slower
    // But we should read a significant portion
    EXPECT_GT(bytes_read.load(), TOTAL_BYTES / 2);
}

// Test sustained concurrent throughput
TEST(CircularBufferTest, SustainedThroughput) {
    CircularBuffer<512> buffer;
    std::atomic<bool> stop{false};
    std::atomic<size_t> write_count{0};
    std::atomic<size_t> read_count{0};
    constexpr auto DURATION = std::chrono::milliseconds(100);

    // Producer thread
    std::thread producer([&]() {
        uint8_t value = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            (void)buffer.write(value++);
            write_count++;
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            uint8_t data = 0;
            if (buffer.read(data)) {
                read_count++;
            } else {
                std::this_thread::yield();
            }
        }
        // Drain remaining data
        uint8_t data = 0;
        while (buffer.read(data)) {
            read_count++;
        }
    });

    // Run for duration
    std::this_thread::sleep_for(DURATION);
    stop.store(true);

    producer.join();
    consumer.join();

    // Verify sustained throughput (should process many bytes in 100ms)
    EXPECT_GT(write_count.load(), 1000U);
    EXPECT_GT(read_count.load(), 1000U);

    // Final buffer should be empty or nearly empty
    EXPECT_LT(buffer.available(), 100U);
}

// Test edge case: single byte before wrap
TEST(CircularBufferTest, SingleByteBeforeWrap) {
    CircularBuffer<8> buffer;

    // Advance to position 7 (last valid position before wrap)
    for (int i = 0; i < 7; i++) {
        (void)buffer.write(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        uint8_t data = 0;
        (void)buffer.read(data);
    }

    // Write one byte at position 7
    buffer.write(99);
    EXPECT_EQ(buffer.available(), 1U);

    // Write one more byte - should wrap to position 0
    buffer.write(100);
    EXPECT_EQ(buffer.available(), 2U);

    // Read and verify
    uint8_t data = 0;
    bool read_success = buffer.read(data);
    EXPECT_TRUE(read_success);
    EXPECT_EQ(data, 99);
    read_success = buffer.read(data);
    EXPECT_TRUE(read_success);
    EXPECT_EQ(data, 100);
}

// Test capacity is correct
TEST(CircularBufferTest, Capacity) {
    CircularBuffer<8> buffer8;
    EXPECT_EQ(buffer8.capacity(), 8U);

    CircularBuffer<256> buffer256;
    EXPECT_EQ(buffer256.capacity(), 256U);
}

// Test that buffer is properly aligned for DMA (Decision 20)
TEST(CircularBufferTest, Alignment) {
    CircularBuffer<64> buffer;

    // Get address of buffer's internal storage (via write_position() to ensure buffer exists)
    const void* const BUFFER_ADDR = &buffer;

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Pointer to integer conversion for alignment check
    EXPECT_EQ(reinterpret_cast<uintptr_t>(BUFFER_ADDR) % 32, 0)
        << "Buffer should be 32-byte aligned for DMA and cache coherency";
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

// Test position tracking (matches Ada's Position variable)
TEST(CircularBufferTest, PositionTracking) {
    CircularBuffer<8> buffer;

    EXPECT_EQ(buffer.get_write_position(), 0U);
    EXPECT_EQ(buffer.get_read_position(), 0U);

    // Write 3 bytes
    (void)buffer.write(1);
    (void)buffer.write(2);
    (void)buffer.write(3);

    EXPECT_EQ(buffer.get_write_position(), 3U);
    EXPECT_EQ(buffer.get_read_position(), 0U);

    // Read 2 bytes
    uint8_t data = 0;
    (void)buffer.read(data);
    (void)buffer.read(data);

    EXPECT_EQ(buffer.get_write_position(), 3U);
    EXPECT_EQ(buffer.get_read_position(), 2U);
}

// Test wraparound of write position
TEST(CircularBufferTest, WritePositionWraparound) {
    CircularBuffer<8> buffer;

    // Write 8 bytes (position wraps from 7 to 0)
    for (uint8_t i = 0; i < 8; i++) {
        (void)buffer.write(i);
    }

    // Write position should have wrapped to 0 or 1 (depending on overwrite)
    EXPECT_LT(buffer.get_write_position(), 8U);

    // After reading, write position doesn't change
    uint8_t data = 0;
    for (int i = 0; i < 7; i++) {
        (void)buffer.read(data);
    }

    size_t write_pos_before = buffer.get_write_position();
    (void)buffer.write(99);
    EXPECT_EQ(buffer.get_write_position(), (write_pos_before + 1) % 8);
}

// Stress test: rapid writes and reads
TEST(CircularBufferTest, StressTest) {
    CircularBuffer<128> buffer;
    constexpr size_t ITERATIONS = 1000;

    for (size_t iter = 0; iter < ITERATIONS; iter++) {
        // Write some data
        for (uint8_t i = 0; i < 10; i++) {
            (void)buffer.write(static_cast<uint8_t>(iter + i));
        }

        // Read some data
        for (int i = 0; i < 5; i++) {
            uint8_t data = 0;
            (void)buffer.read(data);
        }
    }

    // Buffer should still be functional
    buffer.write(42);
    uint8_t data = 0;
    bool read_success = buffer.read(data);
    EXPECT_TRUE(read_success);
}

// Test that CircularBuffer is not copyable or movable (Decision 22)
TEST(CircularBufferTest, NotCopyableOrMovable) {
    EXPECT_FALSE(std::is_copy_constructible_v<CircularBuffer<8>>);
    EXPECT_FALSE(std::is_copy_assignable_v<CircularBuffer<8>>);
    EXPECT_FALSE(std::is_move_constructible_v<CircularBuffer<8>>);
    EXPECT_FALSE(std::is_move_assignable_v<CircularBuffer<8>>);
}
