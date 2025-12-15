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
#include "hm11/util/string_parser.hpp"

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

bool HM11Driver::set_sensor_type(SensorType value) {
    // Build AT+SENS command with numeric sensor type
    auto value_str = NumericFormatter::format(static_cast<int>(value));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetSensorType, {value_str});

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, value_str, expected_buf);

    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_sensor_type(SensorType& result) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSensorType, {});
    Status status = Status::Ok;
    // Use the overload that fills response_buffer
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // The value character follows the OK_GET prefix
    std::size_t ok_get_len = std::strlen(OK_GET);
    std::string_view val_view(reinterpret_cast<const char*>(response_buffer.data() + ok_get_len),
                              response_buffer.size() - ok_get_len);
    unsigned int val = 0;
    if (!util::parse_decimal(val_view, val)) {
        status = Status::InvalidResponse;
        return false;
    }
    result = static_cast<SensorType>(static_cast<int>(val));
    return true;
}

bool HM11Driver::set_sensor_work_interval(const WorkInterval& interval) {
    // The command is AT+RAT<value>
    // Build the interval value as a string_view
    std::string_view interval_str;
    char single_char;
    if (interval[0] == '0') {
        single_char = interval[1];
        interval_str = std::string_view(&single_char, 1);
    } else {
        interval_str = std::string_view(interval.begin(), 2);
    }

    auto cmd_buf = ATCommandBuffer<128>();
    cmd_buf.reset("AT+RAT").append(interval_str);

    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, interval_str, expected_buf);

    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_sensor_work_interval(WorkInterval& interval) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSensorWorkInterval, {});
    Status status = Status::Ok;
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    const std::size_t BASE = std::strlen(OK_GET);
    if (response_buffer[BASE] == 0) {
        interval[0] = '0';
        interval[1] = static_cast<char>(response_buffer[BASE + 1]);
    } else {
        interval[0] = static_cast<char>(response_buffer[BASE]);
        interval[1] = static_cast<char>(response_buffer[BASE + 1]);
    }
    return true;
}

bool HM11Driver::get_sensor_temperature_and_humidity(util::Temperature& temperature, util::Percent& humidity) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSensorTempHumidity, {});
    Status status = Status::Ok;
    // Expect OK_GET plus 6 characters (3 for temperature, 3 for humidity)
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    const std::size_t BASE = std::strlen(OK_GET);
    // Extract temperature and humidity as string_view (3 chars each)
    std::string_view t_view(reinterpret_cast<const char*>(response_buffer.data() + BASE), 3);
    std::string_view h_view(reinterpret_cast<const char*>(response_buffer.data() + BASE + 3), 3);
    unsigned int t_val = 0;
    unsigned int h_val = 0;
    if (!util::parse_decimal(t_view, t_val) || !util::parse_decimal(h_view, h_val)) {
        status = Status::InvalidResponse;
        return false;
    }
    temperature = util::Temperature(static_cast<std::uint8_t>(t_val));
    humidity = util::Percent(static_cast<std::uint8_t>(h_val));
    return true;
}

} // namespace hm11
