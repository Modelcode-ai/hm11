#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/tests/utils/mock_uart_port.hpp"

using namespace hm11;

TEST(HM11DriverIBeaconVersionTest, SetMajorVersion) {
    MockUARTPort uart;
    // Mock response for successful SET command
    uart.set_next_receive(OK_SET + "\r\n");
    HM11Driver driver(uart);
    std::array<char, 4> major = {'F', 'F', 'E', '0'}; // example hex string
    EXPECT_TRUE(driver.set_ibeacon_major(major));
    // Expected AT command: AT+MAJ0xFFE0\r\n
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MAJ0xFFE0\r\n");
}

TEST(HM11DriverIBeaconVersionTest, SetMinorVersion) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "\r\n");
    HM11Driver driver(uart);
    std::array<char, 4> minor = {'0', '0', '0', '1'}; // example
    EXPECT_TRUE(driver.set_ibeacon_minor(minor));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MINO0x0001\r\n");
}

TEST(HM11DriverIBeaconVersionTest, SetMeasuredPower) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "\r\n");
    HM11Driver driver(uart);
    std::array<char, 2> power = {'C', '5'}; // example hex
    EXPECT_TRUE(driver.set_ibeacon_measured_power(power));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MEAS0xC5\r\n");
}
