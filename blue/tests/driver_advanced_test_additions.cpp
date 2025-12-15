// Additional error path and edge case tests for driver_advanced.cpp

#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/types/enums.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Connection Result Code Tests ==========

TEST(HM11DriverAdvancedErrorTest, ConnectLastDevice_ConnectError) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN2\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::ConnectError);
}

TEST(HM11DriverAdvancedErrorTest, ConnectLastDevice_ConnectFail) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN3\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::ConnectFail);
}

TEST(HM11DriverAdvancedErrorTest, ConnectLastDevice_NoAddress) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN4\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::NoAddress);
}

TEST(HM11DriverAdvancedErrorTest, ConnectLastDevice_InvalidResultCode) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN9\r\n");  // Invalid code
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::Connecting;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
}

TEST(HM11DriverAdvancedErrorTest, ConnectLastDevice_Timeout) {
    MockUARTPort uart;
    // Don't set any receive data - will timeout
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::Connecting;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
    EXPECT_NE(status, Status::Ok);
}

// ========== Connect by MAC - All Types and Results ==========

TEST(HM11DriverAdvancedErrorTest, ConnectByMAC_StaticRandomMAC) {
    MockUARTPort uart;
    uart.set_next_receive("OK+COTT1\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("FFEEDDCCBBAA");
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::StaticRandomMAC, mac, result, status));
    EXPECT_EQ(result, ConnectResult::Connected);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COTFFEEDDCCBBAA\r\n");
}

TEST(HM11DriverAdvancedErrorTest, ConnectByMAC_RandomMAC) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CORR0\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("112233445566");
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::RandomMAC, mac, result, status));
    EXPECT_EQ(result, ConnectResult::Connecting);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COR112233445566\r\n");
}

