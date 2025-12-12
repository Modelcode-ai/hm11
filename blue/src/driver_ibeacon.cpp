#include <algorithm>
#include <array>
#include <iostream>
#include <span>
#include <vector>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/ATResponseParser.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

bool HM11Driver::set_ibeacon_major(const std::array<char, 4>& version) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetiBeaconMajor, {"0x", std::string(version.data(), version.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd, OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_minor(const std::array<char, 4>& version) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetiBeaconMinor, {"0x", std::string(version.data(), version.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd, OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_measured_power(const std::array<char, 2>& power) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetiBeaconMeasuredPower, {"0x", std::string(power.data(), power.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd, OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_deploy_mode(HM11Driver::IBeaconDeployMode mode) {
    // Build command AT+DELO<V> where V = mode+1 as per Ada spec
    std::string cmd = ATCommandBuilder::build(
        AtCommand::SetiBeaconDeployMode,
        {static_cast<std::uint32_t>(static_cast<int>(mode) + 1)});
    // Use transmit_and_check to verify OK+DELO<V>
    std::string expected = "OK+DELO" + std::to_string(static_cast<int>(mode) + 1);
    Status status = Status::Ok;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_switch(bool enable) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetiBeaconSwitch, {enable ? 1U : 0U});
    std::string expected = std::string("OK+IBEA") + (enable ? "1" : "0");
    Status status = Status::Ok;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_ibeacon_switch(bool& enabled) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetiBeaconSwitch, {});
    // Expected response format: OK+IBEA=1 or =0
    Status status = Status::Ok;
    // Send command
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }
    // Receive response into last_command via receive simulation
    std::string response;
    receive(response, status);
    if (status != Status::Ok) {
        return false;
    }
    // Parse response
    if (response.starts_with("OK+IBEA=")) {
        enabled = (response.back() == '1');
        return true;
    }
    return false;
}

bool HM11Driver::get_ibeacon_uuid_chunk(std::size_t pos, std::array<char, 32>& uuid_out) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetiBeaconUUID, {pos, "?"});
    Status status = Status::Ok;
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    auto* start = response_buffer.begin() + OK_GET.size() + 2;
    std::ranges::copy(start, start + 8, uuid_out.begin() + (pos * 8));
    return true;
}

bool HM11Driver::get_ibeacon_uuid(std::array<char, 32>& uuid_out) {
    for (std::size_t pos = 0; pos < 4; ++pos) {
        if (!get_ibeacon_uuid_chunk(pos, uuid_out)) {
            return false;
        }
    }
    return true;
}

} // namespace hm11
