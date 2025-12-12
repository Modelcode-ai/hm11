/**
 * @file uart_test.cpp
 * @brief Unit tests for HAL UART interface
 *
 * This file contains unit tests for the HAL UART interface defined in uart.hpp.
 * Tests ensure that the interface behaves as expected and works with both
 * dynamic and static polymorphism.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "hm11/hal/uart.hpp"

namespace {

using namespace hm11::hal;

// Mock UART implementation for testing
class MockUARTPort : public UARTPort {
  public:
    MockUARTPort(UartDataSize data_size) : data_size_(data_size) {}

    // Records of the last operation
    mutable struct {
        std::vector<UInt8> last_tx_8bit;
        std::vector<UInt16> last_tx_9bit;
        std::chrono::milliseconds last_timeout;
        bool tx_called = false;
        bool rx_called = false;
    } record;

    // Configuration for mock behavior
    struct {
        UartStatus status_to_return = UartStatus::Ok;
        std::vector<UInt8> rx_data_8bit;
        std::vector<UInt16> rx_data_9bit;
    } config;

    UartDataSize data_size() const override { return data_size_; }

    void transmit(
        const UartData8b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override {
        record.tx_called = true;
        record.last_timeout = timeout_ms;

        // Store the transmitted data
        record.last_tx_8bit.clear();
        for (size_t i = 0; i < data.size(); ++i) {
            record.last_tx_8bit.push_back(data[i]);
        }

        status = config.status_to_return;
    }

    void transmit(
        const UartData9b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override {
        record.tx_called = true;
        record.last_timeout = timeout_ms;

        // Store the transmitted data
        record.last_tx_9bit.clear();
        for (size_t i = 0; i < data.size(); ++i) {
            record.last_tx_9bit.push_back(data[i]);
        }

        status = config.status_to_return;
    }

    void receive(
        UartData8b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override {
        record.rx_called = true;
        record.last_timeout = timeout_ms;

        // Fill the receive buffer with mock data
        size_t copy_size = std::min(data.size(), config.rx_data_8bit.size());
        for (size_t i = 0; i < copy_size; ++i) {
            data[i] = config.rx_data_8bit[i];
        }

        status = config.status_to_return;
    }

    void receive(
        UartData9b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) override {
        record.rx_called = true;
        record.last_timeout = timeout_ms;

        // Fill the receive buffer with mock data
        size_t copy_size = std::min(data.size(), config.rx_data_9bit.size());
        for (size_t i = 0; i < copy_size; ++i) {
            data[i] = config.rx_data_9bit[i];
        }

        status = config.status_to_return;
    }

  private:
    UartDataSize data_size_;
};

// Test fixture
class UARTTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize with default values
        mock_8bit.config.status_to_return = UartStatus::Ok;
        mock_8bit.config.rx_data_8bit = {0x12, 0x34, 0x56, 0x78};

        mock_9bit.config.status_to_return = UartStatus::Ok;
        mock_9bit.config.rx_data_9bit = {0x123, 0x456, 0x789};
    }

    // Mock implementations for 8-bit and 9-bit data sizes
    MockUARTPort mock_8bit{UartDataSize::DataSize8b};
    MockUARTPort mock_9bit{UartDataSize::DataSize9b};

    // Test data
    std::array<UInt8, 4> test_data_8bit = {0xAA, 0xBB, 0xCC, 0xDD};
    std::array<UInt16, 3> test_data_9bit = {0x1AA, 0x1BB, 0x1CC}; // Using 9-bit values
};

// Test the UartData8b class
TEST_F(UARTTest, UartData8b_Works) {
    // Test construction from array
    UartData8b data(test_data_8bit);
    EXPECT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0xAA);
    EXPECT_EQ(data[3], 0xDD);

    // Test construction from vector
    std::vector<UInt8> vec = {0x11, 0x22, 0x33};
    UartData8b data2(vec);
    EXPECT_EQ(data2.size(), 3u);
    EXPECT_EQ(data2[0], 0x11);
    EXPECT_EQ(data2[2], 0x33);

    // Test iteration
    int i = 0;
    for (auto val : data) {
        EXPECT_EQ(val, test_data_8bit[i++]);
    }
}

// Test the UartData9b class
TEST_F(UARTTest, UartData9b_Works) {
    // Test construction from array
    UartData9b data(test_data_9bit);
    EXPECT_EQ(data.size(), 3u);
    EXPECT_EQ(data[0], 0x1AA);
    EXPECT_EQ(data[2], 0x1CC);

    // Test construction from vector
    std::vector<UInt16> vec = {0x111, 0x122, 0x133};
    UartData9b data2(vec);
    EXPECT_EQ(data2.size(), 3u);
    EXPECT_EQ(data2[0], 0x111);
    EXPECT_EQ(data2[2], 0x133);

    // Test iteration
    int i = 0;
    for (auto val : data) {
        EXPECT_EQ(val, test_data_9bit[i++]);
    }
}

// Test transmitting 8-bit data
TEST_F(UARTTest, Transmit_8bit_Works) {
    // Prepare test data
    UartData8b tx_data(test_data_8bit);
    UartStatus status;

    // Transmit data
    mock_8bit.transmit(tx_data, status);

    // Verify results
    EXPECT_TRUE(mock_8bit.record.tx_called);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_8bit.record.last_tx_8bit.size(), 4u);
    EXPECT_EQ(mock_8bit.record.last_tx_8bit[0], 0xAA);
    EXPECT_EQ(mock_8bit.record.last_tx_8bit[3], 0xDD);
    EXPECT_EQ(mock_8bit.record.last_timeout, std::chrono::milliseconds(1000));
}

// Test transmitting 9-bit data
TEST_F(UARTTest, Transmit_9bit_Works) {
    // Prepare test data
    UartData9b tx_data(test_data_9bit);
    UartStatus status;

    // Transmit data
    mock_9bit.transmit(tx_data, status);

    // Verify results
    EXPECT_TRUE(mock_9bit.record.tx_called);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_9bit.record.last_tx_9bit.size(), 3u);
    EXPECT_EQ(mock_9bit.record.last_tx_9bit[0], 0x1AA);
    EXPECT_EQ(mock_9bit.record.last_tx_9bit[2], 0x1CC);
    EXPECT_EQ(mock_9bit.record.last_timeout, std::chrono::milliseconds(1000));
}

// Test receiving 8-bit data
TEST_F(UARTTest, Receive_8bit_Works) {
    // Prepare receive buffer
    std::array<UInt8, 4> rx_buffer = {0};
    UartData8b rx_data(rx_buffer);
    UartStatus status;

    // Configure mock
    mock_8bit.config.rx_data_8bit = {0x12, 0x34, 0x56, 0x78};

    // Receive data
    mock_8bit.receive(rx_data, status);

    // Verify results
    EXPECT_TRUE(mock_8bit.record.rx_called);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_buffer[0], 0x12);
    EXPECT_EQ(rx_buffer[1], 0x34);
    EXPECT_EQ(rx_buffer[2], 0x56);
    EXPECT_EQ(rx_buffer[3], 0x78);
}

// Test receiving 9-bit data
TEST_F(UARTTest, Receive_9bit_Works) {
    // Prepare receive buffer
    std::array<UInt16, 3> rx_buffer = {0};
    UartData9b rx_data(rx_buffer);
    UartStatus status;

    // Configure mock
    mock_9bit.config.rx_data_9bit = {0x123, 0x456, 0x789};

    // Receive data
    mock_9bit.receive(rx_data, status);

    // Verify results
    EXPECT_TRUE(mock_9bit.record.rx_called);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_buffer[0], 0x123);
    EXPECT_EQ(rx_buffer[1], 0x456);
    EXPECT_EQ(rx_buffer[2], 0x789);
}

// Test error handling
TEST_F(UARTTest, Error_Handling_Works) {
    // Prepare test data
    UartData8b tx_data(test_data_8bit);
    UartStatus status;

    // Configure mock to return an error
    mock_8bit.config.status_to_return = UartStatus::Err_Error;

    // Transmit data
    mock_8bit.transmit(tx_data, status);

    // Verify results
    EXPECT_EQ(status, UartStatus::Err_Error);

    // Configure mock to return a timeout
    mock_8bit.config.status_to_return = UartStatus::Err_Timeout;

    // Transmit data
    mock_8bit.transmit(tx_data, status);

    // Verify results
    EXPECT_EQ(status, UartStatus::Err_Timeout);
}

// Test custom timeout
TEST_F(UARTTest, Custom_Timeout_Works) {
    // Prepare test data
    UartData8b tx_data(test_data_8bit);
    UartStatus status;

    // Transmit data with custom timeout
    mock_8bit.transmit(tx_data, status, std::chrono::milliseconds(500));

    // Verify results
    EXPECT_EQ(mock_8bit.record.last_timeout, std::chrono::milliseconds(500));
}

// Test UART_Implementation concept
TEST_F(UARTTest, UART_Implementation_Concept_Works) {
    // Check if MockUARTPort satisfies the UART_Implementation concept
    static_assert(UART_Implementation<MockUARTPort>, "MockUARTPort should satisfy the UART_Implementation concept");

    // Define a class that doesn't satisfy the concept
    class NotAUARTPort {
      public:
        void some_method() {}
    };

    // Check that NotAUARTPort doesn't satisfy the concept
    static_assert(
        !UART_Implementation<NotAUARTPort>,
        "NotAUARTPort should not satisfy the UART_Implementation concept");
}

// Test polymorphic usage of UARTPort
TEST_F(UARTTest, Polymorphic_Usage_Works) {
    // Create a shared pointer to UARTPort
    AnyUartPort uart_port = std::make_shared<MockUARTPort>(UartDataSize::DataSize8b);

    // Prepare test data
    std::array<UInt8, 4> data = {0xAA, 0xBB, 0xCC, 0xDD};
    UartData8b tx_data(data);
    UartStatus status;

    // Transmit data through the polymorphic interface
    uart_port->transmit(tx_data, status);

    // Verify that the data size is correct
    EXPECT_EQ(uart_port->data_size(), UartDataSize::DataSize8b);
}

} // namespace