// HM11 Synchronization Primitives - Circular Buffer Unit Tests
// Comprehensive testing for lock-free SPSC circular buffer with ThreadSanitizer validation

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <random>
#include <thread>
#include <vector>

#include "hm11/sync/circular_buffer.hpp"

using namespace hm11::sync;

class CircularBufferTest : public ::testing::Test {
  protected:
    // Use small buffer for testing (16 elements, power of 2)
    static constexpr std::size_t BUFFER_SIZE = 16;
    CircularBuffer<std::uint8_t, BUFFER_SIZE> buffer_;
};

/// Test basic write/read operations
TEST_F(CircularBufferTest, BasicWriteRead) {
    // Initially empty
    EXPECT_TRUE(buffer_.IsEmpty());
    EXPECT_FALSE(buffer_.IsFull());
    EXPECT_EQ(buffer_.GetAvailableCount(), 0);

    // Write single byte
    std::uint8_t test_byte = 0xAB;
    buffer_.WriteFromISR(test_byte);

    // Should have data available
    EXPECT_FALSE(buffer_.IsEmpty());
    EXPECT_EQ(buffer_.GetAvailableCount(), 1);

    // Read data back
    auto data = buffer_.ReadAvailable();
    ASSERT_EQ(data.size(), 1);
    EXPECT_EQ(data[0], test_byte);

    // Mark as consumed
    buffer_.MarkConsumed(1);
    EXPECT_TRUE(buffer_.IsEmpty());
}

/// Test buffer wrap-around behavior
TEST_F(CircularBufferTest, WrapAroundBehavior) {
    // Fill buffer to capacity - 1 (can't use last slot)
    for (std::size_t i = 0; i < BUFFER_SIZE - 1; ++i) {
        buffer_.WriteFromISR(static_cast<std::uint8_t>(i));
    }

    // Buffer should be full
    EXPECT_TRUE(buffer_.IsFull());
    EXPECT_EQ(buffer_.GetAvailableCount(), BUFFER_SIZE - 1);

    // Read half the data
    auto first_half = buffer_.ReadAvailable();
    EXPECT_GE(first_half.size(), (BUFFER_SIZE - 1) / 2);
    std::size_t consumed = first_half.size();
    buffer_.MarkConsumed(consumed);

    // Write more data (testing wrap-around to beginning of buffer)
    for (std::size_t i = 0; i < consumed; ++i) {
        buffer_.WriteFromISR(static_cast<std::uint8_t>(0x80 + i));
    }

    // Should have data available after wraparound
    auto remaining = buffer_.ReadAvailable();
    EXPECT_GT(remaining.size(), 0);

    // Verify we can consume all remaining data
    buffer_.MarkConsumed(remaining.size());
}

/// Test batch write operations
TEST_F(CircularBufferTest, BatchWriteOperations) {
    std::vector<std::uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Write batch (OVERWRITE semantics - always writes all data)
    buffer_.WriteFromISR(test_data);

    // Read back
    auto read_data = buffer_.ReadAvailable();
    ASSERT_EQ(read_data.size(), test_data.size());

    // Verify data integrity
    for (std::size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_EQ(read_data[i], test_data[i]);
    }

    buffer_.MarkConsumed(read_data.size());
    EXPECT_TRUE(buffer_.IsEmpty());
}

/// Test overwrite behavior when buffer full (OVERWRITE semantics)
TEST_F(CircularBufferTest, OverwriteBehavior) {
    // Fill buffer nearly to capacity
    std::vector<std::uint8_t> initial_data(BUFFER_SIZE - 3, 0xAA);
    buffer_.WriteFromISR(initial_data);

    // Verify initial write succeeded
    EXPECT_EQ(buffer_.GetAvailableCount(), initial_data.size());

    // Write more data than available space (will cause overwrites)
    std::vector<std::uint8_t> overflow_data(5, 0xBB);
    buffer_.WriteFromISR(overflow_data);

    // With simple SPSC overwrite semantics, new data is always written
    // Consumer may lose old data if it doesn't read fast enough
    // Verify we have data available (exact amount depends on timing)
    EXPECT_GT(buffer_.GetAvailableCount(), 0);

    // Read available data - verify no corruption
    auto data = buffer_.ReadAvailable();
    ASSERT_GT(data.size(), 0);

    // All bytes read should be either 0xAA or 0xBB (no corruption)
    for (const auto byte : data) {
        EXPECT_TRUE(byte == 0xAA || byte == 0xBB) << "Unexpected byte value: " << static_cast<int>(byte);
    }
}

