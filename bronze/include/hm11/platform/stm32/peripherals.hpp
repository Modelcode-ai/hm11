/**
 * @file peripherals.hpp
 * @brief STM32 peripheral type definitions
 *
 * This file contains base peripheral type definitions for STM32F4xx microcontrollers,
 * including UART, DMA, and GPIO peripherals. These types provide a C++ wrapper
 * around the CMSIS peripheral structures with type-safe methods for configuration
 * and control.
 *
 * @note This implementation follows Design Decision 30 by using direct register
 * access via CMSIS headers without the STM32 HAL library dependency.
 */

#ifndef HM11_PLATFORM_STM32_PERIPHERALS_HPP
#define HM11_PLATFORM_STM32_PERIPHERALS_HPP

#include <cstddef>
#include <cstdint>

#include "device.hpp"

namespace hm11 {
namespace platform {
namespace stm32 {

/**
 * @brief Status codes for STM32 peripheral operations
 *
 * These status codes match the UART_Status enum from the original Ada codebase.
 */
enum class Status {
    Ok,          ///< Operation completed successfully
    Err_Error,   ///< General error
    Err_Timeout, ///< Operation timed out
    Busy         ///< Peripheral is busy
};

/**
 * @brief Data size for UART peripherals
 */
enum class UARTDataSize {
    Data_Size_8b, ///< 8-bit data
    Data_Size_9b  ///< 9-bit data
};

/**
 * @brief Baud rate settings for UART peripherals
 */
enum class UARTBaudRate {
    BR_1200,   ///< 1200 baud
    BR_2400,   ///< 2400 baud
    BR_4800,   ///< 4800 baud
    BR_9600,   ///< 9600 baud
    BR_19200,  ///< 19200 baud
    BR_38400,  ///< 38400 baud
    BR_57600,  ///< 57600 baud
    BR_115200, ///< 115200 baud
    BR_230400  ///< 230400 baud
};

/**
 * @brief Parity settings for UART peripherals
 */
enum class UARTParity {
    None, ///< No parity
    Even, ///< Even parity
    Odd   ///< Odd parity
};

/**
 * @brief Stop bit settings for UART peripherals
 */
enum class UARTStopBits {
    StopBits_1, ///< 1 stop bit
    StopBits_2  ///< 2 stop bits
};

/**
 * @brief Flow control settings for UART peripherals
 */
enum class UARTFlowControl {
    None,   ///< No flow control
    RTS,    ///< RTS flow control
    CTS,    ///< CTS flow control
    RTS_CTS ///< RTS and CTS flow control
};

/**
 * @brief GPIO pin mode settings
 */
enum class GPIOMode {
    Input,             ///< Input mode
    Output,            ///< Output mode
    AlternateFunction, ///< Alternate function mode
    Analog             ///< Analog mode
};

/**
 * @brief GPIO pin output type settings
 */
enum class GPIOOutputType {
    PushPull, ///< Push-pull output
    OpenDrain ///< Open-drain output
};

/**
 * @brief GPIO pin pull-up/pull-down settings
 */
enum class GPIOPull {
    NoPull,  ///< No pull-up or pull-down
    PullUp,  ///< Pull-up
    PullDown ///< Pull-down
};

/**
 * @brief GPIO pin output speed settings
 */
enum class GPIOSpeed {
    Low,    ///< Low speed
    Medium, ///< Medium speed
    Fast,   ///< Fast speed
    High    ///< High speed
};

/**
 * @brief GPIO pin alternate function selections
 */
enum class GPIOAlternateFunction {
    AF0,  ///< Alternate function 0
    AF1,  ///< Alternate function 1
    AF2,  ///< Alternate function 2
    AF3,  ///< Alternate function 3
    AF4,  ///< Alternate function 4
    AF5,  ///< Alternate function 5
    AF6,  ///< Alternate function 6
    AF7,  ///< Alternate function 7
    AF8,  ///< Alternate function 8 (UART alternate function)
    AF9,  ///< Alternate function 9
    AF10, ///< Alternate function 10
    AF11, ///< Alternate function 11
    AF12, ///< Alternate function 12
    AF13, ///< Alternate function 13
    AF14, ///< Alternate function 14
    AF15  ///< Alternate function 15
};

/**
 * @brief DMA data transfer direction
 */
enum class DMADirection {
    PeripheralToMemory, ///< Peripheral to memory
    MemoryToPeripheral, ///< Memory to peripheral
    MemoryToMemory      ///< Memory to memory
};

/**
 * @brief DMA data size
 */
enum class DMADataSize {
    Byte,     ///< Byte (8-bit)
    HalfWord, ///< Half-word (16-bit)
    Word      ///< Word (32-bit)
};

/**
 * @brief DMA priority level
 */
enum class DMAPriority {
    Low,     ///< Low priority
    Medium,  ///< Medium priority
    High,    ///< High priority
    VeryHigh ///< Very high priority
};

/**
 * @brief DMA burst transfer configuration
 */
enum class DMABurstLength {
    Single, ///< Single transfer
    Incr4,  ///< Incremental burst of 4 beats
    Incr8,  ///< Incremental burst of 8 beats
    Incr16  ///< Incremental burst of 16 beats
};

/**
 * @brief UART peripheral class for STM32
 *
 * This class provides an interface to configure and control UART peripherals
 * on STM32 microcontrollers using direct register access.
 */
class UARTPort {
  public:
    /**
     * @brief Construct a new UART Port object
     *
     * @param uart The UART peripheral index
     */
    explicit UARTPort(device::UARTIndex uart);

