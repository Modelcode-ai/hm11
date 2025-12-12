/**
 * @file uart.cpp
 * @brief STM32 UART implementation for HM11 driver
 *
 * This file provides the implementation of the STM32 UART class, which is a concrete
 * implementation of the HAL UARTPort interface for STM32 microcontrollers. It handles
 * UART configuration, transmission, reception, and interrupt handling using direct
 * register access.
 *
 * This implementation follows Design Decision 30 by using direct register access
 * via CMSIS headers without the STM32 HAL library dependency.
 */

#include "hm11/platform/stm32/uart.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <mutex>
#include <thread>

#include "hm11/platform/stm32/registers.hpp"

namespace hm11::platform::stm32 {

namespace {
// Array of UART instances for IRQ handling
std::array<STM32UART*, 8> uart_instances{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
std::mutex uart_instances_mutex;

// Mapping from UART index to array index
constexpr size_t uart_index_to_array_index(device::UARTIndex uart) {
    return static_cast<size_t>(uart);
}

// Convert UARTBaudRate to actual baud rate value
constexpr uint32_t baud_rate_to_value(UARTBaudRate baud_rate) {
    switch (baud_rate) {
        case UARTBaudRate::BR_1200:
            return 1200;
        case UARTBaudRate::BR_2400:
            return 2400;
        case UARTBaudRate::BR_4800:
            return 4800;
        case UARTBaudRate::BR_9600:
            return 9600;
        case UARTBaudRate::BR_19200:
            return 19200;
        case UARTBaudRate::BR_38400:
            return 38400;
        case UARTBaudRate::BR_57600:
            return 57600;
        case UARTBaudRate::BR_115200:
            return 115200;
        case UARTBaudRate::BR_230400:
            return 230400;
        default:
            return 115200;
    }
}

// Check if UART has error conditions
bool check_uart_errors(USART_TypeDef* peripheral) {
    return reg::is_set(&peripheral->SR, reg::usart::SR_FE | reg::usart::SR_NE | reg::usart::SR_ORE | reg::usart::SR_PE);
}

// Clear UART error flags
void clear_uart_errors(USART_TypeDef* peripheral) {
    // Reading SR followed by DR clears the error flags
    volatile uint32_t sr = peripheral->SR;
    volatile uint32_t dr = peripheral->DR;
    (void)sr;
    (void)dr;
}
} // namespace

// Public methods

STM32UART::STM32UART(
    device::UARTIndex uart_index,
    UARTBaudRate baud_rate,
    hal::UartDataSize data_size,
    UartOperationMode tx_mode,
    UartOperationMode rx_mode,
    const device::GPIOPin& tx_pin,
    const device::GPIOPin& rx_pin)
    : peripheral_(device::get_uart_peripheral(uart_index)), uart_index_(uart_index), data_size_(data_size),
      tx_mode_(tx_mode), rx_mode_(rx_mode), tx_pin_(tx_pin), rx_pin_(rx_pin) {
    // Validate peripheral pointer
    assert(peripheral_ != nullptr && "Invalid UART peripheral");

    // Register this instance for IRQ handling
    register_uart_instance(uart_index_, this);

    // Initialize UART peripheral and GPIO pins
    init(baud_rate, data_size, tx_pin, rx_pin);

    // If DMA mode is selected for RX, initialize DMA
    if (rx_mode_ == UartOperationMode::DMA) {
        init_dma_rx();
    }
}

STM32UART::~STM32UART() {
    // Disable UART interrupts
    disable_interrupts();
    disable_uart_nvic();

    // If DMA mode is used, disable DMA
    if (dma_initialized_) {
        disable_dma_rx();
    }

    // Disable the UART peripheral
    reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_UE);

