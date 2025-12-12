#include <gtest/gtest.h>

#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

TEST(HM11DriverTest, CanInstantiate) {
    hm11::MockUARTPort uart;
    hm11::HM11Driver driver(uart);
    (void)driver;
}
