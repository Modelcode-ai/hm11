/**
 * @file enhanced_mock_uart.cpp
 * @brief Implementation of enhanced mock UART for HM-11 protocol simulation
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "enhanced_mock_uart.hpp"

#include <algorithm>
#include <cstring>

namespace hm11::test {

// ============================================================================
// UART Port Interface Implementation
// ============================================================================

void EnhancedMockUART::transmit(hal::UartData8bConst data, hal::UartStatus& status, uint32_t /*timeout_ms*/) noexcept {
    // Capture transmitted command
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    last_command_.assign(reinterpret_cast<const char*>(data.data()), data.size());
    transmitted_commands_.push_back(last_command_);

    // Find matching response in queue
    auto match = find_matching_response(last_command_);

    if (match.has_value()) {
        // Store response for next receive() call
        pending_response_ = std::move(match.value());
        status = hal::UartStatus::Ok;
    } else {
        // Unexpected command - record for validation
        record_unexpected_command(last_command_);
        status = hal::UartStatus::Ok; // Still return Ok to allow test to continue
    }
}

void EnhancedMockUART::receive(
    hal::UartData8b data,
    hal::UartStatus& status,
    uint32_t /*timeout_ms*/,
    bool as_stream) noexcept {
    // Check if we have a pending response from transmit
    if (!pending_response_.has_value()) {
        status = hal::UartStatus::ErrTimeout;
        return;
    }

    const ResponseSpec& spec = pending_response_.value();

    // Simulate timeout if programmed
    if (spec.timeout) {
        status = hal::UartStatus::ErrTimeout;
        pending_response_.reset();
        return;
    }

    // Check conditional requirements (e.g., role, connection state)
    if (spec.condition && !spec.condition()) {
        status = hal::UartStatus::ErrError; // Hardware would return error
        pending_response_.reset();
        return;
    }

    // Simulate processing delay
    // In real implementation, this would block or sleep
    // For mock, we just record the delay was programmed
    // (actual timing simulation would require MockTimer integration)

    // Copy response data to output buffer
    const std::size_t TO_COPY = std::min(data.size(), spec.response.size());
    if (TO_COPY > 0) {
        std::memcpy(data.data(), spec.response.data(), TO_COPY);
    }

    status = (TO_COPY > 0 || spec.response.empty()) ? hal::UartStatus::Ok : hal::UartStatus::ErrError;

    // Clear pending response after consumption
    pending_response_.reset();

    // Note: Streaming mode (as_stream=true) would require more sophisticated
    // handling with circular buffer simulation. For now, we handle it the same
    // as blocking mode since most tests use blocking receive.
    (void)as_stream; // Unused for now
}

// ============================================================================
// Response Programming Interface
// ============================================================================

void EnhancedMockUART::expect_command(std::string_view command, std::string_view response) {
    response_queue_.emplace_back(command, response);
}

void EnhancedMockUART::expect_command(
    std::string_view command,
    std::string_view response,
    std::chrono::milliseconds delay) {
    response_queue_.emplace_back(command, response, delay);
}

void EnhancedMockUART::expect_timeout(std::string_view command) {
    response_queue_.push_back(ResponseSpec::timeout_response(command));
}

void EnhancedMockUART::add_response_spec(const ResponseSpec& spec) {
    response_queue_.push_back(spec);
}

// ============================================================================
// HM-11 Protocol Simulation Helpers
// ============================================================================

void EnhancedMockUART::simulate_connection_sequence(std::string_view mac_address) {
    std::string connect_cmd = "AT+CON";
    connect_cmd += mac_address;

    // Connection attempt response with simulated delay
    expect_command(connect_cmd, "OK+CONN", std::chrono::milliseconds(100));

    // Update simulated state
    simulated_connected_ = true;
}

void EnhancedMockUART::simulate_discovery_scan(
    const std::vector<std::tuple<std::string, std::string, std::string>>& devices) {
    // Build discovery response in HM-11 format
    // Format: "OK+DISC:<MAC>\r\n<Name>\r\n<RSSI>\r\n" for each device
    std::string discovery_response = "OK+DISCS";

    for (const auto& [mac, name, rssi] : devices) {
        discovery_response += "OK+DISC:";
        discovery_response += mac;
        discovery_response += "\r\n";
        discovery_response += name;
        discovery_response += "\r\n";
        discovery_response += rssi;
        discovery_response += "\r\n";
    }

    discovery_response += "OK+DISCE"; // End of discovery

    // Program the scan command response
    expect_command("AT+DISC?", discovery_response);
}

void EnhancedMockUART::simulate_gatt_services(const std::vector<std::string>& service_uuids) {
    // Build GATT service discovery response
    // Format: "OK+DISC:" followed by service UUIDs
    std::string response = "OK+DISC:";

    for (std::size_t i = 0; i < service_uuids.size(); ++i) {
        if (i > 0) {
            response += ",";
        }
        response += service_uuids[i];
    }

    expect_command("AT+DISC?", response);
}

void EnhancedMockUART::simulate_gatt_characteristics(const std::vector<std::string>& char_uuids) {
    // Build GATT characteristic discovery response
    // Format: "OK+CHAR:" followed by characteristic UUIDs
    std::string response = "OK+CHAR:";

    for (std::size_t i = 0; i < char_uuids.size(); ++i) {
        if (i > 0) {
            response += ",";
        }
        response += char_uuids[i];
    }

    expect_command("AT+CHAR?", response);
}

void EnhancedMockUART::simulate_sensor_reading(uint8_t temperature, uint8_t humidity) {
    // Build HMSensor reading response
    // Format: "OK+Get:<temp>C <hum>%"
    std::string response = "OK+Get:";
    response += std::to_string(temperature);
    response += "C ";
    response += std::to_string(humidity);
    response += "%";

    expect_command("AT+TEMP?", response);
}

// ============================================================================
// Behavioral Validation Interface
// ============================================================================

std::string_view EnhancedMockUART::get_last_command() const noexcept {
    if (transmitted_commands_.empty()) {
        return "";
    }
    return transmitted_commands_.back();
}

// ============================================================================
// Mock Control
// ============================================================================

void EnhancedMockUART::clear() noexcept {
    response_queue_.clear();
    pending_response_.reset();
    transmitted_commands_.clear();
    last_command_.clear();
    has_unexpected_command_ = false;
    unexpected_command_.clear();
    simulated_role_ = types::Role::Peripheral;
    simulated_connected_ = false;
}

void EnhancedMockUART::clear_transmitted_commands() noexcept {
    transmitted_commands_.clear();
    last_command_.clear();
    has_unexpected_command_ = false;
    unexpected_command_.clear();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::optional<ResponseSpec> EnhancedMockUART::find_matching_response(std::string_view command) {
    if (response_queue_.empty()) {
        return std::nullopt;
    }

    // Match against first queued response (FIFO order)
    ResponseSpec& spec = response_queue_.front();

    if (spec.expected_command == command) {
        // Found matching response - remove from queue and return
        ResponseSpec matched = std::move(spec);
        response_queue_.pop_front();
        return matched;
    }

    // No match found
    return std::nullopt;
}

void EnhancedMockUART::record_unexpected_command(std::string_view command) {
    has_unexpected_command_ = true;
    unexpected_command_ = command;
}

} // namespace hm11::test