    // Unregister this instance from the registry
    unregister_uart_instance(uart_index_);
}

hal::UartDataSize STM32UART::data_size() const {
    return data_size_;
}

void STM32UART::transmit(const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds timeout_ms) {
    // Check data size compatibility
    if (data_size_ != hal::UartDataSize::DataSize8b) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Choose the appropriate transmit method based on the mode
    if (tx_mode_ == UartOperationMode::Polling) {
        transmit_polling(data.data(), data.size(), status, timeout_ms);
    } else if (tx_mode_ == UartOperationMode::Interrupt) {
        transmit_interrupt(data.data(), data.size(), status, timeout_ms);
    } else {
        // DMA transmit not implemented
        status = hal::UartStatus::ErrError;
    }
}

void STM32UART::transmit(const hal::UartData9b& data, hal::UartStatus& status, std::chrono::milliseconds timeout_ms) {
    // Check data size compatibility
    if (data_size_ != hal::UartDataSize::DataSize9b) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Choose the appropriate transmit method based on the mode
    if (tx_mode_ == UartOperationMode::Polling) {
        transmit_polling(data.data(), data.size() * sizeof(hal::UInt16), status, timeout_ms);
    } else if (tx_mode_ == UartOperationMode::Interrupt) {
        transmit_interrupt(data.data(), data.size() * sizeof(hal::UInt16), status, timeout_ms);
    } else {
        // DMA transmit not implemented
        status = hal::UartStatus::ErrError;
    }
}

void STM32UART::receive(hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds timeout_ms) {
    // Check data size compatibility
    if (data_size_ != hal::UartDataSize::DataSize8b) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Choose the appropriate receive method based on the mode
    if (rx_mode_ == UartOperationMode::Polling) {
        receive_polling(data.data(), data.size(), status, timeout_ms);
    } else if (rx_mode_ == UartOperationMode::Interrupt) {
        receive_interrupt(data.data(), data.size(), status, timeout_ms);
    } else if (rx_mode_ == UartOperationMode::DMA) {
        receive_dma(data.data(), data.size(), status, timeout_ms);
    } else {
        // Should never happen
        status = hal::UartStatus::ErrError;
    }
}

void STM32UART::receive(hal::UartData9b& data, hal::UartStatus& status, std::chrono::milliseconds timeout_ms) {
    // Check data size compatibility
    if (data_size_ != hal::UartDataSize::DataSize9b) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Choose the appropriate receive method based on the mode
    if (rx_mode_ == UartOperationMode::Polling) {
        receive_polling(data.data(), data.size() * sizeof(hal::UInt16), status, timeout_ms);
    } else if (rx_mode_ == UartOperationMode::Interrupt) {
        receive_interrupt(data.data(), data.size() * sizeof(hal::UInt16), status, timeout_ms);
    } else if (rx_mode_ == UartOperationMode::DMA) {
        receive_dma(data.data(), data.size() * sizeof(hal::UInt16), status, timeout_ms);
    } else {
        // Should never happen
        status = hal::UartStatus::ErrError;
    }
}

void STM32UART::transmit_polling(
    const void* data,
    std::size_t size,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms) {
    // Start the watchdog with the provided timeout
    if (!tx_watchdog_.start(const_cast<void*>(data), size, timeout_ms)) {
        status = hal::UartStatus::Busy;
        return;
    }

    const uint8_t* buf = static_cast<const uint8_t*>(data);
    std::size_t count = 0;
    sync::UartStatus uart_status = sync::UartStatus::Ok;

    // Check for UART errors before starting
    if (check_uart_errors(peripheral_)) {
        clear_uart_errors(peripheral_);
        tx_watchdog_.release(sync::UartStatus::ErrError);
        status = hal::UartStatus::ErrError;
        return;
    }

    // Transmit each byte (or word if 9-bit)
    while (count < size) {
        // Check for timeout
        if (tx_watchdog_.is_deadline_exceeded()) {
            tx_watchdog_.release(sync::UartStatus::ErrTimeout);
            status = hal::UartStatus::ErrTimeout;
            return;
        }

        // Check for UART errors
        if (check_uart_errors(peripheral_)) {
            clear_uart_errors(peripheral_);
            tx_watchdog_.release(sync::UartStatus::ErrError);
            status = hal::UartStatus::ErrError;
            return;
        }

        // Wait for TXE flag (Transmit data register empty)
        if (reg::is_set(&peripheral_->SR, reg::usart::SR_TXE)) {
            if (data_size_ == hal::UartDataSize::DataSize8b) {
                // Transmit 8-bit data
                peripheral_->DR = buf[count];
                count++;
            } else {
                // Transmit 9-bit data (if we're at an even position)
                if (count + 1 < size) {
                    // Combine two bytes into a 9-bit value (taking only 9 bits)
                    const hal::UInt16* word_buf = static_cast<const hal::UInt16*>(data);
                    peripheral_->DR = word_buf[count / sizeof(hal::UInt16)] & 0x1FF;
                    count += sizeof(hal::UInt16);
                } else {
                    // Incomplete word at the end
                    tx_watchdog_.release(sync::UartStatus::ErrError);
                    status = hal::UartStatus::ErrError;
                    return;
                }
            }
        }
    }

    // Wait for transmission complete flag
    while (!reg::is_set(&peripheral_->SR, reg::usart::SR_TC)) {
        // Check for timeout
        if (tx_watchdog_.is_deadline_exceeded()) {
            tx_watchdog_.release(sync::UartStatus::ErrTimeout);
            status = hal::UartStatus::ErrTimeout;
            return;
        }
    }

    // Transmission complete
    tx_watchdog_.release(sync::UartStatus::Ok);
    status = hal::UartStatus::Ok;
}

void STM32UART::receive_polling(
    void* data,
    std::size_t size,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms) {
    // Start the watchdog with the provided timeout
    if (!rx_watchdog_.start(data, size, timeout_ms)) {
        status = hal::UartStatus::Busy;
        return;
    }

    uint8_t* buf = static_cast<uint8_t*>(data);
    std::size_t count = 0;
    sync::UartStatus uart_status = sync::UartStatus::Ok;

    // Receive each byte (or word if 9-bit)
    while (count < size) {
        // Check for timeout
        if (rx_watchdog_.is_deadline_exceeded()) {
            rx_watchdog_.release(sync::UartStatus::ErrTimeout);
            status = hal::UartStatus::ErrTimeout;
            return;
        }

        // Check for UART errors
        if (check_uart_errors(peripheral_)) {
            clear_uart_errors(peripheral_);
            rx_watchdog_.release(sync::UartStatus::ErrError);
            status = hal::UartStatus::ErrError;
            return;
        }

        // Check if data is available in the receive register
        if (reg::is_set(&peripheral_->SR, reg::usart::SR_RXNE)) {
            if (data_size_ == hal::UartDataSize::DataSize8b) {
                // Receive 8-bit data
                buf[count] = static_cast<uint8_t>(peripheral_->DR & 0xFF);
                count++;
            } else {
                // Receive 9-bit data
                if (count + 1 <= size) {
                    // Store 9-bit value
                    hal::UInt16* word_buf = static_cast<hal::UInt16*>(data);
                    word_buf[count / sizeof(hal::UInt16)] = static_cast<hal::UInt16>(peripheral_->DR & 0x1FF);
                    count += sizeof(hal::UInt16);
                } else {
                    // Incomplete buffer
                    rx_watchdog_.release(sync::UartStatus::ErrError);
                    status = hal::UartStatus::ErrError;
                    return;
                }
            }
        }
    }

    // Reception complete
    rx_watchdog_.release(sync::UartStatus::Ok);
    status = hal::UartStatus::Ok;
}

void STM32UART::transmit_interrupt(
    const void* data,
    std::size_t size,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms) {
    // Start the watchdog with the provided timeout
    if (!tx_watchdog_.start(const_cast<void*>(data), size, timeout_ms)) {
        status = hal::UartStatus::Busy;
        return;
    }

    // Check for UART errors before starting
    if (check_uart_errors(peripheral_)) {
        clear_uart_errors(peripheral_);
        tx_watchdog_.release(sync::UartStatus::ErrError);
        status = hal::UartStatus::ErrError;
        return;
    }

    // Set up the transmit operation
    tx_buffer_ = data;
    tx_size_ = size;
    tx_count_ = 0;

    // Enable transmit interrupt
    reg::set_bits(&peripheral_->CR1, reg::usart::CR1_TXEIE);

    // Wait for the operation to complete or timeout
    sync::UartStatus uart_status;
    const bool completed = tx_watchdog_.await_event(uart_status);

    // Convert the UART status to HAL status
    status = convert_status(uart_status);
}

void STM32UART::receive_interrupt(
    void* data,
    std::size_t size,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms) {
    // Start the watchdog with the provided timeout
    if (!rx_watchdog_.start(data, size, timeout_ms)) {
        status = hal::UartStatus::Busy;
        return;
    }

    // Check for UART errors before starting
    if (check_uart_errors(peripheral_)) {
        clear_uart_errors(peripheral_);
        rx_watchdog_.release(sync::UartStatus::ErrError);
        status = hal::UartStatus::ErrError;
        return;
    }

    // Set up the receive operation
    rx_buffer_ = data;
    rx_size_ = size;
    rx_count_ = 0;

    // Enable receive interrupt
    reg::set_bits(&peripheral_->CR1, reg::usart::CR1_RXNEIE);

    // Wait for the operation to complete or timeout
    sync::UartStatus uart_status;
    const bool completed = rx_watchdog_.await_event(uart_status);

    // Convert the UART status to HAL status
    status = convert_status(uart_status);
}

void STM32UART::handle_irq() {
    // Get the status register
    const uint32_t sr = peripheral_->SR;

    // Check for errors
    if (sr & (reg::usart::SR_FE | reg::usart::SR_NE | reg::usart::SR_ORE | reg::usart::SR_PE)) {
        // Clear the error flags
        clear_uart_errors(peripheral_);

        // If there's an active RX operation, signal error
        if (rx_buffer_ != nullptr && rx_count_ < rx_size_) {
            rx_watchdog_.release(sync::UartStatus::ErrError);
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_RXNEIE);
            rx_buffer_ = nullptr;
        }

        // If there's an active TX operation, signal error
        if (tx_buffer_ != nullptr && tx_count_ < tx_size_) {
            tx_watchdog_.release(sync::UartStatus::ErrError);
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_TXEIE);
            tx_buffer_ = nullptr;
        }

