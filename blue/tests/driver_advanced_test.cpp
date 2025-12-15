#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/types/enums.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Connection Tests ==========

TEST(HM11DriverAdvancedTest, ClearLastConnectedAddress) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CLEAR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.clear_last_connected_address(status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CLEAR\r\n");
}

TEST(HM11DriverAdvancedTest, ConnectLastDevice_Connected) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN1\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::Connected);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CONNL\r\n");
}

TEST(HM11DriverAdvancedTest, ConnectLastDevice_Connecting) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN0\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::Connecting);
}

TEST(HM11DriverAdvancedTest, ConnectByMAC_Normal) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN1\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("112233445566");
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::NormalAddress, mac, result, status));
    EXPECT_EQ(result, ConnectResult::Connected);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CON112233445566\r\n");
}

TEST(HM11DriverAdvancedTest, ConnectByMAC_Static) {
    MockUARTPort uart;
    uart.set_next_receive("OK+COSS2\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("AABBCCDDEEFF");
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect(MACAddressType::StaticMAC, mac, result, status));
    EXPECT_EQ(result, ConnectResult::ConnectError);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COSAABBCCDDEEFF\r\n");
}

TEST(HM11DriverAdvancedTest, ConnectByIndex) {
    MockUARTPort uart;
    uart.set_next_receive("OK+CONN1\r\n");
    HM11Driver driver(uart);
    util::DiscoveryIndex index(2);
    ConnectResult result = ConnectResult::OtherError;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.connect(index, result, status));
    EXPECT_EQ(result, ConnectResult::Connected);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CONN2\r\n");
}

TEST(HM11DriverAdvancedTest, GetLastConnectedDeviceAddress) {
    MockUARTPort uart;
    uart.set_next_receive("OK+RADD:112233445566\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("000000000000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_last_connected_device_address(mac, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RADD?\r\n");
    EXPECT_EQ(std::string(mac.view()), "112233445566");
}

TEST(HM11DriverAdvancedTest, SetSaveConnectedMAC_True) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_save_connected_mac(true, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SAVE0\r\n");
}

TEST(HM11DriverAdvancedTest, SetSaveConnectedMAC_False) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_save_connected_mac(false, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SAVE1\r\n");
}

TEST(HM11DriverAdvancedTest, GetSaveConnectedMAC_True) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    bool save = false;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_save_connected_mac(save, status));
    EXPECT_TRUE(save);
}

TEST(HM11DriverAdvancedTest, GetSaveConnectedMAC_False) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    bool save = true;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_save_connected_mac(save, status));
    EXPECT_FALSE(save);
}

TEST(HM11DriverAdvancedTest, SetConnectRemoteDeviceTimeout) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "005000\r\n");
    HM11Driver driver(uart);
    util::ConnectTimeout timeout("005000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_connect_remote_device_timeout(timeout, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TCON005000\r\n");
}

TEST(HM11DriverAdvancedTest, GetConnectRemoteDeviceTimeout) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "010000\r\n");
    HM11Driver driver(uart);
    util::ConnectTimeout timeout("000000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_connect_remote_device_timeout(timeout, status));
    EXPECT_EQ(std::string(timeout.view()), "010000");
}

// ========== Discovery Tests ==========

TEST(HM11DriverAdvancedTest, SetDiscoveryTime) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    HM11Driver driver(uart);
    util::DiscoveryTime time(3);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_discovery_time(time, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SCAN3\r\n");
}

TEST(HM11DriverAdvancedTest, GetDiscoveryTime) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "5\r\n");
    HM11Driver driver(uart);
    util::DiscoveryTime time(1);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_discovery_time(time, status));
    EXPECT_EQ(static_cast<int>(time), 5);
}

TEST(HM11DriverAdvancedTest, SetShowDeviceInformation_ShowName) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowName, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SHOW1\r\n");
}

TEST(HM11DriverAdvancedTest, SetShowDeviceInformation_ShowRSSIAndName) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowRSSIAndName, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SHOW3\r\n");
}

TEST(HM11DriverAdvancedTest, GetShowDeviceInformation) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "2\r\n");
    HM11Driver driver(uart);
    ShowDeviceInformation show = ShowDeviceInformation::DontShow;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_show_device_information_when_discovery(show, status));
    EXPECT_EQ(show, ShowDeviceInformation::ShowRSSI);
}

// ========== GATT Tests ==========

TEST(HM11DriverAdvancedTest, EnableCharacteristicNotify_Success) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SEND-OK\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001A");
    NotifyResponse response = NotifyResponse::DataEr;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.enable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::SendOk);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTIFY_ON001A\r\n");
}

