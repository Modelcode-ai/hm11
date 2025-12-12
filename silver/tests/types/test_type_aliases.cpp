#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>

#include "hm11/types.hpp"

using namespace hm11;

class TypeAliasesTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Range Type Tests (RangedInteger aliases)
// ============================================================================

TEST_F(TypeAliasesTest, PercentType) {
    Percent battery(85);
    EXPECT_EQ(battery.value(), 85);
    EXPECT_EQ(Percent::MIN_VALUE, 0);
    EXPECT_EQ(Percent::MAX_VALUE, 100);

    // Test range validation
    EXPECT_TRUE(Percent::is_valid(0));
    EXPECT_TRUE(Percent::is_valid(100));
    EXPECT_FALSE(Percent::is_valid(101));
}

TEST_F(TypeAliasesTest, TemperatureType) {
    TemperatureType temp(25);
    EXPECT_EQ(temp.value(), 25);
    EXPECT_EQ(TemperatureType::MIN_VALUE, 0);
    EXPECT_EQ(TemperatureType::MAX_VALUE, 120);

    // Test realistic temperature values
    TemperatureType room_temp(22);
    TemperatureType hot_temp(95);
    EXPECT_TRUE(room_temp < hot_temp);
    EXPECT_TRUE(TemperatureType::is_valid(hot_temp.value()));
}

TEST_F(TypeAliasesTest, HumidityType) {
    HumidityType humidity(60);
    EXPECT_EQ(humidity.value(), 60);
    EXPECT_EQ(HumidityType::MIN_VALUE, 0);
    EXPECT_EQ(HumidityType::MAX_VALUE, 100);

    // Test humidity operations
    HumidityType dry(30);
    HumidityType humid(80);
    EXPECT_TRUE(dry < humid);
}

TEST_F(TypeAliasesTest, LayerConnectionLatency) {
    LayerConnectionLatency latency(2);
    EXPECT_EQ(latency.value(), 2);
    EXPECT_EQ(LayerConnectionLatency::MIN_VALUE, 0);
    EXPECT_EQ(LayerConnectionLatency::MAX_VALUE, 4);
}

TEST_F(TypeAliasesTest, MacWhiteListIndex) {
    MacWhiteListIndex index(2);
    EXPECT_EQ(index.value(), 2);
    EXPECT_EQ(MacWhiteListIndex::MIN_VALUE, 1);
    EXPECT_EQ(MacWhiteListIndex::MAX_VALUE, 3);

    // Test all valid values
    EXPECT_TRUE(MacWhiteListIndex::is_valid(1));
    EXPECT_TRUE(MacWhiteListIndex::is_valid(2));
    EXPECT_TRUE(MacWhiteListIndex::is_valid(3));
    EXPECT_FALSE(MacWhiteListIndex::is_valid(0));
    EXPECT_FALSE(MacWhiteListIndex::is_valid(4));
}

TEST_F(TypeAliasesTest, PioNumber) {
    PioNumber pio(1);
    EXPECT_EQ(pio.value(), 1);
    EXPECT_EQ(PioNumber::MIN_VALUE, 1);
    EXPECT_EQ(PioNumber::MAX_VALUE, 3);

    // HM-11 only has PIO0, PIO1, PIO2, PIO3 but PIO0/PIO1 are system reserved
    EXPECT_TRUE(PioNumber::is_valid(1)); // PIO2
    EXPECT_TRUE(PioNumber::is_valid(2)); // PIO3
    EXPECT_TRUE(PioNumber::is_valid(3)); // Additional if available
}

TEST_F(TypeAliasesTest, DiscoveredIndex) {
    DiscoveredIndex index(0);
    EXPECT_EQ(index.value(), 0);
    EXPECT_EQ(DiscoveredIndex::MIN_VALUE, 0);
    EXPECT_EQ(DiscoveredIndex::MAX_VALUE, 5);

    // Test that all discovered device indices are valid
    for (uint8_t i = 0; i <= 5; ++i) {
        EXPECT_TRUE(DiscoveredIndex::is_valid(i));
    }
    EXPECT_FALSE(DiscoveredIndex::is_valid(6));
}

TEST_F(TypeAliasesTest, DiscoveryTime) {
    DiscoveryTime time(3);
    EXPECT_EQ(time.value(), 3);
    EXPECT_EQ(DiscoveryTime::MIN_VALUE, 1);
    EXPECT_EQ(DiscoveryTime::MAX_VALUE, 9);

    // Default discovery time is 3 seconds according to Ada
    DiscoveryTime default_time(3);
    EXPECT_EQ(default_time.value(), 3);
}

