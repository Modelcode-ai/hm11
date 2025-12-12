/**
 * @file cache_ops.hpp
 * @brief Platform-specific cache coherency operations for DMA buffers
 *
 * This header provides explicit cache management functions for DMA coherency on
 * STM32 microcontrollers. The implementation uses conditional compilation to
 * provide zero-overhead operations on Cortex-M4 (which lacks data cache) while
 * calling proper CMSIS cache maintenance functions on Cortex-M7 devices.
 *
 * ## Overview
 *
 * On ARM Cortex-M7 devices (STM32F7/H7 series), the data cache can cause DMA
 * coherency issues:
 * - After DMA reception, the cache may contain stale data requiring invalidation
 * - Before DMA transmission, dirty cache lines must be written back to memory
 * - Without proper cache operations, the application may read stale data or DMA
 *   may transmit incorrect data
 *
 * The STM32F429 (Cortex-M4) does not have a data cache, so cache operations
 * compile to empty inline functions (zero overhead) for this target.
 *
 * ## Architecture (Decision 20: Memory Layout and DMA Alignment)
 *
 * This file implements the cache operations portion of Decision 20. The memory
 * alignment portion (alignas keyword) is applied where DMA buffers are
 * implemented.
 *
 * Cache operations are conditionally compiled:
 * - **Cortex-M7** (__CORTEX_M == 7): Calls CMSIS SCB functions
 * - **Cortex-M4** (__CORTEX_M == 4): Compiles to empty inline functions
 *
 * ## CMSIS Cache Functions
 *
 * On Cortex-M7, this header uses CMSIS-CORE cache maintenance APIs:
 * - `SCB_CleanDCache_by_Addr(addr, size)` - Flush cache lines to memory
 * - `SCB_InvalidateDCache_by_Addr(addr, size)` - Discard cached data
 *
 * These functions are defined in `core_cm7.h` and operate on 32-byte cache lines.
 *
 * ## Memory Alignment Requirements
 *
 * For optimal performance and correctness on Cortex-M7:
 * - DMA buffers should be aligned to 32-byte boundaries (cache line size)
 * - Use `alignas(32)` when declaring DMA buffers
 * - Address and size parameters will be automatically rounded to cache line
 *   boundaries by the CMSIS functions
 *
 * ## Memory Ordering (Decision 23)
 *
 * Cache operations provide memory ordering guarantees:
 * - `invalidate_dcache_range()` includes acquire semantics (reads after invalidate
 *   see DMA-written data)
 * - `clean_dcache_range()` includes release semantics (DMA reads flushed data)
 *
 * For ISR-safe DMA operations, combine with std::atomic_thread_fence:
 * @code
 * // After DMA RX completion in ISR:
 * std::atomic_thread_fence(std::memory_order_acquire);
 * hm11::platform::invalidate_dcache_range(rx_buffer, size);
 * @endcode
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/platform/cache_ops.hpp>
 *
 * // DMA receive buffer (aligned for cache coherency)
 * alignas(32) uint8_t rx_buffer[256];
 *
 * // Before starting DMA RX operation:
 * // No cache operation needed - just start DMA
 * start_dma_rx(rx_buffer, sizeof(rx_buffer));
 *
 * // After DMA RX completion (in ISR or callback):
 * // Invalidate cache to discard stale data and see DMA-written data
 * hm11::platform::invalidate_dcache_range(rx_buffer, bytes_received);
 * // Now safe to read rx_buffer contents
 *
 * // Before starting DMA TX operation:
 * // Clean cache to ensure DMA reads latest data from memory
 * hm11::platform::clean_dcache_range(tx_buffer, bytes_to_send);
 * start_dma_tx(tx_buffer, bytes_to_send);
 * @endcode
 *
 * ## Performance Characteristics
 *
 * ### Cortex-M4 (STM32F429)
 * - Zero overhead: Functions compile to empty inline stubs
 * - No runtime cost
 *
 * ### Cortex-M7 (STM32F7/H7)
 * - Cache operations execute via CP15 coprocessor instructions
 * - Typical latency: ~10-50 CPU cycles per cache line (32 bytes)
 * - For 256-byte buffer: ~80-400 cycles (~1-5µs at 168 MHz)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <cstddef>
#include <cstdint>

// Include CMSIS headers for Cortex-M7 cache operations
#if defined(__CORTEX_M) && (__CORTEX_M == 7)
// The core_cm7.h header is included via the STM32 device header
// (e.g., stm32f7xx.h) which is included via the STM32_DEVICE define
// For standalone compilation, we need the CMSIS core header
#if defined(STM32F7)
#include "stm32f7xx.h"
#elif defined(STM32H7)
#include "stm32h7xx.h"
#else
// Generic CMSIS include for other Cortex-M7 devices
#include "core_cm7.h"
#endif
#endif

namespace hm11::platform {

/**
 * @brief Cortex-M7 data cache line size in bytes
 *
 * The Cortex-M7 has a fixed 32-byte cache line size. Buffers should be aligned
 * to this boundary for optimal cache operations.
 *
 * @note This constant is defined for all platforms for convenience, but cache
 *       operations are only performed on Cortex-M7.
 */
