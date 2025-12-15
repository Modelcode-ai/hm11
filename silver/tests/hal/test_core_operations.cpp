// Core Operations Tests - Essential driver functionality
// Tests basic connectivity, versioning, reset/restart, and fundamental operations

#include <gtest/gtest.h>
#include <hm11/hm11.hpp>
#include <hm11/util.hpp>

using namespace hm11;

class CoreOperationsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);
    }

    MockUART mock_uart{UARTDataSize::DataSize8b};
};

// ============================================================================
// Core Driver Operations
// ============================================================================

TEST_F(CoreOperationsTest, Test_BasicConnectivity) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K'});
    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_SoftwareVersion) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'H', 'M', 'S', 'o', 'f', 't', 'V', '5', '4', '7'});
    auto version = driver.SoftwareVersion();
    EXPECT_EQ(version, "HMSoftV547");
}

TEST_F(CoreOperationsTest, Test_ResetRestart) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'R', 'E', 'N', 'E', 'W'});
    driver.reset(status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'R', 'E', 'S', 'E', 'T'});
    driver.restart(status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_SleepWake) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'L', 'E', 'E', 'P'});
    driver.sleep(status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K'});
    driver.wake_up(status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Role Configuration
// ============================================================================

TEST_F(CoreOperationsTest, Test_SetRole) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});
    driver.SetRole(Role::Peripheral, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});
    driver.SetRole(Role::Central, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_GetRole) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    Role role{};
    driver.GetRole(role, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// PIN and Bonding
// ============================================================================

TEST_F(CoreOperationsTest, Test_SetPinCode) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    PinType pin("123456");
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1', '2', '3', '4', '5', '6'});
    driver.SetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_GetPinCode) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '0', '0', '0', '0', '0'});
    PinType pin;
    driver.GetPinCode(pin, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_BondMode) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '2'});
    driver.SetBondMode(BondMode::AuthWithPin, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '2'});
    BondMode mode{};
    driver.GetBondMode(mode, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_RemoveBondInfo) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'E', 'R', 'A', 'S', 'E'});
    driver.RemoveBondInformation(status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Notification Configuration
// ============================================================================

TEST_F(CoreOperationsTest, Test_NotifyInformation) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});
    driver.SetNotifyInformation(true, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});
    bool enabled = false;
    driver.GetNotifyInformation(enabled, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_NotifyMode) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});
    driver.SetNotifyMode(NotifyMode::WithAddress, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});
    NotifyMode mode{};
    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Module Configuration
// ============================================================================

TEST_F(CoreOperationsTest, Test_ModuleName) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', 'T', 'e', 's', 't'});
    driver.SetModuleName("Test", status);
    EXPECT_EQ(status, UARTStatus::Ok);

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'N', 'A', 'M', 'E', ':', 'H', 'M', 'S', 'o', 'f', 't', ' ', ' ', ' ', ' ', ' ', ' '});
    std::string name;
    driver.GetModuleName(name, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_RxGain) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});
    driver.SetModuleRxGain(RxGain::No, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    RxGain gain{};
    driver.GetModuleRxGain(gain, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Work Type and Mode
// ============================================================================

TEST_F(CoreOperationsTest, Test_WorkType) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});
    driver.SetWorkType(WorkType::StartImmediately, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    WorkType work_type{};
    driver.GetWorkType(work_type, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_WorkMode) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});
    driver.SetWorkMode(WorkMode::TransmissionMode, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});
    WorkMode mode{};
    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Discovery
// ============================================================================

TEST_F(CoreOperationsTest, Test_DiscoveryTime) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '3'});
    driver.SetDiscoveryTime(DiscoveryTime{3}, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '3'});
    DiscoveryTime time;
    driver.GetDiscoveryTime(time, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Connection Management
// ============================================================================

TEST_F(CoreOperationsTest, Test_Disconnect) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'L', 'O', 'S', 'T'});
    driver.disconnect(status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(CoreOperationsTest, Test_SaveConnectedMac) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});  // Inverted: true = '0'
    driver.SetSaveConnectedMac(true, status);
    EXPECT_EQ(status, UARTStatus::Ok);

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});  // Inverted: enabled = '0'
    bool enabled = false;
    driver.GetSaveConnectedMac(enabled, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Service and Characteristic Configuration
// ============================================================================

TEST_F(CoreOperationsTest, Test_ServiceUuid) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    UUID uuid("FFE0");
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0', 'x', 'F', 'F', 'E', '0'});
    driver.SetServiceUuid(uuid, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', 'x', 'F', 'F', 'E', '0'});
    UUID result_uuid;
    driver.GetServiceUuid(result_uuid, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Temperature Reading
// ============================================================================

TEST_F(CoreOperationsTest, Test_ModuleTemperature) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;

    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '+', '0', '2', '5', '.', '0', '0'});  // 7 chars: +025.00
    InternalTemperatureType temp;
    driver.GetModuleTemperature(temp, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(CoreOperationsTest, ErrorPath_UartBusy) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    
    driver.GetUart().SetForcedStatus(UARTStatus::Busy);
    UARTStatus status = UARTStatus::Ok;
    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::Busy);
}

TEST_F(CoreOperationsTest, ErrorPath_UartError) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    
    driver.GetUart().SetForcedStatus(UARTStatus::ErrError);
    UARTStatus status = UARTStatus::Ok;
    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(CoreOperationsTest, ErrorPath_UartTimeout) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    
    driver.GetUart().SetForcedStatus(UARTStatus::ErrTimeout);
    UARTStatus status = UARTStatus::Ok;
    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(CoreOperationsTest, ErrorPath_EmptyResponse) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{});
    Role role{};
    driver.GetRole(role, status);
    // Should handle gracefully - don't crash
}

TEST_F(CoreOperationsTest, ErrorPath_UnexpectedResponse) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'E', 'R', 'R', 'O', 'R'});
    driver.SetRole(Role::Peripheral, status);
    // Should handle gracefully
}

// ============================================================================
// Transmit Operations
// ============================================================================

TEST_F(CoreOperationsTest, Test_TransmitCommand) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K'});
    driver.Transmit("AT", status);
    EXPECT_EQ(status, UARTStatus::Ok);
    
    auto transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 2);
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
}

TEST_F(CoreOperationsTest, Test_TransmitAndCheck) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;
    
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'R', 'E', 'S', 'E', 'T'});
    driver.TransmitAndCheck("AT+RESET", "OK+RESET", status);
    EXPECT_EQ(status, UARTStatus::Ok);
}
