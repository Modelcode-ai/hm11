#include <gtest/gtest.h>

#include <array>

#include "hm11/driver.hpp"

using namespace hm11;

TEST(HM11DriverMethodRefactorTest, SetRoleCommand) {
    HM11Driver driver;
    EXPECT_TRUE(driver.set_role(1));
    EXPECT_EQ(driver.get_last_command(), "AT+ROLE=1\r\n");
}

TEST(HM11DriverMethodRefactorTest, SetIBeaconUUID) {
    HM11Driver driver;
    std::array<char, 32> uuid;
    uuid.fill('A');
    EXPECT_TRUE(driver.set_ibeacon_uuid(uuid));
}

TEST(HM11DriverMethodRefactorTest, GetIBeaconUUID) {
    HM11Driver driver;
    std::array<char, 32> uuid_out;
    EXPECT_FALSE(driver.get_ibeacon_uuid(uuid_out));
}
