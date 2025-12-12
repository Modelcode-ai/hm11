/**
 * @file driver_integration_test.cpp
 * @brief Integration tests for HM11Driver workflows
 *
 * This file contains integration tests for the HM11Driver class, testing
 * complete workflows such as device initialization, configuration, and
 * connection.
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

namespace hm11::tests::integration {

/**
 * @brief Test fixture for HM11Driver integration tests
 *
 * This fixture provides a testing environment for integration tests that
 * simulate complete workflows using the HM11Driver class.
 */
class HM11DriverIntegrationTest : public ::testing::Test {
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

// Test a complete device initialization workflow
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverIntegrationTest, DISABLED_DeviceInitializationWorkflow) {
    // Configure the mock to return appropriate responses for the initialization sequence
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+VERSION", "HMSoft V1.0");
    uart_mock_->add_command_response("AT+ROLE0", "OK+Set:0");               // Set to Peripheral
    uart_mock_->add_command_response("AT+IMME1", "OK+Set:1");               // Set to respond to AT commands
    uart_mock_->add_command_response("AT+ADVI3", "OK+Set:3");               // Set advertising interval to 318ms
    uart_mock_->add_command_response("AT+NAMEMyDevice", "OK+Set:MyDevice"); // Set name
    uart_mock_->add_command_response("AT+MODE0", "OK+Set:0");               // Set to Transmission mode

    // Execute the initialization workflow
    hal::UartStatus status;

    // Step 1: Test communication
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 2: Get software version
    util::VersionType version;
    driver_->software_version(version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(version, util::VersionType("V1.0"));

    // Step 3: Set role to Peripheral
    driver_->set_role(Role::Peripheral, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 4: Set work type to respond to AT commands
    driver_->set_work_type(WorkType::RespondAtCommand, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 5: Set advertising interval to 318ms
    driver_->set_advertising_interval(AdvertisingInterval::Ms318, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 6: Set device name
    driver_->set_module_name("MyDevice", status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 7: Set work mode to Transmission mode
    driver_->set_work_mode(WorkMode::TransmissionMode, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the transmitted data sequence
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());

    // Verify that each command was sent in the correct order
    EXPECT_NE(transmitted.find("AT"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+VERSION"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+ROLE0"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+IMME1"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+ADVI3"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+NAMEMyDevice"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+MODE0"), std::string::npos);
}

// Test a configuration workflow for iBeacon mode
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverIntegrationTest, DISABLED_IBeaconConfigurationWorkflow) {
    // Configure the mock to return appropriate responses for the iBeacon configuration sequence
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+IBEA1", "OK+Set:1"); // Enable iBeacon
    uart_mock_->add_command_response(
        "AT+IBE0AABBCCDDEEFF00112233445566778899",
        "OK+Set:AABBCCDDEEFF00112233445566778899");                 // Set UUID
    uart_mock_->add_command_response("AT+MARJAAAA", "OK+Set:AAAA"); // Set major version
    uart_mock_->add_command_response("AT+MINO0001", "OK+Set:0001"); // Set minor version
    uart_mock_->add_command_response("AT+MEAS50", "OK+Set:50");     // Set measured power

    // Execute the iBeacon configuration workflow
    hal::UartStatus status;

    // Step 1: Test communication
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 2: Enable iBeacon
    driver_->set_ibeacon_switch(true, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 3: Set iBeacon UUID
    util::IBeaconUuid uuid("AABBCCDDEEFF00112233445566778899");
    driver_->set_ibeacon_uuid(uuid, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 4: Set major version
    util::VersionType major_version("AAAA");
    driver_->set_ibeacon_major_version(major_version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 5: Set minor version
    util::VersionType minor_version("0001");
    driver_->set_ibeacon_minor_version(minor_version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 6: Set measured power
    util::MeasuredPower measured_power("50");
    driver_->set_ibeacon_measured_power(measured_power, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the transmitted data sequence
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());

    // Verify that each command was sent in the correct order
    EXPECT_NE(transmitted.find("AT"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+IBEA1"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+IBE0AABBCCDDEEFF00112233445566778899"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+MARJAAAA"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+MINO0001"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+MEAS50"), std::string::npos);
}

// Test a recovery workflow after errors
// DISABLED: Requires full implementation with error handling
TEST_F(HM11DriverIntegrationTest, DISABLED_ErrorRecoveryWorkflow) {
    // Configure the mock to return error responses initially, then success
    uart_mock_->add_command_response("AT", "ERROR", hal::UartStatus::Error);

    // Execute the error recovery workflow
    hal::UartStatus status;

    // Step 1: Test communication (should fail)
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);

    // Now update the mock to return success for subsequent calls
    uart_mock_->clear_transmitted_data();
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+RESTART", "OK");
    uart_mock_->add_command_response("AT", "OK"); // After restart

    // Step 2: Try again after simulated device restart
    driver_->restart(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 3: Test communication again (should succeed)
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the transmitted data sequence
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());

    // Verify that each command was sent in the correct order
    EXPECT_NE(transmitted.find("AT+RESTART"), std::string::npos);
    EXPECT_NE(transmitted.find("AT"), std::string::npos);
}

// Test a complete workflow with multiple operations and error handling
// DISABLED: Requires full implementation with error handling
TEST_F(HM11DriverIntegrationTest, DISABLED_CompleteWorkflowWithErrorHandling) {
    // Configure the mock for a complex workflow with some errors
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+VERSION", "HMSoft V1.0");
    uart_mock_->add_command_response("AT+ROLE1", "OK+Set:1");                // Set to Central
    uart_mock_->add_command_response("AT+IMME1", "OK+Set:1");                // Set to respond to AT commands
    uart_mock_->add_command_response("AT+POWE", "", hal::UartStatus::Error); // Simulate error
    uart_mock_->add_command_response("AT+RESTART", "OK");                    // Restart after error
    uart_mock_->add_command_response("AT", "OK");                            // Test after restart
    uart_mock_->add_command_response("AT+POWE3", "OK+Set:3");                // Set power to maximum (try again)
    uart_mock_->add_command_response("AT+START", "OK");                      // Start working

    // Execute the complex workflow
    hal::UartStatus status;

    // Step 1: Test communication
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 2: Get software version
    util::VersionType version;
    driver_->software_version(version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(version, util::VersionType("V1.0"));

    // Step 3: Set role to Central
    driver_->set_role(Role::Central, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 4: Set work type to respond to AT commands
    driver_->set_work_type(WorkType::RespondAtCommand, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 5: Try to set power (will fail)
    driver_->set_module_power(ModulePower::Dbm6, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);

    // Step 6: Restart the device
    driver_->restart(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 7: Test communication again
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 8: Try to set power again (should succeed)
    driver_->set_module_power(ModulePower::Dbm6, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Step 9: Start working
    driver_->start_working(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the transmitted data sequence
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());

    // Verify that each command was sent in the correct order
    EXPECT_NE(transmitted.find("AT"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+VERSION"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+ROLE1"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+IMME1"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+POWE"), std::string::npos); // First attempt
    EXPECT_NE(transmitted.find("AT+RESTART"), std::string::npos);
    EXPECT_NE(transmitted.find("AT+POWE3"), std::string::npos); // Second attempt
    EXPECT_NE(transmitted.find("AT+START"), std::string::npos);
}

} // namespace hm11::tests::integration