inline constexpr std::size_t DCACHE_LINE_SIZE = 32;

/**
 * @brief Invalidate data cache for a memory range after DMA reception
 *
 * Invalidates (discards) the data cache lines covering the specified memory
 * range. This operation is required after DMA reception to ensure the CPU reads
 * the DMA-written data from memory instead of stale cached data.
 *
 * ## When to Use
 *
 * Call this function **after** DMA reception completes and **before** the
 * application reads the received data:
 * 1. DMA writes data directly to memory (bypassing cache)
 * 2. Cache may still contain old data for this memory region
 * 3. Invalidate cache to force CPU to read fresh data from memory
 *
 * ## Platform Behavior
 *
 * ### Cortex-M7 (STM32F7/H7):
 * - Calls `SCB_InvalidateDCache_by_Addr()` to invalidate cache lines
 * - Address and size are automatically rounded to cache line boundaries
 * - Discards any dirty cache lines (data not written back)
 *
 * ### Cortex-M4 (STM32F429):
 * - Compiles to empty inline function (zero overhead)
 * - No operation performed (M4 has no data cache)
 *
 * ## Memory Alignment
 *
 * For best performance, align buffers to DCACHE_LINE_SIZE (32 bytes) using
 * `alignas(32)`. The CMSIS function automatically handles unaligned addresses
 * by rounding to cache line boundaries, but proper alignment avoids invalidating
 * adjacent data.
 *
 * ## Memory Ordering
 *
 * This function includes acquire semantics - memory reads after this function
 * returns will observe the DMA-written data. No additional memory barriers
 * are needed for typical DMA RX scenarios.
 *
 * @param addr Pointer to the start of the memory range (any alignment accepted)
 * @param size Size of the memory range in bytes
 *
 * @note This function is noexcept and inline for zero overhead
 * @note Safe to call even if cache is disabled (CMSIS handles this case)
 * @note On M4, this is a no-op and will be optimized away by the compiler
 *
 * @warning Do NOT call this function for memory ranges that contain data you
 *          want to preserve. Invalidation discards any cached data without
 *          writing it back to memory.
 *
 * @see clean_dcache_range() for DMA transmission preparation
 */
inline void invalidate_dcache_range(void* addr, std::size_t size) noexcept {
#if defined(__CORTEX_M) && (__CORTEX_M == 7)
    // Cortex-M7: Perform actual cache invalidation via CMSIS
    if (addr != nullptr && size > 0) {
        SCB_InvalidateDCache_by_Addr(static_cast<uint32_t*>(addr), static_cast<int32_t>(size));
    }
#else
    // Cortex-M4 or other platforms: No data cache, no operation needed
    // These parameters are intentionally unused on M4 (suppress warnings)
    static_cast<void>(addr);
    static_cast<void>(size);
#endif
}

