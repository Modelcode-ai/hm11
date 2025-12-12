/**
 * @file hm11_driver_mock.cpp
 * @brief Mock implementation of the HM11 driver for the configuration example
 *
 * This file provides a mock implementation of the HM11 driver functions
 * that are used in the configuration example. This allows the example to 
 * compile without needing to implement the full HM11 driver and demonstrates
 * all the configuration-related operations.
 */

#include <chrono>
#include <string>
#include "include/hm11/hm11.hpp"
#include "include/hm11/util/fixed_string.hpp"

// Explicit instantiation of the template class for the mock UART implementation
#include "uart_mock.hpp"

// Template instantiation with the UARTMock class
template class hm11::HM11Driver<hm11::tests::mocks::UARTMock>;

namespace hm11 {

// Basic operations
template <>
void HM11Driver<tests::mocks::UARTMock>::test(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::software_version(util::VersionType& version, hal::UartStatus& status) {
    version = util::VersionType("V560");
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::reset(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::restart(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

// Role and Work Mode Configuration
template <>
void HM11Driver<tests::mocks::UARTMock>::get_role(Role& result, hal::UartStatus& status) {
    result = Role::Peripheral;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_role(Role value, hal::UartStatus& status) {
    // Validate input value (should be either Peripheral or Central)
    if (value != Role::Peripheral && value != Role::Central) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_work_type(WorkType type, hal::UartStatus& status) {
    // Validate input value
    if (type != WorkType::StartImmediately && type != WorkType::RespondAtCommand) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_work_type(WorkType& result, hal::UartStatus& status) {
    result = WorkType::StartImmediately;
    status = hal::UartStatus::Ok;
}

// Advertising Configuration
template <>
void HM11Driver<tests::mocks::UARTMock>::set_advertising_type(AdvertisingType type, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(type) > 3) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_advertising_type(AdvertisingType& result, hal::UartStatus& status) {
    result = AdvertisingType::AdvertisingScanResponseConnectable;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_advertising_interval(AdvertisingInterval interval, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(interval) > 15) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_advertising_interval(AdvertisingInterval& result, hal::UartStatus& status) {
    result = AdvertisingInterval::Ms100;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_reliable_advertising_mode(AdvertisingMode mode, hal::UartStatus& status) {
    // Validate input value
    if (mode != AdvertisingMode::NormalAdvertising && mode != AdvertisingMode::ReliableAdvertising) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_reliable_advertising_mode(AdvertisingMode& result, hal::UartStatus& status) {
    result = AdvertisingMode::NormalAdvertising;
    status = hal::UartStatus::Ok;
}

// Security Configuration

template <>
void HM11Driver<tests::mocks::UARTMock>::get_mac_address(util::MacAddress& mac, hal::UartStatus& status) {
    mac = util::MacAddress("123456789ABC");
    status = hal::UartStatus::Ok;
}

// Power Management
template <>
void HM11Driver<tests::mocks::UARTMock>::sleep(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::wake_up(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_module_power(ModulePower power, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(power) > 3) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_module_power(ModulePower& power, hal::UartStatus& status) {
    power = ModulePower::Dbm0;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_module_auto_sleep([[maybe_unused]] bool enable, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_module_auto_sleep(bool& enabled, hal::UartStatus& status) {
    enabled = false;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_output_power(OutputPower power, hal::UartStatus& status) {
    // Validate input value
    if (power != OutputPower::NormalPower && power != OutputPower::MaxPower) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_output_power(OutputPower& power, hal::UartStatus& status) {
    power = OutputPower::NormalPower;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_battery_monitor_switch([[maybe_unused]] bool enable, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_battery_monitor_switch(bool& enabled, hal::UartStatus& status) {
    enabled = false;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_battery_information([[maybe_unused]] util::Percent level, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::query_battery_information(util::Percent& level, hal::UartStatus& status) {
    level = util::Percent(75);
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_uart_sleep_type(UartSleepType type, hal::UartStatus& status) {
    // Validate input value
    if (type != UartSleepType::CanWakeUpThroughUart && type != UartSleepType::ShutdownUart) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_uart_sleep_type(UartSleepType& type, hal::UartStatus& status) {
    type = UartSleepType::CanWakeUpThroughUart;
    status = hal::UartStatus::Ok;
}

// Connection Settings

template <>
void HM11Driver<tests::mocks::UARTMock>::set_connection_supervision_timeout(
    ConnectionSupervisionTimeout value, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(value) > 6) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_connection_supervision_timeout(
    ConnectionSupervisionTimeout& result, hal::UartStatus& status) {
    result = ConnectionSupervisionTimeout::Ms6000;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_update_connection([[maybe_unused]] bool value, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_update_connection(bool& result, hal::UartStatus& status) {
    result = true;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_connect_remote_device_timeout(
    [[maybe_unused]] util::ConnectTimeout timeout, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_connect_remote_device_timeout(
    util::ConnectTimeout& timeout, hal::UartStatus& status) {
    timeout = util::ConnectTimeout("000000");
    status = hal::UartStatus::Ok;
}

// Device Settings
template <>
void HM11Driver<tests::mocks::UARTMock>::set_module_name(std::string_view name, hal::UartStatus& status) {
    // Validate input value (name should be between 1 and 12 characters)
    if (name.empty() || name.length() > 12) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
std::string HM11Driver<tests::mocks::UARTMock>::get_module_name(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
    return "HMTest";
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_uart_baud_rate(UartBaudRate baud_rate, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(baud_rate) > 8) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_uart_baud_rate(UartBaudRate& baud_rate, hal::UartStatus& status) {
    baud_rate = UartBaudRate::Br9600;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_uart_flow_control_switch([[maybe_unused]] bool enable, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_uart_flow_control_switch(bool& enabled, hal::UartStatus& status) {
    enabled = false;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_parity_bit(ParityBit parity, hal::UartStatus& status) {
    // Validate input value
    if (static_cast<uint8_t>(parity) > 2) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_parity_bit(ParityBit& parity, hal::UartStatus& status) {
    parity = ParityBit::None;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_stop_bit(StopBit stop_bit, hal::UartStatus& status) {
    // Validate input value
    if (stop_bit != StopBit::OneStopBit && stop_bit != StopBit::TwoStopBit) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_stop_bit(StopBit& stop_bit, hal::UartStatus& status) {
    stop_bit = StopBit::OneStopBit;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_module_rx_gain(RxGain gain, hal::UartStatus& status) {
    // Validate input value
    if (gain != RxGain::No && gain != RxGain::Open) {
        status = hal::UartStatus::ErrError;
        return;
    }
    
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_module_rx_gain(RxGain& gain, hal::UartStatus& status) {
    gain = RxGain::No;
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_service_uuid([[maybe_unused]] std::string_view uuid, hal::UartStatus& status) {
    // In a real implementation, we would validate that this is a valid UUID
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_service_uuid(std::string& uuid, hal::UartStatus& status) {
    uuid = "FFE0";
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_characteristic([[maybe_unused]] std::string_view characteristic, hal::UartStatus& status) {
    // In a real implementation, we would validate that this is a valid characteristic UUID
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_characteristic(std::string& characteristic, hal::UartStatus& status) {
    characteristic = "FFE1";
    status = hal::UartStatus::Ok;
}

} // namespace hm11