    /**
     * @brief Initialize the UART peripheral
     *
     * @param baud The baud rate
     * @param data_size The data size
     * @param parity The parity setting
     * @param stop_bits The stop bits setting
     * @param flow_control The flow control setting
     */
    void init(
        UARTBaudRate baud = UARTBaudRate::BR_115200,
        UARTDataSize data_size = UARTDataSize::Data_Size_8b,
        UARTParity parity = UARTParity::None,
        UARTStopBits stop_bits = UARTStopBits::StopBits_1,
        UARTFlowControl flow_control = UARTFlowControl::None);

    /**
     * @brief Get the UART data size
     *
     * @return UARTDataSize The current data size setting
     */
    UARTDataSize data_size() const;

    /**
     * @brief Transmit data over UART
     *
     * @param data Pointer to data buffer
     * @param size Size of the data buffer
     * @param status Status output parameter
     * @param timeout Timeout in milliseconds
     */
    void transmit(const uint8_t* data, size_t size, Status& status, uint32_t timeout = 1000);

    /**
     * @brief Receive data over UART
     *
     * @param data Pointer to data buffer
     * @param size Size of the data buffer
     * @param status Status output parameter
     * @param timeout Timeout in milliseconds
     */
    void receive(uint8_t* data, size_t size, Status& status, uint32_t timeout = 1000);

    /**
     * @brief Enable UART interrupts
     *
     * @param rx_interrupt Enable RX interrupt
     * @param tx_interrupt Enable TX interrupt
     * @param error_interrupt Enable error interrupt
     */
    void enable_interrupts(bool rx_interrupt = true, bool tx_interrupt = false, bool error_interrupt = false);

    /**
     * @brief Disable UART interrupts
     */
    void disable_interrupts();

    /**
     * @brief Get the UART peripheral register pointer
     *
     * @return USART_TypeDef* Pointer to UART peripheral registers
     */
    USART_TypeDef* peripheral() const;

    /**
     * @brief Get the IRQ number for this UART
     *
     * @return device::IRQn_Type The IRQ number
     */
    device::IRQn_Type irq() const;

  private:
    USART_TypeDef* const peripheral_; ///< Pointer to UART peripheral registers
    const device::UARTIndex uart_;    ///< UART peripheral index
    UARTDataSize data_size_;          ///< Current data size setting
};

/**
 * @brief DMA stream class for STM32
 *
 * This class provides an interface to configure and control DMA streams
 * on STM32 microcontrollers using direct register access.
 */
class DMAStream {
  public:
    /**
     * @brief Construct a new DMA Stream object
     *
     * @param dma The DMA controller index
     * @param stream The DMA stream
     * @param channel The DMA channel
     */
    DMAStream(device::DMAIndex dma, device::DMAStream stream, device::DMAChannel channel);

    /**
     * @brief Initialize the DMA stream for UART RX
     *
     * @param uart The UART port to use with this DMA stream
     * @param buffer The buffer to receive data into
     * @param size The size of the buffer
     */
    void init_for_uart_rx(const UARTPort& uart, uint8_t* buffer, size_t size);

