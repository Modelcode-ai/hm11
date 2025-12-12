/**
 * @file cache_ops_compile_test.cpp
 * @brief Compilation test for cache operations on embedded targets
 *
 * This file verifies that cache operations compile correctly for STM32 embedded
 * targets. It's not linked into any executable - it exists purely to test
 * conditional compilation when building for ARM Cortex-M targets.
 *
 * ## Purpose
 *
 * When building for STM32F429 (Cortex-M4), this file verifies:
 * - Cache operations compile to empty inline functions (no-op)
 * - No linker errors from missing CMSIS cache functions
 * - Static assertions pass for compile-time guarantees
 *
 * When building for STM32F7/H7 (Cortex-M7), this file verifies:
 * - Cache operations call actual CMSIS functions
 * - Proper CMSIS headers are included
 * - Functions have correct signatures
 *
 * ## How to Use
 *
 * This file is not included in CMakeLists.txt by default. To test compilation
 * for embedded targets:
 *
 * 1. Configure for embedded target:
 *    ```bash
 *    cmake --preset stm32f429-debug
 *    ```
 *
 * 2. Compile this file manually:
 *    ```bash
 *    arm-none-eabi-g++ -c -mcpu=cortex-m4 -mthumb \
 *        -I include -I vendor/CMSIS/Include -I vendor/CMSIS/Device/ST/STM32F4xx/Include \
 *        -DSTM32F429xx -std=c++20 -o build/cache_ops_test.o \
 *        src/platform/cache_ops_compile_test.cpp
 *    ```
 *
 * 3. Inspect object file to verify cache operations compiled correctly:
 *    ```bash
 *    arm-none-eabi-objdump -d build/cache_ops_test.o | grep test_cache_operations -A 20
 *    ```
 *
 * For Cortex-M4, you should see empty function bodies (just return).
 * For Cortex-M7, you should see calls to SCB cache functions.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <cstdint>
#include <hm11/platform/cache_ops.hpp>

// Test buffer aligned to cache line boundary
alignas(32) uint8_t test_buffer[256];

/**
 * @brief Test function that uses all cache operations
 *
 * This function is never called - it exists purely to verify compilation.
 * The compiler will optimize this away in release builds, but we can
 * inspect the object file to verify conditional compilation works.
 */
void test_cache_operations() {
    // Test invalidate (for DMA RX)
    hm11::platform::invalidate_dcache_range(test_buffer, sizeof(test_buffer));

    // Test clean (for DMA TX)
    hm11::platform::clean_dcache_range(test_buffer, sizeof(test_buffer));

    // Test clean+invalidate
    hm11::platform::clean_invalidate_dcache_range(test_buffer, sizeof(test_buffer));

    // Verify constant is accessible
    constexpr std::size_t cache_line_size = hm11::platform::DCACHE_LINE_SIZE;
    static_cast<void>(cache_line_size); // Suppress unused warning
}

// Compile-time verification that functions are noexcept (ISR-safe)
static_assert(
    noexcept(hm11::platform::invalidate_dcache_range(nullptr, 0)),
    "invalidate_dcache_range must be noexcept for ISR safety");
static_assert(
    noexcept(hm11::platform::clean_dcache_range(nullptr, 0)),
    "clean_dcache_range must be noexcept for ISR safety");
static_assert(
    noexcept(hm11::platform::clean_invalidate_dcache_range(nullptr, 0)),
    "clean_invalidate_dcache_range must be noexcept for ISR safety");

// Compile-time verification that constant is constexpr
static_assert(hm11::platform::DCACHE_LINE_SIZE == 32, "DCACHE_LINE_SIZE must be 32 bytes (Cortex-M7 cache line size)");

// Compile-time verification that aligned buffer has correct alignment
static_assert(alignof(test_buffer) == 32, "Test buffer must be aligned to 32-byte cache line boundary");
