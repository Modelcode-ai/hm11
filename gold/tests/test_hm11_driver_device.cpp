/**
 * @file test_hm11_driver_device.cpp
 * @brief Unit tests for HM11Driver device management operations
 *
 * Tests cover:
 * - Get_Module_Name with various length responses (1-12 chars)
 * - Set_Module_Name with valid names and precondition violation
 * - Get_MAC_Address parsing from "OK+ADDR:B4994C123456" response
 * - Battery operations (Query, Set, Monitor switch Get/Set)
 * - Temperature query and advertising byte setting
 * - Humidity advertising byte setting
 * - Status error propagation for all operations
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// ============================================================================
// Mock UART Port for Testing Device Management Operations
// ============================================================================

class DeviceTestMockUARTPort {
  public:
    DeviceTestMockUARTPort() = default;

    // Configure mock to return specific response
    void set_response(std::string_view response) {
        response_data.clear();
        response_data.insert(response_data.end(), response.begin(), response.end());
        next_transmit_fails = false;
        next_receive_fails = false;
    }

    void set_transmit_fails() { next_transmit_fails = true; }

    void set_receive_fails() { next_receive_fails = true; }

    std::string get_last_transmitted() const {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        return {reinterpret_cast<const char*>(last_transmitted.data()), last_transmitted.size()};
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    // UART interface methods matching IS_UART_PORT_V concept
    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        if (next_transmit_fails) {
            status = UartStatus::ErrError;
            return;
        }
        last_transmitted.clear();
        last_transmitted.insert(last_transmitted.end(), data.begin(), data.end());
        status = UartStatus::Ok;
    }

    void receive(
        UartData8b buffer,
        UartStatus& status,
        uint32_t /*timeout_ms*/ = 1000,
        bool /*as_stream*/ = false) noexcept {
        if (next_receive_fails) {
            status = UartStatus::ErrTimeout;
            return;
        }

        const std::size_t COPY_SIZE = std::min(buffer.size(), response_data.size());
        std::copy_n(response_data.begin(), COPY_SIZE, buffer.begin());
        status = UartStatus::Ok;
    }

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

  private:
    std::vector<uint8_t> response_data;
    std::vector<uint8_t> last_transmitted;
    bool next_transmit_fails{false};
    bool next_receive_fails{false};
};

// ============================================================================
// Mock Receive Handler for Testing
// ============================================================================

class MockReceiveHandler {
  public:
    // Overload WITH explicit timeout parameter
    void operator()(
        DeviceTestMockUARTPort& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        // Forward to port's receive method
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload WITHOUT timeout parameter (uses default)
    void operator()(DeviceTestMockUARTPort& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        // Forward to port's receive method with default timeout
        port.receive(buffer, status, 1000, as_stream);
    }
};

// Concept satisfaction checks
static_assert(hal::IS_UART_PORT_V<DeviceTestMockUARTPort>, "DeviceTestMockUARTPort must satisfy IS_UART_PORT_V");
static_assert(
    hal::IS_RECEIVE_HANDLER_V<MockReceiveHandler, DeviceTestMockUARTPort>,
    "MockReceiveHandler must satisfy IS_RECEIVE_HANDLER_V");

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverDeviceTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // GoogleTest convention: test fixture members should be protected
    DeviceTestMockUARTPort uart_port;
    MockReceiveHandler receive_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

// ============================================================================
// Get_Module_Name Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, GetModuleName_ShortName) {
    HM11Driver driver(uart_port, receive_handler);

    // Test with 3-character name
    uart_port.set_response("OK+NAME:ABC\r\n");

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(name, "ABC");
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+NAME?");
}

TEST_F(HM11DriverDeviceTest, GetModuleName_MaxLength) {
    HM11Driver driver(uart_port, receive_handler);

    // Test with 12-character name (maximum)
    uart_port.set_response("OK+NAME:ABCDEFGHIJKL\r\n");

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(name, "ABCDEFGHIJKL");
    EXPECT_EQ(name.size(), 12U);
}

TEST_F(HM11DriverDeviceTest, GetModuleName_MinLength) {
    HM11Driver driver(uart_port, receive_handler);

    // Test with 1-character name (minimum)
    uart_port.set_response("OK+NAME:X\r\n");

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(name, "X");
    EXPECT_EQ(name.size(), 1U);
}

