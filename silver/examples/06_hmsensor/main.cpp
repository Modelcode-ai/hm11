// Example 06: HMSensor-Specific Features Demonstration
// 
// This example demonstrates the complete HMSensor feature set:
// 1. Sensor type configuration (None, DHT11, DS18B20)
// 2. Work interval management (0-99 minutes) 
// 3. Temperature and humidity reading with numeric parsing
//
// Design Implementation:
// - Follows Decision #15 (HMSensor-Specific Features) by including all sensor
//   APIs as regular methods on main driver class with "HMSensor-only" documentation
// - Implements Decision #29 (Sensor Reading Value Representation) by parsing
//   sensor readings into numeric TemperatureType (0-120) and HumidityType (0-100)
// - Uses RangedInteger validation (Decision #2) for type safety
// - Demonstrates unified API approach matching Ada's single-type design
//
// Hardware Requirements:
// - HMSensor module (not regular HM-11)
// - Connected DHT11 or DS18B20 sensor on PIO11 (PIO3 on HM-11)
//
// Expected Output:
// - Successful sensor configuration
// - Valid temperature/humidity readings in specified ranges
// - Proper error handling for unsupported operations on regular HM-11

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "hm11/hm11_driver.hpp"
#include "hm11/hal/mock_uart.hpp"

#ifdef HM11_STM32_PLATFORM
#include "hm11/hal/stm32_uart_port.hpp"
#endif

using namespace hm11;
using namespace hm11::hal;

// LED status indicators for visual feedback
namespace status_led {
void init() {
    // TODO: Initialize status LEDs on actual hardware
    // For now, use console output
}

void set_success() {
    std::cout << "[LED] SUCCESS (Green)\n";
}

void set_error() {
    std::cout << "[LED] ERROR (Red)\n";
}

void set_warning() {
    std::cout << "[LED] WARNING (Yellow)\n";
}

void set_working() {
    std::cout << "[LED] WORKING (Blue)\n";
}
}

// Helper function to print sensor type
void print_sensor_type(SensorType type) {
    switch (type) {
        case SensorType::None:
            std::cout << "None (No sensor connected)";
            break;
        case SensorType::DHT11:
            std::cout << "DHT11 (Temperature & Humidity sensor)";
            break;
        case SensorType::DS18B20:
            std::cout << "DS18B20 (Temperature-only sensor)";
            break;
    }
}

// Helper function to check and display status
bool check_status(UARTStatus status, const char* operation) {
    if (status == UARTStatus::Ok) {
        std::cout << "[✓] " << operation << " - SUCCESS\n";
        return true;
    }
    std::cout << "[✗] " << operation << " - FAILED (Status: "
              << static_cast<int>(status) << ")\n";
    status_led::set_error();
    return false;
}

// Demonstrate sensor type configuration
void demonstrate_sensor_type_configuration(auto& driver) {
    std::cout << "\n=== Sensor Type Configuration Demo ===\n";
    status_led::set_working();

    UARTStatus status = UARTStatus::Ok;
    SensorType current_type = SensorType::DHT11;

    // Get current sensor type
    std::cout << "\n1. Reading current sensor type...\n";
    driver.get_sensor_type(current_type, status);
    if (check_status(status, "Get sensor type")) {
        std::cout << "   Current sensor type: ";
        print_sensor_type(current_type);
        std::cout << "\n";
    }

    // Configure for DHT11 sensor
    std::cout << "\n2. Configuring sensor type to DHT11...\n";
    driver.set_sensor_type(SensorType::DHT11, status);
    if (check_status(status, "Set sensor type to DHT11")) {
        
        // Verify the configuration
        driver.get_sensor_type(current_type, status);
        if (check_status(status, "Verify DHT11 configuration")) {
            std::cout << "   Verified sensor type: ";
            print_sensor_type(current_type);
            std::cout << "\n";
        }
    }

    // Test DS18B20 configuration
    std::cout << "\n3. Testing DS18B20 sensor configuration...\n";
    driver.set_sensor_type(SensorType::DS18B20, status);
    if (check_status(status, "Set sensor type to DS18B20")) {
        
        driver.get_sensor_type(current_type, status);
        if (check_status(status, "Verify DS18B20 configuration")) {
            std::cout << "   Verified sensor type: ";
            print_sensor_type(current_type);
            std::cout << "\n";
        }
    }

    // Reset to original configuration
    if (current_type != SensorType::DHT11) {
        std::cout << "\n4. Restoring DHT11 configuration for sensor reading demo...\n";
        driver.set_sensor_type(SensorType::DHT11, status);
        check_status(status, "Restore DHT11 configuration");
    }
}

