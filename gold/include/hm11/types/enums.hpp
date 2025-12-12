/**
 * @file enums.hpp
 * @brief Enumeration type definitions for HM-11 driver
 *
 * This header provides C++11 enum class definitions for all Ada enumeration
 * types. Enum classes provide strong type safety, preventing implicit
 * conversions and name collisions.
 *
 * ## Overview
 *
 * Ada enumeration types are directly translated to C++11 enum class with
 * identical enumerator names. This provides:
 * - Strong type safety (no implicit int conversion)
 * - Scoped enumerator names (no global namespace pollution)
 * - Direct 1:1 correspondence with Ada source
 *
 * ## Usage Example
 *
 * @code
 * using namespace hm11::types;
 *
 * // Enum usage
 * Role device_role = Role::Peripheral;
 * Advertising_Interval interval = Advertising_Interval::ms_1000;
 *
 * // Type safety - these won't compile
 * // int x = device_role;              // Error: no implicit conversion
 * // if (device_role == interval) {}   // Error: different types
 *
 * // Comparison
 * if (device_role == Role::Central) {
 *     // Device is in central mode
 * }
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

namespace hm11::types {

/**
 * @brief BLE device role
 *
 * Defines whether the device operates as a peripheral (slave) or central (master).
 * Ada source: hm11.ads:76 `type Role is (Peripheral, Central);`
 */
enum class Role : uint8_t {
    Peripheral, ///< Peripheral role (slave, advertises and accepts connections)
    Central     ///< Central role (master, scans and initiates connections)
};

/**
 * @brief Advertising interval presets
 *
 * Predefined advertising interval values in milliseconds. Lower intervals
 * increase power consumption but improve discoverability.
 * Ada source: hm11.ads:81-83
 */
enum class AdvertisingInterval : uint16_t {
    Ms100 = 100,   ///< 100 milliseconds
    Ms211 = 211,   ///< 211 milliseconds
    Ms252 = 252,   ///< 252 milliseconds
    Ms318 = 318,   ///< 318 milliseconds
    Ms417 = 417,   ///< 417 milliseconds
    Ms546 = 546,   ///< 546 milliseconds
    Ms760 = 760,   ///< 760 milliseconds
    Ms852 = 852,   ///< 852 milliseconds
    Ms1022 = 1022, ///< 1022 milliseconds
    Ms1285 = 1285, ///< 1285 milliseconds
    Ms2000 = 2000, ///< 2 seconds
    Ms3000 = 3000, ///< 3 seconds
    Ms4000 = 4000, ///< 4 seconds
    Ms5000 = 5000, ///< 5 seconds
    Ms6000 = 6000, ///< 6 seconds
    Ms7000 = 7000  ///< 7 seconds
};

/**
 * @brief Advertising packet type
 *
 * Defines the type of advertising packets to send.
 * Ada source: hm11.ads:85-89
 */
enum class AdvertisingType : uint8_t {
    AdvertisingScanResponseConnectable, ///< Full advertising (scannable, connectable)
    LastDeviceConnect,                  ///< Only connect to last paired device
    AdvertisingScanResponse,            ///< Advertising with scan response (not connectable)
    Advertising                         ///< Advertising only (no scan response, not connectable)
};

/**
 * @brief Advertising mode
 *
 * Normal advertising or reliable advertising with additional error checking.
 * Ada source: hm11.ads:94
 */
enum class AdvertisingMode : uint8_t {
    NormalAdvertising,  ///< Standard advertising mode
    ReliableAdvertising ///< Reliable advertising with acknowledgment
};

/**
 * @brief MAC address type
 *
 * Bluetooth MAC address type configuration.
 * Ada source: hm11.ads:98-99
 */
enum class MacAddressType : uint8_t {
    NormalAddress,   ///< Normal (public) MAC address
    StaticMac,       ///< Static MAC address
    StaticRandomMac, ///< Static random MAC address
    RandomMac        ///< Random MAC address
};

/**
 * @brief GATT write method
 *
 * Write operation type for GATT characteristics.
 * Ada source: hm11.ads:107
 */
enum class WriteMethod : uint8_t {
    Write,               ///< Write with response
    WriteWithoutResponse ///< Write without response (faster, no acknowledgment)
};

