#include <algorithm>
#include <array>
#include <string>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
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

// ========== White List Methods ==========

bool HM11Driver::set_white_list_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetWhiteListSwitch, {std::string_view(&c, 1)});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view(&c, 1), expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_white_list_switch(bool& enabled, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetWhiteListSwitch, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    enabled = (response_buffer[std::strlen(OK_GET)] != '0');
    return true;
}

bool HM11Driver::set_white_list_mac_addresses(util::MacWhiteListIndex index, const util::MACAddress& mac,
                                                Status& status) {
    auto index_str = NumericFormatter::format(static_cast<int>(index));
    auto mac_str = mac.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetWhiteListMAC, {index_str, mac_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, mac_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_white_list_mac_address(util::MacWhiteListIndex index, util::MACAddress& mac, Status& status) {
    auto index_str = NumericFormatter::format(static_cast<int>(index));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetWhiteListMAC, {index_str, "??"});
    std::array<char, 64> expected_buf{};
    std::size_t pos = 0;
    constexpr const char* PREFIX = "OK+AD";
    std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
    pos += std::strlen(PREFIX);
    std::memcpy(expected_buf.data() + pos, index_str.data(), index_str.size());
    pos += index_str.size();
    expected_buf[pos++] = '?';
    expected_buf[pos++] = ':';
    auto expected = std::string_view(expected_buf.data(), pos);
    transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy MAC address from response
    std::array<char, 12> mac_arr{};
    std::memcpy(mac_arr.data(), response_buffer.data() + expected.size(), 12);
    mac = util::MACAddress(std::string_view(mac_arr.data(), 12));
    return true;
}

// ========== Battery Methods ==========

bool HM11Driver::set_battery_monitor_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetBatteryMonitor, {std::string_view(&c, 1)});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view(&c, 1), expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_battery_monitor_switch(bool& enabled, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetBatteryMonitor, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    enabled = (response_buffer[std::strlen(OK_GET)] != '0');
    return true;
}

bool HM11Driver::set_battery_information(util::Percent value, Status& status) {
    auto cmd_buf =
        ATCommandBuilder::build(AtCommand::SetBatteryInfo, {NumericFormatter::format(static_cast<int>(value))});
    constexpr const char* EXPECTED = "OK+BATT";
    transmit_and_check(cmd_buf.finalize(), EXPECTED, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::query_battery_information(util::Percent& value, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetBatteryInfo, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Parse 3-digit value (e.g., "100" for 100%)
    std::size_t ok_get_len = std::strlen(OK_GET);
    std::string_view val_view(reinterpret_cast<const char*>(response_buffer.data() + ok_get_len), 3);
    unsigned int val = 0;
    if (!util::parse_decimal(val_view, val)) {
        status = Status::InvalidResponse;
        return false;
    }
    value = util::Percent(static_cast<std::uint8_t>(val));
    return true;
}

// ========== UART Config Methods ==========

bool HM11Driver::set_uart_baud_rate(BaudRate rate, Status& status) {
    auto rate_str = NumericFormatter::format(static_cast<int>(rate));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetUARTBaudRate, {rate_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, rate_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_baud_rate(BaudRate& rate, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetUARTBaudRate, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    rate = static_cast<BaudRate>(value);
    return true;
}

bool HM11Driver::set_uart_flow_control_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetUARTFlowControl, {std::string_view(&c, 1)});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view(&c, 1), expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_flow_control_switch(bool& enabled, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetUARTFlowControl, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    enabled = (response_buffer[std::strlen(OK_GET)] != '0');
    return true;
}

bool HM11Driver::set_parity_bit(ParityBit parity, Status& status) {
    auto parity_str = NumericFormatter::format(static_cast<int>(parity));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetParityBit, {parity_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, parity_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_parity_bit(ParityBit& parity, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetParityBit, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    parity = static_cast<ParityBit>(value);
    return true;
}

bool HM11Driver::set_stop_bit(StopBit stop, Status& status) {
    auto stop_str = NumericFormatter::format(static_cast<int>(stop));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetStopBit, {stop_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, stop_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_stop_bit(StopBit& stop, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetStopBit, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    stop = static_cast<StopBit>(value);
    return true;
}

bool HM11Driver::set_uart_sleep_type(UARTSleepType type, Status& status) {
    auto type_str = NumericFormatter::format(static_cast<int>(type));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetUARTSleepType, {type_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, type_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_sleep_type(UARTSleepType& type, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetUARTSleepType, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    type = static_cast<UARTSleepType>(value);
    return true;
}

// ========== Link Layer Methods ==========

bool HM11Driver::set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status) {
    auto interval_str = NumericFormatter::format(static_cast<int>(interval));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetMinConnInterval, {interval_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, interval_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetMinConnInterval, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    interval = static_cast<LinkLayerConnectionInterval>(value);
    return true;
}

bool HM11Driver::set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status) {
    auto interval_str = NumericFormatter::format(static_cast<int>(interval));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetMaxConnInterval, {interval_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, interval_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetMaxConnInterval, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    interval = static_cast<LinkLayerConnectionInterval>(value);
    return true;
}

bool HM11Driver::set_link_layer_connection_slave_latency(util::LayerConnectionLatency latency, Status& status) {
    auto latency_str = NumericFormatter::format(static_cast<int>(latency));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetSlaveLatency, {latency_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, latency_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_link_layer_connection_slave_latency(util::LayerConnectionLatency& latency, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSlaveLatency, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    latency = util::LayerConnectionLatency(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_connection_supervision_timeout(ConnectionSupervisionTimeout timeout, Status& status) {
    auto timeout_str = NumericFormatter::format(static_cast<int>(timeout));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetSupervisionTimeout, {timeout_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, timeout_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connection_supervision_timeout(ConnectionSupervisionTimeout& timeout, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSupervisionTimeout, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    timeout = static_cast<ConnectionSupervisionTimeout>(value);
    return true;
}

bool HM11Driver::set_update_connection(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetUpdateConnection, {std::string_view(&c, 1)});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view(&c, 1), expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_update_connection(bool& enabled, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetUpdateConnection, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    enabled = (response_buffer[std::strlen(OK_GET)] != '0');
    return true;
}

} // namespace hm11
