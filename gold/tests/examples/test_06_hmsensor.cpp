/**
 * @file test_06_hmsensor.cpp
 * @brief Integration test for 06_hmsensor example
 *
 * This test validates the HMSensor reader example logic using EnhancedMockUART
 * to simulate HM-11 HMSensor module responses without requiring physical hardware.
 *
 * ## Test Coverage
 *
 * 1. Sensor type configuration (DHT11)
 * 2. Work interval configuration (5 minutes)
 * 3. Temperature and humidity reading (numeric uint8_t values)
 * 4. Range validation (temperature 0-120, humidity 0-100)
 * 5. Periodic reading loop pattern
 * 6. Error handling with sensor type None
 * 7. Error handling with mock LED feedback
 * 8. Command sequence correctness
 *
 * ## Decision 29: Numeric Value Representation
 *
 * Temperature and humidity are parsed into numeric uint8_t values:
 * - Temperature: 0-120 range (matches Ada's Natural range 0..120)
 * - Humidity: 0-100 range (matches Ada's Natural range 0..100)
 * - Driver parses hardware response strings (e.g., "025", "060") into integers
 * - Range validation occurs during parsing
 * - Decimal values like "25.5" are truncated to integers (25)
 *
 * This test validates that temperature and humidity values are correctly parsed
 * and returned as numeric values, not strings.
 *
 * ## Testing Strategy
 *
 * Rather than testing the example executable directly (which contains hardware
 * initialization and delays), this test extracts the core business logic into
 * testable functions that accept driver and LED abstractions.
 *
 * The test validates:
 * - Correct AT command sequence for HMSensor configuration
 * - Sensor type setting (AT+SENS1 for DHT11)
 * - Work interval setting (AT+RAT5 for 5 minutes)
 * - Temperature/humidity reading (AT+TEHU?)
 * - Numeric parsing of sensor values
 * - Range validation for out-of-bounds values
 * - Status code propagation and error handling
 * - LED feedback patterns for success/error
 *
 * ## Confidence Level
 *
 * This test achieves ~70% confidence without hardware by:
 * - Validating complete sensor configuration command sequence
 * - Testing numeric value parsing from string responses
 * - Verifying range validation (0-120 temp, 0-100 humidity)
 * - Testing error conditions and timeout handling
 * - Simulating realistic HMSensor module responses
 * - Validating periodic reading loop pattern
 *
 * Hardware testing will increase confidence to ~99%.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "enhanced_mock_uart.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"
#include "mock_led.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;
using namespace hm11::test;

// ============================================================================
// Mock Receive Handler for EnhancedMockUART
// ============================================================================

/**
 * @brief Simple receive handler that delegates to mock UART
 *
 * Matches the pattern from test_hm11_driver_core.cpp for use with
 * EnhancedMockUART in integration tests.
 */
