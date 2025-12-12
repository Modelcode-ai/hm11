/**
 * @file hm11_driver.cpp
 * @brief Implementation of the HM11Driver class
 *
 * This file provides the implementation of the HM11Driver class methods.
 * It contains the core functionality for communicating with the HM-11/cc2541
 * Bluetooth Low Energy module.
 */
// NOLINTBEGIN

#include <algorithm>
#include <cassert>
#include <cstring>
#include <thread>

#include "../include/hm11/driver.hpp"
#include "../include/hm11/gatt/gatt.hpp"
#include "../include/hm11/util/at_command_builder.hpp"
#include "../include/hm11/util/fixed_string.hpp"
#include "../include/hm11/util/response_parser.hpp"

namespace hm11 {

// Helper functions for working with buffer positions and parsing data

namespace {
/**
 * @brief Calculate the available data length
 *
 * Calculates the available data length from the current position to
 * the zero position, taking into account buffer wrap-around.
 *
 * @param current Current position in the buffer
 * @param zero Zero position in the buffer
 * @return Size of available data
 */
[[maybe_unused]] static std::size_t calc_length(std::size_t current, std::size_t zero) {
    // Handle buffer wrap-around
    if (current <= zero) {
        return zero - current;
    } else {
        return SIZE_MAX - current + 1 + zero;
    }
}

/**
 * @brief Check if buffer starts with the given string view
 *
 * Checks if the buffer at the given position starts with the specified string view.
 *
 * @param match String to match
 * @param start Start position in the buffer
 * @param max_pos Maximum position to check
 * @return true If buffer starts with the match string
 * @return false Otherwise
 */
template <typename Arr>
bool starts_with(std::string_view match, std::size_t start, std::size_t max_pos, const Arr& buffer) {
    if (match.empty()) {
        return true;
    }

    // Ensure we have enough data
    if (calc_length(start, max_pos + 1) < match.length()) {
        return false;
    }

    // Check each character
    for (std::size_t i = 0; i < match.length(); ++i) {
        std::size_t pos = (start + i) % buffer.size();
        if (static_cast<char>(buffer[pos]) != match[i]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Move the current position forward
 *
 * Moves the current position forward by the specified amount, adjusting
 * the available length and handling buffer wrap-around.
 *
 * @param current Current position (modified)
 * @param amount Amount to move
 * @param zero Zero position
 * @param available Available length (modified)
 */
[[maybe_unused]] static void
move(std::size_t& current, std::size_t amount, [[maybe_unused]] std::size_t zero, std::size_t& available) {
    if (amount > available) {
        amount = available;
    }

    current = (current + amount) % SIZE_MAX;
    available -= amount;
}

/**
 * @brief Find a pattern in the buffer
 *
 * Searches for the specified pattern in the buffer starting at the given position.
 *
 * @param pattern Pattern to find
 * @param start Start position
 * @param max_pos Maximum position to search
 * @return Optional position where pattern is found, or nullopt if not found
 */
template <typename Arr>
std::optional<std::size_t> find(std::string_view pattern, std::size_t start, std::size_t max_pos, const Arr& buffer) {
    if (pattern.empty()) {
        return start;
    }

    std::size_t available = calc_length(start, max_pos + 1);
    if (available < pattern.length()) {
        return std::nullopt;
    }

    // Basic search implementation
    for (std::size_t i = 0; i < available - pattern.length() + 1; ++i) {
        bool found = true;
        for (std::size_t j = 0; j < pattern.length(); ++j) {
            std::size_t pos = (start + i + j) % buffer.size();
            if (static_cast<char>(buffer[pos]) != pattern[j]) {
                found = false;
                break;
            }
        }

        if (found) {
            return (start + i) % SIZE_MAX;
        }
    }

    return std::nullopt;
}

} // anonymous namespace

/**
 * @brief Scan for nearby BLE devices
 *
 * This method performs a scan for nearby Bluetooth Low Energy devices
 * by sending the "AT+DISC?" command. When devices are discovered, the
 * provided callback function is invoked with the discovered device information.
 *
 * The scanning process follows a state machine approach as described in
 * Design Decision 11, with explicit states for parsing the different parts
 * of the discovery data (MAC address, name, RSSI). The method continues
 * scanning until either the timeout is reached or an error occurs.
 *
 * If a timeout occurs during parsing of a device's information, the method
 * will complete the parsing of the current device before returning, in line
 * with Design Decision 28 (soft-stop timeout semantics).
 *
 * @param callback Function to be called when a device is discovered
 * @param timeout_ms Maximum time to scan in milliseconds
 * @param status Output parameter that will be set to the operation status
 */
template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::scan(
    DiscoveryCallback callback,
    std::chrono::milliseconds timeout_ms,
    hal::UartStatus& status) {
    // Constants for matching device information
    static constexpr std::string_view OK_DISCS = "OK+DISCS";
    static constexpr std::string_view OK_DISC = "OK+DISC:";
    static constexpr std::string_view OK_NAME = "OK+NAME:";
    static constexpr std::string_view OK_RSSI = "OK+RSSI:";
    static constexpr std::string_view OK_DISCE = "OK+DISCE";
    static constexpr std::size_t MIN_PREFIX = 8;

    // Reset the discovered info
    discovered_info.reset();

    // Start the scan
    transmit("AT+DISC?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Helper function to call the callback with the current device information
    auto call_callback = [this, &callback]() {
        if (discovered_info.id != ' ') { // Check if we have a valid device
            callback(
                discovered_info.id,
                discovered_info.mac,
                discovered_info.get_name_view(),
                discovered_info.get_rssi_view(),
                nullptr // user_data is nullptr as it's not used in this implementation
            );

            // Reset for next device
            discovered_info.reset();
        }
    };

    // Helper function to append to name buffer
    auto append_name = [this](std::size_t start, std::size_t end) {
        std::size_t count = 0;
        if (start <= end) {
            count = end - start + 1;
        } else {
            count = response.size() - start + end + 1;
        }

        // Limit to available space
        count = std::min(count, discovered_info.name.size() - discovered_info.name_length);

        // Copy characters
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t pos = (start + i) % response.size();
            if (discovered_info.name_length < discovered_info.name.size()) {
                discovered_info.name[discovered_info.name_length++] = static_cast<char>(response[pos]);
            }
        }
    };

    // Helper function to append to RSSI buffer
    auto append_rssi = [this](std::size_t start, std::size_t end) {
        std::size_t count = 0;
        if (start <= end) {
            count = end - start + 1;
        } else {
            count = response.size() - start + end + 1;
        }

        // Limit to available space
        count = std::min(count, discovered_info.rssi.size() - discovered_info.rssi_length);

        // Copy characters
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t pos = (start + i) % response.size();
            if (discovered_info.rssi_length < discovered_info.rssi.size()) {
                discovered_info.rssi[discovered_info.rssi_length++] = static_cast<char>(response[pos]);
            }
        }
    };

    // Begin streaming receive
    hal::AnyUartPort port = std::make_shared<UARTImpl>(uart);
    receive_handler(
        port,
        response.data(),
        response.size(),
        status,
        timeout_ms,
        true // as_stream = true for streaming reception
    );

    // State variables for the scanning state machine
    bool stream_closed = false;
    std::size_t zero_pos;
    std::size_t current_pos = 1; // Start at position 1 (Ada-like 1-based indexing)
    std::size_t available_length;
    ScanStageKind current_stage = ScanStageKind::Selection;

    // Main scanning loop
    while (true) {
        // Get the current read position from the handler
        last_read_handler(stream_closed, zero_pos);

        // Exit if the stream is closed
        if (stream_closed) {
            break;
        }

        // Calculate available data length
        available_length = calc_length(current_pos, zero_pos);

        // Parse data until no more is available
        while (available_length > 0) {
            // Process data based on the current stage
            switch (current_stage) {
                case ScanStageKind::Selection:
                    // Need a minimum amount of data to identify the type
                    if (available_length < MIN_PREFIX) {
                        // Not enough data, exit inner loop and wait for more
                        goto wait_for_more_data;
                    }

                    // Check for the start of device discovery message
                    if (starts_with(OK_DISCS, current_pos, zero_pos - 1, response)) {
                        move(current_pos, OK_DISCS.size(), zero_pos, available_length);
                    }

                    // Check for the start of device details
                    if (starts_with(OK_DISC, current_pos, zero_pos - 1, response)) {
                        move(current_pos, OK_DISC.size(), zero_pos, available_length);
                        current_stage = ScanStageKind::MAC;
                        continue; // Continue to MAC parsing
                    }

                    // Check for the name field
                    if (starts_with(OK_NAME, current_pos, zero_pos - 1, response)) {
                        move(current_pos, OK_NAME.size(), zero_pos, available_length);
                        current_stage = ScanStageKind::Name;
                        continue; // Continue to Name parsing
                    }

                    // Check for the RSSI field
                    if (starts_with(OK_RSSI, current_pos, zero_pos - 1, response)) {
                        move(current_pos, OK_RSSI.size(), zero_pos, available_length);
                        current_stage = ScanStageKind::RSSI;
                        continue; // Continue to RSSI parsing
                    }

                    // Check for end of scan
                    if (starts_with(OK_DISCE, current_pos, zero_pos - 1, response)) {
                        // End of scan, exit the main loop
                        goto scan_complete;
                    }

                    // If none of the above matched, move to the next character and try again
                    move(current_pos, 1, zero_pos, available_length);
                    break;

                case ScanStageKind::MAC:
                    // Call the callback if we have previous data (should not happen in normal flow)
                    call_callback();

                    // Check if we have enough data for a MAC address (12 characters)
                    if (available_length < 12) {
                        // Not enough data, exit inner loop and wait for more
                        goto wait_for_more_data;
                    }

                    // Extract the MAC address
                    {
                        // Temporarily create a buffer for the MAC address
                        char mac_buffer[12] = {0};
                        for (size_t i = 0; i < 12; ++i) {
                            std::size_t pos = (current_pos + i) % response.size();
                            mac_buffer[i] = static_cast<char>(response[pos]);
                        }

                        // Set the MAC address in discovered_info
                        discovered_info.mac = util::MacAddress(std::string_view(mac_buffer, 12));

                        // Move past the MAC address
                        move(current_pos, 12, zero_pos, available_length);
                        current_stage = ScanStageKind::Selection;
                    }
                    break;

                case ScanStageKind::Name:
                    // Look for the end of the name (CR+LF sequence)
                    {
                        auto name_end = find("\r\n", current_pos, zero_pos - 1, response);

                        if (!name_end) {
                            // No end found, append all available data to the name
                            append_name(current_pos, zero_pos - 1);
                            current_pos = zero_pos;
                            available_length = 0;
                        } else {
                            // End found, append up to the end and move past CR+LF
                            append_name(current_pos, *name_end);
                            current_pos = (*name_end + 2) % response.size(); // +2 for CR+LF
                            available_length = calc_length(current_pos, zero_pos);
                            current_stage = ScanStageKind::Selection;
                        }
                    }
                    break;

                case ScanStageKind::RSSI:
                    // Look for the end of the RSSI (CR+LF sequence)
                    {
                        auto rssi_end = find("\r\n", current_pos, zero_pos - 1, response);

                        if (!rssi_end) {
                            // No end found, append all available data to the RSSI
                            append_rssi(current_pos, zero_pos - 1);
                            current_pos = zero_pos;
                            available_length = 0;
                        } else {
                            // End found, append up to the end and move past CR+LF
                            append_rssi(current_pos, *rssi_end);
                            current_pos = (*rssi_end + 2) % response.size(); // +2 for CR+LF
                            available_length = calc_length(current_pos, zero_pos);
                            current_stage = ScanStageKind::Selection;
                        }
                    }
                    break;
            }
        }

    wait_for_more_data:
        // Wait for more data to arrive (will exit loop when last_read_handler is called)
        continue;
    }

scan_complete:
    // Call the callback one last time if we have any pending data
    // This implements Decision 28 (soft-stop timeout semantics)
    call_callback();

    // If the stream wasn't closed due to timeout, finish the receive operation
    if (!stream_closed) {
        hal::AnyUartPort finish_port = std::make_shared<UARTImpl>(uart);
        receive_handler(
            finish_port,
            response.data(),
            response.size(),
            status,
            std::chrono::milliseconds(0), // No timeout needed
            false                         // as_stream = false for normal reception
        );
    }
}

/**
 * @brief Scan for nearby iBeacon devices
 *
 * This method performs a scan specifically for iBeacon devices
 * by sending the "AT+DISI?" command. When iBeacon devices are discovered,
 * the provided callback function is invoked with the discovered device information.
 *
 * Like the scan method, this follows Design Decision 28 for soft-stop timeout
 * semantics, ensuring that any partially parsed device information is delivered
 * via the callback before the method returns on timeout.
 *
 * iBeacon information includes factory ID, UUID, major/minor values, measured power,
 * MAC address, and RSSI.
 *
 * @param callback Function to be called when an iBeacon device is discovered
 * @param timeout_ms Maximum time to scan in milliseconds
 * @param status Output parameter that will be set to the operation status
 */
template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::scan_ibeacon(
    IBeaconDiscoveryCallback callback,
    std::chrono::milliseconds timeout_ms,
    hal::UartStatus& status) {
    // Constants for matching device information
    static constexpr std::string_view OK_DISCS = "OK+DISCS";
    static constexpr std::string_view OK_DISC = "OK+DISC";
    static constexpr std::string_view OK_DISCE = "OK+DISCE";

    // Define the size of the iBeacon data structure
    // Factory ID (8) + UUID (32) + Major/Minor/Power (10) + MAC (12) + RSSI (4)
    static constexpr std::size_t IBEACON_DATA_LENGTH = 8 + 32 + 10 + 12 + 4;

    // Buffer for iBeacon data
    std::array<char, IBEACON_DATA_LENGTH> ibeacon_data{};

    // Start the scan
    transmit("AT+DISI?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Begin streaming receive
    hal::AnyUartPort port = std::make_shared<UARTImpl>(uart);
    receive_handler(
        port,
        response.data(),
        response.size(),
        status,
        timeout_ms,
        true // as_stream = true for streaming reception
    );

    // State variables for scanning
    bool stream_closed = false;
    std::size_t zero_pos;
    std::size_t current_pos = 1; // Start at position 1 (Ada-like 1-based indexing)
    std::size_t available_length;

    // Main scanning loop
    while (true) {
        // Get the current read position from the handler
        last_read_handler(stream_closed, zero_pos);

        // Exit if the stream is closed
        if (stream_closed) {
            break;
        }

        // Calculate available data length
        available_length = calc_length(current_pos, zero_pos);

        // Need a minimum length to process
        if (available_length < OK_DISCS.length()) {
            continue; // Wait for more data
        }

        // Check for end of scan
        if (starts_with(OK_DISCE, current_pos, zero_pos - 1, response)) {
            break; // End of scan
        }

        // Check for start of scan
        if (starts_with(OK_DISCS, current_pos, zero_pos - 1, response)) {
            move(current_pos, OK_DISCS.size(), zero_pos, available_length);
        }

        // Check for iBeacon data
        if (starts_with(OK_DISC, current_pos, zero_pos - 1, response) &&
            available_length >= (OK_DISC.length() + IBEACON_DATA_LENGTH)) {
            // Copy the iBeacon data
            for (std::size_t i = 0; i < IBEACON_DATA_LENGTH; ++i) {
                std::size_t pos = (current_pos + OK_DISC.length() + i - 1) % response.size();
                ibeacon_data[i] = static_cast<char>(response[pos]);
            }

            // Move past this device data
            move(current_pos, OK_DISC.length() + IBEACON_DATA_LENGTH, zero_pos, available_length);

            // Call the callback with the device data
            callback(std::string_view(ibeacon_data.data(), ibeacon_data.size()), nullptr);
        } else {
            // Move forward one character if no match
            move(current_pos, 1, zero_pos, available_length);
        }
    }

    // If the stream wasn't closed due to timeout, finish the receive operation
    if (!stream_closed) {
        hal::AnyUartPort finish_port = std::make_shared<UARTImpl>(uart);
        receive_handler(
            finish_port,
            response.data(),
            response.size(),
            status,
            std::chrono::milliseconds(0), // No timeout needed
            false                         // as_stream = false for normal reception
        );
    }
}

template <hm11::hal::UART_Implementation UARTImpl> void hm11::HM11Driver<UARTImpl>::test(hal::UartStatus& status) {
    transmit_and_check(util::at::cmd::TEST, util::at::response::OK, status);
}

template <hm11::hal::UART_Implementation UARTImpl> void hm11::HM11Driver<UARTImpl>::reset(hal::UartStatus& status) {
    transmit_and_check(util::at::cmd::RENEW, "OK+RENEW", status);
}

template <hm11::hal::UART_Implementation UARTImpl> void hm11::HM11Driver<UARTImpl>::sleep(hal::UartStatus& status) {
    transmit_and_check("AT+SLEEP", "OK+SLEEP", status);
}

template <hm11::hal::UART_Implementation UARTImpl> void hm11::HM11Driver<UARTImpl>::wake_up(hal::UartStatus& status) {
    std::string wake_cmd(7, 'W'); // Command with 7 'W' characters
    transmit_and_check(wake_cmd, "OK+WAKE", status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::software_version(util::VersionType& version, hal::UartStatus& status) {
    // Transmit the command to get software version
    transmit(util::at::cmd::VERSION, status);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the version from the response
    auto response_view = util::parser::to_string_view(response);
    util::parser::extract_version(response_view, version, status);
}

template <hm11::hal::UART_Implementation UARTImpl> void hm11::HM11Driver<UARTImpl>::restart(hal::UartStatus& status) {
    transmit_and_check(util::at::cmd::RESET, "OK+RESET", status);
    // Wait a moment for the module to restart
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_role(Role role, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::ROLE, role);
    transmit_and_check(command, util::at::response::OK, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_role(Role& role, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::ROLE);
    transmit(command, status);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Use the response parser to extract the role
    auto response_view = util::parser::to_string_view(response);
    role = util::parser::extract_role(response_view, status);
}

// Advertising methods removed - they are implemented in full detail below

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_reliable_advertising_mode(AdvertisingMode mode, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::RELI, mode);

    // Transmit the command and check for success response
    transmit_and_check(command, util::at::response::OK, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_reliable_advertising_mode(AdvertisingMode& mode, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::RELI);
    transmit(command, status);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the mode value from the response
    auto response_view = util::parser::to_string_view(response);
    bool success = false;
    mode = util::parser::extract_enum<AdvertisingMode>(response_view, util::at::response::OK_GET_PREFIX, '\0', success);

    if (!success) {
        status = hal::UartStatus::ErrError;
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_advertising_flag(const util::AdvertisingFlag& flag, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::FLAG, flag);

    // Transmit the command and check for success response
    transmit_and_check(command, util::at::response::OK, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::transmit(std::string_view command, hal::UartStatus& status) {
    // Initialize status to OK
    status = hal::UartStatus::Ok;

    // Validate command isn't empty
    if (command.empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Create a vector of bytes from the command string
    std::vector<hal::UInt8> data(command.size());
    std::transform(command.begin(), command.end(), data.begin(), [](char c) { return static_cast<hal::UInt8>(c); });

    // Create a UartData8b view for the data
    hal::UartData8b uart_data(data);

    // Transmit the data using the UART interface
    uart.transmit(uart_data, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::transmit(
    std::string_view command,
    const hal::UartData8b& data,
    hal::UartStatus& status) {
    // Initialize status to OK
    status = hal::UartStatus::Ok;

    // Validate command and data aren't empty
    if (command.empty() || data.size() == 0) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Create a vector of bytes from the command string
    std::vector<hal::UInt8> cmd_bytes(command.size());
    std::transform(command.begin(), command.end(), cmd_bytes.begin(), [](char c) {
        return static_cast<hal::UInt8>(c);
    });

    // Create a combined data array: command + data
    std::vector<hal::UInt8> combined_data;

    try {
        combined_data.reserve(cmd_bytes.size() + data.size());

        // Add the command bytes
        combined_data.insert(combined_data.end(), cmd_bytes.begin(), cmd_bytes.end());

        // Add the additional data bytes
        combined_data.insert(combined_data.end(), data.begin(), data.end());
    } catch (const std::exception&) {
        // Handle memory allocation errors
        status = hal::UartStatus::ErrError;
        return;
    }

    // Create a UartData8b view for the combined data
    hal::UartData8b uart_data(combined_data);

    // Transmit the combined data
    uart.transmit(uart_data, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::check_response(
    void* received,
    std::size_t length,
    std::string_view expect,
    hal::UartStatus& status) {
    // Only proceed if status is already Ok
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Validate inputs
    if (received == nullptr || expect.empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Create a view of the received data
    const auto* data = static_cast<const hal::UInt8*>(received);

    // Check if received data is long enough to contain the expected response
    if (length < expect.length()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Check if the received data matches the expected response
    for (std::size_t i = 0; i < expect.length(); ++i) {
        if (data[i] != static_cast<hal::UInt8>(expect[i])) {
            status = hal::UartStatus::ErrError;
            return;
        }
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
bool hm11::HM11Driver<UARTImpl>::transmit_and_check(
    std::string_view command,
    std::string_view expect,
    hal::UartStatus& status) {
    // Initialize status to OK
    status = hal::UartStatus::Ok;

    // Validate inputs
    if (command.empty() || expect.empty()) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Transmit the command
    transmit(command, status);

    if (status != hal::UartStatus::Ok) {
        return false;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return false;
    }

    // Check if the response matches the expected string
    check_response(response.data(), response.size(), expect, status);

    return status == hal::UartStatus::Ok;
}

template <hm11::hal::UART_Implementation UARTImpl>
bool hm11::HM11Driver<UARTImpl>::transmit_and_check(
    std::string_view command,
    std::string_view expect,
    void* received,
    std::size_t length,
    hal::UartStatus& status) {
    // Initialize status to OK
    status = hal::UartStatus::Ok;

    // Validate inputs
    if (command.empty() || expect.empty() || received == nullptr || length == 0) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Transmit the command
    transmit(command, status);

    if (status != hal::UartStatus::Ok) {
        return false;
    }

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return false;
    }

    // Call the receive handler to get the response
    receive_handler(port, received, length, status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return false;
    }

    // Check if the response matches the expected string
    check_response(received, length, expect, status);

    return status == hal::UartStatus::Ok;
}

// MAC address operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_mac_address(util::MacAddress& mac, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::ADDR);

    // Create a response prefix to expect
    const std::string_view EXPECT_PREFIX = "OK+ADDR:";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Create a buffer to receive the full expected response (prefix + MAC address)
    constexpr std::size_t EXPECTED_LENGTH = 8 + 12; // "OK+ADDR:" + MAC address length

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), EXPECTED_LENGTH, status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the MAC address from the response using the parser
    auto response_view = util::parser::to_string_view(response.data(), EXPECTED_LENGTH);
    util::parser::extract_mac_address(response_view, EXPECT_PREFIX, mac, status);
}

// Advertising interval operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_advertising_interval(AdvertisingInterval interval, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    constexpr std::string_view BASE_CMD = "AT+ADVI";

    // Add the interval value based on enum
    char interval_char = '0'; // Initialize to default value
    switch (interval) {
        case AdvertisingInterval::Ms100:
            interval_char = '0';
            break;
        case AdvertisingInterval::Ms211:
            interval_char = '1';
            break;
        case AdvertisingInterval::Ms252:
            interval_char = '2';
            break;
        case AdvertisingInterval::Ms318:
            interval_char = '3';
            break;
        case AdvertisingInterval::Ms417:
            interval_char = '4';
            break;
        case AdvertisingInterval::Ms546:
            interval_char = '5';
            break;
        case AdvertisingInterval::Ms760:
            interval_char = '6';
            break;
        case AdvertisingInterval::Ms852:
            interval_char = '7';
            break;
        case AdvertisingInterval::Ms1022:
            interval_char = '8';
            break;
        case AdvertisingInterval::Ms1285:
            interval_char = '9';
            break;
        case AdvertisingInterval::Ms2000:
            interval_char = 'A';
            break;
        case AdvertisingInterval::Ms3000:
            interval_char = 'B';
            break;
        case AdvertisingInterval::Ms4000:
            interval_char = 'C';
            break;
        case AdvertisingInterval::Ms5000:
            interval_char = 'D';
            break;
        case AdvertisingInterval::Ms6000:
            interval_char = 'E';
            break;
        case AdvertisingInterval::Ms7000:
            interval_char = 'F';
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }

    // Construct the command and expected response
    std::string command = std::string(BASE_CMD) + interval_char;
    std::string expected_response = "OK+Set:";
    expected_response += interval_char;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_advertising_interval(AdvertisingInterval& interval, hal::UartStatus& status) {
    // Define the advertising interval query command
    constexpr std::string_view BASE_CMD = "AT+ADVI";
    std::string query_command = std::string(BASE_CMD) + "?";

    // Transmit the command
    transmit(query_command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the advertising interval from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    const char INTERVAL_CHAR = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (INTERVAL_CHAR) {
        case '0':
            interval = AdvertisingInterval::Ms100;
            break;
        case '1':
            interval = AdvertisingInterval::Ms211;
            break;
        case '2':
            interval = AdvertisingInterval::Ms252;
            break;
        case '3':
            interval = AdvertisingInterval::Ms318;
            break;
        case '4':
            interval = AdvertisingInterval::Ms417;
            break;
        case '5':
            interval = AdvertisingInterval::Ms546;
            break;
        case '6':
            interval = AdvertisingInterval::Ms760;
            break;
        case '7':
            interval = AdvertisingInterval::Ms852;
            break;
        case '8':
            interval = AdvertisingInterval::Ms1022;
            break;
        case '9':
            interval = AdvertisingInterval::Ms1285;
            break;
        case 'A':
            interval = AdvertisingInterval::Ms2000;
            break;
        case 'B':
            interval = AdvertisingInterval::Ms3000;
            break;
        case 'C':
            interval = AdvertisingInterval::Ms4000;
            break;
        case 'D':
            interval = AdvertisingInterval::Ms5000;
            break;
        case 'E':
            interval = AdvertisingInterval::Ms6000;
            break;
        case 'F':
            interval = AdvertisingInterval::Ms7000;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Advertising type operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_advertising_type(AdvertisingType type, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::ADTY, type);

    // Format for expected response: OK+Set:[type_num]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(type);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_advertising_type(AdvertisingType& type, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::ADTY);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the advertising type from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char type_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (type_char) {
        case '0':
            type = AdvertisingType::AdvertisingScanResponseConnectable;
            break;
        case '1':
            type = AdvertisingType::LastDeviceConnect;
            break;
        case '2':
            type = AdvertisingType::AdvertisingScanResponse;
            break;
        case '3':
            type = AdvertisingType::Advertising;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Module name operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_module_name(std::string_view name, hal::UartStatus& status) {
    // Check name length limit (max 12 characters)
    if (name.length() > 12) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::NAME, name);

    // Format for expected response: OK+Set:[name]
    std::string expected_response = "OK+Set:";
    expected_response += name;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
std::size_t hm11::HM11Driver<UARTImpl>::get_module_name(char* name, std::size_t max_length, hal::UartStatus& status) {
    if (name == nullptr || max_length == 0) {
        status = hal::UartStatus::ErrError;
        return 0;
    }

    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::NAME);

    // Format for expected response: OK+NAME:[name]
    const std::string_view EXPECT_PREFIX = "OK+NAME:";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return 0;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return 0;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return 0;
    }

    // Extract the name from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for the prefix
    auto value_opt = util::parser::extract_after(response_view, EXPECT_PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return 0;
    }

    // Copy the extracted name to the output buffer
    std::string_view name_view = *value_opt;
    std::size_t copy_length = std::min(name_view.length(), max_length - 1); // -1 for null terminator

    std::memcpy(name, name_view.data(), copy_length);
    name[copy_length] = '\0'; // Ensure null termination

    return copy_length;
}

template <hm11::hal::UART_Implementation UARTImpl>
std::string hm11::HM11Driver<UARTImpl>::get_module_name(hal::UartStatus& status) {
    // Buffer to hold the module name (max 12 characters + null terminator)
    std::array<char, 13> name_buffer{};

    // Get the name using the other overload
    std::size_t name_length = get_module_name(name_buffer.data(), name_buffer.size(), status);

    if (status != hal::UartStatus::Ok || name_length == 0) {
        return {};
    }

    // Use braced initializer list
    return {name_buffer.data(), name_length};
}

// Module power operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_module_power(ModulePower power, hal::UartStatus& status) {
    // Convert the string_view to string and create the command
    std::string command = std::string(util::at::cmd::POWE);
    command += std::to_string(static_cast<std::uint8_t>(power));

    // Format for expected response: OK+Set:[power_num]
    std::string expected_response = "OK+Set:";
    expected_response += std::to_string(static_cast<std::uint8_t>(power));

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_module_power(ModulePower& power, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::POWE);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the power setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char power_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (power_char) {
        case '0':
            power = ModulePower::DbmMinus23;
            break;
        case '1':
            power = ModulePower::DbmMinus6;
            break;
        case '2':
            power = ModulePower::Dbm0;
            break;
        case '3':
            power = ModulePower::Dbm6;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Work mode and type operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_work_mode(WorkMode mode, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::MODE, mode);

    // Format for expected response: OK+Set:[mode_num]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(mode);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_work_mode(WorkMode& mode, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::MODE);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the work mode from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char mode_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (mode_char) {
        case '0':
            mode = WorkMode::TransmissionMode;
            break;
        case '1':
            mode = WorkMode::PioCollectionMode;
            break;
        case '2':
            mode = WorkMode::RemoteControlMode;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_work_type(WorkType type, hal::UartStatus& status) {
    // Convert the string_view to string and create the command
    std::string command = std::string(util::at::cmd::IMME);
    command += std::to_string(static_cast<std::uint8_t>(type));

    // Format for expected response: OK+Set:[type_num]
    std::string expected_response = "OK+Set:";
    expected_response += std::to_string(static_cast<std::uint8_t>(type));

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_work_type(WorkType& type, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::IMME);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the work type from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char type_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (type_char) {
        case '0':
            type = WorkType::StartImmediately;
            break;
        case '1':
            type = WorkType::RespondAtCommand;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// UART baudrate operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_uart_baud_rate(UartBaudRate baud_rate, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::BAUD, baud_rate);

    // Format for expected response: OK+Set:[baud_rate_num]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(baud_rate);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_uart_baud_rate(UartBaudRate& baud_rate, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::BAUD);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the baudrate from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char baud_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (baud_char) {
        case '0':
            baud_rate = UartBaudRate::Br9600;
            break;
        case '1':
            baud_rate = UartBaudRate::Br19200;
            break;
        case '2':
            baud_rate = UartBaudRate::Br38400;
            break;
        case '3':
            baud_rate = UartBaudRate::Br57600;
            break;
        case '4':
            baud_rate = UartBaudRate::Br115200;
            break;
        case '5':
            baud_rate = UartBaudRate::Br230400;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Auto-sleep operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_module_auto_sleep(bool enable, hal::UartStatus& status) {
    // For this command, the value is inverted: 0 for enable, 1 for disable
    // This is noted in the HM11 Ada driver where it uses "not Sleep" as the parameter
    std::string command = std::string(util::at::cmd::PWRM);
    command += (enable ? "0" : "1");

    // Format for expected response
    std::string expected_response = "OK+Set:";
    expected_response += (enable ? "0" : "1");

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_module_auto_sleep(bool& enabled, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::PWRM);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the auto-sleep setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // For this command, the value is inverted: 0 for enable, 1 for disable
    char sleep_char = (*value_opt)[0];
    enabled = (sleep_char == '0');
}

// iBeacon operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_ibeacon_switch(bool enable, hal::UartStatus& status) {
    std::string command = std::string(util::at::cmd::IBEA);
    command += (enable ? "1" : "0");

    // Format for expected response
    std::string expected_response = "OK+Set:";
    expected_response += (enable ? "1" : "0");

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_ibeacon_switch(bool& enabled, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::IBEA);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the iBeacon switch setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char switch_char = (*value_opt)[0];
    enabled = (switch_char == '1');
}

// iBeacon UUID operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_ibeacon_uuid(const util::IBeaconUuid& uuid, hal::UartStatus& status) {
    // The UUID is set in 4 parts, each 8 characters long
    for (int i = 0; i < 4; i++) {
        std::string command = "AT+IBE" + std::to_string(i);

        // Create a temporary string to hold the 8-char segment
        std::string segment;
        segment.reserve(8);

        // Copy 8 characters from the UUID at appropriate offset
        for (int j = 0; j < 8; j++) {
            segment.push_back(uuid[(static_cast<std::size_t>(i) * 8) + static_cast<std::size_t>(j)]);
        }

        // Add the hex prefix and segment to the command
        command += "0x" + segment;

        // Format for expected response
        std::string expected_response = "OK+Set:" + std::to_string(i) + "0x" + segment;

        // Send the command and check the response
        transmit_and_check(command, expected_response, status);

        // If any part fails, abort the operation
        if (status != hal::UartStatus::Ok) {
            return;
        }
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_ibeacon_uuid(util::IBeaconUuid& uuid, hal::UartStatus& status) {
    // Initialize the UUID to all zeros
    uuid = util::IBeaconUuid();

    // The UUID is retrieved in 4 parts, each 8 characters long
    for (int i = 0; i < 4; i++) {
        std::string command = "AT+IBE" + std::to_string(i) + "?";

        // Transmit the command
        transmit(command, status);
        if (status != hal::UartStatus::Ok) {
            return;
        }

        // Prepare to receive the response by clearing the buffer
        std::memset(response.data(), 0, response.size());

        // Create a shared port object
        hal::AnyUartPort port;
        try {
            port = std::make_shared<UARTImpl>(uart);
        } catch (const std::exception&) {
            status = hal::UartStatus::ErrError;
            return;
        }

        // Call the receive handler to get the response
        receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

        if (status != hal::UartStatus::Ok) {
            return;
        }

        // Extract the UUID segment from the response
        auto response_view = util::parser::to_string_view(response);

        // Look for "OK+Get:" prefix
        const std::string_view PREFIX = "OK+Get:";
        auto value_opt = util::parser::extract_after(response_view, PREFIX);

        if (!value_opt || value_opt->empty()) {
            status = hal::UartStatus::ErrError;
            return;
        }

        // The value should be in the format "0x12345678", extract the hex part
        std::string_view hex_part = value_opt->substr(2, 8); // Skip "0x" prefix

        // Copy the 8 characters to the appropriate position in the UUID
        for (size_t j = 0; j < 8 && j < hex_part.size(); j++) {
            // Use explicit casting and parentheses for the index calculation
            uuid[(static_cast<std::size_t>(i) * static_cast<std::size_t>(8)) + j] = hex_part[j];
        }
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_ibeacon_major_version(const util::VersionType& version, hal::UartStatus& status) {
    // Create a temporary string to hold the version data
    std::string version_str;
    version_str.reserve(util::VersionType::size());

    // Copy the version characters one by one
    for (std::size_t i = 0; i < util::VersionType::size(); i++) {
        version_str.push_back(version[i]);
    }

    std::string command = "AT+MARJ0x" + version_str;

    // Format for expected response
    std::string expected_response = "OK+Set:0x" + version_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_ibeacon_major_version(util::VersionType& version, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::MARJ);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the major version from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // The value should be in the format "0x1234", extract the hex part
    std::string_view hex_part = value_opt->substr(2, 4); // Skip "0x" prefix

    // Copy the 4 characters to the version
    for (size_t i = 0; i < 4 && i < hex_part.size(); i++) {
        version[i] = hex_part[i];
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_ibeacon_minor_version(const util::VersionType& version, hal::UartStatus& status) {
    // Create a temporary string to hold the version data
    std::string version_str;
    version_str.reserve(util::VersionType::size());

    // Copy the version characters one by one
    for (std::size_t i = 0; i < util::VersionType::size(); i++) {
        version_str.push_back(version[i]);
    }

    std::string command = "AT+MINO0x" + version_str;

    // Format for expected response
    std::string expected_response = "OK+Set:0x" + version_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_ibeacon_minor_version(util::VersionType& version, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::MINO);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the minor version from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // The value should be in the format "0x1234", extract the hex part
    std::string_view hex_part = value_opt->substr(2, 4); // Skip "0x" prefix

    // Copy the 4 characters to the version
    for (size_t i = 0; i < 4 && i < hex_part.size(); i++) {
        version[i] = hex_part[i];
    }
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_ibeacon_measured_power(const util::MeasuredPower& power, hal::UartStatus& status) {
    // Create a temporary string to hold the power data
    std::string power_str;
    power_str.reserve(util::MeasuredPower::size());

    // Copy the power characters one by one
    for (std::size_t i = 0; i < util::MeasuredPower::size(); i++) {
        power_str.push_back(power[i]);
    }

    std::string command = "AT+MEAS0x" + power_str;

    // Format for expected response
    std::string expected_response = "OK+Set:0x" + power_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_ibeacon_measured_power(util::MeasuredPower& power, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::MEAS);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the measured power from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // The value should be in the format "0x12", extract the hex part
    std::string_view hex_part = value_opt->substr(2, 2); // Skip "0x" prefix

    // Copy the 2 characters to the power
    for (size_t i = 0; i < 2 && i < hex_part.size(); i++) {
        power[i] = hex_part[i];
    }
}

// Start working and disconnect operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::start_working(hal::UartStatus& status) {
    // Send the start working command
    transmit_and_check("AT+START", "OK+START", status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::disconnect(hal::UartStatus& status) {
    // Send the disconnect command (use AT command to trigger disconnect)
    transmit_and_check("AT", "OK+LOST", status);
}

/**
 * @brief Set the sensor type connected to the HM11 module
 * 
 * This method configures the type of sensor connected to the HM11 module.
 * This command is only used for HMSensor version.
 * Default is None.
 *
 * @param type The sensor type to set
 * @param status Output parameter that will be set to the operation status
 */
template <hal::UART_Implementation UARTImpl>
void HM11Driver<UARTImpl>::set_sensor_type(SensorType type, hal::UartStatus& status) {
    // Convert the SensorType enum to a string value
    std::string value_str = std::to_string(static_cast<std::uint8_t>(type));
    
    // Construct the command: AT+SENS[value]
    std::string command = "AT+SENS" + value_str;
    
    // Construct the expected response: OK+Set:[value]
    std::string expected_response = "OK+Set:" + value_str;
    
    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

/**
 * @brief Get the current sensor type connected to the HM11 module
 * 
 * This method retrieves the type of sensor currently configured on the HM11 module.
 * This command is only used for HMSensor version.
 *
 * @param type Output parameter that will be set to the current sensor type
 * @param status Output parameter that will be set to the operation status
 */
template <hal::UART_Implementation UARTImpl>
void HM11Driver<UARTImpl>::get_sensor_type(SensorType& type, hal::UartStatus& status) {
    // Construct the query command: AT+SENS?
    std::string command = "AT+SENS?";
    
    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());
    
    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);
    
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Extract the sensor type from the response
    auto response_view = util::parser::to_string_view(response);
    
    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);
    
    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // The value should be a single character representing the sensor type
    char type_char = (*value_opt)[0];
    
    // Convert the character to the appropriate enum value
    switch (type_char) {
        case '0':
            type = SensorType::None;
            break;
        case '1':
            type = SensorType::Dht11;
            break;
        case '2':
            type = SensorType::Ds18b20;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

/**
 * @brief Get the temperature and humidity from the connected sensor
 * 
 * This method retrieves the current temperature and humidity readings from
 * the sensor connected to the HM11 module. The readings are returned as
 * numeric values matching Ada's approach.
 * 
 * This command is only used for HMSensor version and requires a sensor to be
 * connected and configured.
 *
 * @param temperature Output parameter that will be set to the current temperature (0-120)
 * @param humidity Output parameter that will be set to the current humidity (0-100)
 * @param status Output parameter that will be set to the operation status
 */
template <hal::UART_Implementation UARTImpl>
void HM11Driver<UARTImpl>::get_sensor_temperature_and_humidity(
    util::TemperatureType& temperature,
    util::HumidityType& humidity,
    hal::UartStatus& status) {
    
    // Construct the query command: AT+TEHU?
    std::string command = "AT+TEHU?";
    
    // Prepare buffer for response with enough space for the expected data
    // Response format: OK+Get:[temp][humidity]
    // Example: OK+Get:025060 (temperature 25, humidity 60)
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());
    
    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // Call the receive handler to get the response - we expect at least OK+Get: (7 chars) + 6 chars for temp/humidity data
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);
    
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Extract the temperature and humidity values from the response
    auto response_view = util::parser::to_string_view(response);
    
    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);
    
    if (!value_opt || value_opt->size() < 6) { // Need at least 6 characters (3 for temp, 3 for humidity)
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // Extract temperature (first 3 chars) and humidity (next 3 chars)
    try {
        std::string_view temp_str = value_opt->substr(0, 3);
        std::string_view humi_str = value_opt->substr(3, 3);
        
        // Parse temperature and humidity values
        int temp_val = std::stoi(std::string(temp_str));
        int humi_val = std::stoi(std::string(humi_str));
        
        // Create RangedInteger values to ensure type safety
        temperature = util::TemperatureType(static_cast<std::uint8_t>(temp_val));
        humidity = util::HumidityType(static_cast<std::uint8_t>(humi_val));
    } catch (const std::exception&) {
        // Handle parsing errors
        status = hal::UartStatus::ErrError;
        return;
    }
}

/**
 * @brief Set the work interval for the sensor
 * 
 * This method configures the work interval for the sensor connected to the HM11 module.
 * The interval determines how often sensor readings are taken.
 * This command is only used for HMSensor version.
 * Default is "00" (0 minutes).
 *
 * @param interval The work interval to set (00-99 minutes)
 * @param status Output parameter that will be set to the operation status
 */
template <hal::UART_Implementation UARTImpl>
void HM11Driver<UARTImpl>::set_sensor_work_interval(const util::WorkInterval& interval, hal::UartStatus& status) {
    // Construct the command: AT+RAT[interval]
    std::string command = "AT+RAT";
    
    // Convert WorkInterval to std::string_view for easy access
    std::string_view interval_view = interval;
    
    // First check if interval starts with '0'
    if (interval_view[0] == '0') {
        // Use only the second digit
        command += interval_view[1];
        
        // Expected response format: OK+Set:[second_digit]
        std::string expected_response = "OK+Set:";
        expected_response += interval_view[1];
        
        // Send the command and check the response
        transmit_and_check(command, expected_response, status);
    } else {
        // Use both digits
        command += std::string(interval_view);
        
        // Expected response format: OK+Set:[interval]
        std::string expected_response = "OK+Set:";
        expected_response += std::string(interval_view);
        
        // Send the command and check the response
        transmit_and_check(command, expected_response, status);
    }
}

/**
 * @brief Get the current work interval for the sensor
 * 
 * This method retrieves the current work interval configured for the sensor
 * connected to the HM11 module.
 * This command is only used for HMSensor version.
 *
 * @param interval Output parameter that will be set to the current work interval
 * @param status Output parameter that will be set to the operation status
 */
template <hal::UART_Implementation UARTImpl>
void HM11Driver<UARTImpl>::get_sensor_work_interval(util::WorkInterval& interval, hal::UartStatus& status) {
    // Initialize interval to "00" (default value)
    interval = util::WorkInterval("00");
    
    // Construct the query command: AT+RAT??
    std::string command = "AT+RAT??";
    
    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());
    
    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);
    
    if (status != hal::UartStatus::Ok) {
        return;
    }
    
    // Extract the work interval from the response
    auto response_view = util::parser::to_string_view(response);
    
    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);
    
    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    // Check if response has one or two digits
    if (value_opt->length() == 1) {
        // Single digit - set first character to '0' and second to the value
        interval[0] = '0';
        interval[1] = (*value_opt)[0];
    } else if (value_opt->length() == 2) {
        // Two digits - copy both characters
        interval[0] = (*value_opt)[0];
        interval[1] = (*value_opt)[1];
    } else {
        // Invalid response format
        status = hal::UartStatus::ErrError;
        return;
    }
}

// Explicit template instantiations for commonly used UART implementations
// This will be expanded as needed for different platforms
// Include mock implementations for testing
#ifdef BUILD_TESTS
#include "../tests/mocks/hm11_driver_mock.hpp"
#include "../tests/mocks/uart_mock.hpp"

// Explicit instantiation for EnhancedUARTMock
template class HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>;
#endif

/**
 * @brief Find all service UUIDs on a connected device
 *
 * This method sends the "AT+FINDSERVICES?" command to discover all service UUIDs
 * available on the currently connected peripheral device. When services are discovered,
 * the provided callback function is invoked with the service information.
 *
 * This implements both Task 2 requirements and aligns with Design Decision 2 (Service
 * and Characteristic UUID Management) by using the FixedString template with validators for UUIDs.
 * It also follows Decision 28 (soft-stop timeout semantics), ensuring that any partially
 * parsed service information is delivered via the callback before the method returns on timeout.
 *
 * @param callback Function to be called for each discovered service
 * @param user_data Optional user data to be passed to the callback
 * @param timeout_ms Maximum time to wait for service discovery in milliseconds
 * @param status Output parameter for operation status
 */
template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::find_all_services_uuid(
    gatt::ServiceUUIDCallback callback,
    void* user_data,
    std::chrono::milliseconds timeout_ms,
    hal::UartStatus& status) {
    // Constants for service discovery
    static constexpr std::string_view HEADER = "********************************************************";
    static constexpr std::size_t SERVICE_UUID_LENGTH = 14;

    // Flag to track if we've started receiving services
    bool started = false;

    // Buffer for service UUID data (14 characters)
    std::array<char, SERVICE_UUID_LENGTH> service_uuid_data{};

    // Start the service discovery
    transmit("AT+FINDSERVICES?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Begin streaming receive
    hal::AnyUartPort port = std::make_shared<UARTImpl>(uart);
    receive_handler(
        port,
        response.data(),
        response.size(),
        status,
        timeout_ms,
        true // as_stream = true for streaming reception
    );

    // State variables for parsing
    bool stream_closed = false;
    std::size_t zero_pos;
    std::size_t current_pos = 1; // Start at position 1 (Ada-like 1-based indexing)
    std::size_t available_length;

    // Main loop for processing service discovery data
    while (true) {
        // Get the current read position from the handler
        last_read_handler(stream_closed, zero_pos);

        // Exit if the stream is closed
        if (stream_closed) {
            break;
        }

        // Calculate available data length
        available_length = calc_length(current_pos, zero_pos);

        // Process data until we don't have enough
        while (available_length > 0) {
            // Check if we have enough data for meaningful processing
            if (available_length < SERVICE_UUID_LENGTH) {
                // Not enough data, exit inner loop and wait for more
                break;
            }

            if (response[current_pos] == '*') {
                // Check for header pattern
                if (starts_with(HEADER, current_pos, zero_pos - 1, response)) {
                    if (started) {
                        // End of message, exit the main loop
                        goto discovery_complete;
                    } else {
                        // Start of message
                        started = true;
                        move(current_pos, HEADER.size(), zero_pos, available_length);
                    }
                }
            } else if (starts_with("\r\n", current_pos, zero_pos - 1, response)) {
                // Skip CR+LF sequences
                move(current_pos, 2, zero_pos, available_length);
            } else {
                // If we have enough data for a service UUID entry
                if (available_length >= SERVICE_UUID_LENGTH) {
                    // Copy the service UUID data
                    for (std::size_t i = 0; i < SERVICE_UUID_LENGTH; ++i) {
                        std::size_t pos = (current_pos + i) % response.size();
                        service_uuid_data[i] = static_cast<char>(response[pos]);
                    }

                    // Move past the service UUID
                    move(current_pos, SERVICE_UUID_LENGTH, zero_pos, available_length);

                    // Call the callback with the service data
                    if (callback) {
                        callback(std::string(service_uuid_data.data(), service_uuid_data.size()), user_data);
                    }
                }
            }
        }
    }

discovery_complete:
    // If the stream wasn't closed due to timeout, finish the receive operation
    if (!stream_closed) {
        hal::AnyUartPort finish_port = std::make_shared<UARTImpl>(uart);
        receive_handler(
            finish_port,
            response.data(),
            response.size(),
            status,
            std::chrono::milliseconds(0), // No timeout needed
            false                         // as_stream = false for normal reception
        );
    }
}

} // namespace hm11
// PIN code operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_pin_code(const util::PinType& pin, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = util::at::build_set_command(util::at::cmd::PASS, pin);

    // Format for expected response: OK+Set:[pin]
    std::string expected_response = "OK+Set:";

    // Create a string for the pin value
    std::string pin_str;
    pin_str.reserve(util::PinType::size());
    for (size_t i = 0; i < util::PinType::size(); i++) {
        pin_str.push_back(pin[i]);
    }

    // Append the pin to the expected response
    expected_response += pin_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_pin_code(util::PinType& pin, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::PASS);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the PIN code from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty() || value_opt->length() < util::PinType::size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Copy the PIN code characters to the output parameter
    for (size_t i = 0; i < util::PinType::size(); i++) {
        if (i < value_opt->length()) {
            pin[i] = (*value_opt)[i];
        } else {
            pin[i] = '0'; // Default to '0' if response is too short
        }
    }
}

// Bond mode operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_bond_mode(BondMode mode, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = "AT+TYPE" + std::to_string(static_cast<int>(mode));

    // Format for expected response: OK+Set:[mode_num]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(mode);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_bond_mode(BondMode& mode, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = "AT+TYPE?";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the bond mode from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char mode_char = (*value_opt)[0];

    // Convert the character to the appropriate enum value
    switch (mode_char) {
        case '0':
            mode = BondMode::NotNeedPin;
            break;
        case '1':
            mode = BondMode::AuthNotNeedPin;
            break;
        case '2':
            mode = BondMode::AuthWithPin;
            break;
        case '3':
            mode = BondMode::AuthAndBonded;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// White list operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_white_list_switch(bool enable, hal::UartStatus& status) {
    // Create the command with the appropriate value (0 for disable, 1 for enable)
    std::string command = "AT+ALLO";
    command += (enable ? "1" : "0");

    // Format for expected response: OK+Set:[0/1]
    std::string expected_response = "OK+Set:";
    expected_response += (enable ? "1" : "0");

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_white_list_switch(bool& enabled, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = "AT+ALLO?";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the white list switch setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char switch_char = (*value_opt)[0];
    enabled = (switch_char == '1');
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_white_list_mac_address(
    const util::MacWhiteListIndex& index,
    const util::MacAddress& mac,
    hal::UartStatus& status) {
    // Get the index value as a string
    std::string index_str = std::to_string(index.get_value());

    // Create a temporary string to hold the MAC address
    std::string mac_str;
    mac_str.reserve(util::MacAddress::size());

    // Copy the MAC address characters one by one
    for (std::size_t i = 0; i < util::MacAddress::size(); i++) {
        mac_str.push_back(mac[i]);
    }

    // Create the command: AT+AD[index][mac]
    std::string command = "AT+AD" + index_str + mac_str;

    // Format for expected response: OK+Set:[mac]
    std::string expected_response = "OK+Set:" + mac_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_white_list_mac_address(
    const util::MacWhiteListIndex& index,
    util::MacAddress& mac,
    hal::UartStatus& status) {
    // Get the index value as a string
    std::string index_str = std::to_string(index.get_value());

    // Create the command: AT+AD[index]??
    std::string command = "AT+AD" + index_str + "??";

    // Expected prefix in response: OK+AD[index]?:
    std::string expect_prefix = "OK+AD" + index_str + "?:";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response with enough space for the response
    receive_handler(
        port,
        response.data(),
        expect_prefix.length() + util::MacAddress::size(),
        status,
        std::chrono::milliseconds(1000),
        false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the MAC address from the response
    auto response_view = util::parser::to_string_view(response);

    auto mac_opt = util::parser::extract_after(response_view, expect_prefix);

    if (!mac_opt || mac_opt->empty() || mac_opt->length() < util::MacAddress::size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Copy the MAC address to the output parameter
    for (size_t i = 0; i < util::MacAddress::size(); i++) {
        mac[i] = (*mac_opt)[i];
    }
}

// Notification operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_notify_information(bool enable, hal::UartStatus& status) {
    // Create the command with the appropriate value (0 for disable, 1 for enable)
    std::string command = "AT+NOTI";
    command += (enable ? "1" : "0");

    // Format for expected response: OK+Set:[0/1]
    std::string expected_response = "OK+Set:";
    expected_response += (enable ? "1" : "0");

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_notify_information(bool& enabled, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = "AT+NOTI?";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the notification setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char notify_char = (*value_opt)[0];
    enabled = (notify_char == '1');
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_notify_mode(NotifyMode mode, hal::UartStatus& status) {
    // Convert NotifyMode enum to string
    std::string mode_str = (mode == NotifyMode::WithAddress) ? "1" : "0";

    // Create the command: AT+NOTP[mode]
    std::string command = "AT+NOTP" + mode_str;

    // Format for expected response: OK+Set:[mode]
    std::string expected_response = "OK+Set:" + mode_str;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_notify_mode(NotifyMode& mode, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = "AT+NOTP?";

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the notify mode from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    char mode_char = (*value_opt)[0];
    mode = (mode_char == '1') ? NotifyMode::WithAddress : NotifyMode::WithoutAddress;
}

// Output power operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_output_power(OutputPower power, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = std::string(util::at::cmd::PCTL);
    command += std::to_string(static_cast<std::uint8_t>(power));

    // Format for expected response: OK+Set:[power_num]
    std::string expected_response = "OK+Set:";
    expected_response += std::to_string(static_cast<std::uint8_t>(power));

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_output_power(OutputPower& power, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::PCTL);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the output power setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Convert the character to the appropriate enum value
    char power_char = (*value_opt)[0];

    switch (power_char) {
        case '0':
            power = OutputPower::NormalPower;
            break;
        case '1':
            power = OutputPower::MaxPower;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Battery monitoring operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_battery_monitor_switch(bool enable, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = std::string(util::at::cmd::BATC);
    command += (enable ? "1" : "0");

    // Format for expected response: OK+Set:[value]
    std::string expected_response = "OK+Set:";
    expected_response += (enable ? "1" : "0");

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_battery_monitor_switch(bool& enabled, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::BATC);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the battery monitor switch setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Convert the character to boolean
    char switch_char = (*value_opt)[0];
    enabled = (switch_char == '1');
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_battery_information(util::Percent level, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = std::string(util::at::cmd::BATT);
    command += std::to_string(static_cast<std::uint8_t>(level));

    // Format for expected response: OK+BATT
    std::string expected_response = "OK+BATT";

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::query_battery_information(util::Percent& level, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::BATT);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the battery level from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Convert the string to an integer
    try {
        int value = std::stoi(std::string(*value_opt));
        if (value >= 0 && value <= 100) {
            level = util::Percent(static_cast<uint8_t>(value));
        } else {
            status = hal::UartStatus::ErrError;
        }
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
    }
}

// UART sleep type operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_uart_sleep_type(UartSleepType type, hal::UartStatus& status) {
    // Use the AT command builder to create the command
    std::string command = std::string(util::at::cmd::UART);
    command += std::to_string(static_cast<std::uint8_t>(type));

    // Format for expected response: OK+Set:[type_num]
    std::string expected_response = "OK+Set:";
    expected_response += std::to_string(static_cast<std::uint8_t>(type));

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_uart_sleep_type(UartSleepType& type, hal::UartStatus& status) {
    // Use the AT command builder to create the query command
    std::string command = util::at::build_query_command(util::at::cmd::UART);

    // Transmit the command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Call the receive handler to get the response
    receive_handler(port, response.data(), response.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the UART sleep type from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Convert the character to the appropriate enum value
    char type_char = (*value_opt)[0];

    switch (type_char) {
        case '0':
            type = UartSleepType::CanWakeUpThroughUart;
            break;
        case '1':
            type = UartSleepType::ShutdownUart;
            break;
        default:
            status = hal::UartStatus::ErrError;
            return;
    }
}

// Module name operations
// These are defined in device_settings.cpp
// Commented out to fix redefinition errors

/*
template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_module_name(std::string_view name, hal::UartStatus& status) {
    // Check name length limit (max 12 characters)
    if (name.length() > 12) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Construct the command: AT+NAME[name]
    std::string command = "AT+NAME" + std::string(name);

    // Format for expected response: OK+Set:[name]
    std::string expected_response = "OK+Set:" + std::string(name);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
std::string hm11::HM11Driver<UARTImpl>::get_module_name(hal::UartStatus& status) {
    // Define the expected prefix in the response
    const std::string_view expect_prefix = "OK+NAME:";

    // Transmit the command
    transmit("AT+NAME?", status);
    if (status != hal::UartStatus::Ok) {
        return {};
    }

    // Prepare to receive the response by clearing the buffer
    std::memset(response.data(), 0, response.size());

    // Create a shared port object
    hal::AnyUartPort port;
    try {
        port = std::make_shared<UARTImpl>(uart);
    } catch (const std::exception&) {
        status = hal::UartStatus::ErrError;
        return {};
    }

    // Call the receive handler to get the response with enough space for prefix + max name length
    receive_handler(
        port,
        response.data(),
        expect_prefix.length() + 12,
        status,
        std::chrono::milliseconds(1000),
        false);

    if (status != hal::UartStatus::Ok) {
        return {};
    }

    // Extract the name from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for the expected prefix
    auto name_opt = util::parser::extract_after(response_view, expect_prefix);

    if (!name_opt || name_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return {};
    }

    // Find the end of the name (null terminator or end of data)
    std::string_view name_view = *name_opt;
    std::size_t length = 0;
    while (length < name_view.length() && name_view[length] != '\0') {
        length++;
    }

    // Return the name as a string
    return std::string(name_view.substr(0, length));
}
*/

// NOLINTEND