        return;
    }

    // Handle RX (receiver not empty)
    if ((sr & reg::usart::SR_RXNE) && (peripheral_->CR1 & reg::usart::CR1_RXNEIE)) {
        if (rx_buffer_ != nullptr && rx_count_ < rx_size_) {
            // Read the data
            if (data_size_ == hal::UartDataSize::DataSize8b) {
                // 8-bit data
                uint8_t* buf = static_cast<uint8_t*>(rx_buffer_);
                buf[rx_count_] = static_cast<uint8_t>(peripheral_->DR & 0xFF);
                rx_count_++;
            } else {
                // 9-bit data
                hal::UInt16* buf = static_cast<hal::UInt16*>(rx_buffer_);
                const std::size_t index = rx_count_ / sizeof(hal::UInt16);
                buf[index] = static_cast<hal::UInt16>(peripheral_->DR & 0x1FF);
                rx_count_ += sizeof(hal::UInt16);
            }

            // Check if we've received all data
            if (rx_count_ >= rx_size_) {
                // Disable the RX interrupt
                reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_RXNEIE);

                // Signal completion
                rx_watchdog_.release(sync::UartStatus::Ok);
                rx_buffer_ = nullptr;
            }
        } else {
            // No active RX operation, disable interrupt
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_RXNEIE);
        }
    }

    // Handle TX (transmitter empty)
    if ((sr & reg::usart::SR_TXE) && (peripheral_->CR1 & reg::usart::CR1_TXEIE)) {
        if (tx_buffer_ != nullptr && tx_count_ < tx_size_) {
            // Write the data
            if (data_size_ == hal::UartDataSize::DataSize8b) {
                // 8-bit data
                const uint8_t* buf = static_cast<const uint8_t*>(tx_buffer_);
                peripheral_->DR = buf[tx_count_];
                tx_count_++;
            } else {
                // 9-bit data
                const hal::UInt16* buf = static_cast<const hal::UInt16*>(tx_buffer_);
                const std::size_t index = tx_count_ / sizeof(hal::UInt16);
                peripheral_->DR = buf[index] & 0x1FF;
                tx_count_ += sizeof(hal::UInt16);
            }

            // Check if we've transmitted all data
            if (tx_count_ >= tx_size_) {
                // Disable the TX interrupt
                reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_TXEIE);

                // Enable the TC interrupt to wait for transmission completion
                reg::set_bits(&peripheral_->CR1, reg::usart::CR1_TCIE);
            }
        } else {
            // No active TX operation, disable interrupt
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_TXEIE);
        }
    }

    // Handle TC (transmission complete)
    if ((sr & reg::usart::SR_TC) && (peripheral_->CR1 & reg::usart::CR1_TCIE)) {
        // Disable the TC interrupt
        reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_TCIE);

        // Signal completion
        if (tx_buffer_ != nullptr) {
            tx_watchdog_.release(sync::UartStatus::Ok);
            tx_buffer_ = nullptr;
        }
    }

    // Handle IDLE (idle line detected)
    if ((sr & reg::usart::SR_IDLE) && (peripheral_->CR1 & reg::usart::CR1_IDLEIE)) {
        // Clear the IDLE flag by reading SR then DR
        volatile uint32_t tmp = peripheral_->SR;
        tmp = peripheral_->DR;
        (void)tmp;

        // If there's an active RX operation, check if this is a timeout
        if (rx_watchdog_.is_deadline_exceeded()) {
            rx_watchdog_.release(sync::UartStatus::ErrTimeout);
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_RXNEIE);
            rx_buffer_ = nullptr;
        }

        // If there's an active TX operation, check if this is a timeout
        if (tx_watchdog_.is_deadline_exceeded()) {
            tx_watchdog_.release(sync::UartStatus::ErrTimeout);
            reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_TXEIE | reg::usart::CR1_TCIE);
            tx_buffer_ = nullptr;
        }
    }
}

