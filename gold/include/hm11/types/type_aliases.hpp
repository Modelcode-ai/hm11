/**
 * @file type_aliases.hpp
 * @brief Type aliases for all HM-11 driver domain types
 *
 * This header provides type-safe aliases for all Ada strong types using the
 * RangedInteger and FixedString templates. This is part of Decision 2 (Strong
 * Type System Translation) from the migration specification.
 *
 * ## Overview
 *
 * Ada's extensive type system (30+ strong types) is compressed into ~100 lines
 * of C++ type aliases that instantiate the RangedInteger and FixedString
 * templates with specific constraints.
 *
 * ## Type Categories
 *
 * - **RangedInteger types**: Numeric types with compile-time range constraints
 * - **FixedString hex types**: Fixed-size arrays of hexadecimal characters
 * - **FixedString digit types**: Fixed-size arrays of decimal digit characters
 * - **Enum types**: Defined separately in enums.hpp (C++11 enum class)
 *
 * ## Ada Correspondence
 *
 * Each type alias directly corresponds to a type definition in the Ada source
 * file hm11.ads (lines 76-273). The correspondence is documented in each
 * type's documentation comment.
 *
 * ## Usage Example
 *
 * @code
 * using namespace hm11::types;
 *
 * // RangedInteger usage
 * Percent battery{75};        // OK
 * Temperature_Type temp{25};  // OK
 * // Percent invalid{150};    // Debug assertion failure
 *
 * // FixedString usage (hex)
 * MAC_Address mac{"B499CF123456"};        // OK
 * UUID service_uuid{"180F"};              // OK (4 hex chars)
 * // MAC_Address bad{"ZZZZ"};             // Debug assertion failure
 *
 * // FixedString usage (digits)
 * PIN_Type pin{"123456"};                 // OK (6 digits)
 * Work_Interval interval{"30"};           // OK (2 digits)
 * // PIN_Type invalid{"ABC123"};          // Debug assertion failure
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <cstdint>
#include <hm11/types/fixed_string.hpp>
#include <hm11/types/ranged_integer.hpp>
#include <hm11/types/validators.hpp>

namespace hm11::types {

// ============================================================================
// Tag Types for RangedInteger Strong Typing
// ============================================================================
// These empty structs provide unique types for compile-time type safety.
// They prevent mixing different ranged integer types (e.g., Percent vs
// Temperature) even though they may have overlapping ranges.

/// @brief Tag for Percent type (battery percentage, humidity)
struct PercentTag {};

/// @brief Tag for Temperature_Type (sensor temperature in Celsius)
struct TemperatureTag {};

/// @brief Tag for Humidity_Type (sensor humidity percentage)
struct HumidityTag {};

/// @brief Tag for Layer_Connection_Latency (connection latency parameter)
struct LayerConnectionLatencyTag {};

/// @brief Tag for PIO_Collection_Rate (sensor collection rate in seconds)
struct PIOCollectionRateTag {};

/// @brief Tag for MAC_White_List_Index (white list index 1-3)
struct MACWhiteListIndexTag {};

/// @brief Tag for Discovery_Time (discovery time in seconds 1-9)
struct DiscoveryTimeTag {};

/// @brief Tag for Discovered_Index (index in discovered devices list 0-5)
struct DiscoveredIndexTag {};

/// @brief Tag for PIO_Number (PIO pin number 1-3, HM-11 specific)
struct PIONumberTag {};

// ============================================================================
// RangedInteger Type Aliases
// ============================================================================
// These types provide compile-time and runtime range validation for numeric
// values, directly matching Ada's subtype declarations with range constraints.

/**
 * @brief Percentage type (0-100)
 *
 * Used for battery percentage, humidity readings, and other percentage values.
 * Ada source: hm11.ads:111 `type Persent is range 0 .. 100;`
 *
 * Note: Ada spelling preserved as "Persent" for consistency, though we use
 * "Percent" in C++ following correct English spelling.
 */
using Percent = RangedInteger<uint8_t, 0, 100, PercentTag>;

/**
 * @brief Temperature reading type (0-120 Celsius)
 *
 * Used for DHT11 and DS18B20 sensor temperature readings.
 * Ada source: hm11.ads:256 `subtype Temperature_Type is Natural range 0 .. 120;`
 */
using TemperatureType = RangedInteger<uint8_t, 0, 120, TemperatureTag>;

/**
 * @brief Humidity reading type (0-100 percent)
 *
 * Used for DHT11 sensor humidity readings.
 * Ada source: hm11.ads:257 `subtype Humidity_Type is Natural range 0 .. 100;`
 */
using HumidityType = RangedInteger<uint8_t, 0, 100, HumidityTag>;

