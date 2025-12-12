// Tests for platform detection and feature availability
// Validates platform identification and capability queries

#include <gtest/gtest.h>

#include "hm11/platform/platform.hpp"

namespace hm11::platform::test {

class PlatformTest : public ::testing::Test {
  protected:
    void SetUp() override {
        platform_type_ = Platform::GetPlatformType();
        platform_name_ = Platform::GetPlatformName();
    }

    Platform::Type platform_type_;
    const char* platform_name_;
};

// Test platform type detection
TEST_F(PlatformTest, PlatformTypeDetection) {
    // Platform type should be one of the defined types
    EXPECT_TRUE(
        platform_type_ == Platform::Type::ARM_CORTEX_M4 || platform_type_ == Platform::Type::ARM_CORTEX_M7 ||
        platform_type_ == Platform::Type::LINUX_X86_64 || platform_type_ == Platform::Type::OTHER);
}

// Test platform name strings
TEST_F(PlatformTest, PlatformNameStrings) {
    // Platform name should not be null
    EXPECT_NE(platform_name_, nullptr);

    // Platform name should match platform type
    switch (platform_type_) {
        case Platform::Type::ARM_CORTEX_M4:
            EXPECT_STREQ(platform_name_, "ARM Cortex-M4");
            break;
        case Platform::Type::ARM_CORTEX_M7:
            EXPECT_STREQ(platform_name_, "ARM Cortex-M7");
            break;
        case Platform::Type::LINUX_X86_64:
            EXPECT_STREQ(platform_name_, "Linux x86_64");
            break;
        case Platform::Type::OTHER:
            EXPECT_STREQ(platform_name_, "Other/Unknown");
            break;
    }
}

// Test feature availability - data cache
TEST_F(PlatformTest, DataCacheFeature) {
    bool has_cache = Platform::HasDataCache();

    switch (platform_type_) {
        case Platform::Type::ARM_CORTEX_M7:
            EXPECT_TRUE(has_cache) << "Cortex-M7 should have data cache";
            break;
        case Platform::Type::ARM_CORTEX_M4:
        case Platform::Type::LINUX_X86_64:
        case Platform::Type::OTHER:
            EXPECT_FALSE(has_cache) << "Platform should not have data cache";
            break;
    }
}

// Test feature availability - wait for event
TEST_F(PlatformTest, WaitForEventFeature) {
    bool has_wfe = Platform::HasWaitForEvent();

    switch (platform_type_) {
        case Platform::Type::ARM_CORTEX_M4:
        case Platform::Type::ARM_CORTEX_M7:
            EXPECT_TRUE(has_wfe) << "ARM Cortex-M should support WFE/SEV";
            break;
        case Platform::Type::LINUX_X86_64:
        case Platform::Type::OTHER:
            EXPECT_FALSE(has_wfe) << "Non-ARM platform should not support WFE/SEV";
            break;
    }
}

// Test feature availability - futex
TEST_F(PlatformTest, FutexFeature) {
    bool has_futex = Platform::HasFutex();

    switch (platform_type_) {
        case Platform::Type::LINUX_X86_64:
            EXPECT_TRUE(has_futex) << "Linux should support futex operations";
            break;
        case Platform::Type::ARM_CORTEX_M4:
        case Platform::Type::ARM_CORTEX_M7:
        case Platform::Type::OTHER:
            EXPECT_FALSE(has_futex) << "Non-Linux platform should not support futex";
            break;
    }
}

// Test embedded platform detection
TEST_F(PlatformTest, EmbeddedPlatformDetection) {
    bool is_embedded = Platform::IsEmbedded();

    switch (platform_type_) {
        case Platform::Type::ARM_CORTEX_M4:
        case Platform::Type::ARM_CORTEX_M7:
            EXPECT_TRUE(is_embedded) << "ARM Cortex-M should be detected as embedded";
            break;
        case Platform::Type::LINUX_X86_64:
        case Platform::Type::OTHER:
            EXPECT_FALSE(is_embedded) << "Non-ARM platform should not be embedded";
            break;
    }
}

// Test memory barrier operations (should not crash)
TEST_F(PlatformTest, MemoryBarrierOperations) {
    // These should execute without throwing exceptions
    EXPECT_NO_THROW(MemoryBarrier::DataMemoryBarrier());
    EXPECT_NO_THROW(MemoryBarrier::DataSynchronizationBarrier());
    EXPECT_NO_THROW(MemoryBarrier::InstructionSynchronizationBarrier());
}

// Test platform initialization (should be safe to call multiple times)
TEST_F(PlatformTest, PlatformInitialization) {
    // Should be safe to call multiple times
    EXPECT_NO_THROW(Initialize::PlatformFeatures());
    EXPECT_NO_THROW(Initialize::PlatformFeatures());
    EXPECT_NO_THROW(Initialize::PlatformFeatures());
}

// Test compile-time constexpr evaluation
TEST_F(PlatformTest, CompileTimeEvaluation) {
    // These should all be constexpr and evaluable at compile time
    constexpr auto platform = Platform::GetPlatformType();
    constexpr bool has_cache = Platform::HasDataCache();
    constexpr bool has_wfe = Platform::HasWaitForEvent();
    constexpr bool has_futex = Platform::HasFutex();
    constexpr bool is_embedded = Platform::IsEmbedded();
    constexpr const char* name = Platform::GetPlatformName();

    // Use the constexpr values to verify they compiled
    EXPECT_EQ(platform, Platform::GetPlatformType());
    EXPECT_EQ(has_cache, Platform::HasDataCache());
    EXPECT_EQ(has_wfe, Platform::HasWaitForEvent());
    EXPECT_EQ(has_futex, Platform::HasFutex());
    EXPECT_EQ(is_embedded, Platform::IsEmbedded());
    EXPECT_STREQ(name, Platform::GetPlatformName());
}

// Test feature consistency
TEST_F(PlatformTest, FeatureConsistency) {
    // Only one wait mechanism should be available
    bool has_wfe = Platform::HasWaitForEvent();
    bool has_futex = Platform::HasFutex();

    // For current platforms, these should be mutually exclusive
    // (ARM uses WFE/SEV, Linux uses futex)
    if (Platform::IsEmbedded()) {
        EXPECT_TRUE(has_wfe);
        EXPECT_FALSE(has_futex);
    } else if (platform_type_ == Platform::Type::LINUX_X86_64) {
        EXPECT_FALSE(has_wfe);
        EXPECT_TRUE(has_futex);
    }

    // Data cache is only available on Cortex-M7
    if (Platform::HasDataCache()) {
        EXPECT_EQ(platform_type_, Platform::Type::ARM_CORTEX_M7);
    }
}

// Performance test - platform detection should be compile-time
TEST_F(PlatformTest, PerformanceBaseline) {
    const size_t iterations = 10000; // Reduced for CI stability

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        volatile auto platform = Platform::GetPlatformType();
        volatile bool has_cache = Platform::HasDataCache();
        volatile bool is_embedded = Platform::IsEmbedded();
        (void)platform; // Suppress unused variable warnings
        (void)has_cache;
        (void)is_embedded;
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Platform detection should be very fast (compile-time constants)
    // Very generous timeout for CI environment variability
    EXPECT_LT(duration.count(), 50000) << "Platform detection too slow: " << duration.count() << " μs";
}

} // namespace hm11::platform::test