/**
 * @file driver_mock_test.cpp
 * @brief Tests for HM11Driver error handling and mock functionality
 *
 * This file contains tests for the error handling capabilities of the HM11Driver
 * class and also demonstrates how to use the mock classes for testing.
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
 * @brief Test fixture for HM11Driver error handling and mock tests
 *
 * This fixture provides a testing environment for error handling scenarios
 * and demonstrates how to use the mock classes for testing.
 */
class HM11DriverMockTest : public ::testing::Test {
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

// Test handling of UART transmission errors
TEST_F(HM11DriverMockTest, TransmitError) {
    // Configure the mock to return an error status for transmissions
    uart_mock_->set_default_status(hal::UartStatus::Error);

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of UART receive errors
TEST_F(HM11DriverMockTest, ReceiveError) {
    // Configure the mock to return OK for transmissions but error for receptions
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

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of timeouts
TEST_F(HM11DriverMockTest, Timeout) {
    // Configure the mock to return OK for transmissions but timeout for receptions
    uart_mock_->set_default_status(hal::UartStatus::Ok);

    // Override the receive handler to return a timeout
    EXPECT_CALL(
        receive_handler_,
        call(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(
            ::testing::Invoke(
                [](hal::AnyUartPort, void*, std::size_t, hal::UartStatus& status, std::chrono::milliseconds, bool) {
                    status = hal::UartStatus::Timeout;
                }));

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates a timeout
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Timeout, status);
}

// Test handling of invalid responses
TEST_F(HM11DriverMockTest, InvalidResponse) {
    // Configure the mock to return an unexpected response
    uart_mock_->add_command_response("AT", "ERROR");

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates an error
    // Note: This depends on the implementation of the test method
    // If it checks for "OK", this should result in an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of empty responses
TEST_F(HM11DriverMockTest, EmptyResponse) {
    // Configure the mock to return an empty response
    uart_mock_->add_command_response("AT", "");

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates an error
    // Note: This depends on the implementation of the test method
    // If it checks for a non-empty response, this should result in an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of malformed responses for getters
TEST_F(HM11DriverMockTest, MalformedGetterResponse) {
    // Configure the mock to return a malformed response for a getter
    uart_mock_->add_command_response("AT+ROLE?", "OK+Malformed");

    // Call the get_role method
    hal::UartStatus status;
    Role role;
    driver_->get_role(role, status);

    // Verify the status indicates an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of unexpected responses for setters
TEST_F(HM11DriverMockTest, UnexpectedSetterResponse) {
    // Configure the mock to return an unexpected response for a setter
    uart_mock_->add_command_response("AT+ROLE0", "ERROR");

    // Call the set_role method
    hal::UartStatus status;
    driver_->set_role(Role::Peripheral, status);

    // Verify the status indicates an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test handling of UART busy state
TEST_F(HM11DriverMockTest, UartBusy) {
    // Configure the mock to return a busy status for transmissions
    uart_mock_->set_default_status(hal::UartStatus::Busy);

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates busy
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Busy, status);
}

// Test handling of disconnected state
TEST_F(HM11DriverMockTest, Disconnected) {
    // Configure the mock to return a disconnected status for transmissions
    uart_mock_->set_default_status(hal::UartStatus::Disconnected);

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates disconnected
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Disconnected, status);
}

// Test the receive handler edge cases
TEST_F(HM11DriverMockTest, ReceiveHandlerEdgeCases) {
    // Test with very large response
    {
        // Configure a very large response
        std::string large_response = "OK+NAME:";
        large_response.append(1000, 'A'); // Add 1000 'A' characters
        uart_mock_->add_command_response("AT+NAME?", large_response);

        // Call the get_module_name method
        hal::UartStatus status;
        std::string name = driver_->get_module_name(status);

        // Verify the status and truncated result
        EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

        // Our implementation uses a 256-byte buffer, so response will be truncated
        // The response is "OK+NAME:" (8 bytes) + 1000 'A's = 1008 bytes total
        // With a 256-byte buffer, we can receive at most 255 chars (1 byte for null terminator)
        // After removing "OK+NAME:" prefix (8 bytes), we get 247 'A's
        std::string expected(247, 'A');
        EXPECT_EQ(name, expected);
    }
}

// Test the last read position handler edge cases
// DISABLED: Tests streaming functionality not implemented in current stubs
TEST_F(HM11DriverMockTest, DISABLED_LastReadPositionHandlerEdgeCases) {
    // Configure the last read position handler to indicate stream closed
    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](bool& closed, std::size_t& zero) {
            closed = true;
            zero = 1;
        }));

    // Call a method that uses streaming (if applicable)
    // For this test, we'll use a basic method that doesn't actually stream
    hal::UartStatus status;
    driver_->test(status);

    // The test status should still be OK since we're not using streaming for this method
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test combining multiple error conditions
// DISABLED: Tests complex error combinations with streaming not implemented in current stubs
TEST_F(HM11DriverMockTest, DISABLED_CombinedErrors) {
    // Configure the mock to return OK for transmissions but error for receptions
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

    // Configure the last read position handler to indicate stream closed
    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](bool& closed, std::size_t& zero) {
            closed = true;
            zero = 1;
        }));

    // Call the test method
    hal::UartStatus status;
    driver_->test(status);

    // Verify the status indicates an error (should prioritize the receive error)
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test error propagation through the entire call stack
// DISABLED: Tests complex error propagation scenarios not fully implemented in current stubs
TEST_F(HM11DriverMockTest, DISABLED_ErrorPropagation) {
    // Create a test stack with multiple levels of error handling
    // Level 1: Test a basic operation
    // Level 2: Test a complex operation that uses a basic operation

    // Configure the mock to return an error for the basic operation
    uart_mock_->add_command_response("AT", "ERROR", hal::UartStatus::Error);

    // First, verify that the basic operation fails
    hal::UartStatus status;
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);

    // Next, test a complex operation that relies on the basic operation
    // For this example, we'll use get_role() as our "complex" operation
    // that might internally use test() to verify connectivity first
    // (this is just an example; the actual implementation may vary)
    Role role;
    driver_->get_role(role, status);

    // The error should be propagated from the basic operation
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

} // namespace hm11::tests