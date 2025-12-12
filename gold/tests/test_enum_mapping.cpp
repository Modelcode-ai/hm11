/**
 * @file test_enum_mapping.cpp
 * @brief Unit tests for enum-to-string mapping utilities
 *
 * Tests verify:
 * - All enum values map to correct strings
 * - Constexpr evaluation works at compile time
 * - Mappings are exhaustive (all enum values covered)
 * - String_view results are valid and accessible
 */

#include <gtest/gtest.h>

#include "hm11/util/enum_mapping.hpp"

using namespace hm11::util;
using namespace hm11::types;

// =============================================================================
// Role Enum Tests
// =============================================================================

TEST(EnumMappingTest, RolePeripheralMapsToZero) {
    auto result = enum_to_string(Role::Peripheral);
    EXPECT_EQ(result, "0");
}

TEST(EnumMappingTest, RoleCentralMapsToOne) {
    auto result = enum_to_string(Role::Central);
    EXPECT_EQ(result, "1");
}

TEST(EnumMappingTest, RoleMappingIsConstexpr) {
    // This test verifies compile-time evaluation by using the result
    // in a constexpr context
    constexpr auto PERIPHERAL_STR = enum_to_string(Role::Peripheral);
    constexpr auto CENTRAL_STR = enum_to_string(Role::Central);

    EXPECT_EQ(PERIPHERAL_STR, "0");
    EXPECT_EQ(CENTRAL_STR, "1");
}

TEST(EnumMappingTest, RoleMappingReturnsValidStringView) {
    auto result = enum_to_string(Role::Peripheral);

    // Verify string_view properties
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size(), 1);
    EXPECT_NE(result.data(), nullptr);
}

// =============================================================================
// AdvertisingInterval Enum Tests
// =============================================================================

TEST(EnumMappingTest, AdvertisingIntervalMs100MapsTo0) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms100), "0");
}

TEST(EnumMappingTest, AdvertisingIntervalMs211MapsTo1) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms211), "1");
}

TEST(EnumMappingTest, AdvertisingIntervalMs252MapsTo2) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms252), "2");
}

TEST(EnumMappingTest, AdvertisingIntervalMs318MapsTo3) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms318), "3");
}

TEST(EnumMappingTest, AdvertisingIntervalMs417MapsTo4) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms417), "4");
}

TEST(EnumMappingTest, AdvertisingIntervalMs546MapsTo5) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms546), "5");
}

TEST(EnumMappingTest, AdvertisingIntervalMs760MapsTo6) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms760), "6");
}

TEST(EnumMappingTest, AdvertisingIntervalMs852MapsTo7) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms852), "7");
}

TEST(EnumMappingTest, AdvertisingIntervalMs1022MapsTo8) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms1022), "8");
}

TEST(EnumMappingTest, AdvertisingIntervalMs1285MapsTo9) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms1285), "9");
}

TEST(EnumMappingTest, AdvertisingIntervalMs2000MapsToA) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms2000), "A");
}

TEST(EnumMappingTest, AdvertisingIntervalMs3000MapsToB) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms3000), "B");
}

TEST(EnumMappingTest, AdvertisingIntervalMs4000MapsToC) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms4000), "C");
}

TEST(EnumMappingTest, AdvertisingIntervalMs5000MapsToD) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms5000), "D");
}

TEST(EnumMappingTest, AdvertisingIntervalMs6000MapsToE) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms6000), "E");
}

TEST(EnumMappingTest, AdvertisingIntervalMs7000MapsToF) {
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms7000), "F");
}

TEST(EnumMappingTest, AdvertisingIntervalMappingIsConstexpr) {
    // Verify compile-time evaluation for multiple values
    constexpr auto VAL0 = enum_to_string(AdvertisingInterval::Ms100);
    constexpr auto VAL9 = enum_to_string(AdvertisingInterval::Ms1285);
    constexpr auto VALF = enum_to_string(AdvertisingInterval::Ms7000);

    EXPECT_EQ(VAL0, "0");
    EXPECT_EQ(VAL9, "9");
    EXPECT_EQ(VALF, "F");
}

// =============================================================================
// UartBaudRate Enum Tests
// =============================================================================

TEST(EnumMappingTest, UartBaudRateBr9600MapsTo0) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br9600), "0");
}

TEST(EnumMappingTest, UartBaudRateBr19200MapsTo1) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br19200), "1");
}

TEST(EnumMappingTest, UartBaudRateBr38400MapsTo2) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br38400), "2");
}

TEST(EnumMappingTest, UartBaudRateBr57600MapsTo3) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br57600), "3");
}

TEST(EnumMappingTest, UartBaudRateBr115200MapsTo4) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br115200), "4");
}

TEST(EnumMappingTest, UartBaudRateBr4800MapsTo5) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br4800), "5");
}

TEST(EnumMappingTest, UartBaudRateBr2400MapsTo6) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br2400), "6");
}

