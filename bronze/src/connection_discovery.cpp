/**
 * @file connection_discovery.cpp
 * @brief Implementation of connection and discovery operations for HM11Driver
 *
 * This file provides the implementation of connection and discovery operations for the HM11Driver class.
 * It includes methods for managing connections, MAC addresses, and device discovery settings.
 */

#include "../include/hm11/hm11_driver.hpp"

namespace hm11 {

/**
 * @brief Clear the last connected device address
 *
 * This method clears the MAC address of the last connected device from the module's memory.
 * Only used in Central role.
 *
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::clear_last_connected_address(UartStatus& status) {
    transmit_and_check("AT+CLEAR", "OK+CLEAR", status);
}

/**
 * @brief Get the MAC address of the last connected device
 *
 * This method retrieves the MAC address of the last device that was connected.
 * Only used in Central role.
 *
 * @param mac Output parameter that will be set to the MAC address
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_last_connected_device_address(
    std::string& mac, UartStatus& status) {
    const std::string_view EXPECT_PREFIX = "OK+RADD:";
    constexpr std::size_t MAC_ADDRESS_LENGTH = 12;

    transmit_and_check(
        "AT+RADD?",
        EXPECT_PREFIX,
        m_response.data(),
        EXPECT_PREFIX.size() + MAC_ADDRESS_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract MAC address from response after the prefix
        mac = std::string(
            reinterpret_cast<const char*>(m_response.data() + EXPECT_PREFIX.size()),
            MAC_ADDRESS_LENGTH
        );
    }
}

/**
 * @brief Set whether to save the connected device's MAC address
 *
 * This method enables or disables saving the MAC address of connected devices.
 * Only used in Central role.
 * When enabled, the module will try to connect to the saved MAC address on power-on.
 *
 * @param save True to enable saving, false to disable
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_save_connected_mac(
    bool save, UartStatus& status) {
    // Note: Ada implementation inverts the boolean (not Save)
    const std::string value_str = save ? "0" : "1";
    transmit_and_check("AT+SAVE" + value_str, std::string(OK_SET) + value_str, status);
}

/**
 * @brief Get whether saving the connected device's MAC address is enabled
 *
 * Only used in Central role.
 *
 * @param save Output parameter that will be set to true if saving is enabled, false otherwise
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_save_connected_mac(
    bool& save, UartStatus& status) {
    transmit_and_check(
        "AT+SAVE?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract the boolean value (inverted as per Ada implementation)
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        save = (value_char == '0');
    }
}

/**
 * @brief Set the discovery time for scanning
 *
 * This method sets the duration of the scanning procedure.
 * Only used in Central role.
 * Added in V543 firmware.
 *
 * @param time The discovery time (1-9 seconds)
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_discovery_time(
    util::DiscoveryTime time, UartStatus& status) {
    const std::string time_str = std::to_string(time.get_value());
    transmit_and_check("AT+SCAN" + time_str, std::string(OK_SET) + time_str, status);
}

/**
 * @brief Get the current discovery time setting
 *
 * Only used in Central role.
 *
 * @param time Output parameter that will be set to the current discovery time
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_discovery_time(
    util::DiscoveryTime& time, UartStatus& status) {
    transmit_and_check(
        "AT+SCAN?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract the discovery time value
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '1' && value_char <= '9') {
            time = util::DiscoveryTime(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

/**
 * @brief Set whether to show device information during discovery
 *
 * This method configures whether to include device name and/or RSSI information
 * in the scan results.
 *
 * @param show The device information display mode
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_show_device_information_when_discovery(
    ShowDeviceInformation show, UartStatus& status) {
    const std::string show_str = std::to_string(static_cast<int>(show));
    transmit_and_check("AT+SHOW" + show_str, std::string(OK_SET) + show_str, status);
}

/**
 * @brief Get the current device information display setting
 *
 * @param show Output parameter that will be set to the current display mode
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_show_device_information_when_discovery(
    ShowDeviceInformation& show, UartStatus& status) {
    transmit_and_check(
        "AT+SHOW?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract the show device information value
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '0' && value_char <= '3') {
            show = static_cast<ShowDeviceInformation>(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

/**
 * @brief Connect to a discovered device by its index
 *
 * This method connects to a device that was discovered during a scan, using its
 * index in the discovered devices list.
 * Required: Set_Work_Type (Respond_AT_Command) and Set_Role (Central)
 * This command clears all discovered devices list.
 *
 * @param index The index of the discovered device (0-5)
 * @param result Output parameter that will be set to the connection result
 * @param status Output parameter that will be set to the operation status
 */
template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::connect(
    util::DiscoveredIndex index, ConnectResult& result, UartStatus& status) {
    result = ConnectResult::OtherError;

    const std::string index_str = std::to_string(index.get_value());
    const std::string_view EXPECT = "OK+CONN";

    transmit_and_check(
        "AT+CONN" + index_str,
        EXPECT,
        m_response.data(),
        EXPECT.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract the connection result character
        const char result_char = static_cast<char>(m_response[EXPECT.size()]);

        switch (result_char) {
            case 'A':
                result = ConnectResult::Connecting;
                break;
            case 'E':
                result = ConnectResult::Connected;
                break;
            case 'F':
                result = ConnectResult::ConnectError;
                break;
            case 'L':
                result = ConnectResult::ConnectFail;
                break;
            case 'N':
                result = ConnectResult::NoAddress;
                break;
            default:
                result = ConnectResult::OtherError;
                break;
        }
    }
}

// Explicit template instantiations for the PlaceholderDriver
template void PlaceholderDriver::clear_last_connected_address(UartStatus&);
template void PlaceholderDriver::get_last_connected_device_address(std::string&, UartStatus&);
template void PlaceholderDriver::set_save_connected_mac(bool, UartStatus&);
template void PlaceholderDriver::get_save_connected_mac(bool&, UartStatus&);
template void PlaceholderDriver::set_discovery_time(util::DiscoveryTime, UartStatus&);
template void PlaceholderDriver::get_discovery_time(util::DiscoveryTime&, UartStatus&);
template void PlaceholderDriver::set_show_device_information_when_discovery(ShowDeviceInformation, UartStatus&);
template void PlaceholderDriver::get_show_device_information_when_discovery(ShowDeviceInformation&, UartStatus&);
template void PlaceholderDriver::connect(util::DiscoveredIndex, ConnectResult&, UartStatus&);

} // namespace hm11