/**
 * @brief UART baud rate
 *
 * Supported baud rates for UART communication with the HM-11 module.
 * Ada source: hm11.ads:113-115
 */
enum class UartBaudRate : uint32_t {
    Br1200 = 1200,     ///< 1200 baud
    Br2400 = 2400,     ///< 2400 baud
    Br4800 = 4800,     ///< 4800 baud
    Br9600 = 9600,     ///< 9600 baud (default)
    Br19200 = 19200,   ///< 19200 baud
    Br38400 = 38400,   ///< 38400 baud
    Br57600 = 57600,   ///< 57600 baud
    Br115200 = 115200, ///< 115200 baud
    Br230400 = 230400  ///< 230400 baud
};

/**
 * @brief Link layer connection interval
 *
 * BLE connection interval presets in milliseconds. Lower intervals improve
 * latency but increase power consumption.
 * Ada source: hm11.ads:117-118
 */
enum class LinkLayerConnectionInterval : uint16_t {
    Ms7 = 7,      ///< 7.5 milliseconds (minimum)
    Ms10 = 10,    ///< 10 milliseconds
    Ms15 = 15,    ///< 15 milliseconds
    Ms20 = 20,    ///< 20 milliseconds
    Ms25 = 25,    ///< 25 milliseconds
    Ms30 = 30,    ///< 30 milliseconds
    Ms35 = 35,    ///< 35 milliseconds
    Ms40 = 40,    ///< 40 milliseconds
    Ms45 = 45,    ///< 45 milliseconds
    Ms4000 = 4000 ///< 4 seconds (maximum)
};

/**
 * @brief Connection supervision timeout
 *
 * Timeout for detecting link loss in milliseconds.
 * Ada source: hm11.ads:122-123
 */
enum class ConnectionSupervisionTimeout : uint16_t {
    Ms100 = 100,   ///< 100 milliseconds
    Ms1000 = 1000, ///< 1 second
    Ms2000 = 2000, ///< 2 seconds
    Ms3000 = 3000, ///< 3 seconds
    Ms4000 = 4000, ///< 4 seconds
    Ms5000 = 5000, ///< 5 seconds
    Ms6000 = 6000  ///< 6 seconds
};

/**
 * @brief Connection result status
 *
 * Status codes returned from connection attempts.
 * Ada source: hm11.ads:125-127
 */
enum class ConnectResult : uint8_t {
    Connecting,   ///< Connection in progress
    Connected,    ///< Successfully connected
    ConnectError, ///< Connection error occurred
    ConnectFail,  ///< Connection failed
    NoAddress,    ///< No address specified
    OtherError    ///< Other unspecified error
};

/**
 * @brief iBeacon deployment mode
 *
 * Operating mode for iBeacon functionality.
 * Ada source: hm11.ads:169
 */
enum class IBeaconDeployMode : uint8_t {
    BroadcastScanning, ///< Both broadcast iBeacon and scan for others
    OnlyBroadcast      ///< Only broadcast iBeacon (no scanning)
};

/**
 * @brief GATT notify response status
 *
 * Response status for GATT notify operations.
 * Ada source: hm11.ads:191-194
 */
enum class NotifyResponse : uint8_t {
    SendOk, ///< Command sent successfully
    SendEr, ///< Command send error
    DataEr  ///< Characteristic doesn't have notify property
};

/**
 * @brief Data transmission method
 *
 * Method for sending data over GATT.
 * Ada source: hm11.ads:196
 */
enum class SendDataMethod : uint8_t {
    Write,                ///< Write with response
    WriteWithoutResponse, ///< Write without response
    Indicate,             ///< Indication (acknowledged notification)
    Notify                ///< Notification (unacknowledged)
};

/**
 * @brief GATT write characteristic type
 *
 * Characteristic write type for data transmission.
 * Ada source: hm11.ads:198
 */
enum class SendDataCharacteristic : uint8_t {
    Write,               ///< Write with response
    WriteWithoutResponse ///< Write without response
};

/**
 * @brief Characteristic UUID query mode
 *
 * Mode for querying GATT characteristic UUIDs.
 * Ada source: hm11.ads:200-208
 */
enum class CharacteristicUuidCount : uint8_t {
    Query,      ///< Query characteristic
    One,        ///< Use one characteristic (from Set_Characteristic)
    OneAndNext, ///< Use two characteristics (current and next)
    OneAndPrev  ///< Use two characteristics (current and previous)
};

