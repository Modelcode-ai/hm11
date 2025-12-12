// HM11 Bluetooth Low Energy Driver Implementation
// Template instantiation control and common implementations
// Reduces compile time by explicitly instantiating common template patterns

#include "hm11/hm11_driver.hpp"

#include "hm11/hal/mock_uart.hpp"
#include "hm11/sync/watchdog.hpp"
#include "hm11/util/parsing.hpp"

#ifdef HM11_STM32_PLATFORM
#include "hm11/hal/stm32_uart_port.hpp"
#endif

namespace hm11 {

// ============================================================================
// Common Type Aliases
// ============================================================================

/// Mock UART driver for unit testing and development
using MockHM11Driver = HM11Driver<hal::MockUART>;

#ifdef HM11_STM32_PLATFORM
/// STM32 hardware driver for production embedded systems
using STM32HM11Driver = HM11Driver<hal::STM32UARTPort>;
#endif

// ============================================================================
// Compile-time Validation
// ============================================================================

namespace detail {
/// Validation that common instantiations work correctly

// Test basic template instantiation with mock UART
static_assert(ValidHM11Driver<hal::MockUART>);
static_assert(!std::is_move_constructible_v<MockHM11Driver>); // Matches Ada's non-movable semantics
static_assert(!std::is_copy_constructible_v<MockHM11Driver>);
static_assert(!std::is_copy_assignable_v<MockHM11Driver>);
static_assert(!std::is_move_assignable_v<MockHM11Driver>);

// Test that driver has reasonable size for embedded systems
static_assert(sizeof(MockHM11Driver) > sizeof(void*)); // Must contain actual data
static_assert(sizeof(MockHM11Driver) < 4096);          // Must not be excessive

// Test proper alignment for DMA operations
static_assert(alignof(MockHM11Driver) >= alignof(std::max_align_t));

} // namespace detail

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

// Instantiate common template combinations to control binary size
// These definitions ensure only the used combinations are compiled

/// Mock UART driver instantiation for testing
template class HM11Driver<hal::MockUART>;

#ifdef HM11_STM32_PLATFORM
/// STM32 hardware driver instantiation for production
template class HM11Driver<hal::STM32UARTPort>;
#endif

// ============================================================================
// Factory Function Instantiations
// ============================================================================

/// Explicit instantiation of factory functions for common types
template auto make_hm11_driver<hal::MockUART>(hal::MockUART&&) noexcept -> HM11Driver<hal::MockUART>;

#ifdef HM11_STM32_PLATFORM
template auto make_hm11_driver<hal::STM32UARTPort>(hal::STM32UARTPort&&) noexcept -> HM11Driver<hal::STM32UARTPort>;
#endif

// ============================================================================
// GATT Service Discovery Implementation
// ============================================================================

/// Implementation of Find_All_Services_UUID streaming response parsing
/// Equivalent to Ada's Find_All_Services_UUID with explicit state machine
/// This method must be defined after the template class definition
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

    // Reset GATT discovery state
    gatt_service_stage_ = GattServiceStage::LookingForHeader;
    gatt_discovery_started_ = false;
    buffer_position_ = 0;

    // Send discovery command
    Transmit("AT+FINDSERVICES?", status);
    if (status != StatusType::Ok) {
        return;
    }

    try {
        // Start streaming receive - equivalent to Ada's As_Stream => True
        hal::UARTData8b response_data{response_buffer_.data(), response_buffer_.size()};

        // TODO: Implement streaming receive with timeout
        // For now, implement simplified blocking receive for basic functionality
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse the response using the state machine
        parse_service_discovery_response(callback, user_data, timeout_ms, status);

    } catch (...) {
        // Stop stream and report error (equivalent to Ada's exception handler)
        status = StatusType::ErrError;
    }
}

