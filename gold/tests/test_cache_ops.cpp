/**
 * @file test_cache_ops.cpp
 * @brief Unit tests for cache coherency operations
 *
 * These tests verify that cache operations compile correctly for both Cortex-M4
 * and Cortex-M7 targets. Since we're testing on x86 Linux (non-ARM), the cache
 * operations will compile as no-ops, but this validates the API and conditional
 * compilation logic.
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <hm11/platform/cache_ops.hpp>

namespace {

// Test fixture for cache operations
class CacheOpsTest : public ::testing::Test {
  protected:
    // Aligned buffer for testing cache operations
    alignas(32) std::array<uint8_t, 256> buffer{};
};

// Verify cache operations compile and execute without error (no-op on x86)
TEST_F(CacheOpsTest, InvalidateDCacheRange_CompileAndExecute) {
    // Should compile and execute as no-op on non-Cortex-M platforms
    hm11::platform::invalidate_dcache_range(buffer.data(), buffer.size());

    // Verify constant is accessible
    EXPECT_EQ(hm11::platform::DCACHE_LINE_SIZE, 32);
}

TEST_F(CacheOpsTest, CleanDCacheRange_CompileAndExecute) {
    // Should compile and execute as no-op on non-Cortex-M platforms
    hm11::platform::clean_dcache_range(buffer.data(), buffer.size());
}

TEST_F(CacheOpsTest, CleanInvalidateDCacheRange_CompileAndExecute) {
    // Should compile and execute as no-op on non-Cortex-M platforms
    hm11::platform::clean_invalidate_dcache_range(buffer.data(), buffer.size());
}

// Test null pointer safety
TEST_F(CacheOpsTest, NullPointerSafety) {
    // These should not crash (handled by null check in implementation)
    hm11::platform::invalidate_dcache_range(nullptr, 0);
    hm11::platform::clean_dcache_range(nullptr, 0);
    hm11::platform::clean_invalidate_dcache_range(nullptr, 0);
}

// Test zero size safety
TEST_F(CacheOpsTest, ZeroSizeSafety) {
    // These should not crash (handled by size check in implementation)
    hm11::platform::invalidate_dcache_range(buffer.data(), 0);
    hm11::platform::clean_dcache_range(buffer.data(), 0);
    hm11::platform::clean_invalidate_dcache_range(buffer.data(), 0);
}

// Test various buffer sizes
TEST_F(CacheOpsTest, VariousBufferSizes) {
    // Small buffer (less than cache line)
    std::array<uint8_t, 16> small_buffer{};
    hm11::platform::invalidate_dcache_range(small_buffer.data(), small_buffer.size());
    hm11::platform::clean_dcache_range(small_buffer.data(), small_buffer.size());

    // Exactly one cache line
    std::array<uint8_t, 32> line_buffer{};
    hm11::platform::invalidate_dcache_range(line_buffer.data(), line_buffer.size());
    hm11::platform::clean_dcache_range(line_buffer.data(), line_buffer.size());

    // Multiple cache lines
    std::array<uint8_t, 128> multi_line_buffer{};
    hm11::platform::invalidate_dcache_range(multi_line_buffer.data(), multi_line_buffer.size());
    hm11::platform::clean_dcache_range(multi_line_buffer.data(), multi_line_buffer.size());
}

// Test unaligned buffers (CMSIS handles alignment internally)
TEST_F(CacheOpsTest, UnalignedBuffers) {
    std::array<uint8_t, 256> unaligned_buffer{};

    // Test with various offsets from alignment
    for (std::size_t offset = 0; offset < 32; ++offset) {
        void* unaligned_addr = unaligned_buffer.data() + offset;
        std::size_t remaining_size = unaligned_buffer.size() - offset;

        hm11::platform::invalidate_dcache_range(unaligned_addr, remaining_size);
        hm11::platform::clean_dcache_range(unaligned_addr, remaining_size);
        hm11::platform::clean_invalidate_dcache_range(unaligned_addr, remaining_size);
    }
}

// Verify noexcept specification
TEST(CacheOpsCompileTimeTest, NoexceptSpecification) {
    // These should all be noexcept for ISR safety
    static_assert(noexcept(hm11::platform::invalidate_dcache_range(nullptr, 0)));
    static_assert(noexcept(hm11::platform::clean_dcache_range(nullptr, 0)));
    static_assert(noexcept(hm11::platform::clean_invalidate_dcache_range(nullptr, 0)));
}

// Verify constant is constexpr
TEST(CacheOpsCompileTimeTest, ConstexprConstant) {
    constexpr std::size_t cache_line_size = hm11::platform::DCACHE_LINE_SIZE;
    EXPECT_EQ(cache_line_size, 32);

    // Can use in array size (compile-time constant)
    std::array<uint8_t, cache_line_size> aligned_buffer{};
    EXPECT_EQ(aligned_buffer.size(), 32);
}

} // namespace
