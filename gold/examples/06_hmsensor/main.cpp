/**
 * @file main.cpp
 * @brief HMSensor reader example for HM-11 driver
 *
 * This example demonstrates HMSensor-specific features for environmental monitoring.
 * The HMSensor is a variant of the HM-11 module with integrated temperature and
 * humidity sensor capabilities (DHT11 or DS18B20 sensor types).
 *
 * ## What This Example Demonstrates
 *
 * 1. Sensor type configuration (DHT11, DS18B20, None)
 * 2. Work interval setting (sensor reading frequency in minutes)
 * 3. Temperature and humidity reading with numeric value parsing
 * 4. Periodic polling pattern for sensor data
 * 5. Error handling for unconfigured or missing sensors
 * 6. Numeric value representation per Decision 29
 *
 * ## HMSensor Overview (Decision 15)
 *
 * Per Decision 15, all HMSensor operations are always present in the driver API
 * and documented as "HMSensor-only". Hardware returns errors if called on
 * non-HMSensor modules. This example requires an HMSensor variant module.
 *
 * The HMSensor variant provides:
 * - **Sensor Type Configuration**: DHT11 (temp/humidity) or DS18B20 (temperature only)
 * - **Work Interval**: How often sensor automatically reads (0-99 minutes)
 * - **Temperature Reading**: 0-120°C range (uint8_t)
 * - **Humidity Reading**: 0-100% range (uint8_t)
 *
 * ## Sensor Types
 *
 * - **None**: No sensor configured (default)
 * - **DHT11**: Temperature (0-50°C typical) and humidity (20-80% typical) sensor
 * - **DS18B20**: Temperature-only sensor (-55°C to +125°C range)
 *
 * ## Hardware Requirements (STM32F429 target)
 *
 * - STM32F429 Discovery board
 * - HMSensor Bluetooth module (HM-11 variant) connected to UART5:
 *   - PC12 (UART5_TX) -> HMSensor RX (pin 4)
 *   - PD2 (UART5_RX) -> HMSensor TX (pin 2)
 *   - 3.3V and GND connections
 * - DHT11 or DS18B20 sensor connected to HMSensor module
 * - LED indicators for status feedback
 *
 * ## Expected Behavior
 *
 * Success sequence:
 * 1. Initialization phase: Configure UART, DMA, and driver (~4 seconds)
 * 2. Configuration phase: Set sensor type (DHT11) and work interval (5 minutes)
 * 3. Reading loop: Query temperature and humidity every 5 seconds
 * 4. Green LED blinks with each successful reading
 * 5. Temperature and humidity values displayed (if output available)
 *
 * Error sequence:
 * 1. Red LED blinks continuously if any operation fails
 * 2. System halts in error state
 *
 * ## Numeric Value Parsing (Decision 29)
 *
 * Temperature and humidity are parsed into numeric uint8_t values:
 * - **Temperature**: 0-120 range (Natural range 0..120 in Ada)
 * - **Humidity**: 0-100 range (Natural range 0..100 in Ada)
 * - **Parsing**: Driver parses hardware response strings (e.g., "025", "060")
 *   into integers with range validation
 * - **Decimals**: Values like "25.5" are truncated to integers (25)
 *
 * This provides convenient numeric values for applications without requiring
 * every caller to parse strings.
 *
 * ## Configuration Summary
 *
 * This example configures:
 * - **Sensor Type**: DHT11 (temperature and humidity)
 * - **Work Interval**: 05 minutes (sensor reads automatically every 5 minutes)
 * - **Read Period**: Query sensor every 5 seconds in main loop
 *
 * ## Important Notes
 *
 * 1. **Sensor Configuration Required**: Sensor type must be set before reading.
 *    Module returns error if sensor type is None.
 * 2. **Work Interval**: Controls how often sensor hardware reads. Set to 0 for
 *    manual-only reading, or 1-99 for automatic periodic reading.
 * 3. **Reading Latency**: Sensor readings may take 1-2 seconds to complete
 *    depending on sensor type and environmental conditions.
 * 4. **Range Validation**: Driver validates temperature (0-120) and humidity
 *    (0-100) ranges, returning error if out of bounds.
 *
 * ## Timing Considerations
 *
 * - Initial delay: 2 seconds (hardware power-up)
 * - Between init steps: 1 second
 * - Between AT commands: 0.5 seconds
 * - Reading loop: 5 seconds between queries
 *
 * ## AT Command Reference
 *
 * - **AT+SENS1**: Set sensor type to DHT11
 * - **AT+SENS?**: Query current sensor type
 * - **AT+RAT5**: Set work interval to 5 minutes
 * - **AT+RAT?**: Query current work interval
 * - **AT+TEHU?**: Query temperature and humidity
 *   - Response: "OK+Get:TTT:HHH" (3-digit temp, colon, 3-digit humidity)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <chrono>
#include <cstdint>
#include <thread>

#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/stm32/drivers.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

/**
 * @brief Indicate error with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
 */
