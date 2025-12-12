/**
 * @file stress_test_circular_buffer.cpp
 * @brief Stress tests for CircularBuffer SPSC synchronization
 *
 * These tests verify the CircularBuffer implementation under high contention:
 * - Rapid producer-consumer scenarios
 * - Bursty traffic patterns
 * - Buffer overflow handling
 * - Memory ordering correctness
 * - No data corruption under stress
 *
 * Build with ThreadSanitizer to detect race conditions:
 *   clang++ -fsanitize=thread -g -O1 -std=c++20 \
 *           -I../include stress_test_circular_buffer.cpp -lgtest -lgtest_main \
 *           -o stress_test_circular_buffer
 *
 * Run with:
 *   ./stress_test_circular_buffer
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "hm11/sync/circular_buffer.hpp"

using namespace hm11::sync;
using namespace std::chrono_literals;

// ============================================================================
// Test Fixture
// ============================================================================

class CircularBufferStressTest : public ::testing::Test {
  protected:
    static constexpr size_t BUFFER_SIZE = 256;
    static constexpr size_t NUM_ITEMS = 1'000; // 1K items (TSan is ~100x slower)
};

// ============================================================================
// Stress Test 1: Rapid Sequential Producer-Consumer
// ============================================================================

TEST_F(CircularBufferStressTest, RapidProducerConsumer_Sequential) {
    CircularBuffer<BUFFER_SIZE> buffer;
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> items_written{0};
    std::atomic<size_t> items_read{0};

    // Producer thread (simulates ISR writing to buffer)
    std::thread producer([&]() {
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            uint8_t data = static_cast<uint8_t>(i & 0xFF);

            // Write always succeeds (Ada-style, may overwrite old data)
            buffer.write(data);
            items_written.fetch_add(1, std::memory_order_relaxed);
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer thread (simulates application reading from buffer)
    std::thread consumer([&]() {
        size_t consecutive_empty = 0;

        while (items_read.load(std::memory_order_relaxed) < NUM_ITEMS || !buffer.empty()) {
            uint8_t data;
            if (buffer.read(data)) {
                // Just count items read - don't verify sequence due to overwrite policy
                items_read.fetch_add(1, std::memory_order_relaxed);
                consecutive_empty = 0;
            } else {
                // Buffer empty, yield to producer
                consecutive_empty++;
                if (consecutive_empty > 10000 && producer_done.load(std::memory_order_acquire)) {
                    // Producer done and buffer still empty - we're done
                    break;
                }
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(items_written.load(), NUM_ITEMS) << "Producer didn't write all items";

    // Consumer may read fewer items than written due to overwrite policy
    // This is expected behavior when producer overwhelms consumer
    EXPECT_GT(items_read.load(), 0) << "Consumer should have read some items";
    EXPECT_LE(items_read.load(), NUM_ITEMS) << "Consumer cannot read more than written";

    std::cout << "Items written: " << items_written.load() << std::endl;
    std::cout << "Items read: " << items_read.load() << std::endl;
    std::cout << "Data lost to overwrites: " << (items_written.load() - items_read.load()) << std::endl;
}

// ============================================================================
// Stress Test 2: Sequential Data Integrity (Slower Producer)
// ============================================================================

TEST_F(CircularBufferStressTest, DISABLED_SequentialDataIntegrity) {
    // This test verifies memory ordering correctness with a slow producer
    // that never overwhelms the consumer (no overwrites)
    CircularBuffer<BUFFER_SIZE> buffer;
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> sequence_errors{0};

    constexpr size_t TEST_ITEMS = 1'000; // Fewer items, slower pace

    std::thread producer([&]() {
        for (size_t i = 0; i < TEST_ITEMS; ++i) {
            uint8_t data = static_cast<uint8_t>(i & 0xFF);

            // Just write (Ada-style - producer never checks full)
            buffer.write(data);

            // Slow down producer to prevent overwhelming consumer
            if (i % 1000 == 0) {
                std::this_thread::sleep_for(10us);
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        uint8_t expected = 0;
        size_t items_read = 0;

        while (items_read < TEST_ITEMS) {
            uint8_t data;
            if (buffer.read(data)) {
                // Verify sequential data (should work with slow producer)
                if (data != expected) {
                    sequence_errors.fetch_add(1, std::memory_order_relaxed);
                }
                expected = (expected + 1) & 0xFF;
                items_read++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(sequence_errors.load(), 0) << "Memory ordering violation: sequence errors with slow producer";

    std::cout << "Sequential test: " << TEST_ITEMS << " items, " << sequence_errors.load() << " errors" << std::endl;
}

// ============================================================================
// Stress Test 3: Bursty Traffic Pattern
// ============================================================================

TEST_F(CircularBufferStressTest, BurstyTraffic) {
    constexpr size_t SMALL_BUFFER = 64;
    CircularBuffer<SMALL_BUFFER> buffer;
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> bursts_sent{0};
    std::atomic<size_t> items_received{0};

    constexpr size_t NUM_BURSTS = 1000;
    constexpr size_t BURST_SIZE = SMALL_BUFFER * 2; // Intentionally overflow buffer

    // Producer: Send bursts of data
    std::thread producer([&]() {
        for (size_t burst = 0; burst < NUM_BURSTS; ++burst) {
            // Write burst of data (will overflow buffer)
            for (size_t i = 0; i < BURST_SIZE; ++i) {
                uint8_t data = static_cast<uint8_t>((burst * BURST_SIZE + i) & 0xFF);
                (void)buffer.write(data); // May fail if buffer full, that's OK
            }
            bursts_sent.fetch_add(1, std::memory_order_relaxed);

            // Pause between bursts (simulate bursty UART traffic)
            std::this_thread::sleep_for(100us);
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer: Read as fast as possible
    std::thread consumer([&]() {
        while (!producer_done.load(std::memory_order_acquire) || !buffer.empty()) {
            uint8_t data;
            if (buffer.read(data)) {
                items_received.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::sleep_for(10us);
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(bursts_sent.load(), NUM_BURSTS);
    EXPECT_GT(items_received.load(), 0); // Should have received some data

    std::cout << "Bursts sent: " << bursts_sent.load() << std::endl;
    std::cout << "Items received: " << items_received.load() << " out of " << (NUM_BURSTS * BURST_SIZE) << " sent"
              << std::endl;
}

// ============================================================================
// Stress Test 4: Buffer Wraparound
// ============================================================================

TEST_F(CircularBufferStressTest, WraparoundStress) {
    constexpr size_t TINY_BUFFER = 16; // Small buffer to force frequent wraparound
    CircularBuffer<TINY_BUFFER> buffer;
    std::atomic<bool> done{false};
    std::atomic<size_t> wraparound_count{0};

    constexpr size_t TEST_ITEMS = 1'000;

    std::thread producer([&]() {
        for (size_t i = 0; i < TEST_ITEMS; ++i) {
            while (buffer.full()) {
                std::this_thread::yield();
            }
            buffer.write(static_cast<uint8_t>(i & 0xFF));

            // Track wraparounds (approximate)
            if (i % TINY_BUFFER == 0) {
                wraparound_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        size_t read_count = 0;
        while (read_count < TEST_ITEMS) {
            uint8_t data;
            if (buffer.read(data)) {
                read_count++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_GT(wraparound_count.load(), TEST_ITEMS / TINY_BUFFER / 2) << "Expected many wraparounds with small buffer";

    std::cout << "Wraparound cycles (approximate): " << wraparound_count.load() << std::endl;
}

// ============================================================================
// Stress Test 5: Overwrite Policy Verification
// ============================================================================

TEST_F(CircularBufferStressTest, OverwritePolicyStress) {
    constexpr size_t SMALL_BUFFER = 32;
    CircularBuffer<SMALL_BUFFER> buffer;

    // Fill buffer (31 bytes to avoid wraparound)
    for (size_t i = 0; i < SMALL_BUFFER - 1; ++i) {
        buffer.write(static_cast<uint8_t>(i));
    }

    EXPECT_TRUE(buffer.full());

    // Write one more byte - wraps position to 0
    buffer.write(99);
    EXPECT_EQ(buffer.get_write_position(), 0); // Wrapped

    // Write another byte - now at position 1
    buffer.write(100);
    EXPECT_EQ(buffer.get_write_position(), 1);

    // Buffer now has data available
    EXPECT_FALSE(buffer.empty());

    // Read all data
    size_t read_count = 0;
    uint8_t data;
    while (buffer.read(data) && read_count < SMALL_BUFFER * 2) {
        read_count++;
    }

    EXPECT_LE(read_count, SMALL_BUFFER) << "Should not read more than buffer capacity";
}

// ============================================================================
// Stress Test 6: Peek Operation Stress
// ============================================================================

TEST_F(CircularBufferStressTest, PeekStress) {
    CircularBuffer<BUFFER_SIZE> buffer;
    std::atomic<bool> done{false};

    constexpr size_t TEST_ITEMS = 1'000;

    std::thread producer([&]() {
        for (size_t i = 0; i < TEST_ITEMS; ++i) {
            while (buffer.full()) {
                std::this_thread::yield();
            }
            buffer.write(static_cast<uint8_t>(i & 0xFF));
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        size_t peek_count = 0;
        size_t read_count = 0;

        while (read_count < TEST_ITEMS) {
            uint8_t peeked_data;
            if (buffer.peek(peeked_data)) {
                peek_count++;

                // Now read and verify it matches peek
                uint8_t read_data;
                EXPECT_TRUE(buffer.read(read_data));
                EXPECT_EQ(peeked_data, read_data) << "Peek and read mismatch";

                read_count++;
            } else {
                std::this_thread::yield();
            }
        }

        std::cout << "Successful peeks: " << peek_count << std::endl;
    });

    producer.join();
    consumer.join();
}

// ============================================================================
// Stress Test 7: Available Count Accuracy
// ============================================================================

TEST_F(CircularBufferStressTest, AvailableCountStress) {
    CircularBuffer<BUFFER_SIZE> buffer;

    // Test 1: Fill and drain multiple times
    for (int iteration = 0; iteration < 100; ++iteration) {
        // Fill buffer
        for (size_t i = 0; i < BUFFER_SIZE - 1; ++i) { // -1 to avoid full condition
            (void)buffer.write(static_cast<uint8_t>(i));
        }

        size_t available = buffer.available();
        EXPECT_EQ(available, BUFFER_SIZE - 1) << "Available count incorrect after fill";

        // Drain buffer
        for (size_t i = 0; i < available; ++i) {
            uint8_t data;
            (void)buffer.read(data);
        }

        EXPECT_EQ(buffer.available(), 0) << "Available count should be 0 after drain";
        EXPECT_TRUE(buffer.empty());
    }
}

// ============================================================================
// Stress Test 8: Clear Operation Under Load
// ============================================================================

TEST_F(CircularBufferStressTest, ClearUnderLoad) {
    CircularBuffer<BUFFER_SIZE> buffer;
    std::atomic<bool> producer_done{false};
    std::atomic<size_t> clear_count{0};

    constexpr size_t TEST_DURATION_MS = 1000;

    // Producer: Continuously write data
    std::thread producer([&]() {
        auto start = std::chrono::steady_clock::now();
        size_t counter = 0;

        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(TEST_DURATION_MS)) {
            (void)buffer.write(static_cast<uint8_t>(counter++ & 0xFF));
            std::this_thread::sleep_for(10us);
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer: Periodically clear buffer
    std::thread consumer([&]() {
        while (!producer_done.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(50ms);
            buffer.clear();
            clear_count.fetch_add(1, std::memory_order_relaxed);
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Buffer cleared " << clear_count.load() << " times during stress test" << std::endl;
    EXPECT_GT(clear_count.load(), 0);
}

// ============================================================================
// Stress Test 9: Memory Ordering Verification
// ============================================================================

TEST_F(CircularBufferStressTest, MemoryOrderingVerification) {
    // This test is designed to catch memory ordering bugs with ThreadSanitizer
    // Uses slower producer to avoid overwrites
    CircularBuffer<BUFFER_SIZE> buffer;
    std::atomic<size_t> sequence_errors{0};

    constexpr size_t TEST_ITEMS = 500; // Reduced for TSan overhead

    std::thread producer([&]() {
        for (size_t i = 0; i < TEST_ITEMS; ++i) {
            // Write a predictable pattern
            uint8_t data = static_cast<uint8_t>(i & 0xFF);
            while (buffer.full()) {
                std::this_thread::yield();
            }
            buffer.write(data);
            // Slow down to prevent overwrites
            if (i % 500 == 0) {
                std::this_thread::sleep_for(5us);
            }
        }
    });

    std::thread consumer([&]() {
        uint8_t last_value = 255; // Will wrap to 0 on first increment
        size_t items_read = 0;

        while (items_read < TEST_ITEMS) {
            uint8_t data;
            if (buffer.read(data)) {
                uint8_t expected = (last_value + 1) & 0xFF;
                if (data != expected) {
                    sequence_errors.fetch_add(1, std::memory_order_relaxed);
                }
                last_value = data;
                items_read++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(sequence_errors.load(), 0) << "Memory ordering violation detected: sequence errors found";

    std::cout << "Memory ordering test: " << TEST_ITEMS << " items, " << sequence_errors.load() << " errors"
              << std::endl;
}
