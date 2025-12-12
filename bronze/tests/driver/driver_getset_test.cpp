/**
 * @file driver_getset_test.cpp
 * @brief Tests for HM11Driver getter and setter operations
 *
 * This file contains tests for the getter and setter operations of the
 * HM11Driver class, such as role, advertising interval, module name, etc.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "hm11/driver.hpp"
#include "hm11/hal/uart.hpp"
#include "hm11/hm11.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for HM11Driver getter and setter operations
 *
 * This fixture provides a testing environment for the getter and setter
 * operations of the HM11Driver class, reusing the main test fixture.
 */
class HM11DriverGetSetTest : public ::testing::Test {
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
                                      [[maybe_unused]] hal::AnyUartPort port,
                                      void* received,
                                      std::size_t length,
                                      hal::UartStatus& status,
                                      std::chrono::milliseconds timeout_ms,
                                      [[maybe_unused]] bool as_stream) {
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

// Test get and set role operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_RoleGetSet) {
    // Configure the mock to return specific responses for role commands
    uart_mock_->add_command_response("AT+ROLE?", "OK+Get:0"); // Peripheral
    uart_mock_->add_command_response("AT+ROLE0", "OK+Set:0"); // Set to Peripheral
    uart_mock_->add_command_response("AT+ROLE1", "OK+Set:1"); // Set to Central

    // Get the current role (should be Peripheral by default)
    hal::UartStatus status;
    Role role;
    driver_->get_role(role, status);

    // Verify the status and role
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(role, Role::Peripheral);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the role to Central
    driver_->set_role(Role::Central, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+ROLE1");

    // Update the mock to return the new role
    uart_mock_->add_command_response("AT+ROLE?", "OK+Get:1"); // Central

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the role again (should be Central now)
    driver_->get_role(role, status);

    // Verify the status and role
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(role, Role::Central);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the role back to Peripheral
    driver_->set_role(Role::Peripheral, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data2 = uart_mock_->transmitted_data_8b();
    std::string transmitted2(data2.begin(), data2.end());
    EXPECT_EQ(transmitted2, "AT+ROLE0");
}

// Test get and set advertising interval operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_AdvertisingIntervalGetSet) {
    // Configure the mock to return specific responses for advertising interval commands
    uart_mock_->add_command_response("AT+ADVI?", "OK+Get:0"); // Default (100ms)
    uart_mock_->add_command_response("AT+ADVI9", "OK+Set:9"); // Set to 1285ms (max recommended)

    // Get the current advertising interval
    hal::UartStatus status;
    AdvertisingInterval interval;
    driver_->get_advertising_interval(interval, status);

    // Verify the status and interval
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(interval, AdvertisingInterval::Ms100);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the advertising interval to 1285ms (maximum recommended value)
    driver_->set_advertising_interval(AdvertisingInterval::Ms1285, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+ADVI9");

    // Update the mock to return the new interval
    uart_mock_->add_command_response("AT+ADVI?", "OK+Get:9"); // 1285ms

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the interval again (should be 1285ms now)
    driver_->get_advertising_interval(interval, status);

    // Verify the status and interval
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(interval, AdvertisingInterval::Ms1285);
}

// Test get and set advertising type operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_AdvertisingTypeGetSet) {
    // Configure the mock to return specific responses for advertising type commands
    uart_mock_->add_command_response("AT+ADTY?", "OK+Get:0"); // Default (AdvertisingScanResponseConnectable)
    uart_mock_->add_command_response("AT+ADTY3", "OK+Set:3"); // Set to AnyDeviceConnect

    // Get the current advertising type
    hal::UartStatus status;
    AdvertisingType type;
    driver_->get_advertising_type(type, status);

    // Verify the status and type
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, AdvertisingType::AdvertisingScanResponseConnectable);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the advertising type to AnyDeviceConnect
    driver_->set_advertising_type(AdvertisingType::Advertising, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+ADTY3");

    // Update the mock to return the new type
    uart_mock_->add_command_response("AT+ADTY?", "OK+Get:3"); // Advertising

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the type again (should be AnyDeviceConnect now)
    driver_->get_advertising_type(type, status);

    // Verify the status and type
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, AdvertisingType::Advertising);
}

