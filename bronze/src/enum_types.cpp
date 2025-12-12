/**
 * @file enum_types.cpp
 * @brief Implementation of string conversion functions for HM11 enumeration types
 *
 * This file implements the string conversion functions for HM11 driver enumeration types.
 * These functions convert enum values to strings for use in AT commands and debugging.
 */

#include "hm11/enum_types.hpp"

namespace hm11 {

std::string to_string(Role value) {
    switch (value) {
        case Role::Peripheral:
            return "0";
        case Role::Central:
            return "1";
        default:
            return "0"; // Default to Peripheral
    }
}

std::string to_string(AdvertisingInterval value) {
    switch (value) {
        case AdvertisingInterval::Ms100:
            return "0";
        case AdvertisingInterval::Ms211:
            return "1";
        case AdvertisingInterval::Ms252:
            return "2";
        case AdvertisingInterval::Ms318:
            return "3";
        case AdvertisingInterval::Ms417:
            return "4";
        case AdvertisingInterval::Ms546:
            return "5";
        case AdvertisingInterval::Ms760:
            return "6";
        case AdvertisingInterval::Ms852:
            return "7";
        case AdvertisingInterval::Ms1022:
            return "8";
        case AdvertisingInterval::Ms1285:
            return "9";
        case AdvertisingInterval::Ms2000:
            return "A";
        case AdvertisingInterval::Ms3000:
            return "B";
        case AdvertisingInterval::Ms4000:
            return "C";
        case AdvertisingInterval::Ms5000:
            return "D";
        case AdvertisingInterval::Ms6000:
            return "E";
        case AdvertisingInterval::Ms7000:
            return "F";
        default:
            return "0"; // Default to 100ms
    }
}

std::string to_string(AdvertisingType value) {
    switch (value) {
        case AdvertisingType::AdvertisingScanResponseConnectable:
            return "0";
        case AdvertisingType::LastDeviceConnect:
            return "1";
        case AdvertisingType::AdvertisingScanResponse:
            return "2";
        case AdvertisingType::Advertising:
            return "3";
        default:
            return "0"; // Default to AdvertisingScanResponseConnectable
    }
}

std::string to_string(AdvertisingMode value) {
    switch (value) {
        case AdvertisingMode::NormalAdvertising:
            return "0";
        case AdvertisingMode::ReliableAdvertising:
            return "1";
        default:
            return "0"; // Default to NormalAdvertising
    }
}

std::string to_string(MacAddressType value) {
    switch (value) {
        case MacAddressType::NormalAddress:
            return "N";
        case MacAddressType::StaticMac:
            return "0";
        case MacAddressType::StaticRandomMac:
            return "1";
        case MacAddressType::RandomMac:
            return "2";
        default:
            return "N"; // Default to NormalAddress
    }
}

std::string to_string(WriteMethod value) {
    switch (value) {
        case WriteMethod::Write:
            return "1";
        case WriteMethod::WriteWithoutResponse:
            return "0";
        default:
            return "1"; // Default to Write
    }
}

std::string to_string(UartBaudRate value) {
    switch (value) {
        case UartBaudRate::Br9600:
            return "0";
        case UartBaudRate::Br19200:
            return "1";
        case UartBaudRate::Br38400:
            return "2";
        case UartBaudRate::Br57600:
            return "3";
        case UartBaudRate::Br115200:
            return "4";
        case UartBaudRate::Br4800:
            return "5";
        case UartBaudRate::Br2400:
            return "6";
        case UartBaudRate::Br1200:
            return "7";
        case UartBaudRate::Br230400:
            return "8";
        default:
            return "0"; // Default to 9600
    }
}

std::string to_string(LinkLayerConnectionInterval value) {
    switch (value) {
        case LinkLayerConnectionInterval::Ms7:
            return "0";
        case LinkLayerConnectionInterval::Ms10:
            return "1";
        case LinkLayerConnectionInterval::Ms15:
            return "2";
        case LinkLayerConnectionInterval::Ms20:
            return "3";
        case LinkLayerConnectionInterval::Ms25:
            return "4";
        case LinkLayerConnectionInterval::Ms30:
            return "5";
        case LinkLayerConnectionInterval::Ms35:
            return "6";
        case LinkLayerConnectionInterval::Ms40:
            return "7";
        case LinkLayerConnectionInterval::Ms45:
            return "8";
        case LinkLayerConnectionInterval::Ms4000:
            return "9";
        default:
            return "3"; // Default to 20ms
    }
}

std::string to_string(ConnectionSupervisionTimeout value) {
    switch (value) {
        case ConnectionSupervisionTimeout::Ms100:
            return "0";
        case ConnectionSupervisionTimeout::Ms1000:
            return "1";
        case ConnectionSupervisionTimeout::Ms2000:
            return "2";
        case ConnectionSupervisionTimeout::Ms3000:
            return "3";
        case ConnectionSupervisionTimeout::Ms4000:
            return "4";
        case ConnectionSupervisionTimeout::Ms5000:
            return "5";
        case ConnectionSupervisionTimeout::Ms6000:
            // Fall through to default
        default:
            return "6"; // Default to 6000ms
    }
}

std::string to_string(ConnectResult value) {
    switch (value) {
        case ConnectResult::Connecting:
            return "L";
        case ConnectResult::Connected:
            return "OK";
        case ConnectResult::ConnectError:
            return "E";
        case ConnectResult::ConnectFail:
            return "F";
        case ConnectResult::NoAddress:
            return "N";
        case ConnectResult::OtherError:
            // Fall through to default
        default:
            return "X"; // Default to OtherError
    }
}

std::string to_string(IBeaconDeployMode value) {
    switch (value) {
        case IBeaconDeployMode::BroadcastScanning:
            return "0";
        case IBeaconDeployMode::OnlyBroadcast:
            return "1";
        default:
            return "0"; // Default to BroadcastScanning
    }
}

std::string to_string(NotifyResponse value) {
    switch (value) {
        case NotifyResponse::SendOk:
            return "OK";
        case NotifyResponse::SendEr:
            return "ER";
        case NotifyResponse::DataEr:
            return "DE";
        default:
            return "ER"; // Default to SendEr
    }
}

std::string to_string(SendDataMethod value) {
    switch (value) {
        case SendDataMethod::Write:
            return "WR";
        case SendDataMethod::WriteWithoutResponse:
            return "WN";
        case SendDataMethod::Indicate:
            return "IN";
        case SendDataMethod::Notify:
            return "NO";
        default:
            return "WR"; // Default to Write
    }
}

std::string to_string(SendDataCharacteristic value) {
    switch (value) {
        case SendDataCharacteristic::Write:
            return "WR";
        case SendDataCharacteristic::WriteWithoutResponse:
            return "WN";
        default:
            return "WR"; // Default to Write
    }
}

std::string to_string(CharacteristicUuidCount value) {
    switch (value) {
        case CharacteristicUuidCount::Query:
            return "?";
        case CharacteristicUuidCount::One:
            return "0";
        case CharacteristicUuidCount::OneAndNext:
            return "1";
        case CharacteristicUuidCount::OneAndPrev:
            return "2";
        default:
            return "0"; // Default to One
    }
}

std::string to_string(RxGain value) {
    switch (value) {
        case RxGain::No:
            return "0";
        case RxGain::Open:
            return "1";
        default:
            return "0"; // Default to No
    }
}

std::string to_string(WorkType value) {
    switch (value) {
        case WorkType::StartImmediately:
            return "0";
        case WorkType::RespondAtCommand:
            return "1";
        default:
            return "0"; // Default to StartImmediately
    }
}

std::string to_string(WorkMode value) {
    switch (value) {
        case WorkMode::TransmissionMode:
            return "0";
        case WorkMode::PioCollectionMode:
            return "1";
        case WorkMode::RemoteControlMode:
            return "2";
        default:
            return "0"; // Default to TransmissionMode
    }
}

std::string to_string(NotifyMode value) {
    switch (value) {
        case NotifyMode::WithoutAddress:
            return "0";
        case NotifyMode::WithAddress:
            return "1";
        default:
            return "0"; // Default to WithoutAddress
    }
}

std::string to_string(OutputPower value) {
    switch (value) {
        case OutputPower::NormalPower:
            return "0";
        case OutputPower::MaxPower:
            // Fall through to default
        default:
            return "1"; // Default to MaxPower
    }
}

std::string to_string(ParityBit value) {
    switch (value) {
        case ParityBit::None:
            return "0";
        case ParityBit::Odd:
            return "1";
        case ParityBit::Even:
            return "2";
        default:
            return "0"; // Default to None
    }
}

std::string to_string(PioOutput value) {
    switch (value) {
        case PioOutput::Low:
            return "0";
        case PioOutput::High:
            return "1";
        default:
            return "0"; // Default to Low
    }
}

std::string to_string(ModulePower value) {
    switch (value) {
        case ModulePower::DbmMinus23:
            return "0";
        case ModulePower::DbmMinus6:
            return "1";
        case ModulePower::Dbm0:
            return "2";
        case ModulePower::Dbm6:
            return "3";
        default:
            return "2"; // Default to 0 dBm
    }
}

std::string to_string(StopBit value) {
    switch (value) {
        case StopBit::OneStopBit:
            return "0";
        case StopBit::TwoStopBit:
            return "1";
        default:
            return "0"; // Default to OneStopBit
    }
}

std::string to_string(SensorType value) {
    switch (value) {
        case SensorType::None:
            return "0";
        case SensorType::Dht11:
            return "1";
        case SensorType::Ds18b20:
            return "2";
        default:
            return "0"; // Default to None
    }
}

std::string to_string(ShowDeviceInformation value) {
    switch (value) {
        case ShowDeviceInformation::DontShow:
            return "0";
        case ShowDeviceInformation::ShowName:
            return "1";
        case ShowDeviceInformation::ShowRssi:
            return "2";
        case ShowDeviceInformation::ShowRssiAndName:
            return "3";
        default:
            return "0"; // Default to DontShow
    }
}

std::string to_string(BondMode value) {
    switch (value) {
        case BondMode::NotNeedPin:
            return "0";
        case BondMode::AuthNotNeedPin:
            return "1";
        case BondMode::AuthWithPin:
            return "2";
        case BondMode::AuthAndBonded:
            // Fall through to default
        default:
            return "3"; // Default to AuthAndBonded
    }
}

std::string to_string(UartSleepType value) {
    switch (value) {
        case UartSleepType::CanWakeUpThroughUart:
            return "0";
        case UartSleepType::ShutdownUart:
            return "1";
        default:
            return "0"; // Default to CanWakeUpThroughUart
    }
}

std::string to_string(ScanStageKind value) {
    switch (value) {
        case ScanStageKind::Selection:
            return "Selection";
        case ScanStageKind::MAC:
            return "MAC";
        case ScanStageKind::Name:
            return "Name";
        case ScanStageKind::RSSI:
            return "RSSI";
        default:
            return "Unknown"; // Should never happen
    }
}

} // namespace hm11