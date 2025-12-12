#include <gtest/gtest.h>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/types/enums.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Module Config Tests ==========

TEST(HM11DriverModuleConfigTest, SetWorkType_StartImmediately) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_work_type(WorkType::StartImmediately, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+IMME0\r\n");
}

TEST(HM11DriverModuleConfigTest, SetWorkType_RespondATCommand) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_work_type(WorkType::RespondATCommand, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+IMME1\r\n");
}

TEST(HM11DriverModuleConfigTest, GetWorkType) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    WorkType type = WorkType::StartImmediately;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_work_type(type, status));
    EXPECT_EQ(type, WorkType::RespondATCommand);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+IMME?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetWorkMode_TransmissionMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_work_mode(WorkMode::TransmissionMode, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MODE0\r\n");
}

TEST(HM11DriverModuleConfigTest, SetWorkMode_PIOCollectionMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_work_mode(WorkMode::PIOCollectionMode, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MODE1\r\n");
}

TEST(HM11DriverModuleConfigTest, SetWorkMode_RemoteControlMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "2\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_work_mode(WorkMode::RemoteControlMode, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MODE2\r\n");
}

TEST(HM11DriverModuleConfigTest, GetWorkMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "2\r\n");
    HM11Driver driver(uart);
    WorkMode mode = WorkMode::TransmissionMode;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_work_mode(mode, status));
    EXPECT_EQ(mode, WorkMode::RemoteControlMode);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MODE?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetNotifyMode_WithoutAddress) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_notify_mode(NotifyMode::Without_Address, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTP0\r\n");
}

TEST(HM11DriverModuleConfigTest, SetNotifyMode_WithAddress) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_notify_mode(NotifyMode::WithAddress, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTP1\r\n");
}

TEST(HM11DriverModuleConfigTest, GetNotifyMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    NotifyMode mode = NotifyMode::Without_Address;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_notify_mode(mode, status));
    EXPECT_EQ(mode, NotifyMode::WithAddress);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTP?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetModuleName) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "HM11_TEST\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_name("HM11_TEST", status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NAMEHM11_TEST\r\n");
}

TEST(HM11DriverModuleConfigTest, GetModuleName) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "HM11_TEST\r\n");
    HM11Driver driver(uart);
    std::string name;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_name(name, status));
    EXPECT_EQ(name, "HM11_TEST");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NAME?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetNotifyInformation_True) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_notify_information(true, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTI1\r\n");
}

TEST(HM11DriverModuleConfigTest, SetNotifyInformation_False) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_notify_information(false, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTI0\r\n");
}

TEST(HM11DriverModuleConfigTest, GetNotifyInformation) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    bool notify = false;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_notify_information(notify, status));
    EXPECT_TRUE(notify);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+NOTI?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetModuleRXGain_No) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_rx_gain(RXGain::No, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+GAIN0\r\n");
}

TEST(HM11DriverModuleConfigTest, SetModuleRXGain_Open) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_module_rx_gain(RXGain::Open, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+GAIN1\r\n");
}

TEST(HM11DriverModuleConfigTest, GetModuleRXGain) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    RXGain gain = RXGain::No;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_module_rx_gain(gain, status));
    EXPECT_EQ(gain, RXGain::Open);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+GAIN?\r\n");
}

// ========== PIO Control Tests ==========

TEST(HM11DriverModuleConfigTest, SetPIOOutputStatus_PIO1_High) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pio_output_status(util::PioNumber(1), PIOOutput::High, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO11\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPIOOutputStatus_PIO1_Low) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pio_output_status(util::PioNumber(1), PIOOutput::Low, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO10\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPIOOutputStatus_PIO2_High) {
    MockUARTPort uart;
    uart.set_next_receive("OK+PIO2:1\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pio_output_status(util::PioNumber(2), PIOOutput::High, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO21\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPIOOutputStatus_PIO3_Low) {
    MockUARTPort uart;
    uart.set_next_receive("OK+PIO3:0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pio_output_status(util::PioNumber(3), PIOOutput::Low, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO30\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPIOOutputStatus_PIO1) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1\r\n");
    HM11Driver driver(uart);
    PIOOutput output = PIOOutput::Low;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_pio_output_status(util::PioNumber(1), output, status));
    EXPECT_EQ(output, PIOOutput::High);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO1?\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPIOOutputStatus_PIO2) {
    MockUARTPort uart;
    uart.set_next_receive("OK+PIO2:0\r\n");
    HM11Driver driver(uart);
    PIOOutput output = PIOOutput::High;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_pio_output_status(util::PioNumber(2), output, status));
    EXPECT_EQ(output, PIOOutput::Low);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO2?\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPIOsOutputStatus) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "1A3\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_pios_output_status(pios, status));
    EXPECT_EQ(std::string(pios.view()), "1A3");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PIO??\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPIOsOutputStatus) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "2B4\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("2B4");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pios_output_status(pios, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+MPIO2B4\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPIOCollectionRate) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "50\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pio_collection_rate(util::PioCollectionRate(50), status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CYC50\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPIOCollectionRate) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "25\r\n");
    HM11Driver driver(uart);
    util::PioCollectionRate rate(0);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_pio_collection_rate(rate, status));
    EXPECT_EQ(static_cast<int>(rate), 25);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+CYC?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPowerPinOutput) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "123\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("123");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_power_pin_output(pios, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+BEFC123\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPowerPinOutput) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "ABC\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_power_pin_output(pios, status));
    EXPECT_EQ(std::string(pios.view()), "ABC");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+BEFC?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetConnectPinOutput) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "456\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("456");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_connect_pin_output(pios, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+AFTC456\r\n");
}

TEST(HM11DriverModuleConfigTest, GetConnectPinOutput) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "DEF\r\n");
    HM11Driver driver(uart);
    util::PIONumbers pios("000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_connect_pin_output(pios, status));
    EXPECT_EQ(std::string(pios.view()), "DEF");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+AFTC?\r\n");
}

// ========== Security Tests ==========

TEST(HM11DriverModuleConfigTest, SetBondMode_NotNeedPIN) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "0\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_bond_mode(BondMode::NotNeedPIN, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TYPE0\r\n");
}

TEST(HM11DriverModuleConfigTest, SetBondMode_AuthWithPIN) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "2\r\n");
    HM11Driver driver(uart);
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_bond_mode(BondMode::AuthWithPIN, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TYPE2\r\n");
}

TEST(HM11DriverModuleConfigTest, GetBondMode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "3\r\n");
    HM11Driver driver(uart);
    BondMode mode = BondMode::NotNeedPIN;
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_bond_mode(mode, status));
    EXPECT_EQ(mode, BondMode::AuthAndBonded);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+TYPE?\r\n");
}

TEST(HM11DriverModuleConfigTest, SetPINCode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_SET + "123456\r\n");
    HM11Driver driver(uart);
    util::PINType pin("123456");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.set_pin_code(pin, status));
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PASS123456\r\n");
}

TEST(HM11DriverModuleConfigTest, GetPINCode) {
    MockUARTPort uart;
    uart.set_next_receive(OK_GET + "654321\r\n");
    HM11Driver driver(uart);
    util::PINType pin("000000");
    Status status = Status::Ok;
    EXPECT_TRUE(driver.get_pin_code(pin, status));
    EXPECT_EQ(std::string(pin.view()), "654321");
    EXPECT_EQ(uart.get_transmitted_data(), "AT+PASS?\r\n");
}
