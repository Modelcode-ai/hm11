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
    version = util::VersionType("560A");
    status = hal::UartStatus::Ok;
}

// Stub implementation for get_role function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_role(Role& role, hal::UartStatus& status) {
    role = Role::Peripheral;
    status = hal::UartStatus::Ok;
}

// Stub implementation for set_role function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::set_role(Role role, hal::UartStatus& status) {
    std::string cmd_str = (role == Role::Peripheral) ? "AT+ROLE0" : "AT+ROLE1";
    auto cmd = to_uint8_vec(cmd_str);
    hal::UartData8b tx_data(cmd);
    uart->transmit(tx_data, status, std::chrono::milliseconds(1000));
}

// Stub implementation for get_advertising_interval function
template <>
void HM11Driver<std::shared_ptr<tests::mocks::EnhancedUARTMock>>::get_advertising_interval(
    AdvertisingInterval& interval,
    hal::UartStatus& status) {
    interval = AdvertisingInterval::Ms100;
    status = hal::UartStatus::Ok;
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
    type = AdvertisingType::AdvertisingScanResponseConnectable;
    status = hal::UartStatus::Ok;
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
    status = hal::UartStatus::Ok;
    return "HM-11";
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
    power = ModulePower::Dbm0;
    status = hal::UartStatus::Ok;
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
    mode = WorkMode::TransmissionMode;
    status = hal::UartStatus::Ok;
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
    type = WorkType::StartImmediately;
    status = hal::UartStatus::Ok;
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
    baud = UartBaudRate::Br9600;
    status = hal::UartStatus::Ok;
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
    auto_sleep = false;
    status = hal::UartStatus::Ok;
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
    enabled = false;
    status = hal::UartStatus::Ok;
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
    uuid = util::IBeaconUuid("00000000000000000000000000000000");
    status = hal::UartStatus::Ok;
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