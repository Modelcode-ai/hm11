/**
 * @file circular_buffer.hpp
 * @brief Implementation of a circular (ring) buffer for the HM11 driver
 *
 * This file contains a circular buffer implementation to support
 * parsing operations in the HM11 driver, particularly for streaming
 * data reception.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_UTIL_CIRCULAR_BUFFER_HPP
#define HM11_UTIL_CIRCULAR_BUFFER_HPP

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace hm11::util {

/**
 * @brief Single-producer single-consumer lock-free circular buffer
 *
 * This class implements a lock-free circular buffer optimized for the case where
 * there is exactly one producer and one consumer. It provides safe, concurrent
 * operations without requiring mutexes.
 *
 * @tparam T Element type
 * @tparam Capacity Buffer capacity (must be > 0)
 */
template <typename T, std::size_t Capacity> class CircularBuffer {
  public:
    static_assert(Capacity > 0, "Buffer capacity must be greater than 0");

    /**
     * @brief Constructs a new circular buffer
     */
    CircularBuffer() {
        read_pos_.store(0, std::memory_order_relaxed);
        write_pos_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Copy constructor is deleted (can't copy atomic members)
     */
    CircularBuffer(const CircularBuffer&) = delete;

    /**
     * @brief Assignment operator is deleted (can't copy atomic members)
     */
    CircularBuffer& operator=(const CircularBuffer&) = delete;

    /**
     * @brief Returns the buffer's capacity
     */
    constexpr std::size_t capacity() const { return Capacity; }

    /**
     * @brief Returns the current number of elements in the buffer
     */
    std::size_t size() const {
        const std::size_t w = write_pos_.load(std::memory_order_acquire);
        const std::size_t r = read_pos_.load(std::memory_order_acquire);
        return w >= r ? w - r : w + Capacity - r;
    }

    /**
     * @brief Checks if the buffer is empty
     */
    bool empty() const {
        return write_pos_.load(std::memory_order_acquire) == read_pos_.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if the buffer is full
     */
    bool full() const {
        const std::size_t w = write_pos_.load(std::memory_order_acquire);
        const std::size_t next_w = (w + 1) % Capacity;
        return next_w == read_pos_.load(std::memory_order_acquire);
    }

    /**
     * @brief Writes data to the buffer (producer operation)
     *
     * @param item Item to write
     * @return true if the write succeeded, false if the buffer was full
     */
    bool write(const T& item) {
        const std::size_t w = write_pos_.load(std::memory_order_relaxed);
        const std::size_t next_w = (w + 1) % Capacity;

        if (next_w == read_pos_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }

        buffer_[w] = item;
        write_pos_.store(next_w, std::memory_order_release);
        return true;
    }

    /**
     * @brief Writes multiple elements to the buffer (producer operation)
     *
     * @param data Span of data to write
     * @return std::size_t Number of elements actually written
     */
    std::size_t write(std::span<const T> data) {
        std::size_t count = 0;
        for (const auto& item : data) {
            if (!write(item)) {
                break;
            }
            count++;
        }
        return count;
    }

    /**
     * @brief Reads an item from the buffer (consumer operation)
     *
     * @param item Reference to store the read item
     * @return true if the read succeeded, false if the buffer was empty
     */
    bool read(T& item) {
        const std::size_t r = read_pos_.load(std::memory_order_relaxed);
        if (r == write_pos_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }

        item = buffer_[r];
        read_pos_.store((r + 1) % Capacity, std::memory_order_release);
        return true;
    }

    /**
     * @brief Reads multiple elements from the buffer (consumer operation)
     *
     * @param output Span to store the read items
     * @return std::size_t Number of elements actually read
     */
    std::size_t read(std::span<T> output) {
        std::size_t count = 0;
        for (auto& item : output) {
            if (!read(item)) {
                break;
            }
            count++;
        }
        return count;
    }

    /**
     * @brief Peeks at an item in the buffer without removing it
     *
     * @param offset Offset from the current read position
     * @param item Reference to store the peeked item
     * @return true if the peek succeeded, false if the position was invalid
     */
    bool peek(std::size_t offset, T& item) const {
        const std::size_t r = read_pos_.load(std::memory_order_acquire);
        const std::size_t w = write_pos_.load(std::memory_order_acquire);

        if (r == w) {
            return false; // Buffer empty
        }

        std::size_t avail = r <= w ? w - r : Capacity - r + w;
        if (offset >= avail) {
            return false; // Beyond available data
        }

        item = buffer_[(r + offset) % Capacity];
        return true;
    }

    /**
     * @brief Gets the current read position
     */
    std::size_t get_read_position() const { return read_pos_.load(std::memory_order_acquire); }

    /**
     * @brief Gets the current write position
     */
    std::size_t get_write_position() const { return write_pos_.load(std::memory_order_acquire); }

    /**
     * @brief Clears the buffer
     */
    void clear() {
        read_pos_.store(0, std::memory_order_release);
        write_pos_.store(0, std::memory_order_release);
    }

  private:
    // Buffer storage
    T buffer_[Capacity];

    // Read position (modified by consumer, read by producer - must be atomic)
    std::atomic<std::size_t> read_pos_;

    // Write position (modified by producer, read by consumer - must be atomic)
    std::atomic<std::size_t> write_pos_;
};

} // namespace hm11::util

#endif // HM11_UTIL_CIRCULAR_BUFFER_HPP