void STM32UART::receive_dma(
    void* data,
    std::size_t size,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms) {
    // Initialize DMA if not already done
    if (!dma_initialized_) {
        init_dma_rx();
    }

    // Start the watchdog with the provided timeout
    if (!rx_watchdog_.start(data, size, timeout_ms)) {
        status = hal::UartStatus::Busy;
        return;
    }

    // Check for UART errors before starting
    if (check_uart_errors(peripheral_)) {
        clear_uart_errors(peripheral_);
        rx_watchdog_.release(sync::UartStatus::ErrError);
        status = hal::UartStatus::ErrError;
        return;
    }

    // Ensure the DMA is stopped
    disable_dma_rx();

    // Initialize the buffer
    rx_buffer_ = data;
    rx_size_ = size;
    rx_count_ = 0;

    // Configure the DMA transfer
    const std::size_t transfer_size = std::min(size, DMA_BUFFER_SIZE);

    // For 8-bit data, we transfer directly to the user buffer
    // For 9-bit data, we transfer to the DMA buffer and then copy to the user buffer
    void* dma_dest = nullptr;

    if (data_size_ == hal::UartDataSize::DataSize8b) {
        dma_dest = data;
    } else {
        // Use the DMA buffer for 9-bit data
        dma_dest = dma_buffer_.data();
    }

    // Enable DMA for receive
    enable_dma_rx(dma_dest, transfer_size);

    // Wait for the operation to complete or timeout
    sync::UartStatus uart_status;
    const bool completed = rx_watchdog_.await_event(uart_status);

    // If we were using the DMA buffer, copy the data to the user buffer
    if (data_size_ == hal::UartDataSize::DataSize9b && completed && uart_status == sync::UartStatus::Ok) {
        // Copy from DMA buffer to user buffer with proper conversion to 9-bit data
        hal::UInt16* dest = static_cast<hal::UInt16*>(data);
        const uint8_t* src = dma_buffer_.data();

        for (size_t i = 0; i < rx_count_; ++i) {
            dest[i] = static_cast<hal::UInt16>(src[i]) & 0x1FF;
        }
    }

    // Convert the UART status to HAL status
    status = convert_status(uart_status);
}

