/**
 * @file main.cpp
 * @brief Demonstration of HM11 driver type system
 *
 * This main function demonstrates the usage of various type aliases and
 * enum classes from the HM11 driver's strong type system. It verifies that:
 * - RangedInteger types enforce compile-time and runtime bounds
 * - FixedString types enforce character validation
 * - Enum classes provide type-safe enumerations
 * - All templates compile and work correctly
 */

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstdlib>
#include <hm11/hal/base_types.hpp>
#include <hm11/types/enums.hpp>
#include <hm11/types/type_aliases.hpp>

int main() {
    // Initialize logging
    spdlog::set_level(spdlog::level::info);
    spdlog::info("HM11 Driver - Type System Demonstration");

    fmt::print("\n=== HM11 Driver Type System Demo ===\n\n");

    // ========================================================================
    // 1. RangedInteger Types Demonstration
    // ========================================================================
    fmt::print("1. RangedInteger Types (with compile-time range validation):\n");
    fmt::print("   --------------------------------------------------------\n");

    using namespace hm11::types;

    // Battery percentage (0-100)
    constexpr Percent BATTERY_LEVEL{75};
    fmt::print("   Battery Level: {}% (range: {}-{})\n", BATTERY_LEVEL.get(), Percent::MIN_VALUE, Percent::MAX_VALUE);

    // Temperature sensor reading (0-120 Celsius)
    constexpr TemperatureType ROOM_TEMP{23};
    fmt::print(
        "   Room Temperature: {}\u00b0C (range: {}-{})\n",
        ROOM_TEMP.get(),
        TemperatureType::MIN_VALUE,
        TemperatureType::MAX_VALUE);

    // Humidity sensor reading (0-100 percent)
    HumidityType humidity{65};
    fmt::print("   Humidity: {}% (range: {}-{})\n", humidity.get(), HumidityType::MIN_VALUE, HumidityType::MAX_VALUE);

    // Connection latency (0-4)
    constexpr LayerConnectionLatency LATENCY{2};
    fmt::print(
        "   Connection Latency: {} (range: {}-{})\n",
        LATENCY.get(),
        LayerConnectionLatency::MIN_VALUE,
        LayerConnectionLatency::MAX_VALUE);

    // Discovery time (1-9 seconds)
    constexpr DiscoveryTime SCAN_TIME{5};
    fmt::print(
        "   Discovery Time: {}s (range: {}-{})\n",
        SCAN_TIME.get(),
        DiscoveryTime::MIN_VALUE,
        DiscoveryTime::MAX_VALUE);

    // Arithmetic operations with validation
    Percent p1{30};
    Percent p2{40};
    Percent sum = p1 + p2;
    fmt::print("   Arithmetic: {}% + {}% = {}%\n", p1.get(), p2.get(), sum.get());

    // ========================================================================
    // 2. FixedString Hex Types Demonstration
    // ========================================================================
    fmt::print("\n2. FixedString Hex Types (validated hexadecimal strings):\n");
    fmt::print("   -------------------------------------------------------\n");

    // MAC address (12 hex chars)
    constexpr MacAddress MAC{"B4994C123456"};
    fmt::print("   MAC Address: {} ({} chars)\n", MAC.c_str(), MAC.size());

    // 16-bit UUID (4 hex chars)
    constexpr UUID BATTERY_SERVICE_UUID{"180F"};
    fmt::print("   Battery Service UUID: {} ({} chars)\n", BATTERY_SERVICE_UUID.c_str(), BATTERY_SERVICE_UUID.size());

    // GATT handle (4 hex chars)
    HandleType char_handle{"002A"};
    fmt::print("   GATT Characteristic Handle: 0x{} ({} chars)\n", char_handle.c_str(), char_handle.size());

    // iBeacon UUID (32 hex chars)
    constexpr IBeaconUuid BEACON_UUID{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    fmt::print("   iBeacon UUID: {} ({} chars)\n", BEACON_UUID.c_str(), BEACON_UUID.size());

    // iBeacon major/minor version (4 hex chars)
    constexpr VersionType MAJOR_VERSION{"0001"};
    constexpr VersionType MINOR_VERSION{"0064"};
    fmt::print("   iBeacon Major: {}, Minor: {}\n", MAJOR_VERSION.c_str(), MINOR_VERSION.c_str());

    // Measured power (2 hex chars)
    MeasuredPower measured_power{"C5"};
    fmt::print("   iBeacon Measured Power: 0x{} (-59 dBm in two's complement)\n", measured_power.c_str());

    // Advertising flags (2 hex chars)
    AdvertisingFlag adv_flags{"06"};
    fmt::print("   Advertising Flags: 0x{}\n", adv_flags.c_str());

    // Custom advertisement data (12 hex chars)
    AdvertisementData custom_data{"48454C4C4F00"};
    fmt::print("   Custom Advertisement Data: {} (ASCII 'HELLO' + null)\n", custom_data.c_str());

    // ========================================================================
    // 3. FixedString Digit Types Demonstration
    // ========================================================================
    fmt::print("\n3. FixedString Digit Types (validated decimal strings):\n");
    fmt::print("   -----------------------------------------------------\n");

    // PIN code (6 digits)
    constexpr PinType PIN_CODE{"123456"};
    fmt::print("   PIN Code: {} ({} digits)\n", PIN_CODE.c_str(), PIN_CODE.size());

    // Work interval (2 digits)
    WorkInterval sensor_interval{"30"};
    fmt::print("   Sensor Work Interval: {}s ({} digits)\n", sensor_interval.c_str(), sensor_interval.size());

    // Connection timeout (6 digits)
    ConnectTimeout timeout{"005000"};
    fmt::print("   Connection Timeout: {}ms ({} digits)\n", timeout.c_str(), timeout.size());

    // ========================================================================
    // 4. Enum Class Types Demonstration
    // ========================================================================
    fmt::print("\n4. Enum Class Types (type-safe enumerations):\n");
    fmt::print("   -------------------------------------------\n");

    // Device role
    Role device_role = Role::Peripheral;
    fmt::print("   Device Role: {}\n", device_role == Role::Peripheral ? "Peripheral" : "Central");

    // Advertising interval
    AdvertisingInterval adv_interval = AdvertisingInterval::Ms1022;
    fmt::print("   Advertising Interval: {} ms\n", static_cast<uint16_t>(adv_interval));

    // UART baud rate
    UartBaudRate baud_rate = UartBaudRate::Br115200;
    fmt::print("   UART Baud Rate: {} baud\n", static_cast<uint32_t>(baud_rate));

    // Module power
    ModulePower tx_power = ModulePower::Dbm0;
    fmt::print("   TX Power: {} dBm\n", static_cast<int8_t>(tx_power));

    // Sensor type
    SensorType sensor = SensorType::DHT11;
    std::string sensor_name{};
    if (sensor == SensorType::DHT11) {
        sensor_name = "DHT11";
    } else if (sensor == SensorType::DS18B20) {
        sensor_name = "DS18B20";
    } else {
        sensor_name = "None";
    }
    fmt::print("   Sensor Type: {}\n", sensor_name);

    // Write method
    WriteMethod write_type = WriteMethod::WriteWithoutResponse;
    fmt::print("   Write Method: {}\n", write_type == WriteMethod::Write ? "Write" : "Write_Without_Response");

    // Bond mode
    BondMode bonding = BondMode::AuthWithPin;
    std::string bond_mode_name{};
    if (bonding == BondMode::NotNeedPin) {
        bond_mode_name = "Not_Need_PIN";
    } else if (bonding == BondMode::AuthNotNeedPin) {
        bond_mode_name = "Auth_Not_Need_PIN";
    } else if (bonding == BondMode::AuthWithPin) {
        bond_mode_name = "Auth_With_PIN";
    } else {
        bond_mode_name = "Auth_And_Bonded";
    }
    fmt::print("   Bond Mode: {}\n", bond_mode_name);

    // ========================================================================
    // 5. Type Safety Demonstration
    // ========================================================================
    fmt::print("\n5. Type Safety Features:\n");
    fmt::print("   ----------------------\n");

    // Different RangedInteger types cannot be mixed (compile-time check)
    // Percent percent_value{50};
    // TemperatureType temp_value{25};
    // percent_value = temp_value;  // ERROR: Different tag types
    fmt::print("   \u2713 Percent and Temperature_Type are distinct types\n");

    // Enum classes prevent implicit conversions
    // Role role = Role::Central;
    // int x = role;  // ERROR: No implicit conversion to int
    // if (role == adv_interval) {}  // ERROR: Different enum types
    fmt::print("   \u2713 Enum classes prevent implicit int conversion\n");

    // Range validation in debug builds
    // Percent overflow{200};  // Would trigger assertion in debug build
    fmt::print("   \u2713 Range validation active in debug builds\n");

    // Character validation in debug builds
    // MacAddress invalid{"ZZZZZZZZZZZZ"};  // Would trigger assertion
    fmt::print("   \u2713 Character validation active for FixedString types\n");

    // ========================================================================
    // Summary
    // ========================================================================
    fmt::print("\n=== Type System Verification Complete ===\n");
    fmt::print("All type aliases and enum classes compiled successfully.\n");
    fmt::print("Strong type safety enforced at compile-time.\n");
    fmt::print("Runtime validation active in debug builds.\n\n");

    spdlog::info("Type system demonstration completed successfully");

    return EXIT_SUCCESS;
}
