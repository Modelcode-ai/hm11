/**
 * @file main.cpp
 * @brief Central scanner example for HM-11 driver
 *
 * This example demonstrates the HM-11 driver's Central role scanning
 * capabilities. The application:
 * 1. Initializes and configures the HM-11 module as a Central (scanner)
 * 2. Configures device information display options
 * 3. Performs device discovery scan operation
 * 4. Displays discovered devices with optional RSSI filtering
 * 5. Handles streaming discovery data via callback
 *
 * This demonstrates the driver's discovery subsystem including streaming data
 * parsing, callback-based device notification, and handling variable-length
 * device names/RSSI values.
 *
 * ## What This Example Demonstrates
 *
 * 1. Central role configuration (scanner/initiator)
 * 2. Discovery operation with streaming response parsing
 * 3. Callback-based device notification pattern
 * 4. RSSI-based proximity filtering
 * 5. Handling empty Name/RSSI fields based on configuration
 * 6. Long-running streaming operation management
 *
 * ## Hardware Requirements (STM32F429 target)
 *
 * - STM32F429 Discovery board
 * - HM-11 Bluetooth module connected to UART5:
 *   - PC12 (UART5_TX) -> HM-11 RX (pin 4)
 *   - PD2 (UART5_RX) -> HM-11 TX (pin 2)
 *   - 3.3V and GND connections
 * - LED indicators for status feedback
 *
 * ## Expected Behavior
 *
 * Success sequence:
 * 1. Initialization phase: Configure UART, DMA, and driver (4 seconds)
 * 2. Configuration phase: Configure Central role and discovery options (5 seconds)
 * 3. Scanning phase: Discover nearby BLE devices (10 seconds scan duration)
 * 4. For each discovered device:
 *    - Display device ID, MAC address, name (if configured), RSSI (if configured)
 *    - Apply RSSI threshold filter (only show devices above threshold)
 * 5. Completion: Green LED indicates successful scan completion
 *
 * Error sequence:
 * 1. Red LED blinks continuously if any operation fails
 * 2. System halts in error state
 *
 * ## Configuration Summary
 *
 * The example configures the HM-11 module with these settings:
 * - **Role**: Central (scans and initiates connections)
 * - **Work Type**: Respond to AT commands (required for scan)
 * - **Show Device Information**: RSSI and Name (both displayed)
 * - **Discovery Time**: Default (configured by Set_Discovery_Time if needed)
 * - **RSSI Threshold**: -70 dBm (only show devices with signal strength > -70)
 *
 * ## State Flow
 *
 * 1. **Initializing**: Hardware and driver initialization
 * 2. **Configuring**: Sending configuration AT commands
 * 3. **Scanning**: Long-running scan operation with callback invocations
 * 4. **Processing**: Callback receives each discovered device incrementally
 * 5. **Completed**: Scan timeout or early termination ("OK+DISCE" received)
 *
 * ## Timing Considerations
 *
 * - Initial delay: 2 seconds (hardware power-up)
 * - Between init steps: 1 second
 * - Between AT commands: 0.5 seconds
 * - Scan timeout: 10 seconds (configurable)
 * - Discovery typically takes 3-10 seconds depending on nearby device density
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <thread>

#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/stm32/drivers.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

// ============================================================================
// Configuration Constants
// ============================================================================

/// @brief RSSI threshold for proximity filtering (dBm)
/// Only devices with RSSI > threshold will be displayed
/// Typical values: -90 (very far), -70 (medium), -50 (close), -30 (very close)
constexpr int RSSI_THRESHOLD_DBM = -70;

/// @brief Scan timeout duration (seconds)
/// Maximum time to wait for device discovery operation
constexpr int SCAN_TIMEOUT_SECONDS = 10;

/// @brief Device counter for tracking discovered devices
static int device_count = 0;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Indicate error with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
 * In a real embedded system, this would be the error handler.
 */
