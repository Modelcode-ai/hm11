#pragma once

// HM11 Synchronization Primitives - Lock-Free Circular Buffer
// Single-producer-single-consumer (SPSC) circular buffer for ISR-to-thread streaming data

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace hm11::sync {

/// Lock-free SPSC circular buffer for streaming data from ISR to thread
///
/// - Producer (ISR) ONLY modifies write_position
/// - Consumer (thread) ONLY modifies read_position
/// - No concurrent writes to the same variable = no race conditions
/// - Simple overwrite semantics when buffer is full
///
/// Memory Ordering:
/// - ISR write operations: memory_order_release for data visibility
/// - Thread read operations: memory_order_acquire for data consistency
/// - Position updates: release/acquire for synchronization
template <typename T, std::size_t Capacity> class CircularBuffer {
    static_assert(Capacity > 0, "Circular buffer capacity must be greater than 0");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2 for efficient modulo");

  public:
    /// Construct empty circular buffer
    CircularBuffer() noexcept : write_pos_(0), read_pos_(0) {}

    /// Default destructor
    ~CircularBuffer() noexcept = default;

    /// Non-copyable and non-movable (following RAII ownership model)
    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    CircularBuffer(CircularBuffer&&) = delete;
    CircularBuffer& operator=(CircularBuffer&&) = delete;

    /// Write single element from ISR context
    /// ALWAYS succeeds - uses OVERWRITE semantics (never loses new data)
    /// Uses memory_order_release to ensure data visibility before position update
    void WriteFromISR(const T& data) noexcept {
        // Read current write position
        const std::size_t current_write = write_pos_.load(std::memory_order_relaxed);

        // Write data to current position
        buffer_[current_write] = data;

        // Calculate next position with wraparound
        const std::size_t next_write = (current_write + 1) & (Capacity - 1);

        // Update write position with release semantics
        // Ensures all buffer writes are visible before position update
        write_pos_.store(next_write, std::memory_order_release);
    }

    /// Write multiple elements from ISR context
    /// ALWAYS writes all data - uses OVERWRITE semantics
    void WriteFromISR(std::span<const T> data) noexcept {
        for (const auto& item : data) {
            WriteFromISR(item);
        }
    }

    /// Read available data from thread context
    /// Returns span of available data (may be empty if no data available)
    /// Data remains valid until next ReadAvailable() or MarkConsumed() call
    [[nodiscard]] std::span<const T> ReadAvailable() noexcept {
        // Get current write position with acquire semantics
        const std::size_t current_write = write_pos_.load(std::memory_order_acquire);

        if (current_write == read_pos_) {
            // Buffer is empty
            return {};
        }

        // Calculate available data length
        if (current_write > read_pos_) {
            // No wraparound - contiguous data
            const std::size_t available = current_write - read_pos_;
            return std::span<const T>(&buffer_[read_pos_], available);
        }

        // Wraparound case - return data from read_pos to end of buffer
        // Consumer will need to call ReadAvailable() again after consuming
        // this chunk to get data from beginning of buffer
        const std::size_t available = Capacity - read_pos_;
        return std::span<const T>(&buffer_[read_pos_], available);
    }

    /// Mark data as consumed and advance read position
    /// Should be called after processing data from ReadAvailable()
    void MarkConsumed(std::size_t count) noexcept {
        // Only consumer modifies read_pos_, so no synchronization needed
        read_pos_ = (read_pos_ + count) & (Capacity - 1);
    }

    /// Get current number of elements available for reading
    [[nodiscard]] std::size_t GetAvailableCount() const noexcept {
        const std::size_t current_write = write_pos_.load(std::memory_order_acquire);

        if (current_write >= read_pos_) {
            return current_write - read_pos_;
        }
        // Wraparound case
        return (Capacity - read_pos_) + current_write;
    }

    /// Check if buffer is empty (no data available for reading)
    [[nodiscard]] bool IsEmpty() const noexcept {
        const std::size_t current_write = write_pos_.load(std::memory_order_acquire);
        return current_write == read_pos_;
    }

    /// Check if buffer is full (no space for writing without overwriting)
    [[nodiscard]] bool IsFull() const noexcept {
        const std::size_t current_write = write_pos_.load(std::memory_order_relaxed);
        const std::size_t next_write = (current_write + 1) & (Capacity - 1);
        return next_write == read_pos_;
    }

    /// Get buffer capacity
    [[nodiscard]] static constexpr std::size_t GetCapacity() noexcept { return Capacity; }

    /// Reset buffer to empty state
    /// Should only be called when no ISR activity (during shutdown/restart)
    void Reset() noexcept {
        read_pos_ = 0;
        write_pos_.store(0, std::memory_order_relaxed);
    }

  private:
    /// Fixed-size buffer storage
    /// Aligned for potential DMA usage
    alignas(32) std::array<T, Capacity> buffer_;

    /// Write position managed by ISR (producer)
    /// ISR updates with memory_order_release after writing data
    /// Thread reads with memory_order_acquire to observe ISR writes
    std::atomic<std::size_t> write_pos_;

    /// Read position managed by thread (consumer)
    /// Only consumer modifies this - no atomic needed
    /// Producer NEVER touches this variable
    std::size_t read_pos_;
};

/// Common buffer sizes for HM11 operations
using UARTCircularBuffer = CircularBuffer<std::uint8_t, 512>;    ///< General UART data
using DiscoveryBuffer = CircularBuffer<std::uint8_t, 1024>;      ///< Discovery streaming data
using CommandResponseBuffer = CircularBuffer<std::uint8_t, 256>; ///< AT command responses

} // namespace hm11::sync
