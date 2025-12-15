// Coverage Recovery Tests
// Tests to recover lost coverage from deleted test files
// Targets specific uncovered error paths identified in coverage analysis

#include <gtest/gtest.h>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11_driver.hpp>

using namespace hm11;
using namespace hm11::hal;

class CoverageRecoveryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uart = std::make_unique<MockUART>();
    }

    std::unique_ptr<MockUART> mock_uart;

    auto make_driver() {
        return HM11Driver<MockUART>(std::move(*mock_uart));
    }
};

// ============================================================================
// Transmit Error Paths - Early returns when Transmit fails
// ============================================================================

TEST_F(CoverageRecoveryTest, SoftwareVersion_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();

    auto version = driver.SoftwareVersion();
    EXPECT_TRUE(version.empty());
}

TEST_F(CoverageRecoveryTest, SoftwareVersion_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();

    auto version = driver.SoftwareVersion();
    EXPECT_TRUE(version.empty());
}

TEST_F(CoverageRecoveryTest, GetPinCode_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    PinType pin;

    driver.GetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetPinCode_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    PinType pin;

    driver.GetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetBondMode_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    BondMode mode{};

    driver.GetBondMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetBondMode_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    BondMode mode{};

    driver.GetBondMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetWorkType_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkType type{};

    driver.GetWorkType(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetWorkType_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkType type{};

    driver.GetWorkType(type, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetWorkMode_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkMode mode{};

    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetWorkMode_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkMode mode{};

    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetDiscoveryTime_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    DiscoveryTime time;

    driver.GetDiscoveryTime(time, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetDiscoveryTime_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    DiscoveryTime time;

    driver.GetDiscoveryTime(time, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetShowDeviceInformationWhenDiscovery_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ShowDeviceInformation show;

    driver.GetShowDeviceInformationWhenDiscovery(show, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetShowDeviceInformationWhenDiscovery_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ShowDeviceInformation show;

    driver.GetShowDeviceInformationWhenDiscovery(show, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetNotifyInformation_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.GetNotifyInformation(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetNotifyMode_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    NotifyMode mode{};

    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetNotifyMode_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    NotifyMode mode{};

    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetModuleName_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    std::string name;

    driver.GetModuleName(name, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetModuleRxGain_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    RxGain gain{};

    driver.GetModuleRxGain(gain, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetModuleRxGain_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    RxGain gain{};

    driver.GetModuleRxGain(gain, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetServiceUuid_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    UUID uuid;

    driver.GetServiceUuid(uuid, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetCharacteristic_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    CharacteristicType characteristic;

    driver.GetCharacteristic(characteristic, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetCharacteristic_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    CharacteristicType characteristic;

    driver.GetCharacteristic(characteristic, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetLastConnectedDeviceAddress_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    MacAddress mac;

    driver.GetLastConnectedDeviceAddress(mac, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetLastConnectedDeviceAddress_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    MacAddress mac;

    driver.GetLastConnectedDeviceAddress(mac, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetSaveConnectedMac_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.GetSaveConnectedMac(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetSaveConnectedMac_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.GetSaveConnectedMac(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetConnectRemoteDeviceTimeout_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ConnectTimeout timeout;

    driver.GetConnectRemoteDeviceTimeout(timeout, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetConnectRemoteDeviceTimeout_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ConnectTimeout timeout;

    driver.GetConnectRemoteDeviceTimeout(timeout, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetModulePower_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ModulePower power{};

    driver.get_module_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetModulePower_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ModulePower power{};

    driver.get_module_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetOutputPower_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    OutputPower power{};

    driver.get_output_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetOutputPower_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    OutputPower power{};

    driver.get_output_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetModuleAutoSleep_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.get_module_auto_sleep(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetModuleAutoSleep_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.get_module_auto_sleep(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetAdvertisingInterval_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingInterval interval{};

    driver.get_advertising_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetAdvertisingInterval_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingInterval interval{};

    driver.get_advertising_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetAdvertisingType_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingType type{};

    driver.get_advertising_type(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetAdvertisingType_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingType type{};

    driver.get_advertising_type(type, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetReliableAdvertisingMode_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingMode mode{};

    driver.get_reliable_advertising_mode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetReliableAdvertisingMode_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingMode mode{};

    driver.get_reliable_advertising_mode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoverageRecoveryTest, GetUartBaudRate_TransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    UartBaudRate baud{};

    driver.get_uart_baud_rate(baud, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoverageRecoveryTest, GetUartBaudRate_ReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    UartBaudRate baud{};

    driver.get_uart_baud_rate(baud, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

// ============================================================================
// VariableString assign() - Character copy loop coverage
// ============================================================================

TEST_F(CoverageRecoveryTest, VariableString_AssignStringView) {
    VariableString<32> str;
    str.assign("HelloWorld");

    EXPECT_EQ(str.view(), "HelloWorld");
    EXPECT_EQ(str.size(), 10);
}

TEST_F(CoverageRecoveryTest, VariableString_AssignEmptyString) {
    VariableString<32> str;
    str.append("Initial");
    str.assign("");

    EXPECT_EQ(str.view(), "");
    EXPECT_EQ(str.size(), 0);
}

TEST_F(CoverageRecoveryTest, VariableString_AssignSingleChar) {
    VariableString<32> str;
    str.assign("X");

    EXPECT_EQ(str.view(), "X");
    EXPECT_EQ(str.size(), 1);
}

TEST_F(CoverageRecoveryTest, VariableString_AssignMaxCapacity) {
    VariableString<16> str;
    str.assign("0123456789ABCDEF");

    EXPECT_EQ(str.view(), "0123456789ABCDEF");
    EXPECT_EQ(str.size(), 16);
}
