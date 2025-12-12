/**
 * @file characteristic_notify.cpp
 * @brief Implementation of GATT characteristic notification operations
 *
 * This file contains the implementation of methods to enable, disable, and read
 * notifications from BLE characteristics. These operations allow a central device
 * to receive updates from a peripheral device without having to poll for changes.
 *
 * Implementation follows the design decisions outlined in the modernization specs,
 * particularly maintaining Ada's strong type system and error handling approach.
 */

#include "hm11/hm11_driver.hpp"

namespace hm11 {

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::enable_characteristic_notify(
    const gatt::HandleType& handle,
    UartStatus& status,
    gatt::NotifyResponse& response) {
    // Construct and transmit the AT command with the characteristic handle
    std::string command = "AT+NOTIFY_ON";
    command += std::string(handle.data(), handle.size());
    transmit(command, status);

    // Process the response
    read_characteristic_notify_response(status, response);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::disable_characteristic_notify(
    const gatt::HandleType& handle,
    UartStatus& status,
    gatt::NotifyResponse& response) {
    // Construct and transmit the AT command with the characteristic handle
    std::string command = "AT+NOTIFYOFF";
    command += std::string(handle.data(), handle.size());
    transmit(command, status);

    // Process the response
    read_characteristic_notify_response(status, response);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::read_characteristic_notify(
    const gatt::HandleType& handle,
    UartStatus& status,
    gatt::NotifyResponse& response) {
    // Construct and transmit the AT command with the characteristic handle
    std::string command = "AT+READDATA";
    command += std::string(handle.data(), handle.size());
    transmit(command, status);

    // Process the response
    read_characteristic_notify_response(status, response);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::read_characteristic_notify_response(
    UartStatus& status,
    gatt::NotifyResponse& response) {
    // Only process the response if the initial command was successful
    if (status != UartStatus::Ok) {
        return;
    }

    // Constants for matching response strings (same as in Ada implementation)
    static constexpr std::string_view SEND_OK = "OK+SEND-OK";
    static constexpr std::string_view DATA_ER = "OK+DATA-ER";

    // Allocate a buffer for the response (sized to fit the largest expected response)
    std::array<std::uint8_t, SEND_OK.length()> buffer{};

    // Receive the response from the module
    port_.receive(buffer.data(), buffer.size(), status);

    // Only process if receive was successful
    if (status != UartStatus::Ok) {
        return;
    }

    // Convert the response buffer to a string_view for comparison
    std::string_view response_str(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    // Determine the response type based on the received string
    if (response_str == SEND_OK) {
        response = gatt::NotifyResponse::SendOk;
    } else if (response_str == DATA_ER) {
        response = gatt::NotifyResponse::DataError;
    } else {
        response = gatt::NotifyResponse::SendError;
    }
}

// Explicit template instantiations for common UART port types
// These instantiations would be provided elsewhere in the implementation

} // namespace hm11