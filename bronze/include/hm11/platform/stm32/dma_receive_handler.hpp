/**
 * @file dma_receive_handler.hpp
 * @brief DMA receive handler for STM32 UART
 * @details Implements Decision 12 and 23 regarding circular buffer synchronization and memory ordering
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides the DMA receive handler interface for STM32 microcontrollers.
 */

#ifndef HM11_PLATFORM_STM32_DMA_RECEIVE_HANDLER_HPP
#define HM11_PLATFORM_STM32_DMA_RECEIVE_HANDLER_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/platform/alignment.hpp>
#include <hm11/platform/cache.hpp>
#include <hm11/platform/stm32/device.hpp>
#include <hm11/platform/stm32/dma.hpp>
#include <hm11/platform/stm32/uart.hpp>
#include <hm11/sync/binary_semaphore.hpp>
#include <hm11/sync/circular_buffer.hpp>
#include <hm11/sync/watchdog.hpp>

namespace hm11::platform::stm32 {

/**
 * @brief Receive modes for DMA handler
 */
enum class ReceiveMode : std::uint8_t {
    Normal, ///< Normal one-shot mode
    Stream  ///< Stream mode for continuous reception
};

/**
 * @brief DMA receive handler for STM32 UART
 *
 * This class implements the DMA receive handler for STM32 UART peripherals.
 * It supports both normal and stream modes for data reception, with proper
 * timeout handling using the Watchdog mechanism.
 *
 * The implementation follows Decision 12 for circular buffer synchronization
 * and Decision 23 for memory ordering in lock-free operations.
 */
class DmaReceiveHandler {
  public:
    // Default buffer size
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 256;

    /**
     * @brief Construct a new DMA receive handler
     *
     * @param dma The DMA stream to use for reception
     * @param uart The UART peripheral for data reception
     * @param buffer_size Size of the internal buffer (default 256 bytes)
     */
    explicit DmaReceiveHandler(DMAStream& dma, USART_TypeDef* uart, std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

    /**
     * @brief Destroy the DMA receive handler
     *
     * Stops any ongoing reception and cleans up resources.
     */
    ~DmaReceiveHandler();

    // Delete copy and move operations to ensure proper resource management
    DmaReceiveHandler(const DmaReceiveHandler&) = delete;
    DmaReceiveHandler& operator=(const DmaReceiveHandler&) = delete;
    DmaReceiveHandler(DmaReceiveHandler&&) = delete;
    DmaReceiveHandler& operator=(DmaReceiveHandler&&) = delete;

    /**
     * @brief Start a DMA receive operation in normal mode
     *
     * This method starts a DMA reception operation in normal (one-shot) mode,
     * receiving data until the specified buffer is full or a timeout occurs.
     *
     * @param buffer Pointer to the receive buffer
     * @param size Size of the receive buffer
     * @param timeout_ms Timeout in milliseconds
     * @param watchdog Watchdog for timeout monitoring
     * @return true if successfully started, false otherwise
     */
    bool start_normal(void* buffer, std::size_t size, std::chrono::milliseconds timeout_ms, sync::Watchdog& watchdog);

    /**
     * @brief Start a DMA receive operation in stream mode
     *
     * This method starts a DMA reception operation in stream mode,
     * continuously receiving data into a circular buffer. Data is read
     * from the buffer using the get_position method.
     *
     * @param buffer Pointer to the circular buffer
     * @param size Size of the circular buffer
     * @param timeout_ms Timeout in milliseconds
     * @param watchdog Watchdog for timeout monitoring
     * @return true if successfully started, false otherwise
     */
    bool start_stream(void* buffer, std::size_t size, std::chrono::milliseconds timeout_ms, sync::Watchdog& watchdog);

    /**
     * @brief Stop the DMA receive operation
     *
     * This method stops any ongoing DMA reception and releases resources.
     */
    void stop();

    /**
     * @brief Check if a DMA receive operation is active
     *
     * @return true if receiving, false otherwise
     */
    bool is_active() const;

    /**
     * @brief Handle DMA complete interrupt
     *
     * This method is called from the DMA IRQ handler when a transfer completes.
     * It signals completion to the waiting thread.
     */
    void handle_dma_complete();

    /**
     * @brief Handle DMA error interrupt
     *
     * This method is called from the DMA IRQ handler when an error occurs.
     * It signals an error to the waiting thread.
     *
     * @param status The error status code
     */
    void handle_dma_error(sync::UartStatus status);

    /**
     * @brief Handle UART idle interrupt
     *
     * This method is called from the UART IRQ handler when the UART goes idle
     * (no more data being received). In stream mode, this indicates the end
     * of a data packet.
     */
    void handle_uart_idle();

    /**
     * @brief Check for timeout and handle if needed
     *
     * This method is called periodically to check if the reception operation
     * has timed out. If so, it signals a timeout to the waiting thread.
     */
    void check_timeout();

  private:
    // Internal state
    DMAStream& dma_;                        ///< DMA stream for reception
    USART_TypeDef* uart_;                   ///< UART peripheral
    std::atomic<bool> active_{false};       ///< Flag indicating if reception is active
    ReceiveMode mode_{ReceiveMode::Normal}; ///< Current reception mode
    sync::Watchdog* watchdog_{nullptr};     ///< Pointer to watchdog for timeout handling

    // Buffer information
    void* buffer_{nullptr};                ///< Pointer to the receive buffer
    std::size_t buffer_size_{0};           ///< Size of the receive buffer
    std::atomic<std::size_t> position_{1}; ///< Current position in buffer (1-based)
    std::size_t max_position_{1};          ///< Maximum position in buffer (for bounds checking)

    // DMA buffer with 32-byte alignment
    platform::DmaAlignedBuffer<std::uint8_t, DEFAULT_BUFFER_SIZE> dma_buffer_; ///< Internal DMA buffer
};

} // namespace hm11::platform::stm32

#endif // HM11_PLATFORM_STM32_DMA_RECEIVE_HANDLER_HPP