TEST(HM11DriverAdvancedErrorTest, ConnectByMAC_AllResultCodes) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::MACAddress mac("AABBCCDDEEFF");
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;

    // Test ConnectError (2)
    uart.set_next_receive("OK+CONN2\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::ConnectError);

    // Test ConnectFail (3)
    uart.set_next_receive("OK+CONN3\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::ConnectFail);

    // Test NoAddress (4)
    uart.set_next_receive("OK+CONN4\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::NoAddress);

    // Test invalid code
    uart.set_next_receive("OK+CONNX\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
}

TEST(HM11DriverAdvancedErrorTest, ConnectByMAC_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::MACAddress mac("112233445566");
    ConnectResult result = ConnectResult::Connecting;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
    EXPECT_NE(status, Status::Ok);
}

// ========== Connect by Index - All Result Codes ==========

TEST(HM11DriverAdvancedErrorTest, ConnectByIndex_AllResultCodes) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::DiscoveryIndex index(5);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;

    // Test Connecting (0)
    uart.set_next_receive("OK+CONN0\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::Connecting);

    // Test ConnectError (2)
    uart.set_next_receive("OK+CONN2\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::ConnectError);

    // Test ConnectFail (3)
    uart.set_next_receive("OK+CONN3\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::ConnectFail);

    // Test NoAddress (4)
    uart.set_next_receive("OK+CONN4\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::NoAddress);

    // Test invalid code
    uart.set_next_receive("OK+CONN7\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
}

TEST(HM11DriverAdvancedErrorTest, ConnectByIndex_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::DiscoveryIndex index(3);
    ConnectResult result = ConnectResult::Connecting;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
    EXPECT_NE(status, Status::Ok);
}

// ========== Error Path Tests for Get Methods ==========

TEST(HM11DriverAdvancedErrorTest, GetLastConnectedDeviceAddress_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::MACAddress mac("000000000000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_last_connected_device_address(mac, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetSaveConnectedMAC_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    bool save = false;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_save_connected_mac(save, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetConnectRemoteDeviceTimeout_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::ConnectTimeout timeout("000000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_connect_remote_device_timeout(timeout, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetShowDeviceInformation_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    ShowDeviceInformation show = ShowDeviceInformation::DontShow;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_show_device_information_when_discovery(show, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetCharacteristic_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::CharacteristicType result("0000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_characteristic(result, status));
    EXPECT_NE(status, Status::Ok);
}

// ========== Error Path Tests for Set Methods ==========

TEST(HM11DriverAdvancedErrorTest, SetSaveConnectedMAC_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_save_connected_mac(true, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetConnectRemoteDeviceTimeout_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::ConnectTimeout timeout("001000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_connect_remote_device_timeout(timeout, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetDiscoveryTime_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::DiscoveryTime time(2);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_discovery_time(time, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetShowDeviceInformation_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowName, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetCharacteristic_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::CharacteristicType value("FFE1");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_characteristic(value, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetServiceUUID_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::UUID value("FFE0");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_service_uuid(value, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetMethodAndCharacteristicHandle_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::HandleType handle("001A");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_method_and_characteristic_handle(handle, SendDataMethod::Write, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetUseCharacteristicUUIDCount_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_use_characteristic_uuid_count(CharacteristicUUIDCount::One, status));
    EXPECT_NE(status, Status::Ok);
}

// ========== GATT Error Path Tests ==========

TEST(HM11DriverAdvancedErrorTest, EnableCharacteristicNotify_TransmitError) {
    MockUARTPort uart;
    uart.set_transmit_error(true);
    HM11Driver driver(uart);
    util::HandleType handle("001A");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.enable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, EnableCharacteristicNotify_ReceiveTimeout) {
    MockUARTPort uart;
    // Transmit succeeds but receive times out
    HM11Driver driver(uart);
    util::HandleType handle("001B");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.enable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, EnableCharacteristicNotify_InvalidResponse) {
    MockUARTPort uart;
    uart.set_next_receive("INVALID\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001C");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.enable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_EQ(status, Status::InvalidResponse);
}

TEST(HM11DriverAdvancedErrorTest, DisableCharacteristicNotify_TransmitError) {
    MockUARTPort uart;
    uart.set_transmit_error(true);
    HM11Driver driver(uart);
    util::HandleType handle("001D");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.disable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, DisableCharacteristicNotify_ReceiveTimeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::HandleType handle("001E");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.disable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, DisableCharacteristicNotify_InvalidResponse) {
    MockUARTPort uart;
    uart.set_next_receive("BADRESP\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001F");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.disable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_EQ(status, Status::InvalidResponse);
}

TEST(HM11DriverAdvancedErrorTest, DisableCharacteristicNotify_DataError) {
    MockUARTPort uart;
    uart.set_next_receive("OK+DATA-ER\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("0020");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.disable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
}

TEST(HM11DriverAdvancedErrorTest, ReadCharacteristicNotify_TransmitError) {
    MockUARTPort uart;
    uart.set_transmit_error(true);
    HM11Driver driver(uart);
    util::HandleType handle("0021");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.read_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, ReadCharacteristicNotify_ReceiveTimeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::HandleType handle("0022");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.read_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, ReadCharacteristicNotify_InvalidResponse) {
    MockUARTPort uart;
    uart.set_next_receive("GARBAGE\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("0023");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.read_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
    EXPECT_EQ(status, Status::InvalidResponse);
}

TEST(HM11DriverAdvancedErrorTest, ReadCharacteristicNotify_DataError) {
    MockUARTPort uart;
    uart.set_next_receive("OK+DATA-ER\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("0024");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.read_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
}

// ========== Power Tests - Error Paths ==========

TEST(HM11DriverAdvancedErrorTest, SetModulePower_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_module_power(ModulePower::Dbm0, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetModulePower_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    ModulePower power = ModulePower::Dbm0;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_module_power(power, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, SetOutputPower_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_output_power(OutputPower::MaxPower, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedErrorTest, GetOutputPower_Timeout) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    OutputPower power = OutputPower::NormalPower;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_output_power(power, status));
    EXPECT_NE(status, Status::Ok);
}

// ========== Additional SendDataMethod Tests ==========

TEST(HM11DriverAdvancedErrorTest, SetMethodAndCharacteristicHandle_AllMethods) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    util::HandleType handle("0025");
    Status status = Status::Ok;

    // Test WriteWithoutResponse (1)
    uart.set_next_receive("OK+SEND-OK\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.set_method_and_characteristic_handle(handle, SendDataMethod::WriteWithoutResponse, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SET_WAY10025\r\n");

    // Test Indicate (2)
    uart.set_next_receive("OK+SEND-OK\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.set_method_and_characteristic_handle(handle, SendDataMethod::Indicate, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SET_WAY20025\r\n");
}

// ========== CharacteristicUUIDCount Tests ==========

TEST(HM11DriverAdvancedErrorTest, SetUseCharacteristicUUIDCount_AllValues) {
    MockUARTPort uart;
    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Test OneAndNext
    uart.set_next_receive(std::string(OK_SET) + "2\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.set_use_characteristic_uuid_count(CharacteristicUUIDCount::OneAndNext, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FFE22\r\n");

    // Test OneAndPrev
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    status = Status::Ok;
    EXPECT_TRUE(driver.set_use_characteristic_uuid_count(CharacteristicUUIDCount::OneAndPrev, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FFE23\r\n");
}