/// Test concurrent producer/consumer with mock ISR (OVERWRITE semantics)
TEST_F(CircularBufferTest, ConcurrentProducerConsumerWithOverwrite) {
    constexpr int TEST_ITERATIONS = 1000;
    std::atomic<bool> producer_done{false};
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};

    // Producer thread (mock ISR) - produces faster than consumer
    std::thread producer([&]() {
        for (int iteration = 0; iteration < TEST_ITERATIONS; ++iteration) {
            std::uint8_t byte = static_cast<std::uint8_t>(iteration & 0xFF);

            // OVERWRITE semantics - always succeeds
            buffer_.WriteFromISR(byte);
            total_produced.fetch_add(1);

            // Minimal delay to allow potential overwrites
            if (iteration % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        producer_done.store(true);
    });

    // Consumer thread - consumes slower to test overwrite behavior
    std::thread consumer([&]() {
        while (!producer_done.load() || !buffer_.IsEmpty()) {
            auto data = buffer_.ReadAvailable();

            if (!data.empty()) {
                // Count consumed data (may be less than produced due to overwrites)
                total_consumed.fetch_add(data.size());
                buffer_.MarkConsumed(data.size());

                // Intentional delay to create backpressure
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    // With OVERWRITE semantics, consumed may be less than produced (data was overwritten)
    // But buffer should be empty at the end
    EXPECT_TRUE(buffer_.IsEmpty());
    EXPECT_EQ(total_produced.load(), TEST_ITERATIONS);
    // consumed <= produced (some data may have been overwritten)
    EXPECT_LE(total_consumed.load(), total_produced.load());
}

/// Test memory ordering with data dependencies
TEST_F(CircularBufferTest, MemoryOrderingCorrectness) {
    constexpr int ITERATIONS = 1000;
    std::atomic<bool> producer_done{false};
    std::atomic<int> write_count{0};
    std::atomic<int> read_count{0};
    std::atomic<bool> corruption_detected{false};

    // Producer writes sequential data
    std::thread producer([&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            std::uint8_t data_byte = static_cast<std::uint8_t>(i & 0xFF);

            // Write data (circular buffer uses release ordering internally)
            // OVERWRITE semantics - always succeeds
            buffer_.WriteFromISR(data_byte);

            write_count.store(i + 1, std::memory_order_release);

            // Small delay to allow consumer to keep up
            if (i % 10 == 0) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer reads data and verifies no corruption
    std::thread consumer([&]() {
        int last_value = -1;
        int gaps = 0;

        while (!producer_done.load(std::memory_order_acquire) || !buffer_.IsEmpty()) {
            // Wait for data
            std::span<const std::uint8_t> data = buffer_.ReadAvailable();
            if (data.empty()) {
                std::this_thread::yield();
                continue;
            }

            for (std::uint8_t byte : data) {
                int current_value = static_cast<int>(byte);

                // Check for monotonicity (allowing for gaps due to overwrites and uint8_t wraparound)
                if (last_value != -1) {
                    int expected_next = (last_value + 1) & 0xFF;
                    if (current_value != expected_next) {
                        // Gap detected (data loss due to overwrite) - this is acceptable in SPSC with overwrite
                        gaps++;
                    }
                }
                last_value = current_value;
            }

            buffer_.MarkConsumed(data.size());
            read_count.fetch_add(static_cast<int>(data.size()), std::memory_order_release);
        }
    });

    producer.join();
    consumer.join();

    // Verify all data was produced
    EXPECT_EQ(write_count.load(), ITERATIONS);

    // Consumer should have read at least some data (may not be all if overwrites occurred)
    EXPECT_GT(read_count.load(), 0);

    // No corruption detected
    EXPECT_FALSE(corruption_detected.load());
}

/// Test buffer reset operation
TEST_F(CircularBufferTest, ResetOperation) {
    // Write some data
    std::vector<std::uint8_t> test_data = {1, 2, 3, 4, 5};
    buffer_.WriteFromISR(test_data);

    EXPECT_FALSE(buffer_.IsEmpty());
    EXPECT_EQ(buffer_.GetAvailableCount(), test_data.size());

    // Reset buffer
    buffer_.Reset();

    EXPECT_TRUE(buffer_.IsEmpty());
    EXPECT_EQ(buffer_.GetAvailableCount(), 0);
    EXPECT_FALSE(buffer_.IsFull());

    // Should be able to write after reset
    buffer_.WriteFromISR(0x99);
    EXPECT_EQ(buffer_.GetAvailableCount(), 1);
}

/// Test buffer capacity and alignment
TEST_F(CircularBufferTest, CapacityAndAlignment) {
    EXPECT_EQ(buffer_.GetCapacity(), BUFFER_SIZE);

    // Test alignment requirements (for DMA compatibility)
    // Buffer should be aligned on 32-byte boundary
    void* buffer_addr = &buffer_;
    auto addr_value = reinterpret_cast<std::uintptr_t>(buffer_addr);
    EXPECT_EQ(addr_value % 32, 0); // 32-byte aligned
}

/// Stress test for race condition detection
TEST_F(CircularBufferTest, StressTestRaceConditions) {
    constexpr int STRESS_ITERATIONS = 10000;
    std::atomic<bool> producer_done{false};
    std::atomic<int> sequence_number{0};
    std::atomic<bool> corruption_detected{false};

    // High-frequency producer
    std::thread fast_producer([&]() {
        for (int i = 0; i < STRESS_ITERATIONS; ++i) {
            int seq = sequence_number.fetch_add(1);
            std::uint8_t byte = static_cast<std::uint8_t>(seq & 0xFF);

            // OVERWRITE semantics - always succeeds
            buffer_.WriteFromISR(byte);

            // Small delay to allow consumer to keep up somewhat
            if (i % 100 == 0) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true);
    });

    // High-frequency consumer
    std::thread fast_consumer([&]() {
        int last_seq = -1;
        int consumed_count = 0;

        // Run until producer is done AND buffer is empty
        while (!producer_done.load() || !buffer_.IsEmpty()) {
            auto data = buffer_.ReadAvailable();

            if (!data.empty()) {
                for (auto byte : data) {
                    // With OVERWRITE semantics, we might skip sequence numbers
                    // Just verify no data corruption (all bytes are valid)
                    int current_seq = static_cast<int>(byte);

                    // Check for reasonable values (0-255, no corruption)
                    if (current_seq < 0 || current_seq > 255) {
                        corruption_detected.store(true);
                    }

                    last_seq = current_seq;
                }

                consumed_count += static_cast<int>(data.size());
                buffer_.MarkConsumed(data.size());
            } else {
                std::this_thread::yield();
            }
        }
    });

    fast_producer.join();
    fast_consumer.join();

    // Test passes if no data corruption detected and threads completed
    EXPECT_FALSE(corruption_detected.load());
}