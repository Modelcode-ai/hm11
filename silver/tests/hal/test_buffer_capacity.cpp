// Invalid Character Validation Tests
// Tests error paths in hm11_driver.hpp where response parsing detects invalid data

#include <gtest/gtest.h>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11_driver.hpp>

using namespace hm11;
using namespace hm11::hal;

class InvalidCharacterTest : public ::testing::Test {
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
// Invalid Response Character Tests
// These test validation of response characters (lines: 467-470, etc.)
// ============================================================================

TEST_F(InvalidCharacterTest, GetRole_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    Role role{};

    driver.GetRole(role, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetBondMode_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid, should be '0'-'3'
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    BondMode mode{};

    driver.GetBondMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetWorkType_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkType type{};

    driver.GetWorkType(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetWorkMode_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkMode mode{};

    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetNotifyMode_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    NotifyMode mode{};

    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetModuleRxGain_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    RxGain gain{};

    driver.GetModuleRxGain(gain, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetModulePower_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid, should be '0' or '1'
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ModulePower power{};

    driver.get_module_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetOutputPower_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'Z'});  // 'Z' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    OutputPower power{};

    driver.get_output_power(power, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetModuleAutoSleep_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    bool enabled = false;

    driver.get_module_auto_sleep(enabled, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetAdvertisingInterval_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'Z'});  // 'Z' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingInterval interval{};

    driver.get_advertising_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetAdvertisingType_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingType type{};

    driver.get_advertising_type(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetReliableAdvertisingMode_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});  // '9' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    AdvertisingMode mode{};

    driver.get_reliable_advertising_mode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetUartBaudRate_InvalidCharacter) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'Z'});  // 'Z' is invalid
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    UartBaudRate baud{};

    driver.get_uart_baud_rate(baud, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Empty/Malformed Response Tests (lines 874, 972, 1748, 1820, 2157)
// ============================================================================

TEST_F(InvalidCharacterTest, GetWorkMode_EmptyResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':'});  // Missing value
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkMode mode{};

    driver.GetWorkMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetNotifyMode_EmptyResponse) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':'});  // Missing value
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    NotifyMode mode{};

    driver.GetNotifyMode(mode, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetConnectRemoteDeviceTimeout_WrongLength) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1', '2', '3'});  // Too short
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ConnectTimeout timeout{};

    driver.GetConnectRemoteDeviceTimeout(timeout, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// UART Error Tests (lines 1349, 1356, 1505, 1805, 1886, 1896, 2003, 2010, etc.)
// ============================================================================

TEST_F(InvalidCharacterTest, GetSensorType_UARTTransmitError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    SensorType type{};

    driver.get_sensor_type(type, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetSensorWorkInterval_UARTReceiveError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkInterval interval{};

    driver.get_sensor_work_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetSensorTemperatureAndHumidity_UARTError) {
    mock_uart->SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    TemperatureType temp{};
    HumidityType humidity{};

    driver.get_sensor_temperature_and_humidity(temp, humidity, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

// ============================================================================
// MAC Address and Response Format Tests (lines 1621, 1919, 1927)
// ============================================================================

TEST_F(InvalidCharacterTest, GetLastConnectedDeviceAddress_WrongFormat) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'R', 'A', 'D', 'D', ':', 'A', 'B', 'C'});  // Too short
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    MacAddress mac{};

    driver.GetLastConnectedDeviceAddress(mac, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetSensorWorkInterval_InvalidPrefix) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'E', 'R', 'R', 'O', 'R', ':', '5'});  // Wrong prefix
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkInterval interval{};

    driver.get_sensor_work_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetSensorWorkInterval_EmptyData) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':'});  // No data
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkInterval interval{};

    driver.get_sensor_work_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Sensor Data Validation Tests (lines 1960, 2090)
// ============================================================================

TEST_F(InvalidCharacterTest, GetSensorWorkInterval_NonDigit) {
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X'});  // Non-digit
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    WorkInterval interval{};

    driver.get_sensor_work_interval(interval, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, GetSensorTemperatureAndHumidity_InvalidFormat) {
    // Invalid format - not enough digits
    mock_uart->SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '2'});
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    TemperatureType temp{};
    HumidityType humidity{};

    driver.get_sensor_temperature_and_humidity(temp, humidity, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Connect Operation Tests (lines 1442-1568)
// ============================================================================

TEST_F(InvalidCharacterTest, ConnectLastDevice_UARTError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ConnectResult result{};

    driver.connect_last_device(result, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(InvalidCharacterTest, Connect_UARTError) {
    mock_uart->SetForcedStatus(UARTStatus::ErrTimeout);
    auto driver = make_driver();
    UARTStatus status = UARTStatus::Ok;
    ConnectResult result{};
    DiscoveredIndex index{0};

    driver.connect(index, result, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}
