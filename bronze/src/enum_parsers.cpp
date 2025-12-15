/**
 * @file enum_parsers.cpp
 * @brief Implementation of parsing functions for HM11 enumeration types
 *
 * This file implements the parsing functions for HM11 driver enumeration types.
 * These functions convert strings to enum values for parsing responses from
 * the HM11 module.
 */

#include "hm11/enum_parsers.hpp"

#include <algorithm>

namespace hm11 {

std::optional<Role> parse_role(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return Role::Peripheral;
        case '1':
            return Role::Central;
        default:
            return std::nullopt;
    }
}

std::optional<AdvertisingInterval> parse_advertising_interval(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return AdvertisingInterval::Ms100;
        case '1':
            return AdvertisingInterval::Ms211;
        case '2':
            return AdvertisingInterval::Ms252;
        case '3':
            return AdvertisingInterval::Ms318;
        case '4':
            return AdvertisingInterval::Ms417;
        case '5':
            return AdvertisingInterval::Ms546;
        case '6':
            return AdvertisingInterval::Ms760;
        case '7':
            return AdvertisingInterval::Ms852;
        case '8':
            return AdvertisingInterval::Ms1022;
        case '9':
            return AdvertisingInterval::Ms1285;
        case 'A':
        case 'a':
            return AdvertisingInterval::Ms2000;
        case 'B':
        case 'b':
            return AdvertisingInterval::Ms3000;
        case 'C':
        case 'c':
            return AdvertisingInterval::Ms4000;
        case 'D':
        case 'd':
            return AdvertisingInterval::Ms5000;
        case 'E':
        case 'e':
            return AdvertisingInterval::Ms6000;
        case 'F':
        case 'f':
            return AdvertisingInterval::Ms7000;
        default:
            return std::nullopt;
    }
}

std::optional<AdvertisingType> parse_advertising_type(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return AdvertisingType::AdvertisingScanResponseConnectable;
        case '1':
            return AdvertisingType::LastDeviceConnect;
        case '2':
            return AdvertisingType::AdvertisingScanResponse;
        case '3':
            return AdvertisingType::Advertising;
        default:
            return std::nullopt;
    }
}

std::optional<AdvertisingMode> parse_advertising_mode(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return AdvertisingMode::NormalAdvertising;
        case '1':
            return AdvertisingMode::ReliableAdvertising;
        default:
            return std::nullopt;
    }
}

std::optional<MacAddressType> parse_mac_address_type(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case 'N':
        case 'n':
            return MacAddressType::NormalAddress;
        case '0':
            return MacAddressType::StaticMac;
        case '1':
            return MacAddressType::StaticRandomMac;
        case '2':
            return MacAddressType::RandomMac;
        default:
            return std::nullopt;
    }
}

std::optional<WriteMethod> parse_write_method(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return WriteMethod::WriteWithoutResponse;
        case '1':
            return WriteMethod::Write;
        default:
            return std::nullopt;
    }
}

std::optional<UartBaudRate> parse_uart_baud_rate(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return UartBaudRate::Br9600;
        case '1':
            return UartBaudRate::Br19200;
        case '2':
            return UartBaudRate::Br38400;
        case '3':
            return UartBaudRate::Br57600;
        case '4':
            return UartBaudRate::Br115200;
        case '5':
            return UartBaudRate::Br4800;
        case '6':
            return UartBaudRate::Br2400;
        case '7':
            return UartBaudRate::Br1200;
        case '8':
            return UartBaudRate::Br230400;
        default:
            return std::nullopt;
    }
}

