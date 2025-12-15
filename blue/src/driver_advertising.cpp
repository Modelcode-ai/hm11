#include <algorithm>
#include <array>
#include <string>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/util/numeric_formatter.hpp"

namespace hm11 {

using util::NumericFormatter;

// Helper to build expected response strings (OK+SET: or OK+GET: + value)
static std::string_view build_expected(const char* prefix, std::string_view value, std::array<char, 64>& buffer) {
    std::size_t pos = 0;
    std::size_t prefix_len = std::strlen(prefix);
    std::memcpy(buffer.data(), prefix, prefix_len);
    pos += prefix_len;
    std::memcpy(buffer.data() + pos, value.data(), value.size());
    pos += value.size();
    return {buffer.data(), pos};
}

bool HM11Driver::set_advertising_interval(AdvertisingInterval interval, Status& status) {
    // Convert enum to character ('0'-'9', 'A'-'F')
    char c = 0;
    if (static_cast<int>(interval) <= 9) {
        c = static_cast<char>('0' + static_cast<int>(interval));
    } else {
        c = static_cast<char>('A' + static_cast<int>(interval) - 10);
    }
    std::string_view c_str(&c, 1);
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetAdvertisingInterval, {c_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, c_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_advertising_interval(AdvertisingInterval& interval, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetAdvertisingInterval, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse character response ('0'-'9', 'A'-'F')
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    char c = static_cast<char>(response_buffer[std::strlen(OK_GET)]);
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
    auto type_str = NumericFormatter::format(static_cast<int>(type));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetAdvertisingType, {type_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, type_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_advertising_type(AdvertisingType& type, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetAdvertisingType, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse numeric value
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    type = static_cast<AdvertisingType>(value);
    return true;
}

bool HM11Driver::set_advertising_flag(const util::AdvertisingFlag& flag, Status& status) {
    auto flag_str = flag.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetAdvertisingFlag, {flag_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, flag_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_temperature_information_byte_in_advertising(const util::AdvertisingTemperatureType& temp,
                                                                  Status& status) {
    auto temp_str = temp.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetTemperatureInfo, {"0x", temp_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view{"0x"}, expected_buf);
    // Append temp_str to expected
    std::size_t pos = expected.size();
    std::memcpy(expected_buf.data() + pos, temp_str.data(), temp_str.size());
    expected = std::string_view(expected_buf.data(), pos + temp_str.size());

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_humi_information_byte_in_advertising(const util::HumiInformation& humi, Status& status) {
    auto humi_str = humi.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetHumiInfo, {humi_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, humi_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_module_advertisement_data(const util::AdvertisementData& data, Status& status) {
    auto data_str = data.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetAdvertisementData, {data_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, data_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_reliable_advertising_mode(AdvertisingMode mode, Status& status) {
    auto mode_str = NumericFormatter::format(static_cast<int>(mode));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetReliableAdvertising, {mode_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, mode_str, expected_buf);

    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_reliable_advertising_mode(AdvertisingMode& mode, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetReliableAdvertising, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse numeric value
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    mode = static_cast<AdvertisingMode>(value);
    return true;
}

} // namespace hm11
