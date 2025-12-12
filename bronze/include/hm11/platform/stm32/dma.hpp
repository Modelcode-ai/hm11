/**
 * @file dma.hpp
 * @brief STM32 DMA configuration and management
 *
 * This file contains DMA controller and stream configuration for STM32 microcontrollers,
 * providing direct access to DMA hardware following Design Decision 30.
 * It implements proper memory alignment and cache management as per Design Decision 20.
 *
 * @note This implementation uses direct register access via CMSIS headers
 * without the STM32 HAL library dependency, matching the Ada implementation
 * in the original codebase.
 */

#ifndef HM11_PLATFORM_STM32_DMA_HPP
#define HM11_PLATFORM_STM32_DMA_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/platform/alignment.hpp>
#include <hm11/platform/cache.hpp>
#include <hm11/platform/stm32/device.hpp>
#include <hm11/sync/binary_semaphore.hpp>
#include <hm11/util/status.hpp>

namespace hm11::platform::stm32 {

/**
 * @brief Data direction for DMA transfers
 */
enum class DmaDirection : std::uint8_t {
    PeripheralToMemory, ///< Transfer data from peripheral to memory
    MemoryToPeripheral, ///< Transfer data from memory to peripheral
    MemoryToMemory      ///< Transfer data from memory to memory
};

/**
 * @brief DMA priority level
 */
enum class DmaPriority : std::uint8_t {
    Low,     ///< Low priority
    Medium,  ///< Medium priority
    High,    ///< High priority
    VeryHigh ///< Very high priority
};

/**
 * @brief DMA flow controller
 */
enum class DmaFlowController : std::uint8_t {
    DMA,       ///< DMA is the flow controller
    Peripheral ///< Peripheral is the flow controller
};

/**
 * @brief DMA burst size
 */
enum class DmaBurstSize : std::uint8_t {
    Single, ///< Single transfer
    Burst4, ///< Burst of 4 transfers
    Burst8, ///< Burst of 8 transfers
    Burst16 ///< Burst of 16 transfers
};

/**
 * @brief DMA memory data size
 */
enum class DmaDataSize : std::uint8_t {
    Byte,     ///< Byte (8-bit)
    HalfWord, ///< Half-word (16-bit)
    Word      ///< Word (32-bit)
};

/**
 * @brief DMA status codes
 */
enum class DmaStatus : std::uint8_t {
    OK,              ///< Operation completed successfully
    Error,           ///< General error
    Timeout,         ///< Operation timed out
    TransferError,   ///< Transfer error
    DirectModeError, ///< Direct mode error
    FifoError,       ///< FIFO error
    Busy             ///< DMA busy with another transfer
};

/**
 * @brief DMA configuration structure
 */
struct DmaConfig {
    DmaDirection direction;             ///< Transfer direction
    bool circular_mode;                 ///< Circular mode
    bool peripheral_increment_mode;     ///< Peripheral increment mode
    bool memory_increment_mode;         ///< Memory increment mode
    DmaDataSize peripheral_data_size;   ///< Peripheral data size
    DmaDataSize memory_data_size;       ///< Memory data size
    DmaPriority priority;               ///< Priority level
    bool peripheral_burst;              ///< Enable peripheral burst
    DmaBurstSize peripheral_burst_size; ///< Peripheral burst size
    bool memory_burst;                  ///< Enable memory burst
    DmaBurstSize memory_burst_size;     ///< Memory burst size
    bool fifo_mode;                     ///< Enable FIFO mode
    std::uint8_t fifo_threshold;        ///< FIFO threshold (0-3)
    DmaFlowController flow_controller;  ///< Flow controller

    /**
     * @brief Construct a DMA configuration with default values
     */
    DmaConfig()
        : direction(DmaDirection::PeripheralToMemory), circular_mode(false), peripheral_increment_mode(false),
          memory_increment_mode(true), peripheral_data_size(DmaDataSize::Byte), memory_data_size(DmaDataSize::Byte),
          priority(DmaPriority::High), peripheral_burst(false), peripheral_burst_size(DmaBurstSize::Single),
          memory_burst(false), memory_burst_size(DmaBurstSize::Single), fifo_mode(false), fifo_threshold(0),
          flow_controller(DmaFlowController::DMA) {}

    /**
     * @brief Configure for UART transmit (memory to peripheral)
     * @return Reference to this configuration for method chaining
     */
    DmaConfig& configure_for_uart_tx() {
        direction = DmaDirection::MemoryToPeripheral;
        circular_mode = false;
        peripheral_increment_mode = false;
        memory_increment_mode = true;
        peripheral_data_size = DmaDataSize::Byte;
        memory_data_size = DmaDataSize::Byte;
        priority = DmaPriority::High;
        peripheral_burst = false;
        memory_burst = false;
        fifo_mode = false;
        return *this;
    }

