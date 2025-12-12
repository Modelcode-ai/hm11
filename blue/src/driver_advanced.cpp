#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <utility>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

// ========== Connection Methods ==========

bool HM11Driver::clear_last_connected_address(Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::ClearLastConnectedAddress, {});
    transmit_and_check(cmd, "OK+CLEAR", status);
    return status == Status::Ok;
}

bool HM11Driver::connect_last_device(ConnectResult& result, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::ConnectLastDevice, {});
    transmit_and_check(cmd, "OK+CONN", response_buffer, status);
    if (status != Status::Ok) {
        result = ConnectResult::OtherError;
        return false;
    }

    // Parse connect result from response (single character after OK+CONN)
    char c = static_cast<char>(response_buffer[7]); // "OK+CONN" is 7 characters
    switch (c) {
        case '0': result = ConnectResult::Connecting; break;
        case '1': result = ConnectResult::Connected; break;
        case '2': result = ConnectResult::ConnectError; break;
        case '3': result = ConnectResult::ConnectFail; break;
        case '4': result = ConnectResult::NoAddress; break;
        default:  result = ConnectResult::OtherError; break;
    }
    return true;
}

bool HM11Driver::connect(MACAddressType mac_type, const util::MACAddress& address,
                         ConnectResult& result, Status& status) {
    result = ConnectResult::OtherError;

    // Build MAC type string ("N", "S", "T", "R")
    char type_char = 0;
    switch (mac_type) {
        case MACAddressType::NormalAddress:    type_char = 'N'; break;
        case MACAddressType::StaticMAC:        type_char = 'S'; break;
        case MACAddressType::StaticRandomMAC:  type_char = 'T'; break;
        case MACAddressType::RandomMAC:        type_char = 'R'; break;
        default: status = Status::InvalidResponse; return false;
    }

    std::string type_str(1, type_char);
    std::string mac_str(address.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::ConnectByMAC, {type_str, mac_str});
    std::string expected = "OK+CO" + type_str + type_str;

    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse connect result from response
    char c = static_cast<char>(response_buffer.at(expected.size()));
    switch (c) {
        case '0': result = ConnectResult::Connecting; break;
        case '1': result = ConnectResult::Connected; break;
        case '2': result = ConnectResult::ConnectError; break;
        case '3': result = ConnectResult::ConnectFail; break;
        case '4': result = ConnectResult::NoAddress; break;
        default:  result = ConnectResult::OtherError; break;
    }
    return true;
}

bool HM11Driver::connect(util::DiscoveryIndex index, ConnectResult& result, Status& status) {
    result = ConnectResult::OtherError;
    std::string index_str = std::to_string(static_cast<int>(index));
    std::string cmd = ATCommandBuilder::build(AtCommand::ConnectByIndex, {index_str});
    transmit_and_check(cmd, "OK+CONN", response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse connect result from response
    char c = static_cast<char>(response_buffer[7]); // "OK+CONN" is 7 characters
    switch (c) {
        case '0': result = ConnectResult::Connecting; break;
        case '1': result = ConnectResult::Connected; break;
        case '2': result = ConnectResult::ConnectError; break;
        case '3': result = ConnectResult::ConnectFail; break;
        case '4': result = ConnectResult::NoAddress; break;
        default:  result = ConnectResult::OtherError; break;
    }
    return true;
}

bool HM11Driver::get_last_connected_device_address(util::MACAddress& mac, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetLastConnectedDeviceAddress, {});
    std::string expected = "OK+RADD:";
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy MAC address from response (12 hex characters after OK+RADD:)
    std::string mac_str(12, '\0');
    std::memcpy(mac_str.data(), response_buffer.data() + expected.size(), 12);
    mac = util::MACAddress(mac_str);
    return true;
}

