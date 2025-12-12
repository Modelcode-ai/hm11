/**
 * @file enum_types.hpp
 * @brief Enumeration types for HM11 driver
 *
 * This file defines the enumeration types used by the HM11 driver.
 * These enumerations represent the different configurations and status values
 * for the HM11 Bluetooth module, directly mapped from the Ada implementation.
 */

#ifndef HM11_ENUM_TYPES_HPP
#define HM11_ENUM_TYPES_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace hm11 {

/**
 * @brief Scan stage kind for BLE device discovery parsing
 *
 * This enum class defines the different stages of parsing during device discovery.
 * It follows Decision 11 (explicit state enum class with switch/case for state machine)
 * to provide clear, debuggable control flow with zero overhead, handling circular buffer
 * wrapping and variable-length fields.
 */
enum class ScanStageKind : std::uint8_t {
    Selection, ///< Looking for prefixes to identify device types
    MAC,       ///< Parsing 12-byte MAC address
    Name,      ///< Parsing device name until CR+LF
    RSSI       ///< Parsing RSSI value until CR+LF
};

/**
 * @brief Role of the HM11 device
 *
 * Defines whether the HM11 operates in peripheral or central mode.
 */
enum class Role : std::uint8_t {
    Peripheral, ///< Device acts as a peripheral (server)
    Central     ///< Device acts as a central (client)
};

/**
 * @brief Advertising interval settings for HM11 in peripheral mode
 *
 * Defines the interval between advertising packets in milliseconds.
 */
enum class AdvertisingInterval : std::uint8_t {
    Ms100,  ///< 100ms interval
    Ms211,  ///< 211ms interval
    Ms252,  ///< 252ms interval
    Ms318,  ///< 318ms interval
    Ms417,  ///< 417ms interval
    Ms546,  ///< 546ms interval
    Ms760,  ///< 760ms interval
    Ms852,  ///< 852ms interval
    Ms1022, ///< 1022ms interval
    Ms1285, ///< 1285ms interval
    Ms2000, ///< 2000ms interval
    Ms3000, ///< 3000ms interval
    Ms4000, ///< 4000ms interval
    Ms5000, ///< 5000ms interval
    Ms6000, ///< 6000ms interval
    Ms7000  ///< 7000ms interval
};

/**
 * @brief Advertising type for HM11 in peripheral mode
 *
 * Defines the type of advertising packets sent by the HM11.
 */
enum class AdvertisingType : std::uint8_t {
    AdvertisingScanResponseConnectable, ///< Connectable advertising with scan response
    LastDeviceConnect,                  ///< Connect to last device
    AdvertisingScanResponse,            ///< Advertising with scan response
    Advertising                         ///< Regular advertising
};

/**
 * @brief Advertising mode for HM11 in peripheral mode
 *
 * Defines whether the HM11 uses normal or reliable advertising.
 */
enum class AdvertisingMode : std::uint8_t {
    NormalAdvertising,  ///< Normal advertising
    ReliableAdvertising ///< Reliable advertising to ensure advertising in long-time standby
};

/**
 * @brief MAC address type for HM11
 *
 * Defines the type of MAC address used by the HM11.
 */
enum class MacAddressType : std::uint8_t {
    NormalAddress,   ///< Normal address
    StaticMac,       ///< Static MAC address
    StaticRandomMac, ///< Static random MAC address
    RandomMac        ///< Random MAC address
};

/**
 * @brief Write method for GATT operations
 *
 * Defines the write method for GATT operations.
 */
enum class WriteMethod : std::uint8_t {
    Write,               ///< Standard write operation
    WriteWithoutResponse ///< Write without response
};

/**
 * @brief UART baud rate settings
 *
 * Defines the baud rate for UART communication with the HM11 module.
 */
enum class UartBaudRate : std::uint8_t {
    Br9600,   ///< 9600 baud
    Br19200,  ///< 19200 baud
    Br38400,  ///< 38400 baud
    Br57600,  ///< 57600 baud
    Br115200, ///< 115200 baud
    Br4800,   ///< 4800 baud
    Br2400,   ///< 2400 baud
    Br1200,   ///< 1200 baud
    Br230400  ///< 230400 baud
};