// Test get and set module name operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_ModuleNameGetSet) {
    // Configure the mock to return specific responses for module name commands
    uart_mock_->add_command_response("AT+NAME?", "OK+NAME:HMSoft"); // Default
    uart_mock_->add_command_response("AT+NAMETest", "OK+Set:Test"); // Set to "Test"

    // Get the current module name
    hal::UartStatus status;
    std::string name = driver_->get_module_name(status);

    // Verify the status and name
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(name, "HMSoft");

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the module name to "Test"
    driver_->set_module_name("Test", status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+NAMETest");

    // Update the mock to return the new name
    uart_mock_->add_command_response("AT+NAME?", "OK+NAME:Test"); // New name

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the name again (should be "Test" now)
    name = driver_->get_module_name(status);

    // Verify the status and name
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(name, "Test");
}

// Test get and set module power operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_ModulePowerGetSet) {
    // Configure the mock to return specific responses for module power commands
    uart_mock_->add_command_response("AT+POWE?", "OK+Get:2"); // Default (0 dBm)
    uart_mock_->add_command_response("AT+POWE3", "OK+Set:3"); // Set to 6 dBm (maximum)

    // Get the current module power
    hal::UartStatus status;
    ModulePower power;
    driver_->get_module_power(power, status);

    // Verify the status and power
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(power, ModulePower::Dbm0);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the module power to maximum (6 dBm)
    driver_->set_module_power(ModulePower::Dbm6, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+POWE3");

    // Update the mock to return the new power
    uart_mock_->add_command_response("AT+POWE?", "OK+Get:3"); // 6 dBm

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the power again (should be 6 dBm now)
    driver_->get_module_power(power, status);

    // Verify the status and power
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(power, ModulePower::Dbm6);
}

// Test get and set work mode operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_WorkModeGetSet) {
    // Configure the mock to return specific responses for work mode commands
    uart_mock_->add_command_response("AT+MODE?", "OK+Get:0"); // Default (Transmission mode)
    uart_mock_->add_command_response("AT+MODE2", "OK+Set:2"); // Set to Remote Control mode

    // Get the current work mode
    hal::UartStatus status;
    WorkMode mode;
    driver_->get_work_mode(mode, status);

    // Verify the status and mode
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(mode, WorkMode::TransmissionMode);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the work mode to Remote Control mode
    driver_->set_work_mode(WorkMode::RemoteControlMode, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+MODE2");

    // Update the mock to return the new mode
    uart_mock_->add_command_response("AT+MODE?", "OK+Get:2"); // Remote Control mode

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the mode again (should be Remote Control mode now)
    driver_->get_work_mode(mode, status);

    // Verify the status and mode
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(mode, WorkMode::RemoteControlMode);
}

// Test get and set work type operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_WorkTypeGetSet) {
    // Configure the mock to return specific responses for work type commands
    uart_mock_->add_command_response("AT+IMME?", "OK+Get:0"); // Default (Start immediately)
    uart_mock_->add_command_response("AT+IMME1", "OK+Set:1"); // Set to respond to AT commands

    // Get the current work type
    hal::UartStatus status;
    WorkType type;
    driver_->get_work_type(type, status);

    // Verify the status and type
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, WorkType::StartImmediately);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the work type to respond to AT commands
    driver_->set_work_type(WorkType::RespondAtCommand, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+IMME1");

    // Update the mock to return the new type
    uart_mock_->add_command_response("AT+IMME?", "OK+Get:1"); // Respond to AT commands

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the type again (should be Respond to AT commands now)
    driver_->get_work_type(type, status);

    // Verify the status and type
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, WorkType::RespondAtCommand);
}

// Test get and set UART baud rate operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_UartBaudRateGetSet) {
    // Configure the mock to return specific responses for UART baud rate commands
    uart_mock_->add_command_response("AT+BAUD?", "OK+Get:0"); // Default (9600)
    uart_mock_->add_command_response("AT+BAUD4", "OK+Set:4"); // Set to 115200

    // Get the current UART baud rate
    hal::UartStatus status;
    UartBaudRate baud_rate;
    driver_->get_uart_baud_rate(baud_rate, status);

    // Verify the status and baud rate
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(baud_rate, UartBaudRate::Br9600);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the UART baud rate to 115200
    driver_->set_uart_baud_rate(UartBaudRate::Br115200, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+BAUD4");

    // Update the mock to return the new baud rate
    uart_mock_->add_command_response("AT+BAUD?", "OK+Get:4"); // 115200

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the baud rate again (should be 115200 now)
    driver_->get_uart_baud_rate(baud_rate, status);

    // Verify the status and baud rate
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(baud_rate, UartBaudRate::Br115200);
}