// ============================================================================
// String Type Tests (FixedString aliases)
// ============================================================================

TEST_F(TypeAliasesTest, MacAddress) {
    MacAddress mac("B4994C123456");
    EXPECT_EQ(mac.size(), 12);
    EXPECT_EQ(mac.capacity(), 12);
    EXPECT_EQ(std::string_view(mac), "B4994C123456");

    // Test that it accepts both upper and lower case hex
    MacAddress mac_lower("ab12cd34ef56");
    EXPECT_EQ(std::string_view(mac_lower), "ab12cd34ef56");

    // Test validation - should reject non-hex characters
    EXPECT_THROW(MacAddress("B4994CGHIJKL"), std::invalid_argument);
}

TEST_F(TypeAliasesTest, UUID) {
    UUID service_uuid("FE01");
    EXPECT_EQ(service_uuid.size(), 4);
    EXPECT_EQ(service_uuid.capacity(), 4);
    EXPECT_EQ(std::string_view(service_uuid), "FE01");

    // Test 16-bit UUID format
    UUID another_uuid("A0B1");
    EXPECT_EQ(std::string_view(another_uuid), "A0B1");
}

TEST_F(TypeAliasesTest, iBeaconUUID) {
    IBeaconUuid beacon_uuid("74278BDAB64445208F0C720EAF059935");
    EXPECT_EQ(beacon_uuid.size(), 32);
    EXPECT_EQ(beacon_uuid.capacity(), 32);

    // This is the default iBeacon UUID from Ada comments
    EXPECT_EQ(std::string_view(beacon_uuid), "74278BDAB64445208F0C720EAF059935");
}

TEST_F(TypeAliasesTest, VersionType) {
    VersionType major_version("FFE0");
    EXPECT_EQ(major_version.size(), 4);
    EXPECT_EQ(std::string_view(major_version), "FFE0");

    // Default major version from Ada
    VersionType minor_version("FFE1");
    EXPECT_EQ(std::string_view(minor_version), "FFE1");
}

TEST_F(TypeAliasesTest, MeasuredPower) {
    MeasuredPower power("C5");
    EXPECT_EQ(power.size(), 2);
    EXPECT_EQ(power.capacity(), 2);
    EXPECT_EQ(std::string_view(power), "C5");

    // Test validation - Ada spec requires exactly 2 hex characters
    EXPECT_THROW(MeasuredPower("C"), std::length_error);   // Too short (must be exactly 2 chars)
    EXPECT_THROW(MeasuredPower("C5A"), std::length_error); // Too long (must be exactly 2 chars)
}

TEST_F(TypeAliasesTest, HandleType) {
    HandleType handle("1234");
    EXPECT_EQ(handle.size(), 4);
    EXPECT_EQ(std::string_view(handle), "1234");

    // Test characteristic handle format
    HandleType char_handle("FFE1");
    EXPECT_EQ(std::string_view(char_handle), "FFE1");
}

TEST_F(TypeAliasesTest, PinType) {
    PinType pin("000000");
    EXPECT_EQ(pin.size(), 6);
    EXPECT_EQ(pin.capacity(), 6);
    EXPECT_EQ(std::string_view(pin), "000000");

    // Test validation - should accept digits, reject invalid characters
    EXPECT_THROW(PinType("12A456"), std::invalid_argument); // Invalid character
    EXPECT_NO_THROW(PinType("12345"));                      // 5 characters is valid (less than capacity)
}

TEST_F(TypeAliasesTest, WorkInterval) {
    WorkInterval interval("01");
    EXPECT_EQ(interval.size(), 2);
    EXPECT_EQ(std::string_view(interval), "01");

    // Test minutes format (00-99)
    WorkInterval zero_minutes("00");
    WorkInterval max_minutes("99");
    EXPECT_EQ(std::string_view(zero_minutes), "00");
    EXPECT_EQ(std::string_view(max_minutes), "99");
}

TEST_F(TypeAliasesTest, ConnectTimeout) {
    ConnectTimeout timeout("000000");
    EXPECT_EQ(timeout.size(), 6);
    EXPECT_EQ(std::string_view(timeout), "000000");

    // Default is 000000 ms according to Ada
    ConnectTimeout custom_timeout("010000");
    EXPECT_EQ(std::string_view(custom_timeout), "010000");
}

TEST_F(TypeAliasesTest, InternalTemperatureType) {
    InternalTemperatureType temp("25.0°C");
    EXPECT_EQ(temp.size(), 7); // UTF-8 degree symbol takes 2 bytes in C++
    EXPECT_EQ(temp.capacity(), 7);

    // Test that it accepts any characters (not just digits/hex)
    // This string would be 8 bytes, so use a shorter example
    InternalTemperatureType temp2("15.5°C");
    EXPECT_EQ(temp2.size(), 7);
}

