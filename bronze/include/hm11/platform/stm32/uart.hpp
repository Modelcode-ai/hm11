/**
 * @file uart.hpp
 * @brief STM32 UART implementation for HM11 driver
 *
 * This file defines the UART implementation for STM32 microcontrollers,
 * providing a concrete implementation of the HAL UART interface. It uses
 * direct register access as specified in Design Decision 30.
 *
 * @note This implementation uses direct register access via CMSIS headers
 * without the STM32 HAL library dependency, matching the Ada implementation
 * in the original codebase.
 */

#ifndef HM11_PLATFORM_STM32_UART_HPP
#define HM11_PLATFORM_STM32_UART_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/hal/uart.hpp>
#include <hm11/platform/alignment.hpp>
#include <hm11/platform/cache.hpp>
#include <hm11/platform/stm32/device.hpp>
#include <hm11/platform/stm32/gpio_config.hpp>
#include <hm11/platform/stm32/peripherals.hpp>
#include <hm11/sync/binary_semaphore.hpp>
#include <hm11/sync/watchdog.hpp>
#include <memory>

namespace hm11::platform::stm32 {

/**
 * @brief Operating mode for UART TX/RX operations
 *
 * This enum defines the various modes of operation for UART transmit and receive.
 */
enum class UartOperationMode : std::uint8_t {
    Polling,   ///< Blocking polling mode
    Interrupt, ///< Interrupt-driven mode
    DMA        ///< DMA-driven mode
};

/**
 * @brief STM32-specific UART implementation of the HAL UARTPort interface
 *
 * This class provides a concrete implementation of the HAL UARTPort interface
 * for STM32 microcontrollers. It supports both polling and interrupt-driven modes,
 * as well as DMA for receive operations.
 *
 * This implementation follows Design Decision 30 by using direct register access
 * rather than relying on a HAL library.
 */
class STM32UART : public hal::UARTPort {
  public:
    /**
     * @brief Construct a new STM32UART object
     *
     * @param uart_index The UART peripheral to use
     * @param baud_rate The baud rate setting
     * @param data_size The data size (8-bit or 9-bit)
     * @param tx_mode The transmit operation mode
     * @param rx_mode The receive operation mode
     * @param tx_pin The TX pin (defaults to the standard pin for this UART)
     * @param rx_pin The RX pin (defaults to the standard pin for this UART)
     */
    explicit STM32UART(
        device::UARTIndex uart_index,
        UARTBaudRate baud_rate = UARTBaudRate::BR_115200,
        hal::UartDataSize data_size = hal::UartDataSize::DataSize8b,
        UartOperationMode tx_mode = UartOperationMode::Polling,
        UartOperationMode rx_mode = UartOperationMode::Interrupt,
        const device::GPIOPin& tx_pin = device::GPIOPin(device::GPIOPort::PortC, 12),
        const device::GPIOPin& rx_pin = device::GPIOPin(device::GPIOPort::PortD, 2));

    /**
     * @brief Destroy the STM32UART object
     *
     * Cleans up resources and disables the UART peripheral.
     */
    ~STM32UART() override;

    // Disable copying and moving
    STM32UART(const STM32UART&) = delete;
    STM32UART& operator=(const STM32UART&) = delete;
    STM32UART(STM32UART&&) = delete;
    STM32UART& operator=(STM32UART&&) = delete;

    /**
     * @brief Get the data size configuration
     *
     * @return hal::UartDataSize The current data size setting
     */
    hal::UartDataSize data_size() const override;

    /**
     * @brief Transmit 8-bit data over UART
     *
     * @param data The data to transmit
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void transmit(
        const hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override;

    /**
     * @brief Transmit 9-bit data over UART
     *
     * @param data The data to transmit
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void transmit(
        const hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override;

    /**
     * @brief Receive 8-bit data from UART
     *
     * @param data Output parameter for received data
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void receive(
        hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override;

    /**
     * @brief Receive 9-bit data from UART
     *
     * @param data Output parameter for received data
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void receive(
        hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override;

    /**
     * @brief Get the UART peripheral pointer
     *
     * @return USART_TypeDef* Pointer to the UART peripheral
     */
    USART_TypeDef* get_peripheral() const { return peripheral_; }

