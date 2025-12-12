/**
 * @file basic_driver_test.cpp
 * @brief Integration tests for a simplified HM11 driver
 *
 * This file contains integration tests that simulate a simplified version
 * of the HM11 driver, integrating the HAL interfaces, strong types,
 * memory alignment, and error handling mechanisms.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "hm11/hal/uart.hpp"
#include "hm11/hm11.hpp"
#include "hm11/platform/alignment.hpp"
#include "hm11/util/error.hpp"
#include "hm11/util/fixed_string.hpp"
#include "hm11/util/ranged_integer.hpp"
#include "hm11/util/status.hpp"
#include "uart_mock.hpp"

namespace hm11::tests::integration {

using namespace hal;
using namespace platform;
using namespace util;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgReferee;

// Define a simplified version of the HM11 driver that integrates all components
class SimpleHM11Driver {
  public:
    // Role type (enum class)
    enum class Role { Peripheral, Central };

    // Constructor
    explicit SimpleHM11Driver(AnyUartPort uart) : uart_(uart), receive_buffer_() {}

    // Test connection
    Status test(std::chrono::milliseconds timeout = 1000ms) { return send_command("AT", "OK", timeout); }

    // Set role
    Status set_role(Role role, std::chrono::milliseconds timeout = 1000ms) {
        std::string cmd;
        switch (role) {
            case Role::Peripheral:
                cmd = "AT+ROLE=0";
                break;
            case Role::Central:
                cmd = "AT+ROLE=1";
                break;
        }

        Status status = send_command(cmd, "OK", timeout);
        if (is_ok(status)) {
            current_role_ = role;
        }
        return status;
    }

    // Get role
    Status get_role(Role& role, std::chrono::milliseconds timeout = 1000ms) {
        std::string response;
        Status status = send_command("AT+ROLE?", response, timeout);
        if (is_error(status)) {
            return status;
        }

        // Parse response
        if (response.find("OK+Get:0") != std::string::npos) {
            role = Role::Peripheral;
            current_role_ = role;
            return Status::Ok;
        } else if (response.find("OK+Get:1") != std::string::npos) {
            role = Role::Central;
            current_role_ = role;
            return Status::Ok;
        } else {
            return Status::Err_Parse_Error;
        }
    }

    // Set device name
    Status set_name(const std::string& name, std::chrono::milliseconds timeout = 1000ms) {
        if (name.empty() || name.size() > 12) {
            return Status::Err_Invalid_Param;
        }

        std::string cmd = "AT+NAME=" + name;
        return send_command(cmd, "OK", timeout);
    }

    // Get device name
    Status get_name(std::string& name, std::chrono::milliseconds timeout = 1000ms) {
        std::string response;
        Status status = send_command("AT+NAME?", response, timeout);
        if (is_error(status)) {
            return status;
        }

        // Parse response
        const std::string prefix = "OK+NAME:";
        size_t pos = response.find(prefix);
        if (pos != std::string::npos) {
            name = response.substr(pos + prefix.size());
            return Status::Ok;
        } else {
            return Status::Err_Parse_Error;
        }
    }

    // Set PIN code
    Status set_pin(const PinType& pin, std::chrono::milliseconds timeout = 1000ms) {
        std::string cmd = "AT+PIN=" + std::string(pin.to_array().data(), pin.size());
        return send_command(cmd, "OK", timeout);
    }

    // Get MAC address
    Status get_mac_address(MacAddress& mac, std::chrono::milliseconds timeout = 1000ms) {
        std::string response;
        Status status = send_command("AT+MAC?", response, timeout);
        if (is_error(status)) {
            return status;
        }

        // Parse response
        const std::string prefix = "OK+MAC:";
        size_t pos = response.find(prefix);
        if (pos != std::string::npos && response.size() >= pos + prefix.size() + 12) {
            std::string mac_str = response.substr(pos + prefix.size(), 12);
            mac = MacAddress(mac_str);
            return Status::Ok;
        } else {
            return Status::Err_Parse_Error;
        }
    }

    // Set advertising interval
    Status set_advertising_interval(AdvertisingInterval interval, std::chrono::milliseconds timeout = 1000ms) {
        int interval_value = static_cast<int>(interval);
        std::string cmd = "AT+INT=" + std::to_string(interval_value);
        return send_command(cmd, "OK", timeout);
    }

    // Set battery level
    Status set_battery(Percent level, std::chrono::milliseconds timeout = 1000ms) {
        std::string cmd = "AT+BAT=" + std::to_string(level.get_value());
        return send_command(cmd, "OK", timeout);
    }

    // Get battery level
    Status get_battery(Percent& level, std::chrono::milliseconds timeout = 1000ms) {
        std::string response;
        Status status = send_command("AT+BAT?", response, timeout);
        if (is_error(status)) {
            return status;
        }

        // Parse response
        const std::string prefix = "OK+BAT:";
        size_t pos = response.find(prefix);
        if (pos != std::string::npos) {
            std::string level_str = response.substr(pos + prefix.size());
            try {
                int value = std::stoi(level_str);
                level = Percent(static_cast<unsigned char>(value));
                return Status::Ok;
            } catch (const std::exception&) {
                return Status::Err_Parse_Error;
            }
        } else {
            return Status::Err_Parse_Error;
        }
    }

    // Set PIO output
    Status set_pio_output(PioNumber pio, bool high_level, std::chrono::milliseconds timeout = 1000ms) {
        std::string cmd = "AT+PIO" + std::to_string(pio.get_value()) + "=" + (high_level ? "1" : "0");
        return send_command(cmd, "OK", timeout);
    }

    // Reset device
    Status reset(std::chrono::milliseconds timeout = 1000ms) { return send_command("AT+RESET", "OK", timeout); }

  private:
    // Send command and expect a specific response prefix
    Status send_command(
        const std::string& command,
        const std::string& expected_prefix,
        std::chrono::milliseconds timeout = 1000ms) {
        std::string response;
        Status status = send_command(command, response, timeout);
        if (is_error(status)) {
            return status;
        }

        if (!check_response(response, expected_prefix, status, Status::Err_Error)) {
            return status;
        }

        return Status::Ok;
    }

    // Send command and get response
    Status send_command(const std::string& command, std::string& response, std::chrono::milliseconds timeout = 1000ms) {
        // Convert command to UART data
        std::vector<UInt8> cmd_vec(command.begin(), command.end());
        UInt8Span cmd_span(cmd_vec);
        UartData8b uart_cmd(cmd_span);

        // Send command
        UartStatus tx_status;
        uart_->transmit(uart_cmd, tx_status, timeout);

        // Check for transmission errors
        Status status = to_status(tx_status);
        if (is_error(status)) {
            return status;
        }

        // Clear receive buffer
        receive_buffer_.clear();

        // Prepare to receive
        std::vector<UInt8> buffer_vec(256, 0); // Use 256 as the buffer size
        UInt8Span buffer_span(buffer_vec);
        UartData8b uart_buffer(buffer_span);

        // Receive response
        UartStatus rx_status;
        uart_->receive(uart_buffer, rx_status, timeout);

        // Check for reception errors
        if (propagate_error(rx_status, status)) {
            return status;
        }

        // Convert response to string
        // Find the null terminator or use the entire buffer if none
        size_t resp_length = 0;
        while (resp_length < buffer_vec.size() && buffer_vec[resp_length] != 0) {
            resp_length++;
        }
        response = std::string(buffer_vec.begin(), buffer_vec.begin() + static_cast<std::ptrdiff_t>(resp_length));

        return Status::Ok;
    }

  private:
    AnyUartPort uart_;
    DmaAlignedBuffer<UInt8, 256> receive_buffer_; // Aligned buffer for DMA operations
    Role current_role_ = Role::Peripheral;
};

// Test fixture for basic driver integration tests
class BasicDriverIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a mock UART
        uart_mock_ = std::make_shared<mocks::UARTMock>(UartDataSize::DataSize8b);
        uart_mock_->setup_default_behavior();

        // Create a simple HM11 driver
        driver_ = std::make_unique<SimpleHM11Driver>(uart_mock_);
    }

    std::shared_ptr<mocks::UARTMock> uart_mock_;
    std::unique_ptr<SimpleHM11Driver> driver_;
};

// Test basic connectivity
TEST_F(BasicDriverIntegrationTest, TestConnectivity) {
    // Queue response
    uart_mock_->queue_receive_data("OK");

    // Test connectivity
    util::Status status = driver_->test();

    // Verify success
    EXPECT_EQ(status, util::Status::Ok);

    // Verify the correct command was sent
    ASSERT_GE(uart_mock_->transmitted_data_8b().size(), 2);
    EXPECT_EQ(uart_mock_->transmitted_data_8b()[0], 'A');
    EXPECT_EQ(uart_mock_->transmitted_data_8b()[1], 'T');
}

// Test setting and getting role
TEST_F(BasicDriverIntegrationTest, SetAndGetRole) {
    // Clear any previous mock data
    uart_mock_->clear_transmitted_data();
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    using ::testing::_;
    using ::testing::DoAll;
    using ::testing::Invoke;
    using ::testing::Return;
    using ::testing::SetArgReferee;

    // Set expectations for transmit and receive
    EXPECT_CALL(*uart_mock_, transmit(An<const hal::UartData8b&>(), _, _))
        .WillRepeatedly(DoAll(
            Invoke([this](const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                auto& transmitted = uart_mock_->transmitted_data_8b();
                std::vector<hal::UInt8>& non_const_transmitted = const_cast<std::vector<hal::UInt8>&>(transmitted);
                non_const_transmitted.insert(non_const_transmitted.end(), data.begin(), data.end());
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok),
            Return()));

    // Queue responses for set and get operations
    // Instead of queuing data, we'll use the EXPECT_CALL pattern to control responses

    // First call should be for "AT+ROLE=1" command (set_role)
    EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(
            Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK" for the set_role command
                std::string response = "OK";
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)))
        // Second call should be for "AT+ROLE?" command (get_role)
        .WillOnce(DoAll(
            Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK+Get:1" for the get_role command
                std::string response = "OK+Get:1";
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)));

    // Set role to Central
    util::Status status = driver_->set_role(SimpleHM11Driver::Role::Central);
    EXPECT_EQ(status, util::Status::Ok);

    // Get role
    SimpleHM11Driver::Role role;
    status = driver_->get_role(role);
    EXPECT_EQ(status, util::Status::Ok);
    EXPECT_EQ(role, SimpleHM11Driver::Role::Central);

    // Get the transmitted data
    auto& transmitted = uart_mock_->transmitted_data_8b();

    // Verify that some data was transmitted
    ASSERT_GT(transmitted.size(), 0);

    // Look for the set_role command pattern in the transmitted data
    std::string set_cmd = "AT+ROLE=1";
    bool set_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - set_cmd.size(); ++i) {
        if (std::equal(set_cmd.begin(), set_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            set_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(set_cmd_found) << "Failed to find set command in transmitted data";

    // Look for the get_role command pattern in the transmitted data
    std::string get_cmd = "AT+ROLE?";
    bool get_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - get_cmd.size(); ++i) {
        if (std::equal(get_cmd.begin(), get_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            get_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(get_cmd_found) << "Failed to find get command in transmitted data";
}

// Test setting and getting device name
TEST_F(BasicDriverIntegrationTest, SetAndGetName) {
    // Clear any previous mock data
    uart_mock_->clear_transmitted_data();
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    using ::testing::_;
    using ::testing::DoAll;
    using ::testing::Invoke;
    using ::testing::Return;
    using ::testing::SetArgReferee;

    // Set expectations for transmit and receive
    EXPECT_CALL(*uart_mock_, transmit(An<const hal::UartData8b&>(), _, _))
        .WillRepeatedly(DoAll(
            Invoke([this](const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                auto& transmitted = uart_mock_->transmitted_data_8b();
                std::vector<hal::UInt8>& non_const_transmitted = const_cast<std::vector<hal::UInt8>&>(transmitted);
                non_const_transmitted.insert(non_const_transmitted.end(), data.begin(), data.end());
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok),
            Return()));

    // Set name
    std::string test_name = "HM11Test";

    // Configure receive behavior for set_name and get_name
    EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(
            Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK" for the set_name command
                std::string response = "OK";
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)))
        .WillOnce(DoAll(
            Invoke([test_name](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK+NAME:HM11Test" for the get_name command
                std::string response = "OK+NAME:" + test_name;
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)));

    // Set name
    util::Status status = driver_->set_name(test_name);
    EXPECT_EQ(status, util::Status::Ok);

    // Get name
    std::string returned_name;
    status = driver_->get_name(returned_name);
    EXPECT_EQ(status, util::Status::Ok);
    EXPECT_EQ(returned_name, test_name);

    // Get the transmitted data
    auto& transmitted = uart_mock_->transmitted_data_8b();

    // Verify that some data was transmitted
    ASSERT_GT(transmitted.size(), 0);

    // Look for the set_name command pattern in the transmitted data
    std::string set_cmd = "AT+NAME=HM11Test";
    bool set_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - set_cmd.size(); ++i) {
        if (std::equal(set_cmd.begin(), set_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            set_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(set_cmd_found) << "Failed to find set command in transmitted data";

    // Look for the get_name command pattern in the transmitted data
    std::string get_cmd = "AT+NAME?";
    bool get_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - get_cmd.size(); ++i) {
        if (std::equal(get_cmd.begin(), get_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            get_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(get_cmd_found) << "Failed to find get command in transmitted data";
}

// Test setting PIN code with strong type
TEST_F(BasicDriverIntegrationTest, SetPinWithStrongType) {
    // Queue response
    uart_mock_->queue_receive_data("OK");

    // Set PIN using PinType
    util::PinType pin("123456");
    util::Status status = driver_->set_pin(pin);
    EXPECT_EQ(status, util::Status::Ok);

    // Verify the command was sent correctly
    std::string expected_cmd = "AT+PIN=123456";
    auto& transmitted = uart_mock_->transmitted_data_8b();

    ASSERT_GE(transmitted.size(), expected_cmd.size());
    for (size_t i = 0; i < expected_cmd.size(); ++i) {
        EXPECT_EQ(transmitted[i], expected_cmd[i]);
    }
}

// Test getting MAC address with strong type
TEST_F(BasicDriverIntegrationTest, GetMacAddress) {
    // Queue response
    uart_mock_->queue_receive_data("OK+MAC:112233445566");

    // Get MAC address
    util::MacAddress mac;
    util::Status status = driver_->get_mac_address(mac);
    EXPECT_EQ(status, util::Status::Ok);
    EXPECT_EQ(mac, "112233445566");
}

// Test setting advertising interval with enum
TEST_F(BasicDriverIntegrationTest, SetAdvertisingInterval) {
    // Queue response
    uart_mock_->queue_receive_data("OK");

    // Set advertising interval
    util::Status status = driver_->set_advertising_interval(hm11::AdvertisingInterval::Ms1022);
    EXPECT_EQ(status, util::Status::Ok);

    // Verify the command was sent correctly
    std::string expected_cmd = "AT+INT=8"; // Value 8 corresponds to ms_1022
    auto& transmitted = uart_mock_->transmitted_data_8b();

    ASSERT_GE(transmitted.size(), expected_cmd.size());
    for (size_t i = 0; i < expected_cmd.size(); ++i) {
        EXPECT_EQ(transmitted[i], expected_cmd[i]);
    }
}

// Test setting and getting battery level with RangedInteger
TEST_F(BasicDriverIntegrationTest, SetAndGetBatteryLevel) {
    // Clear any previous mock data
    uart_mock_->clear_transmitted_data();
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    using ::testing::_;
    using ::testing::DoAll;
    using ::testing::Invoke;
    using ::testing::Return;
    using ::testing::SetArgReferee;

    // Set expectations for transmit and receive
    EXPECT_CALL(*uart_mock_, transmit(An<const hal::UartData8b&>(), _, _))
        .WillRepeatedly(DoAll(
            Invoke([this](const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                auto& transmitted = uart_mock_->transmitted_data_8b();
                std::vector<hal::UInt8>& non_const_transmitted = const_cast<std::vector<hal::UInt8>&>(transmitted);
                non_const_transmitted.insert(non_const_transmitted.end(), data.begin(), data.end());
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok),
            Return()));

    // Set battery value
    const int battery_value = 75;

    // Configure receive behavior for set_battery and get_battery
    EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(
            Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK" for the set_battery command
                std::string response = "OK";
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)))
        .WillOnce(DoAll(
            Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Return "OK+BAT:75" for the get_battery command
                std::string response = "OK+BAT:75";
                std::copy(response.begin(), response.end(), data.begin());
                data[response.size()] = 0; // Null terminate
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok)));

    // Set battery level
    util::Percent battery(battery_value);
    util::Status status = driver_->set_battery(battery);
    EXPECT_EQ(status, util::Status::Ok);

    // Get battery level
    util::Percent returned_battery;
    status = driver_->get_battery(returned_battery);
    EXPECT_EQ(status, util::Status::Ok);
    EXPECT_EQ(returned_battery.get_value(), battery_value);

    // Get the transmitted data
    auto& transmitted = uart_mock_->transmitted_data_8b();

    // Verify that some data was transmitted
    ASSERT_GT(transmitted.size(), 0);

    // Look for the set_battery command pattern in the transmitted data
    std::string set_cmd = "AT+BAT=75";
    bool set_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - set_cmd.size(); ++i) {
        if (std::equal(set_cmd.begin(), set_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            set_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(set_cmd_found) << "Failed to find set command in transmitted data";

    // Look for the get_battery command pattern in the transmitted data
    std::string get_cmd = "AT+BAT?";
    bool get_cmd_found = false;
    for (size_t i = 0; i <= transmitted.size() - get_cmd.size(); ++i) {
        if (std::equal(get_cmd.begin(), get_cmd.end(), transmitted.begin() + static_cast<std::ptrdiff_t>(i))) {
            get_cmd_found = true;
            break;
        }
    }
    EXPECT_TRUE(get_cmd_found) << "Failed to find get command in transmitted data";
}

// Test setting PIO output with RangedInteger
TEST_F(BasicDriverIntegrationTest, SetPioOutput) {
    // Queue response
    uart_mock_->queue_receive_data("OK");

    // Set PIO output
    util::PioNumber pio(2);
    util::Status status = driver_->set_pio_output(pio, true);
    EXPECT_EQ(status, util::Status::Ok);

    // Verify the command was sent correctly
    std::string expected_cmd = "AT+PIO2=1";
    auto& transmitted = uart_mock_->transmitted_data_8b();

    ASSERT_GE(transmitted.size(), expected_cmd.size());
    for (size_t i = 0; i < expected_cmd.size(); ++i) {
        EXPECT_EQ(transmitted[i], expected_cmd[i]);
    }
}

// Test handling errors
TEST_F(BasicDriverIntegrationTest, HandleErrors) {
    // Test error in transmission
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(hal::UartStatus::ErrError), Return()));

    // Test connectivity
    util::Status status = driver_->test();

    // Verify error was propagated
    EXPECT_EQ(status, util::Status::Err_Error);

    // Reset mock behavior
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    // Make sure we don't lose the default behavior after clearing expectations
    uart_mock_->setup_default_behavior();

    // Test error in reception
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(hal::UartStatus::Ok), Return()));

    EXPECT_CALL(*uart_mock_, receive(::testing::An<hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(hal::UartStatus::ErrTimeout), Return()));

    // Test connectivity again
    status = driver_->test();

    // Verify timeout was propagated
    EXPECT_EQ(status, util::Status::Err_Timeout);

    // Reset mock behavior
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    // Make sure we don't lose the default behavior after clearing expectations
    uart_mock_->setup_default_behavior();

    // Queue an ERROR response
    uart_mock_->queue_receive_data("ERROR");

    // Make sure transmit succeeds
    EXPECT_CALL(*uart_mock_, transmit(::testing::An<const hal::UartData8b&>(), _, _))
        .WillOnce(DoAll(SetArgReferee<1>(hal::UartStatus::Ok), Return()));

    // Test connectivity again
    status = driver_->test();

    // Verify error was detected
    EXPECT_EQ(status, util::Status::Err_Error);
}

// Test handling invalid parameters
TEST_F(BasicDriverIntegrationTest, HandleInvalidParameters) {
    // Test invalid name (too long)
    std::string long_name = "ThisNameIsTooLongForHM11";
    util::Status status = driver_->set_name(long_name);

    // Verify error without sending command
    EXPECT_EQ(status, util::Status::Err_Invalid_Param);
    EXPECT_EQ(uart_mock_->transmitted_data_8b().size(), 0);
}

// Test a sequence of commands simulating a typical usage scenario
TEST_F(BasicDriverIntegrationTest, CompleteUsageScenario) {
    // Clear any previous mock data
    uart_mock_->clear_transmitted_data();
    testing::Mock::VerifyAndClearExpectations(uart_mock_.get());

    using ::testing::_;
    using ::testing::DoAll;
    using ::testing::Invoke;
    using ::testing::Return;
    using ::testing::SetArgReferee;

    // Set expectations for transmit and receive
    EXPECT_CALL(*uart_mock_, transmit(An<const hal::UartData8b&>(), _, _))
        .WillRepeatedly(DoAll(
            Invoke([this](const hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                // Store the transmitted data
                auto& transmitted = uart_mock_->transmitted_data_8b();
                std::vector<hal::UInt8>& non_const_transmitted = const_cast<std::vector<hal::UInt8>&>(transmitted);
                non_const_transmitted.insert(non_const_transmitted.end(), data.begin(), data.end());
                status = hal::UartStatus::Ok;
            }),
            SetArgReferee<1>(hal::UartStatus::Ok),
            Return()));

    // Set up expectations for the complete sequence
    {
        ::testing::InSequence seq;

        // Step 1: Test connectivity (AT)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 2: Set device name (AT+NAME=HM11Device)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 3: Set PIN code (AT+PIN=123456)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 4: Get MAC address (AT+MAC?)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK+MAC:112233445566";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 5: Set advertising interval (AT+INT=8)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 6: Set role to peripheral (AT+ROLE=0)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));

        // Step 7: Reset device (AT+RESET)
        EXPECT_CALL(*uart_mock_, receive(An<hal::UartData8b&>(), _, _))
            .WillOnce(DoAll(
                Invoke([](hal::UartData8b& data, hal::UartStatus& status, std::chrono::milliseconds) {
                    std::string response = "OK";
                    std::copy(response.begin(), response.end(), data.begin());
                    data[response.size()] = 0; // Null terminate
                    status = hal::UartStatus::Ok;
                }),
                SetArgReferee<1>(hal::UartStatus::Ok)));
    }

    // Execute a typical initialization sequence

    // Step 1: Test connectivity
    util::Status status = driver_->test();
    EXPECT_EQ(status, util::Status::Ok);

    // Step 2: Set device name
    status = driver_->set_name("HM11Device");
    EXPECT_EQ(status, util::Status::Ok);

    // Step 3: Set PIN code
    util::PinType pin("123456");
    status = driver_->set_pin(pin);
    EXPECT_EQ(status, util::Status::Ok);

    // Step 4: Get MAC address
    util::MacAddress mac;
    status = driver_->get_mac_address(mac);
    EXPECT_EQ(status, util::Status::Ok);
    EXPECT_EQ(mac, "112233445566");

    // Step 5: Set advertising interval
    status = driver_->set_advertising_interval(hm11::AdvertisingInterval::Ms1022);
    EXPECT_EQ(status, util::Status::Ok);

    // Step 6: Set role to peripheral
    status = driver_->set_role(SimpleHM11Driver::Role::Peripheral);
    EXPECT_EQ(status, util::Status::Ok);

    // Step 7: Reset device to apply settings
    status = driver_->reset();
    EXPECT_EQ(status, util::Status::Ok);

    // Get the transmitted data
    auto& transmitted = uart_mock_->transmitted_data_8b();

    // Check that data was transmitted
    ASSERT_GT(transmitted.size(), 0);

    // Verify all the expected commands were sent
    // Instead of checking exact positions, we'll just verify each command exists somewhere in the data
    const std::vector<std::string> expected_commands =
        {"AT", "AT+NAME=HM11Device", "AT+PIN=123456", "AT+MAC?", "AT+INT=8", "AT+ROLE=0", "AT+RESET"};

    // Create a string from the transmitted data for easier substring search
    std::string transmitted_str(transmitted.begin(), transmitted.end());

    for (const auto& cmd : expected_commands) {
        EXPECT_NE(transmitted_str.find(cmd), std::string::npos)
            << "Failed to find command '" << cmd << "' in transmitted data";
    }
}

} // namespace hm11::tests::integration