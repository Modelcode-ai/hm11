/**
 * @file uart_mock.hpp
 * @brief Simplified UART mock implementation for the configuration example
 *
 * This file provides a simplified version of the UART mock for testing the
 * configuration example. It's based on the mock in tests/mocks/uart_mock.hpp
 * but without the dependency on GoogleTest. The UART mock enables testing of
 * the HM11 driver without requiring actual hardware, by simulating the
 * hardware responses.
 * 
 * This implementation follows several design decisions:
 * - Decision 1: Demonstrates dependency injection mechanism with UARTPort interface
 * - Decision 3: Provides a concrete implementation of the UART interface
 * - Decision 21: Supports testing of ISR-safe code without actual hardware
 * - Decision 30: Simulates the low-level hardware interaction
 *
 * The mock provides methods to:
 * - Queue response data to be returned by receive calls
 * - Track transmitted data for verification
 * - Configure behavior such as default status codes and data size
 */

#ifndef HM11_EXAMPLES_UART_MOCK_HPP
#define HM11_EXAMPLES_UART_MOCK_HPP

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "hm11/hal/uart.hpp"

namespace hm11::tests::mocks {

/**
 * @brief Mock implementation of UARTPort for testing
 *
 * This class provides a mock implementation of the UARTPort interface
 * that can be used in examples. It allows testing without requiring actual hardware
 * by simulating the behavior of a real UART connected to an HM11 module.
 * 
 * The mock stores transmitted data for verification and provides queued
 * responses for receive operations, enabling full simulation of the communication
 * with the HM11 module.
 */
class UARTMock : public hal::UARTPort {
  public:
    /**
     * @brief Constructor
     * 
     * Creates a new UART mock with the specified data size.
     * 
     * @param data_size The data size to report (defaults to 8-bit)
     */
    explicit UARTMock(hal::UartDataSize data_size = hal::UartDataSize::DataSize8b)
        : data_size_(data_size), 
          default_status_(hal::UartStatus::Ok), 
          transmitted_data_8b_(), 
          transmitted_data_9b_(), 
          receive_queue_8b_(), 
          receive_queue_9b_() {}

    /**
     * @brief Get the data size configuration
     * 
     * Returns the configured UART data size (8-bit or 9-bit).
     * 
     * @return The configured data size
     */
    hal::UartDataSize data_size() const override { return data_size_; }

    /**
     * @brief Transmit 8-bit data over the mock UART
     * 
     * Simulates transmitting data over the UART by storing the data
     * for later verification. In a real implementation, this would
     * send data to the hardware UART.
     * 
     * @param data The data to transmit
     * @param status Output parameter for operation status
     * @param timeout Timeout for the operation (default: 1000ms)
     */
    void transmit(
        const hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout; // Unused in this mock implementation
        
        // Store the transmitted data for later verification
        transmitted_data_8b_.insert(transmitted_data_8b_.end(), data.begin(), data.end());
        status = default_status_;
    }

    /**
     * @brief Transmit 9-bit data over the mock UART
     * 
     * Simulates transmitting 9-bit data over the UART by storing the data
     * for later verification. In a real implementation, this would
     * send data to the hardware UART.
     * 
     * @param data The 9-bit data to transmit
     * @param status Output parameter for operation status
     * @param timeout Timeout for the operation (default: 1000ms)
     */
    void transmit(
        const hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout; // Unused in this mock implementation
        
        // Store the transmitted data for later verification
        transmitted_data_9b_.insert(transmitted_data_9b_.end(), data.begin(), data.end());
        status = default_status_;
    }

    /**
     * @brief Receive 8-bit data from the mock UART
     * 
     * Simulates receiving data from the UART by returning data that was
     * previously queued using queue_receive_data(). If no data was queued,
     * it returns a default "OK" response. In a real implementation, this
     * would read data from the hardware UART.
     * 
     * @param data Output buffer to store received data
     * @param status Output parameter for operation status
     * @param timeout Timeout for the operation (default: 1000ms)
     */
    void receive(
        hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout; // Unused in this mock implementation
        
        // If we have data in the queue, fill the buffer
        if (!receive_queue_8b_.empty()) {
            size_t copy_count = 0;
            copy_count = std::min(data.size(), receive_queue_8b_.size());
            std::copy_n(receive_queue_8b_.begin(), copy_count, data.begin());
            // Add a null terminator if space is available
            if (copy_count < data.size()) {
                data[copy_count] = 0;
            }
            receive_queue_8b_.erase(receive_queue_8b_.begin(), 
                                   receive_queue_8b_.begin() + static_cast<long>(copy_count));
            status = hal::UartStatus::Ok;
        } else {
            // Generate a default "OK" response if no specific data is queued
            // This simulates the HM11 default response to AT commands
            std::string default_response = "OK";
            std::copy(default_response.begin(), default_response.end(), data.begin());
            // Add null terminator
            if (default_response.size() < data.size()) {
                data[default_response.size()] = 0;
            }
            status = hal::UartStatus::Ok;
        }
    }

