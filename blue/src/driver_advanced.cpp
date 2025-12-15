#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <utility>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/util/numeric_formatter.hpp"
#include "hm11/util/variable_string.hpp"

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

// ========== Connection Methods ==========

bool HM11Driver::clear_last_connected_address(Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ClearLastConnectedAddress, {});
    transmit_and_check(cmd_buf.finalize(), "OK+CLEAR", status);
    return status == Status::Ok;
}

bool HM11Driver::connect_last_device(ConnectResult& result, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ConnectLastDevice, {});
    transmit_and_check(cmd_buf.finalize(), "OK+CONN", response_buffer, status);
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

    std::array<char, 1> type_str_buf = {type_char};
    std::string_view type_str(type_str_buf.data(), 1);
    auto mac_str = address.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ConnectByMAC, {type_str, mac_str});
    std::array<char, 64> expected_buf{};
    std::memcpy(expected_buf.data(), "OK+CO", 5);
    expected_buf[5] = type_char;
    expected_buf[6] = type_char;
    auto expected = std::string_view(expected_buf.data(), 7);

    transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
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
    auto index_str = NumericFormatter::format(static_cast<int>(index));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ConnectByIndex, {index_str});
    transmit_and_check(cmd_buf.finalize(), "OK+CONN", response_buffer, status);
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
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetLastConnectedDeviceAddress, {});
    constexpr const char* EXPECTED = "OK+RADD:";
    transmit_and_check(cmd_buf.finalize(), EXPECTED, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy MAC address from response (12 hex characters after OK+RADD:)
    std::array<char, 12> mac_arr{};
    std::memcpy(mac_arr.data(), response_buffer.data() + std::strlen(EXPECTED), 12);
    mac = util::MACAddress(std::string_view(mac_arr.data(), 12));
    return true;
}

bool HM11Driver::set_save_connected_mac(bool save, Status& status) {
    // Note: Ada inverts the boolean (not Save)
    char c = save ? '0' : '1';
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetSaveConnectedMAC, {std::string_view(&c, 1)});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, std::string_view(&c, 1), expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_save_connected_mac(bool& save, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetSaveConnectedMAC, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Note: Ada uses inverted logic (Save := S = '0')
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    save = (response_buffer[std::strlen(OK_GET)] == '0');
    return true;
}