/**
 * @brief RX gain control
 *
 * Receiver gain setting for sensitivity vs. power consumption.
 * Ada source: hm11.ads:210
 */
enum class RxGain : uint8_t {
    No,  ///< RX gain disabled (lower power consumption)
    Open ///< RX gain enabled (higher sensitivity)
};

/**
 * @brief Module work type
 *
 * Defines when the module starts operation.
 * Ada source: hm11.ads:215
 */
enum class WorkType : uint8_t {
    StartImmediately, ///< Start immediately on power-up
    RespondAtCommand  ///< Wait for AT command to start
};

/**
 * @brief Module work mode
 *
 * Operating mode for the HM-11 module.
 * Ada source: hm11.ads:223-224
 */
enum class WorkMode : uint8_t {
    TransmissionMode,  ///< Standard data transmission mode
    PioCollectionMode, ///< PIO sensor collection mode
    RemoteControlMode  ///< Remote control mode
};

/**
 * @brief Notify address mode
 *
 * Whether to include address in notify messages.
 * Ada source: hm11.ads:226
 */
enum class NotifyMode : uint8_t {
    WithoutAddress, ///< Don't include address in notifications
    WithAddress     ///< Include address in notifications
};

/**
 * @brief RF output power
 *
 * RF transmission power level setting.
 * Ada source: hm11.ads:228
 */
enum class OutputPower : uint8_t {
    NormalPower, ///< Normal RF output power
    MaxPower     ///< Maximum RF output power
};

/**
 * @brief UART parity bit setting
 *
 * Parity configuration for UART communication.
 * Ada source: hm11.ads:230
 */
enum class ParityBit : uint8_t {
    None, ///< No parity bit
    Odd,  ///< Odd parity
    Even  ///< Even parity
};

/**
 * @brief PIO output state
 *
 * Digital output state for PIO pins.
 * Ada source: hm11.ads:236
 */
enum class PioOutput : uint8_t {
    Low, ///< Logic low (0V)
    High ///< Logic high (3.3V)
};

/**
 * @brief Module power level
 *
 * RF transmission power in dBm.
 * Ada source: hm11.ads:243
 */
enum class ModulePower : int8_t {
    DbmMinus23 = -23, ///< -23 dBm (lowest power)
    DbmMinus6 = -6,   ///< -6 dBm
    Dbm0 = 0,         ///< 0 dBm (default)
    Dbm6 = 6          ///< +6 dBm (maximum power)
};

/**
 * @brief UART stop bit configuration
 *
 * Number of stop bits for UART communication.
 * Ada source: hm11.ads:247
 */
enum class StopBit : uint8_t {
    OneStopBit, ///< 1 stop bit (standard)
    TwoStopBit  ///< 2 stop bits
};

/**
 * @brief Sensor type
 *
 * Type of sensor connected to the HMSensor module.
 * Ada source: hm11.ads:251
 */
enum class SensorType : uint8_t {
    None,   ///< No sensor connected
    DHT11,  ///< DHT11 temperature and humidity sensor
    DS18B20 ///< DS18B20 temperature sensor
};

/**
 * @brief Device information display in discovery
 *
 * What device information to show during discovery scan.
 * Ada source: hm11.ads:253-254
 */
enum class ShowDeviceInformation : uint8_t {
    DontShow,       ///< Don't show device information
    ShowName,       ///< Show device name only
    ShowRssi,       ///< Show RSSI only
    ShowRssiAndName ///< Show both RSSI and name
};

/**
 * @brief Bond mode
 *
 * Bluetooth bonding/pairing configuration.
 * Ada source: hm11.ads:263-264
 */
enum class BondMode : uint8_t {
    NotNeedPin,     ///< No PIN required
    AuthNotNeedPin, ///< Authentication without PIN
    AuthWithPin,    ///< Authentication with PIN
    AuthAndBonded   ///< Authentication with bonding
};

/**
 * @brief UART sleep mode
 *
 * UART behavior in sleep mode.
 * Ada source: hm11.ads:266
 */
enum class UartSleepType : uint8_t {
    CanWakeUpThroughUart, ///< UART can wake module from sleep
    ShutdownUart          ///< UART is shut down in sleep
};

} // namespace hm11::types