TEST(EnumMappingTest, UartBaudRateBr1200MapsTo7) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br1200), "7");
}

TEST(EnumMappingTest, UartBaudRateBr230400MapsTo8) {
    EXPECT_EQ(enum_to_string(UartBaudRate::Br230400), "8");
}

TEST(EnumMappingTest, UartBaudRateMappingIsConstexpr) {
    constexpr auto DEFAULT_BAUD = enum_to_string(UartBaudRate::Br9600);
    constexpr auto MAX_BAUD = enum_to_string(UartBaudRate::Br230400);

    EXPECT_EQ(DEFAULT_BAUD, "0");
    EXPECT_EQ(MAX_BAUD, "8");
}

// =============================================================================
// BondMode Enum Tests
// =============================================================================

TEST(EnumMappingTest, BondModeNotNeedPinMapsTo0) {
    EXPECT_EQ(enum_to_string(BondMode::NotNeedPin), "0");
}

TEST(EnumMappingTest, BondModeAuthNotNeedPinMapsTo1) {
    EXPECT_EQ(enum_to_string(BondMode::AuthNotNeedPin), "1");
}

TEST(EnumMappingTest, BondModeAuthWithPinMapsTo2) {
    EXPECT_EQ(enum_to_string(BondMode::AuthWithPin), "2");
}

TEST(EnumMappingTest, BondModeAuthAndBondedMapsTo3) {
    EXPECT_EQ(enum_to_string(BondMode::AuthAndBonded), "3");
}

TEST(EnumMappingTest, BondModeMappingIsConstexpr) {
    constexpr auto NO_PIN = enum_to_string(BondMode::NotNeedPin);
    constexpr auto BONDED = enum_to_string(BondMode::AuthAndBonded);

    EXPECT_EQ(NO_PIN, "0");
    EXPECT_EQ(BONDED, "3");
}

// =============================================================================
// SensorType Enum Tests
// =============================================================================

TEST(EnumMappingTest, SensorTypeNoneMapsTo0) {
    EXPECT_EQ(enum_to_string(SensorType::None), "0");
}

TEST(EnumMappingTest, SensorTypeDHT11MapsTo1) {
    EXPECT_EQ(enum_to_string(SensorType::DHT11), "1");
}

TEST(EnumMappingTest, SensorTypeDS18B20MapsTo2) {
    EXPECT_EQ(enum_to_string(SensorType::DS18B20), "2");
}

TEST(EnumMappingTest, SensorTypeMappingIsConstexpr) {
    constexpr auto NO_SENSOR = enum_to_string(SensorType::None);
    constexpr auto TEMP_SENSOR = enum_to_string(SensorType::DS18B20);

    EXPECT_EQ(NO_SENSOR, "0");
    EXPECT_EQ(TEMP_SENSOR, "2");
}

// =============================================================================
// SendDataMethod Enum Tests
// =============================================================================

TEST(EnumMappingTest, SendDataMethodWriteMapsToWR) {
    EXPECT_EQ(enum_to_string(SendDataMethod::Write), "WR");
}

TEST(EnumMappingTest, SendDataMethodWriteWithoutResponseMapsToWN) {
    EXPECT_EQ(enum_to_string(SendDataMethod::WriteWithoutResponse), "WN");
}

TEST(EnumMappingTest, SendDataMethodIndicateMapsToIN) {
    EXPECT_EQ(enum_to_string(SendDataMethod::Indicate), "IN");
}

TEST(EnumMappingTest, SendDataMethodNotifyMapsToNO) {
    EXPECT_EQ(enum_to_string(SendDataMethod::Notify), "NO");
}

TEST(EnumMappingTest, SendDataMethodMappingIsConstexpr) {
    constexpr auto WRITE = enum_to_string(SendDataMethod::Write);
    constexpr auto NOTIFY = enum_to_string(SendDataMethod::Notify);

    EXPECT_EQ(WRITE, "WR");
    EXPECT_EQ(NOTIFY, "NO");
}

// =============================================================================
// MacAddressType Enum Tests
// =============================================================================

TEST(EnumMappingTest, MacAddressTypeNormalMapsToN) {
    EXPECT_EQ(enum_to_string(MacAddressType::NormalAddress), "N");
}

TEST(EnumMappingTest, MacAddressTypeStaticMapsTo0) {
    EXPECT_EQ(enum_to_string(MacAddressType::StaticMac), "0");
}

TEST(EnumMappingTest, MacAddressTypeStaticRandomMapsTo1) {
    EXPECT_EQ(enum_to_string(MacAddressType::StaticRandomMac), "1");
}

TEST(EnumMappingTest, MacAddressTypeRandomMapsTo2) {
    EXPECT_EQ(enum_to_string(MacAddressType::RandomMac), "2");
}

