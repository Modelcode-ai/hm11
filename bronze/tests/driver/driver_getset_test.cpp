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
TEST_F(HM11DriverGetSetTest, RoleGetSet) {
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

// Simple test to verify transmit works at all
TEST_F(HM11DriverGetSetTest, SimpleTransmitTest) {
    uart_mock_->add_command_response("AT", "OK");

    hal::UartStatus status;
    driver_->test(status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT");
}

// Test get advertising interval operation
TEST_F(HM11DriverGetSetTest, GetAdvertisingInterval_Ms100) {
    uart_mock_->add_command_response("AT+ADVI?", "OK+Get:0");

    hal::UartStatus status{hal::UartStatus::Busy}; // Initialize to non-Ok
    AdvertisingInterval interval;
    driver_->get_advertising_interval(interval, status);

    // Debug: Check what status we got
    if (status != hal::UartStatus::Ok) {
        GTEST_FAIL() << "Status was not Ok, got: " << static_cast<int>(status);
    }

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(interval, AdvertisingInterval::Ms100);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADVI?");
}

// Test get advertising interval operation with different value
TEST_F(HM11DriverGetSetTest, GetAdvertisingInterval_Ms1285) {
    uart_mock_->add_command_response("AT+ADVI?", "OK+Get:9");

    hal::UartStatus status;
    AdvertisingInterval interval;
    driver_->get_advertising_interval(interval, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(interval, AdvertisingInterval::Ms1285);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADVI?");
}

// Test set advertising interval operation
TEST_F(HM11DriverGetSetTest, SetAdvertisingInterval_Ms1285) {
    uart_mock_->add_command_response("AT+ADVI9", "OK+Set:9");

    hal::UartStatus status;
    driver_->set_advertising_interval(AdvertisingInterval::Ms1285, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADVI9");
}

// Test get advertising type operation
TEST_F(HM11DriverGetSetTest, GetAdvertisingType_ScanResponseConnectable) {
    uart_mock_->add_command_response("AT+ADTY?", "OK+Get:0");

    hal::UartStatus status;
    AdvertisingType type;
    driver_->get_advertising_type(type, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, AdvertisingType::AdvertisingScanResponseConnectable);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADTY?");
}

// Test get advertising type with different value
TEST_F(HM11DriverGetSetTest, GetAdvertisingType_Advertising) {
    uart_mock_->add_command_response("AT+ADTY?", "OK+Get:3");

    hal::UartStatus status;
    AdvertisingType type;
    driver_->get_advertising_type(type, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, AdvertisingType::Advertising);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADTY?");
}

// Test set advertising type operation
TEST_F(HM11DriverGetSetTest, SetAdvertisingType_Advertising) {
    uart_mock_->add_command_response("AT+ADTY3", "OK+Set:3");

    hal::UartStatus status;
    driver_->set_advertising_type(AdvertisingType::Advertising, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+ADTY3");
}

// Test get and set module name operations
TEST_F(HM11DriverGetSetTest, ModuleNameGetSet) {
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
TEST_F(HM11DriverGetSetTest, ModulePowerGetSet) {
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
// Test get work mode operation
TEST_F(HM11DriverGetSetTest, GetWorkMode_TransmissionMode) {
    uart_mock_->add_command_response("AT+MODE?", "OK+Get:0");

    hal::UartStatus status;
    WorkMode mode;
    driver_->get_work_mode(mode, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(mode, WorkMode::TransmissionMode);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+MODE?");
}

// Test get work mode with different value
TEST_F(HM11DriverGetSetTest, GetWorkMode_RemoteControlMode) {
    uart_mock_->add_command_response("AT+MODE?", "OK+Get:2");

    hal::UartStatus status;
    WorkMode mode;
    driver_->get_work_mode(mode, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(mode, WorkMode::RemoteControlMode);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+MODE?");
}

// Test set work mode operation
TEST_F(HM11DriverGetSetTest, SetWorkMode_RemoteControlMode) {
    uart_mock_->add_command_response("AT+MODE2", "OK+Set:2");

    hal::UartStatus status;
    driver_->set_work_mode(WorkMode::RemoteControlMode, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+MODE2");
}

// Test get and set work type operations
// DISABLED: Requires full implementation with AT command parsing
// Test get work type operation
TEST_F(HM11DriverGetSetTest, GetWorkType_StartImmediately) {
    uart_mock_->add_command_response("AT+IMME?", "OK+Get:0");

    hal::UartStatus status;
    WorkType type;
    driver_->get_work_type(type, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, WorkType::StartImmediately);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IMME?");
}

// Test get work type with different value
TEST_F(HM11DriverGetSetTest, GetWorkType_RespondAtCommand) {
    uart_mock_->add_command_response("AT+IMME?", "OK+Get:1");

    hal::UartStatus status;
    WorkType type;
    driver_->get_work_type(type, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(type, WorkType::RespondAtCommand);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IMME?");
}

// Test set work type operation
TEST_F(HM11DriverGetSetTest, SetWorkType_RespondAtCommand) {
    uart_mock_->add_command_response("AT+IMME1", "OK+Set:1");

    hal::UartStatus status;
    driver_->set_work_type(WorkType::RespondAtCommand, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IMME1");
}

// Test get and set UART baud rate operations
TEST_F(HM11DriverGetSetTest, UartBaudRateGetSet) {
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
TEST_F(HM11DriverGetSetTest, ModuleAutoSleepGetSet) {
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
// Test get iBeacon switch operation
TEST_F(HM11DriverGetSetTest, GetIBeaconSwitch_Disabled) {
    uart_mock_->add_command_response("AT+IBEA?", "OK+Get:0");

    hal::UartStatus status;
    bool ibeacon_switch;
    driver_->get_ibeacon_switch(ibeacon_switch, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_FALSE(ibeacon_switch);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IBEA?");
}

// Test get iBeacon switch with different value
TEST_F(HM11DriverGetSetTest, GetIBeaconSwitch_Enabled) {
    uart_mock_->add_command_response("AT+IBEA?", "OK+Get:1");

    hal::UartStatus status;
    bool ibeacon_switch;
    driver_->get_ibeacon_switch(ibeacon_switch, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_TRUE(ibeacon_switch);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IBEA?");
}

// Test set iBeacon switch operation
TEST_F(HM11DriverGetSetTest, SetIBeaconSwitch_Enable) {
    uart_mock_->add_command_response("AT+IBEA1", "OK+Set:1");

    hal::UartStatus status;
    driver_->set_ibeacon_switch(true, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uart_mock_->get_transmitted_string(), "AT+IBEA1");
}

// Test get iBeacon UUID operation
TEST_F(HM11DriverGetSetTest, GetIBeaconUuid_DefaultValue) {
    uart_mock_->add_command_response("AT+IBE0?", "OK+Get:74278BDAB64445208F0C720EAF059935");
    uart_mock_->add_command_response("AT+IBE1?", "OK+Get:");
    uart_mock_->add_command_response("AT+IBE2?", "OK+Get:");
    uart_mock_->add_command_response("AT+IBE3?", "OK+Get:");

    hal::UartStatus status;
    util::IBeaconUuid uuid;
    driver_->get_ibeacon_uuid(uuid, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(uuid, util::IBeaconUuid("74278BDAB64445208F0C720EAF059935"));
}

// Test set iBeacon UUID operation
TEST_F(HM11DriverGetSetTest, SetIBeaconUuid_CustomValue) {
    uart_mock_->add_command_response("AT+IBE0AABBCCDD", "OK+Set:AABBCCDD");
    uart_mock_->add_command_response("AT+IBE1EEFF0011", "OK+Set:EEFF0011");
    uart_mock_->add_command_response("AT+IBE222334455", "OK+Set:22334455");
    uart_mock_->add_command_response("AT+IBE366778899", "OK+Set:66778899");

    util::IBeaconUuid new_uuid("AABBCCDDEEFF00112233445566778899");
    hal::UartStatus status;
    driver_->set_ibeacon_uuid(new_uuid, status);

    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test get MAC address operation
TEST_F(HM11DriverGetSetTest, GetMacAddress) {
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
TEST_F(HM11DriverGetSetTest, ErrorCases) {
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