    /**
     * @brief Get the UART index
     *
     * @return device::UARTIndex The UART index
     */
    device::UARTIndex get_uart_index() const { return uart_index_; }

    /**
     * @brief Get the IRQ number for this UART
     *
     * @return device::IRQn_Type The IRQ number
     */
    device::IRQn_Type get_irq() const { return device::get_uart_irq(uart_index_); }

    /**
     * @brief Handle UART interrupt
     *
     * This method is called from the UART IRQ handler to process UART interrupts.
     * It handles transmission and reception events in interrupt mode.
     */
    void handle_irq();

    /**
     * @brief Handle DMA receive interrupt
     *
     * This method is called from the DMA IRQ handler to process DMA interrupts.
     * It handles DMA reception events.
     */
    void handle_dma_rx_irq();

  private:
    // Private methods

    /**
     * @brief Initialize the UART peripheral
     *
     * @param baud_rate The baud rate setting
     * @param data_size The data size (8-bit or 9-bit)
     * @param tx_pin The TX pin
     * @param rx_pin The RX pin
     */
    void init(
        UARTBaudRate baud_rate,
        hal::UartDataSize data_size,
        const device::GPIOPin& tx_pin,
        const device::GPIOPin& rx_pin);

    /**
     * @brief Configure the UART for the specified data size
     *
     * @param data_size The data size (8-bit or 9-bit)
     */
    void configure_data_size(hal::UartDataSize data_size);

    /**
     * @brief Set the UART baud rate
     *
     * @param baud_rate The baud rate setting
     */
    void set_baud_rate(UARTBaudRate baud_rate);

    /**
     * @brief Enable UART interrupts
     *
     * @param rx_interrupt Enable RX interrupt
     * @param tx_interrupt Enable TX interrupt
     * @param error_interrupt Enable error interrupt
     */
    void enable_interrupts(bool rx_interrupt, bool tx_interrupt, bool error_interrupt);

    /**
     * @brief Disable UART interrupts
     */
    void disable_interrupts();

    /**
     * @brief Enable NVIC for UART
     *
     * @param priority The interrupt priority
     */
    void enable_uart_nvic(std::uint8_t priority = 5);

    /**
     * @brief Disable NVIC for UART
     */
    void disable_uart_nvic();

    /**
     * @brief Transmit data using polling mode
     *
     * @param data Pointer to the data buffer
     * @param size Size of the data in bytes
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void
    transmit_polling(const void* data, std::size_t size, hal::UartStatus& status, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Transmit data using interrupt mode
     *
     * @param data Pointer to the data buffer
     * @param size Size of the data in bytes
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void transmit_interrupt(
        const void* data,
        std::size_t size,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout_ms);

    /**
     * @brief Receive data using polling mode
     *
     * @param data Pointer to the data buffer
     * @param size Size of the data buffer in bytes
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void receive_polling(void* data, std::size_t size, hal::UartStatus& status, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Receive data using interrupt mode
     *
     * @param data Pointer to the data buffer
     * @param size Size of the data buffer in bytes
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void receive_interrupt(void* data, std::size_t size, hal::UartStatus& status, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Receive data using DMA mode
     *
     * @param data Pointer to the data buffer
     * @param size Size of the data buffer in bytes
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     */
    void receive_dma(void* data, std::size_t size, hal::UartStatus& status, std::chrono::milliseconds timeout_ms);

    /**
     * @brief Initialize DMA for receive operation
     */
    void init_dma_rx();

    /**
     * @brief Enable DMA for receive operation
     *
     * @param buffer Pointer to the receive buffer
     * @param size Size of the buffer in bytes
     */
    void enable_dma_rx(void* buffer, std::size_t size);

    /**
     * @brief Disable DMA for receive operation
     */
    void disable_dma_rx();

    /**
     * @brief Convert sync::UartStatus to hal::UartStatus
     *
     * @param status The sync::UartStatus to convert
     * @return hal::UartStatus The corresponding hal::UartStatus
     */
    static hal::UartStatus convert_status(sync::UartStatus status);