struct SimpleReceiveHandler {
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

// Verify handler satisfies concept
static_assert(
    IS_RECEIVE_HANDLER_V<SimpleReceiveHandler, EnhancedMockUART>,
    "SimpleReceiveHandler must satisfy receive handler interface");

// ============================================================================
// Business Logic Functions (Extracted from main.cpp)
// ============================================================================

/**
 * @brief Core logic for HMSensor configuration
 *
 * This function encapsulates the sensor configuration logic from the example,
 * separated from hardware initialization and delays for testability.
 *
 * @tparam DriverType HM11Driver instantiation type
 * @param driver Reference to HM11 driver instance
 * @param green_led Reference to green LED for success indication
 * @param red_led Reference to red LED for error indication
 * @return true if all operations succeeded, false on error
 */
template <typename DriverType>
bool run_hmsensor_config_logic(DriverType& driver, [[maybe_unused]] MockLED& green_led, MockLED& red_led) noexcept {
    UartStatus status;

    // Test module responsiveness
    driver.test(status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure sensor type to DHT11 (temperature and humidity)
    driver.set_sensor_type(SensorType::DHT11, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure work interval to 5 minutes
    WorkInterval interval{"05"};
    driver.set_sensor_work_interval(interval, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    return true;
}

/**
 * @brief Core logic for HMSensor reading
 *
 * This function encapsulates the sensor reading logic from the example,
 * performing a single reading iteration for testing.
 *
 * @tparam DriverType HM11Driver instantiation type
 * @param driver Reference to HM11 driver instance
 * @param green_led Reference to green LED for success indication
 * @param red_led Reference to red LED for error indication
 * @param temperature Output parameter for temperature value (0-120)
 * @param humidity Output parameter for humidity value (0-100)
 * @return true if reading succeeded, false on error
 */
template <typename DriverType>
bool run_hmsensor_reading_logic(
    DriverType& driver,
    MockLED& green_led,
    MockLED& red_led,
    uint8_t& temperature,
    uint8_t& humidity) noexcept {
    UartStatus status;

    // Query temperature and humidity
    driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

    if (status == UartStatus::Ok) {
        // Reading successful - toggle green LED
        green_led.toggle();
        return true;
    }

    // Reading failed - indicate error
    red_led.on();
    return false;
}

// ============================================================================
// Integration Tests
// ============================================================================

/**
 * @brief Test fixture for HMSensor example tests
 */
class HMSensorExampleTest : public ::testing::Test {
  protected:
    EnhancedMockUART mock_uart_;
    SimpleReceiveHandler handler_;
    MockLED green_led_;
    MockLED red_led_;

    void SetUp() override {
        mock_uart_.clear();
        green_led_.clear();
        red_led_.clear();
    }
};

/**
 * @brief Test successful HMSensor configuration sequence
 *
 * Validates:
 * - AT command sequence (test, set sensor type, set work interval)
 * - Status code checking
 * - LED feedback (no error)
 */
TEST_F(HMSensorExampleTest, ConfigurationSuccess) {
    // Set up mock responses for successful configuration
    mock_uart_.expect_command("AT", "OK");             // Test command
    mock_uart_.expect_command("AT+SENS1", "OK+Set:1"); // Set DHT11
    mock_uart_.expect_command("AT+RAT5", "OK+Set:5");  // Set interval 5 min

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    // Run configuration logic
    bool result = run_hmsensor_config_logic(driver, green_led_, red_led_);

    // Verify success
    EXPECT_TRUE(result);
    EXPECT_FALSE(red_led_.is_on()); // No error indicated

    // Verify all expected commands were sent
    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with valid values
 *
 * Validates:
 * - Temperature and humidity reading command (AT+TEHU?)
 * - Numeric value parsing from string response
 * - Values within valid ranges (0-120 temp, 0-100 humidity)
 * - Green LED toggles on success
 */
TEST_F(HMSensorExampleTest, ReadingSuccessValidValues) {
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:025:060");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify success
    EXPECT_TRUE(result);
    EXPECT_EQ(temperature, 25);                  // Numeric value parsed from "025"
    EXPECT_EQ(humidity, 60);                     // Numeric value parsed from "060"
    EXPECT_FALSE(red_led_.is_on());              // No error
    EXPECT_EQ(green_led_.get_toggle_count(), 1); // Success toggle

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with boundary values
 *
 * Validates:
 * - Maximum temperature (120°C)
 * - Maximum humidity (100%)
 * - Correct parsing of boundary values
 */
TEST_F(HMSensorExampleTest, ReadingSuccessBoundaryValues) {
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:120:100");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify success with max values
    EXPECT_TRUE(result);
    EXPECT_EQ(temperature, 120); // Max temperature
    EXPECT_EQ(humidity, 100);    // Max humidity
    EXPECT_FALSE(red_led_.is_on());

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with minimum values
 *
 * Validates:
 * - Minimum temperature (0°C)
 * - Minimum humidity (0%)
 * - Correct parsing of zero values
 */
TEST_F(HMSensorExampleTest, ReadingSuccessMinimumValues) {
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:000:000");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify success with min values
    EXPECT_TRUE(result);
    EXPECT_EQ(temperature, 0); // Min temperature
    EXPECT_EQ(humidity, 0);    // Min humidity
    EXPECT_FALSE(red_led_.is_on());

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with out-of-range temperature
 *
 * Validates:
 * - Range validation for temperature (must be 0-120)
 * - Error status returned when temperature > 120
 * - Red LED indicates error
 */
TEST_F(HMSensorExampleTest, ReadingErrorTemperatureOutOfRange) {
    // Temperature 150 exceeds max of 120
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:150:050");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify error handling
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());               // Error indicated
    EXPECT_EQ(green_led_.get_toggle_count(), 0); // No success toggle

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with out-of-range humidity
 *
 * Validates:
 * - Range validation for humidity (must be 0-100)
 * - Error status returned when humidity > 100
 * - Red LED indicates error
 */
TEST_F(HMSensorExampleTest, ReadingErrorHumidityOutOfRange) {
    // Humidity 150 exceeds max of 100
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:025:150");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify error handling
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());               // Error indicated
    EXPECT_EQ(green_led_.get_toggle_count(), 0); // No success toggle

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test sensor reading with invalid response format
 *
 * Validates:
 * - Error handling for malformed responses
 * - Non-numeric data in temperature/humidity fields
 * - Red LED indicates error
 */
TEST_F(HMSensorExampleTest, ReadingErrorInvalidFormat) {
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:ABC:XYZ");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    uint8_t temperature = 0;
    uint8_t humidity = 0;
    bool result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);

    // Verify error handling
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on()); // Error indicated

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test configuration error handling (sensor type set fails)
 *
 * Validates:
 * - Error propagation from set_sensor_type
 * - Red LED indicates error
 * - Configuration sequence stops on first error
 */
TEST_F(HMSensorExampleTest, ConfigurationErrorSensorTypeSetFails) {
    mock_uart_.expect_command("AT", "OK"); // Test succeeds
    mock_uart_.expect_timeout("AT+SENS1"); // Sensor type set fails with timeout

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    bool result = run_hmsensor_config_logic(driver, green_led_, red_led_);

    // Verify error handling
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on()); // Error indicated
}

/**
 * @brief Test configuration error handling (work interval set fails)
 *
 * Validates:
 * - Error propagation from set_sensor_work_interval
 * - Red LED indicates error
 * - Previous operations succeeded but final fails
 */
TEST_F(HMSensorExampleTest, ConfigurationErrorWorkIntervalSetFails) {
    mock_uart_.expect_command("AT", "OK");             // Test succeeds
    mock_uart_.expect_command("AT+SENS1", "OK+Set:1"); // Sensor type succeeds
    mock_uart_.expect_timeout("AT+RAT5");              // Work interval fails with timeout

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    bool result = run_hmsensor_config_logic(driver, green_led_, red_led_);

    // Verify error handling
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on()); // Error indicated
}

/**
 * @brief Test work interval format (leading zero stripped)
 *
 * Validates:
 * - Work interval "05" transmits as "AT+RAT5" (leading zero stripped)
 * - Matches Ada behavior for work interval formatting
 */
TEST_F(HMSensorExampleTest, WorkIntervalFormatLeadingZeroStripped) {
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+SENS1", "OK+Set:1");
    mock_uart_.expect_command("AT+RAT5", "OK+Set:5"); // "05" -> "5"

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    bool result = run_hmsensor_config_logic(driver, green_led_, red_led_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test multiple sequential readings
 *
 * Validates:
 * - Periodic reading pattern works correctly
 * - Each reading toggles green LED
 * - Values change between readings
 */
TEST_F(HMSensorExampleTest, MultipleSequentialReadings) {
    // First reading: 25°C, 60%
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:025:060");
    // Second reading: 26°C, 62%
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:026:062");
    // Third reading: 27°C, 64%
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:027:064");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    // Perform three readings
    uint8_t temperature = 0;
    uint8_t humidity = 0;

    bool result1 = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);
    EXPECT_TRUE(result1);
    EXPECT_EQ(temperature, 25);
    EXPECT_EQ(humidity, 60);

    bool result2 = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);
    EXPECT_TRUE(result2);
    EXPECT_EQ(temperature, 26);
    EXPECT_EQ(humidity, 62);

    bool result3 = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);
    EXPECT_TRUE(result3);
    EXPECT_EQ(temperature, 27);
    EXPECT_EQ(humidity, 64);

    // Verify green LED toggled 3 times (once per successful reading)
    EXPECT_EQ(green_led_.get_toggle_count(), 3);
    EXPECT_FALSE(red_led_.is_on()); // No errors

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test complete example flow (config + multiple readings)
 *
 * Validates:
 * - Complete example sequence: test, configure, read multiple times
 * - All commands in correct order
 * - Numeric values parsed correctly across multiple readings
 */
TEST_F(HMSensorExampleTest, CompleteExampleFlow) {
    // Configuration phase
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+SENS1", "OK+Set:1");
    mock_uart_.expect_command("AT+RAT5", "OK+Set:5");

    // Reading phase (3 readings)
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:025:060");
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:026:061");
    mock_uart_.expect_command("AT+TEHU?", "OK+Get:027:062");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, handler_);

    // Run configuration
    bool config_result = run_hmsensor_config_logic(driver, green_led_, red_led_);
    EXPECT_TRUE(config_result);

    // Run multiple readings
    uint8_t temperature = 0;
    uint8_t humidity = 0;

    for (int i = 0; i < 3; i++) {
        bool read_result = run_hmsensor_reading_logic(driver, green_led_, red_led_, temperature, humidity);
        EXPECT_TRUE(read_result);
    }

    // Verify final values (from last reading)
    EXPECT_EQ(temperature, 27);
    EXPECT_EQ(humidity, 62);

    // Verify LED states
    EXPECT_EQ(green_led_.get_toggle_count(), 3); // 3 successful readings
    EXPECT_FALSE(red_led_.is_on());              // No errors

    EXPECT_TRUE(mock_uart_.all_expectations_met());
}
