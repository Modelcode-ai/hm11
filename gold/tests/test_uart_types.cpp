/**
 * @file test_uart_types.cpp
 * @brief Unit tests for UART status and data types
 *
 * This test file verifies that:
 * - UART status types are compatible with std::atomic
 * - UART data buffer types work with different container types
 * - Type conversions and usage patterns work as expected
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <vector>

#include "hm11/hal/base_types.hpp"

using namespace hm11::hal;

// ============================================================================
// UartStatus Tests
// ============================================================================

TEST(UartStatusTest, EnumValues) {
    // Verify enum values match Ada specification
    EXPECT_EQ(static_cast<uint8_t>(UartStatus::Ok), 0);
    EXPECT_EQ(static_cast<uint8_t>(UartStatus::ErrError), 1);
    EXPECT_EQ(static_cast<uint8_t>(UartStatus::ErrTimeout), 2);
    EXPECT_EQ(static_cast<uint8_t>(UartStatus::Busy), 3);
}

TEST(UartStatusTest, AtomicCompatibility) {
    // Verify UartStatus can be used with std::atomic
    std::atomic<UartStatus> atomic_status{UartStatus::Busy};

    // Store operation
    atomic_status.store(UartStatus::Ok, std::memory_order_release);

    // Load operation
    UartStatus status = atomic_status.load(std::memory_order_acquire);
    EXPECT_EQ(status, UartStatus::Ok);

    // Exchange operation
    UartStatus old_status = atomic_status.exchange(UartStatus::ErrTimeout, std::memory_order_acq_rel);
    EXPECT_EQ(old_status, UartStatus::Ok);
    EXPECT_EQ(atomic_status.load(std::memory_order_relaxed), UartStatus::ErrTimeout);
}

TEST(UartStatusTest, Comparison) {
    // Verify comparison operations work
    UartStatus ok = UartStatus::Ok;
    UartStatus err = UartStatus::ErrError;

    EXPECT_EQ(ok, UartStatus::Ok);
    EXPECT_NE(ok, err);
    EXPECT_NE(err, UartStatus::Ok);
}

// ============================================================================
// UartDataSize Tests
// ============================================================================

TEST(UartDataSizeTest, EnumValues) {
    // Verify enum values match Ada specification
    EXPECT_EQ(static_cast<uint8_t>(UartDataSize::DataSize8b), 0);
    EXPECT_EQ(static_cast<uint8_t>(UartDataSize::DataSize9b), 1);
}

TEST(UartDataSizeTest, AtomicCompatibility) {
    // Verify UartDataSize can be used with std::atomic (even though not currently used)
    std::atomic<UartDataSize> atomic_size{UartDataSize::DataSize8b};

    atomic_size.store(UartDataSize::DataSize9b, std::memory_order_release);
    UartDataSize size = atomic_size.load(std::memory_order_acquire);
    EXPECT_EQ(size, UartDataSize::DataSize9b);
}

// ============================================================================
// UART Data Buffer Type Tests
// ============================================================================

TEST(UartDataBufferTest, UartData8bWithArray) {
    // Test mutable buffer with std::array
    std::array<UInt8, 5> buffer = {1, 2, 3, 4, 5};
    UartData8b view(buffer);

    EXPECT_EQ(view.size(), 5);
    EXPECT_EQ(view[0], 1);
    EXPECT_EQ(view[4], 5);

    // Modify through span
    view[2] = 42;
    EXPECT_EQ(buffer[2], 42);
}

TEST(UartDataBufferTest, UartData8bWithVector) {
    // Test mutable buffer with std::vector
    std::vector<UInt8> buffer = {10, 20, 30};
    UartData8b view(buffer);

    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view[1], 20);

    // Modify through span
    view[1] = 99;
    EXPECT_EQ(buffer[1], 99);
}

TEST(UartDataBufferTest, UartData8bWithCArray) {
    // Test mutable buffer with C-style array
    std::array<UInt8, 4> buffer = {0xAA, 0xBB, 0xCC, 0xDD};
    UartData8b view(buffer);

    EXPECT_EQ(view.size(), 4);
    EXPECT_EQ(view[0], 0xAA);
    EXPECT_EQ(view[3], 0xDD);
}

TEST(UartDataBufferTest, UartData8bConstWithArray) {
    // Test read-only buffer with const array
    const std::array<UInt8, 3> RX_BUFFER = {0x01, 0x02, 0x03};
    UartData8bConst view(RX_BUFFER);

    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view[0], 0x01);
    EXPECT_EQ(view[2], 0x03);

    // Verify it's read-only (compile-time check - this would fail to compile):
    // view[0] = 42;  // Error: assignment of read-only location
}

TEST(UartDataBufferTest, UartData8bConstFromMutable) {
    // Test that mutable buffer can be passed as const
    std::array<UInt8, 4> buffer = {0x10, 0x20, 0x30, 0x40};
    UartData8bConst const_view(buffer); // Implicit conversion

    EXPECT_EQ(const_view.size(), 4);
    EXPECT_EQ(const_view[1], 0x20);
}

TEST(UartDataBufferTest, UartData8bIteration) {
    // Test iteration over buffer
    std::array<UInt8, 5> buffer = {1, 2, 3, 4, 5};
    UartData8b view(buffer);

    UInt8 sum = 0;
    for (UInt8 byte : view) {
        sum += byte;
    }
    EXPECT_EQ(sum, 15);
}

TEST(UartDataBufferTest, UartData9bWithArray) {
    // Test 9-bit buffer (for completeness, not used in HM-11)
    std::array<UInt16, 3> buffer = {0x01FF, 0x0100, 0x0001}; // 9-bit values
    UartData9b view(buffer);

    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view[0], 0x01FF);
    EXPECT_EQ(view[2], 0x0001);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(UartTypesIntegration, StatusReportingPattern) {
    // Simulate typical UART status reporting pattern
    std::atomic<UartStatus> status{UartStatus::Busy};

    // Simulate ISR completing operation successfully
    status.store(UartStatus::Ok, std::memory_order_release);

    // Application thread checks status
    UartStatus result = status.load(std::memory_order_acquire);
    EXPECT_EQ(result, UartStatus::Ok);
}

TEST(UartTypesIntegration, BufferPassing) {
    // Simulate passing buffer to a function expecting UartData8bConst
    auto process_buffer = [](UartData8bConst data) -> size_t {
        size_t count = 0;
        for (UInt8 byte : data) {
            if (byte != 0) {
                ++count;
            }
        }
        return count;
    };

    // Can pass various container types
    std::array<UInt8, 5> array_buf = {1, 0, 2, 0, 3};
    EXPECT_EQ(process_buffer(array_buf), 3);

    std::vector<UInt8> vec_buf = {1, 2, 3, 4};
    EXPECT_EQ(process_buffer(vec_buf), 4);

    std::array<UInt8, 3> c_array = {1, 1, 1};
    EXPECT_EQ(process_buffer(c_array), 3);
}
