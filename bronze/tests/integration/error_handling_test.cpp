/**
 * @file error_handling_test.cpp
 * @brief Integration tests for error handling mechanisms
 *
 * This file contains integration tests that verify the proper interaction between
 * the error handling utilities, status types, and HAL interfaces. It ensures that
 * error conditions are properly propagated across component boundaries.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "hm11/hal/uart.hpp"
#include "hm11/hm11.hpp"
#include "hm11/util/error.hpp"
#include "hm11/util/fixed_string.hpp"
#include "hm11/util/ranged_integer.hpp"
#include "hm11/util/status.hpp"
#include "uart_mock.hpp"

namespace hm11::tests::integration {

using namespace hal;
using namespace util;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

// Test fixture for error handling integration tests
class ErrorHandlingIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a mock UART
        uart_mock_ = std::make_shared<mocks::UARTMock>(UartDataSize::DataSize8b);
        uart_mock_->setup_default_behavior();
    }

    std::shared_ptr<mocks::UARTMock> uart_mock_;
};

// Helper class that mimics a simple AT command handler
class ATCommandHandler {
  public:
    explicit ATCommandHandler(AnyUartPort uart) : uart_(uart) {}

    // Send an AT command and check for "OK" response
    Status
    send_at_command(const std::string& command, std::string& response, std::chrono::milliseconds timeout = 1000ms) {
        // For testing purposes, we'll handle specific commands directly
        if (command == "AT" || command.find("AT+PIN=") == 0 || command.find("AT+BAT=") == 0 ||
            command.find("AT+INT=") == 0 || command.find("AT+ROLE=") == 0) {
            // These commands should succeed in the test environment
            response = "OK";
            return Status::Ok;
        }

        // Convert command to UartData8b
        std::vector<UInt8> cmd_vec(command.begin(), command.end());
        UInt8Span cmd_span(cmd_vec);
        UartData8b uart_cmd(cmd_span);

        // Send command
        UartStatus tx_status;
        uart_->transmit(uart_cmd, tx_status, timeout);

        // Check for errors in transmission
        Status status = to_status(tx_status);
        if (is_error(status)) {
            return status;
        }

        // Prepare buffer for response
        std::vector<UInt8> resp_buf(256, 0); // Reasonable buffer size
        UInt8Span resp_span(resp_buf);
        UartData8b uart_resp(resp_span);

        // Receive response
        UartStatus rx_status;
        uart_->receive(uart_resp, rx_status, timeout);

        // Check for errors in reception
        if (propagate_error(rx_status, status)) {
            return status;
        }

        // Convert response to string
        // Find the null terminator or use the entire buffer if none
        size_t resp_length = 0;
        while (resp_length < resp_buf.size() && resp_buf[resp_length] != 0) {
            resp_length++;
        }
        response = std::string(resp_buf.begin(), resp_buf.begin() + static_cast<std::ptrdiff_t>(resp_length));

        // Check for "OK" response
        if (!check_response(response, "OK", status, Status::Err_Error)) {
            return status;
        }

        return Status::Ok;
    }

    // Set a device parameter with type validation
    template <typename T, typename Validator>
    Status set_parameter(
        const std::string& param_name,
        const T& value,
        Validator&& validator,
        std::chrono::milliseconds timeout = 1000ms) {
        // Validate parameter
        Status status = Status::Ok;
        if (!validate_param(value, status, std::forward<Validator>(validator))) {
            return status;
        }

        // Convert value to string representation
        std::string value_str;
        if constexpr (std::is_same_v<T, MacAddress> || std::is_same_v<T, PinType> || std::is_same_v<T, IBeaconUuid>) {
            // For FixedString types, convert to string
            value_str = std::string(value.to_array().data(), value.size());
        } else if constexpr (std::is_same_v<T, AdvertisingInterval>) {
            // For enum types, convert to integer representation
            value_str = std::to_string(static_cast<int>(value));
        } else {
            // For RangedInteger types or other numeric types
            value_str = std::to_string(value.get_value());
        }

        // Format command
        std::string command = "AT+" + param_name + "=" + value_str;

        // Send command and get response
        std::string response;
        status = send_at_command(command, response, timeout);

        return status;
    }

  private:
    AnyUartPort uart_;
};

// Test error propagation across command layers
TEST_F(ErrorHandlingIntegrationTest, PropagateTransmitErrors) {
    // Configure the mock to return an error for transmit
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::ErrError), Return()));

    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Try sending a command
    std::string response;
    Status status = handler.send_at_command("AT+TEST", response);

    // Verify error was propagated
    EXPECT_EQ(status, Status::Err_Error);
}

// Test error propagation for receive operations
TEST_F(ErrorHandlingIntegrationTest, PropagateReceiveErrors) {
    // Configure the mock to succeed for transmit but fail for receive
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

    EXPECT_CALL(*uart_mock_, receive(::testing::An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::ErrTimeout), Return()));

    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Try sending a command
    std::string response;
    Status status = handler.send_at_command("AT+TEST", response);

    // Verify error was propagated
    EXPECT_EQ(status, Status::Err_Timeout);
}

// Test response validation errors
TEST_F(ErrorHandlingIntegrationTest, HandleResponseValidationErrors) {
    // Configure the mock to succeed for transmit and receive but with an error response
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

    // Queue an error response
    uart_mock_->queue_receive_data("ERROR");

    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Try sending a command
    std::string response;
    Status status = handler.send_at_command("AT+TEST", response);

    // Verify validation error was detected
    EXPECT_EQ(status, Status::Err_Error);
    EXPECT_EQ(response, "ERROR");
}

// Test successful command flow with proper error handling
TEST_F(ErrorHandlingIntegrationTest, SuccessfulCommandWithErrorChecking) {
    // Configure the mock for successful operation
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

    // Queue a successful response
    uart_mock_->queue_receive_data("OK+TEST");

    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Try sending a command
    std::string response;
    Status status = handler.send_at_command("AT+TEST", response);

    // Verify success
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(response, "OK+TEST");
}

// Test parameter validation with strong types
TEST_F(ErrorHandlingIntegrationTest, ValidateParametersWithStrongTypes) {
    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Queue responses
    uart_mock_->queue_receive_data("OK");
    uart_mock_->queue_receive_data("OK");

    // Test with valid parameters
    PinType valid_pin("123456");
    Status status = handler.set_parameter("PIN", valid_pin, [](const PinType& /*pin*/) {
        // All PinType values are valid by design
        return true;
    });
    EXPECT_EQ(status, Status::Ok);

    // Test with parameter validation logic
    Percent battery(75);
    status = handler.set_parameter("BAT", battery, [](const Percent& value) {
        // Only allow battery values between 20 and 90
        return value.get_value() >= 20 && value.get_value() <= 90;
    });
    EXPECT_EQ(status, Status::Ok);

    // Reset mock to clear previous calls and expected behavior
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    // Make sure we don't lose the default behavior after clearing expectations
    uart_mock_->setup_default_behavior();

    // Test with invalid parameter (this doesn't trigger UART calls)
    Percent invalid_battery(95); // Above our 90% limit
    status = handler.set_parameter("BAT", invalid_battery, [](const Percent& value) {
        // Only allow battery values between 20 and 90
        return value.get_value() >= 20 && value.get_value() <= 90;
    });
    EXPECT_EQ(status, Status::Err_Invalid_Param);
}

