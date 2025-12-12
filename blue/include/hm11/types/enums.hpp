#pragma once

#include <cstdint>

namespace hm11 {
    // Role of the device
    enum class Role : uint8_t {
        Peripheral = 0,
        Central
    };

    // UART baud rates
    enum class BaudRate : uint8_t {
        BR9600 = 0,
        BR19200,
        BR38400,
        BR57600,
        BR115200,
        BR4800,
        BR2400,
        BR1200,
        BR230400
    };

    // Advertising interval values (ms)
    enum class AdvertisingInterval : uint8_t {
        ms100 = 0,
        ms211,
        ms252,
        ms318,
        ms417,
        ms546,
        ms760,
        ms852,
        ms1022,
        ms1285,
        ms2000,
        ms3000,
        ms4000,
        ms5000,
        ms6000,
        ms7000
    };

    // Advertising type
    enum class AdvertisingType : uint8_t {
        AdvertisingScanResponseConnectable = 0,
        LastDeviceConnect,
        AdvertisingScanResponse,
        Advertising
    };

    // Advertising mode
    enum class AdvertisingMode : uint8_t {
        NormalAdvertising = 0,
        ReliableAdvertising
    };

    // MAC address type
    enum class MACAddressType : uint8_t {
        NormalAddress = 0,
        StaticMAC,
        StaticRandomMAC,
        RandomMAC
    };

    // Write method for characteristics
    enum class WriteMethod : uint8_t {
        Write = 0,
        WriteWithoutResponse
    };

    // Link layer connection interval (ms)
    enum class LinkLayerConnectionInterval : uint8_t {
        ms7 = 0,
        ms_10,
        ms15,
        ms20,
        ms25,
        ms30,
        ms35,
        ms40,
        ms45,
        ms4000
    };

    // Connection supervision timeout (ms)
    enum class ConnectionSupervisionTimeout : uint8_t {
        ms100 = 0,
        ms1000,
        ms2000,
        ms3000,
        ms4000,
        ms5000,
        ms6000
    };

    // Result of a connect attempt
    enum class ConnectResult : uint8_t {
        Connecting = 0,
        Connected,
        ConnectError,
        ConnectFail,
        NoAddress,
        OtherError
    };

    // Method for sending data
    enum class SendDataMethod : uint8_t {
        Write = 0,
        WriteWithoutResponse,
        Indicate,
        Notify
    };

    // Characteristic send method
    enum class SendDataCharacteristic : uint8_t {
        Write = 0,
        WriteWithoutResponse
    };

    // UUID query count
    enum class CharacteristicUUIDCount : uint8_t {
        Query = 0,
        One,
        OneAndNext,
        OneAndPrev
    };

    // RX gain settings
    enum class RXGain : uint8_t {
        No = 0,
        Open
    };

    // Work type for the module
    enum class WorkType : uint8_t {
        StartImmediately = 0,
        RespondATCommand
    };

    // Work mode
    enum class WorkMode : uint8_t {
        TransmissionMode = 0,
        PIOCollectionMode,
        RemoteControlMode
    };

    // Notify mode
    enum class NotifyMode : uint8_t {
        Without_Address = 0,
        WithAddress
    };

    // Output power settings
    enum class OutputPower : uint8_t {
        NormalPower = 0,
        MaxPower
    };

    // Parity bit configuration
    enum class ParityBit : uint8_t {
        None = 0,
        Odd,
        Even
    };

    // PIO output state
    enum class PIOOutput : uint8_t {
        Low = 0,
        High
    };

    // Module power levels
    enum class ModulePower : uint8_t {
        DbmMinus23 = 0,
        DbmMinus6,
        Dbm0,
        Dbm6
    };

    // Stop bit configuration
    enum class StopBit : uint8_t {
        OneStopBit = 0,
        TwoStopBit
    };

    // Device information display options during discovery
    enum class ShowDeviceInformation : uint8_t {
        DontShow = 0,
        ShowName,
        ShowRSSI,
        ShowRSSIAndName
    };

    // Bond mode settings
    enum class BondMode : uint8_t {
        NotNeedPIN = 0,
        AuthNotNeedPIN,
        AuthWithPIN,
        AuthAndBonded
    };

    // UART sleep type
    enum class UARTSleepType : uint8_t {
        CanWakeUpThroughUART = 0,
        ShutdownUART
    };

    // Notify response status
    enum class NotifyResponse : uint8_t {
        SendOk = 0,
        SendEr,
        DataEr
    };
} // namespace hm11
