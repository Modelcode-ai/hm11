// Tests for DMA cache operations and memory alignment utilities
// Validates cache management functions and alignment checks across platforms

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

#include "hm11/platform/dma_cache.hpp"

namespace hm11::platform::test {

class DmaCacheTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize test buffers with known data
        test_buffer_.fill(0xAA);
        aligned_buffer_ = std::make_unique<std::uint8_t[]>(128);
        std::fill(aligned_buffer_.get(), aligned_buffer_.get() + 128, 0xBB);
    }

    void TearDown() override {
        // Clean up resources
        aligned_buffer_.reset();
    }

    // Test data buffers
    std::array<std::uint8_t, 64> test_buffer_;
    std::unique_ptr<std::uint8_t[]> aligned_buffer_;
};

// Test cache operations with nullptr/zero size
TEST_F(DmaCacheTest, HandlesNullptrAndZeroSize) {
    // These should not crash or cause undefined behavior
    EXPECT_NO_THROW(dma_cache::CleanForTransmit(nullptr, 64));
    EXPECT_NO_THROW(dma_cache::CleanForTransmit(test_buffer_.data(), 0));
    EXPECT_NO_THROW(dma_cache::InvalidateAfterReceive(nullptr, 64));
    EXPECT_NO_THROW(dma_cache::InvalidateAfterReceive(test_buffer_.data(), 0));
    EXPECT_NO_THROW(dma_cache::CleanAndInvalidate(nullptr, 64));
    EXPECT_NO_THROW(dma_cache::CleanAndInvalidate(test_buffer_.data(), 0));
}

// Test cache operations with valid buffers
TEST_F(DmaCacheTest, OperatesOnValidBuffers) {
    // Operations should complete without throwing
    EXPECT_NO_THROW(dma_cache::CleanForTransmit(test_buffer_.data(), test_buffer_.size()));
    EXPECT_NO_THROW(dma_cache::InvalidateAfterReceive(test_buffer_.data(), test_buffer_.size()));
    EXPECT_NO_THROW(dma_cache::CleanAndInvalidate(test_buffer_.data(), test_buffer_.size()));
}

// Test template convenience functions
TEST_F(DmaCacheTest, TemplateConvenienceFunctions) {
    std::array<std::uint32_t, 16> int_buffer;
    int_buffer.fill(0x12345678);

    // Template functions should work with typed buffers
    EXPECT_NO_THROW(dma_cache::CleanForTransmit(int_buffer.data(), int_buffer.size()));
    EXPECT_NO_THROW(dma_cache::InvalidateAfterReceive(int_buffer.data(), int_buffer.size()));
    EXPECT_NO_THROW(dma_cache::CleanAndInvalidate(int_buffer.data(), int_buffer.size()));

    // Single element versions
    std::uint32_t single_value = 0xDEADBEEF;
    EXPECT_NO_THROW(dma_cache::CleanForTransmit(&single_value));
    EXPECT_NO_THROW(dma_cache::InvalidateAfterReceive(&single_value));
    EXPECT_NO_THROW(dma_cache::CleanAndInvalidate(&single_value));
}

// Test memory alignment checking functions
TEST_F(DmaCacheTest, AlignmentChecking) {
    // Test address alignment checking
    auto aligned_ptr = reinterpret_cast<void*>(0x1000);   // 32-byte aligned
    auto unaligned_ptr = reinterpret_cast<void*>(0x1001); // Not aligned

    EXPECT_TRUE(dma_alignment::IsAligned(aligned_ptr));
    EXPECT_FALSE(dma_alignment::IsAligned(unaligned_ptr));

    // Test size alignment checking
    EXPECT_TRUE(dma_alignment::IsAlignedSize(32));  // Cache line size
    EXPECT_TRUE(dma_alignment::IsAlignedSize(64));  // Multiple of cache line
    EXPECT_FALSE(dma_alignment::IsAlignedSize(33)); // Not aligned
    EXPECT_FALSE(dma_alignment::IsAlignedSize(0));  // Zero size

    // Test combined buffer alignment
    std::uint32_t* typed_aligned = reinterpret_cast<std::uint32_t*>(0x1000);
    std::uint32_t* typed_unaligned = reinterpret_cast<std::uint32_t*>(0x1001);

    EXPECT_TRUE(dma_alignment::IsBufferAligned(typed_aligned, 8));    // 32 bytes total
    EXPECT_FALSE(dma_alignment::IsBufferAligned(typed_unaligned, 8)); // Unaligned address
    EXPECT_FALSE(dma_alignment::IsBufferAligned(typed_aligned, 7));   // 28 bytes, not aligned
}

// Test alignment calculation functions
TEST_F(DmaCacheTest, AlignmentCalculations) {
    // Test address alignment
    EXPECT_EQ(dma_alignment::AlignUp(0x1000), 0x1000U); // Already aligned
    EXPECT_EQ(dma_alignment::AlignUp(0x1001), 0x1020U); // Round up to next boundary
    EXPECT_EQ(dma_alignment::AlignUp(0x101F), 0x1020U); // Round up to next boundary
    EXPECT_EQ(dma_alignment::AlignUp(0x1020), 0x1020U); // Already aligned

    // Test size alignment
    EXPECT_EQ(dma_alignment::AlignSize(32), 32U); // Already aligned
    EXPECT_EQ(dma_alignment::AlignSize(33), 64U); // Round up to next multiple
    EXPECT_EQ(dma_alignment::AlignSize(63), 64U); // Round up to next multiple
    EXPECT_EQ(dma_alignment::AlignSize(64), 64U); // Already aligned
    EXPECT_EQ(dma_alignment::AlignSize(1), 32U);  // Minimum alignment
}

// Test cache line size constant
TEST_F(DmaCacheTest, CacheLineSizeConstant) {
    // Cache line size should be 32 bytes for ARM Cortex-M7
    EXPECT_EQ(CACHE_LINE_SIZE, 32U);

    // Should be power of 2 for efficient alignment calculations
    EXPECT_EQ(CACHE_LINE_SIZE & (CACHE_LINE_SIZE - 1), 0U);
}

// Performance test - cache operations should be fast
TEST_F(DmaCacheTest, PerformanceBaseline) {
    const size_t iterations = 1000;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        dma_cache::CleanForTransmit(test_buffer_.data(), test_buffer_.size());
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Cache operations should be reasonably fast (platform dependent)
    // On platforms without cache, these are no-ops and should be very fast
    // On cached platforms, expect sub-microsecond per operation
    // CI environments may have timing variability
    EXPECT_LT(duration.count(), 50000) << "Cache operations too slow: " << duration.count() << " μs";
}

} // namespace hm11::platform::test