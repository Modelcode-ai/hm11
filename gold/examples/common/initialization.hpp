/**
 * @file initialization.hpp
 * @brief Common initialization helpers for HM-11 driver examples
 *
 * This header provides reusable initialization sequences used across all
 * HM-11 driver examples. These helpers encapsulate common patterns from
 * Ada's hm11_example.adb initialization sequence.
 *
 * ## Usage Pattern
 *
 * Examples use these helpers to reduce boilerplate:
 * ```cpp
 * #include "examples/common/initialization.hpp"
 * #include "examples/common/led_control.hpp"
 *
 * // Standard initialization sequence
 * init_hardware();  // Hardware delay + UART + DMA
 *
 * // Get driver instance
 * auto& driver = get_hm11_driver();
 *
 * // Verify module ready
 * verify_module_ready(driver);  // Test command + MAC address
 * ```
 *
 * ## Design Principles
 *
 * - **Encapsulation**: Common initialization patterns extracted into functions
 * - **Ada Pattern Match**: Timing and sequence match Ada's hm11_example.adb
 * - **Platform Independence**: Works for both STM32 and test builds
 * - **Error Handling**: Integrated with error_handling.hpp for consistent behavior
 *
 * ## Ada Source Reference
 *
 * This header implements C++ equivalents of Ada's initialization:
 * - Init_UART + Initialize_DMA sequence (hm11_example.adb:101-119)
 * - Test + Get_MAC_Address verification (hm11_example.adb:121-135)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#ifndef HM11_EXAMPLES_COMMON_INITIALIZATION_HPP
#define HM11_EXAMPLES_COMMON_INITIALIZATION_HPP

#include <chrono>
#include <string_view>
#include <thread>

#include "examples/common/error_handling.hpp"
#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/stm32/drivers.hpp"
#include "hm11/types/type_aliases.hpp"

namespace hm11::examples {

/**
 * @brief Initialize hardware with standard timing sequence
 *
 * Performs complete hardware initialization sequence:
 * 1. Delay 2 seconds for hardware power-up stabilization
 * 2. Initialize UART5 peripheral
 * 3. Delay 1 second for UART stabilization
 * 4. Initialize DMA for UART5 reception
 * 5. Delay 1 second for DMA initialization
 * 6. Initialize LEDs (if available)
 * 7. Indicate successful initialization
 *
 * This matches Ada's initialization sequence from hm11_example.adb:101-119.
 *
 * @note This function includes delays totaling 4 seconds
 */
inline void init_hardware() noexcept {
    // Step 1: Delay for hardware power-up stabilization
    // HM-11 module needs time to power up and become responsive
    // Ada example uses 2 second delay (hm11_example.adb:101)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Step 2: Initialize UART5 peripheral
    // Configures GPIO pins (PC12=TX, PD2=RX) and UART5 registers
    // Baud rate: 9600, format: 8N1, no flow control
    stm32::init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 3: Initialize DMA for UART5 reception
    // Configures DMA1 Stream 0 Channel 4 for UART5_RX
    // Also enables UART5 and DMA interrupts in NVIC
    stm32::initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 4: Initialize LED hardware (if platform supports it)
    INIT_LEDS();

    // Initialization complete - indicate success
    indicate_success();
}

/**
 * @brief Verify HM-11 module is ready and responsive
 *
 * Performs basic verification sequence:
 * 1. Test command (AT) to verify module responsiveness
 * 2. Get MAC address to verify module identity
 *
 * This matches Ada's verification from hm11_example.adb:121-135.
 *
 * @tparam DriverType Type of HM-11 driver (template for dependency injection)
 * @param driver HM-11 driver instance
 *
 * @note Function enters error state if verification fails
 */
template <typename DriverType>
inline void verify_module_ready(DriverType& driver) noexcept {
    hm11::hal::UartStatus status{};

    // Test module responsiveness
    // Sends "AT" command, expects "OK" response
    driver.test(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Get and verify MAC address
    // This verifies module identity and validates parsing
    hm11::types::MacAddress mac_address{"000000000000"};
    driver.get_mac_address(mac_address, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

/**
 * @brief Configure module as peripheral with standard settings
 *
 * Performs complete peripheral configuration sequence:
 * 1. Set advertising type (scannable and connectable)
 * 2. Set bond mode (authentication with PIN)
 * 3. Enable connection notifications
 * 4. Set module name
 * 5. Set PIN code
 * 6. Set work type (start immediately)
 * 7. Set role to peripheral
 * 8. Restart module to apply configuration
 *
 * This matches Ada's peripheral configuration from hm11_example.adb:137-168.
 *
 * @tparam DriverType Type of HM-11 driver
 * @param driver HM-11 driver instance
 * @param name Module name to advertise (max 12 characters)
 * @param pin PIN code for authentication (6 digits)
 *
 * @note Function enters error state if any configuration step fails
 * @note Total time: ~10 seconds including delays between commands
 */
template <typename DriverType>
inline void configure_as_peripheral(DriverType& driver, std::string_view name, const hm11::types::PinType& pin) noexcept {
    hm11::hal::UartStatus status{};

    // Configure advertising type: scannable and connectable
    driver.set_advertising_type(hm11::types::AdvertisingType::AdvertisingScanResponseConnectable, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure bond mode: require PIN authentication
    driver.set_bond_mode(hm11::types::BondMode::AuthWithPin, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Enable connection notifications (OK+CONN, OK+LOST messages)
    driver.set_notify_information(true, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set module name (advertised name visible to central devices)
    driver.set_module_name(name, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set PIN code for authentication
    driver.set_pin_code(pin, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set work type: start immediately (no wake-up key needed)
    driver.set_work_type(hm11::types::WorkType::StartImmediately, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set role to peripheral (advertises and accepts connections)
    driver.set_role(hm11::types::Role::Peripheral, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Restart module to apply all configuration changes
    driver.restart(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

/**
 * @brief Configure module as central (scanner/initiator)
 *
 * Performs central role configuration sequence:
 * 1. Set work type (respond to AT commands, required for scan)
 * 2. Set role to central
 *
 * Central role allows scanning for nearby BLE devices and initiating connections.
 *
 * @tparam DriverType Type of HM-11 driver
 * @param driver HM-11 driver instance
 *
 * @note Function enters error state if configuration fails
 * @note Total time: ~1.5 seconds including delays
 */
template <typename DriverType>
inline void configure_as_central(DriverType& driver) noexcept {
    hm11::hal::UartStatus status{};

    // Set work type: respond to AT commands (required for scan operation)
    driver.set_work_type(hm11::types::WorkType::RespondAtCommand, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set role to central (scans and initiates connections)
    driver.set_role(hm11::types::Role::Central, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

} // namespace hm11::examples

#endif // HM11_EXAMPLES_COMMON_INITIALIZATION_HPP
