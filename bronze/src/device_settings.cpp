/**
 * @file device_settings.cpp
 * @brief Implementation of device settings operations for HM11Driver
 *
 * This file provides the implementation of device settings operations for the HM11Driver class.
 * It includes methods for managing module name, MAC address, UART configuration, and more.
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <thread>

#include "../include/hm11/driver.hpp"
#include "../include/hm11/util/at_command_builder.hpp"
#include "../include/hm11/util/fixed_string.hpp"
#include "../include/hm11/util/response_parser.hpp"

namespace hm11 {

// MAC address operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_mac_address(util::MacAddress& mac, hal::UartStatus& status) {
    // Define the expected prefix in the response
    const std::string_view EXPECT_PREFIX = "OK+ADDR:";

    // Transmit the command
    transmit("AT+ADDR?", status);
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

    // Call the receive handler to get the response with enough space for prefix + MAC address (12 chars)
    constexpr std::size_t MAC_ADDRESS_LENGTH = 12;
    constexpr std::size_t EXPECTED_LENGTH = 8 + MAC_ADDRESS_LENGTH; // "OK+ADDR:" + MAC address length

    receive_handler(port, response.data(), EXPECTED_LENGTH, status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the MAC address from the response
    auto response_view = util::parser::to_string_view(response.data(), EXPECTED_LENGTH);

    // Look for the expected prefix
    auto mac_opt = util::parser::extract_after(response_view, EXPECT_PREFIX);

    if (!mac_opt || mac_opt->empty() || mac_opt->length() < MAC_ADDRESS_LENGTH) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Copy the MAC address to the MacAddress output
    mac = util::MacAddress(std::string(mac_opt->substr(0, MAC_ADDRESS_LENGTH)));
}

// Module name operations

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
    const std::string_view EXPECT_PREFIX = "OK+NAME:";

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
    receive_handler(port, response.data(), EXPECT_PREFIX.length() + 12, status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return {};
    }

    // Extract the name from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for the expected prefix
    auto name_opt = util::parser::extract_after(response_view, EXPECT_PREFIX);

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

// UART baud rate operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_uart_baud_rate(UartBaudRate baud_rate, hal::UartStatus& status) {
    // Construct the command: AT+BAUD[baud_rate]
    std::string command = std::string(util::at::cmd::BAUD);
    command += util::at::to_string(baud_rate);

    // Format for expected response: OK+Set:[baud_rate]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(baud_rate);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_uart_baud_rate(UartBaudRate& baud_rate, hal::UartStatus& status) {
    // Construct the command: AT+BAUD?
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

    // Extract the baud rate from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    bool success = false;
    baud_rate =
        util::parser::extract_enum<UartBaudRate>(response_view, util::at::response::OK_GET_PREFIX, '\0', success);

    if (!success) {
        status = hal::UartStatus::ErrError;
    }
}

// UART flow control operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_uart_flow_control_switch(bool enable, hal::UartStatus& status) {
    // Convert bool to string (0 or 1)
    std::string value = enable ? "1" : "0";

    // Construct the command: AT+FIOW[value]
    std::string command = "AT+FIOW" + value;

    // Format for expected response: OK+Set:[value]
    std::string expected_response = "OK+Set:" + value;

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_uart_flow_control_switch(bool& enabled, hal::UartStatus& status) {
    // Construct the command: AT+FIOW?
    std::string command = "AT+FIOW?";

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

    // Extract the flow control setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    const std::string_view PREFIX = "OK+Get:";
    auto value_opt = util::parser::extract_after(response_view, PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Check if the value is '1'
    enabled = ((*value_opt)[0] == '1');
}

// Parity bit operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_parity_bit(ParityBit parity, hal::UartStatus& status) {
    // Construct the command: AT+PARI[parity]
    std::string command = "AT+PARI";
    command += util::at::to_string(parity);

    // Format for expected response: OK+Set:[parity]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(parity);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_parity_bit(ParityBit& parity, hal::UartStatus& status) {
    // Construct the command: AT+PARI?
    std::string command = "AT+PARI?";

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

    // Extract the parity bit setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    bool success = false;
    parity = util::parser::extract_enum<ParityBit>(response_view, util::at::response::OK_GET_PREFIX, '\0', success);

    if (!success) {
        status = hal::UartStatus::ErrError;
    }
}

// Stop bit operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_stop_bit(StopBit stop_bit, hal::UartStatus& status) {
    // Construct the command: AT+STOP[stop_bit]
    std::string command = "AT+STOP";
    command += util::at::to_string(stop_bit);

    // Format for expected response: OK+Set:[stop_bit]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(stop_bit);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_stop_bit(StopBit& stop_bit, hal::UartStatus& status) {
    // Construct the command: AT+STOP?
    std::string command = "AT+STOP?";

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

    // Extract the stop bit setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    bool success = false;
    stop_bit = util::parser::extract_enum<StopBit>(response_view, util::at::response::OK_GET_PREFIX, '\0', success);

    if (!success) {
        status = hal::UartStatus::ErrError;
    }
}

// Module RX gain operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_module_rx_gain(RxGain gain, hal::UartStatus& status) {
    // Construct the command: AT+GAIN[gain]
    std::string command = "AT+GAIN";
    command += util::at::to_string(gain);

    // Format for expected response: OK+Set:[gain]
    std::string expected_response = "OK+Set:";
    expected_response += util::at::to_string(gain);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_module_rx_gain(RxGain& gain, hal::UartStatus& status) {
    // Construct the command: AT+GAIN?
    std::string command = "AT+GAIN?";

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

    // Extract the RX gain setting from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    bool success = false;
    gain = util::parser::extract_enum<RxGain>(response_view, util::at::response::OK_GET_PREFIX, '\0', success);

    if (!success) {
        status = hal::UartStatus::ErrError;
    }
}

// Service UUID operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_service_uuid(std::string_view uuid, hal::UartStatus& status) {
    // Check UUID length (typically 4 characters for a short UUID)
    if (uuid.empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Construct the command: AT+UUID0x[uuid]
    std::string command = "AT+UUID0x" + std::string(uuid);

    // Format for expected response: OK+Set:0x[uuid]
    std::string expected_response = "OK+Set:0x" + std::string(uuid);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_service_uuid(std::string& uuid, hal::UartStatus& status) {
    // Construct the command: AT+UUID?
    std::string command = "AT+UUID?";

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

    // Call the receive handler to get the response with enough space for prefix + UUID
    // UUID response is typically "OK+Get:0xXXXX" where XXXX is the UUID
    constexpr std::size_t MAX_UUID_LENGTH = 32; // Allow for long UUIDs

    receive_handler(
        port,
        response.data(),
        util::at::response::OK_GET_PREFIX.length() + 2 + MAX_UUID_LENGTH, // +2 for "0x"
        status,
        std::chrono::milliseconds(1000),
        false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the UUID from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    auto value_opt = util::parser::extract_after(response_view, util::at::response::OK_GET_PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // The UUID is prefixed with "0x", so skip those characters
    if (value_opt->length() >= 2 && (*value_opt)[0] == '0' && (*value_opt)[1] == 'x') {
        uuid = std::string(value_opt->substr(2));
    } else {
        uuid = std::string(*value_opt);
    }
}

// Characteristic operations

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::set_characteristic(std::string_view characteristic, hal::UartStatus& status) {
    // Check characteristic length (typically 4 characters)
    if (characteristic.empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Construct the command: AT+CHAR0x[characteristic]
    std::string command = "AT+CHAR0x" + std::string(characteristic);

    // Format for expected response: OK+Set:0x[characteristic]
    std::string expected_response = "OK+Set:0x" + std::string(characteristic);

    // Send the command and check the response
    transmit_and_check(command, expected_response, status);
}

template <hm11::hal::UART_Implementation UARTImpl>
void hm11::HM11Driver<UARTImpl>::get_characteristic(std::string& characteristic, hal::UartStatus& status) {
    // Construct the command: AT+CHAR?
    std::string command = "AT+CHAR?";

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

    // Call the receive handler to get the response with enough space for prefix + characteristic
    // Characteristic response is typically "OK+Get:0xXXXX" where XXXX is the characteristic
    constexpr std::size_t CHARACTERISTIC_LENGTH = 4;

    receive_handler(
        port,
        response.data(),
        util::at::response::OK_GET_PREFIX.length() + 2 + CHARACTERISTIC_LENGTH, // +2 for "0x"
        status,
        std::chrono::milliseconds(1000),
        false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Extract the characteristic from the response
    auto response_view = util::parser::to_string_view(response);

    // Look for "OK+Get:" prefix
    auto value_opt = util::parser::extract_after(response_view, util::at::response::OK_GET_PREFIX);

    if (!value_opt || value_opt->empty()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // The characteristic is prefixed with "0x", so skip those characters
    if (value_opt->length() >= 2 && (*value_opt)[0] == '0' && (*value_opt)[1] == 'x') {
        characteristic = std::string(value_opt->substr(2));
    } else {
        characteristic = std::string(*value_opt);
    }
}

} // namespace hm11