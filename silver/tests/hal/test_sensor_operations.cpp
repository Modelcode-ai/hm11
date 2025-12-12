// Test file for HMSensor operations: Set_Sensor_Type, Get_Sensor_Type,
// Set_Sensor_Work_Interval, Get_Sensor_Work_Interval, Get_Sensor_Temperature_And_Humidity
// This file validates Task [4] HMSensor Integration implementation requirements

#include <gtest/gtest.h>

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"

using namespace hm11;
using namespace hm11::hal;

class SensorOperationsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Reset mock UARTs for each test
        mock_uart_.ClearBuffers();
        mock_uart_.SetForcedStatus(UARTStatus::Ok);
        mock_uart_.SetForcedStatusForReceive(UARTStatus::Ok);
    }

    MockUART mock_uart_{UARTDataSize::DataSize8b};
};

// ============================================================================
// Sensor Type Configuration Tests
// ============================================================================

TEST_F(SensorOperationsTest, SetSensorTypeNone) {
    // Setup mock to return expected response for sensor type None (0)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute set sensor type operation
    driver.set_sensor_type(SensorType::None, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+SENS0"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+SENS0");
}

TEST_F(SensorOperationsTest, SetSensorTypeDHT11) {
    // Setup mock to return expected response for sensor type DHT11 (1)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute set sensor type operation
    driver.set_sensor_type(SensorType::DHT11, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+SENS1"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+SENS1");
}

TEST_F(SensorOperationsTest, SetSensorTypeDS18B20) {
    // Setup mock to return expected response for sensor type DS18B20 (2)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '2'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute set sensor type operation
    driver.set_sensor_type(SensorType::DS18B20, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+SENS2"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+SENS2");
}

TEST_F(SensorOperationsTest, GetSensorTypeNone) {
    // Setup mock to return sensor type None response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    SensorType sensor_type = SensorType::DHT11; // Initialize to different value

    // Execute get sensor type operation
    driver.get_sensor_type(sensor_type, status);

    // Verify success and correct sensor type
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(sensor_type, SensorType::None);

    // Verify the correct command was sent: "AT+SENS?"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+SENS?");
}

TEST_F(SensorOperationsTest, GetSensorTypeDHT11) {
    // Setup mock to return sensor type DHT11 response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    SensorType sensor_type = SensorType::None; // Initialize to different value

    // Execute get sensor type operation
    driver.get_sensor_type(sensor_type, status);

    // Verify success and correct sensor type
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(sensor_type, SensorType::DHT11);
}

TEST_F(SensorOperationsTest, GetSensorTypeDS18B20) {
    // Setup mock to return sensor type DS18B20 response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '2'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    SensorType sensor_type = SensorType::None; // Initialize to different value

    // Execute get sensor type operation
    driver.get_sensor_type(sensor_type, status);

    // Verify success and correct sensor type
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(sensor_type, SensorType::DS18B20);
}

// ============================================================================
// Work Interval Configuration Tests
// ============================================================================

TEST_F(SensorOperationsTest, SetSensorWorkIntervalSingleDigit) {
    // Setup mock to return expected response for single digit interval (05 -> 5)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '5'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("05");

    // Execute set work interval operation
    driver.set_sensor_work_interval(interval, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+RAT5" (single digit when starts with 0)
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 7);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+RAT5");
}

