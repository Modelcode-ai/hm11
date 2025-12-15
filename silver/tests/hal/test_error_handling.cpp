// Driver Error Path Tests
// Targets uncovered error handling paths in hm11_driver.hpp to improve coverage

#include <gtest/gtest.h>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11_driver.hpp>

using namespace hm11;
using namespace hm11::hal;

class DriverErrorPathTest : public ::testing::Test {
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
// Response Parsing Error Tests
// ============================================================================

TEST_F(DriverErrorPathTest, GetRoleInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'I', 'N', 'V', 'A', 'L', 'I', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    Role role{};

    driver.GetRole(role, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, GetRoleEmptyResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    Role role{};

    driver.GetRole(role, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, GetPinCodeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0', '0', '0', '0', '0', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    PinType pin;

    driver.GetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, GetPinCodeTooShort) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '0', '0', '0', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    PinType pin;

    driver.GetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, GetBondModeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'B', 'A', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    BondMode mode{};

    driver.GetBondMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, GetWorkTypeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    WorkType type{};

    driver.GetWorkType(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// UART Error Propagation Tests
// ============================================================================

TEST_F(DriverErrorPathTest, TransmitErrorPropagation) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, ReceiveErrorPropagation) {
    mock_uart->SetForcedStatus(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    Role role{};

    driver.GetRole(role, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(DriverErrorPathTest, TimeoutErrorPropagation) {
    mock_uart->SetForcedStatus(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

// ============================================================================
// Response Mismatch Tests
// ============================================================================

TEST_F(DriverErrorPathTest, SetRoleWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetRole(Role::Peripheral, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, SetBondModeWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '9'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetBondMode(BondMode::NotNeedPin, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathTest, TestOperationWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'F', 'A', 'I', 'L'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}



#include <gtest/gtest.h>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11_driver.hpp>

using namespace hm11;
using namespace hm11::hal;

class DriverErrorPathExtendedTest : public ::testing::Test {
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
// Additional Parsing Error Tests
// ============================================================================

TEST_F(DriverErrorPathExtendedTest, GetNotifyInformationInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'X', 'Y', 'Z'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    bool enabled = false;

    driver.GetNotifyInformation(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetNotifyModeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'I', 'N', 'V', 'A', 'L', 'I', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    NotifyMode mode{};

    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetModuleNameInvalidPrefix) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'W', 'R', 'O', 'N', 'G'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    std::string name;

    driver.GetModuleName(name, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetModuleRxGainInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'B', 'A', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    RxGain gain{};

    driver.GetModuleRxGain(gain, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetWorkModeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    WorkMode mode{};

    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetDiscoveryTimeInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'Z'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    DiscoveryTime time;

    driver.GetDiscoveryTime(time, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetShowDeviceInformationWhenDiscoveryInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    ShowDeviceInformation show;

    driver.GetShowDeviceInformationWhenDiscovery(show, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetShowDeviceInformationWhenDiscoveryBadPrefix) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'B', 'A', 'D', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    ShowDeviceInformation show;

    driver.GetShowDeviceInformationWhenDiscovery(show, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetServiceUuidInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'W', 'R', 'O', 'N', 'G'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    UUID uuid;

    driver.GetServiceUuid(uuid, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetCharacteristicInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'B', 'A', 'D', 'D', 'A', 'T', 'A'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    CharacteristicType characteristic;

    driver.GetCharacteristic(characteristic, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetLastConnectedDeviceAddressInvalidFormat) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'C', 'O', 'N', 'N', 'E', 'C', 'T', 'E', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    MacAddress mac;

    driver.GetLastConnectedDeviceAddress(mac, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetSaveConnectedMacInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'X', 'X', 'X'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    bool enabled = false;

    driver.GetSaveConnectedMac(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetConnectRemoteDeviceTimeoutInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X', 'X'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    ConnectTimeout timeout;

    driver.GetConnectRemoteDeviceTimeout(timeout, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetModulePowerInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    ModulePower power{};

    driver.get_module_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetOutputPowerInvalidResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    OutputPower power{};

    driver.get_output_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Set Operation Response Mismatch Tests
// ============================================================================

TEST_F(DriverErrorPathExtendedTest, SetNotifyInformationWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetNotifyInformation(true, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetNotifyModeWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'W', 'R', 'O', 'N', 'G'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetNotifyMode(NotifyMode::WithoutAddress, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetModuleNameWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'E', 'T'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetModuleName("TestName", status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetModuleRxGainWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'F', 'A', 'I', 'L'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetModuleRxGain(RxGain::Open, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetWorkTypeWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetWorkType(WorkType::StartImmediately, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetWorkModeWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetWorkMode(WorkMode::TransmissionMode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetDiscoveryTimeWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetDiscoveryTime(DiscoveryTime{1}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetShowDeviceInformationWhenDiscoveryWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetShowDeviceInformationWhenDiscovery(ShowDeviceInformation::DontShow, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetServiceUuidWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    UUID uuid("FFE0");

    driver.SetServiceUuid(uuid, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetCharacteristicWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'F', 'A', 'I', 'L', 'E', 'D'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    CharacteristicType characteristic("FFE1");

    driver.SetCharacteristic(characteristic, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetSaveConnectedMacWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetSaveConnectedMac(true, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, SetConnectRemoteDeviceTimeoutWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    ConnectTimeout timeout("000100");

    driver.SetConnectRemoteDeviceTimeout(timeout, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, RemoveBondInformationWrongResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'E', 'T'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.RemoveBondInformation(status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// UART Error Propagation in Additional Methods
// ============================================================================

TEST_F(DriverErrorPathExtendedTest, GetNotifyInformationUARTError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    bool enabled = false;

    driver.GetNotifyInformation(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetModuleNameUARTTimeout) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    std::string name;

    driver.GetModuleName(name, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(DriverErrorPathExtendedTest, SetModuleNameTransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.SetModuleName("Test", status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, GetServiceUuidUARTError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;
    UUID uuid;

    driver.GetServiceUuid(uuid, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DriverErrorPathExtendedTest, DisconnectTransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Busy;

    driver.disconnect(status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// VariableString Capacity Tests
// ============================================================================

TEST_F(DriverErrorPathExtendedTest, VariableStringAppendCharOverflow) {
    VariableString<4> str;
    str.append('A');
    str.append('B');
    str.append('C');
    str.append('D');

    // This should throw std::length_error
    EXPECT_THROW(str.append('E'), std::length_error);
}

TEST_F(DriverErrorPathExtendedTest, VariableStringAppendStringOverflow) {
    VariableString<4> str;
    str.append("AB");

    // This should throw std::length_error (trying to append 3 more chars when only 2 left)
    EXPECT_THROW(str.append("CDE"), std::length_error);
}

TEST_F(DriverErrorPathExtendedTest, VariableStringAssignOverflow) {
    VariableString<4> str;

    // This should throw std::length_error (string too long for capacity)
    EXPECT_THROW(str.assign("ABCDE"), std::length_error);
}
