#include "hm11_driver.hpp"
#include "uart_interface.hpp"
#include "utils.hpp"
#include "util/ranged_integer.hpp"
#include "util/fixed_string.hpp"
#include <string>
#include <string_view>
#include <cstddef>
#include <cstdint>

namespace hm11 {

/**
 * @brief Role of the HM11 device
 * 
 * Defines whether the HM11 operates in peripheral or central mode.
 */
enum class Role : std::uint8_t {
    Peripheral, ///< Device acts as a peripheral (server)
    Central     ///< Device acts as a central (client)
};

/**
 * @brief Convert a Role enum to a string
 * 
 * @param role The role to convert
 * @return The string representation of the role
 */
std::string to_string(Role role) {
    return std::to_string(static_cast<int>(role));
}

/**
 * @brief Parse a string into a Role enum
 * 
 * @param str The string to parse
 * @return The parsed Role value
 */
Role parse_role(char c) {
    return static_cast<Role>(utils::value(c));
}

/**
 * @brief Get the role of the HM-11 module
 * 
 * @param result Output parameter that will be set to the current role
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_role(Role& result, UartStatus& status) {
    transmit_and_check(
        "AT+ROLE?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        result = parse_role(static_cast<char>(response_[OK_GET.size()]));
    }
}

/**
 * @brief Set the role of the HM-11 module
 * 
 * @param value The role to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_role(Role value, UartStatus& status) {
    const std::string command = "AT+ROLE" + to_string(value);
    const std::string expected = std::string(OK_SET) + to_string(value);
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Set the minimum link layer connection interval
 * 
 * @param value The connection interval to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval value, UartStatus& status) {
    const std::string command = "AT+COMI" + std::to_string(static_cast<int>(value));
    const std::string expected = std::string(OK_SET) + std::to_string(static_cast<int>(value));
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the minimum link layer connection interval
 * 
 * @param result Output parameter that will be set to the current minimum connection interval
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result, UartStatus& status) {
    transmit_and_check(
        "AT+COMI?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        result = static_cast<LinkLayerConnectionInterval>(utils::value(
            response_[OK_GET.size()]));
    }
}

/**
 * @brief Set the maximum link layer connection interval
 * 
 * @param value The connection interval to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval value, UartStatus& status) {
    const std::string command = "AT+COMA" + std::to_string(static_cast<int>(value));
    const std::string expected = std::string(OK_SET) + std::to_string(static_cast<int>(value));
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the maximum link layer connection interval
 * 
 * @param result Output parameter that will be set to the current maximum connection interval
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result, UartStatus& status) {
    transmit_and_check(
        "AT+COMA?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        result = static_cast<LinkLayerConnectionInterval>(utils::value(
            response_[OK_GET.size()]));
    }
}

/**
 * @brief Set the link layer connection slave latency
 * 
 * @param value The slave latency to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_link_layer_connection_slave_latency(
    util::LayerConnectionLatency value, UartStatus& status) {
    const std::string command = "AT+COLA" + std::to_string(value.get_value());
    const std::string expected = std::string(OK_SET) + std::to_string(value.get_value());
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the link layer connection slave latency
 * 
 * @param result Output parameter that will be set to the current slave latency
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_link_layer_connection_slave_latency(
    util::LayerConnectionLatency& result, UartStatus& status) {
    transmit_and_check(
        "AT+COLA?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        result = util::LayerConnectionLatency(utils::value(
            response_[OK_GET.size()]));
    }
}

/**
 * @brief Set the connection supervision timeout
 * 
 * @param value The supervision timeout to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_connection_supervision_timeout(
    ConnectionSupervisionTimeout value, UartStatus& status) {
    const std::string command = "AT+COSU" + std::to_string(static_cast<int>(value));
    const std::string expected = std::string(OK_SET) + std::to_string(static_cast<int>(value));
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the connection supervision timeout
 * 
 * @param result Output parameter that will be set to the current supervision timeout
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_connection_supervision_timeout(
    ConnectionSupervisionTimeout& result, UartStatus& status) {
    transmit_and_check(
        "AT+COSU?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        result = static_cast<ConnectionSupervisionTimeout>(utils::value(
            response_[OK_GET.size()]));
    }
}

/**
 * @brief Set the update connection flag
 * 
 * @param value True to enable connection parameter updates, false to disable
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_update_connection(
    bool value, UartStatus& status) {
    const char value_char = value ? '1' : '0';
    const std::string command = std::string("AT+COUP") + value_char;
    const std::string expected = std::string(OK_SET) + value_char;
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the update connection flag
 * 
 * @param result Output parameter that will be set to the current update connection flag
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_update_connection(
    bool& result, UartStatus& status) {
    transmit_and_check(
        "AT+COUP?",
        OK_GET,
        response_.data(),
        OK_GET.size() + 1,
        status
    );
    
    if (status == UartStatus::Ok) {
        char value_char = static_cast<char>(response_[OK_GET.size()]);
        result = (value_char == '1');
    }
}

/**
 * @brief Set the timeout for connecting to a remote device
 * 
 * @param timeout The timeout value to set
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_connect_remote_device_timeout(
    util::ConnectTimeout timeout, UartStatus& status) {
    const std::string command = "AT+TCON" + std::string(timeout);
    const std::string expected = std::string(OK_SET) + std::string(timeout);
    
    transmit_and_check(command, expected, status);
}

/**
 * @brief Get the timeout for connecting to a remote device
 * 
 * @param timeout Output parameter that will be set to the current timeout
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_connect_remote_device_timeout(
    util::ConnectTimeout& timeout, UartStatus& status) {
    transmit_and_check(
        "AT+TCON?",
        OK_GET,
        response_.data(),
        OK_GET.size() + util::ConnectTimeout::size(),
        status
    );
    
    if (status == UartStatus::Ok) {
        char buffer[util::ConnectTimeout::size() + 1] = {0};
        std::copy_n(&response_[OK_GET.size()], util::ConnectTimeout::size(), buffer);
        timeout = util::ConnectTimeout(buffer);
    }
}

// Explicit instantiation of template methods to satisfy the linker
// For now, we're using a placeholder for the Port, ReceiveHandler, and ReadPositionHandler types
// In a real implementation, these would be properly defined
class PlaceholderPort {
public:
    void transmit(std::span<std::uint8_t>, UartStatus&) {}
    void receive(std::span<std::uint8_t>, UartStatus&) {}
};

using PlaceholderDriver = HM11Driver<
    PlaceholderPort,
    decltype(defaultReceiveHandler<PlaceholderPort>),
    std::function<void(bool&, std::size_t&)>
>;

template void PlaceholderDriver::get_role(Role&, UartStatus&);
template void PlaceholderDriver::set_role(Role, UartStatus&);
template void PlaceholderDriver::set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval, UartStatus&);
template void PlaceholderDriver::get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval&, UartStatus&);
template void PlaceholderDriver::set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval, UartStatus&);
template void PlaceholderDriver::get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval&, UartStatus&);
template void PlaceholderDriver::set_link_layer_connection_slave_latency(util::LayerConnectionLatency, UartStatus&);
template void PlaceholderDriver::get_link_layer_connection_slave_latency(util::LayerConnectionLatency&, UartStatus&);
template void PlaceholderDriver::set_connection_supervision_timeout(ConnectionSupervisionTimeout, UartStatus&);
template void PlaceholderDriver::get_connection_supervision_timeout(ConnectionSupervisionTimeout&, UartStatus&);
template void PlaceholderDriver::set_update_connection(bool, UartStatus&);
template void PlaceholderDriver::get_update_connection(bool&, UartStatus&);
template void PlaceholderDriver::set_connect_remote_device_timeout(util::ConnectTimeout, UartStatus&);
template void PlaceholderDriver::get_connect_remote_device_timeout(util::ConnectTimeout&, UartStatus&);

/**
 * @brief Connect to a device by MAC address
 *
 * @param mac The MAC address of the device to connect to
 * @param result Output parameter that will be set to the connection result
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::connect(
    const util::MacAddress& mac, ConnectResult& result, UartStatus& status) {
    
    // Create connect command using the AT+CON command and the MAC address
    const std::string command = util::at::cmd::CON + std::string(mac);
    
    // Transmit the command
    transmit(command, status);
    
    if (status != UartStatus::Ok) {
        return;
    }
    
    // Receive and parse the response
    receive_handler_(port_, response_.data(), response_.size(), status);
    
    if (status != UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }
    
    // Check for specific responses to determine the connection result
    if (utils::starts_with(response_.data(), response_.size(), util::at::response::OK_CONN)) {
        result = ConnectResult::Connected;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNA")) {
        result = ConnectResult::Connecting;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNE")) {
        result = ConnectResult::ConnectError;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNF")) {
        result = ConnectResult::ConnectFail;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONN?")) {
        result = ConnectResult::NoAddress;
    } else {
        result = ConnectResult::OtherError;
        status = UartStatus::ErrError;
    }
}

/**
 * @brief Connect to a device by MAC address string
 *
 * @param mac_str The MAC address string of the device to connect to
 * @param result Output parameter that will be set to the connection result
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::connect(
    std::string_view mac_str, ConnectResult& result, UartStatus& status) {
    
    // Validate MAC address string format (should be 12 hexadecimal characters)
    if (mac_str.size() != 12) {
        status = UartStatus::ErrError;
        result = ConnectResult::OtherError;
        return;
    }
    
    // Attempt to convert to MacAddress type
    util::MacAddress mac(mac_str);
    
    // Use the MAC address overload to perform the connection
    connect(mac, result, status);
}

/**
 * @brief Connect to the last connected device
 *
 * @param result Output parameter that will be set to the connection result
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::connect_last_device(
    ConnectResult& result, UartStatus& status) {
    
    // Send the connect last device command (AT+CONNL)
    transmit("AT+CONNL", status);
    
    if (status != UartStatus::Ok) {
        return;
    }
    
    // Receive and parse the response
    receive_handler_(port_, response_.data(), response_.size(), status);
    
    if (status != UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }
    
    // Check for specific responses to determine the connection result
    if (utils::starts_with(response_.data(), response_.size(), util::at::response::OK_CONN)) {
        result = ConnectResult::Connected;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNA")) {
        result = ConnectResult::Connecting;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNE")) {
        result = ConnectResult::ConnectError;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONNF")) {
        result = ConnectResult::ConnectFail;
    } else if (utils::starts_with(response_.data(), response_.size(), "OK+CONN?")) {
        result = ConnectResult::NoAddress;
    } else {
        result = ConnectResult::OtherError;
        status = UartStatus::ErrError;
    }
}

/**
 * @brief Disconnect from the currently connected device
 *
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::disconnect(UartStatus& status) {
    // Send the disconnect command (AT)
    transmit_and_check("AT", util::at::response::OK, status);
}

// Explicit instantiations for the new methods
template void PlaceholderDriver::connect(const util::MacAddress&, ConnectResult&, UartStatus&);
template void PlaceholderDriver::connect(std::string_view, ConnectResult&, UartStatus&);
template void PlaceholderDriver::connect_last_device(ConnectResult&, UartStatus&);
template void PlaceholderDriver::disconnect(UartStatus&);

} // namespace hm11