[[noreturn]] void on_error() noexcept {
    while (true) {
        LED_RED_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Check operation status and indicate error if failed
 *
 * Provides consistent error handling pattern used throughout the example.
 * If status is Ok, continues normally. Otherwise, enters error state.
 *
 * @param status Operation status to check
 */
void check_status(UartStatus status) noexcept {
    if (status != UartStatus::Ok) {
        on_error();
    }
}

/**
 * @brief Parse RSSI string to integer value
 *
 * Converts RSSI string (e.g., "-045", "-070") to integer dBm value.
 * HM-11 returns RSSI in format: "-XXX" where XXX is absolute value.
 *
 * @param rssi_str RSSI string from discovery callback
 * @return Parsed RSSI value in dBm, or 0 if parsing fails
 */
int parse_rssi(std::string_view rssi_str) noexcept {
    if (rssi_str.empty()) {
        return 0; // No RSSI information available
    }

    // Skip leading minus sign and parse absolute value
    int value = 0;
    bool negative = false;
    std::size_t i = 0;

    if (rssi_str[0] == '-') {
        negative = true;
        i = 1;
    }

    for (; i < rssi_str.size(); ++i) {
        if (rssi_str[i] >= '0' && rssi_str[i] <= '9') {
            value = value * 10 + (rssi_str[i] - '0');
        }
    }

    return negative ? -value : value;
}

/**
 * @brief Discovery callback function invoked for each discovered device
 *
 * This callback is invoked inline during parsing of streaming discovery data.
 * It receives device information and applies RSSI filtering before displaying.
 *
 * ## RSSI Filtering Logic
 *
 * Devices are only displayed if their RSSI value exceeds RSSI_THRESHOLD_DBM.
 * This allows proximity-based filtering (e.g., only show nearby devices).
 *
 * ## Empty Field Handling
 *
 * Name and RSSI may be empty depending on Set_Show_Device_Information_When_Discovery
 * configuration. Empty fields are handled gracefully:
 * - Empty name: Display "(no name)"
 * - Empty RSSI: Skip filtering (show all devices)
 *
 * @param id Device ID character (index in discovered devices list)
 * @param mac 12-character hex MAC address
 * @param name Device name (may be empty)
 * @param rssi RSSI value string (may be empty)
 */
void on_device_discovered(
    char id,
    const types::MacAddress& mac,
    std::string_view name,
    std::string_view rssi) noexcept {
    // Apply RSSI threshold filter if RSSI available
    if (!rssi.empty()) {
        int rssi_dbm = parse_rssi(rssi);
        if (rssi_dbm <= RSSI_THRESHOLD_DBM) {
            // Device at or below threshold, skip display
            return;
        }
    }

    // Increment device counter
    ++device_count;

    // Display device information
    // In real hardware build, this would output to UART console or LCD
    // In tests, these calls are mocked for validation
    (void)id;   // Device ID (unused in this simple display)
    (void)mac;  // MAC address
    (void)name; // Device name
    (void)rssi; // RSSI value

    // Visual feedback: blink LED for each discovered device
    LED_GREEN_TOGGLE();
}

/**
 * @brief Main example entry point
 *
 * Demonstrates complete Central scanner implementation.
 * This is the main application flow for device discovery.
 */
int main() {
    using namespace hm11;
    using namespace hm11::hal;
    using namespace hm11::stm32;
    using namespace hm11::types;

    UartStatus status;

    // ========================================================================
    // Phase 1: Hardware Initialization
    // ========================================================================

    // Step 1: Delay for hardware power-up stabilization
    // HM-11 module needs time to power up and become responsive
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Step 2: Initialize UART5 peripheral
    init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 3: Initialize DMA for UART5 reception
    initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 4: Get reference to driver
    auto& driver = get_hm11_driver();

    // ========================================================================
    // Phase 2: Module Verification
    // ========================================================================

    // Test module responsiveness
    driver.test(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 3: Central Configuration
    // ========================================================================
    // Configure module as BLE Central (scanner/initiator)

    // Set work type to respond to AT commands (required for scan)
    driver.set_work_type(WorkType::RespondAtCommand, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure device information display (show both name and RSSI)
    driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowRssiAndName, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set role to Central (scans for peripherals)
    driver.set_role(Role::Central, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Restart module to apply all configuration changes
    driver.restart(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========================================================================
    // Phase 4: Scanning Operation
    // ========================================================================
    // Perform device discovery scan with callback

    // Reset device counter
    device_count = 0;

    // Indicate scanning in progress
    LED_GREEN_OFF();

    // Perform scan operation (streaming, may take several seconds)
    // Callback will be invoked for each discovered device
    driver.scan(on_device_discovered, std::chrono::seconds(SCAN_TIMEOUT_SECONDS), status);

    // Check scan status
    if (status != UartStatus::Ok && status != UartStatus::ErrTimeout) {
        // Scan failed with error (timeout is acceptable - means no more devices)
        on_error();
    }

    // ========================================================================
    // Phase 5: Completion
    // ========================================================================
    // Display scan results summary

    // Indicate successful completion
    LED_GREEN_ON();

    // In real hardware, would display summary:
    // - Total devices discovered
    // - Devices above RSSI threshold
    // For this example, device_count tracks qualifying devices
    (void)device_count; // Suppress unused warning

// In embedded system, enter idle loop
// In hosted environment, exit cleanly
#ifdef __ARM_ARCH
    // Embedded target - enter infinite loop with LED blinking
    while (true) {
        LED_GREEN_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // In real application, could perform periodic re-scans
    }
#else
    // Hosted target - return success
    return 0;
#endif
}
