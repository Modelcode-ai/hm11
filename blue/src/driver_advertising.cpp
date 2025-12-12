#include <algorithm>
#include <array>
#include <string>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

bool HM11Driver::set_advertising_interval(AdvertisingInterval interval, Status& status) {
    // Convert enum to character ('0'-'9', 'A'-'F')
    char c = 0;
    if (static_cast<int>(interval) <= 9) {
        c = static_cast<char>('0' + static_cast<int>(interval));
    } else {
        c = static_cast<char>('A' + static_cast<int>(interval) - 10);
    }
    std::string cmd = ATCommandBuilder::build(AtCommand::SetAdvertisingInterval, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_advertising_interval(AdvertisingInterval& interval, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetAdvertisingInterval, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse character response ('0'-'9', 'A'-'F')
    char c = static_cast<char>(response_buffer[OK_GET.size()]);
    int value = 0;
    if (c >= '0' && c <= '9') {
        value = c - '0';
    } else if (c >= 'A' && c <= 'F') {
        value = 10 + (c - 'A');
    } else {
        status = Status::Error;
        return false;
    }
    interval = static_cast<AdvertisingInterval>(value);
    return true;
}

bool HM11Driver::set_advertising_type(AdvertisingType type, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetAdvertisingType, {std::to_string(static_cast<int>(type))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(type));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_advertising_type(AdvertisingType& type, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetAdvertisingType, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse numeric value
    int value = response_buffer[OK_GET.size()] - '0';
    type = static_cast<AdvertisingType>(value);
    return true;
}

bool HM11Driver::set_advertising_flag(const util::AdvertisingFlag& flag, Status& status) {
    std::string flag_str(flag.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetAdvertisingFlag, {flag_str});
    std::string expected = OK_SET + flag_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_temperature_information_byte_in_advertising(const util::AdvertisingTemperatureType& temp,
                                                                  Status& status) {
    std::string temp_str(temp.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetTemperatureInfo, {"0x", temp_str});
    std::string expected = OK_SET + "0x" + temp_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_humi_information_byte_in_advertising(const util::HumiInformation& humi, Status& status) {
    std::string humi_str(humi.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetHumiInfo, {humi_str});
    std::string expected = OK_SET + humi_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_module_advertisement_data(const util::AdvertisementData& data, Status& status) {
    std::string data_str(data.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetAdvertisementData, {data_str});
    std::string expected = OK_SET + data_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_reliable_advertising_mode(AdvertisingMode mode, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetReliableAdvertising, {std::to_string(static_cast<int>(mode))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(mode));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_reliable_advertising_mode(AdvertisingMode& mode, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetReliableAdvertising, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse numeric value
    int value = response_buffer[OK_GET.size()] - '0';
    mode = static_cast<AdvertisingMode>(value);
    return true;
}

} // namespace hm11
