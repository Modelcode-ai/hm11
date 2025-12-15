#include <algorithm>
#include <array>
#include <iostream>
#include <span>
#include <vector>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
#include "hm11/ATResponseParser.hpp"
#include "hm11/driver.hpp"
#include "hm11/util/numeric_formatter.hpp"

namespace hm11 {

using util::NumericFormatter;

// Helper to build expected response strings
static std::string_view build_expected(const char* prefix, std::string_view value, std::array<char, 64>& buffer) {
    std::size_t pos = 0;
    std::size_t prefix_len = std::strlen(prefix);
    std::memcpy(buffer.data(), prefix, prefix_len);
    pos += prefix_len;
    std::memcpy(buffer.data() + pos, value.data(), value.size());
    pos += value.size();
    return {buffer.data(), pos};
}

bool HM11Driver::set_ibeacon_major(const std::array<char, 4>& version) {
    auto cmd_buf =
        ATCommandBuilder::build(AtCommand::SetiBeaconMajor, {"0x", std::string_view(version.data(), version.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_minor(const std::array<char, 4>& version) {
    auto cmd_buf =
        ATCommandBuilder::build(AtCommand::SetiBeaconMinor, {"0x", std::string_view(version.data(), version.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_measured_power(const std::array<char, 2>& power) {
    auto cmd_buf =
        ATCommandBuilder::build(AtCommand::SetiBeaconMeasuredPower, {"0x", std::string_view(power.data(), power.size())});
    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), OK_SET, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_deploy_mode(HM11Driver::IBeaconDeployMode mode) {
    // Build command AT+DELO<V> where V = mode+1 as per Ada spec
    auto mode_str = NumericFormatter::format(static_cast<std::uint32_t>(static_cast<int>(mode) + 1));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetiBeaconDeployMode, {mode_str});

    // Build expected: "OK+DELO<V>"
    std::array<char, 64> expected_buf{};
    auto expected = build_expected("OK+DELO", mode_str, expected_buf);

    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_ibeacon_switch(bool enable) {
    auto enable_str = NumericFormatter::format(enable ? 1U : 0U);
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetiBeaconSwitch, {enable_str});

    // Build expected: "OK+IBEA0" or "OK+IBEA1"
    std::array<char, 64> expected_buf{};
    auto expected = build_expected("OK+IBEA", enable_str, expected_buf);

    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_ibeacon_switch(bool& enabled) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetiBeaconSwitch, {});
    // Expected response format: OK+IBEA=1 or =0
    Status status = Status::Ok;
    // Send command
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }
    // Receive response into internal buffer
    std::string_view response;
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
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetiBeaconUUID, {NumericFormatter::format(pos), "?"});
    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    auto* start = response_buffer.begin() + std::strlen(OK_GET) + 2;
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
