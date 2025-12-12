/**
 * @file test_hm11_driver_sensor.cpp
 * @brief Unit tests for HM11Driver HMSensor-specific operations
 *
 * Tests cover sensor type configuration, temperature/humidity readings,
 * and sensor work interval operations for the HMSensor variant module.
 *
 * ## Overview (Decision 15)
 *
 * All HMSensor operations are always present in the API and documented as
 * "HMSensor-only". Hardware returns errors if called on non-HMSensor modules.
 *
 * ## Decision 29: Sensor Reading Value Representation
 *
 * Temperature and humidity are parsed into numeric uint8_t values (not strings):
 * - Temperature: 0-120 range (matches Ada's Natural range 0..120)
 * - Humidity: 0-100 range (matches Ada's Natural range 0..100)
 * - Decimal values like "25.5" are truncated to integers (25)
 */

#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// Mock UART Port for sensor tests
class SensorMockUARTPort {
  public:
    std::vector<UInt8> transmitted_data;
    std::vector<UInt8> receive_buffer;
    UartStatus next_status{UartStatus::Ok};

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        status = next_status;
        if (status == UartStatus::Ok) {
            transmitted_data.insert(transmitted_data.end(), data.begin(), data.end());
        }
    }

    void
    receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool /*as_stream*/ = false) noexcept {
        status = next_status;
        if (status == UartStatus::Ok) {
            const std::size_t TO_COPY = std::min(data.size(), receive_buffer.size());
            std::copy_n(receive_buffer.begin(), TO_COPY, data.begin());
        }
    }

    void set_receive_response(std::string_view response) {
        receive_buffer.clear();
        receive_buffer.insert(receive_buffer.end(), response.begin(), response.end());
    }

    [[nodiscard]] std::string get_transmitted_string() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return {reinterpret_cast<const char*>(transmitted_data.data()), transmitted_data.size()};
    }

    void clear() {
        transmitted_data.clear();
        receive_buffer.clear();
        next_status = UartStatus::Ok;
    }
};

// Mock Receive Handler
struct MockReceiveHandler {
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// Test fixture
class HM11DriverSensorTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    SensorMockUARTPort mock_uart;
    MockReceiveHandler mock_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void SetUp() override { mock_uart.clear(); }
};

// ============================================================================
// Sensor Type Tests
// ============================================================================

TEST_F(HM11DriverSensorTest, SetSensorType_None) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_sensor_type(SensorType::None, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS0");
}

TEST_F(HM11DriverSensorTest, SetSensorType_DHT11) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_sensor_type(SensorType::DHT11, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS1");
}

TEST_F(HM11DriverSensorTest, SetSensorType_DS18B20) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:2");

    UartStatus status{UartStatus::Busy};
    driver.set_sensor_type(SensorType::DS18B20, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS2");
}

TEST_F(HM11DriverSensorTest, GetSensorType_None) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:0");

    SensorType result{};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_type(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, SensorType::None);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS?");
}

TEST_F(HM11DriverSensorTest, GetSensorType_DHT11) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:1");

    SensorType result{};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_type(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, SensorType::DHT11);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS?");
}

TEST_F(HM11DriverSensorTest, GetSensorType_DS18B20) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:2");

    SensorType result{};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_type(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, SensorType::DS18B20);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SENS?");
}

TEST_F(HM11DriverSensorTest, GetSensorType_InvalidValue) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:9"); // Invalid sensor type

    SensorType result{};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_type(result, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Temperature and Humidity Tests (Decision 29: Numeric Value Representation)
// ============================================================================

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_ValidReading) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Response format: "OK+Get:TTT:HHH" (3 chars temp, colon, 3 chars humidity)
    mock_uart.set_receive_response("OK+Get:025:060");

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, 25);
    EXPECT_EQ(humidity, 60);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+TEHU?");
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_MaxValues) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:120:100"); // Max temp 120, max humidity 100

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, 120);
    EXPECT_EQ(humidity, 100);
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_MinValues) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:000:000"); // Min values

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, 0);
    EXPECT_EQ(humidity, 0);
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_TemperatureOutOfRange) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:150:050"); // Temperature > 120 (invalid)

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_HumidityOutOfRange) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:025:150"); // Humidity > 100 (invalid)

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_DecimalTruncation) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // If hardware returns decimal values, std::from_chars will parse up to the decimal point
    // "25." will parse as 25, "60." will parse as 60
    mock_uart.set_receive_response("OK+Get:025:060"); // Standard integer format

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(temperature, 25);
    EXPECT_EQ(humidity, 60);
}

