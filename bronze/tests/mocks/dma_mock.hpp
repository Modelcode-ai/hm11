/**
 * @file dma_mock.hpp
 * @brief Mock implementation of the DMA interface for testing
 *
 * This file provides a mock implementation of the DMA hardware interface
 * for testing the HM11 driver components. It simulates the behavior of a
 * real DMA peripheral with configurable behavior and instrumentation.
 */

#ifndef HM11_TESTS_MOCKS_DMA_MOCK_HPP
#define HM11_TESTS_MOCKS_DMA_MOCK_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "hm11/sync/binary_semaphore.hpp"
#include "hm11/util/memory_order.hpp"
#include "hm11/platform/cache.hpp"

namespace hm11::test {

/**
 * @brief Mock implementation of the DMA hardware interface
 *
 * This class simulates the behavior of a DMA peripheral with:
 * - Control over data transfers between memory and peripherals
 * - Simulation of interrupts (transfer complete, half-transfer, error)
 * - Configurable error injection
 * - Thread-safe operation
 * - ISR-like behavior for testing synchronization primitives
 */
class DMAMock {
public:
    // Error conditions that can be simulated
    enum class ErrorType : std::uint8_t {
        None = 0,
        TransferError,
        DirectModeError,
        FIFOError,
        AddressError
    };

    // Transfer direction
    enum class Direction : std::uint8_t {
        PeripheralToMemory,
        MemoryToPeripheral,
        MemoryToMemory
    };

    // DMA stream state
    enum class State : std::uint8_t {
        Disabled,
        Configured,
        Running,
        Paused,
        Error
    };

    /**
     * @brief Constructor
     */
    DMAMock()
        : state_(State::Disabled)
        , direction_(Direction::PeripheralToMemory)
        , error_type_(ErrorType::None)
        , complete_interrupt_enabled_(false)
        , half_complete_interrupt_enabled_(false)
        , error_interrupt_enabled_(false)
        , source_address_(nullptr)
        , destination_address_(nullptr)
        , transfer_size_(0)
        , bytes_transferred_(0)
    {
    }

    /**
     * @brief Destructor
     */
    ~DMAMock() = default;

    /**
     * @brief Configure the DMA transfer
     *
     * @param source_address Source address for the transfer
     * @param destination_address Destination address for the transfer
     * @param transfer_size Number of bytes to transfer
     * @param direction Transfer direction
     * @return bool True if configuration was successful
     */
    bool configure(void* source_address, void* destination_address,
                  std::size_t transfer_size, Direction direction) {
        if (state_ == State::Running) {
            return false;  // Cannot configure while running
        }

        source_address_ = source_address;
        destination_address_ = destination_address;
        transfer_size_ = transfer_size;
        direction_ = direction;
        bytes_transferred_ = 0;
        state_ = State::Configured;
        return true;
    }

    /**
     * @brief Enable the DMA transfer
     *
     * @return bool True if successfully enabled
     */
    bool enable() {
        if (state_ != State::Configured && state_ != State::Paused) {
            return false;
        }

        state_ = State::Running;
        return true;
    }

    /**
     * @brief Disable the DMA transfer
     *
     * @return bool True if successfully disabled
     */
    bool disable() {
        if (state_ == State::Disabled) {
            return false;
        }

        state_ = State::Disabled;
        return true;
    }

    /**
     * @brief Pause the DMA transfer
     *
     * @return bool True if successfully paused
     */
    bool pause() {
        if (state_ != State::Running) {
            return false;
        }

        state_ = State::Paused;
        return true;
    }

    /**
     * @brief Get the current DMA state
     *
     * @return State The current state
     */
    State getState() const {
        return state_;
    }

    /**
     * @brief Enable or disable transfer complete interrupt
     *
     * @param enable True to enable, false to disable
     */
    void setCompleteInterruptEnabled(bool enable) {
        complete_interrupt_enabled_ = enable;
    }

    /**
     * @brief Enable or disable half-transfer complete interrupt
     *
     * @param enable True to enable, false to disable
     */
    void setHalfCompleteInterruptEnabled(bool enable) {
        half_complete_interrupt_enabled_ = enable;
    }

    /**
     * @brief Enable or disable error interrupt
     *
     * @param enable True to enable, false to disable
     */
    void setErrorInterruptEnabled(bool enable) {
        error_interrupt_enabled_ = enable;
    }

