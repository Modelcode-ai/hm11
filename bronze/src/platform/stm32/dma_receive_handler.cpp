/**
 * @file dma_receive_handler.cpp
 * @brief Implementation of DMA receive handler for STM32 UART
 *
 * This file implements the DMA receive handler for STM32 UART peripherals,
 * supporting both normal and stream modes for data reception, with proper
 * timeout handling using the Watchdog mechanism.
 */

#include <hm11/platform/stm32/dma_receive_handler.hpp>
#include <hm11/platform/stm32/watchdog.hpp>

namespace hm11::platform::stm32 {

DmaReceiveHandler::DmaReceiveHandler(DMAStream& dma, USART_TypeDef* uart, std::size_t buffer_size)
    : dma_(dma), uart_(uart) {
    // Initialize DMA buffer (should already be zero-initialized by the constructor)
    dma_buffer_.clear();
}

DmaReceiveHandler::~DmaReceiveHandler() {
    // Ensure any active reception is stopped
    stop();
}

bool DmaReceiveHandler::start_normal(
    void* buffer,
    std::size_t size,
    std::chrono::milliseconds timeout_ms,
    sync::Watchdog& watchdog) {
    // Check if already active
    bool expected_active = false;
    if (!active_.compare_exchange_strong(expected_active, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        return false; // Already active
    }

    // Store buffer information
    buffer_ = buffer;
    buffer_size_ = size;
    max_position_ = size;
    position_.store(1, std::memory_order_relaxed); // 1-based index like Ada

    // Set mode and watchdog
    mode_ = ReceiveMode::Normal;
    watchdog_ = &watchdog;

    // Start the watchdog
    auto* stm32_watchdog = dynamic_cast<Stm32Watchdog*>(&watchdog);
    if (stm32_watchdog) {
        stm32_watchdog->start_dma(buffer, size, timeout_ms, uart_, false);
    } else {
        watchdog.start(buffer, size, timeout_ms);
    }

    // Clear DMA buffer
    dma_buffer_.clear();

    // Configure DMA
    DmaConfig config;
    config.configure_for_uart_rx();
    config.circular_mode = false; // One-shot mode for normal reception

    // Initialize DMA
    dma_.init(config);

    // Enable DMA
    // For normal mode, DMA reads directly into the user buffer
    std::uint32_t peripheral_address = reinterpret_cast<std::uint32_t>(&(uart_->DR));
    std::uint32_t memory_address = reinterpret_cast<std::uint32_t>(buffer);

    // Enable interrupts
    dma_.enable_interrupts(true, false, true, false, false); // TC and TE interrupts

    // Start DMA transfer
    dma_.start_transfer(peripheral_address, memory_address, static_cast<std::uint16_t>(size));

    // Enable UART DMA reception
    uart_->CR3 |= USART_CR3_DMAR;

    // Enable UART IDLE interrupt for end-of-packet detection
    uart_->CR1 |= USART_CR1_IDLEIE;

    return true;
}

bool DmaReceiveHandler::start_stream(
    void* buffer,
    std::size_t size,
    std::chrono::milliseconds timeout_ms,
    sync::Watchdog& watchdog) {
    // Check if already active
    bool expected_active = false;
    if (!active_.compare_exchange_strong(expected_active, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        return false; // Already active
    }

    // Store buffer information
    buffer_ = buffer;
    buffer_size_ = size;
    max_position_ = size;
    position_.store(1, std::memory_order_relaxed); // 1-based index like Ada

    // Set mode and watchdog
    mode_ = ReceiveMode::Stream;
    watchdog_ = &watchdog;

    // Start the watchdog
    auto* stm32_watchdog = dynamic_cast<Stm32Watchdog*>(&watchdog);
    if (stm32_watchdog) {
        stm32_watchdog->start_dma(buffer, size, timeout_ms, uart_, true);
    } else {
        watchdog.start(buffer, size, timeout_ms);
    }

    // Clear DMA buffer
    dma_buffer_.clear();

    // Configure DMA
    DmaConfig config;
    config.configure_for_uart_rx();
    config.circular_mode = true; // Circular mode for stream reception

    // Initialize DMA
    dma_.init(config);

    // For stream mode, DMA writes into the internal buffer, and we copy to the user buffer
    // This approach is similar to the Ada implementation, where the DMA writes to a fixed buffer
    // and the Ada code copies the data to the user buffer
    std::uint32_t peripheral_address = reinterpret_cast<std::uint32_t>(&(uart_->DR));
    std::uint32_t memory_address = reinterpret_cast<std::uint32_t>(dma_buffer_.data());

    // Enable interrupts
    dma_.enable_interrupts(true, true, true, false, false); // TC, HT, and TE interrupts

    // Start DMA transfer
    dma_.start_transfer(peripheral_address, memory_address, DEFAULT_BUFFER_SIZE);

    // Enable UART DMA reception
    uart_->CR3 |= USART_CR3_DMAR;

    // Enable UART IDLE interrupt for end-of-packet detection
    uart_->CR1 |= USART_CR1_IDLEIE;

    return true;
}

void DmaReceiveHandler::stop() {
    // Check if active
    bool expected_active = true;
    if (!active_
             .compare_exchange_strong(expected_active, false, std::memory_order_acquire, std::memory_order_relaxed)) {
        return; // Not active
    }

    // Disable UART DMA reception
    if (uart_) {
        uart_->CR3 &= ~USART_CR3_DMAR;

        // Disable UART IDLE interrupt
        uart_->CR1 &= ~USART_CR1_IDLEIE;
    }

    // Stop DMA transfer
    dma_.stop_transfer();

    // Disable DMA interrupts
    dma_.disable_interrupts();

    // Stop watchdog
    if (watchdog_) {
        auto* stm32_watchdog = dynamic_cast<Stm32Watchdog*>(watchdog_);
        if (stm32_watchdog) {
            stm32_watchdog->stop_dma();
        } else {
            watchdog_->stop();
        }
        watchdog_ = nullptr;
    }

    // Reset state
    buffer_ = nullptr;
    buffer_size_ = 0;
    position_.store(1, std::memory_order_relaxed);
    max_position_ = 1;
}

bool DmaReceiveHandler::is_active() const {
    return active_.load(std::memory_order_acquire);
}

void DmaReceiveHandler::handle_dma_complete() {
    if (!is_active()) {
        return; // Not active
    }

    // Check for errors
    if (watchdog_) {
        auto* stm32_watchdog = dynamic_cast<Stm32Watchdog*>(watchdog_);
        if (stm32_watchdog) {
            stm32_watchdog->check_uart_errors();
        }
    }

    if (mode_ == ReceiveMode::Normal) {
        // Normal mode - complete the transfer
        if (watchdog_) {
            // Set position to the end of the buffer (all data received)
            position_.store(buffer_size_ + 1, std::memory_order_release);
            watchdog_->set_position(buffer_size_ + 1);

            // Signal completion
            watchdog_->release(sync::UartStatus::Ok);
        }
    } else {
        // Stream mode - update position
        if (buffer_ && watchdog_) {
            // In stream mode, DMA complete doesn't end the transfer
            // We just wrap around and continue receiving

            // Update position
            std::size_t dma_remaining = dma_.get_remaining_transfers();
            std::size_t dma_progress = DEFAULT_BUFFER_SIZE - dma_remaining;

            // Calculate new position (wrap around if needed)
            std::size_t new_position = dma_progress + 1; // +1 for 1-based indexing
            if (new_position > buffer_size_) {
                new_position = 1;
            }

            // Copy data from DMA buffer to user buffer
            if (buffer_ && dma_buffer_.data()) {
                for (std::size_t i = 0; i < dma_progress; ++i) {
                    static_cast<std::uint8_t*>(buffer_)[i] = dma_buffer_[i];
                }
            }

            // Update position
            position_.store(new_position, std::memory_order_release);
            watchdog_->set_position(new_position);

            // Check UART status register to clear IDLE flag if set
            volatile std::uint32_t temp = uart_->SR;
            temp = uart_->DR;
            (void)temp; // Avoid unused variable warning
        }
    }
}

void DmaReceiveHandler::handle_dma_error(sync::UartStatus status) {
    if (!is_active()) {
        return; // Not active
    }

    // Signal error to watchdog
    if (watchdog_) {
        watchdog_->release(status);
    }

    // Stop DMA reception
    stop();
}

void DmaReceiveHandler::handle_uart_idle() {
    if (!is_active()) {
        return; // Not active
    }

    // Check UART status register to clear IDLE flag
    volatile std::uint32_t temp = uart_->SR;
    temp = uart_->DR;
    (void)temp; // Avoid unused variable warning

    // Check for errors
    if (watchdog_) {
        auto* stm32_watchdog = dynamic_cast<Stm32Watchdog*>(watchdog_);
        if (stm32_watchdog) {
            if (stm32_watchdog->check_uart_errors()) {
                return; // Error handled
            }
        }
    }

    if (mode_ == ReceiveMode::Normal) {
        // For normal mode, IDLE indicates end of reception
        if (watchdog_) {
            // Get current DMA position
            std::size_t dma_remaining = dma_.get_remaining_transfers();
            std::size_t received = buffer_size_ - dma_remaining;

            // Update position
            position_.store(received + 1, std::memory_order_release); // +1 for 1-based indexing
            watchdog_->set_position(received + 1);

            // Signal completion
            watchdog_->release(sync::UartStatus::Ok);

            // Stop DMA reception
            stop();
        }
    } else {
        // For stream mode, IDLE indicates end of a packet, but we continue reception
        if (buffer_ && watchdog_) {
            // Get current DMA position
            std::size_t dma_remaining = dma_.get_remaining_transfers();
            std::size_t dma_position = DEFAULT_BUFFER_SIZE - dma_remaining;

            // Copy data from DMA buffer to user buffer
            if (buffer_ && dma_buffer_.data()) {
                // Calculate how much data to copy and where to put it
                std::size_t current_position = position_.load(std::memory_order_acquire);
                std::size_t bytes_to_copy = dma_position;

                // Adjust for circular buffer wrap-around
                if (current_position + bytes_to_copy > max_position_) {
                    bytes_to_copy = max_position_ - current_position + 1;
                }

                // Copy data
                for (std::size_t i = 0; i < bytes_to_copy; ++i) {
                    static_cast<std::uint8_t*>(buffer_)[(current_position - 1 + i) % buffer_size_] = dma_buffer_[i];
                }

                // Update position
                std::size_t new_position = current_position + bytes_to_copy;
                if (new_position > max_position_) {
                    new_position = 1 + (new_position - max_position_);
                }

                position_.store(new_position, std::memory_order_release);
                watchdog_->set_position(new_position);
            }
        }
    }
}

void DmaReceiveHandler::check_timeout() {
    if (!is_active() || !watchdog_) {
        return; // Not active or no watchdog
    }

    // Check if deadline exceeded
    if (watchdog_->is_deadline_exceeded()) {
        // Signal timeout
        watchdog_->release(sync::UartStatus::ErrTimeout);

        // Stop DMA reception
        stop();
    }
}

} // namespace hm11::platform::stm32