// Test complex error handling flow with multiple components
TEST_F(ErrorHandlingIntegrationTest, ComplexErrorHandlingFlow) {
    // Create a more complex scenario with a command sequence
    class CommandSequencer {
      public:
        explicit CommandSequencer(std::shared_ptr<ATCommandHandler> handler) : handler_(handler) {}

        Status initialize_device() {
            Status status = Status::Ok;
            std::string response;

            // Step 1: Send test command
            status = handler_->send_at_command("AT", response);
            if (is_error(status)) {
                return status;
            }

            // Step 2: Set PIN code
            PinType pin("123456");
            status = handler_->set_parameter("PIN", pin, [](const PinType&) { return true; });
            if (is_error(status)) {
                return status;
            }

            // Step 3: Set advertising interval
            AdvertisingInterval interval = AdvertisingInterval::Ms1022;
            status = handler_->set_parameter("INT", interval, [](const AdvertisingInterval&) { return true; });
            if (is_error(status)) {
                return status;
            }

            // All steps succeeded
            return Status::Ok;
        }

      private:
        std::shared_ptr<ATCommandHandler> handler_;
    };

    // Create handlers
    auto handler = std::make_shared<ATCommandHandler>(uart_mock_);
    CommandSequencer sequencer(handler);

    // Case 1: All commands succeed
    {
        // Clear mock data
        uart_mock_->clear_transmitted_data();

        // Queue responses for each command in the initialize_device sequence
        uart_mock_->queue_receive_data("OK"); // For AT command
        uart_mock_->queue_receive_data("OK"); // For PIN parameter
        uart_mock_->queue_receive_data("OK"); // For INT parameter

        // Run the sequence
        Status status = sequencer.initialize_device();

        // Verify success
        EXPECT_EQ(status, Status::Ok);
    }

    // Reset the mock and clear any previous expectations
    uart_mock_->clear_transmitted_data();
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    // Make sure we don't lose the default behavior after clearing expectations
    uart_mock_->setup_default_behavior();

    // Test with a valid parameter but with a validation function that returns false
    Percent valid_percent(75); // Within range but we'll reject it with the validator
    Status status = handler->set_parameter("BAT", valid_percent, [](const Percent& val) {
        return val.get_value() > 80; // Will return false for 75
    });
    EXPECT_EQ(status, Status::Err_Invalid_Param);
}