    /**
     * @brief Configure for UART receive (peripheral to memory)
     * @return Reference to this configuration for method chaining
     */
    DmaConfig& configure_for_uart_rx() {
        direction = DmaDirection::PeripheralToMemory;
        circular_mode = true;
        peripheral_increment_mode = false;
        memory_increment_mode = true;
        peripheral_data_size = DmaDataSize::Byte;
        memory_data_size = DmaDataSize::Byte;
        priority = DmaPriority::High;
        peripheral_burst = false;
        memory_burst = false;
        fifo_mode = false;
        return *this;
    }
};

/**
 * @brief STM32 DMA stream controller
 *
 * This class provides a C++ interface for controlling DMA streams on STM32 microcontrollers.
 * It includes functions for initialization, configuration, and transfer operations.
 */
class DMAStream {
  public:
    /**
     * @brief Construct a new DMAStream object
     *
     * @param dma_index The DMA controller index
     * @param stream The DMA stream
     * @param channel The DMA channel
     */
    explicit DMAStream(device::DMAIndex dma_index, device::DMAStream stream, device::DMAChannel channel);

    /**
     * @brief Destroy the DMAStream object
     */
    ~DMAStream();

    // Rule of 5: delete copy and move operations to ensure proper cleanup of hardware
    DMAStream(const DMAStream&) = delete;
    DMAStream& operator=(const DMAStream&) = delete;
    DMAStream(DMAStream&&) = delete;
    DMAStream& operator=(DMAStream&&) = delete;

    /**
     * @brief Initialize the DMA stream with the given configuration
     *
     * @param config The DMA configuration
     * @return DmaStatus Status of the operation
     */
    DmaStatus init(const DmaConfig& config);

    /**
     * @brief Start a DMA transfer
     *
     * @param src_addr Source address
     * @param dest_addr Destination address
     * @param data_length Number of data items to transfer
     * @return DmaStatus Status of the operation
     */
    DmaStatus start_transfer(std::uint32_t src_addr, std::uint32_t dest_addr, std::uint16_t data_length);

