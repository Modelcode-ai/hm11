/**
 * @file dma_receive_handler.cpp
 * @brief DMA receive handler implementation
 *
 * This file implements the non-template methods of the DMA_Receive_Handler class.
 * The main receive logic is in the header file as a template method.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "hm11/stm32/dma_receive_handler.hpp"

#include <algorithm>

namespace hm11::stm32 {

DMA_Receive_Handler::DMA_Receive_Handler(sync::Watchdog& watchdog) noexcept : watchdog_(watchdog) {
    // Initialize atomic state to defaults
    // (already done by member initializers)
}

DMA_Receive_Handler::~DMA_Receive_Handler() noexcept {
    // If streaming is active, stop it
    if (is_stream_.load(std::memory_order_acquire)) {
        watchdog_.stop();
        is_stream_.store(false, std::memory_order_release);
    }
}

std::size_t DMA_Receive_Handler::get_write_position() const noexcept {
    // Load write position with acquire ordering to observe ISR writes
    // (Decision 23: Memory Ordering for Lock-Free Synchronization)
    return write_position_.load(std::memory_order_acquire);
}

bool DMA_Receive_Handler::stream_closed() const noexcept {
    // Stream is closed if we're in streaming mode but the watchdog is no longer started
    // This happens when timeout expires or error occurs
    if (!is_stream_.load(std::memory_order_acquire)) {
        return true; // Not in streaming mode = closed
    }

    // Check if watchdog is still active
    // Note: This is a simple implementation. The Watchdog class doesn't expose
    // a direct "is_started" query, but we can infer closure from the event_occurred
    // flag or by the application tracking when it called await_event().
    // For now, we'll return false (stream still open) and let the application
    // decide when to stop based on timeout or parsed data.
    return false;
}

} // namespace hm11::stm32
