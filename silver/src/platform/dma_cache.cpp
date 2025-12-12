// HM11 Platform Abstraction Layer - DMA Cache Operations Implementation
// Platform-specific cache management implementation with conditional compilation

#include "hm11/platform/dma_cache.hpp"

// Platform-specific headers for cache operations
#if defined(STM32F7xx) || defined(STM32H7xx)
// STM32F7/H7 series with Cortex-M7 and data cache
#include "stm32f7xx.h" // or stm32h7xx.h for H7 series
#elif defined(STM32F4xx)
// STM32F4 series with Cortex-M4 (no data cache)
// No cache operations needed
#elif defined(__arm__) || defined(__aarch64__)
// Generic ARM - use CMSIS if available
#ifdef __CMSIS_HEADER
#include __CMSIS_HEADER
#endif
#endif

// Compiler barrier for non-cached platforms
#ifndef SCB_CleanDCache_by_Addr
namespace {
inline void CompilerBarrier() noexcept {
    __asm__ volatile("" ::: "memory");
}
} // anonymous namespace
#else
#define CompilerBarrier() // No-op when hardware barriers available
#endif

namespace hm11::platform::dma_cache {

void CleanForTransmit(const void* buffer, std::size_t size) noexcept {
    if (buffer == nullptr || size == 0) {
        return;
    }

#if defined(STM32F7xx) || defined(STM32H7xx)
    // STM32F7/H7: Use hardware data cache clean operation
    // Ensures CPU writes are visible to DMA controller
    SCB_CleanDCache_by_Addr(const_cast<uint32_t*>(static_cast<const uint32_t*>(buffer)), static_cast<int32_t>(size));
#elif defined(STM32F4xx)
    // STM32F4: No cache present, no operation needed
    // Buffer modifications are immediately visible to DMA
    (void)buffer; // Suppress unused parameter warning
    (void)size;
#else
    // Other platforms: Use compiler barrier for safety
    // Prevents compiler from reordering memory operations
    (void)buffer;
    (void)size;
    CompilerBarrier();
#endif
}

void InvalidateAfterReceive(void* buffer, std::size_t size) noexcept {
    if (buffer == nullptr || size == 0) {
        return;
    }

#if defined(STM32F7xx) || defined(STM32H7xx)
    // STM32F7/H7: Use hardware data cache invalidate operation
    // Ensures DMA writes are visible to CPU
    SCB_InvalidateDCache_by_Addr(static_cast<uint32_t*>(buffer), static_cast<int32_t>(size));
#elif defined(STM32F4xx)
    // STM32F4: No cache present, no operation needed
    // DMA writes are immediately visible to CPU
    (void)buffer; // Suppress unused parameter warning
    (void)size;
#else
    // Other platforms: Use compiler barrier for safety
    // Prevents compiler from reordering memory operations
    (void)buffer;
    (void)size;
    CompilerBarrier();
#endif
}

void CleanAndInvalidate(void* buffer, std::size_t size) noexcept {
    if (buffer == nullptr || size == 0) {
        return;
    }

#if defined(STM32F7xx) || defined(STM32H7xx)
    // STM32F7/H7: Combined clean and invalidate operation
    // More efficient than separate operations for bidirectional buffers
    SCB_CleanInvalidateDCache_by_Addr(static_cast<uint32_t*>(buffer), static_cast<int32_t>(size));
#elif defined(STM32F4xx)
    // STM32F4: No cache present, no operation needed
    // Buffer is always coherent between CPU and DMA
    (void)buffer; // Suppress unused parameter warning
    (void)size;
#else
    // Other platforms: Use compiler barrier for safety
    // Prevents compiler from reordering memory operations
    (void)buffer;
    (void)size;
    CompilerBarrier();
#endif
}

} // namespace hm11::platform::dma_cache