TEST_F(HM11DriverDeviceTest, GetModuleName_DefaultName) {
    HM11Driver driver(uart_port, receive_handler);

    // Test with default name "HMSoft"
    uart_port.set_response("OK+NAME:HMSoft\r\n");

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(name, "HMSoft");
}

TEST_F(HM11DriverDeviceTest, GetModuleName_TransmitFails) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_transmit_fails();

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverDeviceTest, GetModuleName_ReceiveFails) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_receive_fails();

    std::string_view name;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_name(name, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

// ============================================================================
// Set_Module_Name Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, SetModuleName_ValidName) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:TestDev");

    UartStatus status{UartStatus::ErrError};
    driver.set_module_name("TestDev", status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+NAMETestDev");
}

TEST_F(HM11DriverDeviceTest, SetModuleName_MaxLength) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:ABCDEFGHIJKL");

    UartStatus status{UartStatus::ErrError};
    driver.set_module_name("ABCDEFGHIJKL", status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+NAMEABCDEFGHIJKL");
}

TEST_F(HM11DriverDeviceTest, SetModuleName_MinLength) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:X");

    UartStatus status{UartStatus::ErrError};
    driver.set_module_name("X", status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+NAMEX");
}

TEST_F(HM11DriverDeviceTest, SetModuleName_TooLong) {
#ifdef NDEBUG
    // Release build: runtime check returns error status
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:ABCDEFGHIJKLM");

    UartStatus status{UartStatus::ErrError};
    // Name with 13 characters (exceeds maximum of 12)
    driver.set_module_name("ABCDEFGHIJKLM", status);

    // Should fail precondition check (returns error in release)
    EXPECT_EQ(status, UartStatus::ErrError);
#else
    // Debug build: assertion fires and terminates
    EXPECT_DEATH(
        {
            HM11Driver driver(uart_port, receive_handler);
            UartStatus status{UartStatus::ErrError};
            driver.set_module_name("ABCDEFGHIJKLM", status);
        },
        "Module name must be 1-12 characters");
#endif
}

TEST_F(HM11DriverDeviceTest, SetModuleName_Empty) {
#ifdef NDEBUG
    // Release build: runtime check returns error status
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:");

    UartStatus status{UartStatus::ErrError};
    // Empty name (violates precondition)
    driver.set_module_name("", status);

    // Should fail precondition check
    EXPECT_EQ(status, UartStatus::ErrError);
#else
    // Debug build: assertion fires and terminates
    EXPECT_DEATH(
        {
            HM11Driver driver(uart_port, receive_handler);
            UartStatus status{UartStatus::ErrError};
            driver.set_module_name("", status);
        },
        "Module name must be 1-12 characters");
#endif
}

// ============================================================================
// Get_MAC_Address Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, GetMACAddress_ValidResponse) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+ADDR:B4994C123456");

    MacAddress mac{};
    UartStatus status{UartStatus::ErrError};
    driver.get_mac_address(mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mac.view(), "B4994C123456");
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+ADDR?");
}

TEST_F(HM11DriverDeviceTest, GetMACAddress_DifferentAddress) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+ADDR:AABBCCDDEEFF");

    MacAddress mac{};
    UartStatus status{UartStatus::ErrError};
    driver.get_mac_address(mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mac.view(), "AABBCCDDEEFF");
}

TEST_F(HM11DriverDeviceTest, GetMACAddress_TransmitFails) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_transmit_fails();

    MacAddress mac{};
    UartStatus status{UartStatus::ErrError};
    driver.get_mac_address(mac, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Battery Information Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, QueryBatteryInformation_FullyCharged) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:100\r\n");

    Percent result{0};
    UartStatus status{UartStatus::ErrError};
    driver.query_battery_information(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.get(), 100);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATT?");
}

TEST_F(HM11DriverDeviceTest, QueryBatteryInformation_HalfCharged) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:50\r\n");

    Percent result{0};
    UartStatus status{UartStatus::ErrError};
    driver.query_battery_information(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.get(), 50);
}

TEST_F(HM11DriverDeviceTest, QueryBatteryInformation_Empty) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:0\r\n");

    Percent result{100};
    UartStatus status{UartStatus::ErrError};
    driver.query_battery_information(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.get(), 0);
}