// Test get and set module auto sleep operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_ModuleAutoSleepGetSet) {
    // Configure the mock to return specific responses for module auto sleep commands
    uart_mock_->add_command_response("AT+ASLP?", "OK+Get:0"); // Default (false)
    uart_mock_->add_command_response("AT+ASLP1", "OK+Set:1"); // Set to true

    // Get the current module auto sleep setting
    hal::UartStatus status;
    bool auto_sleep;
    driver_->get_module_auto_sleep(auto_sleep, status);

    // Verify the status and auto sleep setting
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_FALSE(auto_sleep);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Enable module auto sleep
    driver_->set_module_auto_sleep(true, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+ASLP1");

    // Update the mock to return the new auto sleep setting
    uart_mock_->add_command_response("AT+ASLP?", "OK+Get:1"); // true

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the auto sleep setting again (should be true now)
    driver_->get_module_auto_sleep(auto_sleep, status);

    // Verify the status and auto sleep setting
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_TRUE(auto_sleep);
}

// Test get and set iBeacon switch operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_IBeaconSwitchGetSet) {
    // Configure the mock to return specific responses for iBeacon switch commands
    uart_mock_->add_command_response("AT+IBEA?", "OK+Get:0"); // Default (false)
    uart_mock_->add_command_response("AT+IBEA1", "OK+Set:1"); // Set to true

    // Get the current iBeacon switch setting
    hal::UartStatus status;
    bool ibeacon_switch;
    driver_->get_ibeacon_switch(ibeacon_switch, status);

    // Verify the status and iBeacon switch setting
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_FALSE(ibeacon_switch);

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Enable iBeacon
    driver_->set_ibeacon_switch(true, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+IBEA1");

    // Update the mock to return the new iBeacon switch setting
    uart_mock_->add_command_response("AT+IBEA?", "OK+Get:1"); // true

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the iBeacon switch setting again (should be true now)
    driver_->get_ibeacon_switch(ibeacon_switch, status);

    // Verify the status and iBeacon switch setting
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_TRUE(ibeacon_switch);
}

// Test get and set iBeacon UUID operations
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_IBeaconUuidGetSet) {
    // Configure the mock to return specific responses for iBeacon UUID commands
    uart_mock_->add_command_response("AT+IBE0?",
                                     "OK+Get:74278BDAB64445208F0C720EAF059935"); // Default UUID

    // Create a new UUID to set
    util::IBeaconUuid new_uuid("AABBCCDDEEFF00112233445566778899");

    // Configure mock for setting the UUID
    uart_mock_->add_command_response(
        "AT+IBE0AABBCCDDEEFF00112233445566778899",
        "OK+Set:AABBCCDDEEFF00112233445566778899");

    // Get the current iBeacon UUID
    hal::UartStatus status;
    util::IBeaconUuid uuid;
    driver_->get_ibeacon_uuid(uuid, status);

    // Verify the status and UUID
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uuid, util::IBeaconUuid("74278BDAB64445208F0C720EAF059935"));

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Set the iBeacon UUID
    driver_->set_ibeacon_uuid(new_uuid, status);

    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+IBE0AABBCCDDEEFF00112233445566778899");

    // Update the mock to return the new UUID
    uart_mock_->add_command_response("AT+IBE0?", "OK+Get:AABBCCDDEEFF00112233445566778899");

    // Clear the transmitted data for the next test
    uart_mock_->clear_transmitted_data();

    // Get the UUID again (should be the new UUID now)
    driver_->get_ibeacon_uuid(uuid, status);

    // Verify the status and UUID
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uuid, new_uuid);
}

// Test get MAC address operation
// DISABLED: Requires full implementation with AT command parsing
TEST_F(HM11DriverGetSetTest, DISABLED_GetMacAddress) {
    // Configure the mock to return a specific MAC address
    uart_mock_->add_command_response("AT+ADDR?", "OK+ADDR:112233445566");

    // Get the MAC address
    hal::UartStatus status;
    util::MacAddress mac;
    driver_->get_mac_address(mac, status);

    // Verify the status and MAC address
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(mac, util::MacAddress("112233445566"));

    // Verify the command was sent
    const auto& data = uart_mock_->transmitted_data_8b();
    std::string transmitted(data.begin(), data.end());
    EXPECT_EQ(transmitted, "AT+ADDR?");
}

// Test error case for getters and setters
// DISABLED: Requires full implementation with error handling
TEST_F(HM11DriverGetSetTest, DISABLED_ErrorCases) {
    // Configure the mock to return an error status
    uart_mock_->add_command_response("AT+ROLE?", "", hal::UartStatus::Error);

    // Try to get the role
    hal::UartStatus status;
    Role role;
    driver_->get_role(role, status);

    // Verify the status indicates an error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

} // namespace hm11::tests