// ============================================================================
// Enum Type Tests
// ============================================================================

TEST_F(TypeAliasesTest, RoleEnum) {
    Role peripheral = Role::Peripheral;
    Role central = Role::Central;

    EXPECT_EQ(static_cast<uint8_t>(peripheral), 0);
    EXPECT_EQ(static_cast<uint8_t>(central), 1);
    EXPECT_NE(peripheral, central);
}

TEST_F(TypeAliasesTest, AdvertisingIntervalEnum) {
    // Test that all values are available
    AdvertisingInterval interval_100ms = AdvertisingInterval::Ms100;
    AdvertisingInterval interval_1285ms = AdvertisingInterval::Ms1285;
    AdvertisingInterval interval_7000ms = AdvertisingInterval::Ms7000;

    EXPECT_EQ(static_cast<uint8_t>(interval_100ms), 0);
    EXPECT_EQ(static_cast<uint8_t>(interval_1285ms), 9);
    EXPECT_EQ(static_cast<uint8_t>(interval_7000ms), 15);
}

TEST_F(TypeAliasesTest, UartBaudRateEnum) {
    UartBaudRate default_baud = UartBaudRate::Br9600;
    UartBaudRate high_baud = UartBaudRate::Br115200;

    EXPECT_EQ(static_cast<uint8_t>(default_baud), 0);
    EXPECT_EQ(static_cast<uint8_t>(high_baud), 4);
}

TEST_F(TypeAliasesTest, ConnectResultEnum) {
    ConnectResult connecting = ConnectResult::Connecting;
    ConnectResult connected = ConnectResult::Connected;
    ConnectResult error = ConnectResult::ConnectError;

    EXPECT_EQ(static_cast<uint8_t>(connecting), 0);
    EXPECT_EQ(static_cast<uint8_t>(connected), 1);
    EXPECT_EQ(static_cast<uint8_t>(error), 2);
}

TEST_F(TypeAliasesTest, SensorTypeEnum) {
    SensorType none = SensorType::None;
    SensorType dht11 = SensorType::DHT11;
    SensorType ds18b20 = SensorType::DS18B20;

    EXPECT_EQ(static_cast<uint8_t>(none), 0);
    EXPECT_EQ(static_cast<uint8_t>(dht11), 1);
    EXPECT_EQ(static_cast<uint8_t>(ds18b20), 2);
}

// ============================================================================
// Type Safety Tests - Ensuring Different Types Can't Be Mixed
// ============================================================================

TEST_F(TypeAliasesTest, TypeSafetyRangedIntegers) {
    Percent battery(50);
    TemperatureType temp(50);
    HumidityType humidity(50);

    // All have the same underlying value but different types
    EXPECT_EQ(battery.value(), 50);
    EXPECT_EQ(temp.value(), 50);
    EXPECT_EQ(humidity.value(), 50);

    // But they are different types and cannot be compared directly
    static_assert(!std::is_same_v<Percent, TemperatureType>);
    static_assert(!std::is_same_v<TemperatureType, HumidityType>);
    static_assert(!std::is_same_v<Percent, HumidityType>);

    // This should NOT compile (uncomment to test):
    // bool mixed = (battery == temp);  // Compilation error expected
    // bool mixed2 = (temp == humidity);  // Compilation error expected
}

TEST_F(TypeAliasesTest, TypeSafetyFixedStrings) {
    MacAddress mac("123456789012");
    PinType pin("123456");
    UUID uuid("1234");

    // Different types with different validators and sizes
    static_assert(!std::is_same_v<MacAddress, PinType>);
    static_assert(!std::is_same_v<PinType, UUID>);
    static_assert(!std::is_same_v<MacAddress, UUID>);

    // Test that validators are correctly applied
    EXPECT_TRUE(mac.is_valid());  // Hex validator
    EXPECT_TRUE(pin.is_valid());  // Digit validator
    EXPECT_TRUE(uuid.is_valid()); // Hex validator

    // These should NOT compile (uncomment to test):
    // bool str_eq = (mac == pin);  // Different template parameters
}

// ============================================================================
// Constants Tests
// ============================================================================

TEST_F(TypeAliasesTest, Constants) {
    EXPECT_EQ(MAX_NAME_LENGTH, 248);
    EXPECT_EQ(MAX_RSSI_LENGTH, 4);
    EXPECT_GT(MAX_MESSAGE_LENGTH, 250); // Should be large enough for discovery messages

    EXPECT_EQ(WHEN_CONNECTED_MESSAGE, "OK+CONN");
    EXPECT_EQ(WHEN_DISCONNECTED_MESSAGE, "OK+LOST");
}