TEST_F(HM11DriverDeviceTest, SetBatteryInformation_ValidValue) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+BATT075");

    Percent value{75};
    UartStatus status{UartStatus::ErrError};
    driver.set_battery_information(value, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATT75");
}

TEST_F(HM11DriverDeviceTest, SetBatteryInformation_Zero) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+BATT000");

    Percent value{0};
    UartStatus status{UartStatus::ErrError};
    driver.set_battery_information(value, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATT0");
}

TEST_F(HM11DriverDeviceTest, SetBatteryInformation_Max) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+BATT100");

    Percent value{100};
    UartStatus status{UartStatus::ErrError};
    driver.set_battery_information(value, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATT100");
}

// ============================================================================
// Battery Monitor Switch Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, SetBatteryMonitorSwitch_Enable) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:1");

    UartStatus status{UartStatus::ErrError};
    driver.set_battery_monitor_switch(true, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATC1");
}

TEST_F(HM11DriverDeviceTest, SetBatteryMonitorSwitch_Disable) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:0");

    UartStatus status{UartStatus::ErrError};
    driver.set_battery_monitor_switch(false, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATC0");
}

TEST_F(HM11DriverDeviceTest, GetBatteryMonitorSwitch_Enabled) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:1");

    bool enabled = false;
    UartStatus status{UartStatus::ErrError};
    driver.get_battery_monitor_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(enabled);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BATC?");
}

TEST_F(HM11DriverDeviceTest, GetBatteryMonitorSwitch_Disabled) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:0");

    bool enabled = true;
    UartStatus status{UartStatus::ErrError};
    driver.get_battery_monitor_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_FALSE(enabled);
}

// ============================================================================
// Module Temperature Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, GetModuleTemperature_ValidResponse) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:25\r\n");

    std::string_view temperature;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_temperature(temperature, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, "25");
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TEMP?");
}

TEST_F(HM11DriverDeviceTest, GetModuleTemperature_HighValue) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:100\r\n");

    std::string_view temperature;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_temperature(temperature, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, "100");
}

TEST_F(HM11DriverDeviceTest, GetModuleTemperature_SingleDigit) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Get:5\r\n");

    std::string_view temperature;
    UartStatus status{UartStatus::ErrError};
    driver.get_module_temperature(temperature, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, "5");
}

// ============================================================================
// Advertising Temperature Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, SetTemperatureInAdvertising_ValidHex) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:0x19");

    AdvertisingTemperatureType temp{"19"}; // 25 degrees Celsius in hex
    UartStatus status{UartStatus::ErrError};
    driver.set_temperature_information_byte_in_advertising(temp, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TEMP0x19");
}

TEST_F(HM11DriverDeviceTest, SetTemperatureInAdvertising_Zero) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:0x00");

    AdvertisingTemperatureType temp{"00"};
    UartStatus status{UartStatus::ErrError};
    driver.set_temperature_information_byte_in_advertising(temp, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TEMP0x00");
}

TEST_F(HM11DriverDeviceTest, SetTemperatureInAdvertising_Max) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:0xFF");

    AdvertisingTemperatureType temp{"FF"};
    UartStatus status{UartStatus::ErrError};
    driver.set_temperature_information_byte_in_advertising(temp, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TEMP0xFF");
}

// ============================================================================
// Advertising Humidity Tests
// ============================================================================

TEST_F(HM11DriverDeviceTest, SetHumidityInAdvertising_ValidHex) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:3C");

    HumiInformation humidity{"3C"}; // 60 percent in hex
    UartStatus status{UartStatus::ErrError};
    driver.set_humi_information_byte_in_advertising(humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+HUMI3C");
}

TEST_F(HM11DriverDeviceTest, SetHumidityInAdvertising_Zero) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:00");

    HumiInformation humidity{"00"};
    UartStatus status{UartStatus::ErrError};
    driver.set_humi_information_byte_in_advertising(humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+HUMI00");
}

TEST_F(HM11DriverDeviceTest, SetHumidityInAdvertising_Max) {
    HM11Driver driver(uart_port, receive_handler);

    uart_port.set_response("OK+Set:FF");

    HumiInformation humidity{"FF"};
    UartStatus status{UartStatus::ErrError};
    driver.set_humi_information_byte_in_advertising(humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+HUMIFF");
}