/**
 * @brief Link layer connection interval
 *
 * Defines the connection interval for the BLE link layer in milliseconds.
 */
enum class LinkLayerConnectionInterval : std::uint8_t {
    Ms7,   ///< 7ms interval
    Ms10,  ///< 10ms interval
    Ms15,  ///< 15ms interval
    Ms20,  ///< 20ms interval
    Ms25,  ///< 25ms interval
    Ms30,  ///< 30ms interval
    Ms35,  ///< 35ms interval
    Ms40,  ///< 40ms interval
    Ms45,  ///< 45ms interval
    Ms4000 ///< 4000ms interval
};

/**
 * @brief Connection supervision timeout
 *
 * Defines the supervision timeout for BLE connections in milliseconds.
 */
enum class ConnectionSupervisionTimeout : std::uint8_t {
    Ms100,  ///< 100ms timeout
    Ms1000, ///< 1000ms timeout
    Ms2000, ///< 2000ms timeout
    Ms3000, ///< 3000ms timeout
    Ms4000, ///< 4000ms timeout
    Ms5000, ///< 5000ms timeout
    Ms6000  ///< 6000ms timeout
};

/**
 * @brief Connect result status
 *
 * Defines the result of a connection attempt.
 */
enum class ConnectResult : std::uint8_t {
    Connecting,   ///< Connection in progress
    Connected,    ///< Successfully connected
    ConnectError, ///< Error during connection
    ConnectFail,  ///< Connection failed
    NoAddress,    ///< No address to connect to
    OtherError    ///< Other error
};

/**
 * @brief iBeacon deployment mode
 *
 * Defines the deployment mode for iBeacon functionality.
 */
enum class IBeaconDeployMode : std::uint8_t {
    BroadcastScanning, ///< Broadcast and scanning
    OnlyBroadcast      ///< Only broadcast, no scanning
};

/**
 * @brief Notification response codes
 *
 * Defines the response codes for notification operations.
 */
enum class NotifyResponse : std::uint8_t {
    SendOk, ///< Send command successful
    SendEr, ///< Send command error
    DataEr  ///< Characteristic doesn't have notify property
};

/**
 * @brief Send data method for GATT operations
 *
 * Defines the method for sending data in GATT operations.
 */
enum class SendDataMethod : std::uint8_t {
    Write,                ///< Standard write operation
    WriteWithoutResponse, ///< Write without response
    Indicate,             ///< Indicate operation
    Notify                ///< Notify operation
};

/**
 * @brief Send data characteristic for GATT operations
 *
 * Defines the characteristic for sending data in GATT operations.
 */
enum class SendDataCharacteristic : std::uint8_t {
    Write,               ///< Write characteristic
    WriteWithoutResponse ///< Write without response characteristic
};

/**
 * @brief Characteristic UUID count configuration
 *
 * Defines how many characteristic UUIDs to use.
 */
enum class CharacteristicUuidCount : std::uint8_t {
    Query,      ///< Query the device
    One,        ///< Use one characteristic
    OneAndNext, ///< Use one characteristic and the next
    OneAndPrev  ///< Use one characteristic and the previous
};

/**
 * @brief RX gain configuration
 *
 * Defines whether RX gain is enabled.
 */
enum class RxGain : std::uint8_t {
    No,  ///< No RX gain
    Open ///< RX gain enabled
};

/**
 * @brief Work type configuration
 *
 * Defines how the module works after power-on.
 */
enum class WorkType : std::uint8_t {
    StartImmediately, ///< Start working immediately
    RespondAtCommand  ///< Respond to AT commands
};

/**
 * @brief Work mode configuration
 *
 * Defines the working mode of the module.
 */
enum class WorkMode : std::uint8_t {
    TransmissionMode,  ///< Transmission mode
    PioCollectionMode, ///< PIO collection mode
    RemoteControlMode  ///< Remote control mode
};

/**
 * @brief Notify mode configuration
 *
 * Defines whether to include MAC address in notify messages.
 */
