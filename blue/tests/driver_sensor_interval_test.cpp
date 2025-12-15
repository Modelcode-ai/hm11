#include <gtest/gtest.h>

#include <array>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

TEST(HM11DriverSensorIntervalTest, SetSensorWorkIntervalSingleChar) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "5\r\n");
    HM11Driver driver(uart);
    HM11Driver::WorkInterval interval = {'0', '5'}; // represents "05"
    EXPECT_TRUE(driver.set_sensor_work_interval(interval));
    // Expected command: AT+RAT5\r\n (since first char is '0', use second char only)
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RAT5\r\n");
}

TEST(HM11DriverSensorIntervalTest, SetSensorWorkIntervalTwoChars) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "12\r\n");
    HM11Driver driver(uart);
    HM11Driver::WorkInterval interval = {'1', '2'}; // "12"
    EXPECT_TRUE(driver.set_sensor_work_interval(interval));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RAT12\r\n");
}

TEST(HM11DriverSensorIntervalTest, GetSensorWorkIntervalFails) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "\r\n");
    HM11Driver driver(uart);
    HM11Driver::WorkInterval interval = {'0', '0'};
    // Since no simulated response, get should return false and command should be sent
    EXPECT_FALSE(driver.get_sensor_work_interval(interval));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+RAT??\r\n");
}