/**
 * @brief Clean data cache for a memory range before DMA transmission
 *
 * Cleans (writes back) the data cache lines covering the specified memory range.
 * This operation is required before DMA transmission to ensure the DMA reads the
 * latest data from memory instead of stale memory that hasn't been updated with
 * cached modifications.
 *
 * ## When to Use
 *
 * Call this function **before** starting DMA transmission and **after** the
 * application has written data to the buffer:
 * 1. Application writes data to buffer (may be cached)
 * 2. Clean cache to flush writes to memory
 * 3. Start DMA transmission (DMA reads fresh data from memory)
 *
 * ## Platform Behavior
 *
 * ### Cortex-M7 (STM32F7/H7):
 * - Calls `SCB_CleanDCache_by_Addr()` to write back dirty cache lines
 * - Address and size are automatically rounded to cache line boundaries
 * - Cache lines remain valid after cleaning (not invalidated)
 *
 * ### Cortex-M4 (STM32F429):
 * - Compiles to empty inline function (zero overhead)
 * - No operation performed (M4 has no data cache)
 *
 * ## Memory Alignment
 *
 * For best performance, align buffers to DCACHE_LINE_SIZE (32 bytes) using
 * `alignas(32)`. The CMSIS function automatically handles unaligned addresses
 * by rounding to cache line boundaries.
 *
 * ## Memory Ordering
 *
 * This function includes release semantics - the DMA will observe all memory
 * writes that happened before this function call. No additional memory barriers
 * are needed for typical DMA TX scenarios.
 *
 * @param addr Pointer to the start of the memory range (any alignment accepted)
 * @param size Size of the memory range in bytes
 *
 * @note This function is noexcept and inline for zero overhead
 * @note Safe to call even if cache is disabled (CMSIS handles this case)
 * @note On M4, this is a no-op and will be optimized away by the compiler
 * @note This operation does NOT invalidate cache (data remains cached after clean)
 *
 * @see invalidate_dcache_range() for DMA reception handling
 */
inline void clean_dcache_range(void* addr, std::size_t size) noexcept {
#if defined(__CORTEX_M) && (__CORTEX_M == 7)
    // Cortex-M7: Perform actual cache cleaning via CMSIS
    if (addr != nullptr && size > 0) {
        SCB_CleanDCache_by_Addr(static_cast<uint32_t*>(addr), static_cast<int32_t>(size));
    }
#else
    // Cortex-M4 or other platforms: No data cache, no operation needed
    // These parameters are intentionally unused on M4 (suppress warnings)
    static_cast<void>(addr);
    static_cast<void>(size);
#endif
}

/**
 * @brief Clean and invalidate data cache for a memory range
 *
 * Performs both clean (write back) and invalidate (discard) operations on the
 * data cache lines covering the specified memory range. This is equivalent to
 * calling clean_dcache_range() followed by invalidate_dcache_range(), but may
 * be more efficient on some platforms.
 *
 * ## When to Use
 *
 * This operation is rarely needed for typical DMA scenarios. Use individual
 * clean or invalidate operations instead. Potential use cases:
 * - Bidirectional DMA (device both reads and writes same buffer)
 * - Cache maintenance during buffer reuse between operations
 * - Cleanup operations during error handling
 *
 * ## Platform Behavior
 *
 * ### Cortex-M7 (STM32F7/H7):
 * - Calls `SCB_CleanInvalidateDCache_by_Addr()` for efficient operation
 * - Writes back dirty data, then discards cache lines
 *
 * ### Cortex-M4 (STM32F429):
 * - Compiles to empty inline function (zero overhead)
 * - No operation performed (M4 has no data cache)
 *
 * @param addr Pointer to the start of the memory range (any alignment accepted)
 * @param size Size of the memory range in bytes
 *
 * @note This function is noexcept and inline for zero overhead
 * @note On M4, this is a no-op and will be optimized away by the compiler
 *
 * @see clean_dcache_range() for DMA transmission preparation
 * @see invalidate_dcache_range() for DMA reception handling
 */
inline void clean_invalidate_dcache_range(void* addr, std::size_t size) noexcept {
#if defined(__CORTEX_M) && (__CORTEX_M == 7)
    // Cortex-M7: Perform actual cache clean+invalidate via CMSIS
    if (addr != nullptr && size > 0) {
        SCB_CleanInvalidateDCache_by_Addr(static_cast<uint32_t*>(addr), static_cast<int32_t>(size));
    }
#else
    // Cortex-M4 or other platforms: No data cache, no operation needed
    // These parameters are intentionally unused on M4 (suppress warnings)
    static_cast<void>(addr);
    static_cast<void>(size);
#endif
}

} // namespace hm11::platform