    /**
     * @brief Receive 9-bit data from the mock UART
     * 
     * Simulates receiving 9-bit data from the UART by returning data that was
     * previously queued using queue_receive_data_9b(). If no data was queued,
     * it returns a default "OK" response. In a real implementation, this
     * would read 9-bit data from the hardware UART.
     * 
     * @param data Output buffer to store received 9-bit data
     * @param status Output parameter for operation status
     * @param timeout Timeout for the operation (default: 1000ms)
     */
    void receive(
        hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout; // Unused in this mock implementation
        
        // If we have data in the queue, fill the buffer
        if (!receive_queue_9b_.empty()) {
            size_t copy_count = 0;
            copy_count = std::min(data.size(), receive_queue_9b_.size());
            std::copy_n(receive_queue_9b_.begin(), copy_count, data.begin());
            // Add a null terminator if space is available
            if (copy_count < data.size()) {
                data[copy_count] = 0;
            }
            receive_queue_9b_.erase(receive_queue_9b_.begin(), 
                                   receive_queue_9b_.begin() + static_cast<long>(copy_count));
            status = hal::UartStatus::Ok;
        } else {
            // Generate a default "OK" response (using the first 16 bits)
            // This simulates the HM11 default response to AT commands
            if (data.size() > 0) {
                data[0] = 'O';
            }
            if (data.size() > 1) {
                data[1] = 'K';
            }
            // Add null terminator
            if (data.size() > 2) {
                data[2] = 0;
            }
            status = hal::UartStatus::Ok;
        }
    }

    /**
     * @brief Set the default status to return for operations
     * 
     * Configures the default status code that will be returned by transmit
     * and receive operations. This allows testing error handling by simulating
     * various error conditions.
     * 
     * @param status The status to return
     */
    void set_default_status(hal::UartStatus status) { default_status_ = status; }

    /**
     * @brief Queue binary data to be returned by the next receive call
     * 
     * Adds 8-bit data to the receive queue, which will be returned by
     * subsequent calls to receive(). This allows testing specific responses
     * from the HM11 module.
     * 
     * @param data The binary data to queue
     */
    void queue_receive_data(const std::vector<hal::UInt8>& data) {
        receive_queue_8b_.insert(receive_queue_8b_.end(), data.begin(), data.end());
    }

    /**
     * @brief Queue string data to be returned by the next receive call
     * 
     * Adds string data to the receive queue, which will be returned by
     * subsequent calls to receive(). This is a convenience method for
     * queueing text-based AT command responses.
     * 
     * @param data The string data to queue
     */
    void queue_receive_data(const std::string& data) {
        for (char c : data) {
            receive_queue_8b_.push_back(static_cast<hal::UInt8>(c));
        }
    }

    /**
     * @brief Queue 9-bit data to be returned by the next receive call
     * 
     * Adds 9-bit data to the receive queue, which will be returned by
     * subsequent calls to receive() for 9-bit data. This allows testing
     * specific responses for 9-bit UART mode.
     * 
     * @param data The 9-bit data to queue
     */
    void queue_receive_data_9b(const std::vector<hal::UInt16>& data) {
        receive_queue_9b_.insert(receive_queue_9b_.end(), data.begin(), data.end());
    }

    /**
     * @brief Get the 8-bit data that has been transmitted
     * 
     * Returns all 8-bit data that has been transmitted via this mock.
     * This allows verification of the data sent to the HM11 module.
     * 
     * @return The transmitted 8-bit data
     */
    const std::vector<hal::UInt8>& transmitted_data_8b() const { return transmitted_data_8b_; }

    /**
     * @brief Get the 9-bit data that has been transmitted
     * 
     * Returns all 9-bit data that has been transmitted via this mock.
     * This allows verification of the data sent to the HM11 module
     * when using 9-bit UART mode.
     * 
     * @return The transmitted 9-bit data
     */
    const std::vector<hal::UInt16>& transmitted_data_9b() const { return transmitted_data_9b_; }

    /**
     * @brief Clear all transmitted data records
     * 
     * Resets the internal storage of transmitted data. This is useful
     * when you need to verify only data transmitted after a certain point.
     */
    void clear_transmitted_data() {
        transmitted_data_8b_.clear();
        transmitted_data_9b_.clear();
    }

  private:
    hal::UartDataSize data_size_;               ///< Configured data size
    hal::UartStatus default_status_;            ///< Default status to return
    std::vector<hal::UInt8> transmitted_data_8b_;    ///< Storage for transmitted 8-bit data
    std::vector<hal::UInt16> transmitted_data_9b_;   ///< Storage for transmitted 9-bit data
    std::deque<hal::UInt8> receive_queue_8b_;        ///< Queue of 8-bit data to return on receive
    std::deque<hal::UInt16> receive_queue_9b_;       ///< Queue of 9-bit data to return on receive
};

/**
 * @brief Create a shared pointer to a UARTMock
 * 
 * Factory function that creates a new UARTMock instance and returns it
 * as a shared pointer to the UARTPort interface. This follows the factory
 * pattern and makes it easy to inject the mock into the HM11 driver.
 * 
 * @param data_size The data size to configure (defaults to 8-bit)
 * @return A shared pointer to the created UARTMock
 */
inline hal::AnyUartPort create_uart_mock(
    hal::UartDataSize data_size = hal::UartDataSize::DataSize8b) {
    return std::make_shared<UARTMock>(data_size);
}

} // namespace hm11::tests::mocks

#endif // HM11_EXAMPLES_UART_MOCK_HPP