bool HM11Driver::set_save_connected_mac(bool save, Status& status) {
    // Note: Ada inverts the boolean (not Save)
    char c = save ? '0' : '1';
    std::string cmd = ATCommandBuilder::build(AtCommand::SetSaveConnectedMAC, {std::string(1, c)});
    std::string expected = OK_SET + std::string(1, c);
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_save_connected_mac(bool& save, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetSaveConnectedMAC, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Note: Ada uses inverted logic (Save := S = '0')
    save = (response_buffer[OK_GET.size()] == '0');
    return true;
}

bool HM11Driver::set_connect_remote_device_timeout(const util::ConnectTimeout& timeout, Status& status) {
    std::string timeout_str(timeout.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetConnectRemoteDeviceTimeout, {timeout_str});
    std::string expected = OK_SET + timeout_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connect_remote_device_timeout(util::ConnectTimeout& timeout, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetConnectRemoteDeviceTimeout, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy timeout from response (6 digit characters after OK+GET:)
    std::string timeout_str(6, '\0');
    std::memcpy(timeout_str.data(), response_buffer.data() + OK_GET.size(), 6);
    timeout = util::ConnectTimeout(timeout_str);
    return true;
}

// ========== Discovery Methods ==========

bool HM11Driver::set_discovery_time(util::DiscoveryTime time, Status& status) {
    std::string time_str = std::to_string(static_cast<int>(time));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetDiscoveryTime, {time_str});
    std::string expected = OK_SET + time_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_discovery_time(util::DiscoveryTime& time, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetDiscoveryTime, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    time = util::DiscoveryTime(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_show_device_information_when_discovery(ShowDeviceInformation show, Status& status) {
    std::string show_str = std::to_string(static_cast<int>(show));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetShowDeviceInformation, {show_str});
    std::string expected = OK_SET + show_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_show_device_information_when_discovery(ShowDeviceInformation& show, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetShowDeviceInformation, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    show = static_cast<ShowDeviceInformation>(value);
    return true;
}

// ========== GATT Methods ==========

bool HM11Driver::enable_characteristic_notify(const util::HandleType& handle,
                                               NotifyResponse& response, Status& status) {
    std::string handle_str(handle.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::EnableCharacteristicNotify, {handle_str});
    transmit(cmd, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Read response: OK+SEND-OK or OK+DATA-ER
    receive(response_buffer, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Check response
    std::string resp_str(10, '\0');
    std::memcpy(resp_str.data(), response_buffer.data(), std::min(static_cast<std::size_t>(10), response_buffer.size()));
    if (resp_str.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_str.starts_with("OK+DATA-ER")) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::DataEr;
        status = Status::InvalidResponse;
        return false;
    }
    return true;
}

bool HM11Driver::disable_characteristic_notify(const util::HandleType& handle,
                                                NotifyResponse& response, Status& status) {
    std::string handle_str(handle.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::DisableCharacteristicNotify, {handle_str});
    transmit(cmd, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Read response: OK+SEND-OK or OK+DATA-ER
    receive(response_buffer, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Check response
    std::string resp_str(10, '\0');
    std::memcpy(resp_str.data(), response_buffer.data(), std::min(static_cast<std::size_t>(10), response_buffer.size()));
    if (resp_str.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_str.starts_with("OK+DATA-ER")) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::DataEr;
        status = Status::InvalidResponse;
        return false;
    }
    return true;
}

bool HM11Driver::read_characteristic_notify(const util::HandleType& handle,
                                             NotifyResponse& response, Status& status) {
    std::string handle_str(handle.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::ReadCharacteristicNotify, {handle_str});
    transmit(cmd, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Read response: OK+SEND-OK or OK+DATA-ER
    receive(response_buffer, status);
    if (status != Status::Ok) {
        response = NotifyResponse::DataEr;
        return false;
    }

    // Check response
    std::string resp_str(10, '\0');
    std::memcpy(resp_str.data(), response_buffer.data(), std::min(static_cast<std::size_t>(10), response_buffer.size()));
    if (resp_str.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_str.starts_with("OK+DATA-ER")) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::DataEr;
        status = Status::InvalidResponse;
        return false;
    }
    return true;
}

bool HM11Driver::set_method_and_characteristic_handle(const util::HandleType& handle,
                                                       SendDataMethod method, Status& status) {
    // Convert method to string
    std::string method_str = std::to_string(static_cast<int>(method));
    std::string handle_str(handle.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetMethodAndCharacteristicHandle,
                                               {method_str, handle_str});
    transmit_and_check(cmd, "OK+SEND-OK", status);
    return status == Status::Ok;
}

bool HM11Driver::set_use_characteristic_uuid_count(CharacteristicUUIDCount count, Status& status) {
    std::string count_str;
    if (count == CharacteristicUUIDCount::Query) {
        count_str = "?";
    } else {
        count_str = std::to_string(static_cast<int>(count));
    }
    std::string cmd = ATCommandBuilder::build(AtCommand::SetUseCharacteristicUUIDCount, {count_str});
    std::string expected = OK_SET + count_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_characteristic(const util::CharacteristicType& value, Status& status) {
    std::string value_str(value.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetCharacteristic, {value_str});
    transmit_and_check(cmd, OK_SET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_characteristic(util::CharacteristicType& result, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetCharacteristic, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy characteristic from response (4 hex characters after OK+GET:0x)
    std::string char_str(4, '\0');
    std::memcpy(char_str.data(), response_buffer.data() + OK_GET.size() + 2, 4);
    result = util::CharacteristicType(char_str);
    return true;
}

// ========== Service UUID Methods ==========

bool HM11Driver::set_service_uuid(const util::UUID& value, Status& status) {
    std::string value_str(value.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetServiceUUID, {value_str});
    std::string expected = OK_SET + "0x" + value_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_service_uuid(util::UUID& result, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetServiceUUID, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy UUID from response (4 hex characters after OK+GET:0x)
    std::string uuid_str(4, '\0');
    std::memcpy(uuid_str.data(), response_buffer.data() + OK_GET.size() + 2, 4);
    result = util::UUID(uuid_str);
    return true;
}

// ========== GATT Discovery Methods ==========

bool HM11Driver::find_all_services_uuid(
    const std::function<void(const std::string& service_info)>& callback,
    Status& status,
    std::uint32_t timeout_ms) {

    // Send service discovery command
    std::string cmd = ATCommandBuilder::build(AtCommand::FindAllServicesUUID, {});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Service info format: "xxxx:xxxx:xxxx" (start_handle:end_handle:uuid)
    constexpr std::size_t SERVICE_INFO_LENGTH = 14;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    std::string buffer;
    bool started = false;
    bool discovery_complete = false;

    while (!discovery_complete) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (std::cmp_greater(elapsed, timeout_ms)) {
            status = Status::Timeout;
            return false;
        }

        // Try to receive data
        std::string received;
        if (receive(received, status, 512)) {
            buffer += received;

            // Parse buffer for service information
            std::size_t pos = 0;
            while (pos < buffer.size()) {
                // Check for header markers
                if (buffer.substr(pos, std::min(HEADER.size(), buffer.size() - pos)) == HEADER) {
                    if (started) {
                        // End marker - discovery complete
                        discovery_complete = true;
                        break;
                    }
                    started = true;
                    pos += HEADER.size();
                    continue;
                }

                // Skip CRLF
                if (pos + 1 < buffer.size() && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for service info
                if (started && pos + SERVICE_INFO_LENGTH <= buffer.size()) {
                    std::string potential_service = buffer.substr(pos, SERVICE_INFO_LENGTH);
                    // Validate format: xxxx:xxxx:xxxx
                    if (potential_service[4] == ':' && potential_service[9] == ':') {
                        callback(potential_service);
                        pos += SERVICE_INFO_LENGTH;
                        continue;
                    }
                }

                ++pos;
            }
        } else if (status == Status::Timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            status = Status::Ok;
        } else {
            return false;
        }
    }

    status = Status::Ok;
    return true;
}

bool HM11Driver::find_all_characteristic_uuid(
    const std::function<void(const std::string& characteristic_info)>& callback,
    Status& status,
    std::uint32_t timeout_ms) {

    // Send characteristic discovery command
    std::string cmd = ATCommandBuilder::build(AtCommand::FindAllCharacteristicUUID, {});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Characteristic info format: "xxxx:ppppppppppppp:xxxx" (handle:properties:uuid)
    constexpr std::size_t CHAR_INFO_LENGTH = 24;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    std::string buffer;
    bool started = false;
    bool discovery_complete = false;

    while (!discovery_complete) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (std::cmp_greater(elapsed, timeout_ms)) {
            status = Status::Timeout;
            return false;
        }

        // Try to receive data
        std::string received;
        if (receive(received, status, 512)) {
            buffer += received;

            // Parse buffer for characteristic information
            std::size_t pos = 0;
            while (pos < buffer.size()) {
                // Check for header markers
                if (buffer.substr(pos, std::min(HEADER.size(), buffer.size() - pos)) == HEADER) {
                    if (started) {
                        // End marker - discovery complete
                        discovery_complete = true;
                        break;
                    }
                    started = true;
                    pos += HEADER.size();
                    continue;
                }

                // Skip CRLF
                if (pos + 1 < buffer.size() && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for characteristic info
                if (started && pos + CHAR_INFO_LENGTH <= buffer.size()) {
                    std::string potential_char = buffer.substr(pos, CHAR_INFO_LENGTH);
                    // Validate format: xxxx:ppppppppppppp:xxxx (4:14:4 with colons)
                    if (potential_char[4] == ':' && potential_char[19] == ':') {
                        callback(potential_char);
                        pos += CHAR_INFO_LENGTH;
                        continue;
                    }
                }

                ++pos;
            }
        } else if (status == Status::Timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            status = Status::Ok;
        } else {
            return false;
        }
    }

    status = Status::Ok;
    return true;
}

bool HM11Driver::find_characteristic_uuid(
    const util::HandleType& from,
    const util::HandleType& to,
    const std::function<void(const std::string& characteristic_info)>& callback,
    Status& status,
    std::uint32_t timeout_ms) {

    // Build command: AT+CHARxxxxYYYY? where xxxx=from, YYYY=to
    std::string from_str(from.view());
    std::string to_str(to.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::FindCharacteristicUUID, {from_str, to_str});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Characteristic info format: "xxxx:ppppppppppppp:xxxx" (handle:properties:uuid)
    constexpr std::size_t CHAR_INFO_LENGTH = 24;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    std::string buffer;
    bool started = false;
    bool discovery_complete = false;

    while (!discovery_complete) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (std::cmp_greater(elapsed, timeout_ms)) {
            status = Status::Timeout;
            return false;
        }

        // Try to receive data
        std::string received;
        if (receive(received, status, 512)) {
            buffer += received;

            // Parse buffer for characteristic information
            std::size_t pos = 0;
            while (pos < buffer.size()) {
                // Check for header markers
                if (buffer.substr(pos, std::min(HEADER.size(), buffer.size() - pos)) == HEADER) {
                    if (started) {
                        // End marker - discovery complete
                        discovery_complete = true;
                        break;
                    }
                    started = true;
                    pos += HEADER.size();
                    continue;
                }

                // Skip CRLF
                if (pos + 1 < buffer.size() && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for characteristic info
                if (started && pos + CHAR_INFO_LENGTH <= buffer.size()) {
                    std::string potential_char = buffer.substr(pos, CHAR_INFO_LENGTH);
                    // Validate format: xxxx:ppppppppppppp:xxxx (4:14:4 with colons)
                    if (potential_char[4] == ':' && potential_char[19] == ':') {
                        callback(potential_char);
                        pos += CHAR_INFO_LENGTH;
                        continue;
                    }
                }

                ++pos;
            }
        } else if (status == Status::Timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            status = Status::Ok;
        } else {
            return false;
        }
    }

    status = Status::Ok;
    return true;
}

bool HM11Driver::send_data_to_characteristic(
    const util::HandleType& handle,
    SendDataCharacteristic method,
    std::span<const uint8_t> data,
    Status& status) {

    // Build command: AT+SEND_DATA[handle][method][data]
    std::string handle_str(handle.view());
    std::string method_str = (method == SendDataCharacteristic::Write) ? "0" : "1";

    // Convert data to hex string
    std::string data_hex;
    constexpr std::array<char, 16> HEX_CHARS = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (const auto BYTE : data) {
        data_hex += HEX_CHARS.at((BYTE >> 4) & 0xF);
        data_hex += HEX_CHARS.at(BYTE & 0xF);
    }

    std::string cmd = ATCommandBuilder::build(AtCommand::SendDataToCharacteristic,
                                              {handle_str, method_str, data_hex});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Receive response: OK+SEND-OK or OK+DATA-ER
    receive(response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Check response
    std::string resp_str(10, '\0');
    std::memcpy(resp_str.data(), response_buffer.data(), std::min(static_cast<std::size_t>(10), response_buffer.size()));
    if (resp_str.starts_with("OK+SEND-OK")) {
        status = Status::Ok;
        return true;
    }
    status = Status::InvalidResponse;
    return false;
}

// ========== Power Methods ==========

bool HM11Driver::set_module_power(ModulePower power, Status& status) {
    std::string power_str = std::to_string(static_cast<int>(power));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetModulePower, {power_str});
    std::string expected = OK_SET + power_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_power(ModulePower& power, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetModulePower, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    power = static_cast<ModulePower>(value);
    return true;
}

bool HM11Driver::set_output_power(OutputPower power, Status& status) {
    std::string power_str = std::to_string(static_cast<int>(power));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetOutputPower, {power_str});
    std::string expected = OK_SET + power_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_output_power(OutputPower& power, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetOutputPower, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    power = static_cast<OutputPower>(value);
    return true;
}

} // namespace hm11