TEST(EnumMappingTest, MacAddressTypeMappingIsConstexpr) {
    constexpr auto NORMAL = enum_to_string(MacAddressType::NormalAddress);
    constexpr auto RANDOM = enum_to_string(MacAddressType::RandomMac);

    EXPECT_EQ(NORMAL, "N");
    EXPECT_EQ(RANDOM, "2");
}

// =============================================================================
// Boolean Mapping Tests
// =============================================================================

TEST(EnumMappingTest, BoolTrueMapsTo1) {
    EXPECT_EQ(bool_to_string(true), "1");
}

TEST(EnumMappingTest, BoolFalseMapsTo0) {
    EXPECT_EQ(bool_to_string(false), "0");
}

TEST(EnumMappingTest, BoolMappingIsConstexpr) {
    constexpr auto TRUE_STR = bool_to_string(true);
    constexpr auto FALSE_STR = bool_to_string(false);

    EXPECT_EQ(TRUE_STR, "1");
    EXPECT_EQ(FALSE_STR, "0");
}

// =============================================================================
// Integration Tests - Simulated AT Command Construction
// =============================================================================

TEST(EnumMappingTest, ATCommandConstructionRole) {
    // Simulate building AT+ROLE command
    std::string cmd = "AT+ROLE";
    cmd += enum_to_string(Role::Central);

    EXPECT_EQ(cmd, "AT+ROLE1");
}

TEST(EnumMappingTest, ATCommandConstructionAdvertisingInterval) {
    // Simulate building AT+ADVI command
    std::string cmd = "AT+ADVI";
    cmd += enum_to_string(AdvertisingInterval::Ms1285);

    EXPECT_EQ(cmd, "AT+ADVI9");
}

TEST(EnumMappingTest, ATCommandConstructionBaudRate) {
    // Simulate building AT+BAUD command
    std::string cmd = "AT+BAUD";
    cmd += enum_to_string(UartBaudRate::Br115200);

    EXPECT_EQ(cmd, "AT+BAUD4");
}

TEST(EnumMappingTest, ATCommandConstructionBondMode) {
    // Simulate building AT+TYPE command
    std::string cmd = "AT+TYPE";
    cmd += enum_to_string(BondMode::AuthWithPin);

    EXPECT_EQ(cmd, "AT+TYPE2");
}

TEST(EnumMappingTest, ATCommandConstructionSensorType) {
    // Simulate building AT+SENS command
    std::string cmd = "AT+SENS";
    cmd += enum_to_string(SensorType::DHT11);

    EXPECT_EQ(cmd, "AT+SENS1");
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(EnumMappingTest, StringViewsAreNonOwning) {
    // Verify that string_view doesn't copy the underlying string
    auto sv1 = enum_to_string(Role::Peripheral);
    auto sv2 = enum_to_string(Role::Peripheral);

    // Both should point to the same static storage
    EXPECT_EQ(sv1.data(), sv2.data());
}

TEST(EnumMappingTest, StringViewsHaveExpectedLength) {
    // Single-character mappings
    EXPECT_EQ(enum_to_string(Role::Peripheral).size(), 1);
    EXPECT_EQ(enum_to_string(AdvertisingInterval::Ms100).size(), 1);
    EXPECT_EQ(enum_to_string(BondMode::NotNeedPin).size(), 1);

    // Two-character mappings
    EXPECT_EQ(enum_to_string(SendDataMethod::Write).size(), 2);
    EXPECT_EQ(enum_to_string(SendDataMethod::Notify).size(), 2);
}

TEST(EnumMappingTest, ConstexprContextCompileTimeEvaluation) {
    // This test ensures the entire mapping is evaluated at compile time
    // by using the result in a compile-time constant expression

    constexpr std::string_view COMPILE_TIME_ROLE = enum_to_string(Role::Peripheral);
    constexpr std::string_view COMPILE_TIME_INTERVAL = enum_to_string(AdvertisingInterval::Ms1285);
    constexpr std::string_view COMPILE_TIME_BAUD = enum_to_string(UartBaudRate::Br9600);

    // If these weren't constexpr, the code wouldn't compile
    static_assert(COMPILE_TIME_ROLE == "0", "Must be evaluable at compile time");
    static_assert(COMPILE_TIME_INTERVAL == "9", "Must be evaluable at compile time");
    static_assert(COMPILE_TIME_BAUD == "0", "Must be evaluable at compile time");
}

// =============================================================================
// Documentation Examples Verification
// =============================================================================

TEST(EnumMappingTest, DocumentationExampleCompiles) {
    // This verifies the code example from the header documentation works
    using namespace hm11::util;
    using namespace hm11::types;

    // Compile-time evaluation (from documentation)
    constexpr auto ROLE_STR = enum_to_string(Role::Peripheral);
    static_assert(ROLE_STR == "0", "Documentation example must work");

    // Runtime usage in AT command construction (from documentation)
    Role role = Role::Central; // Simulate getting device role
    std::string cmd = "AT+ROLE";
    cmd += enum_to_string(role);

    EXPECT_EQ(cmd, "AT+ROLE1");
}