[[noreturn]] void on_error() noexcept {
    while (true) {
        LED_RED_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Check operation status and indicate error if failed
 *
 * Provides consistent error handling pattern used throughout the example.
 * If status is Ok, continues normally. Otherwise, enters error state.
 *
 * @param status Operation status to check
 */
void check_status(UartStatus status) noexcept {
    if (status != UartStatus::Ok) {
        on_error();
    }
}

/**
 * @brief Main example entry point
 *
 * Demonstrates complete HMSensor configuration and periodic reading.
 */
int main() {
    using namespace hm11;
    using namespace hm11::hal;
    using namespace hm11::stm32;
    using namespace hm11::types;

    UartStatus status;

    // ========================================================================
    // Phase 1: Hardware Initialization
    // ========================================================================

    // Delay for hardware power-up stabilization
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Initialize UART5 peripheral
    init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Initialize DMA for UART5 reception
    initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get driver reference
    auto& driver = get_hm11_driver();

    // ========================================================================
    // Phase 2: Module Verification
    // ========================================================================

    // Test module responsiveness
    driver.test(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 3: HMSensor Configuration
    // ========================================================================
    // Configure sensor type and work interval
    // Ada source: hm11.adb:2134-2225 (sensor operations)

    // Configure sensor type to DHT11 (temperature and humidity)
    // DHT11: 0-50°C temperature range, 20-80% humidity range typical
    // DS18B20 alternative: Temperature-only, -55°C to +125°C range
    driver.set_sensor_type(SensorType::DHT11, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure work interval to 5 minutes
    // Work interval controls how often sensor hardware automatically reads
    // Range: 00-99 minutes (0 = manual only, 1-99 = automatic periodic)
    WorkInterval interval{"05"};
    driver.set_sensor_work_interval(interval, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 4: Periodic Reading Loop
    // ========================================================================
    // Query temperature and humidity periodically
    // Ada source: hm11.adb:2198-2225 (Get_Sensor_Temperature_And_Humidity)

    // In embedded system, enter infinite reading loop
    // In hosted environment, perform limited readings for testing
#ifdef __ARM_ARCH
    // Embedded target - infinite loop
    const int MAX_READINGS = -1; // Infinite
#else
    // Hosted target - limited readings for testing
    const int MAX_READINGS = 10;
#endif

    int reading_count = 0;
    while (MAX_READINGS < 0 || reading_count < MAX_READINGS) {
        // Query temperature and humidity
        // Response format: "OK+Get:TTT:HHH" (3-digit temp, 3-digit humidity)
        // Values are parsed into uint8_t with range validation
        uint8_t temperature = 0;
        uint8_t humidity = 0;
        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        if (status == UartStatus::Ok) {
            // Reading successful
            // Temperature: 0-120°C range (uint8_t)
            // Humidity: 0-100% range (uint8_t)
            // Toggle green LED to indicate successful reading
            LED_GREEN_TOGGLE();

            // In real application, display or process values here
            // Example: Send to UART, log to storage, update display, etc.
            // Note: Values are already numeric (uint8_t), no parsing needed

            // For demonstration, we just continue reading
            // Real applications would process temperature and humidity values
        } else {
            // Reading failed - sensor may not be configured or hardware issue
            check_status(status);
        }

        // Wait 5 seconds before next reading
        // Adjust this interval based on application requirements
        // Shorter intervals: More responsive but higher power consumption
        // Longer intervals: Lower power but less frequent updates
        std::this_thread::sleep_for(std::chrono::seconds(5));

        reading_count++;
    }

    // ========================================================================
    // Phase 5: Completion (Non-Embedded Only)
    // ========================================================================
    // Hosted target completes after limited readings
    // Embedded target never reaches here (infinite loop above)

#ifndef __ARM_ARCH
    // Turn on green LED to indicate successful completion
    LED_GREEN_ON();
    return 0;
#endif
}