std::optional<LinkLayerConnectionInterval> parse_link_layer_connection_interval(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return LinkLayerConnectionInterval::Ms7;
        case '1':
            return LinkLayerConnectionInterval::Ms10;
        case '2':
            return LinkLayerConnectionInterval::Ms15;
        case '3':
            return LinkLayerConnectionInterval::Ms20;
        case '4':
            return LinkLayerConnectionInterval::Ms25;
        case '5':
            return LinkLayerConnectionInterval::Ms30;
        case '6':
            return LinkLayerConnectionInterval::Ms35;
        case '7':
            return LinkLayerConnectionInterval::Ms40;
        case '8':
            return LinkLayerConnectionInterval::Ms45;
        case '9':
            return LinkLayerConnectionInterval::Ms4000;
        default:
            return std::nullopt;
    }
}

std::optional<ConnectionSupervisionTimeout> parse_connection_supervision_timeout(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return ConnectionSupervisionTimeout::Ms100;
        case '1':
            return ConnectionSupervisionTimeout::Ms1000;
        case '2':
            return ConnectionSupervisionTimeout::Ms2000;
        case '3':
            return ConnectionSupervisionTimeout::Ms3000;
        case '4':
            return ConnectionSupervisionTimeout::Ms4000;
        case '5':
            return ConnectionSupervisionTimeout::Ms5000;
        case '6':
            return ConnectionSupervisionTimeout::Ms6000;
        default:
            return std::nullopt;
    }
}

std::optional<ConnectResult> parse_connect_result(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    char c = str[0];

    // Handle the special case for Connected which might be "OK" or a digit
    if (c >= '0' && c <= '5') {
        return ConnectResult::Connected;
    }

    // Match the first character for other cases
    switch (c) {
        case 'L':
        case 'l':
            return ConnectResult::Connecting;
        case 'E':
        case 'e':
            return ConnectResult::ConnectError;
        case 'F':
        case 'f':
            return ConnectResult::ConnectFail;
        case 'N':
        case 'n':
            return ConnectResult::NoAddress;
        case 'O': // Part of "OK"
            if (str.size() > 1 && (str[1] == 'K' || str[1] == 'k')) {
                return ConnectResult::Connected;
            }
            return ConnectResult::OtherError;
        default:
            return ConnectResult::OtherError;
    }
}

std::optional<IBeaconDeployMode> parse_ibeacon_deploy_mode(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return IBeaconDeployMode::BroadcastScanning;
        case '1':
            return IBeaconDeployMode::OnlyBroadcast;
        default:
            return std::nullopt;
    }
}

std::optional<NotifyResponse> parse_notify_response(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    if (str.size() >= 2) {
        if ((str[0] == 'O' || str[0] == 'o') && (str[1] == 'K' || str[1] == 'k')) {
            return NotifyResponse::SendOk;
        }

        if ((str[0] == 'E' || str[0] == 'e') && (str[1] == 'R' || str[1] == 'r')) {
            return NotifyResponse::SendEr;
        }

        if ((str[0] == 'D' || str[0] == 'd') && (str[1] == 'E' || str[1] == 'e')) {
            return NotifyResponse::DataEr;
        }
    }

    return std::nullopt;
}

std::optional<SendDataMethod> parse_send_data_method(std::string_view str) {
    if (str.size() < 2) {
        return std::nullopt;
    }

    char c1 = str[0];
    char c2 = str[1];

    if ((c1 == 'W' || c1 == 'w') && (c2 == 'R' || c2 == 'r')) {
        return SendDataMethod::Write;
    }

    if ((c1 == 'W' || c1 == 'w') && (c2 == 'N' || c2 == 'n')) {
        return SendDataMethod::WriteWithoutResponse;
    }

    if ((c1 == 'I' || c1 == 'i') && (c2 == 'N' || c2 == 'n')) {
        return SendDataMethod::Indicate;
    }

    if ((c1 == 'N' || c1 == 'n') && (c2 == 'O' || c2 == 'o')) {
        return SendDataMethod::Notify;
    }

    return std::nullopt;
}