    // Private member variables

    USART_TypeDef* const peripheral_;    ///< Pointer to UART peripheral
    const device::UARTIndex uart_index_; ///< UART peripheral index
    hal::UartDataSize data_size_;        ///< Current data size setting
    UartOperationMode tx_mode_;          ///< Transmit operation mode
    UartOperationMode rx_mode_;          ///< Receive operation mode
    device::GPIOPin tx_pin_;             ///< TX pin
    device::GPIOPin rx_pin_;             ///< RX pin

    // Synchronization primitives
    sync::BinarySemaphore tx_semaphore_; ///< Transmit semaphore
    sync::BinarySemaphore rx_semaphore_; ///< Receive semaphore
    sync::Watchdog tx_watchdog_;         ///< Transmit watchdog
    sync::Watchdog rx_watchdog_;         ///< Receive watchdog

    // Interrupt mode state
    std::atomic<const void*> tx_buffer_{nullptr}; ///< Transmit buffer
    std::atomic<std::size_t> tx_size_{0};         ///< Transmit size
    std::atomic<std::size_t> tx_count_{0};        ///< Transmit count
    std::atomic<void*> rx_buffer_{nullptr};       ///< Receive buffer
    std::atomic<std::size_t> rx_size_{0};         ///< Receive size
    std::atomic<std::size_t> rx_count_{0};        ///< Receive count

    // DMA state
    bool dma_initialized_{false};                                   ///< DMA initialized flag
    DMA_Stream_TypeDef* dma_stream_{nullptr};                       ///< DMA stream for RX
    std::uint32_t dma_channel_{0};                                  ///< DMA channel for RX
    device::IRQn_Type dma_irq_{static_cast<device::IRQn_Type>(-1)}; ///< DMA IRQ number

    // DMA buffer with 32-byte alignment
    static constexpr std::size_t DMA_BUFFER_SIZE = 256;                    ///< DMA buffer size
    platform::DmaAlignedBuffer<std::uint8_t, DMA_BUFFER_SIZE> dma_buffer_; ///< DMA buffer
};

// Singleton instance lookup function
/**
 * @brief Get the STM32UART instance for the specified UART
 *
 * This function returns a shared pointer to the STM32UART instance for the
 * specified UART peripheral. If the instance doesn't exist, it creates one.
 * This allows UART IRQ handlers to find the corresponding C++ object.
 *
 * @param uart_index The UART peripheral index
 * @return std::shared_ptr<STM32UART> Shared pointer to the UART instance
 */
std::shared_ptr<STM32UART> get_uart_instance(device::UARTIndex uart_index);

/**
 * @brief Register a STM32UART instance for IRQ handling
 *
 * This function registers a STM32UART instance for IRQ handling. It's called
 * by the STM32UART constructor to make the instance available to IRQ handlers.
 *
 * @param uart_index The UART peripheral index
 * @param uart Pointer to the STM32UART instance
 */
void register_uart_instance(device::UARTIndex uart_index, STM32UART* uart);

/**
 * @brief Unregister a STM32UART instance
 *
 * This function unregisters a STM32UART instance. It's called by the STM32UART
 * destructor to remove the instance from the registry.
 *
 * @param uart_index The UART peripheral index
 */
void unregister_uart_instance(device::UARTIndex uart_index);

/**
 * @brief UART IRQ handler function
 *
 * This function is called from the UART IRQ handler to process UART interrupts.
 * It delegates to the appropriate STM32UART instance.
 *
 * @param uart_index The UART peripheral index
 */
void handle_uart_irq(device::UARTIndex uart_index);

/**
 * @brief DMA RX IRQ handler function
 *
 * This function is called from the DMA IRQ handler to process DMA interrupts.
 * It delegates to the appropriate STM32UART instance.
 *
 * @param uart_index The UART peripheral index
 */
void handle_dma_rx_irq(device::UARTIndex uart_index);

} // namespace hm11::platform::stm32

#endif // HM11_PLATFORM_STM32_UART_HPP