void STM32UART::init_dma_rx() {
    if (dma_initialized_) {
        return;
    }

    // For UART5, we'll use DMA1 Stream 0, Channel 4 as defined in the device.hpp file
    const auto dma = device::UART5_DMA;
    const auto stream = device::UART5_RX_Stream;
    const auto channel = device::UART5_RX_Channel;

    // Enable DMA clock
    device::enable_dma_clock(dma);

    // Get the DMA stream
    dma_stream_ = device::get_dma_stream_peripheral(dma, stream);
    dma_channel_ = static_cast<uint32_t>(channel) << reg::dma::SxCR_CHSEL_SHIFT;

    // Get the DMA IRQ number
    dma_irq_ = device::get_dma_stream_irq(dma, stream);

    // Configure UART to use DMA for receive
    reg::set_bits(&peripheral_->CR3, reg::usart::CR3_DMAR);

    dma_initialized_ = true;
}

void STM32UART::enable_dma_rx(void* buffer, std::size_t size) {
    if (!dma_initialized_) {
        return;
    }

    // Disable DMA stream first
    reg::clear_bits(&dma_stream_->CR, reg::dma::SxCR_EN);

    // Wait until DMA stream is disabled
    reg::wait_until_clear(&dma_stream_->CR, reg::dma::SxCR_EN);

    // Clear DMA interrupt flags
    if (dma_stream_ == device::DMA1_Stream0_Peripheral) {
        reg::set_bits(
            &DMA1->LIFCR,
            reg::dma::LIFCR_CTCIF0 | reg::dma::LIFCR_CHTIF0 | reg::dma::LIFCR_CTEIF0 | reg::dma::LIFCR_CDMEIF0 |
                reg::dma::LIFCR_CFEIF0);
    } else if (dma_stream_ == device::DMA2_Stream0_Peripheral) {
        reg::set_bits(
            &DMA2->LIFCR,
            reg::dma::LIFCR_CTCIF0 | reg::dma::LIFCR_CHTIF0 | reg::dma::LIFCR_CTEIF0 | reg::dma::LIFCR_CDMEIF0 |
                reg::dma::LIFCR_CFEIF0);
    }

    // Configure DMA stream
    // Direction: Peripheral to memory
    // Memory increment mode: Enabled
    // Peripheral increment mode: Disabled
    // Circular mode: Disabled
    // Priority: High
    uint32_t config = reg::dma::SxCR_DIR_P2M | reg::dma::SxCR_MINC | reg::dma::SxCR_PL_HIGH | dma_channel_;

    // Configure data size based on UART data size
    if (data_size_ == hal::UartDataSize::DataSize8b) {
        config |= reg::dma::SxCR_PSIZE_BYTE | reg::dma::SxCR_MSIZE_BYTE;
    } else {
        config |= reg::dma::SxCR_PSIZE_HALF_WORD | reg::dma::SxCR_MSIZE_HALF_WORD;
    }

    // Write the configuration
    dma_stream_->CR = config;

    // Configure DMA FIFO
    dma_stream_->FCR = reg::dma::SxFCR_DMDIS | reg::dma::SxFCR_FTH_FULL;

    // Set memory address (destination)
    dma_stream_->M0AR = reinterpret_cast<uint32_t>(buffer);

    // Set peripheral address (source)
    dma_stream_->PAR = reinterpret_cast<uint32_t>(&peripheral_->DR);

    // Set number of data items to transfer
    dma_stream_->NDTR = static_cast<uint32_t>(size);

    // Enable DMA interrupts
    dma_stream_->CR |= reg::dma::SxCR_TCIE | reg::dma::SxCR_TEIE;

    // Enable DMA IRQ in NVIC
    const int16_t irq_num = device::to_cmsis_irqn(dma_irq_);
    NVIC_SetPriority(static_cast<IRQn_Type>(irq_num), 5);
    NVIC_EnableIRQ(static_cast<IRQn_Type>(irq_num));

    // Enable DMA stream
    reg::set_bits(&dma_stream_->CR, reg::dma::SxCR_EN);
}

