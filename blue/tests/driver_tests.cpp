#include <gtest/gtest.h>

#include <array>

#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

class HM11DriverTest : public ::testing::Test {
  protected:
    MockUARTPort uart;
    HM11Driver driver{uart};
};

TEST_F(HM11DriverTest, CanInstantiate) {
    // Just verify construction works
}

// ========== Basic Communication Tests ==========

TEST_F(HM11DriverTest, SendSuccess) {
    Status status = Status::Error;
    bool result = driver.send("TEST", status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "TEST");
}

TEST_F(HM11DriverTest, SendError) {
    uart.set_transmit_error(true);
    Status status = Status::Ok;
    bool result = driver.send("TEST", status);
    EXPECT_FALSE(result);
    EXPECT_EQ(status, Status::Error);
}

TEST_F(HM11DriverTest, ReceiveSuccess) {
    uart.set_next_receive("RESPONSE");
    Status status = Status::Error;
    std::string_view response;
    bool result = driver.receive(response, status, 100);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(response, "RESPONSE");
}

TEST_F(HM11DriverTest, ReceiveError) {
    // Don't set any receive data - will timeout
    Status status = Status::Ok;
    std::string_view response;
    bool result = driver.receive(response, status, 100);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
    EXPECT_TRUE(response.empty());
}

TEST_F(HM11DriverTest, ReceiveWithNullBytes) {
    std::string data = "HELLO";
    data += '\0'; // Add null byte
    data += "WORLD"; // This should be ignored
    uart.set_next_receive(data);

    Status status = Status::Error;
    std::string_view response;
    bool result = driver.receive(response, status, 100);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(response, "HELLO"); // Should stop at null byte
}

// ========== Role Tests ==========

TEST_F(HM11DriverTest, SetRolePeripheral) {
    bool result = driver.set_role(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ROLE0\r\n");
}

TEST_F(HM11DriverTest, SetRoleCentral) {
    bool result = driver.set_role(1);
    EXPECT_TRUE(result);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+ROLE1\r\n");
}

TEST_F(HM11DriverTest, SetRoleInvalid) {
    bool result = driver.set_role(2);
    EXPECT_FALSE(result);
}

TEST_F(HM11DriverTest, GetRoleSuccess) {
    uart.set_next_receive("OK+GET:1");
    Status status = Status::Ok;
    unsigned int role = 99;
    bool result = driver.get_role(role, status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(role, 1);
}

TEST_F(HM11DriverTest, GetRoleError) {
    // Don't set receive data - timeout
    Status status = Status::Ok;
    unsigned int role = 99;
    bool result = driver.get_role(role, status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

TEST_F(HM11DriverTest, GetRoleInvalidResponse) {
    uart.set_next_receive("OK+GET:X"); // Invalid role character
    Status status = Status::Ok;
    unsigned int role = 99;
    bool result = driver.get_role(role, status);
    EXPECT_FALSE(result);
}

// ========== Test Command ==========

TEST_F(HM11DriverTest, TestCommandSuccess) {
    uart.set_next_receive("OK");
    Status status = Status::Ok;
    bool result = driver.test(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "AT");
}

TEST_F(HM11DriverTest, TestCommandFailure) {
    uart.set_next_receive("ERROR");
    Status status = Status::Ok;
    bool result = driver.test(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

// ========== Software Version ==========

TEST_F(HM11DriverTest, SoftwareVersionSuccess) {
    uart.set_next_receive("HMSoft V540");
    Status status = Status::Ok;
    std::string_view version;
    bool result = driver.software_version(version, status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(version, "HMSoft V540");
}

TEST_F(HM11DriverTest, SoftwareVersionTransmitError) {
    uart.set_transmit_error(true);
    Status status = Status::Ok;
    std::string_view version;
    bool result = driver.software_version(version, status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

TEST_F(HM11DriverTest, SoftwareVersionReceiveError) {
    // Don't set receive data - timeout
    Status status = Status::Ok;
    std::string_view version;
    bool result = driver.software_version(version, status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

// ========== MAC Address ==========

TEST_F(HM11DriverTest, GetMACAddressSuccess) {
    uart.set_next_receive("OK+ADDR:001122334455");
    Status status = Status::Ok;
    std::array<char, 12> mac{};
    bool result = driver.get_mac_address(mac, status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    std::string mac_str(mac.data(), 12);
    EXPECT_EQ(mac_str, "001122334455");
}

TEST_F(HM11DriverTest, GetMACAddressError) {
    Status status = Status::Ok;
    std::array<char, 12> mac{};
    bool result = driver.get_mac_address(mac, status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

// ========== Reset/Restart Commands ==========

TEST_F(HM11DriverTest, ResetSuccess) {
    uart.set_next_receive("OK+RENEW");
    Status status = Status::Ok;
    bool result = driver.reset(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
}

TEST_F(HM11DriverTest, ResetError) {
    Status status = Status::Ok;
    bool result = driver.reset(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

TEST_F(HM11DriverTest, RestartSuccess) {
    uart.set_next_receive("OK+RESET");
    Status status = Status::Ok;
    bool result = driver.restart(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
}

TEST_F(HM11DriverTest, RestartError) {
    Status status = Status::Ok;
    bool result = driver.restart(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

// ========== Connection State Commands ==========

TEST_F(HM11DriverTest, DisconnectSuccess) {
    uart.set_next_receive("OK+LOST");
    Status status = Status::Ok;
    bool result = driver.disconnect(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
}

TEST_F(HM11DriverTest, DisconnectError) {
    Status status = Status::Ok;
    bool result = driver.disconnect(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

TEST_F(HM11DriverTest, StartWorkingSuccess) {
    uart.set_next_receive("OK+START");
    Status status = Status::Ok;
    bool result = driver.start_working(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
}

TEST_F(HM11DriverTest, StartWorkingError) {
    Status status = Status::Ok;
    bool result = driver.start_working(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

// ========== Sleep/Wake Commands ==========

TEST_F(HM11DriverTest, SleepSuccess) {
    uart.set_next_receive("OK+SLEEP");
    Status status = Status::Ok;
    bool result = driver.sleep(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
}

TEST_F(HM11DriverTest, SleepError) {
    Status status = Status::Ok;
    bool result = driver.sleep(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}

TEST_F(HM11DriverTest, WakeUpSuccess) {
    uart.set_next_receive("OK+WAKE");
    Status status = Status::Ok;
    bool result = driver.wake_up(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(uart.get_transmitted_data(), "WWWWWWW"); // 7 W's, no \r\n
}

TEST_F(HM11DriverTest, WakeUpError) {
    Status status = Status::Ok;
    bool result = driver.wake_up(status);
    EXPECT_FALSE(result);
    EXPECT_NE(status, Status::Ok);
}
