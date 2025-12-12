#include <algorithm>
#include <string>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

// ========== White List Methods ==========

bool HM11Driver::set_white_list_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    std::string cmd = ATCommandBuilder::build(AtCommand::SetWhiteListSwitch, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_white_list_switch(bool& enabled, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetWhiteListSwitch, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    enabled = (response_buffer[OK_GET.size()] != '0');
    return true;
}

bool HM11Driver::set_white_list_mac_addresses(util::MacWhiteListIndex index, const util::MACAddress& mac,
                                                Status& status) {
    std::string index_str = std::to_string(static_cast<int>(index));
    std::string mac_str(mac.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetWhiteListMAC, {index_str, mac_str});
    std::string expected = OK_SET + mac_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_white_list_mac_address(util::MacWhiteListIndex index, util::MACAddress& mac, Status& status) {
    std::string index_str = std::to_string(static_cast<int>(index));
    std::string cmd = ATCommandBuilder::build(AtCommand::GetWhiteListMAC, {index_str, "??"});
    std::string expected = "OK+AD" + index_str + "?:";
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy MAC address from response
    std::string mac_str(12, '\0');
    std::memcpy(mac_str.data(), response_buffer.data() + expected.size(), 12);
    mac = util::MACAddress(mac_str);
    return true;
}

// ========== Battery Methods ==========

bool HM11Driver::set_battery_monitor_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    std::string cmd = ATCommandBuilder::build(AtCommand::SetBatteryMonitor, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_battery_monitor_switch(bool& enabled, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetBatteryMonitor, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    enabled = (response_buffer[OK_GET.size()] != '0');
    return true;
}

bool HM11Driver::set_battery_information(util::Percent value, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetBatteryInfo, {std::to_string(static_cast<int>(value))});
    std::string expected = "OK+BATT";
    transmit_and_check(cmd, expected, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::query_battery_information(util::Percent& value, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetBatteryInfo, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Parse 3-digit value (e.g., "100" for 100%)
    std::string val_str(response_buffer.begin() + OK_GET.size(), response_buffer.begin() + OK_GET.size() + 3);
    int val = std::stoi(val_str);
    value = util::Percent(static_cast<std::uint8_t>(val));
    return true;
}

// ========== UART Config Methods ==========

bool HM11Driver::set_uart_baud_rate(BaudRate rate, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetUARTBaudRate, {std::to_string(static_cast<int>(rate))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(rate));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_baud_rate(BaudRate& rate, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetUARTBaudRate, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    rate = static_cast<BaudRate>(value);
    return true;
}

bool HM11Driver::set_uart_flow_control_switch(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    std::string cmd = ATCommandBuilder::build(AtCommand::SetUARTFlowControl, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_flow_control_switch(bool& enabled, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetUARTFlowControl, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    enabled = (response_buffer[OK_GET.size()] != '0');
    return true;
}

bool HM11Driver::set_parity_bit(ParityBit parity, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetParityBit, {std::to_string(static_cast<int>(parity))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(parity));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_parity_bit(ParityBit& parity, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetParityBit, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    parity = static_cast<ParityBit>(value);
    return true;
}

bool HM11Driver::set_stop_bit(StopBit stop, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetStopBit, {std::to_string(static_cast<int>(stop))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(stop));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_stop_bit(StopBit& stop, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetStopBit, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    stop = static_cast<StopBit>(value);
    return true;
}

bool HM11Driver::set_uart_sleep_type(UARTSleepType type, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetUARTSleepType, {std::to_string(static_cast<int>(type))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(type));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_uart_sleep_type(UARTSleepType& type, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetUARTSleepType, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    type = static_cast<UARTSleepType>(value);
    return true;
}

// ========== Link Layer Methods ==========

bool HM11Driver::set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetMinConnInterval, {std::to_string(static_cast<int>(interval))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(interval));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetMinConnInterval, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    interval = static_cast<LinkLayerConnectionInterval>(value);
    return true;
}

bool HM11Driver::set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetMaxConnInterval, {std::to_string(static_cast<int>(interval))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(interval));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetMaxConnInterval, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    interval = static_cast<LinkLayerConnectionInterval>(value);
    return true;
}

bool HM11Driver::set_link_layer_connection_slave_latency(util::LayerConnectionLatency latency, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetSlaveLatency, {std::to_string(static_cast<int>(latency))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(latency));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_link_layer_connection_slave_latency(util::LayerConnectionLatency& latency, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSlaveLatency, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    latency = util::LayerConnectionLatency(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_connection_supervision_timeout(ConnectionSupervisionTimeout timeout, Status& status) {
    std::string cmd =
        ATCommandBuilder::build(AtCommand::SetSupervisionTimeout, {std::to_string(static_cast<int>(timeout))});
    std::string expected = OK_SET + std::to_string(static_cast<int>(timeout));
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connection_supervision_timeout(ConnectionSupervisionTimeout& timeout, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSupervisionTimeout, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    timeout = static_cast<ConnectionSupervisionTimeout>(value);
    return true;
}

bool HM11Driver::set_update_connection(bool enable, Status& status) {
    char c = enable ? '1' : '0';
    std::string cmd = ATCommandBuilder::build(AtCommand::SetUpdateConnection, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_update_connection(bool& enabled, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetUpdateConnection, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    enabled = (response_buffer[OK_GET.size()] != '0');
    return true;
}

} // namespace hm11