TEST(HM11DriverAdvancedTest, EnableCharacteristicNotify_Error) {
    MockUARTPort uart;
    uart.set_next_receive("OK+DATA-ER\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001B");
    NotifyResponse response = NotifyResponse::SendOk;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.enable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::DataEr);
}

TEST(HM11DriverAdvancedTest, DisableCharacteristicNotify_Success) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SEND-OK\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001C");
    NotifyResponse response = NotifyResponse::DataEr;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.disable_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::SendOk);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTIFYOFF001C\r\n");
}

TEST(HM11DriverAdvancedTest, ReadCharacteristicNotify_Success) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SEND-OK\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001D");
    NotifyResponse response = NotifyResponse::DataEr;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.read_characteristic_notify(handle, response, status));
    EXPECT_EQ(response, NotifyResponse::SendOk);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+READDATA001D\r\n");
}

TEST(HM11DriverAdvancedTest, SetMethodAndCharacteristicHandle_Write) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SEND-OK\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001E");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_method_and_characteristic_handle(handle, SendDataMethod::Write, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SET_WAY0001E\r\n");
}

TEST(HM11DriverAdvancedTest, SetMethodAndCharacteristicHandle_Notify) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SEND-OK\r\n");
    HM11Driver driver(uart);
    util::HandleType handle("001F");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_method_and_characteristic_handle(handle, SendDataMethod::Notify, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SET_WAY3001F\r\n");
}

TEST(HM11DriverAdvancedTest, SetUseCharacteristicUUIDCount_Query) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "?\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_use_characteristic_uuid_count(CharacteristicUUIDCount::Query, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FFE2?\r\n");
}

TEST(HM11DriverAdvancedTest, SetUseCharacteristicUUIDCount_One) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_use_characteristic_uuid_count(CharacteristicUUIDCount::One, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FFE21\r\n");
}

TEST(HM11DriverAdvancedTest, SetCharacteristic) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "\r\n");
    HM11Driver driver(uart);
    util::CharacteristicType value("FFE1");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_characteristic(value, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CHAR0xFFE1\r\n");
}

TEST(HM11DriverAdvancedTest, GetCharacteristic) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0xFFE2\r\n");
    HM11Driver driver(uart);
    util::CharacteristicType result("0000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_characteristic(result, status));
    EXPECT_EQ(std::string(result.view()), "FFE2");
}

// ========== Service UUID Tests ==========

TEST(HM11DriverAdvancedTest, SetServiceUUID) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "0xFFE0\r\n");
    HM11Driver driver(uart);
    util::UUID value("FFE0");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_service_uuid(value, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+UUID0xFFE0\r\n");
}

TEST(HM11DriverAdvancedTest, GetServiceUUID) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0x1800\r\n");
    HM11Driver driver(uart);
    util::UUID result("0000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_service_uuid(result, status));
    EXPECT_EQ(std::string(result.view()), "1800");
}

// ========== Power Tests ==========

TEST(HM11DriverAdvancedTest, SetModulePower_Dbm0) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "2\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_power(ModulePower::Dbm0, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+POWE2\r\n");
}

TEST(HM11DriverAdvancedTest, SetModulePower_Dbm6) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_power(ModulePower::Dbm6, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+POWE3\r\n");
}

TEST(HM11DriverAdvancedTest, GetModulePower) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    ModulePower power = ModulePower::Dbm0;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_power(power, status));
    EXPECT_EQ(power, ModulePower::DbmMinus6);
}

TEST(HM11DriverAdvancedTest, SetOutputPower_Normal) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_output_power(OutputPower::NormalPower, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PCTL0\r\n");
}

TEST(HM11DriverAdvancedTest, SetOutputPower_Max) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_output_power(OutputPower::MaxPower, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PCTL1\r\n");
}

TEST(HM11DriverAdvancedTest, GetOutputPower) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    OutputPower power = OutputPower::NormalPower;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_output_power(power, status));
    EXPECT_EQ(power, OutputPower::MaxPower);
}

// ========== Error Handling Tests ==========

TEST(HM11DriverAdvancedTest, ClearLastConnectedAddress_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.clear_last_connected_address(status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedTest, ConnectLastDevice_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    ConnectResult result = ConnectResult::Connecting;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.connect_last_device(result, status));
    EXPECT_EQ(result, ConnectResult::OtherError);
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedTest, GetDiscoveryTime_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::DiscoveryTime time(1);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_discovery_time(time, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvancedTest, GetServiceUUID_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::UUID result("0000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_service_uuid(result, status));
    EXPECT_NE(status, Status::Ok);
}
