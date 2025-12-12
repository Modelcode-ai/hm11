#include <algorithm>
#include <array>
#include <iostream>
#include <span>
#include <vector>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/ATResponseParser.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

bool HM11Driver::set_sensor_type(SensorType value) {
    // Build AT+SENS command with numeric sensor type
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetSensorType, static_cast<std::uint32_t>(static_cast<int>(value)));
    std::string expected = OK_SET + std::to_string(static_cast<int>(value));
    Status status = Status::Ok;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_sensor_type(SensorType& result) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSensorType, {});
    Status status = Status::Ok;
    // Use the overload that fills response_buffer
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // The value character follows the OK_GET prefix
    auto val = std::stoul(std::string(response_buffer.begin() + OK_GET.size(), response_buffer.end()));
    result = static_cast<SensorType>(static_cast<int>(val));
    return true;
}

bool HM11Driver::set_sensor_work_interval(const WorkInterval& interval) {
    // The command is AT+RAT<value>
    std::string cmd = "AT+RAT";
    std::string expected = OK_SET;
    if (interval[0] == '0') {
        std::string c(1, interval[1]);
        cmd += c;
        expected += c;
    } else {
        std::string s(interval.begin(), interval.end());
        cmd += s;
        expected += s;
    }
    cmd += "\r\n";
    Status status = Status::Ok;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_sensor_work_interval(WorkInterval& interval) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSensorWorkInterval, {});
    Status status = Status::Ok;
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    const auto BASE = OK_GET.size();
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
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSensorTempHumidity, {});
    Status status = Status::Ok;
    // Expect OK_GET plus 6 characters (3 for temperature, 3 for humidity)
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    const auto BASE = OK_GET.size();
    // Extract temperature and humidity strings (3 chars each)
    std::string t_str(response_buffer.begin() + BASE, response_buffer.begin() + BASE + 3);
    std::string h_str(response_buffer.begin() + BASE + 3, response_buffer.begin() + BASE + 6);
    int t_val = std::stoi(t_str);
    int h_val = std::stoi(h_str);
    temperature = util::Temperature(static_cast<std::uint8_t>(t_val));
    humidity = util::Percent(static_cast<std::uint8_t>(h_val));
    return true;
}

} // namespace hm11