/// Parse service discovery streaming response using explicit state machine
/// Equivalent to Ada's Main loop with streaming parsing logic
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_service_discovery_response(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    constexpr std::string_view HEADER = util::gatt_service_discovery::SERVICE_HEADER;
    constexpr std::size_t SERVICE_DATA_LENGTH = util::gatt_service_discovery::SERVICE_UUID_LENGTH;
    constexpr std::string_view CRLF = "\r\n";

    std::size_t pos = 0;
    const std::size_t buffer_end = response_buffer_.size(); // Simplified for non-streaming

    // Main parsing loop equivalent to Ada's Main loop
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        switch (gatt_service_stage_) {
            case GattServiceStage::LookingForHeader: {
                // Look for service discovery header (56 stars)
                if (remaining >= HEADER.size()) {
                    const std::string_view buffer_view{
                        reinterpret_cast<const char*>(response_buffer_.data() + pos),
                        remaining};

                    if (util::starts_with(buffer_view, HEADER)) {
                        if (gatt_discovery_started_) {
                            // End message, exit parsing
                            status = StatusType::Ok;
                            return;
                        }
                        // Start message
                        gatt_discovery_started_ = true;
                        gatt_service_stage_ = GattServiceStage::ParsingService;
                        pos += HEADER.size();
                    } else {
                        ++pos; // Advance to look for header
                    }
                } else {
                    // Not enough data for header
                    ++pos;
                }
                break;
            }

            case GattServiceStage::ParsingService: {
                // Look for CRLF to skip
                if (remaining >= CRLF.size()) {
                    const std::string_view buffer_view{
                        reinterpret_cast<const char*>(response_buffer_.data() + pos),
                        remaining};

                    if (util::starts_with(buffer_view, CRLF)) {
                        pos += CRLF.size();
                        break;
                    }
                }

                // Look for service UUID data
                if (remaining >= SERVICE_DATA_LENGTH) {
                    // Extract service UUID data
                    std::array<char, SERVICE_DATA_LENGTH> service_data{};
                    for (std::size_t i = 0; i < SERVICE_DATA_LENGTH; ++i) {
                        service_data[i] = static_cast<char>(response_buffer_[pos + i]);
                    }

                    const std::string_view service_str{service_data.data(), SERVICE_DATA_LENGTH};

                    // Validate and invoke callback
                    std::string_view start_handle;
                    std::string_view end_handle;
                    std::string_view service_uuid;
                    if (util::parse_service_uuid_components(service_str, start_handle, end_handle, service_uuid)) {
                        callback(service_str, user_data);
                    }

                    pos += SERVICE_DATA_LENGTH;
                    gatt_service_stage_ = GattServiceStage::LookingForHeader;
                } else {
                    // Not enough data
                    ++pos;
                }
                break;
            }

            case GattServiceStage::LookingForEnd:
            default:
                // Should not reach here in current implementation
                ++pos;
                break;
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Discovery Implementation
// ============================================================================

/// Implementation of Find_All_Characteristics_UUID streaming response parsing
/// Equivalent to Ada's Find_All_Characteristic_UUID with explicit state machine
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

    // Reset GATT characteristic discovery state
    gatt_characteristic_stage_ = GattCharacteristicStage::LookingForHeader;
    gatt_characteristic_discovery_started_ = false;
    buffer_position_ = 0;

    // Send discovery command
    Transmit("AT+FINDALLCHARS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    try {
        // Start streaming receive - equivalent to Ada's As_Stream => True
        hal::UARTData8b response_data{response_buffer_.data(), response_buffer_.size()};

        // TODO: Implement streaming receive with timeout
        // For now, implement simplified blocking receive for basic functionality
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse the response using the state machine
        parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);

    } catch (...) {
        // Stop stream and report error (equivalent to Ada's exception handler)
        status = StatusType::ErrError;
    }
}

/// Implementation of Find_Characteristics_UUID with handle range
/// Equivalent to Ada's Find_Characteristic_UUID procedure
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

        // Reset GATT characteristic discovery state
        gatt_characteristic_stage_ = GattCharacteristicStage::LookingForHeader;
        gatt_characteristic_discovery_started_ = false;
        buffer_position_ = 0;

        // Send discovery command
        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        // Start streaming receive - equivalent to Ada's As_Stream => True
        hal::UARTData8b response_data{response_buffer_.data(), response_buffer_.size()};

        // TODO: Implement streaming receive with timeout
        // For now, implement simplified blocking receive for basic functionality
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse the response using the state machine
        parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);

    } catch (...) {
        // Stop stream and report error (equivalent to Ada's exception handler)
        status = StatusType::ErrError;
    }
}

