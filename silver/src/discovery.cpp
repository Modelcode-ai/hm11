/// @file discovery.cpp
/// @brief Implementation file for HM11 discovery operations
///
/// This file contains the actual implementation of the discovery state machine
/// and streaming parser. It's compiled separately to avoid template
/// instantiation issues while keeping the declarations in the header.

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/util/parsing.hpp"

#ifdef HM11_STM32_PLATFORM
#include "hm11/hal/stm32_uart_port.hpp"
#endif

namespace hm11 {

// Discovery implementation for all template instantiations
// This approach avoids linker errors by providing concrete implementations

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::scan(
    DiscoveredCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Reset discovery state and clear discovered devices storage
    discovered_info_.reset();
    scan_stage_ = ScanStage::Selection;
    buffer_position_ = 0;
    discovered_device_count_ = 0;

    // Clear discovered devices array for new scan
    for (auto& device : discovered_devices_) {
        device.reset();
    }

    // Helper function to invoke callback with accumulated data and store device with index
    auto invoke_callback = [&]() {
        if (discovered_info_.id != ' ') {
            // Store device in discovered devices array (up to 6 devices)
            if (discovered_device_count_ < MAX_DISCOVERED_DEVICES) {
                discovered_devices_[discovered_device_count_] = discovered_info_;
                // Assign sequential index as device ID for Connect(index) operations
                discovered_devices_[discovered_device_count_].id = '0' + static_cast<char>(discovered_device_count_);
                discovered_device_count_++;
            }

            // Invoke callback with current device information
            callback(
                discovered_info_.id,
                discovered_info_.mac,
                discovered_info_.name.view(),
                discovered_info_.rssi.view(),
                user_data);

            // Reset for next device (matches Ada's Null_Discovered_Info assignment)
            discovered_info_.reset();
        }
    };

    // Send discovery command
    Transmit("AT+DISC?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // For prototype, receive into internal buffer and parse synchronously
    constexpr std::size_t DISCOVERY_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), DISCOVERY_BUFFER_SIZE};