    /**
     * @brief Initialize the DMA stream for UART TX
     *
     * @param uart The UART port to use with this DMA stream
     * @param buffer The buffer containing data to send
     * @param size The size of the buffer
     */
    void init_for_uart_tx(const UARTPort& uart, const uint8_t* buffer, size_t size);

    /**
     * @brief Start the DMA transfer
     */
    void start();

    /**
     * @brief Stop the DMA transfer
     */
    void stop();

    /**
     * @brief Check if DMA transfer is complete
     *
     * @return true if transfer is complete
     * @return false if transfer is ongoing
     */
    bool is_transfer_complete() const;

    /**
     * @brief Check if DMA transfer has an error
     *
     * @return true if error occurred
     * @return false if no error
     */
    bool has_error() const;

    /**
     * @brief Get the number of remaining data items to transfer
     *
     * @return size_t The number of remaining items
     */
    size_t remaining_transfers() const;

    /**
     * @brief Enable DMA interrupts
     *
     * @param transfer_complete_interrupt Enable transfer complete interrupt
     * @param half_transfer_interrupt Enable half transfer interrupt
     * @param error_interrupt Enable error interrupt
     */
    void enable_interrupts(
        bool transfer_complete_interrupt = true,
        bool half_transfer_interrupt = false,
        bool error_interrupt = true);

    /**
     * @brief Disable DMA interrupts
     */
    void disable_interrupts();

    /**
     * @brief Get the DMA stream peripheral register pointer
     *
     * @return DMA_Stream_TypeDef* Pointer to DMA stream peripheral registers
     */
    DMA_Stream_TypeDef* peripheral() const;

    /**
     * @brief Get the IRQ number for this DMA stream
     *
     * @return device::IRQn_Type The IRQ number
     */
    device::IRQn_Type irq() const;

  private:
    DMA_Stream_TypeDef* const peripheral_; ///< Pointer to DMA stream peripheral registers
    const device::DMAIndex dma_;           ///< DMA controller index
    const device::DMAStream stream_;       ///< DMA stream
    const device::DMAChannel channel_;     ///< DMA channel
};

/**
 * @brief GPIO pin class for STM32
 *
 * This class provides an interface to configure and control GPIO pins
 * on STM32 microcontrollers using direct register access.
 */
class GPIOPin {
  public:
    /**
     * @brief Construct a new GPIO Pin object
     *
     * @param pin The GPIO pin specification
     */
    explicit GPIOPin(const device::GPIOPin& pin);

    /**
     * @brief Initialize the GPIO pin
     *
     * @param mode The GPIO mode
     * @param pull The pull-up/pull-down setting
     * @param speed The output speed setting
     * @param output_type The output type setting
     */
    void init(
        GPIOMode mode = GPIOMode::Input,
        GPIOPull pull = GPIOPull::NoPull,
        GPIOSpeed speed = GPIOSpeed::High,
        GPIOOutputType output_type = GPIOOutputType::PushPull);

    /**
     * @brief Configure the GPIO pin for alternate function
     *
     * @param af The alternate function to use
     * @param output_type The output type setting
     * @param pull The pull-up/pull-down setting
     * @param speed The output speed setting
     */
    void set_alternate_function(
        GPIOAlternateFunction af,
        GPIOOutputType output_type = GPIOOutputType::PushPull,
        GPIOPull pull = GPIOPull::NoPull,
        GPIOSpeed speed = GPIOSpeed::High);

    /**
     * @brief Set the output state of the pin
     *
     * @param state true for high, false for low
     */
    void set(bool state);

    /**
     * @brief Get the input state of the pin
     *
     * @return true if high
     * @return false if low
     */
    bool get() const;

    /**
     * @brief Toggle the output state of the pin
     */
    void toggle();

    /**
     * @brief Get the GPIO peripheral register pointer
     *
     * @return GPIO_TypeDef* Pointer to GPIO peripheral registers
     */
    GPIO_TypeDef* peripheral() const;

    /**
     * @brief Get the pin number
     *
     * @return uint8_t The pin number (0-15)
     */
    uint8_t pin_number() const;

  private:
    GPIO_TypeDef* const peripheral_; ///< Pointer to GPIO peripheral registers
    const uint8_t pin_;              ///< Pin number (0-15)
    const device::GPIOPort port_;    ///< GPIO port
};

} // namespace stm32
} // namespace platform
} // namespace hm11

#endif // HM11_PLATFORM_STM32_PERIPHERALS_HPP