bool HM11Driver::set_connect_remote_device_timeout(const util::ConnectTimeout& timeout, Status& status) {
    auto timeout_str = timeout.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetConnectRemoteDeviceTimeout, {timeout_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, timeout_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connect_remote_device_timeout(util::ConnectTimeout& timeout, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetConnectRemoteDeviceTimeout, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy timeout from response (6 digit characters after OK+GET:)
    std::array<char, 6> timeout_arr{};
    std::memcpy(timeout_arr.data(), response_buffer.data() + std::strlen(OK_GET), 6);
    timeout = util::ConnectTimeout(std::string_view(timeout_arr.data(), 6));
    return true;
}

// ========== Discovery Methods ==========

bool HM11Driver::set_discovery_time(util::DiscoveryTime time, Status& status) {
    auto time_str = NumericFormatter::format(static_cast<int>(time));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetDiscoveryTime, {time_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, time_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_discovery_time(util::DiscoveryTime& time, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetDiscoveryTime, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    time = util::DiscoveryTime(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_show_device_information_when_discovery(ShowDeviceInformation show, Status& status) {
    auto show_str = NumericFormatter::format(static_cast<int>(show));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetShowDeviceInformation, {show_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, show_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_show_device_information_when_discovery(ShowDeviceInformation& show, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetShowDeviceInformation, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    show = static_cast<ShowDeviceInformation>(value);
    return true;
}

// ========== GATT Methods ==========

bool HM11Driver::enable_characteristic_notify(const util::HandleType& handle,
                                               NotifyResponse& response, Status& status) {
    auto handle_str = handle.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::EnableCharacteristicNotify, {handle_str});
    transmit(cmd_buf.finalize(), status);
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
    std::size_t resp_len = std::min(static_cast<std::size_t>(10), response_buffer.size());
    std::string_view resp_view(reinterpret_cast<const char*>(response_buffer.data()), resp_len);
    if (resp_view.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_view.starts_with("OK+DATA-ER")) {
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
    auto handle_str = handle.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::DisableCharacteristicNotify, {handle_str});
    transmit(cmd_buf.finalize(), status);
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
    std::size_t resp_len = std::min(static_cast<std::size_t>(10), response_buffer.size());
    std::string_view resp_view(reinterpret_cast<const char*>(response_buffer.data()), resp_len);
    if (resp_view.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_view.starts_with("OK+DATA-ER")) {
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
    auto handle_str = handle.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ReadCharacteristicNotify, {handle_str});
    transmit(cmd_buf.finalize(), status);
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
    std::size_t resp_len = std::min(static_cast<std::size_t>(10), response_buffer.size());
    std::string_view resp_view(reinterpret_cast<const char*>(response_buffer.data()), resp_len);
    if (resp_view.starts_with("OK+SEND-OK")) {
        response = NotifyResponse::SendOk;
    } else if (resp_view.starts_with("OK+DATA-ER")) {
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
    auto method_str = NumericFormatter::format(static_cast<int>(method));
    auto handle_str = handle.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetMethodAndCharacteristicHandle,
                                               {method_str, handle_str});
    transmit_and_check(cmd_buf.finalize(), "OK+SEND-OK", status);
    return status == Status::Ok;
}

bool HM11Driver::set_use_characteristic_uuid_count(CharacteristicUUIDCount count, Status& status) {
    std::string_view count_str;
    constexpr const char* QUERY_STR = "?";
    if (count == CharacteristicUUIDCount::Query) {
        count_str = QUERY_STR;
    } else {
        count_str = NumericFormatter::format(static_cast<int>(count));
    }
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetUseCharacteristicUUIDCount, {count_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, count_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_characteristic(const util::CharacteristicType& value, Status& status) {
    auto value_str = value.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetCharacteristic, {value_str});
    transmit_and_check(cmd_buf.finalize(), OK_SET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_characteristic(util::CharacteristicType& result, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetCharacteristic, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy characteristic from response (4 hex characters after OK+GET:0x)
    std::array<char, 4> char_arr{};
    std::memcpy(char_arr.data(), response_buffer.data() + std::strlen(OK_GET) + 2, 4);
    result = util::CharacteristicType(std::string_view(char_arr.data(), 4));
    return true;
}

// ========== Service UUID Methods ==========

bool HM11Driver::set_service_uuid(const util::UUID& value, Status& status) {
    auto value_str = value.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetServiceUUID, {value_str});
    std::array<char, 64> expected_buf{};
    std::memcpy(expected_buf.data(), OK_SET, std::strlen(OK_SET));
    // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
    std::memcpy(expected_buf.data() + std::strlen(OK_SET), "0x", 2);
    std::memcpy(expected_buf.data() + std::strlen(OK_SET) + 2, value_str.data(), value_str.size());
    auto expected = std::string_view(expected_buf.data(), std::strlen(OK_SET) + 2 + value_str.size());
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_service_uuid(util::UUID& result, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetServiceUUID, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy UUID from response (4 hex characters after OK+GET:0x)
    std::array<char, 4> uuid_arr{};
    std::memcpy(uuid_arr.data(), response_buffer.data() + std::strlen(OK_GET) + 2, 4);
    result = util::UUID(std::string_view(uuid_arr.data(), 4));
    return true;
}

// ========== GATT Discovery Methods ==========

bool HM11Driver::find_all_services_uuid(
    ServiceUuidCallback callback,
    void* user_data,
    Status& status,
    std::uint32_t timeout_ms) {

    // Send service discovery command
    auto cmd_buf = ATCommandBuilder::build(AtCommand::FindAllServicesUUID, {});
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }

    // Service info format: "xxxx:xxxx:xxxx" (start_handle:end_handle:uuid)
    constexpr std::size_t SERVICE_INFO_LENGTH = 14;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    util::VariableString<1024> buffer;  // Fixed-size buffer for accumulation
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
        std::string_view received;
        if (receive(received, status, 512)) {
            buffer.append(received);

            // Parse buffer for service information
            std::size_t pos = 0;
            std::string_view buffer_view = buffer.view();
            while (pos < buffer_view.size()) {
                // Check for header markers
                if (buffer_view.substr(pos, std::min(HEADER.size(), buffer_view.size() - pos)) == HEADER) {
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
                if (pos + 1 < buffer_view.size() && buffer_view[pos] == '\r' && buffer_view[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for service info
                if (started && pos + SERVICE_INFO_LENGTH <= buffer_view.size()) {
                    std::string_view potential_service = buffer_view.substr(pos, SERVICE_INFO_LENGTH);
                    // Validate format: xxxx:xxxx:xxxx
                    if (potential_service[4] == ':' && potential_service[9] == ':') {
                        callback(potential_service, user_data);
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
    CharacteristicUuidCallback callback,
    void* user_data,
    Status& status,
    std::uint32_t timeout_ms) {

    // Send characteristic discovery command
    auto cmd_buf = ATCommandBuilder::build(AtCommand::FindAllCharacteristicUUID, {});
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }

    // Characteristic info format: "xxxx:ppppppppppppp:xxxx" (handle:properties:uuid)
    constexpr std::size_t CHAR_INFO_LENGTH = 24;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    util::VariableString<1024> buffer;  // Fixed-size buffer for accumulation
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
        std::string_view received;
        if (receive(received, status, 512)) {
            buffer.append(received);

            // Parse buffer for characteristic information
            std::size_t pos = 0;
            std::string_view buffer_view = buffer.view();
            while (pos < buffer_view.size()) {
                // Check for header markers
                if (buffer_view.substr(pos, std::min(HEADER.size(), buffer_view.size() - pos)) == HEADER) {
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
                if (pos + 1 < buffer_view.size() && buffer_view[pos] == '\r' && buffer_view[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for characteristic info
                if (started && pos + CHAR_INFO_LENGTH <= buffer_view.size()) {
                    std::string_view potential_char = buffer_view.substr(pos, CHAR_INFO_LENGTH);
                    // Validate format: xxxx:ppppppppppppp:xxxx (4:14:4 with colons)
                    if (potential_char[4] == ':' && potential_char[19] == ':') {
                        callback(potential_char, user_data);
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
    CharacteristicUuidCallback callback,
    void* user_data,
    Status& status,
    std::uint32_t timeout_ms) {

    // Build command: AT+CHARxxxxYYYY? where xxxx=from, YYYY=to
    auto from_str = from.view();
    auto to_str = to.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::FindCharacteristicUUID, {from_str, to_str});
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }

    // Characteristic info format: "xxxx:ppppppppppppp:xxxx" (handle:properties:uuid)
    constexpr std::size_t CHAR_INFO_LENGTH = 24;
    constexpr std::string_view HEADER = "********************************************************";

    auto start_time = std::chrono::steady_clock::now();
    util::VariableString<1024> buffer;  // Fixed-size buffer for accumulation
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
        std::string_view received;
        if (receive(received, status, 512)) {
            buffer.append(received);

            // Parse buffer for characteristic information
            std::size_t pos = 0;
            std::string_view buffer_view = buffer.view();
            while (pos < buffer_view.size()) {
                // Check for header markers
                if (buffer_view.substr(pos, std::min(HEADER.size(), buffer_view.size() - pos)) == HEADER) {
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
                if (pos + 1 < buffer_view.size() && buffer_view[pos] == '\r' && buffer_view[pos + 1] == '\n') {
                    pos += 2;
                    continue;
                }

                // Check for characteristic info
                if (started && pos + CHAR_INFO_LENGTH <= buffer_view.size()) {
                    std::string_view potential_char = buffer_view.substr(pos, CHAR_INFO_LENGTH);
                    // Validate format: xxxx:ppppppppppppp:xxxx (4:14:4 with colons)
                    if (potential_char[4] == ':' && potential_char[19] == ':') {
                        callback(potential_char, user_data);
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
    auto handle_str = handle.view();
    constexpr const char* METHOD_STR_0 = "0";
    constexpr const char* METHOD_STR_1 = "1";
    const auto *method_str = (method == SendDataCharacteristic::Write) ? METHOD_STR_0 : METHOD_STR_1;

    // Convert data to hex string (max 128 bytes -> 256 hex chars)
    util::VariableString<256> data_hex;
    constexpr std::array<char, 16> HEX_CHARS = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (const auto BYTE : data) {
        data_hex.append(HEX_CHARS.at((BYTE >> 4) & 0xF));
        data_hex.append(HEX_CHARS.at(BYTE & 0xF));
    }

    auto cmd_buf = ATCommandBuilder::build(AtCommand::SendDataToCharacteristic, {handle_str, method_str, data_hex.view()});
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }

    // Receive response: OK+SEND-OK or OK+DATA-ER
    receive(response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Check response
    std::size_t resp_len = std::min(static_cast<std::size_t>(10), response_buffer.size());
    std::string_view resp_view(reinterpret_cast<const char*>(response_buffer.data()), resp_len);
    if (resp_view.starts_with("OK+SEND-OK")) {
        status = Status::Ok;
        return true;
    }
    status = Status::InvalidResponse;
    return false;
}

// ========== Power Methods ==========

bool HM11Driver::set_module_power(ModulePower power, Status& status) {
    auto power_str = NumericFormatter::format(static_cast<int>(power));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetModulePower, {power_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, power_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_power(ModulePower& power, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetModulePower, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    power = static_cast<ModulePower>(value);
    return true;
}

bool HM11Driver::set_output_power(OutputPower power, Status& status) {
    auto power_str = NumericFormatter::format(static_cast<int>(power));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetOutputPower, {power_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, power_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_output_power(OutputPower& power, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetOutputPower, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    power = static_cast<OutputPower>(value);
    return true;
}

} // namespace hm11
