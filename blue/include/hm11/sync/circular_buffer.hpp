#pragma once

/**
 * @file circular_buffer.hpp
 * @brief Lock‑free single‑producer‑single‑consumer circular buffer.
 *
 * This buffer is intended for ISR‑to‑thread communication.  The producer
 * (typically an ISR) pushes items using an atomic write position with a
 * `memory_order_release` to publish the data.  The consumer reads using a
 * non‑atomic read position and loads the write position with
 * `memory_order_acquire`.
 *
 * The size of the buffer must be a power of two and at least two elements.
 * A power‑of‑two size enables fast wrap‑around using a bit mask.
 */

#include <array>
#include <atomic>
#include <cstddef>

namespace hm11 {

/**
 * @brief Simple lock‑free SPSC circular buffer.
 *
 * @tparam T    Type of elements stored in the buffer.
 * @tparam Size Capacity of the buffer (must be power of two and >= 2).
 */
template <typename T, std::size_t Size> class CircularBuffer {
  public:
    static_assert(Size >= 2, "CircularBuffer size must be at least 2");
    static_assert((Size & (Size - 1)) == 0, "CircularBuffer size must be a power of two");

    /** Construct an empty buffer. */
    constexpr CircularBuffer() noexcept = default;

    /**
     * @brief Write an item into the buffer.
     *
     * This function is safe to call from an ISR (producer).  It stores the
     * element and then publishes the new write position with
     * `std::memory_order_release`. If the buffer is full, this will overwrite
     * the oldest unread data, matching Ada ISR circular buffer semantics.
     *
     * @param item Item to write.
     */
    void write(const T& item) noexcept {
        const std::size_t W = write_pos.load(std::memory_order_relaxed);
        const std::size_t R = read_pos;
        const std::size_t NEXT = (W + 1) & MASK;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer[W & MASK] = item;

        // If buffer is full, we overwrite oldest data, so advance read position
        if (NEXT == R) {
            read_pos = (R + 1) & MASK;
        }

        // Release the new write position so the consumer sees the data.
        write_pos.store(NEXT, std::memory_order_release);
    }

    /**
     * @brief Pop an item from the buffer.
     *
     * This function is intended for the consumer (thread) side.  It loads the
     * write position with `std::memory_order_acquire` to ensure the most recent
     * writes are visible.
     *
     * @param out Destination for the popped item.
     * @return true  An item was retrieved.
     * @return false Buffer was empty.
     */
    bool pop(T& out) noexcept {
        const std::size_t W = write_pos.load(std::memory_order_acquire);
        const std::size_t R = read_pos;
        if (R == W) {
            // Buffer empty.
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        out = buffer[R & MASK];
        read_pos = (R + 1) & MASK;
        return true;
    }

    /** @brief Check whether the buffer is empty. */
    bool empty() const noexcept { return read_pos == write_pos.load(std::memory_order_acquire); }

    /** @brief Check whether the buffer is full. */
    bool full() const noexcept {
        const std::size_t W = write_pos.load(std::memory_order_acquire);
        const std::size_t NEXT = (W + 1) & MASK;
        return NEXT == read_pos;
    }

  private:
    static constexpr std::size_t MASK = Size - 1;
    alignas(32) std::array<T, Size> buffer{};
    std::atomic<std::size_t> write_pos{0}; // producer index
    std::size_t read_pos{0};               // consumer index (non‑atomic)
};

} // namespace hm11