    /**
     * @brief Check if transfer complete interrupt is enabled
     *
     * @return bool True if enabled, false otherwise
     */
    bool isCompleteInterruptEnabled() const {
        return complete_interrupt_enabled_;
    }

    /**
     * @brief Check if half-transfer complete interrupt is enabled
     *
     * @return bool True if enabled, false otherwise
     */
    bool isHalfCompleteInterruptEnabled() const {
        return half_complete_interrupt_enabled_;
    }

    /**
     * @brief Check if error interrupt is enabled
     *
     * @return bool True if enabled, false otherwise
     */
    bool isErrorInterruptEnabled() const {
        return error_interrupt_enabled_;
    }

    /**
     * @brief Set the transfer complete interrupt callback
     *
     * @param callback Function to call when transfer is complete
     */
    void setCompleteInterruptCallback(std::function<void()> callback) {
        complete_interrupt_callback_ = std::move(callback);
    }

    /**
     * @brief Set the half-transfer complete interrupt callback
     *
     * @param callback Function to call when half-transfer is complete
     */
    void setHalfCompleteInterruptCallback(std::function<void()> callback) {
        half_complete_interrupt_callback_ = std::move(callback);
    }

    /**
     * @brief Set the error interrupt callback
     *
     * @param callback Function to call when an error occurs
     */
    void setErrorInterruptCallback(std::function<void()> callback) {
        error_interrupt_callback_ = std::move(callback);
    }

    /**
     * @brief Simulate an error condition
     *
     * @param error_type Type of error to simulate
     * @return bool True if error interrupt was triggered, false otherwise
     */
    bool simulateError(ErrorType error_type) {
        if (state_ != State::Running || !error_interrupt_enabled_ || !error_interrupt_callback_) {
            return false;
        }

        error_type_ = error_type;
        state_ = State::Error;
        error_interrupt_callback_();
        return true;
    }

    /**
     * @brief Simulate a partial transfer
     *
     * This method simulates a DMA transfer of a specified number of bytes.
     * If the number reaches half of the total, it triggers a half-transfer interrupt.
     * If the number reaches the total, it triggers a transfer complete interrupt.
     *
     * @param bytes_to_transfer Number of bytes to transfer
     * @return size_t Number of bytes actually transferred
     */
    std::size_t simulatePartialTransfer(std::size_t bytes_to_transfer) {
        if (state_ != State::Running) {
            return 0;
        }

        // Calculate how many bytes we can actually transfer
        const std::size_t remaining_bytes = transfer_size_ - bytes_transferred_;
        const std::size_t actual_transfer = std::min(bytes_to_transfer, remaining_bytes);
        
        if (actual_transfer == 0) {
            return 0;
        }

        // Perform the transfer based on direction
        if (direction_ == Direction::PeripheralToMemory || direction_ == Direction::MemoryToMemory) {
            // If destination is memory, perform the actual copy
            if (source_address_ && destination_address_) {
                // Cast to uint8_t* for byte-wise access
                auto src = static_cast<const std::uint8_t*>(source_address_);
                auto dst = static_cast<std::uint8_t*>(destination_address_);
                
                // Copy data byte by byte (simulating DMA behavior)
                std::memcpy(dst + bytes_transferred_, src + bytes_transferred_, actual_transfer);
            }
        } else if (direction_ == Direction::MemoryToPeripheral) {
            // For memory to peripheral, we just track the bytes transferred
            // The actual peripheral behavior would be simulated elsewhere
        }

        // Update bytes transferred
        bytes_transferred_ += actual_transfer;

        // Check for half-transfer
        const std::size_t half_size = transfer_size_ / 2;
        if (half_complete_interrupt_enabled_ && half_complete_interrupt_callback_ &&
            bytes_transferred_ >= half_size && bytes_transferred_ - actual_transfer < half_size) {
            // We just crossed the half-transfer threshold
            half_complete_interrupt_callback_();
        }

        // Check for transfer complete
        if (bytes_transferred_ >= transfer_size_) {
            // Transfer is complete
            bytes_transferred_ = transfer_size_;
            state_ = State::Configured;  // Return to configured state
            
            // Add memory fence to ensure all writes are visible (simulating DMA behavior)
            hm11::util::acquire_fence();

            // For Cortex-M7, also invalidate data cache
            if (direction_ == Direction::PeripheralToMemory && destination_address_) {
                hm11::platform::cache::invalidate_dma_buffer(
                    destination_address_, transfer_size_);
            }

            // Trigger transfer complete interrupt
            if (complete_interrupt_enabled_ && complete_interrupt_callback_) {
                complete_interrupt_callback_();
            }
        }

        return actual_transfer;
    }

