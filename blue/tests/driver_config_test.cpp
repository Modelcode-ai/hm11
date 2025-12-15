#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/types/enums.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== White List Tests ==========

TEST(HM11DriverConfigTest, SetWhiteListSwitch_Enable) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_white_list_switch(true, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ALLO1\r\n");
}

TEST(HM11DriverConfigTest, GetWhiteListSwitch) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    bool enabled = false;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_white_list_switch(enabled, status));
    EXPECT_TRUE(enabled);
}

TEST(HM11DriverConfigTest, SetWhiteListMACAddress) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "AABBCCDDEEFF\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("AABBCCDDEEFF");
    util::MacWhiteListIndex index(1);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_white_list_mac_addresses(index, mac, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+AD1AABBCCDDEEFF\r\n");
}

TEST(HM11DriverConfigTest, GetWhiteListMACAddress) {
    MockUARTPort uart;
    uart.set_next_receive("OK+AD2?:112233445566\r\n");
    HM11Driver driver(uart);
    util::MACAddress mac("000000000000");
    util::MacWhiteListIndex index(2);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_white_list_mac_address(index, mac, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+AD2??\r\n");
}

// ========== Battery Tests ==========

TEST(HM11DriverConfigTest, SetBatteryMonitorSwitch) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_battery_monitor_switch(true, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+BATC1\r\n");
}

TEST(HM11DriverConfigTest, GetBatteryMonitorSwitch) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    bool enabled = true;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_battery_monitor_switch(enabled, status));
    EXPECT_FALSE(enabled);
}

TEST(HM11DriverConfigTest, SetBatteryInformation) {
    MockUARTPort uart;
    uart.set_next_receive("OK+BATT\r\n");
    HM11Driver driver(uart);
    util::Percent value(75);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_battery_information(value, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+BATT75\r\n");
}

TEST(HM11DriverConfigTest, QueryBatteryInformation) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "085\r\n");
    HM11Driver driver(uart);
    util::Percent value(0);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.query_battery_information(value, status));
    EXPECT_EQ(static_cast<int>(value), 85);
}

// ========== UART Config Tests ==========

TEST(HM11DriverConfigTest, SetUARTBaudRate) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "4\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_uart_baud_rate(BaudRate::BR115200, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+BAUD4\r\n");
}

TEST(HM11DriverConfigTest, GetUARTBaudRate) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    BaudRate rate = BaudRate::BR115200;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_uart_baud_rate(rate, status));
    EXPECT_EQ(rate, BaudRate::BR9600);
}

TEST(HM11DriverConfigTest, SetUARTFlowControl) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_uart_flow_control_switch(true, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FIOW1\r\n");
}

TEST(HM11DriverConfigTest, GetUARTFlowControl) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    bool enabled = true;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_uart_flow_control_switch(enabled, status));
    EXPECT_FALSE(enabled);
}

TEST(HM11DriverConfigTest, SetParityBit_None) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_parity_bit(ParityBit::None, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PARI0\r\n");
}

TEST(HM11DriverConfigTest, SetParityBit_Even) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "2\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_parity_bit(ParityBit::Even, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PARI2\r\n");
}

TEST(HM11DriverConfigTest, GetParityBit) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    ParityBit parity = ParityBit::None;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_parity_bit(parity, status));
    EXPECT_EQ(parity, ParityBit::Odd);
}

TEST(HM11DriverConfigTest, SetStopBit) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_stop_bit(StopBit::TwoStopBit, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+STOP1\r\n");
}

TEST(HM11DriverConfigTest, GetStopBit) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    StopBit stop = StopBit::TwoStopBit;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_stop_bit(stop, status));
    EXPECT_EQ(stop, StopBit::OneStopBit);
}

TEST(HM11DriverConfigTest, SetUARTSleepType) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_uart_sleep_type(UARTSleepType::ShutdownUART, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PWRM1\r\n");
}

TEST(HM11DriverConfigTest, GetUARTSleepType) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    UARTSleepType type = UARTSleepType::ShutdownUART;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_uart_sleep_type(type, status));
    EXPECT_EQ(type, UARTSleepType::CanWakeUpThroughUART);
}

// ========== Link Layer Tests ==========

TEST(HM11DriverConfigTest, SetMinimumLinkLayerConnectionInterval) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval::Ms20, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COMI3\r\n");
}

TEST(HM11DriverConfigTest, GetMinimumLinkLayerConnectionInterval) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "2\r\n");
    HM11Driver driver(uart);
    LinkLayerConnectionInterval interval = LinkLayerConnectionInterval::Ms7;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_minimum_link_layer_connection_interval(interval, status));
    EXPECT_EQ(interval, LinkLayerConnectionInterval::Ms15);
}

TEST(HM11DriverConfigTest, SetMaximumLinkLayerConnectionInterval) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "5\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval::Ms30, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COMA5\r\n");
}

TEST(HM11DriverConfigTest, GetMaximumLinkLayerConnectionInterval) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "4\r\n");
    HM11Driver driver(uart);
    LinkLayerConnectionInterval interval = LinkLayerConnectionInterval::Ms7;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_maximum_link_layer_connection_interval(interval, status));
    EXPECT_EQ(interval, LinkLayerConnectionInterval::Ms25);
}

TEST(HM11DriverConfigTest, SetSlaveLatency) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "3\r\n");
    HM11Driver driver(uart);
    util::LayerConnectionLatency latency(3);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_link_layer_connection_slave_latency(latency, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COLA3\r\n");
}

TEST(HM11DriverConfigTest, GetSlaveLatency) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "2\r\n");
    HM11Driver driver(uart);
    util::LayerConnectionLatency latency(0);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_link_layer_connection_slave_latency(latency, status));
    EXPECT_EQ(static_cast<int>(latency), 2);
}

TEST(HM11DriverConfigTest, SetConnectionSupervisionTimeout) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "5\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_connection_supervision_timeout(ConnectionSupervisionTimeout::Ms5000, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COSU5\r\n");
}

TEST(HM11DriverConfigTest, GetConnectionSupervisionTimeout) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "3\r\n");
    HM11Driver driver(uart);
    ConnectionSupervisionTimeout timeout = ConnectionSupervisionTimeout::Ms100;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_connection_supervision_timeout(timeout, status));
    EXPECT_EQ(timeout, ConnectionSupervisionTimeout::Ms3000);
}

TEST(HM11DriverConfigTest, SetUpdateConnection_Enable) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_update_connection(true, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+COUP1\r\n");
}

TEST(HM11DriverConfigTest, GetUpdateConnection) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "0\r\n");
    HM11Driver driver(uart);
    bool enabled = true;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_update_connection(enabled, status));
    EXPECT_FALSE(enabled);
}

// ========== Error Handling Tests ==========

TEST(HM11DriverConfigTest, SetWhiteListSwitch_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_white_list_switch(true, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverConfigTest, GetBatteryInfo_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::Percent value(0);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.query_battery_information(value, status));
    EXPECT_NE(status, Status::Ok);
}
