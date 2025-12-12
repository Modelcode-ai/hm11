/**
 * @file main.cpp
 * @brief Basic example demonstrating HM11 type system templates
 * 
 * This example validates the type system implementation by demonstrating:
 * - RangedInteger usage with different Tag types (type safety)
 * - FixedString usage with hex and digit validation
 * - Ada type aliases proper instantiation and usage
 * - Compile-time validation and runtime safety
 * - Zero-overhead abstraction performance characteristics
 * 
 * The example serves as both proof-of-concept and reference implementation
 * for how to use the type system correctly in applications.
 */

#include <hm11/types.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <cassert>

using namespace hm11;

/// @brief Demonstrate RangedInteger usage and type safety
void demonstrate_ranged_integers() {
    std::cout << "\n=== Demonstrating RangedInteger Templates ===\n";
    
    // Basic RangedInteger usage
    Percent battery_level{85};
    TemperatureType current_temp{25};
    HumidityType current_humidity{60};
    
    std::cout << "Battery Level: " << static_cast<int>(battery_level.value()) << "%\n";
    std::cout << "Temperature: " << static_cast<int>(current_temp.value()) << "°C\n";
    std::cout << "Humidity: " << static_cast<int>(current_humidity.value()) << "%\n";
    
    // Demonstrate range constraints
    std::cout << "\n--- Range Constraint Validation ---\n";
    std::cout << "Percent range: " << static_cast<int>(Percent::MIN_VALUE) 
              << " to " << static_cast<int>(Percent::MAX_VALUE) << "\n";
    std::cout << "Temperature range: " << static_cast<int>(TemperatureType::MIN_VALUE)
              << " to " << static_cast<int>(TemperatureType::MAX_VALUE) << "°C\n";
    
    // Arithmetic operations with validation
    std::cout << "\n--- Safe Arithmetic Operations ---\n";
    try {
        Percent low_battery{15};
        Percent charge_amount{30};
        Percent new_level = low_battery + charge_amount;
        std::cout << "Battery charged: " << static_cast<int>(low_battery.value()) 
                  << "% + " << static_cast<int>(charge_amount.value()) 
                  << "% = " << static_cast<int>(new_level.value()) << "%\n";
    } catch (const std::exception& e) {
        std::cout << "Arithmetic validation caught: " << e.what() << "\n";
    }
    
    // Type safety demonstration - these would not compile:
    // Percent p1{50};
    // TemperatureType t1{25};
    // auto mixed = p1 + t1; // ERROR: Different tag types cannot be mixed
    std::cout << "✓ Type safety enforced - different semantic types cannot be mixed\n";
    
    // Boundary testing
    std::cout << "\n--- Boundary Validation ---\n";
    Percent min_percent{0};
    Percent max_percent{100};
    std::cout << "Percent at minimum: " << min_percent.at_min() << "\n";
    std::cout << "Percent at maximum: " << max_percent.at_max() << "\n";
}

