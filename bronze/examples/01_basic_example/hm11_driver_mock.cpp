/**
 * @file hm11_driver_mock.cpp
 * @brief Mock implementation of the HM11 driver for the example
 *
 * This file provides a mock implementation of the HM11 driver functions
 * that are used in the example. This allows the example to compile without
 * needing to implement the full HM11 driver.
 */

#include <chrono>
#include "include/hm11/hm11.hpp"

// Explicit instantiation of the template class for the mock UART implementation
#include "uart_mock.hpp"

// Template instantiation with the UARTMock class
template class hm11::HM11Driver<hm11::tests::mocks::UARTMock>;

namespace hm11 {

// Implement all the methods needed for the example
template <>
void HM11Driver<tests::mocks::UARTMock>::test(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::get_mac_address(util::MacAddress& mac, hal::UartStatus& status) {
    mac = util::MacAddress("123456789ABC");
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_advertising_type([[maybe_unused]] AdvertisingType type, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_module_name([[maybe_unused]] std::string_view name, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_work_type([[maybe_unused]] WorkType type, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::set_role([[maybe_unused]] Role role, hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

template <>
void HM11Driver<tests::mocks::UARTMock>::restart(hal::UartStatus& status) {
    status = hal::UartStatus::Ok;
}

} // namespace hm11