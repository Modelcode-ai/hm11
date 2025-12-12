#pragma once

// HM11 Platform Abstraction Layer - DMA Cache Operations
// Platform-specific cache management for DMA buffers and memory coherency
// Supports different ARM Cortex-M variants with conditional compilation

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace hm11::platform {

/// Cache line size for ARM Cortex-M7 (32 bytes)
/// Used for alignment requirements and cache operations
constexpr std::size_t CACHE_LINE_SIZE = 32U;

/// Platform-specific DMA cache management
///
/// Implementation Strategy:
/// - STM32F7/H7 (Cortex-M7): SCB cache management functions
/// - STM32F4 (Cortex-M4): No cache operations (cache not present)
/// - Other platforms: Compiler barriers for safety
///
/// Memory Ordering:
/// - Clean operations ensure CPU writes are visible to DMA
/// - Invalidate operations ensure DMA writes are visible to CPU
/// - Use before/after DMA transfers for cache coherency
namespace dma_cache {

/// Clean data cache for DMA transmit operations
/// Ensures CPU writes to buffer are visible to DMA controller
/// Call before starting DMA transmit transfer
///
/// @param buffer Pointer to start of buffer
/// @param size Size of buffer in bytes
void CleanForTransmit(const void* buffer, std::size_t size) noexcept;

/// Invalidate data cache for DMA receive operations
/// Ensures DMA writes to buffer are visible to CPU
/// Call after completing DMA receive transfer
///
/// @param buffer Pointer to start of buffer
/// @param size Size of buffer in bytes
void InvalidateAfterReceive(void* buffer, std::size_t size) noexcept;

/// Clean and invalidate data cache for bidirectional DMA operations
/// Combines clean and invalidate for buffers used in both directions
/// Call before starting bidirectional DMA transfer
///
/// @param buffer Pointer to start of buffer
/// @param size Size of buffer in bytes
void CleanAndInvalidate(void* buffer, std::size_t size) noexcept;

/// Template convenience functions for typed buffers
template <typename T> void CleanForTransmit(const T* buffer, std::size_t count = 1) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "DMA buffer type must be trivially copyable");
    CleanForTransmit(static_cast<const void*>(buffer), count * sizeof(T));
}

template <typename T> void InvalidateAfterReceive(T* buffer, std::size_t count = 1) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "DMA buffer type must be trivially copyable");
    InvalidateAfterReceive(static_cast<void*>(buffer), count * sizeof(T));
}

template <typename T> void CleanAndInvalidate(T* buffer, std::size_t count = 1) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "DMA buffer type must be trivially copyable");
    CleanAndInvalidate(static_cast<void*>(buffer), count * sizeof(T));
}

} // namespace dma_cache

/// Memory alignment utilities for DMA buffer requirements
namespace dma_alignment {

/// Check if address is properly aligned for cache coherency
/// DMA buffers should be aligned to cache line boundaries (32 bytes)
///
/// @param address Memory address to check
/// @return true if properly aligned, false otherwise
inline bool IsAligned(const void* address) noexcept {
    // Use reinterpret_cast for pointer to integer conversion as required
    return (reinterpret_cast<std::uintptr_t>(address) % CACHE_LINE_SIZE) ==
           0; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

/// Check if size is multiple of cache line size
/// DMA buffer sizes should be multiples of cache line size for efficiency
///
/// @param size Buffer size in bytes
/// @return true if properly sized, false otherwise
constexpr bool IsAlignedSize(std::size_t size) noexcept {
    return size > 0 && (size % CACHE_LINE_SIZE) == 0;
}

/// Calculate next aligned address
/// Rounds up address to next cache line boundary
///
/// @param address Input address
/// @return Next aligned address (>= input address)
constexpr std::uintptr_t AlignUp(std::uintptr_t address) noexcept {
    return (address + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
}

/// Calculate aligned size
/// Rounds up size to next cache line multiple
///
/// @param size Input size in bytes
/// @return Next aligned size (>= input size)
constexpr std::size_t AlignSize(std::size_t size) noexcept {
    return (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
}

/// Template helper for checking buffer alignment
template <typename T> constexpr bool IsBufferAligned(const T* buffer, std::size_t count = 1) noexcept {
    return IsAligned(buffer) && IsAlignedSize(count * sizeof(T));
}

} // namespace dma_alignment

} // namespace hm11::platform