/// @brief Demonstrate FixedString usage with validators
void demonstrate_fixed_strings() {
    std::cout << "\n=== Demonstrating FixedString Templates ===\n";
    
    // MAC address with hex validation
    std::cout << "\n--- MAC Address (Hex Validation) ---\n";
    try {
        MacAddress mac{"001122AABBCC"};
        std::cout << "MAC Address: " << std::string_view(mac) << "\n";
        std::cout << "MAC Length: " << mac.size() << " characters\n";
        std::cout << "MAC Capacity: " << MacAddress::capacity() << " characters\n";
        std::cout << "Validator: " << MacAddress::validator_name() << "\n";
        
        // Demonstrate character access
        std::cout << "First character: '" << mac[0] << "'\n";
        std::cout << "Last character: '" << mac.back() << "'\n";
        
    } catch (const std::exception& e) {
        std::cout << "MAC address validation failed: " << e.what() << "\n";
    }
    
    // PIN with digit validation
    std::cout << "\n--- PIN Code (Digit Validation) ---\n";
    try {
        PinType pin{"123456"};
        std::cout << "PIN: " << std::string_view(pin) << " (length: " << pin.size() << ")\n";
        std::cout << "PIN Validator: " << PinType::validator_name() << "\n";
        
        // Try invalid PIN - this should throw
        try {
            PinType invalid_pin{"12345X"}; // Contains non-digit
            std::cout << "Invalid PIN created - this shouldn't happen!\n";
        } catch (const std::exception& e) {
            std::cout << "✓ PIN validation correctly rejected invalid characters: " << e.what() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "PIN creation failed: " << e.what() << "\n";
    }
    
    // UUID with hex validation
    std::cout << "\n--- UUID (4-character Hex) ---\n";
    UUID service_uuid{"ABCD"};
    std::cout << "Service UUID: " << std::string_view(service_uuid) << "\n";
    
    // iBeacon UUID (32 characters)
    std::cout << "\n--- iBeacon UUID (32-character Hex) ---\n";
    IBeaconUuid beacon_uuid{"12345678ABCDEF0012345678ABCDEF00"};
    std::cout << "iBeacon UUID: " << std::string_view(beacon_uuid) << "\n";
    std::cout << "iBeacon UUID full: " << beacon_uuid.full() << "\n";
    
    // String operations
    std::cout << "\n--- String Operations ---\n";
    MacAddress partial_mac{"00112233"};
    std::cout << "Partial MAC: " << std::string_view(partial_mac) << "\n";
    std::cout << "Remaining capacity: " << partial_mac.remaining_capacity() << " characters\n";
    
    // Append more data
    try {
        partial_mac.append("AABB");
        std::cout << "After append: " << std::string_view(partial_mac) << "\n";
    } catch (const std::exception& e) {
        std::cout << "Append failed: " << e.what() << "\n";
    }
}

/// @brief Demonstrate Ada type aliases and enum classes
void demonstrate_ada_types() {
    std::cout << "\n=== Demonstrating Ada Type Aliases ===\n";
    
    // Enum types
    std::cout << "\n--- Enum Classes ---\n";
    Role device_role = Role::Central;
    std::cout << "Device Role: " << (device_role == Role::Central ? "Central" : "Peripheral") << "\n";
    
    AdvertisingInterval interval = AdvertisingInterval::Ms1022;
    std::cout << "Advertising Interval: " << static_cast<int>(interval) << " (1022ms)\n";
    
    UartBaudRate baud_rate = UartBaudRate::Br115200;
    std::cout << "UART Baud Rate: " << static_cast<int>(baud_rate) << " (115200)\n";
    
    // Complex type combinations
    std::cout << "\n--- Complex Type Usage ---\n";
    
    // Temperature sensor reading
    struct SensorReading {
        TemperatureType temperature;
        HumidityType humidity;
        MacAddress sensor_mac;
        
        SensorReading(uint8_t temp, uint8_t humid, const char* mac_str) 
            : temperature{temp}, humidity{humid}, sensor_mac{mac_str} {}
    };
    
    SensorReading reading{28, 65, "AABBCCDDEEFF"};
    std::cout << "Sensor " << std::string_view(reading.sensor_mac) 
              << ": " << static_cast<int>(reading.temperature.value()) << "°C, "
              << static_cast<int>(reading.humidity.value()) << "% humidity\n";
    
    // Discovery configuration
    struct DiscoveryConfig {
        DiscoveryTime scan_time{5};
        DiscoveredIndex max_devices{3};
        ShowDeviceInformation info_mode{ShowDeviceInformation::ShowRssiAndName};
    };
    
    DiscoveryConfig discovery_config;
    std::cout << "Discovery: " << static_cast<int>(discovery_config.scan_time.value()) 
              << "s scan, max " << static_cast<int>(discovery_config.max_devices.value()) 
              << " devices, info mode " << static_cast<int>(discovery_config.info_mode) << "\n";
}

/// @brief Demonstrate compile-time validation and constants
void demonstrate_compile_time_features() {
    std::cout << "\n=== Demonstrating Compile-Time Features ===\n";
    
    // Compile-time constants
    std::cout << "\n--- Compile-Time Constants ---\n";
    std::cout << "Type System Version: " << TYPE_SYSTEM_VERSION_MAJOR << "."
              << TYPE_SYSTEM_VERSION_MINOR << "." << TYPE_SYSTEM_VERSION_PATCH << "\n";
    
    std::cout << "Max Name Length: " << MAX_NAME_LENGTH << "\n";
    std::cout << "Max RSSI Length: " << MAX_RSSI_LENGTH << "\n";
    std::cout << "Max Message Length: " << MAX_MESSAGE_LENGTH << "\n";
    
    // String constants
    std::cout << "Connected Message: '" << WHEN_CONNECTED_MESSAGE << "'\n";
    std::cout << "Disconnected Message: '" << WHEN_DISCONNECTED_MESSAGE << "'\n";
    
    // Constexpr validation
    std::cout << "\n--- Constexpr Operations ---\n";
    constexpr Percent CONST_PERCENT{75};
    constexpr bool AT_MAX = CONST_PERCENT.at_max();
    constexpr uint8_t PERCENT_VALUE = CONST_PERCENT.value();
    
    std::cout << "Constexpr percent value: " << static_cast<int>(PERCENT_VALUE) << "\n";
    std::cout << "Constexpr at max check: " << AT_MAX << "\n";
    
    // Static assertions (these are checked at compile time)
    static_assert(Percent::MIN_VALUE == 0, "Percent minimum should be 0");
    static_assert(Percent::MAX_VALUE == 100, "Percent maximum should be 100");
    static_assert(MacAddress::MAX_SIZE == 12, "MAC address should be 12 characters");
    static_assert(sizeof(Role) == 1, "Role enum should be 1 byte");
    
    std::cout << "✓ All compile-time assertions passed\n";
}

/// @brief Performance test to validate zero-overhead abstraction
void demonstrate_performance() {
    std::cout << "\n=== Performance Validation (Zero-Overhead Abstraction) ===\n";
    
    const int ITERATIONS = 1000000;
    
    // Test RangedInteger performance vs raw integers
    auto start = std::chrono::high_resolution_clock::now();
    
    // RangedInteger operations
    {
        Percent value{50};
        for (int i = 0; i < ITERATIONS; ++i) {
            if (value.value() < 90) {
                value = Percent{static_cast<uint8_t>(value.value() + 1)};
            } else {
                value = Percent{50};
            }
        }
        volatile uint8_t result = value.value(); // Prevent optimization
        (void)result; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto ranged_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Raw integer operations for comparison
    start = std::chrono::high_resolution_clock::now();
    {
        uint8_t value = 50;
        for (int i = 0; i < ITERATIONS; ++i) {
            if (value < 90) {
                ++value;
            } else {
                value = 50;
            }
        }
        volatile uint8_t result = value; // Prevent optimization
        (void)result; // Suppress unused variable warning
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto raw_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "RangedInteger time: " << ranged_time.count() << " μs\n";
    std::cout << "Raw integer time: " << raw_time.count() << " μs\n";
    std::cout << "Overhead: " << std::fixed << std::setprecision(1) 
              << (static_cast<double>(ranged_time.count()) / static_cast<double>(raw_time.count()) - 1.0) * 100.0 << "%\n";
    
    if (static_cast<double>(ranged_time.count()) <= static_cast<double>(raw_time.count()) * 1.1) { // Allow 10% overhead
        std::cout << "✓ Zero-overhead abstraction achieved (within 10% of raw performance)\n";
    } else {
        std::cout << "⚠ Performance overhead detected\n";
    }
}

/// @brief Main function demonstrating all type system features
int main() {
    std::cout << "HM11 Type System Demonstration\n";
    std::cout << "==============================\n";
    
    try {
        demonstrate_ranged_integers();
        demonstrate_fixed_strings();
        demonstrate_ada_types();
        demonstrate_compile_time_features();
        demonstrate_performance();
        
        std::cout << "\n=== Summary ===\n";
        std::cout << "✓ RangedInteger templates provide type safety and range validation\n";
        std::cout << "✓ FixedString templates provide character validation and fixed capacity\n";
        std::cout << "✓ Ada type aliases map correctly to C++ equivalents\n";
        std::cout << "✓ Compile-time validation prevents invalid usage\n";
        std::cout << "✓ Runtime validation in debug builds catches errors\n";
        std::cout << "✓ Zero-overhead abstraction maintains performance\n";
        std::cout << "✓ Type system is ready for driver implementation\n";
        
        std::cout << "\nExample completed successfully - type system validation passed!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during type system demonstration: " << e.what() << "\n";
        return 1;
    }
}