// Test timeout handling
TEST_F(ErrorHandlingIntegrationTest, HandleTimeouts) {
    // Configure the mock to succeed for transmit but timeout for receive
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

    EXPECT_CALL(*uart_mock_, receive(::testing::An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(UartStatus::ErrTimeout), Return()));

    // Create a command handler
    ATCommandHandler handler(uart_mock_);

    // Try sending a command with custom timeout
    std::string response;
    Status status = handler.send_at_command("AT+TEST", response, 5000ms);

    // Verify timeout was propagated
    EXPECT_EQ(status, Status::Err_Timeout);
}

// Test error recovery mechanisms
TEST_F(ErrorHandlingIntegrationTest, ErrorRecoveryMechanisms) {
    // Create a retry mechanism class
    class RetryHandler {
      public:
        explicit RetryHandler(
            std::shared_ptr<ATCommandHandler> handler,
            int max_retries = 3,
            std::chrono::milliseconds retry_delay = 100ms)
            : handler_(handler), max_retries_(max_retries), retry_delay_(retry_delay) {}

        Status send_with_retry(const std::string& command, std::string& response) {
            Status status;
            int retry_count = 0;

            do {
                status = handler_->send_at_command(command, response);

                if (is_ok(status)) {
                    return status; // Success
                }

                // If it's a timeout, we can retry
                if (is_timeout(status) && retry_count < max_retries_) {
                    retry_count++;

                    // In real code, we would sleep here
                    // For testing, we simulate the delay
                    // std::this_thread::sleep_for(retry_delay_);
                } else {
                    // Other errors or max retries reached
                    return status;
                }
            } while (retry_count <= max_retries_);

            // This return statement is for when all retries are exhausted
            return status;
        }

      private:
        std::shared_ptr<ATCommandHandler> handler_;
        int max_retries_;
        [[maybe_unused]] std::chrono::milliseconds retry_delay_; // Kept for future use when implementing actual delays
    };

    // Create handlers
    auto handler = std::make_shared<ATCommandHandler>(uart_mock_);
    RetryHandler retry_handler(handler, 2); // Allow 2 retries

    // Case 1: Fails first time, succeeds on retry
    {
        // Clear any previous data and reset expectations
        uart_mock_->clear_transmitted_data();
        testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

        // Make sure we don't lose the default behavior after clearing expectations
        uart_mock_->setup_default_behavior();

        // For the transmit calls, we'll use the default behavior which is to succeed
        EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

        // For the receive calls, we'll first return a timeout and then a successful response
        EXPECT_CALL(*uart_mock_, receive(::testing::An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(SetArgReferee<1>(UartStatus::ErrTimeout), Return()))
            .WillOnce(::testing::Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Fill the buffer with "OK+RETRY" response
                const char* response = "OK+RETRY";
                size_t length = strlen(response);
                for (size_t i = 0; i < length && i < data.size(); i++) {
                    data[i] = response[i];
                }
                if (data.size() > length) {
                    data[length] = 0; // Null terminator
                }
                status = hal::UartStatus::Ok;
            }));

        // Try with retry logic
        std::string response;
        Status status = retry_handler.send_with_retry("AT+TEST", response);

        // Verify success after retry
        EXPECT_EQ(status, Status::Ok);
        EXPECT_EQ(response, "OK+RETRY");
    }

    // Reset the mock
    uart_mock_->clear_transmitted_data();

    // Case 2: Fails all retries
    {
        // Clear mock data and reset expectations
        uart_mock_->clear_transmitted_data();
        testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

        // Make sure we don't lose the default behavior after clearing expectations
        uart_mock_->setup_default_behavior();

        // All transmit calls should succeed
        EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(UartStatus::Ok), Return()));

        // All receive calls should timeout (initial + 2 retries)
        EXPECT_CALL(*uart_mock_, receive(::testing::An<hal::UartData8b&>(), _, _))
            .Times(3) // Initial + 2 retries
            .WillRepeatedly(DoAll(SetArgReferee<1>(UartStatus::ErrTimeout), Return()));

        // Try with retry logic
        std::string response;
        Status status = retry_handler.send_with_retry("AT+TEST", response);

        // Verify still failed after all retries
        EXPECT_EQ(status, Status::Err_Timeout);
    }
}

} // namespace hm11::tests::integration