/**
 * @brief BLE connection latency parameter (0-4)
 *
 * Link layer connection latency parameter for power optimization.
 * Ada source: hm11.ads:120 `type Layer_Connection_Latency is range 0 .. 4;`
 */
using LayerConnectionLatency = RangedInteger<uint8_t, 0, 4, LayerConnectionLatencyTag>;

/**
 * @brief PIO sensor collection rate (0-99 seconds)
 *
 * Sensor data collection rate in seconds for PIO collection mode.
 * Ada source: hm11.ads:272 `type PIO_Collection_Rate is range 0 .. 99;`
 */
using PioCollectionRate = RangedInteger<uint8_t, 0, 99, PIOCollectionRateTag>;

/**
 * @brief MAC address white list index (1-3)
 *
 * Index for MAC address white list entries (HM-11 supports up to 3 entries).
 * Ada source: hm11.ads:109 `type MAC_White_List_Index is range 1 .. 3;`
 */
using MacWhiteListIndex = RangedInteger<uint8_t, 1, 3, MACWhiteListIndexTag>;

/**
 * @brief Discovery time in seconds (1-9)
 *
 * Time duration for device discovery scanning operations.
 * Ada source: hm11.ads:249 `subtype Discovery_Time is Positive range 1 .. 9;`
 */
using DiscoveryTime = RangedInteger<uint8_t, 1, 9, DiscoveryTimeTag>;

/**
 * @brief Index in discovered devices list (0-5)
 *
 * Index for selecting a discovered device from the discovery results.
 * Ada source: hm11.ads:129 `type Discovered_Index is range 0 .. 5;`
 */
using DiscoveredIndex = RangedInteger<uint8_t, 0, 5, DiscoveredIndexTag>;

/**
 * @brief PIO pin number (1-3)
 *
 * PIO (Programmable Input/Output) pin number for HM-11 module.
 * Note: HM-11 has PIO0-PIO3, but PIO0 is system KEY and PIO1 is system LED.
 * Ada source: hm11.ads:232 `subtype PIO_Number is Positive range 1 .. 3;`
 */
using PioNumber = RangedInteger<uint8_t, 1, 3, PIONumberTag>;

// ============================================================================
// FixedString Hex Type Aliases
// ============================================================================
// These types represent fixed-size hexadecimal character arrays with compile-
// time size validation and runtime character validation. All characters must
// be in the range '0'-'9' or 'A'-'F' (uppercase).

/**
 * @brief Bluetooth MAC address (12 hex characters)
 *
 * 48-bit Bluetooth MAC address in hexadecimal format without delimiters.
 * Example: "B4994C123456"
 * Format: 0xB4:0x99:0x4C:0xXX:0xXX:0xXX is BLE MAC Address
 * Ada source: hm11.ads:101 `type MAC_Address is array (1 .. 12) of Hex_Character;`
 */
using MacAddress = FixedString<12, HexCharValidator>;

/**
 * @brief 16-bit UUID (4 hex characters)
 *
 * 16-bit Bluetooth UUID in hexadecimal format. The HM-11 module only
 * supports 16-bit UUIDs, not full 128-bit UUIDs.
 * Example: "180F" (Battery Service UUID)
 * Ada source: hm11.ads:104 `type UUID is array (1 .. 4) of Hex_Character;`
 */
using UUID = FixedString<4, HexCharValidator>;

/**
 * @brief GATT handle (4 hex characters)
 *
 * 16-bit handle for GATT services, characteristics, and descriptors.
 * Example: "0001", "002A"
 * Ada source: hm11.ads:189 `type Handle_Type is array (1 .. 4) of Hex_Character;`
 */
using HandleType = FixedString<4, HexCharValidator>;

/**
 * @brief iBeacon UUID (32 hex characters)
 *
 * 128-bit iBeacon proximity UUID in hexadecimal format without delimiters.
 * Example: "E2C56DB5DFFB48D2B060D0F5A71096E0"
 * Ada source: hm11.ads:217 `type iBeacon_UUID is array (1 .. 32) of Hex_Character;`
 */
using IBeaconUuid = FixedString<32, HexCharValidator>;

/**
 * @brief iBeacon major/minor version (4 hex characters)
 *
 * 16-bit major or minor version number for iBeacon proximity detection.
 * Example: "0001", "FFFF"
 * Ada source: hm11.ads:219 `type Version_Type is array (1 .. 4) of Hex_Character;`
 */
using VersionType = FixedString<4, HexCharValidator>;