/// Parse characteristic discovery streaming response using explicit state machine
/// Equivalent to Ada's Main loop with streaming parsing logic for characteristics
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    constexpr std::string_view HEADER = util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER;
    constexpr std::size_t CHARACTERISTIC_DATA_LENGTH = util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH;
    constexpr std::string_view CRLF = "\r\n";

    std::size_t pos = 0;
    const std::size_t buffer_end = response_buffer_.size(); // Simplified for non-streaming

    // Main parsing loop equivalent to Ada's Main loop
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        switch (gatt_characteristic_stage_) {
            case GattCharacteristicStage::LookingForHeader: {
                // Look for characteristic discovery header (56 stars)
                if (remaining >= HEADER.size()) {
                    const std::string_view buffer_view{
                        reinterpret_cast<const char*>(response_buffer_.data() + pos),
                        remaining};

                    if (util::starts_with(buffer_view, HEADER)) {
                        if (gatt_characteristic_discovery_started_) {
                            // End message, exit parsing
                            status = StatusType::Ok;
                            return;
                        }
                        // Start message
                        gatt_characteristic_discovery_started_ = true;
                        gatt_characteristic_stage_ = GattCharacteristicStage::ParsingCharacteristic;
                        pos += HEADER.size();
                    } else {
                        ++pos; // Advance to look for header
                    }
                } else {
                    // Not enough data for header
                    ++pos;
                }
                break;
            }

            case GattCharacteristicStage::ParsingCharacteristic: {
                // Look for CRLF to skip
                if (remaining >= CRLF.size()) {
                    const std::string_view buffer_view{
                        reinterpret_cast<const char*>(response_buffer_.data() + pos),
                        remaining};

                    if (util::starts_with(buffer_view, CRLF)) {
                        pos += CRLF.size();
                        break;
                    }
                }

                // Look for characteristic UUID data
                if (remaining >= CHARACTERISTIC_DATA_LENGTH) {
                    // Extract characteristic UUID data
                    std::array<char, CHARACTERISTIC_DATA_LENGTH> characteristic_data{};
                    for (std::size_t i = 0; i < CHARACTERISTIC_DATA_LENGTH; ++i) {
                        characteristic_data[i] = static_cast<char>(response_buffer_[pos + i]);
                    }

                    const std::string_view characteristic_str{characteristic_data.data(), CHARACTERISTIC_DATA_LENGTH};

                    // Validate and invoke callback
                    std::string_view handle;
                    util::CharacteristicProperty properties = {};
                    std::string_view characteristic_uuid;
                    if (util::parse_characteristic_uuid_components(
                            characteristic_str,
                            handle,
                            properties,
                            characteristic_uuid)) {
                        callback(characteristic_str, user_data);
                    }

                    pos += CHARACTERISTIC_DATA_LENGTH;
                    // Continue parsing characteristics - don't transition back to LookingForHeader
                    // The state machine will naturally transition when it encounters the end header
                } else {
                    // Not enough data
                    ++pos;
                }
                break;
            }

            case GattCharacteristicStage::LookingForEnd:
            default:
                // Should not reach here in current implementation
                ++pos;
                break;
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Operations Implementation
// ============================================================================

/// Enable characteristic notification
/// Equivalent to Ada's Enable_Characteristic_Notify procedure
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
        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

/// Disable characteristic notification
/// Equivalent to Ada's Disable_Characteristic_Notify procedure
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
        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

/// Read characteristic value
/// Equivalent to Ada's Read_Characteristic_Notify procedure
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
        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

/// Set method and characteristic handle for data operations
/// Equivalent to Ada's Set_Method_And_Characteristic_Handle procedure
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

/// Send data to characteristic (deprecated)
/// Equivalent to Ada's Send_Data_To_Characteristic procedure
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

/// Parse notification response from characteristic operations
/// Equivalent to Ada's Read_Characteristic_Notify_Responce procedure
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_notify_response(StatusType& status, NotifyResponse& response) noexcept {
    constexpr std::string_view SEND_OK = "OK+SEND-OK";
    constexpr std::string_view DATA_ER = "OK+DATA-ER";
    constexpr std::size_t RESPONSE_LENGTH = 10; // "OK+SEND-OK" or "OK+DATA-ER"

    response = NotifyResponse::SendEr; // Initialize to error state

    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), RESPONSE_LENGTH};
    uart_.Receive(response_data, status);

    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), RESPONSE_LENGTH};

    if (response_str == SEND_OK) {
        response = NotifyResponse::SendOk;
    } else if (response_str == DATA_ER) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::SendEr;
        status = StatusType::ErrError;
    }
}

