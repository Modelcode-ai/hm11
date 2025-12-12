/**
 * @file uart_mock_test.cpp
 * @brief Tests for the UART mock implementation
 *
 * This file contains unit tests for the UART mock implementation.
 * It verifies that the mock behaves correctly and can be used for testing.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include "mocks/uart_mock.hpp"

namespace hm11::tests {

class UARTMockTest : public ::testing::Test {
  protected:
    void SetUp() override {
        uart_mock_ = std::make_shared<mocks::UARTMock>();
        uart_mock_->setup_default_behavior();
    }

    std::shared_ptr<mocks::UARTMock> uart_mock_;
};

// Test that the mock can be created with the default data size
TEST_F(UARTMockTest, DefaultDataSize) {
    EXPECT_EQ(uart_mock_->data_size(), hal::UartDataSize::DataSize8b);
}

// Test that the mock can be created with a custom data size
TEST_F(UARTMockTest, CustomDataSize) {
    auto uart_mock = std::make_shared<mocks::UARTMock>(hal::UartDataSize::DataSize9b);
    EXPECT_EQ(uart_mock->data_size(), hal::UartDataSize::DataSize9b);
}

// Test that the mock can transmit 8-bit data
TEST_F(UARTMockTest, Transmit8BitData) {
    // Create test data
    std::vector<hal::UInt8> data = {0x01, 0x02, 0x03, 0x04};
    hal::UartData8b uart_data(data);

    // Transmit the data
    hal::UartStatus status;
    uart_mock_->transmit(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status and transmitted data
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_mock_->transmitted_data_8b(), data);
}

// Test that the mock can transmit 9-bit data
TEST_F(UARTMockTest, Transmit9BitData) {
    // Create test data
    std::vector<hal::UInt16> data = {0x001, 0x002, 0x003, 0x004};
    hal::UartData9b uart_data(data);

    // Transmit the data
    hal::UartStatus status;
    uart_mock_->transmit(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status and transmitted data
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_mock_->transmitted_data_9b(), data);
}

// Test that the mock can receive 8-bit data
TEST_F(UARTMockTest, Receive8BitData) {
    // Queue data to be received
    std::vector<hal::UInt8> data = {0x01, 0x02, 0x03, 0x04};
    uart_mock_->queue_receive_data(data);

    // Create a buffer to receive the data
    std::vector<hal::UInt8> buffer(4);
    hal::UartData8b uart_data(buffer);

    // Receive the data
    hal::UartStatus status;
    uart_mock_->receive(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status and received data
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(buffer, data);
}

// Test that the mock can receive 9-bit data
TEST_F(UARTMockTest, Receive9BitData) {
    // Queue data to be received
    std::vector<hal::UInt16> data = {0x001, 0x002, 0x003, 0x004};
    uart_mock_->queue_receive_data_9b(data);

    // Create a buffer to receive the data
    std::vector<hal::UInt16> buffer(4);
    hal::UartData9b uart_data(buffer);

    // Receive the data
    hal::UartStatus status;
    uart_mock_->receive(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status and received data
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(buffer, data);
}

// Test that the mock can receive string data
TEST_F(UARTMockTest, ReceiveStringData) {
    // Queue string data to be received
    std::string data = "Test";
    uart_mock_->queue_receive_data(data);

    // Create a buffer to receive the data
    std::vector<hal::UInt8> buffer(4);
    hal::UartData8b uart_data(buffer);

    // Receive the data
    hal::UartStatus status;
    uart_mock_->receive(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status and received data
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(buffer.to_array().data()), buffer.size()), data);
}

// Test that the mock returns a timeout when no data is available
TEST_F(UARTMockTest, ReceiveTimeout) {
    // Create a buffer to receive the data
    std::vector<hal::UInt8> buffer(4);
    hal::UartData8b uart_data(buffer);

    // Set up the mock to return a timeout
    uart_mock_->set_default_status(hal::UartStatus::Err_Timeout);

    // Receive the data
    hal::UartStatus status;
    uart_mock_->receive(uart_data, status, std::chrono::milliseconds(100));

    // Verify the status
    EXPECT_EQ(status, hal::UartStatus::Err_Timeout);
}

// Test that the mock can be used with the factory function
TEST_F(UARTMockTest, CreateUARTMock) {
    auto uart = mocks::create_uart_mock(hal::UartDataSize::DataSize9b);
    EXPECT_EQ(uart->data_size(), hal::UartDataSize::DataSize9b);
}

} // namespace hm11::tests