// ============================================================================
// Callback Type Tests
// ============================================================================

TEST_F(TypeAliasesTest, CallbackTypes) {
    // Test that callback types are correctly defined
    static_assert(std::is_same_v<
                  DiscoveredCallback,
                  void (*)(char, const MacAddress&, std::string_view, std::string_view, void*)>);

    static_assert(std::is_same_v<DiscoveredIBeaconCallback, void (*)(std::string_view, void*)>);

    static_assert(std::is_same_v<ServiceUuidCallback, void (*)(std::string_view, void*)>);

    static_assert(std::is_same_v<CharacteristicUuidCallback, void (*)(std::string_view, void*)>);
}

// ============================================================================
// Real-world Usage Scenarios
// ============================================================================

TEST_F(TypeAliasesTest, RealisticScenario) {
    // Simulate a real HM11 device configuration
    MacAddress device_mac("B4994C112233");
    PinType device_pin("123456");
    Percent battery_level(85);
    TemperatureType sensor_temp(25);
    HumidityType sensor_humidity(60);

    // Verify all values are reasonable
    EXPECT_EQ(std::string_view(device_mac), "B4994C112233");
    EXPECT_EQ(std::string_view(device_pin), "123456");
    EXPECT_GE(battery_level.value(), 0);
    EXPECT_LE(battery_level.value(), 100);
    EXPECT_GE(sensor_temp.value(), 0);
    EXPECT_LE(sensor_temp.value(), 120);
    EXPECT_GE(sensor_humidity.value(), 0);
    EXPECT_LE(sensor_humidity.value(), 100);
}

TEST_F(TypeAliasesTest, iBeaconConfiguration) {
    // Configure iBeacon settings
    IBeaconUuid beacon_uuid("74278BDAB64445208F0C720EAF059935");
    VersionType major_version("FFE0");
    VersionType minor_version("FFE1");
    MeasuredPower measured_power("C5");

    EXPECT_EQ(beacon_uuid.size(), 32);
    EXPECT_EQ(major_version.size(), 4);
    EXPECT_EQ(minor_version.size(), 4);
    EXPECT_EQ(measured_power.size(), 2);

    // Verify default values match Ada specification
    EXPECT_EQ(std::string_view(beacon_uuid), "74278BDAB64445208F0C720EAF059935");
    EXPECT_EQ(std::string_view(major_version), "FFE0");
    EXPECT_EQ(std::string_view(minor_version), "FFE1");
    EXPECT_EQ(std::string_view(measured_power), "C5");
}

TEST_F(TypeAliasesTest, ConnectionParameters) {
    // Test connection-related parameters
    MacWhiteListIndex white_list_slot(1);
    DiscoveredIndex discovered_device(3);
    DiscoveryTime discovery_duration(5);
    ConnectTimeout connection_timeout("010000");

    EXPECT_GE(white_list_slot.value(), 1);
    EXPECT_LE(white_list_slot.value(), 3);
    EXPECT_GE(discovered_device.value(), 0);
    EXPECT_LE(discovered_device.value(), 5);
    EXPECT_GE(discovery_duration.value(), 1);
    EXPECT_LE(discovery_duration.value(), 9);
    EXPECT_EQ(std::string_view(connection_timeout), "010000");
}

// ============================================================================
// Memory Efficiency Tests
// ============================================================================

TEST_F(TypeAliasesTest, MemoryEfficiency) {
    // Ensure enum types are efficiently packed
    EXPECT_EQ(sizeof(Role), 1);
    EXPECT_EQ(sizeof(AdvertisingInterval), 1);
    EXPECT_EQ(sizeof(UartBaudRate), 1);
    EXPECT_EQ(sizeof(SensorType), 1);
    EXPECT_EQ(sizeof(ModulePower), 1);

    // Ensure ranged integers don't add overhead
    EXPECT_EQ(sizeof(Percent), sizeof(uint8_t));
    EXPECT_EQ(sizeof(TemperatureType), sizeof(uint8_t));
    EXPECT_EQ(sizeof(MacWhiteListIndex), sizeof(uint8_t));

    // Fixed strings should be reasonably sized
    EXPECT_LE(sizeof(MacAddress), 32); // 12 chars + size_t + alignment
    EXPECT_LE(sizeof(PinType), 24);    // 6 chars + size_t + alignment
    EXPECT_LE(sizeof(UUID), 24);       // 4 chars + size_t + alignment
}

// Tests will be run by gtest_main