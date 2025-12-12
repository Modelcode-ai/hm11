/**
 * @file main.cpp
 * @brief iBeacon broadcaster example for HM-11 driver
 *
 * This example demonstrates iBeacon configuration and broadcasting. The HM-11
 * module is configured as an iBeacon broadcaster with a specific UUID, major
 * and minor version numbers, and measured power, then activated in deploy mode
 * to continuously broadcast iBeacon advertisements.
 *
 * ## What This Example Demonstrates
 *
 * 1. iBeacon UUID configuration (128-bit proximity UUID)
 * 2. iBeacon major and minor version configuration (16-bit identifiers)
 * 3. Measured power setting for distance estimation
 * 4. iBeacon switch enable/disable
 * 5. Deploy mode activation (broadcast-only mode)
 * 6. Strong type validation with FixedString types (Decision 14)
 * 7. Module reset behavior after deploy mode
 *
 * ## iBeacon Overview
 *
 * iBeacon is Apple's proximity beacon technology based on BLE advertising.
 * It enables:
 * - Indoor positioning and navigation
 * - Proximity-based notifications and content delivery
 * - Retail applications (product information, offers)
 * - Attendance tracking and presence detection
 *
 * iBeacon broadcasts consist of:
 * - **UUID (128-bit)**: Application-specific identifier (same for all beacons
 *   in a deployment)
 * - **Major (16-bit)**: Group identifier (e.g., store location)
 * - **Minor (16-bit)**: Individual beacon identifier (e.g., department)
 * - **Measured Power**: RSSI at 1 meter (used for distance estimation)
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
 * 1. Initialization phase: Configure UART, DMA, and driver (~4 seconds)
 * 2. Configuration phase: Test module, set iBeacon parameters (5 commands)
 * 3. Deploy mode: Activate broadcast-only mode
 * 4. Module resets after ~500ms and begins broadcasting
 * 5. Green LED stays on to indicate successful deployment
 * 6. Module broadcasts iBeacon until power cycled
 *
 * Error sequence:
 * 1. Red LED blinks continuously if any operation fails
 * 2. System halts in error state
 *
 * ## iBeacon Detection
 *
 * After deployment, detect the beacon using:
 * 1. iPhone/iPad: Install "Locate Beacon" or similar iBeacon scanner app
 * 2. Android: Install "Beacon Simulator" or "nRF Connect"
 * 3. Look for beacon with configured UUID
 * 4. Verify major/minor versions match configuration
 * 5. Distance estimation uses measured power value
 *
 * ## Configuration Summary
 *
 * This example configures:
 * - **UUID**: E2C56DB5DFFB48D2B060D0F5A71096E0 (example UUID)
 * - **Major**: 0001 (example major version)
 * - **Minor**: 0001 (example minor version)
 * - **Measured Power**: C5 (representing -59 dBm in two's complement)
 * - **Deploy Mode**: Only_Broadcast (non-connectable, broadcast only)
 *
 * ## Important Notes
 *
 * 1. **Deploy Mode Reset**: After activating deploy mode, the module
 *    automatically resets after ~500ms and enters non-connectable status.
 * 2. **Power Cycle Required**: To exit deploy mode and reconfigure, power
 *    cycle the module (disconnect/reconnect power).
 * 3. **UUID Format**: 32 hexadecimal characters without delimiters.
 *    Standard UUID format (8-4-4-4-12) is converted to continuous hex.
 * 4. **Measured Power**: Two's complement 8-bit signed value. 0xC5 = -59 dBm
 *    (typical value for distance estimation at 1 meter).
 *
 * ## Timing Considerations
 *
 * - Initial delay: 2 seconds (hardware power-up)
 * - Between init steps: 1 second
 * - Between AT commands: 0.5 seconds
 * - After deploy mode: Module resets automatically after ~500ms
 *
 * ## Strong Type Validation (Decision 14)
 *
 * This example uses strong types for iBeacon parameters:
 * - `IBeaconUuid`: 32 hexadecimal characters (compile-time size check)
 * - `VersionType`: 4 hexadecimal characters (major/minor)
 * - `MeasuredPower`: 2 hexadecimal characters
 *
 * Character validation ensures only valid hex characters (0-9, A-F) are
 * accepted, preventing configuration errors at compile time or early runtime.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <chrono>
#include <cstdint>
#include <thread>

#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/stm32/drivers.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

/**
 * @brief Indicate error with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
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
 * @brief Main example entry point
 *
 * Demonstrates complete iBeacon broadcaster configuration and deployment.
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

    // Delay for hardware power-up stabilization
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Initialize UART5 peripheral
    init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Initialize DMA for UART5 reception
    initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get driver reference
    auto& driver = get_hm11_driver();

    // ========================================================================
    // Phase 2: Module Verification
    // ========================================================================

    // Test module responsiveness
    driver.test(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 3: iBeacon Configuration
    // ========================================================================
    // Configure iBeacon parameters before enabling
    // Ada source: hm11.adb:1681-1844

    // Configure iBeacon UUID (128-bit proximity identifier)
    // Example UUID: E2C56DB5-DFFB-48D2-B060-D0F5A71096E0 (without delimiters)
    // This UUID should be unique to your application or organization
    IBeaconUuid uuid{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    driver.set_ibeacon_uuid(uuid, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure major version (16-bit group identifier)
    // Example: 0001 = Location/Store identifier
    // All beacons in the same location typically share the same major version
    VersionType major{"0001"};
    driver.set_ibeacon_major_version(major, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure minor version (16-bit individual identifier)
    // Example: 0001 = Specific beacon/department identifier
    // Different beacons in the same location have different minor versions
    VersionType minor{"0001"};
    driver.set_ibeacon_minor_version(minor, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure measured power (RSSI at 1 meter for distance estimation)
    // 0xC5 = -59 dBm in two's complement (typical calibrated value)
    // This value is used by receivers to estimate distance from beacon
    MeasuredPower power{"C5"};
    driver.set_ibeacon_measured_power(power, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Enable iBeacon functionality
    // This activates iBeacon advertising with configured parameters
    // Ada source: hm11.adb:1644-1652
    driver.set_ibeacon_switch(true, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 4: Deploy Mode Activation
    // ========================================================================
    // Enter broadcast-only mode (non-connectable)
    // Ada source: hm11.adb:1162-1171

    // Activate deploy mode: Only_Broadcast
    // After this command, module will:
    // 1. Acknowledge with "OK+DELO2"
    // 2. Reset automatically after ~500ms
    // 3. Enter non-connectable status
    // 4. Continuously broadcast iBeacon advertisements
    // 5. Remain in this mode until power cycled
    driver.set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);
    check_status(status);

    // ========================================================================
    // Phase 5: Completion
    // ========================================================================
    // All operations completed successfully
    // Module will reset and begin broadcasting after ~500ms

    // Turn on green LED to indicate successful deployment
    LED_GREEN_ON();

    // In embedded system, enter idle loop
    // In hosted environment, exit cleanly
#ifdef __ARM_ARCH
    // Embedded target - enter infinite loop
    // Module is now broadcasting iBeacon advertisements
    while (true) {
        LED_GREEN_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Module is in deploy mode and cannot accept AT commands
        // Power cycle required to reconfigure
    }
#else
    // Hosted target - return success
    return 0;
#endif
}
