/**
 * @file uart_mock.hpp
 * @brief Simplified UART mock for the peripheral echo example
 *
 * This file provides a simplified version of the UART mock for the peripheral echo example.
 * It's based on the mock in the basic example.
 */

#ifndef HM11_EXAMPLES_PERIPHERAL_ECHO_UART_MOCK_HPP
#define HM11_EXAMPLES_PERIPHERAL_ECHO_UART_MOCK_HPP

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
 * that can be used in examples. It allows testing without requiring actual hardware.
 */
class UARTMock : public hal::UARTPort {
  public:
    /**
     * @brief Constructor
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
     * @brief Get the data size
     * @return The data size (8-bit or 9-bit)
     */
    hal::UartDataSize data_size() const override { return data_size_; }

    // Override methods with actual implementations instead of mocks
    void transmit(
        const hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout;
        // Store the transmitted data
        transmitted_data_8b_.insert(transmitted_data_8b_.end(), data.begin(), data.end());
        status = default_status_;
    }

    void transmit(
        const hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout;
        // Store the transmitted data
        transmitted_data_9b_.insert(transmitted_data_9b_.end(), data.begin(), data.end());
        status = default_status_;
    }

    void receive(
        hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout;
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
            std::string default_response = "OK";
            std::copy(default_response.begin(), default_response.end(), data.begin());
            // Add null terminator
            if (default_response.size() < data.size()) {
                data[default_response.size()] = 0;
            }
            status = hal::UartStatus::Ok;
        }
    }

    void receive(
        hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override {
        (void)timeout;
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
     * @param status The status to return
     */
    void set_default_status(hal::UartStatus status) { default_status_ = status; }

    /**
     * @brief Queue data to be returned by the next receive call
     * @param data The data to queue
     */
    void queue_receive_data(const std::vector<hal::UInt8>& data) {
        receive_queue_8b_.insert(receive_queue_8b_.end(), data.begin(), data.end());
    }

    /**
     * @brief Queue data to be returned by the next receive call
     * @param data The data to queue (as a string)
     */
    void queue_receive_data(const std::string& data) {
        for (char c : data) {
            receive_queue_8b_.push_back(static_cast<hal::UInt8>(c));
        }
    }

    /**
     * @brief Queue 9-bit data to be returned by the next receive call
     * @param data The data to queue
     */
    void queue_receive_data_9b(const std::vector<hal::UInt16>& data) {
        receive_queue_9b_.insert(receive_queue_9b_.end(), data.begin(), data.end());
    }

    /**
     * @brief Get the data that has been transmitted
     * @return The transmitted data
     */
    const std::vector<hal::UInt8>& transmitted_data_8b() const { return transmitted_data_8b_; }

    /**
     * @brief Get the 9-bit data that has been transmitted
     * @return The transmitted 9-bit data
     */
    const std::vector<hal::UInt16>& transmitted_data_9b() const { return transmitted_data_9b_; }

    /**
     * @brief Clear the transmitted data
     */
    void clear_transmitted_data() {
        transmitted_data_8b_.clear();
        transmitted_data_9b_.clear();
    }

  private:
    hal::UartDataSize data_size_;
    hal::UartStatus default_status_;
    std::vector<hal::UInt8> transmitted_data_8b_;
    std::vector<hal::UInt16> transmitted_data_9b_;
    std::deque<hal::UInt8> receive_queue_8b_;
    std::deque<hal::UInt16> receive_queue_9b_;
};

/**
 * @brief Create a shared pointer to a UARTMock
 * @param data_size The data size to report (defaults to 8-bit)
 * @return A shared pointer to a UARTMock
 */
inline hal::AnyUartPort create_uart_mock(
    hal::UartDataSize data_size = hal::UartDataSize::DataSize8b) {
    return std::make_shared<UARTMock>(data_size);
}

} // namespace hm11::tests::mocks

#endif // HM11_EXAMPLES_PERIPHERAL_ECHO_UART_MOCK_HPP