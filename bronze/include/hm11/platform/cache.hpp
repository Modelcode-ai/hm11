/**
 * @file cache.hpp
 * @brief Platform-specific cache management functions
 * @details Provides cache invalidation and cleaning operations for different platforms
 *
 * This file is part of the HM11 driver C++ migration project.
 * Implements Decision 20 regarding memory layout and DMA alignment.
 */

#ifndef HM11_PLATFORM_CACHE_HPP
#define HM11_PLATFORM_CACHE_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring> // for std::memcpy

namespace hm11::platform::cache {

/**
 * @brief Check if address is properly aligned
 *
 * @param address Memory address to check
 * @param alignment Required alignment (must be power of 2)
 * @return true if properly aligned, false otherwise
 */
inline bool is_aligned(const void* address, std::size_t alignment) {
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");

    // This implementation simply checks if the address is a multiple of the alignment
    // For better compatibility, we convert to size_t which is safe for pointer arithmetic

    // Warning: casting pointers to integers is technically implementation-defined
    // but is standard practice for alignment checks on all major compilers
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,modernize-use-auto)
    std::size_t addr = reinterpret_cast<std::size_t>(address);

    // Check alignment by testing if address is a multiple of alignment
    return (addr & (alignment - 1)) == 0;
}

// STM32F7/H7 (Cortex-M7) has data cache that needs explicit management
#if defined(STM32F7) || defined(STM32H7)

/**
 * @brief Invalidate data cache for the given memory region
 *
 * This function invalidates the data cache for the specified memory region,
 * ensuring that subsequent reads will fetch data from external memory rather
 * than using potentially stale cached values. This should be called after
 * DMA writes to memory (e.g., after a DMA receive operation completes).
 *
 * @param buffer Pointer to the memory region (must be 32-byte aligned)
 * @param size Size of the memory region in bytes
 */
void invalidate_dma_buffer(void* buffer, std::size_t size);

/**
 * @brief Clean data cache for the given memory region
 *
 * This function cleans (flushes) the data cache for the specified memory region,
 * ensuring that all cached writes are propagated to external memory. This should
 * be called before DMA reads from memory (e.g., before a DMA transmit operation starts).
 *
 * @param buffer Pointer to the memory region (must be 32-byte aligned)
 * @param size Size of the memory region in bytes
 */
void clean_dma_buffer(void* buffer, std::size_t size);

/**
 * @brief Clean and invalidate data cache for the given memory region
 *
 * This function cleans and invalidates the data cache for the specified memory region,
 * ensuring both that all cached writes are propagated to external memory and that
 * subsequent reads will fetch fresh data.
 *
 * @param buffer Pointer to the memory region (must be 32-byte aligned)
 * @param size Size of the memory region in bytes
 */
void clean_and_invalidate_dma_buffer(void* buffer, std::size_t size);

#else // No data cache or STM32F4 (Cortex-M4)

/**
 * @brief No-op implementation for platforms without data cache
 */
inline void invalidate_dma_buffer(const void* buffer, std::size_t size) {
    // No-op implementation for platforms without data cache
    (void)buffer; // Prevent unused parameter warning
    (void)size;   // Prevent unused parameter warning
}

/**
 * @brief No-op implementation for platforms without data cache
 */
inline void clean_dma_buffer(const void* buffer, std::size_t size) {
    // No-op implementation for platforms without data cache
    (void)buffer; // Prevent unused parameter warning
    (void)size;   // Prevent unused parameter warning
}

/**
 * @brief No-op implementation for platforms without data cache
 */
inline void clean_and_invalidate_dma_buffer(const void* buffer, std::size_t size) {
    // No-op implementation for platforms without data cache
    (void)buffer; // Prevent unused parameter warning
    (void)size;   // Prevent unused parameter warning
}

#endif

} // namespace hm11::platform::cache

#endif // HM11_PLATFORM_CACHE_HPP