std::optional<SendDataCharacteristic> parse_send_data_characteristic(std::string_view str) {
    if (str.size() < 2) {
        return std::nullopt;
    }

    char c1 = str[0];
    char c2 = str[1];

    if ((c1 == 'W' || c1 == 'w') && (c2 == 'R' || c2 == 'r')) {
        return SendDataCharacteristic::Write;
    }

    if ((c1 == 'W' || c1 == 'w') && (c2 == 'N' || c2 == 'n')) {
        return SendDataCharacteristic::WriteWithoutResponse;
    }

    return std::nullopt;
}

std::optional<CharacteristicUuidCount> parse_characteristic_uuid_count(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '?':
            return CharacteristicUuidCount::Query;
        case '0':
            return CharacteristicUuidCount::One;
        case '1':
            return CharacteristicUuidCount::OneAndNext;
        case '2':
            return CharacteristicUuidCount::OneAndPrev;
        default:
            return std::nullopt;
    }
}

std::optional<RxGain> parse_rx_gain(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return RxGain::No;
        case '1':
            return RxGain::Open;
        default:
            return std::nullopt;
    }
}

std::optional<WorkType> parse_work_type(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return WorkType::StartImmediately;
        case '1':
            return WorkType::RespondAtCommand;
        default:
            return std::nullopt;
    }
}

std::optional<WorkMode> parse_work_mode(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return WorkMode::TransmissionMode;
        case '1':
            return WorkMode::PioCollectionMode;
        case '2':
            return WorkMode::RemoteControlMode;
        default:
            return std::nullopt;
    }
}

std::optional<NotifyMode> parse_notify_mode(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return NotifyMode::WithoutAddress;
        case '1':
            return NotifyMode::WithAddress;
        default:
            return std::nullopt;
    }
}

std::optional<OutputPower> parse_output_power(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return OutputPower::NormalPower;
        case '1':
            return OutputPower::MaxPower;
        default:
            return std::nullopt;
    }
}

std::optional<ParityBit> parse_parity_bit(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return ParityBit::None;
        case '1':
            return ParityBit::Odd;
        case '2':
            return ParityBit::Even;
        default:
            return std::nullopt;
    }
}

std::optional<PioOutput> parse_pio_output(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return PioOutput::Low;
        case '1':
            return PioOutput::High;
        default:
            return std::nullopt;
    }
}

std::optional<ModulePower> parse_module_power(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return ModulePower::DbmMinus23;
        case '1':
            return ModulePower::DbmMinus6;
        case '2':
            return ModulePower::Dbm0;
        case '3':
            return ModulePower::Dbm6;
        default:
            return std::nullopt;
    }
}

std::optional<StopBit> parse_stop_bit(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return StopBit::OneStopBit;
        case '1':
            return StopBit::TwoStopBit;
        default:
            return std::nullopt;
    }
}

std::optional<SensorType> parse_sensor_type(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return SensorType::None;
        case '1':
            return SensorType::Dht11;
        case '2':
            return SensorType::Ds18b20;
        default:
            return std::nullopt;
    }
}

std::optional<ShowDeviceInformation> parse_show_device_information(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return ShowDeviceInformation::DontShow;
        case '1':
            return ShowDeviceInformation::ShowName;
        case '2':
            return ShowDeviceInformation::ShowRssi;
        case '3':
            return ShowDeviceInformation::ShowRssiAndName;
        default:
            return std::nullopt;
    }
}

std::optional<BondMode> parse_bond_mode(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return BondMode::NotNeedPin;
        case '1':
            return BondMode::AuthNotNeedPin;
        case '2':
            return BondMode::AuthWithPin;
        case '3':
            return BondMode::AuthAndBonded;
        default:
            return std::nullopt;
    }
}

std::optional<UartSleepType> parse_uart_sleep_type(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    switch (str[0]) {
        case '0':
            return UartSleepType::CanWakeUpThroughUart;
        case '1':
            return UartSleepType::ShutdownUart;
        default:
            return std::nullopt;
    }
}

// ============================================================================
// Character Conversion Functions
// ============================================================================

