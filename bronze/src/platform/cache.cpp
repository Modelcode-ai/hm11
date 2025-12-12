/**
 * @file cache.cpp
 * @brief Implementation of platform-specific cache management functions
 *
 * This file is part of the HM11 driver C++ migration project.
 * Implements Decision 20 regarding memory layout and DMA alignment.
 */

#include "hm11/platform/cache.hpp"

#include <cassert>

namespace hm11::platform::cache {

// STM32F7/H7 (Cortex-M7) has data cache that needs explicit management
#if defined(STM32F7) || defined(STM32H7)

// When building for STM32F7 or STM32H7, include the STM32 CMSIS header for cache functions
// These platforms include the Cortex-M7 core with data cache
#if defined(STM32F7)
#include "stm32f7xx.h" // STM32F7 CMSIS header
#elif defined(STM32H7)
#include "stm32h7xx.h" // STM32H7 CMSIS header
#endif

void invalidate_dma_buffer(void* buffer, std::size_t size) {
    // Check alignment requirements
    assert(is_aligned(buffer, 32) && "Buffer must be 32-byte aligned for cache operations");

    // Size must be multiple of 32 bytes for SCB functions
    std::size_t aligned_size = (size + 31) & ~31;

    // Call CMSIS provided cache invalidation function
    SCB_InvalidateDCache_by_Addr(static_cast<uint32_t*>(buffer), aligned_size);
}

void clean_dma_buffer(void* buffer, std::size_t size) {
    // Check alignment requirements
    assert(is_aligned(buffer, 32) && "Buffer must be 32-byte aligned for cache operations");

    // Size must be multiple of 32 bytes for SCB functions
    std::size_t aligned_size = (size + 31) & ~31;

    // Call CMSIS provided cache clean function
    SCB_CleanDCache_by_Addr(static_cast<uint32_t*>(buffer), aligned_size);
}

void clean_and_invalidate_dma_buffer(void* buffer, std::size_t size) {
    // Check alignment requirements
    assert(is_aligned(buffer, 32) && "Buffer must be 32-byte aligned for cache operations");

    // Size must be multiple of 32 bytes for SCB functions
    std::size_t aligned_size = (size + 31) & ~31;

    // Call CMSIS provided cache clean and invalidate function
    SCB_CleanInvalidateDCache_by_Addr(static_cast<uint32_t*>(buffer), aligned_size);
}

#endif // STM32F7 || STM32H7

} // namespace hm11::platform::cache