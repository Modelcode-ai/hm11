/**
 * @file alignment_test.cpp
 * @brief Unit tests for platform alignment utilities
 *
 * This file is part of the HM11 driver C++ migration project.
 * It tests the platform-specific alignment utilities.
 */

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

#include "hm11/platform/alignment.hpp"
#include "hm11/platform/cache.hpp"

// Test fixture for platform alignment tests
class AlignmentTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

// Test DMA buffer alignment
TEST_F(AlignmentTest, DmaBufferAlignment) {
    // Create a DMA aligned buffer
    hm11::platform::DmaAlignedBuffer<uint8_t, 100> buffer;

    // Verify the buffer size
    EXPECT_EQ(buffer.size(), 100);
    EXPECT_EQ(buffer.byte_size(), 100);

    // Verify the buffer alignment
    uintptr_t address = reinterpret_cast<uintptr_t>(buffer.to_array().data());
    EXPECT_EQ(address % hm11::platform::DMA_ALIGNMENT, 0);

    // Test is_dma_aligned utility
    EXPECT_TRUE(hm11::platform::is_dma_aligned(buffer.to_array().data()));

    // Test assert_dma_aligned utility (should not throw/assert in normal case)
    hm11::platform::assert_dma_aligned(buffer.to_array().data());
}

// Test DMA buffer access
TEST_F(AlignmentTest, DmaBufferAccess) {
    // Create a DMA aligned buffer
    hm11::platform::DmaAlignedBuffer<uint32_t, 10> buffer;

    // Write to buffer
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] = static_cast<uint32_t>(i);
    }

    // Verify buffer contents
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], i);
    }

    // Test fill method
    buffer.fill(42);
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], 42);
    }

    // Test clear method
    buffer.clear();
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], 0);
    }

    // Test cache operations (these should be no-ops in test environment)
    buffer.invalidate_cache();
    buffer.clean_cache();
    buffer.clean_and_invalidate_cache();
}

// Test dynamic DMA buffer
TEST_F(AlignmentTest, DynamicDmaBufferTest) {
    // Create a dynamic DMA buffer
    hm11::platform::DynamicDmaBuffer<uint8_t> buffer(200);

    // Check size
    EXPECT_EQ(buffer.size(), 200);
    EXPECT_EQ(buffer.byte_size(), 200);

    // Check alignment
    EXPECT_TRUE(hm11::platform::is_dma_aligned(buffer.to_array().data()));

    // Test fill and access
    buffer.fill(0xFF);
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], 0xFF);
    }

    // Test modification
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Verify contents
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], static_cast<uint8_t>(i & 0xFF));
    }

    // Test resize
    buffer.resize(300);
    EXPECT_EQ(buffer.size(), 300);
    EXPECT_TRUE(hm11::platform::is_dma_aligned(buffer.to_array().data()));

    // Test clear
    buffer.clear();
    for (size_t i = 0; i < buffer.size(); i++) {
        EXPECT_EQ(buffer[i], 0);
    }

    // Test cache operations (these should be no-ops in test environment)
    buffer.invalidate_cache();
    buffer.clean_cache();
    buffer.clean_and_invalidate_cache();
}

// Test alignment utilities
TEST_F(AlignmentTest, AlignmentUtilities) {
    // Test is_aligned with aligned and unaligned pointers
    alignas(32) uint8_t aligned_data[64];
    uint8_t unaligned_data[64];

    // Aligned data should pass alignment check
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 32));
    EXPECT_TRUE(hm11::platform::is_dma_aligned(aligned_data));

    // Unaligned data might pass by coincidence, but we can't reliably test that it fails
    // We can only test the API exists and doesn't crash

    // Test with different alignments
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 16));
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 8));
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 4));
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 2));
    EXPECT_TRUE(hm11::platform::cache::is_aligned(aligned_data, 1));

    // Test with null pointer (should be aligned to any power of 2)
    EXPECT_TRUE(hm11::platform::cache::is_aligned(nullptr, 32));
}

// Test cache operation functions exist and don't crash
TEST_F(AlignmentTest, CacheOperations) {
    alignas(32) uint8_t aligned_data[64];

    // These should be no-ops in test environment
    hm11::platform::cache::invalidate_dma_buffer(aligned_data, sizeof(aligned_data));
    hm11::platform::cache::clean_dma_buffer(aligned_data, sizeof(aligned_data));
    hm11::platform::cache::clean_and_invalidate_dma_buffer(aligned_data, sizeof(aligned_data));
}