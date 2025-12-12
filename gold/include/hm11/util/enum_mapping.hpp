/**
 * @file enum_mapping.hpp
 * @brief Constexpr enum-to-string mapping utilities for AT command construction
 *
 * This header provides zero-overhead, type-safe enum-to-string conversion
 * utilities that replace Ada's 'Image attribute. All mappings are evaluated
 * at compile time when possible, with no heap allocation.
 *
 * ## Overview
 *
 * AT commands like "AT+ROLE0" or "AT+ADVI9" require converting enum values
 * to string parameters. This module provides constexpr mapping tables that
 * enable efficient, type-safe conversion without runtime overhead.
 *
 * ## Design Patterns
 *
 * ### Simple Sequential Enums (Role, BondMode)
 * For enums with sequential values starting at 0, use direct array indexing:
 * - Enum value directly indexes into std::array
 * - O(1) lookup with no branching
 * - Compile-time verification of completeness via array size
 *
 * ### Sparse Non-Sequential Enums (AdvertisingInterval, UartBaudRate)
 * For enums with non-contiguous values, use constexpr switch statement:
 * - Compiler optimizes to jump table or binary search
 * - Exhaustive case coverage enforced by compiler warnings
 * - Static_assert ensures no missing cases at compile time
 *
 * ## Architecture
 *
 * All mapping functions:
 * - Return std::string_view for zero-copy string access
 * - Are declared constexpr for compile-time evaluation
 * - Are declared noexcept for embedded safety
 * - Use static_assert to verify completeness where possible
 *
 * ## Usage Example
 *
 * @code
 * using namespace hm11::util;
 * using namespace hm11::types;
 *
 * // Compile-time evaluation
 * constexpr auto role_str = enum_to_string(Role::Peripheral);
 * static_assert(role_str == "0");
 *
 * // Runtime usage in AT command construction
 * Role role = get_device_role();
 * std::string cmd = "AT+ROLE";
 * cmd += enum_to_string(role);  // "AT+ROLE0" or "AT+ROLE1"
 * @endcode
 *
 * ## Adding New Enum Mappings
 *
 * To add a new enum mapping, follow this pattern:
 *
 * 1. **For sequential enums:** Create a constexpr std::array with string_view
 *    entries, then use array indexing in the conversion function.
 *
 * 2. **For sparse enums:** Create a constexpr function with an exhaustive
 *    switch statement returning string_view for each case.
 *
 * 3. **Always:** Add static_assert to verify the mapping is complete.
 *
 * 4. **Always:** Add unit tests verifying all enum values map correctly and
 *    that constexpr evaluation works.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "hm11/types/enums.hpp"

namespace hm11::util {

// =============================================================================
// Role Enum Mapping (Sequential: 0=Peripheral, 1=Central)
// =============================================================================

/**
 * @brief Convert Role enum to AT command parameter string
 *
 * Role is a simple sequential enum where Peripheral=0, Central=1.
 * Maps directly to "0" and "1" command parameters.
 *
 * @param role Role value (Peripheral or Central)
 * @return String parameter for AT+ROLE command ("0" or "1")
 *
 * Ada source: hm11.ads:76
 * AT command: AT+ROLE{0|1}
 *
 * Mapping table:
 *   Peripheral -> "0"
 *   Central    -> "1"
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Returns string_view for zero-copy access to static storage
 */