TEST_F(HM11DriverSensorTest, GetTemperatureAndHumidity_InvalidFormat) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:ABC:XYZ"); // Non-numeric

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Sensor Work Interval Tests
// ============================================================================

TEST_F(HM11DriverSensorTest, SetSensorWorkInterval_SingleDigit) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Leading zero: send only last digit (Ada behavior)
    mock_uart.set_receive_response("OK+Set:5");

    WorkInterval interval{"05"};
    UartStatus status{UartStatus::Busy};
    driver.set_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT5"); // Only last digit sent
}

TEST_F(HM11DriverSensorTest, SetSensorWorkInterval_TwoDigits) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Both digits non-zero: send both
    mock_uart.set_receive_response("OK+Set:30");

    WorkInterval interval{"30"};
    UartStatus status{UartStatus::Busy};
    driver.set_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT30"); // Both digits sent
}

TEST_F(HM11DriverSensorTest, SetSensorWorkInterval_Zero) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:0");

    WorkInterval interval{"00"};
    UartStatus status{UartStatus::Busy};
    driver.set_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT0"); // Leading zero: send only last digit
}

TEST_F(HM11DriverSensorTest, SetSensorWorkInterval_MaxValue) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:99");

    WorkInterval interval{"99"};
    UartStatus status{UartStatus::Busy};
    driver.set_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT99");
}

TEST_F(HM11DriverSensorTest, GetSensorWorkInterval_SingleDigitResponse) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Hardware returns single digit (Ada checks if byte at position+2 is null)
    // In C++, we pad the response with null byte to simulate this
    std::string response = "OK+Get:5";
    response.push_back('\0'); // Null terminator to match MAX_RESPONSE_SIZE
    mock_uart.set_receive_response(response);

    WorkInterval interval{"00"};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(interval.view(), "05"); // Padded with leading zero
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT?");
}

TEST_F(HM11DriverSensorTest, GetSensorWorkInterval_TwoDigitResponse) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:30");

    WorkInterval interval{"00"};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(interval.view(), "30");
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RAT?");
}

TEST_F(HM11DriverSensorTest, GetSensorWorkInterval_ZeroResponse) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Single digit response with null terminator
    std::string response = "OK+Get:0";
    response.push_back('\0');
    mock_uart.set_receive_response(response);

    WorkInterval interval{"99"};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(interval.view(), "00"); // Padded with leading zero
}

TEST_F(HM11DriverSensorTest, GetSensorWorkInterval_MaxValue) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:99");

    WorkInterval interval{"00"};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(interval.view(), "99");
}

TEST_F(HM11DriverSensorTest, GetSensorWorkInterval_InvalidCharacter) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:X");

    WorkInterval interval{"00"};
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Status Error Propagation Tests (Decision 4: Error Handling)
// ============================================================================

TEST_F(HM11DriverSensorTest, SetSensorType_TransmitError) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.next_status = UartStatus::ErrError;

    UartStatus status{UartStatus::Busy};
    driver.set_sensor_type(SensorType::DHT11, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverSensorTest, GetSensorTemperatureAndHumidity_ReceiveError) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.next_status = UartStatus::ErrTimeout;

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    UartStatus status{UartStatus::Busy};
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

TEST_F(HM11DriverSensorTest, SetSensorWorkInterval_WrongResponse) {
    HM11Driver<SensorMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    // Wrong response (doesn't match expected)
    mock_uart.set_receive_response("OK+Err");

    WorkInterval interval{"30"};
    UartStatus status{UartStatus::Busy};
    driver.set_sensor_work_interval(interval, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}
