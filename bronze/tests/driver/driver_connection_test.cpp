/**
 * @file driver_connection_test.cpp
 * @brief Tests for HM11Driver connection management functionality
 *
 * This file contains tests for the connection management functions of the
 * HM11Driver class, including connect, connect_last_device, and disconnect.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>

#include "hm11/driver.hpp"
#include "hm11/enum_types.hpp"
#include "hm11/hal/uart.hpp"
#include "hm11/util/fixed_string.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for HM11Driver connection management tests
 *
 * This fixture provides common setup and teardown code for testing
 * the connection management functions of the HM11Driver.
 */
class HM11DriverConnectionTest : public ::testing::Test {
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

// Test connecting to a device with MAC address
TEST_F(HM11DriverConnectionTest, ConnectWithMacAddress) {
    // Create a test MAC address
    util::MacAddress mac("123456789ABC");

    // Configure the mock to return a successful connection response
    uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONN");

    // Call the connect method
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Error; // Start with error to verify it changes
    driver_->connect(mac, result, status);

    // Verify the status and result
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(ConnectResult::Connected, result);
}

// Test connecting to a device with MAC address string
TEST_F(HM11DriverConnectionTest, ConnectWithMacAddressString) {
    // Configure the mock to return a successful connection response
    uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONN");

    // Call the connect method with a string
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Error; // Start with error to verify it changes
    driver_->connect("123456789ABC", result, status);

    // Verify the status and result
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(ConnectResult::Connected, result);
}

// Test connecting with an invalid MAC address string (too short)
TEST_F(HM11DriverConnectionTest, ConnectWithInvalidMacAddressString) {
    // Call the connect method with an invalid string (too short)
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Ok; // Start with OK to verify it changes
    driver_->connect("12345", result, status);

    // Verify the status and result
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
    EXPECT_EQ(ConnectResult::OtherError, result);
}

// Test various connection result responses
TEST_F(HM11DriverConnectionTest, ConnectResultResponses) {
    // Test "Connecting" response
    {
        util::MacAddress mac("123456789ABC");
        uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONNA");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect(mac, result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::Connecting, result);
    }

    // Test "ConnectError" response
    {
        util::MacAddress mac("123456789ABC");
        uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONNE");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect(mac, result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::ConnectError, result);
    }

    // Test "ConnectFail" response
    {
        util::MacAddress mac("123456789ABC");
        uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONNF");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect(mac, result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::ConnectFail, result);
    }

    // Test "NoAddress" response
    {
        util::MacAddress mac("123456789ABC");
        uart_mock_->add_command_response("AT+CON123456789ABC", "OK+CONN?");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect(mac, result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::NoAddress, result);
    }

    // Test unknown response
    {
        util::MacAddress mac("123456789ABC");
        uart_mock_->add_command_response("AT+CON123456789ABC", "OK+UNKNOWN");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect(mac, result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
        EXPECT_EQ(ConnectResult::OtherError, result);
    }
}

// Test connecting to the last connected device
TEST_F(HM11DriverConnectionTest, ConnectLastDevice) {
    // Configure the mock to return a successful connection response
    uart_mock_->add_command_response("AT+CONNL", "OK+CONN");

    // Call the connect_last_device method
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Error; // Start with error to verify it changes
    driver_->connect_last_device(result, status);

    // Verify the status and result
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(ConnectResult::Connected, result);
}

// Test various connect_last_device result responses
TEST_F(HM11DriverConnectionTest, ConnectLastDeviceResultResponses) {
    // Test "Connecting" response
    {
        uart_mock_->add_command_response("AT+CONNL", "OK+CONNA");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect_last_device(result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::Connecting, result);
    }

    // Test "ConnectError" response
    {
        uart_mock_->add_command_response("AT+CONNL", "OK+CONNE");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect_last_device(result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::ConnectError, result);
    }

    // Test "ConnectFail" response
    {
        uart_mock_->add_command_response("AT+CONNL", "OK+CONNF");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect_last_device(result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::ConnectFail, result);
    }

    // Test "NoAddress" response
    {
        uart_mock_->add_command_response("AT+CONNL", "OK+CONN?");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect_last_device(result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
        EXPECT_EQ(ConnectResult::NoAddress, result);
    }

    // Test unknown response
    {
        uart_mock_->add_command_response("AT+CONNL", "OK+UNKNOWN");
        ConnectResult result;
        hal::UartStatus status;
        driver_->connect_last_device(result, status);
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
        EXPECT_EQ(ConnectResult::OtherError, result);
    }
}

// Test disconnecting from a device
TEST_F(HM11DriverConnectionTest, Disconnect) {
    // Configure the mock to return a successful disconnect response
    uart_mock_->add_command_response("AT", "OK");

    // Call the disconnect method
    hal::UartStatus status = hal::UartStatus::Error; // Start with error to verify it changes
    driver_->disconnect(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test disconnecting with error
TEST_F(HM11DriverConnectionTest, DisconnectWithError) {
    // Configure the mock to return an error status for the disconnect command
    uart_mock_->add_command_response("AT+DISC", "", hal::UartStatus::Error);

    // Call the disconnect method
    hal::UartStatus status = hal::UartStatus::Ok; // Start with OK to verify it changes
    driver_->disconnect(status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test transmission error during connect
TEST_F(HM11DriverConnectionTest, ConnectTransmissionError) {
    // Configure the mock to return a transmission error
    uart_mock_->set_default_status(hal::UartStatus::Error);

    // Call the connect method
    util::MacAddress mac("123456789ABC");
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Ok; // Start with OK to verify it changes
    driver_->connect(mac, result, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test receive error during connect
TEST_F(HM11DriverConnectionTest, ConnectReceiveError) {
    // Configure the mock to return OK for transmission but error for reception
    uart_mock_->set_default_status(hal::UartStatus::Ok);

    // Override the receive handler to return an error
    EXPECT_CALL(
        receive_handler_,
        call(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(
            ::testing::Invoke(
                [](hal::AnyUartPort, void*, std::size_t, hal::UartStatus& status, std::chrono::milliseconds, bool) {
                    status = hal::UartStatus::Error;
                }));

    // Call the connect method
    util::MacAddress mac("123456789ABC");
    ConnectResult result;
    hal::UartStatus status = hal::UartStatus::Ok;
    driver_->connect(mac, result, status);

    // Verify the status and result
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
    EXPECT_EQ(ConnectResult::ConnectError, result);
}

} // namespace hm11::tests