// ============================================================================
// Advanced Configuration Management Implementation
// ============================================================================

// Power Management Configuration - implementations moved to header as inline methods

// get_module_power moved to header as inline

/* ADVANCED CONFIGURATION TEMPLATE IMPLEMENTATIONS MOVED TO HEADER AS INLINE METHODS
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_output_power(OutputPower power, StatusType& status) noexcept {
    try {
        const auto power_str = util::to_string(power);
        util::ATCommandBuilder cmd;
        cmd.append("AT+POWR").append(power_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(power_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_output_power(OutputPower& power, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+POWR?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_power = util::parse_output_power(parsed_response->front());
    if (!parsed_power.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    power = *parsed_power;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_module_auto_sleep(bool sleep, StatusType& status) noexcept {
    try {
        const auto sleep_str = util::to_string(sleep);
        util::ATCommandBuilder cmd;
        cmd.append("AT+PWRM").append(sleep_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(sleep_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_module_auto_sleep(bool& sleep, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+PWRM?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(*parsed_response);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    sleep = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::sleep(StatusType& status) noexcept {
    TransmitAndCheck("AT+SLEEP", "OK+SLEEP", status);
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::wake_up(StatusType& status) noexcept {
    TransmitAndCheck("AT", "OK", status);
}

// Advertising Configuration
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_advertising_interval(AdvertisingInterval interval, StatusType& status) noexcept {
    try {
        const auto interval_char = util::to_char(interval);
        util::ATCommandBuilder cmd;
        cmd.append("AT+ADVI").append(interval_char);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(interval_char);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_advertising_interval(AdvertisingInterval& interval, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+ADVI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_interval = util::parse_advertising_interval(parsed_response->front());
    if (!parsed_interval.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    interval = *parsed_interval;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_advertising_type(AdvertisingType type, StatusType& status) noexcept {
    try {
        const auto type_str = util::to_string(type);
        util::ATCommandBuilder cmd;
        cmd.append("AT+ADTY").append(type_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(type_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_advertising_type(AdvertisingType& type, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+ADTY?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_type = util::parse_advertising_type(parsed_response->front());
    if (!parsed_type.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    type = *parsed_type;
}

// Continue with a more concise approach for remaining methods to save space
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_advertising_flag(const AdvertisingFlag& flag, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+FLAG").append(std::string_view(flag));
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(std::string_view(flag));
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_reliable_advertising_mode(AdvertisingMode mode, StatusType& status) noexcept {
    try {
        const auto mode_str = util::to_string(mode);
        util::ATCommandBuilder cmd;
        cmd.append("AT+RELI").append(mode_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(mode_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_reliable_advertising_mode(AdvertisingMode& mode, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for mode character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+RELI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_mode = util::parse_advertising_mode(parsed_response->front());
    if (!parsed_mode.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    mode = *parsed_mode;
    status = StatusType::ErrError; // Placeholder
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_module_advertisement_data(const AdvertisementData& data, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+MARJ").append(std::string_view(data));
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(std::string_view(data));
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// UART Configuration
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_uart_baud_rate(UartBaudRate rate, StatusType& status) noexcept {
    try {
        const auto rate_str = util::to_string(rate);
        util::ATCommandBuilder cmd;
        cmd.append("AT+BAUD").append(rate_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(rate_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_uart_baud_rate(UartBaudRate& rate, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for rate character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BAUD?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_rate = util::parse_uart_baud_rate(parsed_response->front());
    if (!parsed_rate.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    rate = *parsed_rate;
    status = StatusType::ErrError; // Placeholder
}

// UART Configuration Implementation
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_parity_bit(ParityBit parity, StatusType& status) noexcept {
    try {
        const auto parity_str = util::to_string(parity);

        util::ATCommandBuilder cmd;
        cmd.append("AT+PARI").append(parity_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(parity_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_parity_bit(ParityBit& parity, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for parity character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+PARI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_parity = util::parse_parity_bit(parsed_response->front());
    if (!parsed_parity.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    parity = *parsed_parity;
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::reset(StatusType& status) noexcept {
    TransmitAndCheck("AT+RENEW", "OK+RENEW", status);
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::restart(StatusType& status) noexcept {
    TransmitAndCheck("AT+RESET", "OK+RESET", status);
}
END OF ADVANCED CONFIGURATION TEMPLATE IMPLEMENTATIONS */

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_mac_address(MacAddress& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 12; // +12 for MAC address

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+ADDR?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->size() != 12) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = MacAddress(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::start_working(StatusType& status) noexcept {
    TransmitAndCheck("AT+START", "OK+START", status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::clear_last_connected_address(StatusType& status) noexcept {
    TransmitAndCheck("AT+CLEAR", "OK+CLEAR", status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_stop_bit(StopBit value, StatusType& status) noexcept {
    try {
        const auto stop_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+STOP").append(stop_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(stop_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_stop_bit(StopBit& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for stop bit character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+STOP?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_stop_bit = util::parse_stop_bit(parsed_response->front());
    if (!parsed_stop_bit.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_stop_bit;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_uart_flow_control_switch(bool switch_enabled, StatusType& status) noexcept {
    try {
        const auto switch_str = util::to_string(switch_enabled);

        util::ATCommandBuilder cmd;
        cmd.append("AT+FLOW").append(switch_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(switch_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_uart_flow_control_switch(bool& switch_enabled, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+FLOW?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    switch_enabled = parsed_bool.value;
}

// Connection Parameter Management Implementation
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval value,
    StatusType& status) noexcept {
    try {
        const auto interval_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COMI").append(interval_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(interval_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for interval character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COMI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_interval = util::parse_link_layer_connection_interval(parsed_response->front());
    if (!parsed_interval.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_interval;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval value,
    StatusType& status) noexcept {
    try {
        const auto interval_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COMA").append(interval_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(interval_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for interval character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COMA?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_interval = util::parse_link_layer_connection_interval(parsed_response->front());
    if (!parsed_interval.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_interval;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_link_layer_connection_slave_latency(
    LayerConnectionLatency value,
    StatusType& status) noexcept {
    try {
        // Convert uint8_t to string using simple arithmetic
        util::ATCommandBuilder cmd;
        cmd.append("AT+COLA").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_link_layer_connection_slave_latency(
    LayerConnectionLatency& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for latency character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COLA?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse single digit (0-4)
    const char latency_char = parsed_response->front();
    if (latency_char >= '0' && latency_char <= '4') {
        try {
            result = LayerConnectionLatency(static_cast<uint8_t>(latency_char - '0'));
        } catch (...) {
            status = StatusType::ErrError;
        }
    } else {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_connection_supervision_timeout(
    ConnectionSupervisionTimeout value,
    StatusType& status) noexcept {
    try {
        const auto timeout_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COSU").append(timeout_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(timeout_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_connection_supervision_timeout(
    ConnectionSupervisionTimeout& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for timeout character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COSU?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_timeout = util::parse_connection_supervision_timeout(parsed_response->front());
    if (!parsed_timeout.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_timeout;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_update_connection(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COUP").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_update_connection(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COUP?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

// White List Management Implementation
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_white_list_switch(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+WHLI").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_white_list_switch(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+WHLI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_white_list_mac_addresses(
    MacWhiteListIndex index,
    const MacAddress& value,
    StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+AD").append_int(index.value()).append(std::string_view(value));

        util::ATCommandBuilder expected;
        expected.append("OK+Set:");

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_white_list_mac_address(
    MacWhiteListIndex index,
    MacAddress& result,
    StatusType& status) noexcept {
    try {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 12; // +12 for MAC address

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        util::ATCommandBuilder cmd;
        cmd.append("AT+AD").append_int(index.value()).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->size() != 12) {
            status = StatusType::ErrError;
            return;
        }

        result = MacAddress(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// PIO Control Implementation
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pio_output_status(PioNumber pio, PioOutput output, StatusType& status) noexcept {
    try {
        const auto output_str = util::to_string(output);

        util::ATCommandBuilder cmd;
        cmd.append("AT+PIO").append_int(pio.value()).append(output_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(pio.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pio_output_status(PioNumber pio, PioOutput& output, StatusType& status) noexcept {
    try {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for output character

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        util::ATCommandBuilder cmd;
        cmd.append("AT+PIO").append_int(pio.value()).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_output = util::parse_pio_output(parsed_response->front());
        if (!parsed_output.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        output = *parsed_output;
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pios_output_status(const PioNumbers& pios, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+PIOS").append(std::string_view(pios));

        util::ATCommandBuilder expected;
        expected.append("OK+Set:");

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pios_output_status(PioNumbers& pios, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 3; // +3 for PIO numbers

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+PIOS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->size() != 3) {
        status = StatusType::ErrError;
        return;
    }

    try {
        pios = PioNumbers(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pio_collection_rate(PioCollectionRate value, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+CYCL").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pio_collection_rate(PioCollectionRate& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 2; // +2 for max 2-digit rate

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+CYCL?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse rate as integer
    const auto rate_result = util::parse_uint<uint8_t>(parsed_response.value, 10);
    if (!rate_result.has_value() || rate_result.value > 99) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = PioCollectionRate(rate_result.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// Additional Configuration Implementation
template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_battery_monitor_switch(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+BATC").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_battery_monitor_switch(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BATC?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_battery_information(Percent value, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+BATT").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::query_battery_information(Percent& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 3; // +3 for max 3-digit percentage

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BATT?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse percentage as integer
    const auto percent_result = util::parse_uint<uint8_t>(parsed_response.value, 10);
    if (!percent_result.has_value() || percent_result.value > 100) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = Percent(percent_result.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// Explicit template instantiations for GATT service discovery methods
template void
HM11Driver<hal::MockUART>::find_all_services_uuid(ServiceUuidCallback, void*, uint32_t, hal::UARTStatus&) noexcept;

template void HM11Driver<hal::MockUART>::parse_service_discovery_response(
    ServiceUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

// Explicit template instantiations for GATT characteristic discovery methods
template void HM11Driver<hal::MockUART>::find_all_characteristics_uuid(
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::MockUART>::find_characteristics_uuid(
    const HandleType&,
    const HandleType&,
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::MockUART>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

// Explicit template instantiations for GATT characteristic operations
template void
HM11Driver<hal::MockUART>::enable_characteristic_notify(const HandleType&, hal::UARTStatus&, NotifyResponse&) noexcept;

template void
HM11Driver<hal::MockUART>::disable_characteristic_notify(const HandleType&, hal::UARTStatus&, NotifyResponse&) noexcept;

template void
HM11Driver<hal::MockUART>::read_characteristic_notify(const HandleType&, hal::UARTStatus&, NotifyResponse&) noexcept;

template void HM11Driver<hal::MockUART>::set_method_and_characteristic_handle(
    const HandleType&,
    SendDataMethod,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::MockUART>::send_data_to_characteristic(
    const HandleType&,
    SendDataCharacteristic,
    hal::UARTData8bConst,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::MockUART>::parse_characteristic_notify_response(
    hal::UARTStatus&,
    NotifyResponse&) noexcept;

// Advanced configuration methods are now implemented inline in header

#ifdef HM11_STM32_PLATFORM
template void
HM11Driver<hal::STM32UARTPort>::find_all_services_uuid(ServiceUuidCallback, void*, uint32_t, hal::UARTStatus&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_service_discovery_response(
    ServiceUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

// STM32 template instantiations for GATT characteristic discovery methods
template void HM11Driver<hal::STM32UARTPort>::find_all_characteristics_uuid(
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::find_characteristics_uuid(
    const HandleType&,
    const HandleType&,
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback,
    void*,
    uint32_t,
    hal::UARTStatus&) noexcept;

// STM32 template instantiations for GATT characteristic operations
template void HM11Driver<hal::STM32UARTPort>::enable_characteristic_notify(
    const HandleType&,
    hal::UARTStatus&,
    NotifyResponse&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::disable_characteristic_notify(
    const HandleType&,
    hal::UARTStatus&,
    NotifyResponse&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::read_characteristic_notify(
    const HandleType&,
    hal::UARTStatus&,
    NotifyResponse&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::set_method_and_characteristic_handle(
    const HandleType&,
    SendDataMethod,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::send_data_to_characteristic(
    const HandleType&,
    SendDataCharacteristic,
    hal::UARTData8bConst,
    hal::UARTStatus&) noexcept;

template void HM11Driver<hal::STM32UARTPort>::parse_characteristic_notify_response(
    hal::UARTStatus&,
    NotifyResponse&) noexcept;

#endif

} // namespace hm11