// Demonstrate work interval configuration
void demonstrate_work_interval_configuration(auto& driver) {
    std::cout << "\n=== Work Interval Configuration Demo ===\n";
    status_led::set_working();

    UARTStatus status = UARTStatus::Ok;
    WorkInterval current_interval("99");

    // Get current work interval
    std::cout << "\n1. Reading current work interval...\n";
    driver.get_sensor_work_interval(current_interval, status);
    if (check_status(status, "Get work interval")) {
        std::cout << "   Current work interval: " << std::string_view(current_interval) 
                  << " minutes\n";
    }

    // Test single-digit interval (00-09)
    std::cout << "\n2. Setting single-digit work interval (05 minutes)...\n";
    WorkInterval short_interval("05");
    driver.set_sensor_work_interval(short_interval, status);
    if (check_status(status, "Set work interval to 05 minutes")) {
        
        WorkInterval verify_interval("99");
        driver.get_sensor_work_interval(verify_interval, status);
        if (check_status(status, "Verify short interval")) {
            std::cout << "   Verified interval: " << std::string_view(verify_interval) 
                      << " minutes\n";
        }
    }

    // Test double-digit interval (10-99)
    std::cout << "\n3. Setting double-digit work interval (15 minutes)...\n";
    WorkInterval long_interval("15");
    driver.set_sensor_work_interval(long_interval, status);
    if (check_status(status, "Set work interval to 15 minutes")) {
        
        WorkInterval verify_interval("99");
        driver.get_sensor_work_interval(verify_interval, status);
        if (check_status(status, "Verify long interval")) {
            std::cout << "   Verified interval: " << std::string_view(verify_interval) 
                      << " minutes\n";
        }
    }

    // Test minimum and maximum values
    std::cout << "\n4. Testing edge values...\n";
    
    // Minimum value: 00
    WorkInterval min_interval("00");
    driver.set_sensor_work_interval(min_interval, status);
    if (check_status(status, "Set minimum interval (00)")) {
        WorkInterval verify_min("99");
        driver.get_sensor_work_interval(verify_min, status);
        if (check_status(status, "Verify minimum interval")) {
            std::cout << "   Minimum interval: " << std::string_view(verify_min) 
                      << " minutes\n";
        }
    }

    // Maximum value: 99
    WorkInterval max_interval("99");
    driver.set_sensor_work_interval(max_interval, status);
    if (check_status(status, "Set maximum interval (99)")) {
        WorkInterval verify_max("00");
        driver.get_sensor_work_interval(verify_max, status);
        if (check_status(status, "Verify maximum interval")) {
            std::cout << "   Maximum interval: " << std::string_view(verify_max) 
                      << " minutes\n";
        }
    }
}

