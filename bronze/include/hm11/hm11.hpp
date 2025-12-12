/**
 * @file hm11.hpp
 * @brief HM11 BLE driver main header
 *
 * This file is part of the HM11 driver C++ migration project.
 * Based on the original Ada implementation of the HM11 driver.
 */

#ifndef HM11_HM11_HPP
#define HM11_HM11_HPP

#include "enum_types.hpp"
#include "hal/hal.hpp"
#include "hal/uart.hpp"
#include "util/fixed_string.hpp"

namespace hm11 {

/**
 * @brief Type for BLE device discovery callback
 *
 * This implements Decision 10 (plain C-style function pointers for callbacks) to maintain
 * zero overhead and ISR compatibility. The callback is invoked with device information
 * when a BLE device is discovered during scanning.
 *
 * @param id Identifier character indicating device type
 * @param mac_address MAC address of the discovered device (12 hex characters)
 * @param name Name of the discovered device
 * @param rssi RSSI value of the discovered device
 * @param user_data Optional pointer to user-defined data
 */
using DiscoveryCallback = void (*)(
    char id,
    const util::MacAddress& mac_address,
    std::string_view name,
    std::string_view rssi,
    void* user_data);

/**
 * @brief Callback function type for iBeacon discovery
 *
 * This function is called when an iBeacon device is discovered during scanning.
 *
 * @param device Device information string
 * @param user_data Optional pointer to user-defined data
 */
using IBeaconDiscoveryCallback = void (*)(std::string_view device, void* user_data);

/**
 * @brief Type for GATT service discovery callback
 *
 * This callback is invoked when GATT services are discovered during a
 * service discovery operation.
 *
 * @param service_uuid UUID of the discovered service
 * @param user_data Optional pointer to user-defined data
 */
using ServiceDiscoveryCallback = void (*)(std::string_view service_uuid, void* user_data);

/**
 * @brief Type for GATT characteristic discovery callback
 *
 * This callback is invoked when GATT characteristics are discovered during a
 * characteristic discovery operation.
 *
 * @param char_uuid UUID of the discovered characteristic
 * @param properties Properties of the discovered characteristic
 * @param user_data Optional pointer to user-defined data
 */
using CharacteristicDiscoveryCallback = void (*)(std::string_view char_uuid, std::uint8_t properties, void* user_data);

/**
 * @brief Forward declaration of HM11Driver template class
 *
 * Actual implementation will be defined in a separate header file.
 * This implements Decision 1 (Dependency Injection Mechanism) by making
 * the driver a class template parameterized by UARTImpl type.
 *
 * @tparam UARTImpl UART implementation that satisfies the UartPort concept
 */
template <hal::UART_Implementation UARTImpl> class HM11Driver;

} // namespace hm11

// Include the driver implementation
#include "driver.hpp"

#endif // HM11_HM11_HPP
