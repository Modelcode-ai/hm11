#pragma once

#include <cstdint>

#include "fixed_string.hpp"
#include "ranged_integer.hpp"

namespace hm11 {

// ========================================================================
// Tag types for semantic differentiation
// ========================================================================

// Each tag type creates a unique template instantiation, preventing
// accidental mixing of semantically different values (e.g., temperature
// assigned to humidity percentage)

// Range type tags
struct PercentTag {};
struct TemperatureTag {};
struct HumidityTag {};
struct LayerConnectionLatencyTag {};
struct MacWhiteListIndexTag {};
struct PioNumberTag {};
struct PioCollectionRateTag {};
struct DiscoveredIndexTag {};
struct DiscoveryTimeTag {};

// Connection and timeout related tags
struct LinkLayerConnectionIntervalTag {};
struct ConnectionSupervisionTimeoutTag {};

// Power and sensor tags
struct ModulePowerTag {};
struct OutputPowerTag {};

// Enum type tags (for ranges that map to enums)
struct UartBaudRateTag {};
struct AdvertisingIntervalTag {};

// ========================================================================
// Range-constrained integer types (mapping Ada range types)
// ========================================================================

/// @brief Percentage value (0-100)
/// Maps to Ada: `type Persent is range 0 .. 100;`
using Percent = RangedInteger<uint8_t, 0, 100, PercentTag>;

/// @brief Temperature in Celsius (0-120)
/// Maps to Ada: `subtype Temperature_Type is Natural range 0 .. 120;`
using TemperatureType = RangedInteger<uint8_t, 0, 120, TemperatureTag>;

/// @brief Humidity percentage (0-100)
/// Maps to Ada: `subtype Humidity_Type is Natural range 0 .. 100;`
using HumidityType = RangedInteger<uint8_t, 0, 100, HumidityTag>;

/// @brief Layer connection latency (0-4)
/// Maps to Ada: `type Layer_Connection_Latency is range 0 .. 4;`
using LayerConnectionLatency = RangedInteger<uint8_t, 0, 4, LayerConnectionLatencyTag>;

/// @brief MAC white list index (1-3)
/// Maps to Ada: `type MAC_White_List_Index is range 1 .. 3;`
using MacWhiteListIndex = RangedInteger<uint8_t, 1, 3, MacWhiteListIndexTag>;

/// @brief PIO pin number (1-3)
/// Maps to Ada: `subtype PIO_Number is Positive range 1 .. 3;`
using PioNumber = RangedInteger<uint8_t, 1, 3, PioNumberTag>;

/// @brief PIO collection rate in seconds (0-99)
/// Maps to Ada: `type PIO_Collection_Rate is range 0 .. 99;`
using PioCollectionRate = RangedInteger<uint8_t, 0, 99, PioCollectionRateTag>;

/// @brief Discovered device index (0-5)
/// Maps to Ada: `type Discovered_Index is range 0 .. 5;`
using DiscoveredIndex = RangedInteger<uint8_t, 0, 5, DiscoveredIndexTag>;

/// @brief Discovery time in seconds (1-9)
/// Maps to Ada: `subtype Discovery_Time is Positive range 1 .. 9;`
using DiscoveryTime = RangedInteger<uint8_t, 1, 9, DiscoveryTimeTag>;

// ========================================================================
// Fixed-size string types (mapping Ada array types)
// ========================================================================

/// @brief MAC address as 12 hex characters
/// Maps to Ada: `type MAC_Address is array (1 .. 12) of Hex_Character;`
using MacAddress = FixedString<12, HexCharValidator>;

/// @brief UUID as 4 hex characters (16-bit UUID)
/// Maps to Ada: `type UUID is array (1 .. 4) of Hex_Character;`
using UUID = FixedString<4, HexCharValidator>;

/// @brief iBeacon UUID as 32 hex characters
/// Maps to Ada: `type iBeacon_UUID is array (1 .. 32) of Hex_Character;`
using IBeaconUuid = ExactFixedString<32, HexCharValidator>;

/// @brief Version type as 4 hex characters
/// Maps to Ada: `type Version_Type is array (1 .. 4) of Hex_Character;`
using VersionType = ExactFixedString<4, HexCharValidator>;

/// @brief Measured power as 2 hex characters
/// Maps to Ada: `type Measured_Power is array (1 .. 2) of Hex_Character;`
using MeasuredPower = ExactFixedString<2, HexCharValidator>;

/// @brief Handle type as 4 hex characters
/// Maps to Ada: `type Handle_Type is array (1 .. 4) of Hex_Character;`
using HandleType = FixedString<4, HexCharValidator>;

/// @brief Advertising FLAG as 2 hex characters
/// Maps to Ada: `type Advertising_FLAG is array (1 .. 2) of Hex_Character;`
using AdvertisingFlag = ExactFixedString<2, HexCharValidator>;

/// @brief Advertising temperature type as 2 hex characters
/// Maps to Ada: `type Advertising_Temperature_Type is array (1 .. 2) of Hex_Character;`
using AdvertisingTemperatureType = FixedString<2, HexCharValidator>;

/// @brief Humidity information as 2 hex characters
/// Maps to Ada: `type Humi_Information is array (1 .. 2) of Hex_Character;`
using HumiInformation = FixedString<2, HexCharValidator>;

/// @brief PIO numbers as 3 hex characters
/// Maps to Ada: `type PIO_Numbers is array (1 .. 3) of Hex_Character;`
using PioNumbers = FixedString<3, HexCharValidator>;

/// @brief PIN code as 6 digit characters
/// Maps to Ada: `type PIN_Type is array (1 .. 6) of Digit_Character;`
using PinType = FixedString<6, DigitCharValidator>;

/// @brief Work interval as 2 digit characters
/// Maps to Ada: `type Work_Interval is array (1 .. 2) of Digit_Character;`
using WorkInterval = FixedString<2, DigitCharValidator>;

/// @brief Connect timeout as 6 digit characters
/// Maps to Ada: `type Connect_Timeout is array (1 .. 6) of Digit_Character;`
using ConnectTimeout = FixedString<6, DigitCharValidator>;

/// @brief Advertisement data as 12 hex characters
/// Maps to Ada: `type Advertisement_Data is array (1 .. 12) of Hex_Character;`
using AdvertisementData = ExactFixedString<12, HexCharValidator>;

/// @brief Characteristic type as 4 hex characters
/// Maps to Ada: `type Characteristic_Type is array (1 .. 4) of Hex_Character;`
using CharacteristicType = FixedString<4, HexCharValidator>;

/// @brief Internal temperature type as 7-character string
/// Maps to Ada: `subtype Internal_Temperature_Type is String (1 .. 7);`
using InternalTemperatureType = FixedString<7, AnyCharValidator>;

// ========================================================================
// Enum types (preserved as C++ enum classes for type safety)
// ========================================================================

/// @brief HM-11 role: Peripheral or Central
/// Maps to Ada: `type Role is (Peripheral, Central);`
enum class Role : uint8_t { Peripheral = 0, Central = 1 };

/// @brief Advertising intervals with predefined values
/// Maps to Ada: `type Advertising_Interval is (...);`
enum class AdvertisingInterval : uint8_t {
    Ms100 = 0,   // '0'
    Ms211 = 1,   // '1'
    Ms252 = 2,   // '2'
    Ms318 = 3,   // '3'
    Ms417 = 4,   // '4'
    Ms546 = 5,   // '5'
    Ms760 = 6,   // '6'
    Ms852 = 7,   // '7'
    Ms1022 = 8,  // '8'
    Ms1285 = 9,  // '9'
    Ms2000 = 10, // 'A'
    Ms3000 = 11, // 'B'
    Ms4000 = 12, // 'C'
    Ms5000 = 13, // 'D'
    Ms6000 = 14, // 'E'
    Ms7000 = 15  // 'F'
};

/// @brief Advertising type enumeration
/// Maps to Ada: `type Advertising_Type is (...);`
enum class AdvertisingType : uint8_t {
    AdvertisingScanResponseConnectable = 0,
    LastDeviceConnect = 1,
    AdvertisingScanResponse = 2,
    Advertising = 3
};

/// @brief Advertising mode
/// Maps to Ada: `type Advertising_Mode is (Normal_Advertising, Reliable_Advertising);`
enum class AdvertisingMode : uint8_t { NormalAdvertising = 0, ReliableAdvertising = 1 };

/// @brief MAC address type
/// Maps to Ada: `type MAC_Address_Type is (...);`
enum class MacAddressType : uint8_t { NormalAddress = 0, StaticMac = 1, StaticRandomMac = 2, RandomMac = 3 };

/// @brief Write method for characteristics
/// Maps to Ada: `type Write_Method is (Write, Write_Without_Response);`
enum class WriteMethod : uint8_t { Write = 0, WriteWithoutResponse = 1 };

/// @brief UART baud rates
/// Maps to Ada: `type UART_Baud_Rate is (...);`
enum class UartBaudRate : uint8_t {
    Br9600 = 0,
    Br19200 = 1,
    Br38400 = 2,
    Br57600 = 3,
    Br115200 = 4,
    Br4800 = 5,
    Br2400 = 6,
    Br1200 = 7,
    Br230400 = 8
};

/// @brief Link layer connection intervals
/// Maps to Ada: `type Link_Layer_Connection_Interval is (...);`
enum class LinkLayerConnectionInterval : uint8_t {
    Ms7 = 0,
    Ms10 = 1,
    Ms15 = 2,
    Ms20 = 3,
    Ms25 = 4,
    Ms30 = 5,
    Ms35 = 6,
    Ms40 = 7,
    Ms45 = 8,
    Ms4000 = 9
};

/// @brief Connection supervision timeouts
/// Maps to Ada: `type Connection_Supervision_Timeout is (...);`
enum class ConnectionSupervisionTimeout : uint8_t {
    Ms100 = 0,
    Ms1000 = 1,
    Ms2000 = 2,
    Ms3000 = 3,
    Ms4000 = 4,
    Ms5000 = 5,
    Ms6000 = 6
};

/// @brief Connection result status
/// Maps to Ada: `type Connect_Result is (...);`
enum class ConnectResult : uint8_t {
    Connecting = 0,
    Connected = 1,
    ConnectError = 2,
    ConnectFail = 3,
    NoAddress = 4,
    OtherError = 5
};

/// @brief iBeacon deploy modes
/// Maps to Ada: `type iBeacon_Deploy_Mode is (Broadcast_Scanning, Only_Broadcast);`
enum class IBeaconDeployMode : uint8_t { BroadcastScanning = 0, OnlyBroadcast = 1 };

/// @brief Notify response types
/// Maps to Ada: `type Notify_Responce is (Send_Ok, Send_Er, Data_Er);`
enum class NotifyResponse : uint8_t { SendOk = 0, SendEr = 1, DataEr = 2 };

/// @brief Send data methods
/// Maps to Ada: `type Send_Data_Method is (...);`
enum class SendDataMethod : uint8_t { Write = 0, WriteWithoutResponse = 1, Indicate = 2, Notify = 3 };

/// @brief Send data characteristic methods
/// Maps to Ada: `type Send_Data_Characteristic is (Write, Write_Without_Response);`
enum class SendDataCharacteristic : uint8_t { Write = 0, WriteWithoutResponse = 1 };

/// @brief Characteristic UUID count options
/// Maps to Ada: `type Characteristic_UUID_Count is (...);`
enum class CharacteristicUuidCount : uint8_t { Query = 0, One = 1, OneAndNext = 2, OneAndPrev = 3 };

/// @brief RX gain settings
/// Maps to Ada: `type RX_Gain is (No, Open);`
enum class RxGain : uint8_t { No = 0, Open = 1 };

/// @brief Work type settings
/// Maps to Ada: `type Work_Type is (Start_Immediately, Respond_AT_Command);`
enum class WorkType : uint8_t { StartImmediately = 0, RespondAtCommand = 1 };

/// @brief Work modes
/// Maps to Ada: `type Work_Mode is (...);`
enum class WorkMode : uint8_t { TransmissionMode = 0, PioCollectionMode = 1, RemoteControlMode = 2 };

/// @brief Notify modes
/// Maps to Ada: `type Notify_Mode is (Without_Address, With_Address);`
enum class NotifyMode : uint8_t { WithoutAddress = 0, WithAddress = 1 };

/// @brief Output power levels
/// Maps to Ada: `type Output_Power is (Normal_Power, Max_Power);`
enum class OutputPower : uint8_t { NormalPower = 0, MaxPower = 1 };

/// @brief Parity bit settings
/// Maps to Ada: `type Parity_Bit is (None, Odd, Even);`
enum class ParityBit : uint8_t { None = 0, Odd = 1, Even = 2 };

/// @brief PIO output levels
/// Maps to Ada: `type PIO_Output is (Low, High);`
enum class PioOutput : uint8_t { Low = 0, High = 1 };

/// @brief Module power levels
/// Maps to Ada: `type Module_Power is (Dbm_Minus_23, Dbm_Minus_6, Dbm_0, Dbm_6);`
enum class ModulePower : uint8_t { DbmMinus23 = 0, DbmMinus6 = 1, Dbm0 = 2, Dbm6 = 3 };

/// @brief Stop bit settings
/// Maps to Ada: `type Stop_Bit is (One_Stop_Bit, Two_Stop_Bit);`
enum class StopBit : uint8_t { OneStopBit = 0, TwoStopBit = 1 };

/// @brief Sensor types
/// Maps to Ada: `type Sensor_Type is (None, DHT11, DS18B20);`
enum class SensorType : uint8_t { None = 0, DHT11 = 1, DS18B20 = 2 };

/// @brief Device information display options
/// Maps to Ada: `type Show_Device_Information is (...);`
enum class ShowDeviceInformation : uint8_t { DontShow = 0, ShowName = 1, ShowRssi = 2, ShowRssiAndName = 3 };

/// @brief Bond modes
/// Maps to Ada: `type Bond_Mode is (...);`
enum class BondMode : uint8_t { NotNeedPin = 0, AuthNotNeedPin = 1, AuthWithPin = 2, AuthAndBonded = 3 };

/// @brief UART sleep types
/// Maps to Ada: `type UART_Sleep_Type is (Can_Wake_Up_Through_UART, Shutdown_UART);`
enum class UartSleepType : uint8_t { CanWakeUpThroughUart = 0, ShutdownUart = 1 };

// ========================================================================
// Constants from Ada
// ========================================================================

/// @brief Maximum name length for discovered devices
/// Maps to Ada: `Max_Name_Length : constant := 248;`
constexpr std::size_t MAX_NAME_LENGTH = 248;

/// @brief Maximum RSSI string length
/// Maps to Ada: `Max_RSII_Length : constant := 4;`
constexpr std::size_t MAX_RSSI_LENGTH = 4;

/// @brief Maximum message length
/// Maps to Ada: `Max_Message_Length : constant := Discovered_Lenght;`
constexpr std::size_t MAX_MESSAGE_LENGTH = 270; // Calculated from Ada constants

/// @brief Connection established message
/// Maps to Ada: `When_Connected_Message : constant String := "OK+CONN";`
constexpr std::string_view WHEN_CONNECTED_MESSAGE = "OK+CONN";

/// @brief Disconnection message
/// Maps to Ada: `When_Disconnected_Message : constant String := "OK+LOST";`
constexpr std::string_view WHEN_DISCONNECTED_MESSAGE = "OK+LOST";

// ========================================================================
// Callback type aliases
// ========================================================================

/// @brief Callback for discovered devices
/// Maps to Ada: `type Discovered_Callback is access procedure (...);`
/// Extended with optional user_data parameter for C-style callback pattern
using DiscoveredCallback =
    void (*)(char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data);

/// @brief Callback for discovered iBeacon devices
/// Maps to Ada: `type Discovered_iBeacon_Callback is access procedure (Device : String);`
/// Extended with optional user_data parameter for C-style callback pattern
using DiscoveredIBeaconCallback = void (*)(std::string_view device, void* user_data);

/// @brief Callback for service UUIDs
/// Maps to Ada: `type Service_UUID_Callback is access procedure (Service : String);`
/// Extended with optional user_data parameter for C-style callback pattern
using ServiceUuidCallback = void (*)(std::string_view service, void* user_data);

/// @brief Callback for characteristic UUIDs
/// Maps to Ada: `type Characteristic_UUID_Callback is access procedure (...);`
/// Extended with optional user_data parameter for C-style callback pattern
using CharacteristicUuidCallback = void (*)(std::string_view characteristic, void* user_data);

} // namespace hm11