    /**
     * @brief Poll for DMA transfer completion
     *
     * @param timeout_ms Timeout in milliseconds
     * @return DmaStatus Status of the operation
     */
    DmaStatus poll_for_completion(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Stop the DMA transfer
     *
     * @return DmaStatus Status of the operation
     */
    DmaStatus stop_transfer();

    /**
     * @brief Get the number of data items remaining to be transferred
     *
     * @return std::uint16_t Number of remaining data items
     */
    std::uint16_t get_remaining_transfers() const;

    /**
     * @brief Get the DMA stream IRQ
     *
     * @return device::IRQn_Type The IRQ number
     */
    device::IRQn_Type get_irq() const { return device::get_dma_stream_irq(dma_index_, stream_); }

    /**
     * @brief Get the DMA stream peripheral
     *
     * @return DMA_Stream_TypeDef* Pointer to the DMA stream peripheral
     */
    DMA_Stream_TypeDef* get_stream_peripheral() const { return stream_peripheral_; }

    /**
     * @brief Enable DMA interrupts
     *
     * @param transfer_complete Enable transfer complete interrupt
     * @param half_transfer Enable half transfer interrupt
     * @param transfer_error Enable transfer error interrupt
     * @param direct_mode_error Enable direct mode error interrupt
     * @param fifo_error Enable FIFO error interrupt
     */
    void enable_interrupts(
        bool transfer_complete = true,
        bool half_transfer = false,
        bool transfer_error = true,
        bool direct_mode_error = false,
        bool fifo_error = false);

    /**
     * @brief Disable all DMA interrupts
     */
    void disable_interrupts();

    /**
     * @brief Enable NVIC for DMA
     *
     * @param priority The interrupt priority (0-15, lower is higher priority)
     */
    void enable_dma_nvic(std::uint8_t priority = 5);

    /**
     * @brief Disable NVIC for DMA
     */
    void disable_dma_nvic();

    /**
     * @brief Handle DMA interrupt
     *
     * This method is called from the DMA IRQ handler to process DMA interrupts.
     * It handles DMA transfer events and error conditions.
     */
    void handle_irq();

    /**
     * @brief Register a callback function for DMA transfer completion
     *
     * @param callback Callback function to call when DMA transfer completes
     */
    void set_completion_callback(void (*callback)(DmaStatus));

    /**
     * @brief Check if the DMA stream is enabled
     *
     * @return true if the DMA stream is enabled, false otherwise
     */
    bool is_enabled() const;

    /**
     * @brief Get the current DMA status
     *
     * @return DmaStatus The current status
     */
    DmaStatus get_status() const;

    /**
     * @brief Wait for DMA transfer completion
     *
     * @param timeout_ms Timeout in milliseconds
     * @param status Output parameter for operation status
     */
    void wait_for_completion(std::chrono::milliseconds timeout_ms, DmaStatus& status);

  private:
    // Private methods

    /**
     * @brief Clear all DMA stream flags
     */
    void clear_all_flags();

    /**
     * @brief Check if the DMA stream is busy
     *
     * @return true if the DMA stream is busy, false otherwise
     */
    bool is_busy() const;

    /**
     * @brief Get the flag position for the specified DMA stream
     *
     * @return std::uint32_t The flag position
     */
    std::uint32_t get_flag_position() const;

    /**
     * @brief Check if the transfer complete flag is set
     *
     * @return true if the flag is set, false otherwise
     */
    bool is_transfer_complete_flag_set() const;

    /**
     * @brief Check if the half transfer flag is set
     *
     * @return true if the flag is set, false otherwise
     */
    bool is_half_transfer_flag_set() const;

    /**
     * @brief Check if the transfer error flag is set
     *
     * @return true if the flag is set, false otherwise
     */
    bool is_transfer_error_flag_set() const;

    /**
     * @brief Check if the direct mode error flag is set
     *
     * @return true if the flag is set, false otherwise
     */
    bool is_direct_mode_error_flag_set() const;

    /**
     * @brief Check if the FIFO error flag is set
     *
     * @return true if the flag is set, false otherwise
     */
    bool is_fifo_error_flag_set() const;

    /**
     * @brief Clear the transfer complete flag
     */
    void clear_transfer_complete_flag();

    /**
     * @brief Clear the half transfer flag
     */
    void clear_half_transfer_flag();

    /**
     * @brief Clear the transfer error flag
     */
    void clear_transfer_error_flag();

    /**
     * @brief Clear the direct mode error flag
     */
    void clear_direct_mode_error_flag();

    /**
     * @brief Clear the FIFO error flag
     */
    void clear_fifo_error_flag();

    // Private member variables
    DMA_TypeDef* const dma_peripheral_;               ///< Pointer to DMA controller peripheral
    DMA_Stream_TypeDef* const stream_peripheral_;     ///< Pointer to DMA stream peripheral
    const device::DMAIndex dma_index_;                ///< DMA controller index
    const device::DMAStream stream_;                  ///< DMA stream
    const device::DMAChannel channel_;                ///< DMA channel
    std::atomic<DmaStatus> status_;                   ///< Current DMA status
    sync::BinarySemaphore completion_semaphore_;      ///< Completion semaphore
    void (*completion_callback_)(DmaStatus){nullptr}; ///< Completion callback function
};

/**
 * @brief Get the DMAStream instance for the specified DMA stream
 *
 * This function returns a pointer to the DMAStream instance for the
 * specified DMA stream. If the instance doesn't exist, it returns nullptr.
 * This allows DMA IRQ handlers to find the corresponding C++ object.
 *
 * @param dma_index The DMA controller index
 * @param stream The DMA stream
 * @return DMAStream* Pointer to the DMAStream instance
 */
DMAStream* get_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream);

/**
 * @brief Register a DMAStream instance for IRQ handling
 *
 * This function registers a DMAStream instance for IRQ handling. It's called
 * by the DMAStream constructor to make the instance available to IRQ handlers.
 *
 * @param dma_index The DMA controller index
 * @param stream The DMA stream
 * @param dma_stream Pointer to the DMAStream instance
 */
void register_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream, DMAStream* dma_stream);

/**
 * @brief Unregister a DMAStream instance
 *
 * This function unregisters a DMAStream instance. It's called by the DMAStream
 * destructor to remove the instance from the registry.
 *
 * @param dma_index The DMA controller index
 * @param stream The DMA stream
 */
void unregister_dma_stream_instance(device::DMAIndex dma_index, device::DMAStream stream);

/**
 * @brief DMA IRQ handler function
 *
 * This function is called from the DMA IRQ handler to process DMA interrupts.
 * It delegates to the appropriate DMAStream instance.
 *
 * @param dma_index The DMA controller index
 * @param stream The DMA stream
 */
void handle_dma_stream_irq(device::DMAIndex dma_index, device::DMAStream stream);

/**
 * @brief Get the appropriate DMA channel for the specified peripheral and stream
 *
 * @param peripheral_address The peripheral register address
 * @param dma_index The DMA controller index
 * @param stream The DMA stream
 * @return device::DMAChannel The appropriate DMA channel
 */
device::DMAChannel
get_dma_channel_for_peripheral(void* peripheral_address, device::DMAIndex dma_index, device::DMAStream stream);

} // namespace hm11::platform::stm32

#endif // HM11_PLATFORM_STM32_DMA_HPP