void STM32UART::disable_dma_rx() {
    if (!dma_initialized_) {
        return;
    }

    // Disable DMA IRQ in NVIC
    const int16_t irq_num = device::to_cmsis_irqn(dma_irq_);
    NVIC_DisableIRQ(static_cast<IRQn_Type>(irq_num));

    // Disable DMA stream
    reg::clear_bits(&dma_stream_->CR, reg::dma::SxCR_EN);

    // Wait until DMA stream is disabled
    reg::wait_until_clear(&dma_stream_->CR, reg::dma::SxCR_EN);

    // Disable DMA request in UART
    reg::clear_bits(&peripheral_->CR3, reg::usart::CR3_DMAR);
}

void STM32UART::handle_dma_rx_irq() {
    // Check DMA flags
    bool tc_flag = false;
    bool te_flag = false;

    // For DMA1 Stream 0
    if (dma_stream_ == device::DMA1_Stream0_Peripheral) {
        tc_flag = reg::is_set(&DMA1->LISR, reg::dma::LISR_TCIF0);
        te_flag = reg::is_set(&DMA1->LISR, reg::dma::LISR_TEIF0);

        // Clear flags
        reg::set_bits(&DMA1->LIFCR, reg::dma::LIFCR_CTCIF0 | reg::dma::LIFCR_CTEIF0);
    }
    // For DMA2 Stream 0
    else if (dma_stream_ == device::DMA2_Stream0_Peripheral) {
        tc_flag = reg::is_set(&DMA2->LISR, reg::dma::LISR_TCIF0);
        te_flag = reg::is_set(&DMA2->LISR, reg::dma::LISR_TEIF0);

        // Clear flags
        reg::set_bits(&DMA2->LIFCR, reg::dma::LIFCR_CTCIF0 | reg::dma::LIFCR_CTEIF0);
    }

    // Check UART errors
    const bool uart_error = check_uart_errors(peripheral_);
    if (uart_error) {
        clear_uart_errors(peripheral_);
    }

    // Handle transfer complete
    if (tc_flag) {
        // Get number of bytes transferred
        const std::size_t bytes_transferred = rx_size_ - dma_stream_->NDTR;
        rx_count_ = bytes_transferred;

        // Invalidate cache for DMA buffer if needed
        dma_buffer_.invalidate_cache();

        // Signal completion
        rx_watchdog_.release(sync::UartStatus::Ok);
    }
    // Handle transfer error
    else if (te_flag || uart_error) {
        rx_watchdog_.release(sync::UartStatus::ErrError);
    }

    // Disable DMA stream
    disable_dma_rx();
}