TEST_F(SensorOperationsTest, SetSensorWorkIntervalTwoDigits) {
    // Setup mock to return expected response for two digit interval (15)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1', '5'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("15");

    // Execute set work interval operation
    driver.set_sensor_work_interval(interval, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+RAT15" (full string when not starting with 0)
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+RAT15");
}

TEST_F(SensorOperationsTest, SetSensorWorkIntervalZero) {
    // Setup mock to return expected response for zero interval (00 -> 0)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("00");

    // Execute set work interval operation
    driver.set_sensor_work_interval(interval, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent: "AT+RAT0" (single digit for 00)
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 7);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+RAT0");
}

TEST_F(SensorOperationsTest, GetSensorWorkIntervalSingleDigit) {
    // Setup mock to simulate Ada response where byte after interval is null (0)
    // indicating single digit response "5\0"
    std::vector<std::uint8_t> response = {'O', 'K', '+', 'G', 'e', 't', ':', '5', 0};
    mock_uart_.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("99"); // Initialize to different value

    // Execute get work interval operation
    driver.get_sensor_work_interval(interval, status);

    // Verify success and correct interval (should be "05" for single digit 5)
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(std::string_view(interval), "05");

    // Verify the correct command was sent: "AT+RAT??"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+RAT??");
}

TEST_F(SensorOperationsTest, GetSensorWorkIntervalTwoDigits) {
    // Setup mock to simulate Ada response with two digits "15"
    std::vector<std::uint8_t> response = {'O', 'K', '+', 'G', 'e', 't', ':', '1', '5'};
    mock_uart_.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("99"); // Initialize to different value

    // Execute get work interval operation
    driver.get_sensor_work_interval(interval, status);

    // Verify success and correct interval
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(std::string_view(interval), "15");

    // Verify the correct command was sent
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+RAT??");
}

// ============================================================================
// Temperature and Humidity Reading Tests
// ============================================================================

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityValid) {
    // Setup mock to return temperature 25°C and humidity 60% ("OK+Get:025060")
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '2', '5', '0', '6', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(0);
    HumidityType humidity(0);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify success and correct values
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(temperature.value(), 25);
    EXPECT_EQ(humidity.value(), 60);

    // Verify the correct command was sent: "AT+TEHU?"
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8);
    std::string cmd(transmitted.begin(), transmitted.end());
    EXPECT_EQ(cmd, "AT+TEHU?");
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityMaxValues) {
    // Test with maximum valid values: temperature 120°C and humidity 100% ("OK+Get:120100")
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1', '2', '0', '1', '0', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(0);
    HumidityType humidity(0);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify success and correct maximum values
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(temperature.value(), 120);
    EXPECT_EQ(humidity.value(), 100);
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityMinValues) {
    // Test with minimum valid values: temperature 0°C and humidity 0% ("OK+Get:000000")
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '0', '0', '0', '0', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(50);
    HumidityType humidity(50);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify success and correct minimum values
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(temperature.value(), 0);
    EXPECT_EQ(humidity.value(), 0);
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityInvalidTemperature) {
    // Test with invalid temperature (125 > 120 max) ("OK+Get:125060")
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1', '2', '5', '0', '6', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(25);
    HumidityType humidity(60);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify error due to invalid temperature range
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityInvalidHumidity) {
    // Test with invalid humidity (105 > 100 max) ("OK+Get:025105")
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '2', '5', '1', '0', '5'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(25);
    HumidityType humidity(60);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify error due to invalid humidity range
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityInvalidResponse) {
    // Test with malformed response (wrong length)
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '2', '5'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(25);
    HumidityType humidity(60);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Verify error due to malformed response
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(SensorOperationsTest, SetSensorTypeUARTError) {
    // Force UART error
    mock_uart_.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;

    // Execute set sensor type operation
    driver.set_sensor_type(SensorType::DHT11, status);

    // Verify error propagation
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(SensorOperationsTest, GetSensorTypeInvalidResponse) {
    // Setup mock to return invalid sensor type response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '9'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    SensorType sensor_type = SensorType::None;

    // Execute get sensor type operation
    driver.get_sensor_type(sensor_type, status);

    // Verify error due to invalid sensor type value
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(SensorOperationsTest, SetSensorWorkIntervalUARTError) {
    // Force UART error
    mock_uart_.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;
    WorkInterval interval("05");

    // Execute set work interval operation
    driver.set_sensor_work_interval(interval, status);

    // Verify error propagation
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(SensorOperationsTest, GetSensorWorkIntervalInvalidDigit) {
    // Setup mock to return invalid digit in response
    std::vector<std::uint8_t> response = {'O', 'K', '+', 'G', 'e', 't', ':', 'X', 0}; // Invalid character 'X'
    mock_uart_.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    WorkInterval interval("00");

    // Execute get work interval operation
    driver.get_sensor_work_interval(interval, status);

    // Verify error due to invalid digit
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Advanced Sensor Reading Tests - Numeric Parsing
// ============================================================================

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityDecimalTruncation) {
    // Test handling of decimal values (should truncate to integer per Ada semantics)
    // Hardware response might include decimal points which need to be handled
    mock_uart_.SetReceiveData(
        std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0', '2', '5', '.', '5', '0', '6', '0', '.', '8'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    TemperatureType temperature(0);
    HumidityType humidity(0);

    // Execute get sensor readings operation
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    // Should handle decimal truncation correctly
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(temperature.value(), 25); // 25.5 truncated to 25
    EXPECT_EQ(humidity.value(), 60);    // 60.8 truncated to 60
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityLeadingZeros) {
    // Test parsing of values with different leading zero patterns
    const std::vector<std::tuple<std::string, int, int>> test_cases = {
        {"025060", 25, 60},   // Standard format
        {"005010", 5, 10},    // Single digit values with leading zeros
        {"100100", 100, 100}, // Maximum values
        {"000000", 0, 0},     // Minimum values
        {"009099", 9, 99},    // Edge cases
    };

    for (const auto& [response_data, expected_temp, expected_humidity] : test_cases) {
        std::string full_response = "OK+Get:" + response_data;
        std::vector<std::uint8_t> response_bytes(full_response.begin(), full_response.end());

        MockUART test_uart;
        test_uart.SetReceiveData(response_bytes);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Busy;
        TemperatureType temperature(120); // Initialize with max valid value to test parsing
        HumidityType humidity(100);

        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Failed for response: " << response_data;
        EXPECT_EQ(temperature.value(), expected_temp) << "Temperature parsing failed for: " << response_data;
        EXPECT_EQ(humidity.value(), expected_humidity) << "Humidity parsing failed for: " << response_data;
    }
}

TEST_F(SensorOperationsTest, GetSensorTemperatureAndHumidityParsingErrors) {
    // Test various malformed responses and error conditions
    const std::vector<std::pair<std::string, std::string>> error_cases = {
        {"OK+Get:02506", "Incomplete data (5 chars instead of 6)"},
        {"OK+Get:0250600", "Too much data (7 chars instead of 6)"},
        {"OK+Get:02A060", "Invalid temperature digit (A)"},
        {"OK+Get:025X60", "Invalid humidity digit (X)"},
        {"OK+Get:", "No data after colon"},
        {"OK+Err:SENSOR", "Error response from hardware"},
        {"INVALID", "Completely invalid response"},
    };

    for (const auto& [response, description] : error_cases) {
        std::vector<std::uint8_t> response_bytes(response.begin(), response.end());

        MockUART test_uart;
        test_uart.SetReceiveData(response_bytes);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Ok; // Initialize to success
        TemperatureType temperature(50);    // Initialize with valid values
        HumidityType humidity(50);

        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        // Should return error for malformed responses
        EXPECT_EQ(status, UARTStatus::ErrError) << "Should fail for: " << description;
    }
}

// ============================================================================
// Sensor Interval Configuration Edge Cases
// ============================================================================

TEST_F(SensorOperationsTest, GetSensorWorkIntervalBoundaryConditions) {
    // Test boundary conditions for work interval parsing
    const std::vector<std::tuple<std::vector<std::uint8_t>, std::string, std::string>> boundary_cases = {
        // Test case: Single digit response with null terminator
        {{'O', 'K', '+', 'G', 'e', 't', ':', '0', 0}, "00", "Zero interval"},
        {{'O', 'K', '+', 'G', 'e', 't', ':', '9', 0}, "09", "Single digit 9"},
        {{'O', 'K', '+', 'G', 'e', 't', ':', '1', 0}, "01", "Single digit 1"},

        // Test case: Two digit response
        {{'O', 'K', '+', 'G', 'e', 't', ':', '1', '0'}, "10", "Two digit 10"},
        {{'O', 'K', '+', 'G', 'e', 't', ':', '9', '9'}, "99", "Maximum value 99"},
        {{'O', 'K', '+', 'G', 'e', 't', ':', '2', '5'}, "25", "Typical value 25"},
    };

    for (const auto& [response_data, expected_interval, description] : boundary_cases) {
        MockUART test_uart;
        test_uart.SetReceiveData(response_data);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Busy;
        WorkInterval interval("99"); // Initialize with valid value for testing

        driver.get_sensor_work_interval(interval, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Failed for: " << description;
        EXPECT_EQ(std::string_view(interval), expected_interval) << "Parsing failed for: " << description;
    }
}

TEST_F(SensorOperationsTest, SetSensorWorkIntervalFormatting) {
    // Test proper formatting of work interval values when sending commands
    const std::vector<std::tuple<std::string, std::string, std::string>> format_cases = {
        {"00", "AT+RAT0", "Zero should format as single digit"},
        {"01", "AT+RAT1", "Leading zero should be stripped for single digit"},
        {"09", "AT+RAT9", "Single digit with leading zero"},
        {"10", "AT+RAT10", "Two digit value should remain as-is"},
        {"99", "AT+RAT99", "Maximum two digit value"},
    };

    for (const auto& [input_interval, expected_command, description] : format_cases) {
        // Set up mock to return success response
        std::string response_value = input_interval;
        if (response_value[0] == '0' && response_value.size() == 2 && response_value[1] != '0') {
            response_value = response_value.substr(1); // Strip leading zero for response
        }

        MockUART test_uart;
        std::string response = "OK+Set:" + response_value;
        std::vector<std::uint8_t> response_data(response.begin(), response.end());
        test_uart.SetReceiveData(response_data);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Busy;
        WorkInterval interval(input_interval);

        driver.set_sensor_work_interval(interval, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Command failed for: " << description;

        // Verify the correct command was sent
        const auto& transmitted = driver.GetUart().GetTransmitted8b();
        EXPECT_EQ(transmitted.size(), expected_command.size()) << "Wrong command length for: " << description;

        std::string actual_command(transmitted.begin(), transmitted.end());
        EXPECT_EQ(actual_command, expected_command) << "Wrong command for: " << description;
    }
}

// ============================================================================
// Multi-Sensor Type Support Tests
// ============================================================================

TEST_F(SensorOperationsTest, SensorTypeOperationsComprehensive) {
    // Test all sensor types with their respective behaviors
    const std::vector<std::tuple<SensorType, std::string, std::string, std::string>> sensor_cases = {
        {SensorType::None, "0", "AT+SENS0", "No sensor attached"},
        {SensorType::DHT11, "1", "AT+SENS1", "DHT11 temperature and humidity sensor"},
        {SensorType::DS18B20, "2", "AT+SENS2", "DS18B20 temperature-only sensor"},
    };

    for (const auto& [sensor_type, type_value, expected_command, description] : sensor_cases) {
        // Test SET operation
        {
            MockUART set_uart;
            std::string set_response = "OK+Set:" + type_value;
            std::vector<std::uint8_t> set_response_data(set_response.begin(), set_response.end());
            set_uart.SetReceiveData(set_response_data);

            auto set_driver = make_hm11_driver(std::move(set_uart));
            UARTStatus set_status = UARTStatus::Busy;

            set_driver.set_sensor_type(sensor_type, set_status);

            EXPECT_EQ(set_status, UARTStatus::Ok) << "SET failed for: " << description;

            const auto& set_transmitted = set_driver.GetUart().GetTransmitted8b();
            std::string set_command(set_transmitted.begin(), set_transmitted.end());
            EXPECT_EQ(set_command, expected_command) << "SET command wrong for: " << description;
        }

        // Test GET operation
        {
            MockUART get_uart;
            std::string get_response = "OK+Get:" + type_value;
            std::vector<std::uint8_t> get_response_data(get_response.begin(), get_response.end());
            get_uart.SetReceiveData(get_response_data);

            auto get_driver = make_hm11_driver(std::move(get_uart));
            UARTStatus get_status = UARTStatus::Busy;
            SensorType retrieved_type = SensorType::None; // Initialize to different value

            get_driver.get_sensor_type(retrieved_type, get_status);

            EXPECT_EQ(get_status, UARTStatus::Ok) << "GET failed for: " << description;
            EXPECT_EQ(retrieved_type, sensor_type) << "GET result wrong for: " << description;

            const auto& get_transmitted = get_driver.GetUart().GetTransmitted8b();
            std::string get_command(get_transmitted.begin(), get_transmitted.end());
            EXPECT_EQ(get_command, "AT+SENS?") << "GET command wrong for: " << description;
        }
    }
}

TEST_F(SensorOperationsTest, SensorTemperatureRangeValidationExtensive) {
    // Test temperature range validation with comprehensive edge cases
    const std::vector<std::tuple<std::string, int, bool, std::string>> temp_cases = {
        {"000060", 0, true, "Minimum valid temperature (0°C)"},
        {"120060", 120, true, "Maximum valid temperature (120°C)"},
        {"025060", 25, true, "Typical room temperature"},
        {"100060", 100, true, "Boiling point of water"},
        {"121060", 121, false, "Above maximum temperature (should fail)"},
        {"255060", 255, false, "Way above maximum (should fail)"},
        {"999060", 999, false, "Extremely high value (should fail)"},
    };

    for (const auto& [response_data, temp_value, should_succeed, description] : temp_cases) {
        std::string full_response = "OK+Get:" + response_data;
        std::vector<std::uint8_t> response_bytes(full_response.begin(), full_response.end());

        MockUART test_uart;
        test_uart.SetReceiveData(response_bytes);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Busy;
        TemperatureType temperature(50); // Initialize with valid value
        HumidityType humidity(50);

        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        if (should_succeed) {
            EXPECT_EQ(status, UARTStatus::Ok) << "Should succeed for: " << description;
            EXPECT_EQ(temperature.value(), temp_value) << "Temperature value wrong for: " << description;
        } else {
            EXPECT_EQ(status, UARTStatus::ErrError) << "Should fail for: " << description;
        }
    }
}

TEST_F(SensorOperationsTest, SensorHumidityRangeValidationExtensive) {
    // Test humidity range validation with comprehensive edge cases
    const std::vector<std::tuple<std::string, int, bool, std::string>> humidity_cases = {
        {"025000", 0, true, "Minimum valid humidity (0%)"},
        {"025100", 100, true, "Maximum valid humidity (100%)"},
        {"025050", 50, true, "Typical indoor humidity"},
        {"025090", 90, true, "High humidity"},
        {"025101", 101, false, "Above maximum humidity (should fail)"},
        {"025150", 150, false, "Way above maximum (should fail)"},
        {"025255", 255, false, "Extremely high value (should fail)"},
    };

    for (const auto& [response_data, humidity_value, should_succeed, description] : humidity_cases) {
        std::string full_response = "OK+Get:" + response_data;
        std::vector<std::uint8_t> response_bytes(full_response.begin(), full_response.end());

        MockUART test_uart;
        test_uart.SetReceiveData(response_bytes);

        auto driver = make_hm11_driver(std::move(test_uart));
        UARTStatus status = UARTStatus::Busy;
        TemperatureType temperature(25);
        HumidityType humidity(50); // Initialize with valid value

        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        if (should_succeed) {
            EXPECT_EQ(status, UARTStatus::Ok) << "Should succeed for: " << description;
            EXPECT_EQ(humidity.value(), humidity_value) << "Humidity value wrong for: " << description;
        } else {
            EXPECT_EQ(status, UARTStatus::ErrError) << "Should fail for: " << description;
        }
    }
}

// ============================================================================
// Type Safety and Range Validation Tests
// ============================================================================

TEST_F(SensorOperationsTest, TemperatureTypeRangeValidation) {
    // Test that TemperatureType enforces 0-120 range
    EXPECT_NO_THROW({ TemperatureType temp(0); (void)temp; });   // Minimum valid
    EXPECT_NO_THROW({ TemperatureType temp(120); (void)temp; }); // Maximum valid
    EXPECT_NO_THROW({ TemperatureType temp(25); (void)temp; });  // Typical valid

    // Range checking via RangedInteger is tested in type tests
    EXPECT_TRUE(TemperatureType::is_valid(0));
    EXPECT_TRUE(TemperatureType::is_valid(120));
    EXPECT_FALSE(TemperatureType::is_valid(121)); // Over maximum
}

TEST_F(SensorOperationsTest, HumidityTypeRangeValidation) {
    // Test that HumidityType enforces 0-100 range
    EXPECT_NO_THROW({ HumidityType humidity(0); (void)humidity; });   // Minimum valid
    EXPECT_NO_THROW({ HumidityType humidity(100); (void)humidity; }); // Maximum valid
    EXPECT_NO_THROW({ HumidityType humidity(60); (void)humidity; });  // Typical valid

    // Range checking via RangedInteger is tested in type tests
    EXPECT_TRUE(HumidityType::is_valid(0));
    EXPECT_TRUE(HumidityType::is_valid(100));
    EXPECT_FALSE(HumidityType::is_valid(101)); // Over maximum
}

TEST_F(SensorOperationsTest, WorkIntervalValidation) {
    // Test that WorkInterval enforces 2-digit format with digit characters
    EXPECT_NO_THROW(WorkInterval interval("00")); // Minimum valid
    EXPECT_NO_THROW(WorkInterval interval("99")); // Maximum valid
    EXPECT_NO_THROW(WorkInterval interval("15")); // Typical valid
}

// ============================================================================
// Integration Test - Complete Sensor Workflow
// ============================================================================

TEST_F(SensorOperationsTest, CompleteSensorWorkflow) {
    // Integration test demonstrating complete sensor configuration and reading workflow

    // Step 1: Set sensor type to DHT11
    {
        MockUART uart;
        uart.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});
        auto driver = make_hm11_driver(std::move(uart));

        UARTStatus status = UARTStatus::Busy;
        driver.set_sensor_type(SensorType::DHT11, status);

        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 2: Configure work interval
    {
        MockUART uart;
        uart.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '5'});
        auto driver = make_hm11_driver(std::move(uart));

        UARTStatus status = UARTStatus::Busy;
        WorkInterval interval("05");
        driver.set_sensor_work_interval(interval, status);

        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 3: Read sensor values multiple times (simulating periodic readings)
    for (int reading = 1; reading <= 3; ++reading) {
        MockUART uart;

        // Simulate different readings over time
        std::string temp_str = std::to_string(20 + reading);
        std::string humidity_str = std::to_string(60 + reading);
        if (temp_str.size() == 1)
            temp_str = "00" + temp_str;
        else if (temp_str.size() == 2)
            temp_str = "0" + temp_str;
        if (humidity_str.size() == 1)
            humidity_str = "00" + humidity_str;
        else if (humidity_str.size() == 2)
            humidity_str = "0" + humidity_str;

        std::string response = "OK+Get:" + temp_str + humidity_str;
        std::vector<std::uint8_t> response_data(response.begin(), response.end());
        uart.SetReceiveData(response_data);

        auto driver = make_hm11_driver(std::move(uart));

        UARTStatus status = UARTStatus::Busy;
        TemperatureType temperature(0);
        HumidityType humidity(0);

        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Reading " << reading << " failed";
        EXPECT_EQ(temperature.value(), 20 + reading) << "Temperature reading " << reading << " incorrect";
        EXPECT_EQ(humidity.value(), 60 + reading) << "Humidity reading " << reading << " incorrect";
    }
}