char to_char(AdvertisingInterval interval) {
    switch (interval) {
        case AdvertisingInterval::Ms100:  return '0';
        case AdvertisingInterval::Ms211:  return '1';
        case AdvertisingInterval::Ms252:  return '2';
        case AdvertisingInterval::Ms318:  return '3';
        case AdvertisingInterval::Ms417:  return '4';
        case AdvertisingInterval::Ms546:  return '5';
        case AdvertisingInterval::Ms760:  return '6';
        case AdvertisingInterval::Ms852:  return '7';
        case AdvertisingInterval::Ms1022: return '8';
        case AdvertisingInterval::Ms1285: return '9';
        case AdvertisingInterval::Ms2000: return 'A';
        case AdvertisingInterval::Ms3000: return 'B';
        case AdvertisingInterval::Ms4000: return 'C';
        case AdvertisingInterval::Ms5000: return 'D';
        case AdvertisingInterval::Ms6000: return 'E';
        case AdvertisingInterval::Ms7000: return 'F';
        default: return '0';
    }
}

AdvertisingInterval from_char(char c, [[maybe_unused]] AdvertisingInterval* tag) {
    switch (c) {
        case '0': return AdvertisingInterval::Ms100;
        case '1': return AdvertisingInterval::Ms211;
        case '2': return AdvertisingInterval::Ms252;
        case '3': return AdvertisingInterval::Ms318;
        case '4': return AdvertisingInterval::Ms417;
        case '5': return AdvertisingInterval::Ms546;
        case '6': return AdvertisingInterval::Ms760;
        case '7': return AdvertisingInterval::Ms852;
        case '8': return AdvertisingInterval::Ms1022;
        case '9': return AdvertisingInterval::Ms1285;
        case 'A': case 'a': return AdvertisingInterval::Ms2000;
        case 'B': case 'b': return AdvertisingInterval::Ms3000;
        case 'C': case 'c': return AdvertisingInterval::Ms4000;
        case 'D': case 'd': return AdvertisingInterval::Ms5000;
        case 'E': case 'e': return AdvertisingInterval::Ms6000;
        case 'F': case 'f': return AdvertisingInterval::Ms7000;
        default: return AdvertisingInterval::Ms100;
    }
}

char to_char(AdvertisingType type) {
    return static_cast<char>('0' + static_cast<int>(type));
}

AdvertisingType from_char(char c, [[maybe_unused]] AdvertisingType* tag) {
    if (c >= '0' && c <= '3') {
        return static_cast<AdvertisingType>(c - '0');
    }
    return AdvertisingType::AdvertisingScanResponseConnectable;
}

char to_char(AdvertisingMode mode) {
    return static_cast<char>('0' + static_cast<int>(mode));
}

AdvertisingMode from_char(char c, [[maybe_unused]] AdvertisingMode* tag) {
    if (c >= '0' && c <= '1') {
        return static_cast<AdvertisingMode>(c - '0');
    }
    return AdvertisingMode::NormalAdvertising;
}

char to_char(WorkMode mode) {
    return static_cast<char>('0' + static_cast<int>(mode));
}

WorkMode from_char(char c, [[maybe_unused]] WorkMode* tag) {
    if (c >= '0' && c <= '2') {
        return static_cast<WorkMode>(c - '0');
    }
    return WorkMode::TransmissionMode;
}

char to_char(WorkType type) {
    return static_cast<char>('0' + static_cast<int>(type));
}

WorkType from_char(char c, [[maybe_unused]] WorkType* tag) {
    if (c >= '0' && c <= '1') {
        return static_cast<WorkType>(c - '0');
    }
    return WorkType::StartImmediately;
}

char to_char(NotifyMode mode) {
    return static_cast<char>('0' + static_cast<int>(mode));
}

NotifyMode from_char(char c, [[maybe_unused]] NotifyMode* tag) {
    if (c >= '0' && c <= '1') {
        return static_cast<NotifyMode>(c - '0');
    }
    return NotifyMode::WithoutAddress;
}

} // namespace hm11