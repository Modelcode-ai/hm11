/**
 * @file hm11_driver_test_stubs.cpp
 * @brief Stub implementations of HM11Driver functions for tests
 */

#include "hm11/driver.hpp"
#include "mocks/hm11_driver_mock.hpp"

namespace hm11 {

// Helper function to convert string to UInt8 vector
static std::vector<hal::UInt8> to_uint8_vec(const std::string& str) {
    return std::vector<hal::UInt8>(str.begin(), str.end());
}

// Stub implementation for test function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::test(hal::UartStatus& status) {
    std::vector<hal::UInt8> cmd = {'A', 'T'};
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Validate the response is "OK"
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str != "OK") {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for reset function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::reset(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+RENEW");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for restart function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::restart(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+RESTART");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for sleep function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::sleep(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+SLEEP");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for wake_up function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::wake_up(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

// Stub implementation for disconnect function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::disconnect(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+DISC");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for start_working function
template <> void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::start_working(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+START");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for software_version function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::software_version(
    util::VersionType& version,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+VERSION");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse response to extract version
    // Expected format: "HMSoft V545" or similar, where version is hex digits after "V"
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);

    // Look for "V" and extract hex digits after it
    auto v_pos = response_str.find('V');
    if (v_pos != std::string_view::npos && v_pos + 1 < response_str.size()) {
        // Extract everything after 'V', but only take hex characters
        std::string version_hex;
        for (size_t i = v_pos + 1; i < response_str.size() && version_hex.size() < 4; ++i) {
            char c = response_str[i];
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
                version_hex += c;
            } else if (c >= 'a' && c <= 'f') {
                // Convert lowercase to uppercase
                version_hex += static_cast<char>(c - 'a' + 'A');
            } else {
                // Stop at first non-hex character
                break;
            }
        }

        // Pad with zeros if needed to make it 4 characters
        while (version_hex.size() < 4) {
            version_hex = '0' + version_hex;
        }

        version = util::VersionType(version_hex);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for get_role function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_role(Role& role, hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ROLE?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the role value
    // Expected format: "OK+Get:X" where X is '0' (Peripheral) or '1' (Central)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char == '0') {
            role = Role::Peripheral;
        } else if (value_char == '1') {
            role = Role::Central;
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_role function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_role(Role role, hal::UartStatus& status) {
    std::string cmd_str = (role == Role::Peripheral) ? "AT+ROLE0" : "AT+ROLE1";
    auto cmd = to_uint8_vec(cmd_str);
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Validate the response starts with "OK+Set:"
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() < 7 || response_str.substr(0, 7) != "OK+Set:") {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for get_advertising_interval function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_advertising_interval(
    AdvertisingInterval& interval,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ADVI?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the interval value
    // Expected format: "OK+Get:X" where X is a single character ('0'-'9', 'A'-'F')
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        // Convert character to AdvertisingInterval
        if (value_char >= '0' && value_char <= '9') {
            interval = static_cast<AdvertisingInterval>(value_char - '0');
        } else if (value_char >= 'A' && value_char <= 'F') {
            interval = static_cast<AdvertisingInterval>(10 + (value_char - 'A'));
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_advertising_interval function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_advertising_interval(
    AdvertisingInterval interval,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ADVI" + std::to_string(static_cast<int>(interval)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_advertising_type function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_advertising_type(
    AdvertisingType& type,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ADTY?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the type value
    // Expected format: "OK+Get:X" where X is '0'-'3'
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char >= '0' && value_char <= '3') {
            type = static_cast<AdvertisingType>(value_char - '0');
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_advertising_type function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_advertising_type(
    AdvertisingType type,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ADTY" + std::to_string(static_cast<int>(type)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_module_name function
template <>
std::string HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_module_name(hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+NAME?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return "";
    }

    // Receive the response (leave last byte for null terminator)
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size() - 1, status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return "";
    }

    // Ensure null termination for strlen
    rx_buffer[rx_buffer.size() - 1] = 0;

    // Parse the response to extract the module name
    // Expected format: "OK+NAME:XXX" where XXX is the name
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 8) == "OK+NAME:") {
        return std::string(response_str.substr(8));
    } else {
        status = hal::UartStatus::ErrError;
        return "";
    }
}

// Stub implementation for set_module_name function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_module_name(
    std::string_view name,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+NAME" + std::string(name));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_module_power function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_module_power(
    ModulePower& power,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+POWE?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the power value
    // Expected format: "OK+Get:X" where X is '0'-'3'
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char >= '0' && value_char <= '3') {
            power = static_cast<ModulePower>(value_char - '0');
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_module_power function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_module_power(
    ModulePower power,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+POWE" + std::to_string(static_cast<int>(power)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_work_mode function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_work_mode(
    WorkMode& mode,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+MODE?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the mode value
    // Expected format: "OK+Get:X" where X is '0'-'2'
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char >= '0' && value_char <= '2') {
            mode = static_cast<WorkMode>(value_char - '0');
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_work_mode function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_work_mode(
    WorkMode mode,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+MODE" + std::to_string(static_cast<int>(mode)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_work_type function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_work_type(
    WorkType& type,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+IMME?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the type value
    // Expected format: "OK+Get:X" where X is '0'-'1'
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char >= '0' && value_char <= '1') {
            type = static_cast<WorkType>(value_char - '0');
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_work_type function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_work_type(
    WorkType type,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+IMME" + std::to_string(static_cast<int>(type)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_uart_baud_rate function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_uart_baud_rate(
    UartBaudRate& baud,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+BAUD?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the baud rate value
    // Expected format: "OK+Get:X" where X is the baud rate index
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        if (value_char >= '0' && value_char <= '8') {
            baud = static_cast<UartBaudRate>(value_char - '0');
        }
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_uart_baud_rate function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_uart_baud_rate(
    UartBaudRate baud,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+BAUD" + std::to_string(static_cast<int>(baud)));
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_module_auto_sleep function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_module_auto_sleep(
    bool& auto_sleep,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ASLP?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the auto sleep value
    // Expected format: "OK+Get:X" where X is '0' (false) or '1' (true)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        auto_sleep = (value_char == '1');
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_module_auto_sleep function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_module_auto_sleep(
    bool auto_sleep,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec(auto_sleep ? "AT+ASLP1" : "AT+ASLP0");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_mac_address function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_mac_address(
    util::MacAddress& mac,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+ADDR?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
    mac = util::MacAddress("112233445566");
    status = hal::UartStatus::Ok;
}

// Stub implementation for get_ibeacon_switch function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_ibeacon_switch(
    bool& enabled,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+IBEA?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the switch value
    // Expected format: "OK+Get:X" where X is '0' or '1'
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 8 && response_str.substr(0, 7) == "OK+Get:") {
        char value_char = response_str[7];
        enabled = (value_char == '1');
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_ibeacon_switch function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_ibeacon_switch(
    bool enabled,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec(enabled ? "AT+IBEA1" : "AT+IBEA0");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_ibeacon_uuid function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_ibeacon_uuid(
    util::IBeaconUuid& uuid,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+IBE0?");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the response to extract the UUID value
    // Expected format: "OK+Get:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" where X are hex digits (32 characters)
    // First find the actual length of the response (null-terminated)
    std::size_t response_len = std::strlen(reinterpret_cast<const char*>(rx_buffer.data()));
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), response_len);
    if (response_str.size() >= 39 && response_str.substr(0, 7) == "OK+Get:") {
        std::string uuid_str(response_str.substr(7, 32));
        uuid = util::IBeaconUuid(uuid_str);
    } else {
        status = hal::UartStatus::ErrError;
    }
}

// Stub implementation for set_ibeacon_uuid function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_ibeacon_uuid(
    const util::IBeaconUuid& uuid,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+IBE0" + uuid.to_string());
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for set_ibeacon_major_version function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_ibeacon_major_version(
    const util::FixedString<4, util::HexCharValidator>& version,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+MARJ" + version.to_string());
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for set_ibeacon_minor_version function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_ibeacon_minor_version(
    const util::FixedString<4, util::HexCharValidator>& version,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+MINO" + version.to_string());
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for set_ibeacon_measured_power function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_ibeacon_measured_power(
    const util::FixedString<2, util::HexCharValidator>& power,
    hal::UartStatus& status) {
    auto cmd = to_uint8_vec("AT+MEAS" + power.to_string());
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for connect function (MacAddress overload)
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::connect(
    const util::MacAddress& mac,
    ConnectResult& result,
    hal::UartStatus& status) {
    // Transmit the connect command
    auto cmd = to_uint8_vec("AT+CON" + mac.to_string());
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }

    // Parse the response to determine the result
    // Check longer/more specific strings first to avoid false matches
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), rx_buffer.size());

    if (response_str.find("OK+CONNA") == 0) {
        result = ConnectResult::Connecting;
    } else if (response_str.find("OK+CONNE") == 0) {
        result = ConnectResult::ConnectError;
    } else if (response_str.find("OK+CONNF") == 0) {
        result = ConnectResult::ConnectFail;
    } else if (response_str.find("OK+CONN?") == 0) {
        result = ConnectResult::NoAddress;
    } else if (response_str.find("OK+CONN") == 0) {
        result = ConnectResult::Connected;
    } else {
        // Unknown response - set error status
        status = hal::UartStatus::Error;
        result = ConnectResult::OtherError;
    }
}

// Stub implementation for connect function (string_view overload)
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::connect(
    std::string_view mac_str,
    ConnectResult& result,
    hal::UartStatus& status) {
    // Validate MAC address string (should be 12 hex characters)
    if (mac_str.size() != 12) {
        status = hal::UartStatus::ErrError;
        result = ConnectResult::OtherError;
        return;
    }

    // Convert to MacAddress and call the other overload
    util::MacAddress mac(mac_str);
    connect(mac, result, status);
}

// Stub implementation for connect_last_device function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::connect_last_device(
    ConnectResult& result,
    hal::UartStatus& status) {
    // Transmit the connect last device command
    auto cmd = to_uint8_vec("AT+CONNL");
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));

    if (status != hal::UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }

    // Receive the response
    std::array<hal::UInt8, 256> rx_buffer{};
    hal::AnyUartPort port = uart;
    receive_handler(port, rx_buffer.data(), rx_buffer.size(), status, std::chrono::milliseconds(1000), false);

    if (status != hal::UartStatus::Ok) {
        result = ConnectResult::ConnectError;
        return;
    }

    // Parse the response to determine the result
    // Check longer/more specific strings first to avoid false matches
    std::string_view response_str(reinterpret_cast<const char*>(rx_buffer.data()), rx_buffer.size());

    if (response_str.find("OK+CONNA") == 0) {
        result = ConnectResult::Connecting;
    } else if (response_str.find("OK+CONNE") == 0) {
        result = ConnectResult::ConnectError;
    } else if (response_str.find("OK+CONNF") == 0) {
        result = ConnectResult::ConnectFail;
    } else if (response_str.find("OK+CONN?") == 0) {
        result = ConnectResult::NoAddress;
    } else if (response_str.find("OK+CONN") == 0) {
        result = ConnectResult::Connected;
    } else {
        // Unknown response - set error status
        status = hal::UartStatus::Error;
        result = ConnectResult::OtherError;
    }
}

} // namespace hm11