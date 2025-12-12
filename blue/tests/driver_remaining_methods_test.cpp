#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/util/type_traits.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Security Methods Tests ==========

TEST(HM11DriverRemainingMethodsTest, RemoveBondInformation) {
    MockUARTPort uart;
    uart.set_next_receive("OK+ERASE\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.remove_bond_information(status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ERASE\r\n");
}

TEST(HM11DriverRemainingMethodsTest, RemoveBondInformationFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.remove_bond_information(status));
    EXPECT_NE(status, Status::Ok);
}

// ========== iBeacon Query Methods Tests ==========

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMajor) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:0x1234\r\n");
    HM11Driver driver(uart);
    util::VersionType major("0000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_ibeacon_major(major, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(major.view(), "1234");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MARJ?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMajorFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::VersionType major("0000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_ibeacon_major(major, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMinor) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:0xABCD\r\n");
    HM11Driver driver(uart);
    util::VersionType minor("0000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_ibeacon_minor(minor, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(minor.view(), "ABCD");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MINO?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMinorFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::VersionType minor("0000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_ibeacon_minor(minor, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMeasuredPower) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:0xC5\r\n");
    HM11Driver driver(uart);
    util::MeasuredPower power("00");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_ibeacon_measured_power(power, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(power.view(), "C5");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MEAS?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetIBeaconMeasuredPowerFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::MeasuredPower power("00");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_ibeacon_measured_power(power, status));
    EXPECT_NE(status, Status::Ok);
}

// ========== Power Management Methods Tests ==========

TEST(HM11DriverRemainingMethodsTest, SetModuleAutoSleepTrue) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SET:0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_auto_sleep(true, status));
    EXPECT_EQ(status, Status::Ok);
    // Inverted boolean: auto_sleep=true sends "0"
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PWRM0\r\n");
}

TEST(HM11DriverRemainingMethodsTest, SetModuleAutoSleepFalse) {
    MockUARTPort uart;
    uart.set_next_receive("OK+SET:1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_auto_sleep(false, status));
    EXPECT_EQ(status, Status::Ok);
    // Inverted boolean: auto_sleep=false sends "1"
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PWRM1\r\n");
}

TEST(HM11DriverRemainingMethodsTest, SetModuleAutoSleepFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_FALSE(driver.set_module_auto_sleep(true, status));
    EXPECT_NE(status, Status::Ok);
}

TEST(HM11DriverRemainingMethodsTest, GetModuleAutoSleepTrue) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:0\r\n");
    HM11Driver driver(uart);
    bool auto_sleep = false;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_auto_sleep(auto_sleep, status));
    EXPECT_EQ(status, Status::Ok);
    // Inverted boolean: response '0' means auto_sleep=true
    EXPECT_TRUE(auto_sleep);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PWRM?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetModuleAutoSleepFalse) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:1\r\n");
    HM11Driver driver(uart);
    bool auto_sleep = true;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_auto_sleep(auto_sleep, status));
    EXPECT_EQ(status, Status::Ok);
    // Inverted boolean: response '1' means auto_sleep=false
    EXPECT_FALSE(auto_sleep);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PWRM?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetModuleAutoSleepFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    bool auto_sleep = false;
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_module_auto_sleep(auto_sleep, status));
    EXPECT_NE(status, Status::Ok);
}

// ========== HMSensor Methods Tests ==========

TEST(HM11DriverRemainingMethodsTest, GetModuleTemperature) {
    MockUARTPort uart;
    uart.set_next_receive("OK+GET:0025000\r\n");
    HM11Driver driver(uart);
    util::InternalTemperatureType temperature("0000000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_temperature(temperature, status));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(temperature.view(), "0025000");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TEMP?\r\n");
}

TEST(HM11DriverRemainingMethodsTest, GetModuleTemperatureFailure) {
    MockUARTPort uart;
    uart.set_next_receive("ERROR\r\n");
    HM11Driver driver(uart);
    util::InternalTemperatureType temperature("0000000");
    Status status = Status::Ok;
    EXPECT_FALSE(driver.get_module_temperature(temperature, status));
    EXPECT_NE(status, Status::Ok);
}