enum class NotifyMode : std::uint8_t {
    WithoutAddress, ///< No MAC address in notify messages
    WithAddress     ///< Include MAC address in notify messages
};

/**
 * @brief Output power configuration
 *
 * Defines the output power level.
 */
enum class OutputPower : std::uint8_t {
    NormalPower, ///< Normal power
    MaxPower     ///< Maximum power
};

/**
 * @brief Parity bit configuration
 *
 * Defines the UART parity bit configuration.
 */
enum class ParityBit : std::uint8_t {
    None, ///< No parity bit
    Odd,  ///< Odd parity
    Even  ///< Even parity
};

/**
 * @brief PIO output state
 *
 * Defines the output state of a PIO pin.
 */
enum class PioOutput : std::uint8_t {
    Low, ///< Low output
    High ///< High output
};

/**
 * @brief Module power configuration
 *
 * Defines the module's RF power level.
 */
enum class ModulePower : std::uint8_t {
    DbmMinus23, ///< -23 dBm
    DbmMinus6,  ///< -6 dBm
    Dbm0,       ///< 0 dBm
    Dbm6        ///< 6 dBm
};

/**
 * @brief Stop bit configuration
 *
 * Defines the UART stop bit configuration.
 */
enum class StopBit : std::uint8_t {
    OneStopBit, ///< One stop bit
    TwoStopBit  ///< Two stop bits
};

/**
 * @brief Sensor type configuration
 *
 * Defines the type of sensor connected to the module.
 */
enum class SensorType : std::uint8_t {
    None,   ///< No sensor
    Dht11,  ///< DHT11 temperature and humidity sensor
    Ds18b20 ///< DS18B20 temperature sensor
};

/**
 * @brief Show device information configuration
 *
 * Defines what device information to show during discovery.
 */
enum class ShowDeviceInformation : std::uint8_t {
    DontShow,       ///< Don't show device information
    ShowName,       ///< Show device name
    ShowRssi,       ///< Show RSSI
    ShowRssiAndName ///< Show both RSSI and name
};

/**
 * @brief Bond mode configuration
 *
 * Defines the bonding and authentication mode.
 */
enum class BondMode : std::uint8_t {
    NotNeedPin,     ///< No PIN needed
    AuthNotNeedPin, ///< Authentication but no PIN
    AuthWithPin,    ///< Authentication with PIN
    AuthAndBonded   ///< Authentication and bonded
};

/**
 * @brief UART sleep type configuration
 *
 * Defines whether UART can wake up the module.
 */
enum class UartSleepType : std::uint8_t {
    CanWakeUpThroughUart, ///< Can wake up through UART
    ShutdownUart          ///< UART shutdown during sleep
};

// Forward declarations for string conversion functions
std::string to_string(Role value);
std::string to_string(AdvertisingInterval value);
std::string to_string(AdvertisingType value);
std::string to_string(AdvertisingMode value);
std::string to_string(MacAddressType value);
std::string to_string(WriteMethod value);
std::string to_string(UartBaudRate value);
std::string to_string(LinkLayerConnectionInterval value);
std::string to_string(ConnectionSupervisionTimeout value);
std::string to_string(ConnectResult value);
std::string to_string(IBeaconDeployMode value);
std::string to_string(NotifyResponse value);
std::string to_string(SendDataMethod value);
std::string to_string(SendDataCharacteristic value);
std::string to_string(CharacteristicUuidCount value);
std::string to_string(RxGain value);
std::string to_string(WorkType value);
std::string to_string(WorkMode value);
std::string to_string(NotifyMode value);
std::string to_string(OutputPower value);
std::string to_string(ParityBit value);
std::string to_string(PioOutput value);
std::string to_string(ModulePower value);
std::string to_string(StopBit value);
std::string to_string(SensorType value);
std::string to_string(ShowDeviceInformation value);
std::string to_string(BondMode value);
std::string to_string(UartSleepType value);
std::string to_string(ScanStageKind value);

} // namespace hm11

#endif // HM11_ENUM_TYPES_HPP