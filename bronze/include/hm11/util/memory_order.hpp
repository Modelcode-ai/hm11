/**
 * @file memory_order.hpp
 * @brief Memory ordering utilities for safe concurrent access
 * @details Provides memory ordering constants and utility functions for ISR-to-thread synchronization
 *
 * This file is part of the HM11 driver C++ migration project.
 * Implements Decision 23 regarding memory ordering for lock-free synchronization.
 */

#ifndef HM11_UTIL_MEMORY_ORDER_HPP
#define HM11_UTIL_MEMORY_ORDER_HPP

#include <atomic>
#include <type_traits>

namespace hm11::util {

/**
 * @brief Memory ordering constants for ISR-to-thread synchronization
 *
 * These constants define the memory ordering semantics for atomic operations
 * to ensure safe concurrent access between ISRs and threads.
 *
 * Following Decision 23, we use release/acquire semantics for all
 * ISR-to-thread synchronization, which provides the necessary happens-before
 * relationships for single-producer, single-consumer patterns.
 */
struct memory_order {
    /**
     * @brief Used for publishing data or signaling from ISR (producer) to thread
     *
     * Used when an ISR (producer) is publishing data or signaling by storing
     * a value. This creates a release sequence that ensures all prior memory
     * operations are visible to the consuming thread when it acquires the value.
     *
     * Example: BinarySemaphore::GiveFromISR(), updating circular buffer write position
     */
    static constexpr std::memory_order release = std::memory_order_release;

    /**
     * @brief Used for observing signals or data in thread (consumer) from ISR
     *
     * Used when a thread (consumer) is checking for signals or data by loading
     * a value. This ensures that all memory operations performed by the ISR before
     * the release are visible after the acquire.
     *
     * Example: BinarySemaphore::TakeWithTimeout(), checking circular buffer positions
     */
    static constexpr std::memory_order acquire = std::memory_order_acquire;

    /**
     * @brief Used when clearing state (consumer only)
     *
     * Used when a single consumer is clearing state or performing operations
     * where no synchronization with other threads is needed.
     */
    static constexpr std::memory_order relaxed = std::memory_order_relaxed;
};

/**
 * @brief Apply memory fence with acquire semantics
 *
 * This function is used after DMA completion to ensure visibility of DMA writes.
 * Typically paired with cache invalidation on platforms with data cache.
 *
 * On ARM Cortex-M processors, this generates a DMB (Data Memory Barrier) instruction
 * with appropriate flags.
 */
inline void acquire_fence() {
    std::atomic_thread_fence(std::memory_order_acquire);
}

/**
 * @brief Apply memory fence with release semantics
 *
 * This function is used before starting DMA operations to ensure all prior
 * writes are visible to DMA. Typically paired with cache clean operations on
 * platforms with data cache.
 */
inline void release_fence() {
    std::atomic_thread_fence(std::memory_order_release);
}

/**
 * @brief Atomic operations helper class template
 *
 * This template provides a type-safe wrapper around atomic operations with
 * appropriate memory ordering semantics for different access patterns.
 *
 * @tparam T The underlying value type (must be trivially copyable)
 */
template <typename T> class atomic_ops {
    static_assert(std::is_trivially_copyable_v<T>, "Atomic operations require trivially copyable types");

  public:
    /**
     * @brief Store value with release semantics (ISR-to-thread signaling)
     *
     * Use this function when an ISR needs to signal a thread by storing a value.
     *
     * @param atomic Reference to the atomic variable
     * @param value The value to store
     */
    static void store_release(std::atomic<T>& atomic, T value) { atomic.store(value, memory_order::release); }

    /**
     * @brief Load value with acquire semantics (thread observing ISR signal)
     *
     * Use this function when a thread needs to observe a signal from an ISR.
     *
     * @param atomic Reference to the atomic variable
     * @return The loaded value
     */
    static T load_acquire(const std::atomic<T>& atomic) { return atomic.load(memory_order::acquire); }

    /**
     * @brief Load value with relaxed semantics (no synchronization needed)
     *
     * Use this function when no synchronization is needed, typically for
     * single-thread or read-only access patterns.
     *
     * @param atomic Reference to the atomic variable
     * @return The loaded value
     */
    static T load_relaxed(const std::atomic<T>& atomic) { return atomic.load(memory_order::relaxed); }

    /**
     * @brief Exchange value with acquire-release semantics
     *
     * Use this function to atomically replace the value and get the old value
     * with acquire-release semantics for bidirectional synchronization.
     *
     * @param atomic Reference to the atomic variable
     * @param value The new value to store
     * @return The previous value
     */
    static T exchange(std::atomic<T>& atomic, T value) { return atomic.exchange(value, std::memory_order_acq_rel); }

    /**
     * @brief Compare-exchange operation with acquire-release semantics
     *
     * Use this function to atomically compare and exchange values with
     * acquire-release semantics for bidirectional synchronization.
     *
     * @param atomic Reference to the atomic variable
     * @param expected Reference to the expected value (updated if comparison fails)
     * @param desired The value to store if comparison succeeds
     * @return true if the comparison succeeded and the value was updated
     */
    static bool compare_exchange_strong(std::atomic<T>& atomic, T& expected, T desired) {
        return atomic.compare_exchange_strong(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire);
    }
};

} // namespace hm11::util

#endif // HM11_UTIL_MEMORY_ORDER_HPP