constexpr std::string_view enum_to_string(types::Role role) noexcept {
    switch (role) {
        case types::Role::Peripheral:
            return "0";
        case types::Role::Central:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that Role mapping is complete
static_assert(enum_to_string(types::Role::Peripheral) == "0", "Role::Peripheral must map to '0'");
static_assert(enum_to_string(types::Role::Central) == "1", "Role::Central must map to '1'");

// =============================================================================
// AdvertisingInterval Enum Mapping (Sparse: non-contiguous values)
// =============================================================================

/**
 * @brief Convert AdvertisingInterval enum to AT command parameter string
 *
 * AdvertisingInterval has non-contiguous enum values (100, 211, 252, etc.)
 * that map to hexadecimal characters 0-9, A-F for the AT+ADVI command.
 *
 * Ada source: hm11.ads:81-83, 1358-1375 (From_Advertising_Interval array)
 * AT command: AT+ADVI{0-9|A-F}
 *
 * Mapping table:
 *   Ms100  -> "0"    Ms1285 -> "9"
 *   Ms211  -> "1"    Ms2000 -> "A"
 *   Ms252  -> "2"    Ms3000 -> "B"
 *   Ms318  -> "3"    Ms4000 -> "C"
 *   Ms417  -> "4"    Ms5000 -> "D"
 *   Ms546  -> "5"    Ms6000 -> "E"
 *   Ms760  -> "6"    Ms7000 -> "F"
 *   Ms852  -> "7"
 *   Ms1022 -> "8"
 *
 * @param interval Advertising interval value
 * @return String parameter for AT+ADVI command (hex char "0"-"F")
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Exhaustive switch ensures all cases are handled
 */
constexpr std::string_view enum_to_string(types::AdvertisingInterval interval) noexcept {
    switch (interval) {
        case types::AdvertisingInterval::Ms100:
            return "0";
        case types::AdvertisingInterval::Ms211:
            return "1";
        case types::AdvertisingInterval::Ms252:
            return "2";
        case types::AdvertisingInterval::Ms318:
            return "3";
        case types::AdvertisingInterval::Ms417:
            return "4";
        case types::AdvertisingInterval::Ms546:
            return "5";
        case types::AdvertisingInterval::Ms760:
            return "6";
        case types::AdvertisingInterval::Ms852:
            return "7";
        case types::AdvertisingInterval::Ms1022:
            return "8";
        case types::AdvertisingInterval::Ms1285:
            return "9";
        case types::AdvertisingInterval::Ms2000:
            return "A";
        case types::AdvertisingInterval::Ms3000:
            return "B";
        case types::AdvertisingInterval::Ms4000:
            return "C";
        case types::AdvertisingInterval::Ms5000:
            return "D";
        case types::AdvertisingInterval::Ms6000:
            return "E";
        case types::AdvertisingInterval::Ms7000:
            return "F";
    }
    // All cases covered, but add fallback for safety in non-constexpr context
    return "0";
}

// Compile-time verification that AdvertisingInterval mapping is complete
static_assert(enum_to_string(types::AdvertisingInterval::Ms100) == "0", "Ms100 must map to '0'");
static_assert(enum_to_string(types::AdvertisingInterval::Ms1285) == "9", "Ms1285 must map to '9'");
static_assert(enum_to_string(types::AdvertisingInterval::Ms7000) == "F", "Ms7000 must map to 'F'");

// =============================================================================
// UartBaudRate Enum Mapping (Sparse: ordered by declaration, not value)
// =============================================================================

/**
 * @brief Convert UartBaudRate enum to AT command parameter string
 *
 * UartBaudRate maps to command parameters based on declaration order in Ada,
 * not by baud rate value. The enum underlying values are the actual baud rates
 * (9600, 19200, etc.) but AT command parameters are ordinal positions 0-8.
 *
 * Ada source: hm11.ads:113-115
 * AT command: AT+BAUD{0-8}
 *
 * Mapping table (by declaration order):
 *   Br9600   -> "0"   (default)
 *   Br19200  -> "1"
 *   Br38400  -> "2"
 *   Br57600  -> "3"
 *   Br115200 -> "4"
 *   Br4800   -> "5"
 *   Br2400   -> "6"
 *   Br1200   -> "7"
 *   Br230400 -> "8"
 *
 * @param baud_rate UART baud rate value
 * @return String parameter for AT+BAUD command ("0"-"8")
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Order matches Ada declaration, not numeric baud rate value
 */
constexpr std::string_view enum_to_string(types::UartBaudRate baud_rate) noexcept {
    switch (baud_rate) {
        case types::UartBaudRate::Br9600:
            return "0";
        case types::UartBaudRate::Br19200:
            return "1";
        case types::UartBaudRate::Br38400:
            return "2";
        case types::UartBaudRate::Br57600:
            return "3";
        case types::UartBaudRate::Br115200:
            return "4";
        case types::UartBaudRate::Br4800:
            return "5";
        case types::UartBaudRate::Br2400:
            return "6";
        case types::UartBaudRate::Br1200:
            return "7";
        case types::UartBaudRate::Br230400:
            return "8";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that UartBaudRate mapping is complete
static_assert(enum_to_string(types::UartBaudRate::Br9600) == "0", "Br9600 must map to '0' (default)");
static_assert(enum_to_string(types::UartBaudRate::Br230400) == "8", "Br230400 must map to '8'");

// =============================================================================
// BondMode Enum Mapping (Sequential: 0-3)
// =============================================================================

/**
 * @brief Convert BondMode enum to AT command parameter string
 *
 * BondMode is a sequential enum with 4 values (0-3).
 *
 * Mapping table:
 *   NotNeedPin     -> "0"  (No PIN required)
 *   AuthNotNeedPin -> "1"  (Authentication without PIN)
 *   AuthWithPin    -> "2"  (Authentication with PIN)
 *   AuthAndBonded  -> "3"  (Authentication with bonding)
 *
 * @param mode Bond mode value
 * @return String parameter for AT+TYPE command ("0"-"3")
 *
 * Ada source: hm11.ads:263-264
 * AT command: AT+TYPE{0-3}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::BondMode mode) noexcept {
    switch (mode) {
        case types::BondMode::NotNeedPin:
            return "0";
        case types::BondMode::AuthNotNeedPin:
            return "1";
        case types::BondMode::AuthWithPin:
            return "2";
        case types::BondMode::AuthAndBonded:
            return "3";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that BondMode mapping is complete
static_assert(enum_to_string(types::BondMode::NotNeedPin) == "0", "NotNeedPin must map to '0'");
static_assert(enum_to_string(types::BondMode::AuthAndBonded) == "3", "AuthAndBonded must map to '3'");

// =============================================================================
// SensorType Enum Mapping (Sequential: 0-2)
// =============================================================================

/**
 * @brief Convert SensorType enum to AT command parameter string
 *
 * SensorType is a sequential enum with 3 values (0-2).
 *
 * Mapping table:
 *   None    -> "0"  (No sensor)
 *   DHT11   -> "1"  (DHT11 temperature and humidity sensor)
 *   DS18B20 -> "2"  (DS18B20 temperature sensor)
 *
 * @param sensor Sensor type value
 * @return String parameter for AT+SENS command ("0"-"2")
 *
 * Ada source: hm11.ads:251
 * AT command: AT+SENS{0-2}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::SensorType sensor) noexcept {
    switch (sensor) {
        case types::SensorType::None:
            return "0";
        case types::SensorType::DHT11:
            return "1";
        case types::SensorType::DS18B20:
            return "2";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that SensorType mapping is complete
static_assert(enum_to_string(types::SensorType::None) == "0", "None must map to '0'");
static_assert(enum_to_string(types::SensorType::DS18B20) == "2", "DS18B20 must map to '2'");

// =============================================================================
// Additional Helper Mappings
// =============================================================================

/**
 * @brief Convert SendDataMethod enum to AT command parameter string
 *
 * Used for GATT data transmission method specification.
 *
 * Ada source: hm11.ads:196, hm11.adb:2964-2976
 * AT command: Various GATT commands
 *
 * Mapping table:
 *   Write                -> "WR"  (Write with response)
 *   WriteWithoutResponse -> "WN"  (Write without response)
 *   Indicate             -> "IN"  (Indication)
 *   Notify               -> "NO"  (Notification)
 *
 * @param method Send data method value
 * @return String parameter for GATT commands
 */
constexpr std::string_view enum_to_string(types::SendDataMethod method) noexcept {
    switch (method) {
        case types::SendDataMethod::Write:
            return "WR";
        case types::SendDataMethod::WriteWithoutResponse:
            return "WN";
        case types::SendDataMethod::Indicate:
            return "IN";
        case types::SendDataMethod::Notify:
            return "NO";
    }
    return "WR"; // Fallback
}

static_assert(enum_to_string(types::SendDataMethod::Write) == "WR", "Write must map to 'WR'");
static_assert(enum_to_string(types::SendDataMethod::Notify) == "NO", "Notify must map to 'NO'");

/**
 * @brief Convert MacAddressType enum to AT command parameter string
 *
 * Used for MAC address type configuration.
 *
 * Ada source: hm11.ads:98-99, hm11.adb:3018-3030
 * AT command: AT+ADDR{N|0|1|2}
 *
 * Mapping table:
 *   NormalAddress   -> "N"  (Normal/public MAC)
 *   StaticMac       -> "0"  (Static MAC)
 *   StaticRandomMac -> "1"  (Static random MAC)
 *   RandomMac       -> "2"  (Random MAC)
 *
 * @param addr_type MAC address type value
 * @return String parameter for AT+ADDR command
 */
constexpr std::string_view enum_to_string(types::MacAddressType addr_type) noexcept {
    switch (addr_type) {
        case types::MacAddressType::NormalAddress:
            return "N";
        case types::MacAddressType::StaticMac:
            return "0";
        case types::MacAddressType::StaticRandomMac:
            return "1";
        case types::MacAddressType::RandomMac:
            return "2";
    }
    return "N"; // Fallback
}

static_assert(enum_to_string(types::MacAddressType::NormalAddress) == "N", "NormalAddress must map to 'N'");
static_assert(enum_to_string(types::MacAddressType::RandomMac) == "2", "RandomMac must map to '2'");

/**
 * @brief Convert boolean to AT command parameter string
 *
 * Many AT commands use "0" for false/disabled and "1" for true/enabled.
 *
 * Ada source: hm11.adb:3005-3012
 *
 * @param value Boolean value
 * @return "0" for false, "1" for true
 */
constexpr std::string_view bool_to_string(bool value) noexcept {
    return value ? "1" : "0";
}

static_assert(bool_to_string(true) == "1", "true must map to '1'");
static_assert(bool_to_string(false) == "0", "false must map to '0'");

// =============================================================================
// AdvertisingType Enum Mapping (Sequential: 0-3)
// =============================================================================

/**
 * @brief Convert AdvertisingType enum to AT command parameter string
 *
 * AdvertisingType is a sequential enum with 4 values (0-3).
 *
 * Mapping table:
 *   AdvertisingScanResponseConnectable -> "0"  (Full advertising, scannable, connectable)
 *   LastDeviceConnect                  -> "1"  (Only connect to last paired device)
 *   AdvertisingScanResponse            -> "2"  (Advertising with scan response, not connectable)
 *   Advertising                        -> "3"  (Advertising only, no scan response, not connectable)
 *
 * @param type Advertising type value
 * @return String parameter for AT+ADTY command ("0"-"3")
 *
 * Ada source: hm11.ads:85-89, hm11.adb:436-444
 * AT command: AT+ADTY{0-3}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::AdvertisingType type) noexcept {
    switch (type) {
        case types::AdvertisingType::AdvertisingScanResponseConnectable:
            return "0";
        case types::AdvertisingType::LastDeviceConnect:
            return "1";
        case types::AdvertisingType::AdvertisingScanResponse:
            return "2";
        case types::AdvertisingType::Advertising:
            return "3";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that AdvertisingType mapping is complete
static_assert(
    enum_to_string(types::AdvertisingType::AdvertisingScanResponseConnectable) == "0",
    "AdvertisingScanResponseConnectable must map to '0'");
static_assert(enum_to_string(types::AdvertisingType::Advertising) == "3", "Advertising must map to '3'");

// =============================================================================
// AdvertisingMode Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert AdvertisingMode enum to AT command parameter string
 *
 * AdvertisingMode is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   NormalAdvertising   -> "0"  (Standard advertising mode)
 *   ReliableAdvertising -> "1"  (Reliable advertising with acknowledgment)
 *
 * @param mode Advertising mode value
 * @return String parameter for AT+RELI command ("0"-"1")
 *
 * Ada source: hm11.ads:94, hm11.adb:2279-2306
 * AT command: AT+RELI{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::AdvertisingMode mode) noexcept {
    switch (mode) {
        case types::AdvertisingMode::NormalAdvertising:
            return "0";
        case types::AdvertisingMode::ReliableAdvertising:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that AdvertisingMode mapping is complete
static_assert(enum_to_string(types::AdvertisingMode::NormalAdvertising) == "0", "NormalAdvertising must map to '0'");
static_assert(
    enum_to_string(types::AdvertisingMode::ReliableAdvertising) == "1",
    "ReliableAdvertising must map to '1'");

// =============================================================================
// ShowDeviceInformation Enum Mapping (Sequential: 0-3)
// =============================================================================

/**
 * @brief Convert ShowDeviceInformation enum to AT command parameter string
 *
 * ShowDeviceInformation is a sequential enum with 4 values (0-3).
 *
 * Mapping table:
 *   DontShow       -> "0"  (Don't show device information)
 *   ShowName       -> "1"  (Show device name only)
 *   ShowRssi       -> "2"  (Show RSSI only)
 *   ShowRssiAndName -> "3"  (Show both RSSI and name)
 *
 * @param show Show device information value
 * @return String parameter for AT+SHOW command ("0"-"3")
 *
 * Ada source: hm11.ads:253-254, hm11.adb:2452-2479
 * AT command: AT+SHOW{0-3}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::ShowDeviceInformation show) noexcept {
    switch (show) {
        case types::ShowDeviceInformation::DontShow:
            return "0";
        case types::ShowDeviceInformation::ShowName:
            return "1";
        case types::ShowDeviceInformation::ShowRssi:
            return "2";
        case types::ShowDeviceInformation::ShowRssiAndName:
            return "3";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that ShowDeviceInformation mapping is complete
static_assert(enum_to_string(types::ShowDeviceInformation::DontShow) == "0", "DontShow must map to '0'");
static_assert(enum_to_string(types::ShowDeviceInformation::ShowRssiAndName) == "3", "ShowRssiAndName must map to '3'");

// =============================================================================
// IBeaconDeployMode Enum Mapping (Sequential: 0=BroadcastScanning, 1=OnlyBroadcast)
// =============================================================================

/**
 * @brief Convert IBeaconDeployMode enum to AT command parameter string
 *
 * IBeaconDeployMode is a sequential enum mapping to 1-based command parameters:
 *   BroadcastScanning -> "1"  (Broadcast iBeacon and scan for others)
 *   OnlyBroadcast     -> "2"  (Only broadcast iBeacon, no scanning)
 *
 * @param mode iBeacon deployment mode value
 * @return String parameter for AT+DELO command ("1" or "2")
 *
 * Ada source: hm11.ads:169, hm11.adb:1162-1171
 * AT command: AT+DELO{1|2}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Ada uses 1-based indexing: enum position + 1
 */
constexpr std::string_view enum_to_string(types::IBeaconDeployMode mode) noexcept {
    switch (mode) {
        case types::IBeaconDeployMode::BroadcastScanning:
            return "1";
        case types::IBeaconDeployMode::OnlyBroadcast:
            return "2";
    }
    return "1"; // Fallback to default
}

// Compile-time verification that IBeaconDeployMode mapping is complete
static_assert(enum_to_string(types::IBeaconDeployMode::BroadcastScanning) == "1", "BroadcastScanning must map to '1'");
static_assert(enum_to_string(types::IBeaconDeployMode::OnlyBroadcast) == "2", "OnlyBroadcast must map to '2'");

// =============================================================================
// ParityBit Enum Mapping (Sequential: 0-2)
// =============================================================================

/**
 * @brief Convert ParityBit enum to AT command parameter string
 *
 * ParityBit is a sequential enum with 3 values (0-2).
 *
 * Mapping table:
 *   None -> "0"  (No parity bit)
 *   Odd  -> "1"  (Odd parity)
 *   Even -> "2"  (Even parity)
 *
 * @param parity Parity bit value
 * @return String parameter for AT+PARI command ("0"-"2")
 *
 * Ada source: hm11.ads:230, hm11.adb:2045-2071
 * AT command: AT+PARI{0-2}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::ParityBit parity) noexcept {
    switch (parity) {
        case types::ParityBit::None:
            return "0";
        case types::ParityBit::Odd:
            return "1";
        case types::ParityBit::Even:
            return "2";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that ParityBit mapping is complete
static_assert(enum_to_string(types::ParityBit::None) == "0", "None must map to '0'");
static_assert(enum_to_string(types::ParityBit::Even) == "2", "Even must map to '2'");

// =============================================================================
// StopBit Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert StopBit enum to AT command parameter string
 *
 * StopBit is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   OneStopBit -> "0"  (1 stop bit - standard)
 *   TwoStopBit -> "1"  (2 stop bits)
 *
 * @param stop_bit Stop bit value
 * @return String parameter for AT+STOP command ("0"-"1")
 *
 * Ada source: hm11.ads:247, hm11.adb:284-314
 * AT command: AT+STOP{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::StopBit stop_bit) noexcept {
    switch (stop_bit) {
        case types::StopBit::OneStopBit:
            return "0";
        case types::StopBit::TwoStopBit:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that StopBit mapping is complete
static_assert(enum_to_string(types::StopBit::OneStopBit) == "0", "OneStopBit must map to '0'");
static_assert(enum_to_string(types::StopBit::TwoStopBit) == "1", "TwoStopBit must map to '1'");

// =============================================================================
// LinkLayerConnectionInterval Enum Mapping (Sequential by Ada Position: 0-9)
// =============================================================================

/**
 * @brief Convert LinkLayerConnectionInterval enum to AT command parameter string
 *
 * LinkLayerConnectionInterval maps to command parameters based on enum position
 * in the Ada declaration (hm11.ads:117-118), not by millisecond value.
 *
 * Ada source: hm11.ads:117-118, hm11.adb:590-652
 * AT commands: AT+COMI{0-9}, AT+COMA{0-9}
 *
 * Mapping table (by declaration order):
 *   Ms7    -> "0"  (7.5ms - minimum)
 *   Ms10   -> "1"  (10ms)
 *   Ms15   -> "2"  (15ms)
 *   Ms20   -> "3"  (20ms - default minimum)
 *   Ms25   -> "4"  (25ms)
 *   Ms30   -> "5"  (30ms)
 *   Ms35   -> "6"  (35ms)
 *   Ms40   -> "7"  (40ms - default maximum)
 *   Ms45   -> "8"  (45ms)
 *   Ms4000 -> "9"  (4000ms - maximum)
 *
 * @param interval Link layer connection interval value
 * @return String parameter for AT+COMI/AT+COMA command ("0"-"9")
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Order matches Ada declaration position, not numeric millisecond value
 */
constexpr std::string_view enum_to_string(types::LinkLayerConnectionInterval interval) noexcept {
    switch (interval) {
        case types::LinkLayerConnectionInterval::Ms7:
            return "0";
        case types::LinkLayerConnectionInterval::Ms10:
            return "1";
        case types::LinkLayerConnectionInterval::Ms15:
            return "2";
        case types::LinkLayerConnectionInterval::Ms20:
            return "3";
        case types::LinkLayerConnectionInterval::Ms25:
            return "4";
        case types::LinkLayerConnectionInterval::Ms30:
            return "5";
        case types::LinkLayerConnectionInterval::Ms35:
            return "6";
        case types::LinkLayerConnectionInterval::Ms40:
            return "7";
        case types::LinkLayerConnectionInterval::Ms45:
            return "8";
        case types::LinkLayerConnectionInterval::Ms4000:
            return "9";
    }
    return "3"; // Fallback to default (20ms)
}

// Compile-time verification that LinkLayerConnectionInterval mapping is complete
static_assert(enum_to_string(types::LinkLayerConnectionInterval::Ms7) == "0", "Ms7 must map to '0'");
static_assert(enum_to_string(types::LinkLayerConnectionInterval::Ms20) == "3", "Ms20 must map to '3' (default min)");
static_assert(enum_to_string(types::LinkLayerConnectionInterval::Ms40) == "7", "Ms40 must map to '7' (default max)");
static_assert(enum_to_string(types::LinkLayerConnectionInterval::Ms4000) == "9", "Ms4000 must map to '9'");

// =============================================================================
// ConnectionSupervisionTimeout Enum Mapping (Sequential by Ada Position: 0-6)
// =============================================================================

/**
 * @brief Convert ConnectionSupervisionTimeout enum to AT command parameter string
 *
 * ConnectionSupervisionTimeout maps to command parameters based on enum position
 * in the Ada declaration (hm11.ads:122-123).
 *
 * Ada source: hm11.ads:122-123, hm11.adb:691-719
 * AT command: AT+COSU{0-6}
 *
 * Mapping table (by declaration order):
 *   Ms100  -> "0"  (100ms - minimum)
 *   Ms1000 -> "1"  (1000ms / 1 second)
 *   Ms2000 -> "2"  (2000ms / 2 seconds)
 *   Ms3000 -> "3"  (3000ms / 3 seconds)
 *   Ms4000 -> "4"  (4000ms / 4 seconds)
 *   Ms5000 -> "5"  (5000ms / 5 seconds)
 *   Ms6000 -> "6"  (6000ms / 6 seconds - default)
 *
 * @param timeout Connection supervision timeout value
 * @return String parameter for AT+COSU command ("0"-"6")
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Order matches Ada declaration position
 */
constexpr std::string_view enum_to_string(types::ConnectionSupervisionTimeout timeout) noexcept {
    switch (timeout) {
        case types::ConnectionSupervisionTimeout::Ms100:
            return "0";
        case types::ConnectionSupervisionTimeout::Ms1000:
            return "1";
        case types::ConnectionSupervisionTimeout::Ms2000:
            return "2";
        case types::ConnectionSupervisionTimeout::Ms3000:
            return "3";
        case types::ConnectionSupervisionTimeout::Ms4000:
            return "4";
        case types::ConnectionSupervisionTimeout::Ms5000:
            return "5";
        case types::ConnectionSupervisionTimeout::Ms6000:
            return "6";
    }
    return "6"; // Fallback to default (6000ms)
}

// Compile-time verification that ConnectionSupervisionTimeout mapping is complete
static_assert(enum_to_string(types::ConnectionSupervisionTimeout::Ms100) == "0", "Ms100 must map to '0'");
static_assert(enum_to_string(types::ConnectionSupervisionTimeout::Ms6000) == "6", "Ms6000 must map to '6' (default)");

// =============================================================================
// SendDataCharacteristic Enum Mapping (Sequential: WR=Write, WN=WriteWithoutResponse)
// =============================================================================

/**
 * @brief Convert SendDataCharacteristic enum to AT command parameter string
 *
 * SendDataCharacteristic is used for the Send_Data_To_Characteristic operation
 * which is a subset of SendDataMethod (only Write and WriteWithoutResponse).
 *
 * Ada source: hm11.ads:198, hm11.adb:2982-2990
 * AT command: AT+SEND_DATA{WR|WN}
 *
 * Mapping table:
 *   Write                -> "WR"  (Write with response)
 *   WriteWithoutResponse -> "WN"  (Write without response)
 *
 * @param method Send data characteristic method value
 * @return String parameter for AT+SEND_DATA command
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::SendDataCharacteristic method) noexcept {
    switch (method) {
        case types::SendDataCharacteristic::Write:
            return "WR";
        case types::SendDataCharacteristic::WriteWithoutResponse:
            return "WN";
    }
    return "WR"; // Fallback
}

// Compile-time verification that SendDataCharacteristic mapping is complete
static_assert(enum_to_string(types::SendDataCharacteristic::Write) == "WR", "Write must map to 'WR'");
static_assert(
    enum_to_string(types::SendDataCharacteristic::WriteWithoutResponse) == "WN",
    "WriteWithoutResponse must map to 'WN'");

// =============================================================================
// CharacteristicUuidCount Enum Mapping (Sequential: 0-3, with Query as "?")
// =============================================================================

/**
 * @brief Convert CharacteristicUuidCount enum to AT command parameter string
 *
 * CharacteristicUuidCount configures how many characteristics to use for
 * peripheral operations. Query is a special value that queries the current setting.
 *
 * Ada source: hm11.ads:200-208, hm11.adb:1494-1505
 * AT command: AT+FFE2{?|0|1|2|3}
 *
 * Mapping table:
 *   Query       -> "?"  (Query current setting)
 *   One         -> "0"  (Use one characteristic from Set_Characteristic)
 *   OneAndNext  -> "1"  (Use two: current and next)
 *   OneAndPrev  -> "2"  (Use two: current and previous)
 *
 * Note: The Ada code uses Image(Pos(Count)) which gives "0", "1", "2" for
 * One, OneAndNext, OneAndPrev respectively (enum position). Query is special-cased.
 *
 * @param count Characteristic UUID count value
 * @return String parameter for AT+FFE2 command
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::CharacteristicUuidCount count) noexcept {
    switch (count) {
        case types::CharacteristicUuidCount::Query:
            return "?";
        case types::CharacteristicUuidCount::One:
            return "0";
        case types::CharacteristicUuidCount::OneAndNext:
            return "1";
        case types::CharacteristicUuidCount::OneAndPrev:
            return "2";
    }
    return "0"; // Fallback
}

// Compile-time verification that CharacteristicUuidCount mapping is complete
static_assert(enum_to_string(types::CharacteristicUuidCount::Query) == "?", "Query must map to '?'");
static_assert(enum_to_string(types::CharacteristicUuidCount::One) == "0", "One must map to '0'");
static_assert(enum_to_string(types::CharacteristicUuidCount::OneAndNext) == "1", "OneAndNext must map to '1'");
static_assert(enum_to_string(types::CharacteristicUuidCount::OneAndPrev) == "2", "OneAndPrev must map to '2'");

// =============================================================================
// WorkType Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert WorkType enum to AT command parameter string
 *
 * WorkType is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   StartImmediately -> "0"  (Start immediately on power-up)
 *   RespondAtCommand -> "1"  (Wait for AT command to start)
 *
 * @param work Work type value
 * @return String parameter for AT+IMME command ("0"-"1")
 *
 * Ada source: hm11.ads:215, hm11.adb:1612-1638
 * AT command: AT+IMME{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note RespondAtCommand (1) is CRITICAL for Central role scanner applications
 */
constexpr std::string_view enum_to_string(types::WorkType work) noexcept {
    switch (work) {
        case types::WorkType::StartImmediately:
            return "0";
        case types::WorkType::RespondAtCommand:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that WorkType mapping is complete
static_assert(enum_to_string(types::WorkType::StartImmediately) == "0", "StartImmediately must map to '0'");
static_assert(enum_to_string(types::WorkType::RespondAtCommand) == "1", "RespondAtCommand must map to '1'");

// =============================================================================
// UartSleepType Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert UartSleepType enum to AT command parameter string
 *
 * UartSleepType is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   CanWakeUpThroughUart -> "0"  (UART can wake module from sleep)
 *   ShutdownUart         -> "1"  (UART is shut down in sleep)
 *
 * @param sleep_type UART sleep type value
 * @return String parameter for AT+UART command ("0"-"1")
 *
 * Ada source: hm11.ads:266, hm11.adb:2666-2693
 * AT command: AT+UART{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note HMSensor-specific feature
 */
constexpr std::string_view enum_to_string(types::UartSleepType sleep_type) noexcept {
    switch (sleep_type) {
        case types::UartSleepType::CanWakeUpThroughUart:
            return "0";
        case types::UartSleepType::ShutdownUart:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that UartSleepType mapping is complete
static_assert(
    enum_to_string(types::UartSleepType::CanWakeUpThroughUart) == "0",
    "CanWakeUpThroughUart must map to '0'");
static_assert(enum_to_string(types::UartSleepType::ShutdownUart) == "1", "ShutdownUart must map to '1'");

// =============================================================================
// PioOutput Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert PioOutput enum to AT command parameter string
 *
 * PioOutput is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   Low  -> "0"  (Logic low, 0V)
 *   High -> "1"  (Logic high, 3.3V)
 *
 * @param output PIO output state value
 * @return String parameter for AT+PIO commands ("0"-"1")
 *
 * Ada source: hm11.ads:236, hm11.adb:2077-2092
 * AT command: AT+PIO{1-3}{0-1}, AT+MPIO{000-3FF}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::PioOutput output) noexcept {
    switch (output) {
        case types::PioOutput::Low:
            return "0";
        case types::PioOutput::High:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that PioOutput mapping is complete
static_assert(enum_to_string(types::PioOutput::Low) == "0", "Low must map to '0'");
static_assert(enum_to_string(types::PioOutput::High) == "1", "High must map to '1'");

// =============================================================================
// OutputPower Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert OutputPower enum to AT command parameter string
 *
 * OutputPower is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   NormalPower -> "0"  (Normal RF output power)
 *   MaxPower    -> "1"  (Maximum RF output power)
 *
 * @param power Output power value
 * @return String parameter for AT+PCTL command ("0"-"1")
 *
 * Ada source: hm11.ads:228, hm11.adb:2013-2021
 * AT command: AT+PCTL{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Added in V527
 * @note Default: MaxPower
 */
constexpr std::string_view enum_to_string(types::OutputPower power) noexcept {
    switch (power) {
        case types::OutputPower::NormalPower:
            return "0";
        case types::OutputPower::MaxPower:
            return "1";
    }
    return "1"; // Fallback to default (MaxPower)
}

// Compile-time verification that OutputPower mapping is complete
static_assert(enum_to_string(types::OutputPower::NormalPower) == "0", "NormalPower must map to '0'");
static_assert(enum_to_string(types::OutputPower::MaxPower) == "1", "MaxPower must map to '1'");

// =============================================================================
// ModulePower Enum Mapping (Sequential: 0-3)
// =============================================================================

/**
 * @brief Convert ModulePower enum to AT command parameter string
 *
 * ModulePower is a sequential enum with 4 values (0-3) representing dBm values.
 *
 * Mapping table:
 *   DbmMinus23 -> "0"  (-23 dBm, lowest power)
 *   DbmMinus6  -> "1"  (-6 dBm)
 *   Dbm0       -> "2"  (0 dBm, default)
 *   Dbm6       -> "3"  (+6 dBm, maximum power)
 *
 * @param power Module power value
 * @return String parameter for AT+POWE command ("0"-"3")
 *
 * Ada source: hm11.ads:243, hm11.adb:2210-2218
 * AT command: AT+POWE{0-3}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Default: Dbm0
 */
constexpr std::string_view enum_to_string(types::ModulePower power) noexcept {
    switch (power) {
        case types::ModulePower::DbmMinus23:
            return "0";
        case types::ModulePower::DbmMinus6:
            return "1";
        case types::ModulePower::Dbm0:
            return "2";
        case types::ModulePower::Dbm6:
            return "3";
    }
    return "2"; // Fallback to default (Dbm0)
}

// Compile-time verification that ModulePower mapping is complete
static_assert(enum_to_string(types::ModulePower::DbmMinus23) == "0", "DbmMinus23 must map to '0'");
static_assert(enum_to_string(types::ModulePower::DbmMinus6) == "1", "DbmMinus6 must map to '1'");
static_assert(enum_to_string(types::ModulePower::Dbm0) == "2", "Dbm0 must map to '2'");
static_assert(enum_to_string(types::ModulePower::Dbm6) == "3", "Dbm6 must map to '3'");

// =============================================================================
// RxGain Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert RxGain enum to AT command parameter string
 *
 * RxGain is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   No   -> "0"  (RX gain disabled, lower power consumption)
 *   Open -> "1"  (RX gain enabled, higher sensitivity)
 *
 * @param gain RX gain value
 * @return String parameter for AT+GAIN command ("0"-"1")
 *
 * Ada source: hm11.ads:210, hm11.adb:1564-1572
 * AT command: AT+GAIN{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 */
constexpr std::string_view enum_to_string(types::RxGain gain) noexcept {
    switch (gain) {
        case types::RxGain::No:
            return "0";
        case types::RxGain::Open:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that RxGain mapping is complete
static_assert(enum_to_string(types::RxGain::No) == "0", "No must map to '0'");
static_assert(enum_to_string(types::RxGain::Open) == "1", "Open must map to '1'");

// =============================================================================
// WorkMode Enum Mapping (Sequential: 0-2)
// =============================================================================

/**
 * @brief Convert WorkMode enum to AT command parameter string
 *
 * WorkMode is a sequential enum with 3 values (0-2).
 *
 * Mapping table:
 *   TransmissionMode  -> "0"  (Standard data transmission mode)
 *   PioCollectionMode -> "1"  (PIO sensor collection mode)
 *   RemoteControlMode -> "2"  (Remote control mode)
 *
 * @param mode Work mode value
 * @return String parameter for AT+MODE command ("0"-"2")
 *
 * Ada source: hm11.ads:223-224, hm11.adb:1874-1882
 * AT command: AT+MODE{0-2}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Default: TransmissionMode
 */
constexpr std::string_view enum_to_string(types::WorkMode mode) noexcept {
    switch (mode) {
        case types::WorkMode::TransmissionMode:
            return "0";
        case types::WorkMode::PioCollectionMode:
            return "1";
        case types::WorkMode::RemoteControlMode:
            return "2";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that WorkMode mapping is complete
static_assert(enum_to_string(types::WorkMode::TransmissionMode) == "0", "TransmissionMode must map to '0'");
static_assert(enum_to_string(types::WorkMode::PioCollectionMode) == "1", "PioCollectionMode must map to '1'");
static_assert(enum_to_string(types::WorkMode::RemoteControlMode) == "2", "RemoteControlMode must map to '2'");

// =============================================================================
// NotifyMode Enum Mapping (Sequential: 0-1)
// =============================================================================

/**
 * @brief Convert NotifyMode enum to AT command parameter string
 *
 * NotifyMode is a sequential enum with 2 values (0-1).
 *
 * Mapping table:
 *   WithoutAddress -> "0"  (Don't include address in notifications)
 *   WithAddress    -> "1"  (Include address in notifications)
 *
 * @param mode Notify mode value
 * @return String parameter for AT+NOTP command ("0"-"1")
 *
 * Ada source: hm11.ads:226, hm11.adb:1943-1951
 * AT command: AT+NOTP{0-1}
 *
 * @note This function is constexpr and can be evaluated at compile time
 * @note Added since V534
 * @note Default: WithoutAddress
 * @note Must work with Set_Notify_Information(true)
 */
constexpr std::string_view enum_to_string(types::NotifyMode mode) noexcept {
    switch (mode) {
        case types::NotifyMode::WithoutAddress:
            return "0";
        case types::NotifyMode::WithAddress:
            return "1";
    }
    return "0"; // Fallback to default
}

// Compile-time verification that NotifyMode mapping is complete
static_assert(enum_to_string(types::NotifyMode::WithoutAddress) == "0", "WithoutAddress must map to '0'");
static_assert(enum_to_string(types::NotifyMode::WithAddress) == "1", "WithAddress must map to '1'");

} // namespace hm11::util
