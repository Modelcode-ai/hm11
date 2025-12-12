/**
 * @file driver_test.cpp
 * @brief Main test file for HM11Driver class
 *
 * This file contains the main test fixture and test infrastructure for the
 * HM11Driver class.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>

#include "hm11/driver.hpp"
#include "hm11/hal/uart.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for HM11Driver tests
 *
 * This fixture provides common setup and teardown code for HM11Driver tests.
 * It creates a mock UART port, receive handler, and last read position handler,
 * and initializes an HM11Driver instance with them.
 */
class HM11DriverTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create and configure the mock UART
        uart_mock_ = std::make_shared<mocks::EnhancedUARTMock>();

        // Set up the mock receive handler
        mocks::MockReceiveHandler::SetInstance(&receive_handler_);

        // Set up the mock last read position handler
        mocks::MockLastReadPositionHandler::SetInstance(&last_read_handler_);

        // Set default behavior for the receive handler
        ON_CALL(
            receive_handler_,
            call(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Invoke([this](
                                      hal::AnyUartPort port [[maybe_unused]],
                                      void* received,
                                      std::size_t length,
                                      hal::UartStatus& status,
                                      std::chrono::milliseconds timeout_ms,
                                      bool as_stream [[maybe_unused]]) {
                    // Default implementation that delegates to the UART mock
                    hal::UartData8b rx_data(static_cast<hal::UInt8*>(received), length);
                    uart_mock_->receive(rx_data, status, timeout_ms);
                }));

        // Set default behavior for the last read position handler
        ON_CALL(last_read_handler_, call(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke([](bool& closed, std::size_t& zero) {
                closed = false;
                zero = 1;
            }));

        // Create the driver instance
        driver_ = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
            uart_mock_,
            mocks::MockReceiveHandler::ReceiveWrapper,
            mocks::MockLastReadPositionHandler::LastReadPositionWrapper);
    }

    void TearDown() override {
        // Clean up the driver
        driver_.reset();

        // Clean up the mock handlers
        mocks::MockReceiveHandler::SetInstance(nullptr);
        mocks::MockLastReadPositionHandler::SetInstance(nullptr);
    }

    // Helper method to queue a response in the UART mock
    void QueueResponse(const std::string& response) { uart_mock_->queue_receive_data(response); }

    // Helper method to add a command response mapping
    void AddCommandResponse(const std::string& command, const std::string& response) {
        uart_mock_->add_command_response(command, response);
    }

    // Helper method to verify transmitted data contains a specific string
    bool VerifyTransmittedData(const std::string& expected) {
        const auto& data = uart_mock_->transmitted_data_8b();
        std::string transmitted(data.begin(), data.end());
        return transmitted.find(expected) != std::string::npos;
    }

    std::shared_ptr<mocks::EnhancedUARTMock> uart_mock_;
    std::unique_ptr<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>> driver_;
    ::testing::NiceMock<mocks::MockReceiveHandler> receive_handler_;
    ::testing::NiceMock<mocks::MockLastReadPositionHandler> last_read_handler_;
};

// Basic test to verify the fixture works
TEST_F(HM11DriverTest, FixtureWorks) {
    EXPECT_NE(driver_, nullptr);
    EXPECT_NE(uart_mock_, nullptr);
}

} // namespace hm11::tests