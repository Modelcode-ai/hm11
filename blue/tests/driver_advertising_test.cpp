#include <gtest/gtest.h>

#include <array>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/types/enums.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Advertising Interval Tests ==========

TEST(HM11DriverAdvertisingTest, SetAdvertisingInterval_ms100) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_advertising_interval(AdvertisingInterval::ms100, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ADVI0\r\n");
}

TEST(HM11DriverAdvertisingTest, SetAdvertisingInterval_ms7000) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "F\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_advertising_interval(AdvertisingInterval::ms7000, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ADVIF\r\n");
}

TEST(HM11DriverAdvertisingTest, GetAdvertisingInterval_Numeric) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "5\r\n");
    HM11Driver driver(uart);
    AdvertisingInterval interval;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_advertising_interval(interval, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(interval, AdvertisingInterval::ms546);
}

TEST(HM11DriverAdvertisingTest, GetAdvertisingInterval_Hex) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "A\r\n");
    HM11Driver driver(uart);
    AdvertisingInterval interval;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_advertising_interval(interval, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(interval, AdvertisingInterval::ms2000);
}

// ========== Advertising Type Tests ==========

TEST(HM11DriverAdvertisingTest, SetAdvertisingType_Connectable) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_advertising_type(AdvertisingType::AdvertisingScanResponseConnectable, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ADTY0\r\n");
}

TEST(HM11DriverAdvertisingTest, SetAdvertisingType_Advertising) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "3\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_advertising_type(AdvertisingType::Advertising, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ADTY3\r\n");
}

TEST(HM11DriverAdvertisingTest, GetAdvertisingType) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "2\r\n");
    HM11Driver driver(uart);
    AdvertisingType type;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_advertising_type(type, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(type, AdvertisingType::AdvertisingScanResponse);
}

// ========== Advertising Flag Test ==========

TEST(HM11DriverAdvertisingTest, SetAdvertisingFlag) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "06\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    util::AdvertisingFlag flag("06");
    EXPECT_TRUE(driver.set_advertising_flag(flag, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+FLAG06\r\n");
}

// ========== Temperature Information Test ==========

TEST(HM11DriverAdvertisingTest, SetTemperatureInformation) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0x1A\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    util::AdvertisingTemperatureType temp("1A");
    EXPECT_TRUE(driver.set_temperature_information_byte_in_advertising(temp, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TEMP0x1A\r\n");
}

// ========== Humidity Information Test ==========

TEST(HM11DriverAdvertisingTest, SetHumidityInformation) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "3C\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    util::HumiInformation humi("3C");
    EXPECT_TRUE(driver.set_humi_information_byte_in_advertising(humi, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+HUMI3C\r\n");
}

// ========== Advertisement Data Test ==========

TEST(HM11DriverAdvertisingTest, SetModuleAdvertisementData) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0102030405AA\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    util::AdvertisementData data("0102030405AA");
    EXPECT_TRUE(driver.set_module_advertisement_data(data, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PACK0102030405AA\r\n");
}

// ========== Reliable Advertising Mode Tests ==========

TEST(HM11DriverAdvertisingTest, SetReliableAdvertisingMode_Normal) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_reliable_advertising_mode(AdvertisingMode::NormalAdvertising, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RELI0\r\n");
}

TEST(HM11DriverAdvertisingTest, SetReliableAdvertisingMode_Reliable) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_reliable_advertising_mode(AdvertisingMode::ReliableAdvertising, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RELI1\r\n");
}

TEST(HM11DriverAdvertisingTest, GetReliableAdvertisingMode_Normal) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "0\r\n");
    HM11Driver driver(uart);
    AdvertisingMode mode;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_reliable_advertising_mode(mode, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(mode, AdvertisingMode::NormalAdvertising);
}

TEST(HM11DriverAdvertisingTest, GetReliableAdvertisingMode_Reliable) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    AdvertisingMode mode;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_reliable_advertising_mode(mode, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(mode, AdvertisingMode::ReliableAdvertising);
}

// ========== Error Handling Tests ==========

TEST(HM11DriverAdvertisingTest, SetAdvertisingInterval_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_advertising_interval(AdvertisingInterval::ms100, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvertisingTest, GetAdvertisingInterval_Error) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    AdvertisingInterval interval;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_advertising_interval(interval, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverAdvertisingTest, GetAdvertisingInterval_InvalidChar) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "Z\r\n");  // Invalid character
    HM11Driver driver(uart);
    AdvertisingInterval interval;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_advertising_interval(interval, status));
    EXPECT_EQ(status, Status::Error);
}
