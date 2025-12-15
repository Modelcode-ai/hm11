/**
 * @file driver_basic_ops_test.cpp
 * @brief Tests for basic HM11Driver operations
 *
 * This file contains tests for the basic operations of the HM11Driver class,
 * such as test, restart, reset, sleep, wake_up, etc.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "hm11/driver.hpp"
#include "hm11/hal/uart.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for basic HM11Driver operations
 *
 * This fixture provides a testing environment for the basic operations of the
 * HM11Driver class, reusing the main test fixture.
 */
class HM11DriverBasicOpsTest : public ::testing::Test {
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

    std::shared_ptr<mocks::EnhancedUARTMock> uart_mock_;
    std::unique_ptr<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>> driver_;
    ::testing::NiceMock<mocks::MockReceiveHandler> receive_handler_;
    ::testing::NiceMock<mocks::MockLastReadPositionHandler> last_read_handler_;
};

// Test the test operation
TEST_F(HM11DriverBasicOpsTest, TestOperation) {
    // Configure the mock to return "OK" when receiving "AT"
    uart_mock_->add_command_response("AT", "OK");

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT");
}

// Test the test operation with error
TEST_F(HM11DriverBasicOpsTest, TestOperationWithError) {
    // Configure the mock to return an error status
    uart_mock_->add_command_response("AT", "ERROR", hal::UartStatus::Error);

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test the restart operation
TEST_F(HM11DriverBasicOpsTest, RestartOperation) {
    // Configure the mock to return "OK" when receiving "AT+RESTART"
    uart_mock_->add_command_response("AT+RESTART", "OK");

    // Call the restart method
    hal::UartStatus status;
    driver_->restart(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+RESTART");
}

// Test the reset operation
TEST_F(HM11DriverBasicOpsTest, ResetOperation) {
    // Configure the mock to return "OK" when receiving "AT+RENEW"
    uart_mock_->add_command_response("AT+RENEW", "OK");

    // Call the reset method
    hal::UartStatus status;
    driver_->reset(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+RENEW");
}

// Test the sleep operation
TEST_F(HM11DriverBasicOpsTest, SleepOperation) {
    // Configure the mock to return "OK+SLEEP" when receiving "AT+SLEEP"
    uart_mock_->add_command_response("AT+SLEEP", "OK+SLEEP");

    // Call the sleep method
    hal::UartStatus status;
    driver_->sleep(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+SLEEP");
}

// Test the wake_up operation
TEST_F(HM11DriverBasicOpsTest, WakeUpOperation) {
    // Configure the mock to return "OK" when receiving a wake-up signal
    // Note: Wake-up is often just a UART character or timing-based operation
    uart_mock_->add_command_response("AT", "OK");

    // Call the wake_up method
    hal::UartStatus status;
    driver_->wake_up(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test the software_version operation
TEST_F(HM11DriverBasicOpsTest, SoftwareVersionOperation) {
    // Configure the mock to return a version string
    // HM-11 firmware versions are hex numbers like V545, V540, etc.
    uart_mock_->add_command_response("AT+VERSION", "HMSoft V545");

    // Call the software_version method
    util::VersionType version;
    hal::UartStatus status;
    driver_->software_version(version, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+VERSION");

    // Verify the version was extracted correctly (should be "0545" - 4 hex chars)
    EXPECT_EQ(version, util::VersionType("0545"));
}

// Test the disconnect operation
TEST_F(HM11DriverBasicOpsTest, DisconnectOperation) {
    // Configure the mock to return "OK" when receiving "AT"
    uart_mock_->add_command_response("AT+DISC", "OK");

    // Call the disconnect method
    hal::UartStatus status;
    driver_->disconnect(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+DISC");
}

// Test the start_working operation
TEST_F(HM11DriverBasicOpsTest, StartWorkingOperation) {
    // Configure the mock to return "OK" when receiving "AT+START"
    uart_mock_->add_command_response("AT+START", "OK");

    // Call the start_working method
    hal::UartStatus status;
    driver_->start_working(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+START");
}

// Test multiple operations in sequence
TEST_F(HM11DriverBasicOpsTest, MultipleOperations) {
    // Configure the mock to return specific responses for each command
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+RESET", "OK");
    uart_mock_->add_command_response("AT+VERSION", "HMSoft V545");

    // Perform multiple operations in sequence
    hal::UartStatus status;

    // Test operation
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Reset operation
    driver_->reset(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Software version operation
    util::VersionType version;
    driver_->software_version(version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(version, util::VersionType("0545"));
}

// Test operations with timeouts
TEST_F(HM11DriverBasicOpsTest, OperationWithTimeout) {
    // Configure the mock to simulate a timeout by setting an error status
    uart_mock_->add_command_response("AT", "", hal::UartStatus::Timeout);

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status reflects the timeout
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Timeout, status);
}

// Test operations with bad responses
TEST_F(HM11DriverBasicOpsTest, OperationWithBadResponse) {
    // Configure the mock to return an unexpected response
    uart_mock_->add_command_response("AT", "UNEXPECTED");

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // The expected behavior depends on the implementation
    // If it checks for "OK", this should result in an error
    // If it just checks for a non-empty response, this might succeed
    // For this test, we'll assume it checks for "OK" and should fail
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

} // namespace hm11::tests