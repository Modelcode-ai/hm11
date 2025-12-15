#include <gtest/gtest.h>

#include <array>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

TEST(HM11DriverSensorTypeTest, SetSensorTypeCommand) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_SET) + "1\r\n");
    HM11Driver driver(uart);
    EXPECT_TRUE(driver.set_sensor_type(HM11Driver::SensorType::DHT11));
    // Expected command: AT+SENS1\r\n (value of DHT11 is 1)
    EXPECT_EQ(uart.get_transmitted_data(), "AT+SENS1\r\n");
}

TEST(HM11DriverSensorTypeTest, GetSensorTypeDefault) {
    MockUARTPort uart;
    uart.set_next_receive(std::string(OK_GET) + "1\r\n");
    HM11Driver driver(uart);
    HM11Driver::SensorType result = HM11Driver::SensorType::None;
    EXPECT_TRUE(driver.get_sensor_type(result));
    EXPECT_EQ(result, HM11Driver::SensorType::DHT11);
}
