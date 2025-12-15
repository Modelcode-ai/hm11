/**
 * @file uart_mock.hpp
 * @brief Mock implementation of the UART interface for testing
 *
 * This file provides a mock implementation of the UART Hardware Abstraction Layer
 * interface for testing purposes. It allows tests to simulate UART behavior without
 * requiring actual hardware.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_TESTS_MOCKS_UART_MOCK_HPP
#define HM11_TESTS_MOCKS_UART_MOCK_HPP

#include <deque>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "hm11/hal/uart.hpp"

namespace hm11::tests::mocks {

/**
 * @brief Mock implementation of UARTPort for testing
 *
 * This class provides a mock implementation of the UARTPort interface
 * that can be used in unit tests. It allows tests to verify interactions
 * with the UART interface without requiring actual hardware.
 */
class UARTMock : public hal::UARTPort {
  public:
    /**
     * @brief Constructor
     * @param data_size The data size to report (defaults to 8-bit)
     */
    explicit UARTMock(hal::UartDataSize data_size_param = hal::UartDataSize::DataSize8b)
        : m_data_size(data_size_param), 
          m_transmitted_data_8b{},
          m_transmitted_data_9b{},
          m_receive_queue_8b{},
          m_receive_queue_9b{} {}

    /**
     * @brief Get the data size
     * @return The data size (8-bit or 9-bit)
     */
    hal::UartDataSize data_size() const override { return m_data_size; }

    // Mock methods with MOCK_METHOD
    MOCK_METHOD(void, transmit,
                (const hal::UartData8b&, hal::UartStatus&,
                 std::chrono::milliseconds),
                (override));
    MOCK_METHOD(void, transmit,
                (const hal::UartData9b&, hal::UartStatus&,
                 std::chrono::milliseconds),
                (override));
    MOCK_METHOD(void, receive,
                (hal::UartData8b&, hal::UartStatus&, std::chrono::milliseconds),
                (override));
    MOCK_METHOD(void, receive,
                (hal::UartData9b&, hal::UartStatus&, std::chrono::milliseconds),
                (override));

    /**
     * @brief Set the default status to return for operations
     * @param status The status to return
     */
    void set_default_status(hal::UartStatus status) { m_default_status = status; }

    /**
     * @brief Queue data to be returned by the next receive call
     * @param data The data to queue
     */
    void queue_receive_data(const std::vector<hal::UInt8>& data) {
        m_receive_queue_8b.insert(m_receive_queue_8b.end(), data.begin(), data.end());
    }

    /**
     * @brief Queue data to be returned by the next receive call
     * @param data The data to queue (as a string)
     */
    void queue_receive_data(const std::string& data) {
        for (char c : data) {
            m_receive_queue_8b.push_back(static_cast<hal::UInt8>(c));
        }
        // For empty strings, queue a single null terminator to mark that data was set
        // For non-empty strings, don't add null - let receive add it
        if (data.empty()) {
            m_receive_queue_8b.push_back(0);
        }
    }

    /**
     * @brief Queue 9-bit data to be returned by the next receive call
     * @param data The data to queue
     */
    void queue_receive_data_9b(const std::vector<hal::UInt16>& data) {
        m_receive_queue_9b.insert(m_receive_queue_9b.end(), data.begin(), data.end());
    }

     /**
     * @brief Get the data that has been transmitted
     * @return The transmitted data
     */
    const std::vector<hal::UInt8>& transmitted_data_8b() const { return m_transmitted_data_8b; }

    /**
     * @brief Get the 9-bit data that has been transmitted
     * @return The transmitted 9-bit data
     */
    const std::vector<hal::UInt16>& transmitted_data_9b() const { return m_transmitted_data_9b; }

    /**
     * @brief Get the transmitted data as a string
     * @return The transmitted data as a string
     */
    std::string get_transmitted_string() const {
        return {reinterpret_cast<const char*>(m_transmitted_data_8b.data()), m_transmitted_data_8b.size()};
    }

    /**
     * @brief Clear the transmitted data
     */
    void clear_transmitted_data() {
        m_transmitted_data_8b.clear();
        m_transmitted_data_9b.clear();
    }

