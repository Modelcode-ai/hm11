/**
 * @file alignment.hpp
 * @brief Platform-specific memory alignment utilities
 * @details Provides alignment utilities for DMA operations
 *
 * This file is part of the HM11 driver C++ migration project.
 * Implements Decision 20 regarding memory layout and DMA alignment.
 */

#ifndef HM11_PLATFORM_ALIGNMENT_HPP
#define HM11_PLATFORM_ALIGNMENT_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "hm11/platform/cache.hpp"

namespace hm11::platform {

/**
 * @brief Standard DMA alignment requirement for STM32
 * 32-byte alignment ensures cache coherency and DMA requirements
 */
constexpr std::size_t DMA_ALIGNMENT = 32;

/**
 * @brief Check if a pointer is properly aligned for DMA operations
 *
 * @param ptr Pointer to check
 * @return true if pointer is aligned according to DMA_ALIGNMENT
 * @return false otherwise
 */
inline bool is_dma_aligned(const void* ptr) {
    return cache::is_aligned(ptr, DMA_ALIGNMENT);
}

/**
 * @brief Asserts that a pointer is properly aligned for DMA operations
 * This function is typically used in debug builds to verify alignment.
 *
 * @param ptr Pointer to check
 * @param message Error message for assertion failure
 */
inline void assert_dma_aligned(
    [[maybe_unused]] const void* ptr,
    [[maybe_unused]] const char* message = "DMA buffer not properly aligned") {
    assert(is_dma_aligned(ptr) && message);
}

/**
 * @brief DMA aligned buffer template
 *
 * Template class for creating buffers with proper alignment for DMA operations.
 * Uses C++11 alignas keyword for explicit alignment.
 *
 * @tparam T Element type
 * @tparam N Number of elements
 */
template <typename T, std::size_t N> struct alignas(DMA_ALIGNMENT) DmaAlignedBuffer {
    static_assert(std::is_trivial_v<T>, "DMA buffers must contain trivial types");

    std::array<T, N> buffer;

    // Return reference to underlying array for compatibility with test code
    [[nodiscard]] const std::array<T, N>& to_array() const noexcept { return buffer; }
    [[nodiscard]] std::array<T, N>& to_array() noexcept { return buffer; }

    // Array-like access
    T& operator[](std::size_t index) { return buffer[index]; }
    const T& operator[](std::size_t index) const { return buffer[index]; }

    // Pointer access for DMA operations
    T* data() { return buffer.data(); }
    const T* data() const { return buffer.data(); }

    // Size information
    constexpr std::size_t size() const { return N; }

    // Byte size information
    constexpr std::size_t byte_size() const { return N * sizeof(T); }

    // Initialize buffer with a value
    void fill(const T& value) { buffer.fill(value); }

    // Clear buffer (set to zero)
    void clear() { buffer.fill(T{}); }

    // Cache operations
    void invalidate_cache() const { cache::invalidate_dma_buffer(data(), byte_size()); }

    void clean_cache() const { cache::clean_dma_buffer(data(), byte_size()); }

    void clean_and_invalidate_cache() const { cache::clean_and_invalidate_dma_buffer(data(), byte_size()); }
};

/**
 * @brief Dynamically sized DMA aligned buffer
 *
 * Similar to DmaAlignedBuffer, but with dynamic size determined at runtime.
 * This type has more overhead and uses heap allocation, so prefer the fixed-size
 * version when the buffer size is known at compile time.
 */
template <typename T> class alignas(DMA_ALIGNMENT) DynamicDmaBuffer {
    static_assert(std::is_trivial_v<T>, "DMA buffers must contain trivial types");

  private:
    std::vector<T> buffer;

  public:
    /**
     * @brief Construct a new Dynamic DMA Buffer
     *
     * @param size Number of elements to allocate
     */
    explicit DynamicDmaBuffer(std::size_t size) : buffer(size) {
        // Ensure the allocated buffer is properly aligned
        assert_dma_aligned(data(), "Dynamic DMA buffer not properly aligned");
    }

    // Array-like access
    T& operator[](std::size_t index) { return buffer[index]; }
    const T& operator[](std::size_t index) const { return buffer[index]; }

    // Pointer access for DMA operations
    T* data() { return buffer.data(); }
    const T* data() const { return buffer.data(); }

    // Size information
    std::size_t size() const { return buffer.size(); }
    std::size_t byte_size() const { return buffer.size() * sizeof(T); }

    // Initialize buffer with a value
    void fill(const T& value) { std::fill(buffer.begin(), buffer.end(), value); }

    // Clear buffer (set to zero)
    void clear() { fill(T{}); }

    // Resize buffer (note: may invalidate alignment if implementation changes)
    void resize(std::size_t new_size) {
        buffer.resize(new_size);
        assert_dma_aligned(data(), "Dynamic DMA buffer lost alignment after resize");
    }

    // Return reference to underlying vector for compatibility with test code
    [[nodiscard]] const std::vector<T>& to_array() const noexcept { return buffer; }
    [[nodiscard]] std::vector<T>& to_array() noexcept { return buffer; }

    // Cache operations
    void invalidate_cache() const { cache::invalidate_dma_buffer(data(), byte_size()); }

    void clean_cache() const { cache::clean_dma_buffer(data(), byte_size()); }

    void clean_and_invalidate_cache() const { cache::clean_and_invalidate_dma_buffer(data(), byte_size()); }
};

} // namespace hm11::platform

#endif // HM11_PLATFORM_ALIGNMENT_HPP