    // Simple timeout implementation - in Task 4 this will use Watchdog with ISR polling
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse discovery responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        // Check for discovery end marker
        if (util::starts_with(
                std::string_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining),
                util::discovery_prefixes::OK_DISCE)) {
            break;
        }

        // State machine processing
        switch (scan_stage_) {
            case ScanStage::Selection: {
                // Look for command prefixes
                if (remaining < util::discovery_prefixes::MIN_PREFIX) {
                    // Not enough data for any prefix
                    break;
                }

                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                if (util::starts_with(current_view, util::discovery_prefixes::OK_DISCS)) {
                    pos += util::discovery_prefixes::OK_DISCS.size();
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_DISC)) {
                    pos += util::discovery_prefixes::OK_DISC.size();
                    scan_stage_ = ScanStage::MAC;
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_NAME)) {
                    pos += util::discovery_prefixes::OK_NAME.size();
                    scan_stage_ = ScanStage::Name;
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_RSSI)) {
                    pos += util::discovery_prefixes::OK_RSSI.size();
                    scan_stage_ = ScanStage::RSSI;
                    continue;
                }

                // No prefix matched, advance by one character
                pos++;
                break;
            }

            case ScanStage::MAC: {
                // Call callback for any previous complete device
                invoke_callback();

                constexpr std::size_t MAC_LENGTH = 12;
                if (remaining < MAC_LENGTH) {
                    // Not enough data for MAC address
                    return;
                }

                // Extract MAC address (12 hex characters)
                const std::string_view mac_str(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    MAC_LENGTH);

                const auto parsed_mac = util::parse_mac_address(mac_str);
                if (parsed_mac.has_value()) {
                    discovered_info_.mac = *parsed_mac;
                    discovered_info_.id = '0'; // Set default ID
                }

                pos += MAC_LENGTH;
                scan_stage_ = ScanStage::Selection;
                break;
            }

            case ScanStage::Name: {
                // Find CR+LF terminator
                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                const std::size_t crlf_pos = current_view.find(util::discovery_prefixes::CRLF);
                if (crlf_pos == std::string_view::npos) {
                    // No terminator found, append all remaining data
                    try {
                        discovered_info_.name.append(current_view);
                        pos = buffer_end; // Consumed all data
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                } else {
                    // Found terminator, append data before it
                    try {
                        discovered_info_.name.append(current_view.substr(0, crlf_pos));
                        pos += crlf_pos + util::discovery_prefixes::CRLF.size();
                        scan_stage_ = ScanStage::Selection;
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                }
                break;
            }

            case ScanStage::RSSI: {
                // Find CR+LF terminator
                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                const std::size_t crlf_pos = current_view.find(util::discovery_prefixes::CRLF);
                if (crlf_pos == std::string_view::npos) {
                    // No terminator found, append all remaining data
                    try {
                        discovered_info_.rssi.append(current_view);
                        pos = buffer_end; // Consumed all data
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                } else {
                    // Found terminator, append data before it
                    try {
                        discovered_info_.rssi.append(current_view.substr(0, crlf_pos));
                        pos += crlf_pos + util::discovery_prefixes::CRLF.size();
                        scan_stage_ = ScanStage::Selection;
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                }
                break;
            }
        }
    }

    // Call callback for any final accumulated data (matching Ada's final Call)
    invoke_callback();

    status = StatusType::Ok;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::scan_ibeacon(
    DiscoveredIBeaconCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send iBeacon discovery command (matching Ada's "AT+DISI?")
    Transmit("AT+DISI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t IBEACON_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), IBEACON_BUFFER_SIZE};

    // Receive iBeacon discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse iBeacon responses using simplified approach based on Ada implementation
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure for iBeacon
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        // Need at least minimum prefix length
        if (remaining < util::ibeacon_discovery_prefixes::OK_DISC.size()) {
            break;
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // Check for iBeacon discovery end marker
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISCE)) {
            break;
        }

        // Check for iBeacon discovery start (OK+DISCS)
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISCS)) {
            pos += util::ibeacon_discovery_prefixes::OK_DISCS.size();
            continue;
        }

        // Check for iBeacon device data (OK+DISC + 66 characters)
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISC)) {
            const std::size_t data_start = pos + util::ibeacon_discovery_prefixes::OK_DISC.size();
            const std::size_t required_length = util::ibeacon_discovery_prefixes::OK_DISC.size() +
                                                util::ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH;

            if (remaining >= required_length) {
                // Extract the 66-character iBeacon data
                const std::string_view ibeacon_data(
                    reinterpret_cast<const char*>(response_buffer_.data() + data_start),
                    util::ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH);

                // Invoke callback with the device string (matches Ada interface exactly)
                try {
                    callback(ibeacon_data, user_data);
                } catch (...) {
                    // Continue processing even if callback throws - log error but don't halt parsing
                    // In production code, this should log the error appropriately
                    // For now, we silently continue to prevent termination of discovery
                    (void)0; // Explicit no-op to satisfy linter
                }

                pos += required_length;
                continue;
            }
            // Not enough data for complete iBeacon record
            break;
        }

        // No prefix matched, advance by one character
        pos++;
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Service Discovery Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_all_services_uuid(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send service discovery command
    Transmit("AT+FINDALLSERVICES?", status);
    if (status != StatusType::Ok) {
        return;
    }

    parse_service_discovery_response(callback, user_data, timeout_ms, status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_service_discovery_response(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    // Reset service discovery state
    gatt_service_stage_ = GattServiceStage::LookingForHeader;
    gatt_discovery_started_ = false;

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t SERVICE_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), SERVICE_BUFFER_SIZE};

    // Receive service discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse service responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        if (remaining < util::gatt_service_discovery::SERVICE_UUID_LENGTH) {
            break; // Not enough data for service UUID
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // State machine processing
        switch (gatt_service_stage_) {
            case GattServiceStage::LookingForHeader: {
                // Look for header marker
                if (remaining >= util::gatt_service_discovery::SERVICE_HEADER_LENGTH) {
                    if (util::starts_with(current_view, util::gatt_service_discovery::SERVICE_HEADER)) {
                        if (gatt_discovery_started_) {
                            // End message, exit
                            return;
                        }
                        // Start message
                        gatt_discovery_started_ = true;
                        pos += util::gatt_service_discovery::SERVICE_HEADER_LENGTH;
                        gatt_service_stage_ = GattServiceStage::ParsingService;
                        continue;
                    }
                }
                pos++;
                break;
            }

            case GattServiceStage::ParsingService: {
                // Look for CRLF to skip line breaks
                if (util::starts_with(current_view, util::discovery_prefixes::CRLF)) {
                    pos += util::discovery_prefixes::CRLF.size();
                    continue;
                }

                // Check for end header
                if (remaining >= util::gatt_service_discovery::SERVICE_HEADER_LENGTH &&
                    util::starts_with(current_view, util::gatt_service_discovery::SERVICE_HEADER)) {
                    // End of services
                    return;
                }

                // Parse service UUID line (14 characters: 4:4:4)
                if (remaining >= util::gatt_service_discovery::SERVICE_UUID_LENGTH) {
                    const std::string_view service_data =
                        current_view.substr(0, util::gatt_service_discovery::SERVICE_UUID_LENGTH);

                    // Validate service format and invoke callback
                    std::string_view start_handle;
                    std::string_view end_handle;
                    std::string_view service_uuid;
                    if (util::parse_service_uuid_components(service_data, start_handle, end_handle, service_uuid)) {
                        try {
                            callback(service_data, user_data);
                        } catch (...) {
                            // Continue processing even if callback throws - log error but don't halt parsing
                            // In production code, this should log the error appropriately
                            // For now, we silently continue to prevent termination of discovery
                            (void)0; // Explicit no-op to satisfy linter
                        }
                    }

                    pos += util::gatt_service_discovery::SERVICE_UUID_LENGTH;
                    continue;
                }

                pos++;
                break;
            }

            case GattServiceStage::LookingForEnd: {
                // This state is handled by the header detection above
                pos++;
                break;
            }
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Discovery Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_all_characteristics_uuid(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send characteristic discovery command
    Transmit("AT+FINDALLCHARS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_characteristics_uuid(
    const HandleType& from,
    const HandleType& to,
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    try {
        // Build command: "AT+CHAR" + from + to + "?"
        util::ATCommandBuilder cmd;
        cmd.append("AT+CHAR").append(std::string_view(from)).append(std::string_view(to)).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    // Reset characteristic discovery state
    gatt_characteristic_stage_ = GattCharacteristicStage::LookingForHeader;
    gatt_characteristic_discovery_started_ = false;

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t CHARACTERISTIC_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), CHARACTERISTIC_BUFFER_SIZE};

    // Receive characteristic discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse characteristic responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        if (remaining < util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH) {
            break; // Not enough data for characteristic UUID
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // State machine processing
        switch (gatt_characteristic_stage_) {
            case GattCharacteristicStage::LookingForHeader: {
                // Look for header marker
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH) {
                    if (util::starts_with(current_view, util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER)) {
                        if (gatt_characteristic_discovery_started_) {
                            // End message, exit
                            return;
                        }
                        // Start message
                        gatt_characteristic_discovery_started_ = true;
                        pos += util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH;
                        gatt_characteristic_stage_ = GattCharacteristicStage::ParsingCharacteristic;
                        continue;
                    }
                }
                pos++;
                break;
            }

            case GattCharacteristicStage::ParsingCharacteristic: {
                // Look for CRLF to skip line breaks
                if (util::starts_with(current_view, util::discovery_prefixes::CRLF)) {
                    pos += util::discovery_prefixes::CRLF.size();
                    continue;
                }

                // Check for end header
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH &&
                    util::starts_with(current_view, util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER)) {
                    // End of characteristics
                    return;
                }

                // Parse characteristic UUID line (24 characters: 4:14:4)
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH) {
                    const std::string_view characteristic_data =
                        current_view.substr(0, util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH);

                    // Validate characteristic format and invoke callback
                    std::string_view handle;
                    util::CharacteristicProperty properties = {};
                    std::string_view characteristic_uuid;

                    if (util::parse_characteristic_uuid_components(
                            characteristic_data,
                            handle,
                            properties,
                            characteristic_uuid)) {
                        try {
                            callback(characteristic_data, user_data);
                        } catch (...) {
                            // Continue processing even if callback throws - log error but don't halt parsing
                            // In production code, this should log the error appropriately
                            // For now, we silently continue to prevent termination of discovery
                            (void)0; // Explicit no-op to satisfy linter
                        }
                    }

                    pos += util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH;
                    continue;
                }

                pos++;
                break;
            }

            case GattCharacteristicStage::LookingForEnd: {
                // This state is handled by the header detection above
                pos++;
                break;
            }
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::enable_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+NOTIFY_ON" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+NOTIFY_ON").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::disable_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+NOTIFYOFF" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+NOTIFYOFF").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::read_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+READDATA" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+READDATA").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_method_and_characteristic_handle(
    const HandleType& handle,
    SendDataMethod method,
    StatusType& status) noexcept {
    try {
        // Build command: "AT+SET_WAY" + method_string + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+SET_WAY").append(util::to_string(method)).append(std::string_view(handle));

        TransmitAndCheck(cmd.view(), "OK+SEND-OK", status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::send_data_to_characteristic(
    const HandleType& handle,
    SendDataCharacteristic method,
    hal::UARTData8bConst data,
    StatusType& status) noexcept {
    try {
        // Build command: "AT+SEND_DATA" + method_string + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+SEND_DATA").append(util::to_string(method)).append(std::string_view(handle));

        Transmit(cmd.view(), data, status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_notify_response(StatusType& status, NotifyResponse& response) noexcept {
    if (status != StatusType::Ok) {
        response = NotifyResponse::SendEr;
        return;
    }

    // Expected responses: "OK+SEND-OK", "OK+DATA-ER", or other (mapped to Send_Er)
    constexpr std::string_view SEND_OK = "OK+SEND-OK";
    constexpr std::string_view DATA_ER = "OK+DATA-ER";
    const auto expected_length = std::max(SEND_OK.size(), DATA_ER.size());

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        response = NotifyResponse::SendEr;
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (response_str == SEND_OK) {
        response = NotifyResponse::SendOk;
    } else if (response_str == DATA_ER) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::SendEr;
    }
}

// Explicit template instantiations
template void HM11Driver<hal::MockUART>::scan(
    DiscoveredCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::scan_ibeacon(
    DiscoveredIBeaconCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Service Discovery
template void HM11Driver<hal::MockUART>::find_all_services_uuid(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::parse_service_discovery_response(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Characteristic Discovery
template void HM11Driver<hal::MockUART>::find_all_characteristics_uuid(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::find_characteristics_uuid(
    const HandleType& from,
    const HandleType& to,
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Characteristic Operations
template void HM11Driver<hal::MockUART>::enable_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::MockUART>::disable_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::MockUART>::read_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::MockUART>::set_method_and_characteristic_handle(
    const HandleType& handle,
    SendDataMethod method,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::send_data_to_characteristic(
    const HandleType& handle,
    SendDataCharacteristic method,
    hal::UARTData8bConst data,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::MockUART>::parse_characteristic_notify_response(
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

#ifdef HM11_STM32_PLATFORM
template void HM11Driver<hal::STM32UARTPort>::scan(
    DiscoveredCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::scan_ibeacon(
    DiscoveredIBeaconCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Service Discovery
template void HM11Driver<hal::STM32UARTPort>::find_all_services_uuid(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_service_discovery_response(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Characteristic Discovery
template void HM11Driver<hal::STM32UARTPort>::find_all_characteristics_uuid(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::find_characteristics_uuid(
    const HandleType& from,
    const HandleType& to,
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    hal::UARTStatus& status) noexcept;

// GATT Characteristic Operations
template void HM11Driver<hal::STM32UARTPort>::enable_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::STM32UARTPort>::disable_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::STM32UARTPort>::read_characteristic_notify(
    const HandleType& handle,
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;

template void HM11Driver<hal::STM32UARTPort>::set_method_and_characteristic_handle(
    const HandleType& handle,
    SendDataMethod method,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::send_data_to_characteristic(
    const HandleType& handle,
    SendDataCharacteristic method,
    hal::UARTData8bConst data,
    hal::UARTStatus& status) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_characteristic_notify_response(
    hal::UARTStatus& status,
    NotifyResponse& response) noexcept;
#endif

} // namespace hm11