    /**
     * @brief Clear the receive queue
     */
    void clear_receive_queue() {
        m_receive_queue_8b.clear();
        m_receive_queue_9b.clear();
    }

/**
 * @brief Set up default behavior for the mock
 *
 * This method sets up default behavior for the mock methods. It's useful
 * for tests that don't need to customize the mock's behavior.
 */
// Not making this method static since it uses non-static class members
void setup_default_behavior() {
    using ::testing::_;
    using ::testing::DoAll;
    using ::testing::Invoke;
    using ::testing::Return;
    using ::testing::SetArgReferee;

    // We need to use concrete types to avoid ambiguity in the overloaded methods
    using Data8b = const hal::UartData8b&;
    using Data9b = const hal::UartData9b&;
    using OutData8b = hal::UartData8b&;
    using OutData9b = hal::UartData9b&;
    // These type aliases are used for clarity in method signatures
    using StatusRef [[maybe_unused]] = hal::UartStatus&;
    using Timeout [[maybe_unused]] = std::chrono::milliseconds;

     // Set the default behavior for transmit with 8-bit data
    ON_CALL(*this, transmit(::testing::An<Data8b>(), _, _))
        .WillByDefault(DoAll(
             Invoke([this](const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                m_transmitted_data_8b.insert(m_transmitted_data_8b.end(), data.begin(), data.end());
                status = m_default_status;
            }),
            SetArgReferee<1>(m_default_status),
            Return()
        ));

    // Set the default behavior for transmit with 9-bit data
    ON_CALL(*this, transmit(::testing::An<Data9b>(), _, _))
        .WillByDefault(DoAll(
             Invoke([this](const hal::UartData9b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                m_transmitted_data_9b.insert(m_transmitted_data_9b.end(), data.begin(), data.end());
                status = m_default_status;
            }),
            SetArgReferee<1>(m_default_status),
            Return()
        ));

    // Set the default behavior for receive with 8-bit data
    ON_CALL(*this, receive(::testing::An<OutData8b>(), _, _))
        .WillByDefault(
             Invoke([this](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // If we have data in the queue, fill the buffer
                if (!m_receive_queue_8b.empty()) {
                    // Copy as much data as possible
                    size_t copy_count = std::min(data.size(), m_receive_queue_8b.size());
                    std::copy_n(m_receive_queue_8b.begin(), copy_count, data.begin());
                    // Add a null terminator if there's space after the data
                    if (copy_count < data.size()) {
                        data[copy_count] = 0;
                    }
                    m_receive_queue_8b.erase(m_receive_queue_8b.begin(), m_receive_queue_8b.begin() + static_cast<long>(copy_count));
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
            })
        );

    // Set the default behavior for receive with 9-bit data
    ON_CALL(*this, receive(::testing::An<OutData9b>(), _, _))
        .WillByDefault(
             Invoke([this](hal::UartData9b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // If we have data in the queue, fill the buffer
                if (!m_receive_queue_9b.empty()) {
                    size_t copy_count = std::min(data.size(), m_receive_queue_9b.size());
                    std::copy_n(m_receive_queue_9b.begin(), copy_count, data.begin());
                    // Add a null terminator if space is available
                    if (copy_count < data.size()) {
                        data[copy_count] = 0;
                    }
                    m_receive_queue_9b.erase(m_receive_queue_9b.begin(), m_receive_queue_9b.begin() + static_cast<long>(copy_count));
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
            })
        );
}

  protected:
    // Protected so derived classes can access transmitted data
    std::vector<hal::UInt8> m_transmitted_data_8b;
    std::vector<hal::UInt16> m_transmitted_data_9b;
    // Protected so derived classes can check default status
    hal::UartStatus m_default_status{hal::UartStatus::Ok};

  private:
    hal::UartDataSize m_data_size;
    std::deque<hal::UInt8> m_receive_queue_8b;
    std::deque<hal::UInt16> m_receive_queue_9b;
};

/**
 * @brief Create a shared pointer to a UARTMock
 * @param data_size The data size to report (defaults to 8-bit)
 * @return A shared pointer to a UARTMock
 */
inline hal::AnyUartPort create_uart_mock(
    hal::UartDataSize data_size = hal::UartDataSize::DataSize8b) {
    auto mock = std::make_shared<UARTMock>(data_size);
    mock->setup_default_behavior();
    return mock;
}

} // namespace hm11::tests::mocks

#endif // HM11_TESTS_MOCKS_UART_MOCK_HPP