// Demonstrate temperature and humidity reading
void demonstrate_sensor_readings(auto& driver) {
    std::cout << "\n=== Temperature and Humidity Reading Demo ===\n";
    status_led::set_working();

    UARTStatus status = UARTStatus::Ok;

    // Perform multiple sensor readings
    std::cout << "\n1. Taking sensor readings (5 samples)...\n";
    for (int i = 1; i <= 5; ++i) {
        TemperatureType temperature(0);
        HumidityType humidity(0);

        std::cout << "\n   Sample " << i << ": ";
        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);
        
        if (check_status(status, "Read temperature & humidity")) {
            std::cout << "      Temperature: " << static_cast<int>(temperature.value()) 
                      << "°C (Range: " << static_cast<int>(TemperatureType::MIN_VALUE)
                      << "-" << static_cast<int>(TemperatureType::MAX_VALUE) << "°C)\n";
            std::cout << "      Humidity:    " << static_cast<int>(humidity.value()) 
                      << "% (Range: " << static_cast<int>(HumidityType::MIN_VALUE)
                      << "-" << static_cast<int>(HumidityType::MAX_VALUE) << "%)\n";

            // Validate ranges (should always pass due to RangedInteger validation)
            if (temperature.value() >= TemperatureType::MIN_VALUE && 
                temperature.value() <= TemperatureType::MAX_VALUE) {
                std::cout << "      Temperature range: VALID ✓\n";
            } else {
                std::cout << "      Temperature range: INVALID ✗\n";
            }

            if (humidity.value() >= HumidityType::MIN_VALUE && 
                humidity.value() <= HumidityType::MAX_VALUE) {
                std::cout << "      Humidity range: VALID ✓\n";
            } else {
                std::cout << "      Humidity range: INVALID ✗\n";
            }
        }

        // Small delay between readings
        if (i < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    std::cout << "\n2. Sensor reading statistics:\n";
    std::cout << "   - Temperature type: RangedInteger<uint8_t, 0, 120, TemperatureTag>\n";
    std::cout << "   - Humidity type: RangedInteger<uint8_t, 0, 100, HumidityTag>\n";
    std::cout << "   - Validation: Automatic range checking on every operation\n";
    std::cout << "   - Parsing: Hardware strings -> validated numeric types\n";
}

// Demonstrate error handling
void demonstrate_error_handling(auto& driver) {
    std::cout << "\n=== Error Handling Demo ===\n";
    status_led::set_working();

    std::cout << "\n1. Testing behavior on non-HMSensor modules:\n";
    std::cout << "   Note: These operations may fail on regular HM-11 modules\n";
    std::cout << "   that don't support HMSensor-specific features.\n";
    std::cout << "   The driver provides a unified API and lets hardware report\n";
    std::cout << "   appropriate error codes for unsupported operations.\n";

    UARTStatus status = UARTStatus::Ok;
    SensorType sensor_type = SensorType::DHT11;
    
    // This might fail on non-HMSensor hardware
    driver.get_sensor_type(sensor_type, status);
    if (status == UARTStatus::Ok) {
        std::cout << "   ✓ HMSensor features are supported\n";
    } else {
        std::cout << "   ⚠ HMSensor features not supported (expected for regular HM-11)\n";
        status_led::set_warning();
    }

    std::cout << "\n2. API Design Benefits:\n";
    std::cout << "   - Unified interface for all HM-11 variants\n";
    std::cout << "   - No conditional compilation or feature flags\n";
    std::cout << "   - Hardware enforces constraints via status codes\n";
    std::cout << "   - Clear error propagation via out parameters\n";
}

#ifdef HM11_STM32_PLATFORM
void run_with_stm32_hardware() {
    std::cout << "Running HMSensor example with STM32 hardware...\n";
    
    // TODO: Initialize STM32 UART port with proper configuration
    // STM32UARTPort uart_port(/* configuration parameters */);
    // auto driver = make_hm11_driver(std::move(uart_port));
    
    std::cout << "STM32 hardware initialization not yet implemented.\n";
    std::cout << "Please implement STM32UARTPort initialization for your hardware.\n";
}
#endif

void run_with_mock_uart() {
    std::cout << "Running HMSensor example with mock UART (simulation)...\n\n";
    
    // Create mock UART for demonstration
    MockUART mock_uart(UARTDataSize::DataSize8b);
    
    // Set up mock responses for sensor operations
    // Note: In real testing, these would be configured per operation
    mock_uart.SetForcedStatus(UARTStatus::Ok);
    mock_uart.SetForcedStatusForReceive(UARTStatus::Ok);
    
    // Create driver with mock UART
    auto driver = make_hm11_driver(std::move(mock_uart));

    std::cout << "Mock UART configured for HMSensor operation simulation.\n";
    std::cout << "Note: Actual sensor readings require real HMSensor hardware.\n";

    // Demonstrate basic connectivity
    std::cout << "\n=== Basic Connectivity Test ===\n";
    UARTStatus status = UARTStatus::Ok;
    driver.Test(status);
    if (check_status(status, "HM11 connectivity test")) {
        status_led::set_success();
    }

    // Run sensor feature demonstrations
    demonstrate_sensor_type_configuration(driver);
    demonstrate_work_interval_configuration(driver);
    demonstrate_sensor_readings(driver);
    demonstrate_error_handling(driver);
    
    status_led::set_success();
}

int main() {
    std::cout << "=============================================================\n";
    std::cout << "HM11 Driver - Example 06: HMSensor-Specific Features\n";
    std::cout << "=============================================================\n";
    std::cout << "\nThis example demonstrates:\n";
    std::cout << "• Sensor type configuration (None, DHT11, DS18B20)\n";
    std::cout << "• Work interval management (0-99 minutes)\n";
    std::cout << "• Temperature/humidity reading with numeric parsing\n";
    std::cout << "• Unified API approach for all HM-11 variants\n";
    std::cout << "• Type safety through RangedInteger validation\n\n";

    // Initialize status LEDs
    status_led::init();
    
    try {
#ifdef HM11_STM32_PLATFORM
        run_with_stm32_hardware();
#else
        run_with_mock_uart();
#endif
        
        std::cout << "\n=== Example Completed Successfully ===\n";
        std::cout << "\nKey Takeaways:\n";
        std::cout << "1. All sensor methods are regular driver methods (Decision #15)\n";
        std::cout << "2. Temperature/humidity parsed to numeric types (Decision #29)\n";
        std::cout << "3. Range validation automatic via RangedInteger (Decision #2)\n";
        std::cout << "4. Hardware enforces feature support via status codes\n";
        std::cout << "5. Unified API works with all HM-11 variants\n";
        
        status_led::set_success();
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "\n[EXCEPTION] " << e.what() << "\n";
        status_led::set_error();
        return 1;
    }
}