/**
 * @brief iBeacon measured power (2 hex characters)
 *
 * 8-bit measured power value for iBeacon distance estimation.
 * Example: "C5" (representing -59 dBm in two's complement)
 * Ada source: hm11.ads:221 `type Measured_Power is array (1 .. 2) of Hex_Character;`
 */
using MeasuredPower = FixedString<2, HexCharValidator>;

/**
 * @brief PIO pin states bitmap (3 hex characters)
 *
 * 12-bit bitmap representing PIO pin states (000 to 3FF in hex).
 * Example: "000", "3FF", "155"
 * Ada source: hm11.ads:238 `type PIO_Numbers is array (1 .. 3) of Hex_Character;`
 */
using PioNumbers = FixedString<3, HexCharValidator>;

/**
 * @brief Advertising flags (2 hex characters)
 *
 * 8-bit advertising flags for BLE advertising packets (0x00 to 0xFF).
 * Example: "06" (LE General Discoverable + BR/EDR Not Supported)
 * Ada source: hm11.ads:91 `type Advertising_FLAG is array (1 .. 2) of Hex_Character;`
 */
using AdvertisingFlag = FixedString<2, HexCharValidator>;

/**
 * @brief Temperature value in advertising (2 hex characters)
 *
 * 8-bit temperature value for inclusion in advertising packets.
 * Example: "19" (25 degrees Celsius in hex)
 * Ada source: hm11.ads:96 `type Advertising_Temperature_Type is array (1 .. 2) of Hex_Character;`
 */
using AdvertisingTemperatureType = FixedString<2, HexCharValidator>;

/**
 * @brief Humidity information (2 hex characters)
 *
 * 8-bit humidity value for sensor readings (0x00 to 0xFF).
 * Example: "3C" (60 percent in hex)
 * Ada source: hm11.ads:212 `type Humi_Information is array (1 .. 2) of Hex_Character;`
 */
using HumiInformation = FixedString<2, HexCharValidator>;

/**
 * @brief Custom advertisement data (12 hex characters)
 *
 * 48-bit custom data payload for advertising packets.
 * Example: "48454C4C4F00" (ASCII "HELLO" + null byte)
 * Ada source: hm11.ads:268 `type Advertisement_Data is array (1 .. 12) of Hex_Character;`
 */
using AdvertisementData = FixedString<12, HexCharValidator>;

/**
 * @brief GATT characteristic UUID (4 hex characters)
 *
 * 16-bit UUID for GATT characteristics.
 * Example: "2A19" (Battery Level characteristic UUID)
 * Ada source: hm11.ads:270 `type Characteristic_Type is array (1 .. 4) of Hex_Character;`
 */
using CharacteristicType = FixedString<4, HexCharValidator>;

// ============================================================================
// FixedString Digit Type Aliases
// ============================================================================
// These types represent fixed-size decimal digit arrays with compile-time
// size validation and runtime character validation. All characters must be
// in the range '0'-'9'.

/**
 * @brief PIN code (6 decimal digits)
 *
 * 6-digit PIN code for Bluetooth pairing authentication.
 * Example: "123456", "000000"
 * Ada source: hm11.ads:241 `type PIN_Type is array (1 .. 6) of Digit_Character;`
 */
using PinType = FixedString<6, DigitCharValidator>;

/**
 * @brief Sensor work interval (2 decimal digits)
 *
 * Sensor work interval in seconds (00 to 99) for periodic sensor readings.
 * Example: "30", "05"
 * Ada source: hm11.ads:245 `type Work_Interval is array (1 .. 2) of Digit_Character;`
 */
using WorkInterval = FixedString<2, DigitCharValidator>;

/**
 * @brief Connection timeout (6 decimal digits)
 *
 * Connection timeout value in milliseconds (000000 to 999999).
 * Example: "005000" (5 seconds)
 * Ada source: hm11.ads:261 `type Connect_Timeout is array (1 .. 6) of Digit_Character;`
 */
using ConnectTimeout = FixedString<6, DigitCharValidator>;

// ============================================================================
// Variable-Length String Type
// ============================================================================
// This type handles string values with variable length but fixed capacity.

/**
 * @brief Internal temperature string (up to 7 characters)
 *
 * String representation of internal temperature sensor reading.
 * Format: Variable length (e.g., "25.5", "100.25")
 * Ada source: hm11.ads:259 `subtype Internal_Temperature_Type is String (1 .. 7);`
 *
 * Note: This uses FixedString with digit validator as the most reasonable
 * C++ equivalent. In practice, the actual length may vary (Ada uses 7 as
 * max capacity). For proper variable-length handling, parser code should
 * use string_view and convert to numeric types as needed (per Decision 29).
 */
using InternalTemperatureType = FixedString<7, DigitCharValidator>;

} // namespace hm11::types
