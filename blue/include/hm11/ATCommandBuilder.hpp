// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file ATCommandBuilder.hpp
 * @brief Runtime AT command construction utilities.
 *
 * Provides a simple runtime API to build AT command strings without compile‑time
 * templates. Users specify the command enum and a list of unsigned integer
 * parameters; the builder concatenates the base command, parameters (in decimal),
 * and appends a trailing CR/LF.
 */

#pragma once

#include "hm11/ATCommandBuffer.hpp"

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace hm11 {

/**
 * @brief Enumeration of supported AT commands.
 */
enum class AtCommand : std::uint8_t {
    SetRole,                 // "AT+ROLE="
    SetiBeaconUUID,          // "AT+IBE="
    SetiBeaconSwitch,        // "AT+IBEA"
    GetiBeaconSwitch,        // "AT+IBEA?"
    SetiBeaconDeployMode,    // "AT+DELO"
    SetiBeaconMajor,         // "AT+MAJ"
    SetiBeaconMinor,         // "AT+MINO"
    SetiBeaconMeasuredPower, // "AT+MEAS"
    SetWorkType,             // "AT+IMME="
    GetWorkType,             // "AT+IMME?"
    SetSensorType,           // "AT+SENS"
    GetSensorType,           // "AT+SENS?"
    SetSensorWorkInterval,   // "AT+RAT"
    GetSensorWorkInterval,   // "AT+RAT??"
    GetSensorTempHumidity,   // "AT+TEHU"
    SetAdvertisingInterval,  // "AT+ADVI"
    GetAdvertisingInterval,  // "AT+ADVI?"
    SetAdvertisingType,      // "AT+ADTY"
    GetAdvertisingType,      // "AT+ADTY?"
    SetAdvertisingFlag,      // "AT+FLAG"
    SetTemperatureInfo,      // "AT+TEMP"
    SetHumiInfo,             // "AT+HUMI"
    SetAdvertisementData,    // "AT+PACK"
    SetReliableAdvertising,  // "AT+RELI"
    GetReliableAdvertising,  // "AT+RELI?"
    // White List
    SetWhiteListSwitch,      // "AT+ALLO"
    GetWhiteListSwitch,      // "AT+ALLO?"
    SetWhiteListMAC,         // "AT+AD"
    GetWhiteListMAC,         // "AT+AD"
    // Battery
    SetBatteryMonitor,       // "AT+BATC"
    GetBatteryMonitor,       // "AT+BATC?"
    SetBatteryInfo,          // "AT+BATT"
    GetBatteryInfo,          // "AT+BATT?"
    // UART Config
    SetUARTBaudRate,         // "AT+BAUD"
    GetUARTBaudRate,         // "AT+BAUD?"
    SetUARTFlowControl,      // "AT+FIOW"
    GetUARTFlowControl,      // "AT+FIOW?"
    SetParityBit,            // "AT+PARI"
    GetParityBit,            // "AT+PARI?"
    SetStopBit,              // "AT+STOP"
    GetStopBit,              // "AT+STOP?"
    SetUARTSleepType,        // "AT+PWRM"
    GetUARTSleepType,        // "AT+PWRM?"
    // Link Layer
    SetMinConnInterval,      // "AT+COMI"
    GetMinConnInterval,      // "AT+COMI?"
    SetMaxConnInterval,      // "AT+COMA"
    GetMaxConnInterval,      // "AT+COMA?"
    SetSlaveLatency,         // "AT+COLA"
    GetSlaveLatency,         // "AT+COLA?"
    SetSupervisionTimeout,   // "AT+COSU"
    GetSupervisionTimeout,   // "AT+COSU?"
    SetUpdateConnection,     // "AT+COUP"
    GetUpdateConnection,     // "AT+COUP?"
    // Connection
    ClearLastConnectedAddress,    // "AT+CLEAR"
    ConnectLastDevice,            // "AT+CONNL"
    ConnectByMAC,                 // "AT+CO"
    ConnectByIndex,               // "AT+CONN"
    GetLastConnectedDeviceAddress,// "AT+RADD?"
    SetSaveConnectedMAC,          // "AT+SAVE"
    GetSaveConnectedMAC,          // "AT+SAVE?"
    SetConnectRemoteDeviceTimeout,// "AT+TCON"
    GetConnectRemoteDeviceTimeout,// "AT+TCON?"
    // Discovery
    SetDiscoveryTime,             // "AT+SCAN"
    GetDiscoveryTime,             // "AT+SCAN?"
    SetShowDeviceInformation,     // "AT+SHOW"
    GetShowDeviceInformation,     // "AT+SHOW?"
    // GATT
    EnableCharacteristicNotify,   // "AT+NOTIFY_ON"
    DisableCharacteristicNotify,  // "AT+NOTIFYOFF"
    ReadCharacteristicNotify,     // "AT+READDATA"
    SetMethodAndCharacteristicHandle, // "AT+SET_WAY"
    SendDataToCharacteristic,     // "AT+SEND_DATA"
    SetUseCharacteristicUUIDCount,// "AT+FFE2"
    SetCharacteristic,            // "AT+CHAR0x"
    GetCharacteristic,            // "AT+CHAR?"
    // Service UUID
    SetServiceUUID,               // "AT+UUID0x"
    GetServiceUUID,               // "AT+UUID?"
    // Power
    SetModulePower,               // "AT+POWE"
    GetModulePower,               // "AT+POWE?"
    SetOutputPower,               // "AT+PCTL"
    GetOutputPower,               // "AT+PCTL?"
    // Module Config (additional methods)
    SetWorkMode,                  // "AT+MODE"
    GetWorkMode,                  // "AT+MODE?"
    SetNotifyMode,                // "AT+NOTP"
    GetNotifyMode,                // "AT+NOTP?"
    SetModuleName,                // "AT+NAME"
    GetModuleName,                // "AT+NAME?"
    SetNotifyInformation,         // "AT+NOTI"
    GetNotifyInformation,         // "AT+NOTI?"
    SetModuleRXGain,              // "AT+GAIN"
    GetModuleRXGain,              // "AT+GAIN?"
    // PIO Control
    SetPIOOutputStatus,           // "AT+PIO" or "AT+PIO1"
    GetPIOOutputStatus,           // "AT+PIO?" or "AT+PIO1?"
    GetPIOsOutputStatus,          // "AT+PIO??"
    SetPIOsOutputStatus,          // "AT+MPIO"
    SetPIOCollectionRate,         // "AT+CYC"
    GetPIOCollectionRate,         // "AT+CYC?"
    SetPowerPinOutput,            // "AT+BEFC"
    GetPowerPinOutput,            // "AT+BEFC?"
    SetConnectPinOutput,          // "AT+AFTC"
    GetConnectPinOutput,          // "AT+AFTC?"
    // Security
    SetBondMode,                  // "AT+TYPE"
    GetBondMode,                  // "AT+TYPE?"
    SetPINCode,                   // "AT+PASS"
    GetPINCode,                   // "AT+PASS?"
    RemoveBondInformation,        // "AT+ERASE"
    // iBeacon Query
    GetiBeaconMajor,              // "AT+MARJ?"
    GetiBeaconMinor,              // "AT+MINO?"
    GetiBeaconMeasuredPower,      // "AT+MEAS?"
    // Power Management
    SetModuleAutoSleep,           // "AT+PWRM"
    GetModuleAutoSleep,           // "AT+PWRM?"
    // HMSensor
    GetModuleTemperature,         // "AT+TEMP?"
    // Discovery
    Scan,                         // "AT+DISC?"
    ScaniBeacon,                  // "AT+DISI?"
    // GATT Discovery
    FindAllServicesUUID,          // "AT+FINDALLSERVICES?"
    FindAllCharacteristicUUID,    // "AT+FINDALLCHARS?"
    FindCharacteristicUUID        // "AT+FINDCHAR"
};

/**
 * @brief Retrieve the base command string for a given AtCommand.
 * The returned string does **not** include the terminating CR/LF; the builder
 * appends "\r\n" automatically.
 */
constexpr const char* command_base(AtCommand cmd) noexcept {
    switch (cmd) {
        case AtCommand::SetRole:
            return "AT+ROLE";
        case AtCommand::SetiBeaconUUID:
            return "AT+IBE";
        case AtCommand::SetWorkType:
            return "AT+IMME";
        case AtCommand::GetWorkType:
            return "AT+IMME?";
        case AtCommand::SetiBeaconDeployMode:
            return "AT+DELO";
        case AtCommand::SetiBeaconSwitch:
            return "AT+IBEA";
        case AtCommand::GetiBeaconSwitch:
            return "AT+IBEA?";
        case AtCommand::SetSensorType:
            return "AT+SENS";
        case AtCommand::GetSensorType:
            return "AT+SENS?";
        case AtCommand::SetSensorWorkInterval:
            return "AT+RAT";
        case AtCommand::GetSensorWorkInterval:
            return "AT+RAT??";
        case AtCommand::SetiBeaconMajor:
            return "AT+MAJ";
        case AtCommand::SetiBeaconMinor:
            return "AT+MINO";
        case AtCommand::SetiBeaconMeasuredPower:
            return "AT+MEAS";
        case AtCommand::GetSensorTempHumidity:
            return "AT+TEHU?";
        case AtCommand::SetAdvertisingInterval:
            return "AT+ADVI";
        case AtCommand::GetAdvertisingInterval:
            return "AT+ADVI?";
        case AtCommand::SetAdvertisingType:
            return "AT+ADTY";
        case AtCommand::GetAdvertisingType:
            return "AT+ADTY?";
        case AtCommand::SetAdvertisingFlag:
            return "AT+FLAG";
        case AtCommand::SetTemperatureInfo:
            return "AT+TEMP";
        case AtCommand::SetHumiInfo:
            return "AT+HUMI";
        case AtCommand::SetAdvertisementData:
            return "AT+PACK";
        case AtCommand::SetReliableAdvertising:
            return "AT+RELI";
        case AtCommand::GetReliableAdvertising:
            return "AT+RELI?";
        case AtCommand::SetWhiteListSwitch:
            return "AT+ALLO";
        case AtCommand::GetWhiteListSwitch:
            return "AT+ALLO?";
        case AtCommand::SetWhiteListMAC:
        case AtCommand::GetWhiteListMAC:
            return "AT+AD";
        case AtCommand::SetBatteryMonitor:
            return "AT+BATC";
        case AtCommand::GetBatteryMonitor:
            return "AT+BATC?";
        case AtCommand::SetBatteryInfo:
            return "AT+BATT";
        case AtCommand::GetBatteryInfo:
            return "AT+BATT?";
        // UART Config
        case AtCommand::SetUARTBaudRate:
            return "AT+BAUD";
        case AtCommand::GetUARTBaudRate:
            return "AT+BAUD?";
        case AtCommand::SetUARTFlowControl:
            return "AT+FIOW";
        case AtCommand::GetUARTFlowControl:
            return "AT+FIOW?";
        case AtCommand::SetParityBit:
            return "AT+PARI";
        case AtCommand::GetParityBit:
            return "AT+PARI?";
        case AtCommand::SetStopBit:
            return "AT+STOP";
        case AtCommand::GetStopBit:
            return "AT+STOP?";
        case AtCommand::SetUARTSleepType:
            return "AT+PWRM";
        case AtCommand::GetUARTSleepType:
            return "AT+PWRM?";
        // Link Layer
        case AtCommand::SetMinConnInterval:
            return "AT+COMI";
        case AtCommand::GetMinConnInterval:
            return "AT+COMI?";
        case AtCommand::SetMaxConnInterval:
            return "AT+COMA";
        case AtCommand::GetMaxConnInterval:
            return "AT+COMA?";
        case AtCommand::SetSlaveLatency:
            return "AT+COLA";
        case AtCommand::GetSlaveLatency:
            return "AT+COLA?";
        case AtCommand::SetSupervisionTimeout:
            return "AT+COSU";
        case AtCommand::GetSupervisionTimeout:
            return "AT+COSU?";
        case AtCommand::SetUpdateConnection:
            return "AT+COUP";
        case AtCommand::GetUpdateConnection:
            return "AT+COUP?";
        // Connection
        case AtCommand::ClearLastConnectedAddress:
            return "AT+CLEAR";
        case AtCommand::ConnectLastDevice:
            return "AT+CONNL";
        case AtCommand::ConnectByMAC:
            return "AT+CO";
        case AtCommand::ConnectByIndex:
            return "AT+CONN";
        case AtCommand::GetLastConnectedDeviceAddress:
            return "AT+RADD?";
        case AtCommand::SetSaveConnectedMAC:
            return "AT+SAVE";
        case AtCommand::GetSaveConnectedMAC:
            return "AT+SAVE?";
        case AtCommand::SetConnectRemoteDeviceTimeout:
            return "AT+TCON";
        case AtCommand::GetConnectRemoteDeviceTimeout:
            return "AT+TCON?";
        // Discovery
        case AtCommand::SetDiscoveryTime:
            return "AT+SCAN";
        case AtCommand::GetDiscoveryTime:
            return "AT+SCAN?";
        case AtCommand::SetShowDeviceInformation:
            return "AT+SHOW";
        case AtCommand::GetShowDeviceInformation:
            return "AT+SHOW?";
        // GATT
        case AtCommand::EnableCharacteristicNotify:
            return "AT+NOTIFY_ON";
        case AtCommand::DisableCharacteristicNotify:
            return "AT+NOTIFYOFF";
        case AtCommand::ReadCharacteristicNotify:
            return "AT+READDATA";
        case AtCommand::SetMethodAndCharacteristicHandle:
            return "AT+SET_WAY";
        case AtCommand::SendDataToCharacteristic:
            return "AT+SEND_DATA";
        case AtCommand::SetUseCharacteristicUUIDCount:
            return "AT+FFE2";
        case AtCommand::SetCharacteristic:
            return "AT+CHAR0x";
        case AtCommand::GetCharacteristic:
            return "AT+CHAR?";
        // Service UUID
        case AtCommand::SetServiceUUID:
            return "AT+UUID0x";
        case AtCommand::GetServiceUUID:
            return "AT+UUID?";
        // Power
        case AtCommand::SetModulePower:
            return "AT+POWE";
        case AtCommand::GetModulePower:
            return "AT+POWE?";
        case AtCommand::SetOutputPower:
            return "AT+PCTL";
        case AtCommand::GetOutputPower:
            return "AT+PCTL?";
        // Module Config (additional methods)
        case AtCommand::SetWorkMode:
            return "AT+MODE";
        case AtCommand::GetWorkMode:
            return "AT+MODE?";
        case AtCommand::SetNotifyMode:
            return "AT+NOTP";
        case AtCommand::GetNotifyMode:
            return "AT+NOTP?";
        case AtCommand::SetModuleName:
            return "AT+NAME";
        case AtCommand::GetModuleName:
            return "AT+NAME?";
        case AtCommand::SetNotifyInformation:
            return "AT+NOTI";
        case AtCommand::GetNotifyInformation:
            return "AT+NOTI?";
        case AtCommand::SetModuleRXGain:
            return "AT+GAIN";
        case AtCommand::GetModuleRXGain:
            return "AT+GAIN?";
        // PIO Control
        case AtCommand::SetPIOOutputStatus:
        case AtCommand::GetPIOOutputStatus:
            return "AT+PIO";
        case AtCommand::GetPIOsOutputStatus:
            return "AT+PIO??";
        case AtCommand::SetPIOsOutputStatus:
            return "AT+MPIO";
        case AtCommand::SetPIOCollectionRate:
            return "AT+CYC";
        case AtCommand::GetPIOCollectionRate:
            return "AT+CYC?";
        case AtCommand::SetPowerPinOutput:
            return "AT+BEFC";
        case AtCommand::GetPowerPinOutput:
            return "AT+BEFC?";
        case AtCommand::SetConnectPinOutput:
            return "AT+AFTC";
        case AtCommand::GetConnectPinOutput:
            return "AT+AFTC?";
        // Security
        case AtCommand::SetBondMode:
            return "AT+TYPE";
        case AtCommand::GetBondMode:
            return "AT+TYPE?";
        case AtCommand::SetPINCode:
            return "AT+PASS";
        case AtCommand::GetPINCode:
            return "AT+PASS?";
        case AtCommand::RemoveBondInformation:
            return "AT+ERASE";
        case AtCommand::GetiBeaconMajor:
            return "AT+MARJ?";
        case AtCommand::GetiBeaconMinor:
            return "AT+MINO?";
        case AtCommand::GetiBeaconMeasuredPower:
            return "AT+MEAS?";
        case AtCommand::SetModuleAutoSleep:
            return "AT+PWRM";
        case AtCommand::GetModuleAutoSleep:
            return "AT+PWRM?";
        case AtCommand::GetModuleTemperature:
            return "AT+TEMP?";
        // Discovery
        case AtCommand::Scan:
            return "AT+DISC?";
        case AtCommand::ScaniBeacon:
            return "AT+DISI?";
        // GATT Discovery
        case AtCommand::FindAllServicesUUID:
            return "AT+FINDALLSERVICES?";
        case AtCommand::FindAllCharacteristicUUID:
            return "AT+FINDALLCHARS?";
        case AtCommand::FindCharacteristicUUID:
            return "AT+FINDCHAR";
    }
    return ""; // unreachable
}

constexpr const char* OK_GET = "OK+GET:";
constexpr const char* OK_SET = "OK+SET:";

/**
 * @brief Runtime builder for AT command strings using fixed-size buffers.
 *
 * This class provides a simple runtime API for building AT commands without
 * heap allocation. Uses fixed-size ATCommandBuffer internally for embedded
 * systems and real-time requirements.
 */
class ATCommandBuilder {
  public:
    /**
     * @brief Create a new command buffer initialized with a command base.
     *
     * @param cmd The AT command identifier.
     * @return ATCommandBuffer<128> initialized with the command base.
     *
     * @par Example
     * @code
     * auto cmd = ATCommandBuilder::begin(AtCommand::SetRole)
     *     .append("1")
     *     .finalize();
     * uart.transmit(cmd.view());
     * @endcode
     */
    static ATCommandBuffer<128> begin(AtCommand cmd) noexcept {
        ATCommandBuffer<128> buffer;
        buffer.reset(command_base(cmd));
        return buffer;
    }

    /**
     * @brief Build an AT command from a command enum and a list of parameters.
     *
     * @param cmd The AT command identifier.
     * @param params A list of string_view parameters to append.
     * @return ATCommandBuffer<128> containing the command (not yet finalized).
     *
     * @par Example
     * @code
     * auto cmd_buf = ATCommandBuilder::build(AtCommand::SetRole, {"1"});
     * uart.transmit(cmd_buf.finalize().view());
     * @endcode
     */
    static ATCommandBuffer<128> build(AtCommand cmd,
                                     std::initializer_list<std::string_view> params) noexcept {
        auto buffer = begin(cmd);
        for (auto p : params) {
            buffer.append(p);
        }
        return buffer;
    }
};

} // namespace hm11