    /**
     * @brief Simulate a complete transfer in one step
     *
     * @return size_t Number of bytes transferred
     */
    std::size_t simulateCompleteTransfer() {
        return simulatePartialTransfer(transfer_size_);
    }

    /**
     * @brief Simulate a complete transfer with a specified completion time
     *
     * This method simulates a DMA transfer that takes a specific amount of time to complete.
     * It runs in a separate thread to avoid blocking the caller.
     *
     * @param completion_time Time to complete the transfer
     * @return bool True if transfer was started, false otherwise
     */
    bool simulateTimedTransfer(std::chrono::milliseconds completion_time) {
        if (state_ != State::Running || transfer_size_ == 0) {
            return false;
        }

        // Calculate bytes per millisecond for smooth simulation
        const double bytes_per_ms = static_cast<double>(transfer_size_) / static_cast<double>(completion_time.count());
        
        // Start a thread to simulate the transfer
        transfer_thread_ = std::thread([this, bytes_per_ms, completion_time]() {
            const std::size_t bytes_per_step = std::max<std::size_t>(1, static_cast<std::size_t>(bytes_per_ms * 10));
            const std::chrono::milliseconds step_time(10);
            
            auto start_time = std::chrono::steady_clock::now();
            
            while (bytes_transferred_ < transfer_size_ && state_ == State::Running) {
                // Sleep for the step time
                std::this_thread::sleep_for(step_time);
                
                // Calculate progress based on elapsed time
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                
                // Calculate how many bytes should be transferred by now
                const std::size_t target_bytes = static_cast<std::size_t>(bytes_per_ms * static_cast<double>(elapsed_ms));
                const std::size_t bytes_to_transfer = target_bytes - bytes_transferred_;
                
                if (bytes_to_transfer > 0) {
                    simulatePartialTransfer(bytes_to_transfer);
                }
                
                // Check if we're done
                if (elapsed_ms >= completion_time.count() || bytes_transferred_ >= transfer_size_) {
                    // Ensure we complete the transfer
                    simulateCompleteTransfer();
                    break;
                }
            }
        });

        // Detach the thread to allow it to continue independently
        transfer_thread_.detach();
        return true;
    }

    /**
     * @brief Get the number of bytes transferred
     *
     * @return size_t Number of bytes transferred
     */
    std::size_t getBytesTransferred() const {
        return bytes_transferred_;
    }

    /**
     * @brief Get the total transfer size
     *
     * @return size_t Total transfer size
     */
    std::size_t getTransferSize() const {
        return transfer_size_;
    }

    /**
     * @brief Get the transfer progress as a percentage
     *
     * @return float Percentage of transfer completed (0-100)
     */
    float getTransferProgress() const {
        if (transfer_size_ == 0) {
            return 0.0f;
        }
        return (static_cast<float>(bytes_transferred_) / static_cast<float>(transfer_size_)) * 100.0f;
    }

private:
    // DMA state
    std::atomic<State> state_;
    std::atomic<Direction> direction_;
    std::atomic<ErrorType> error_type_;
    
    // Interrupt flags
    std::atomic<bool> complete_interrupt_enabled_;
    std::atomic<bool> half_complete_interrupt_enabled_;
    std::atomic<bool> error_interrupt_enabled_;
    
    // Transfer configuration
    void* source_address_;
    void* destination_address_;
    std::size_t transfer_size_;
    std::atomic<std::size_t> bytes_transferred_;
    
    // Interrupt callbacks
    std::function<void()> complete_interrupt_callback_;
    std::function<void()> half_complete_interrupt_callback_;
    std::function<void()> error_interrupt_callback_;
    
    // Thread for timed transfers
    std::thread transfer_thread_;
};

} // namespace hm11::test

#endif // HM11_TESTS_MOCKS_DMA_MOCK_HPP