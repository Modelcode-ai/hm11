/**
 * @file example_test.cpp
 * @brief Example test demonstrating the test infrastructure
 *
 * This file contains an example test that demonstrates how to use the
 * test infrastructure, including the UART mock and test utilities.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hm11/hal/uart.hpp"
#include "mocks/uart_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

class ExampleTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a UART mock
        uart_mock_ = std::make_shared<mocks::UARTMock>();
        uart_mock_->setup_default_behavior();
    }

    std::shared_ptr<mocks::UARTMock> uart_mock_;
};

// Example test that demonstrates how to use the UART mock
TEST_F(ExampleTest, UARTMockExample) {
    // Create test data
    const std::string test_data = "AT+VERSION";
    const std::string response = "HMSoft V1.0";

    // Set up the mock to return the response
    uart_mock_->queue_receive_data(response);

    // Create buffers for transmitting and receiving
    std::vector<hal::UInt8> tx_buffer(test_data.begin(), test_data.end());
    std::vector<hal::UInt8> rx_buffer(response.size());

    hal::UartData8b tx_data(tx_buffer);
    hal::UartData8b rx_data(rx_buffer);

    // Transmit the data
    hal::UartStatus tx_status;
    uart_mock_->transmit(tx_data, tx_status, std::chrono::milliseconds(100));

    // Verify the transmit status and data
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, tx_status);
    EXPECT_EQ(uart_mock_->transmitted_data_8b(), tx_buffer);

    // Receive the response
    hal::UartStatus rx_status;
    uart_mock_->receive(rx_data, rx_status, std::chrono::milliseconds(100));

    // Verify the receive status and data
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, rx_status);
    EXPECT_EQ(utils::uint8_vector_to_string(rx_buffer), response);
}

// Example test that demonstrates the test utilities
TEST_F(ExampleTest, TestUtilitiesExample) {
    // Generate random data
    const std::size_t data_size = 10;
    auto random_data = utils::random_uint8_vector(data_size);
    auto random_str = utils::random_string(data_size);
    auto random_hex = utils::random_hex_string(data_size);

    // Verify the size of the generated data
    EXPECT_EQ(random_data.size(), data_size);
    EXPECT_EQ(random_str.size(), data_size);
    EXPECT_EQ(random_hex.size(), data_size);

    // Verify conversion between string and UInt8 vector
    auto str_from_vector = utils::uint8_vector_to_string(random_data);
    auto vector_from_str = utils::string_to_uint8_vector(str_from_vector);
    EXPECT_EQ(vector_from_str, random_data);

    // Verify the execute_with_timeout utility
    bool executed = utils::execute_with_timeout(
        []() {
            utils::sleep_for(std::chrono::milliseconds(10));
            return true;
        },
        std::chrono::milliseconds(100));
    EXPECT_TRUE(executed);

    bool timed_out = utils::execute_with_timeout(
        []() {
            utils::sleep_for(std::chrono::milliseconds(200));
            return true;
        },
        std::chrono::milliseconds(100));
    EXPECT_FALSE(timed_out);
}

} // namespace hm11::tests