// Private methods

void STM32UART::init(
    UARTBaudRate baud_rate,
    hal::UartDataSize data_size,
    const device::GPIOPin& tx_pin,
    const device::GPIOPin& rx_pin) {
    // Enable UART clock
    device::enable_uart_clock(uart_index_);

    // Configure GPIO pins
    gpio_config::configure_uart_pins(uart_index_, tx_pin, rx_pin);

    // Ensure UART is disabled during configuration
    reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_UE);

    // Configure data size
    configure_data_size(data_size);

    // Configure baud rate
    set_baud_rate(baud_rate);

    // Configure stop bits (1 stop bit)
    reg::modify_bits(&peripheral_->CR2, reg::usart::CR2_STOP_MASK, reg::usart::CR2_STOP_1);

    // Configure no parity
    reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_PCE);

    // Enable TX and RX
    reg::set_bits(&peripheral_->CR1, reg::usart::CR1_TE | reg::usart::CR1_RE);

    // Enable UART
    reg::set_bits(&peripheral_->CR1, reg::usart::CR1_UE);

    // If using interrupt mode, enable appropriate interrupts
    if (tx_mode_ == UartOperationMode::Interrupt || rx_mode_ == UartOperationMode::Interrupt) {
        const bool rx_interrupt = (rx_mode_ == UartOperationMode::Interrupt);
        const bool tx_interrupt = (tx_mode_ == UartOperationMode::Interrupt);
        enable_interrupts(rx_interrupt, tx_interrupt, true);
        enable_uart_nvic();
    }

    // If using DMA for RX, enable UART DMA RX
    if (rx_mode_ == UartOperationMode::DMA) {
        reg::set_bits(&peripheral_->CR3, reg::usart::CR3_DMAR);
    }
}

void STM32UART::configure_data_size(hal::UartDataSize data_size) {
    // Store the data size
    data_size_ = data_size;

    // Configure CR1 M bit: 0 for 8 data bits, 1 for 9 data bits
    if (data_size == hal::UartDataSize::DataSize8b) {
        reg::clear_bits(&peripheral_->CR1, reg::usart::CR1_M);
    } else {
        reg::set_bits(&peripheral_->CR1, reg::usart::CR1_M);
    }
}

void STM32UART::set_baud_rate(UARTBaudRate baud_rate) {
    // For STM32F4, assuming PCLK1 or PCLK2 frequency is 42MHz
    // UART2, 3, 4, 5, 7, 8 use PCLK1, UART1, 6 use PCLK2
    const uint32_t pclk_hz = 42000000U; // This should come from a system config
    const uint32_t baud_value = baud_rate_to_value(baud_rate);

    // Calculate BRR value: PCLK / baud_rate
    uint32_t brr = pclk_hz / baud_value;
    peripheral_->BRR = brr;
}

