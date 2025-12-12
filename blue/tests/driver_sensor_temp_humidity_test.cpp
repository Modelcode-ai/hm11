#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

TEST(HM11DriverSensorTempHumidityTest, GetTemperatureAndHumidity) {
    MockUARTPort uart;
    // Simulate response: OK+GET:025060\r\n (temperature 25, humidity 60)
    uart.set_next_receive(OK_GET + "025060\r\n");
    HM11Driver driver(uart);
    util::Temperature temperature;
    util::Percent humidity;
    EXPECT_TRUE(driver.get_sensor_temperature_and_humidity(temperature, humidity));
    EXPECT_EQ(temperature, util::Temperature(static_cast<std::uint8_t>(25)));
    EXPECT_EQ(humidity, util::Percent(static_cast<std::uint8_t>(60)));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TEHU?\r\n");
}
