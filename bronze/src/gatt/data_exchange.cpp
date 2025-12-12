/**
 * @file data_exchange.cpp
 * @brief Implementation of GATT data exchange operations for HM11Driver
 *
 * This file provides the implementation of GATT data exchange operations for the HM11Driver class.
 * These operations allow sending data to BLE characteristics and configuring data transmission settings.
 */

#include <string>
#include <string_view>

#include "../../include/hm11/enum_types.hpp"
#include "../../include/hm11/gatt/gatt.hpp"
#include "../../include/hm11/hm11_driver.hpp"
#include "../../include/hm11/sync/uart_status.hpp"

namespace hm11 {

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_method_and_characteristic_handle(
    const gatt::HandleType& handle,
    gatt::SendDataMethod method,
    UartStatus& status) {
    // Create command: "AT+SET_WAY" + method + handle
    std::string command = "AT+SET_WAY";
    command += to_string(method);
    command += std::string(handle.data(), handle.size());

    // Send command and expect "OK+SEND-OK" response
    transmit_and_check(command, "OK+SEND-OK", status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::send_data_to_characteristic(
    const gatt::HandleType& handle,
    gatt::SendDataCharacteristic method,
    std::span<const std::uint8_t> data,
    UartStatus& status) {
    // Create command: "AT+SEND_DATA" + method + handle
    std::string command = "AT+SEND_DATA";
    command += to_string(method);
    command += std::string(handle.data(), handle.size());

    // Transmit command and data
    transmit(command, data, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_use_characteristic_uuid_count(
    gatt::CharacteristicUUIDCount count,
    UartStatus& status) {
    // Convert count to string
    std::string count_str = to_string(count);

    // Create command: "AT+FFE2" + count_str
    std::string command = "AT+FFE2" + count_str;

    // Create expected response: OK_SET + count_str
    std::string expected = std::string(OK_SET) + count_str;

    // Send command and check response
    transmit_and_check(command, expected, status);
}

// Explicit instantiation for common UART implementations
// This is necessary because we define the template methods in a .cpp file
// Add any additional instantiations needed for your specific UART implementations
#ifdef EXPLICIT_INSTANTIATION
// Add your explicit instantiations here when needed
#endif

} // namespace hm11