void STM32UART::enable_interrupts(bool rx_interrupt, bool tx_interrupt, bool error_interrupt) {
    uint32_t cr1_bits = 0;

    if (rx_interrupt) {
        cr1_bits |= reg::usart::CR1_RXNEIE;
    }

    if (tx_interrupt) {
        cr1_bits |= reg::usart::CR1_TXEIE;
    }

    if (error_interrupt) {
        cr1_bits |= reg::usart::CR1_PEIE; // Parity error
    }

    // Set the interrupt enable bits
    reg::set_bits(&peripheral_->CR1, cr1_bits);

    // Also enable IDLE interrupt for better reception performance
    if (rx_interrupt) {
        reg::set_bits(&peripheral_->CR1, reg::usart::CR1_IDLEIE);
    }
}

void STM32UART::disable_interrupts() {
    // Clear all interrupt enable bits
    reg::clear_bits(
        &peripheral_->CR1,
        reg::usart::CR1_RXNEIE | reg::usart::CR1_TXEIE | reg::usart::CR1_PEIE | reg::usart::CR1_IDLEIE);
}

void STM32UART::enable_uart_nvic(std::uint8_t priority) {
    // Get the NVIC IRQ number
    const int16_t irq_num = device::to_cmsis_irqn(get_irq());

    // Set priority (0-15, where 0 is highest)
    NVIC_SetPriority(static_cast<IRQn_Type>(irq_num), priority);

    // Enable the interrupt
    NVIC_EnableIRQ(static_cast<IRQn_Type>(irq_num));
}

void STM32UART::disable_uart_nvic() {
    // Get the NVIC IRQ number
    const int16_t irq_num = device::to_cmsis_irqn(get_irq());

    // Disable the interrupt
    NVIC_DisableIRQ(static_cast<IRQn_Type>(irq_num));
}

hal::UartStatus STM32UART::convert_status(sync::UartStatus status) {
    switch (status) {
        case sync::UartStatus::Ok:
            return hal::UartStatus::Ok;
        case sync::UartStatus::ErrError:
            return hal::UartStatus::ErrError;
        case sync::UartStatus::ErrTimeout:
            return hal::UartStatus::ErrTimeout;
        case sync::UartStatus::ErrInvalidState:
            return hal::UartStatus::ErrError;
        case sync::UartStatus::ErrNotSupported:
            return hal::UartStatus::ErrError;
        default:
            return hal::UartStatus::ErrError;
    }
}

// Singleton instance management
std::shared_ptr<STM32UART> get_uart_instance(device::UARTIndex uart_index) {
    std::lock_guard<std::mutex> lock(uart_instances_mutex);
    const size_t idx = uart_index_to_array_index(uart_index);

    // If there's an existing instance, return a pointer to it
    if (idx < uart_instances.size() && uart_instances[idx] != nullptr) {
        return std::shared_ptr<STM32UART>(uart_instances[idx], [](STM32UART*) {
            // Empty deleter - we don't want to delete the instance here
        });
    }

    // No instance found
    return nullptr;
}

void register_uart_instance(device::UARTIndex uart_index, STM32UART* uart) {
    std::lock_guard<std::mutex> lock(uart_instances_mutex);
    const size_t idx = uart_index_to_array_index(uart_index);

    if (idx < uart_instances.size()) {
        uart_instances[idx] = uart;
    }
}

void unregister_uart_instance(device::UARTIndex uart_index) {
    std::lock_guard<std::mutex> lock(uart_instances_mutex);
    const size_t idx = uart_index_to_array_index(uart_index);

    if (idx < uart_instances.size()) {
        uart_instances[idx] = nullptr;
    }
}

void handle_uart_irq(device::UARTIndex uart_index) {
    // Lookup the UART instance
    auto uart = get_uart_instance(uart_index);

    // If found, delegate to the instance
    if (uart) {
        uart->handle_irq();
    }
}

void handle_dma_rx_irq(device::UARTIndex uart_index) {
    // Lookup the UART instance
    auto uart = get_uart_instance(uart_index);

    // If found, delegate to the instance
    if (uart) {
        uart->handle_dma_rx_irq();
    }
}

} // namespace hm11::platform::stm32