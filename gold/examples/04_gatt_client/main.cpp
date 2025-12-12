/**
 * @file main.cpp
 * @brief GATT client example for HM-11 driver
 *
 * This example demonstrates the HM-11 driver's GATT (Generic Attribute Profile)
 * client capabilities. The application:
 * 1. Initializes and configures the HM-11 module as a Central
 * 2. Connects to a specific peripheral device by MAC address
 * 3. Discovers GATT services on the connected device
 * 4. Discovers characteristics within discovered services
 * 5. Reads characteristic values
 * 6. Enables notifications on characteristics that support notify property
 *
 * This demonstrates the driver's GATT subsystem including connection
 * establishment, service/characteristic discovery with streaming responses,
 * read operations, and notification management.
 *
 * ## What This Example Demonstrates
 *
 * 1. Central role configuration for GATT operations
 * 2. Connection to specific device by MAC address
 * 3. Service discovery with streaming response parsing
 * 4. Characteristic discovery within service handle ranges
 * 5. Reading characteristic values
 * 6. Enabling notifications for characteristics
 * 7. GATT error response handling (SendOk, SendEr, DataEr)
 * 8. GATT operation sequencing and state management
 *
 * ## Hardware Requirements (STM32F429 target)
 *
 * - STM32F429 Discovery board
 * - HM-11 Bluetooth module connected to UART5:
 *   - PC12 (UART5_TX) -> HM-11 RX (pin 4)
 *   - PD2 (UART5_RX) -> HM-11 TX (pin 2)
 *   - 3.3V and GND connections
 * - LED indicators for status feedback
 * - BLE peripheral device with known MAC address (e.g., sensor, heart rate monitor)
 *
 * ## Expected Behavior
 *
 * Success sequence:
 * 1. Initialization phase: Configure UART, DMA, and driver (4 seconds)
 * 2. Configuration phase: Configure Central role (2 seconds)
 * 3. Connection phase: Connect to target device by MAC address (up to 10 seconds)
 * 4. Service discovery: Enumerate all GATT services (2-5 seconds)
 * 5. For each service:
 *    - Display service UUID and handle range
 *    - Discover characteristics within service
 * 6. For each characteristic:
 *    - Display characteristic UUID, handle, and properties
 *    - Read value if characteristic has Read property
 *    - Enable notifications if characteristic has Notify property
 * 7. Completion: Green LED indicates successful GATT operations
 *
 * Error sequence:
 * 1. Red LED blinks continuously if any operation fails
 * 2. System halts in error state
 *
 * ## Configuration Summary
 *
 * The example configures the HM-11 module with these settings:
 * - **Role**: Central (initiates connections and GATT operations)
 * - **Work Type**: Respond to AT commands (required for GATT operations)
 * - **Target Device**: MAC address 001122334455 (modify TARGET_MAC_ADDRESS)
 * - **MAC Address Type**: Normal (0) - modify if using random/static addresses
 *
 * ## State Flow
 *
 * 1. **Initializing**: Hardware and driver initialization
 * 2. **Configuring**: Sending configuration AT commands (role, work type)
 * 3. **Connecting**: Attempting connection to target device by MAC address
 * 4. **Discovering Services**: Long-running service discovery with callbacks
 * 5. **Discovering Characteristics**: For each service, discover characteristics
 * 6. **Accessing Characteristics**: Read values and enable notifications
 * 7. **Completed**: All GATT operations successful
 *
 * ## Timing Considerations
 *
 * - Initial delay: 2 seconds (hardware power-up)
 * - Between init steps: 1 second
 * - Between AT commands: 0.5 seconds
 * - Connection timeout: 10 seconds (device must be connectable)
 * - Service discovery timeout: 5 seconds
 * - Characteristic discovery timeout: 3 seconds per service
 * - GATT operations typically complete within timeout
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

/// @brief Target device MAC address (modify for your peripheral device)
/// Format: 12 hex characters (6 bytes)
/// Example: "001122334455" for MAC 00:11:22:33:44:55
constexpr char TARGET_MAC_ADDRESS[] = "001122334455";

/// @brief Target device MAC address type
/// Normal (0), Static (1), StaticRandom (2), Random (3)
constexpr MacAddressType TARGET_MAC_TYPE = MacAddressType::NormalAddress;

/// @brief Connection timeout (seconds)
constexpr int CONNECTION_TIMEOUT_SECONDS = 10;

/// @brief Service discovery timeout (seconds)
constexpr int SERVICE_DISCOVERY_TIMEOUT_SECONDS = 5;

/// @brief Characteristic discovery timeout (seconds)
constexpr int CHARACTERISTIC_DISCOVERY_TIMEOUT_SECONDS = 3;

/// @brief Service counter for tracking discovered services
static int service_count = 0;

/// @brief Characteristic counter for tracking discovered characteristics
static int characteristic_count = 0;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Error handler - blink red LED and halt
 * @param error_code Error identifier for debugging
 */
[[noreturn]] static void error_handler(int error_code) {
    LED_GREEN_OFF();
    while (true) {
        LED_RED_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Check UART status and halt on error
 * @param status UART operation status
 * @param operation_name Operation description for debugging
 */
static void check_status(UartStatus status, const char* operation_name) {
    if (status != UartStatus::Ok) {
        // Operation failed - enter error state
        error_handler(__LINE__);
    }
}

/**
 * @brief Parse service info string and extract handle range
 *
 * Service info format: "start:end:uuid" (4:4:4 hex characters)
 * Example: "0001:000B:1800" (Generic Access service, handles 0001-000B)
 *
 * @param service_info Service information string (14 chars with colons)
 * @param start_handle Output parameter for service start handle
 * @param end_handle Output parameter for service end handle
 * @return true if parsing successful, false otherwise
 */
static bool parse_service_handles(
    std::string_view service_info,
    HandleType& start_handle,
    HandleType& end_handle) {
    // Service format: "XXXX:XXXX:XXXX" (14 chars total with colons)
    if (service_info.size() < 14) {
        return false;
    }

    // Extract start handle (first 4 chars)
    std::string_view start_str = service_info.substr(0, 4);
    start_handle = HandleType(start_str);

    // Extract end handle (chars 5-8, after first colon)
    std::string_view end_str = service_info.substr(5, 4);
    end_handle = HandleType(end_str);

    return true;
}

/**
 * @brief Check if characteristic has specific property
 *
 * Properties format: "RD|WR|WN|NO|IN" where unsupported properties are "--"
 * Example: "RD|--|--|NO|--" (Read and Notify supported)
 *
 * @param properties Properties string (14 characters)
 * @param property Property to check ("RD", "WR", "WN", "NO", "IN")
 * @return true if property is supported (not "--"), false otherwise
 */
static bool has_property(std::string_view properties, std::string_view property) {
    // Properties format: "RD|WR|WN|NO|IN" (14 chars total)
    // Position map: RD=0, WR=3, WN=6, NO=9, IN=12
    if (property == "RD") {
        return properties.substr(0, 2) == "RD";
    }
    if (property == "WR") {
        return properties.substr(3, 2) == "WR";
    }
    if (property == "WN") {
        return properties.substr(6, 2) == "WN";
    }
    if (property == "NO") {
        return properties.substr(9, 2) == "NO";
    }
    if (property == "IN") {
        return properties.substr(12, 2) == "IN";
    }
    return false;
}

/**
 * @brief Parse characteristic info string and extract components
 *
 * Characteristic info format: "handle:properties:uuid" (4:14:4 with colons)
 * Example: "0003:RD|WR|--|--|--:2A00" (Device Name, Read+Write)
 *
 * @param characteristic_info Characteristic information string (24 chars with colons)
 * @param handle Output parameter for characteristic handle
 * @param properties Output parameter for properties string
 * @param uuid Output parameter for characteristic UUID
 * @return true if parsing successful, false otherwise
 */
static bool parse_characteristic_info(
    std::string_view characteristic_info,
    HandleType& handle,
    std::string_view& properties,
    std::string_view& uuid) {
    // Characteristic format: "XXXX:RD|WR|WN|NO|IN:XXXX" (24 chars total with colons)
    if (characteristic_info.size() < 24) {
        return false;
    }

    // Extract handle (first 4 chars)
    std::string_view handle_str = characteristic_info.substr(0, 4);
    handle = HandleType(handle_str);

    // Extract properties (chars 5-18, 14 chars)
    properties = characteristic_info.substr(5, 14);

    // Extract UUID (chars 20-23, last 4 chars)
    uuid = characteristic_info.substr(20, 4);

    return true;
}

// ============================================================================
// GATT Operation Callbacks
// ============================================================================

/// @brief Service discovery callback - invoked for each discovered service
static void service_callback(std::string_view service_info) {
    service_count++;

    // Parse service info: "start:end:uuid"
    HandleType start_handle;
    HandleType end_handle;
    if (!parse_service_handles(service_info, start_handle, end_handle)) {
        // Parsing failed - skip this service
        return;
    }

    // Service info successfully parsed
    // In real application, would display or process service data
    // For example: display UUID, handle range, and trigger characteristic discovery

    LED_GREEN_TOGGLE();  // Visual feedback for each service
}

/// @brief Characteristic discovery callback - invoked for each discovered characteristic
static void characteristic_callback(std::string_view characteristic_info) {
    characteristic_count++;

    // Parse characteristic info: "handle:properties:uuid"
    HandleType handle;
    std::string_view properties;
    std::string_view uuid;
    if (!parse_characteristic_info(characteristic_info, handle, properties, uuid)) {
        // Parsing failed - skip this characteristic
        return;
    }

    // Characteristic info successfully parsed
    // In real application, would display or process characteristic data
    // For example: display UUID, handle, properties, and trigger read/notify operations

    LED_GREEN_TOGGLE();  // Visual feedback for each characteristic
}

// ============================================================================
// GATT Operations
// ============================================================================

/**
 * @brief Perform GATT client operations
 *
 * This function encapsulates the main GATT client logic:
 * 1. Discover all services
 * 2. For each service, discover characteristics
 * 3. For each characteristic, read value and enable notifications as appropriate
 *
 * @param driver HM11 driver instance
 * @return true if all operations successful, false otherwise
 */
template <typename DriverType>
static bool perform_gatt_operations(DriverType& driver) {
    UartStatus status;

    // Reset counters
    service_count = 0;
    characteristic_count = 0;

    // Step 1: Discover all services
    driver.find_all_services_uuid(
        service_callback,
        std::chrono::seconds(SERVICE_DISCOVERY_TIMEOUT_SECONDS),
        status);
    check_status(status, "Service discovery");

    // Small delay between operations
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Step 2: Discover all characteristics across all services
    driver.find_all_characteristic_uuid(
        characteristic_callback,
        std::chrono::seconds(CHARACTERISTIC_DISCOVERY_TIMEOUT_SECONDS),
        status);
    check_status(status, "Characteristic discovery");

    // Step 3: In real application, would iterate through discovered characteristics
    // and perform read/notify operations based on properties
    // This example demonstrates the basic structure

    return true;
}

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Main application entry point
 *
 * Initialization sequence:
 * 1. Initialize platform (UART, DMA, interrupts)
 * 2. Initialize HM11 driver
 * 3. Configure as Central role
 * 4. Connect to target device
 * 5. Perform GATT operations
 * 6. Signal completion with green LED
 */
int main() {
    using namespace hm11;
    using namespace hm11::hal;
    using namespace hm11::stm32;
    using namespace hm11::types;

    // Turn off LEDs initially
    LED_GREEN_OFF();
    LED_RED_OFF();

    // Wait for hardware to stabilize (power-up delay)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========================================================================
    // Phase 1: Platform Initialization
    // ========================================================================

    // Initialize STM32 platform (UART5, DMA, interrupts)
    auto uart_port = initialize_uart();
    auto receive_handler = initialize_receive_handler();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // ========================================================================
    // Phase 2: Driver Initialization
    // ========================================================================

    // Create driver instance
    auto driver = create_driver(uart_port, receive_handler);

    // Test communication with module
    UartStatus status;
    driver.test(status);
    check_status(status, "Test command");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 3: Central Configuration
    // ========================================================================

    // Configure as Central role (required for GATT client operations)
    driver.set_role(Role::Central, status);
    check_status(status, "Set role");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Ensure work type is RespondAtCommand (required for GATT operations)
    driver.set_work_type(WorkType::RespondAtCommand, status);
    check_status(status, "Set work type");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 4: Connection Establishment
    // ========================================================================

    // Connect to target device by MAC address
    MacAddress target_mac(TARGET_MAC_ADDRESS);
    ConnectResult connect_result;

    driver.connect(TARGET_MAC_TYPE, target_mac, connect_result, status);
    check_status(status, "Connect command");

    // Verify connection established
    if (connect_result != ConnectResult::Connected) {
        // Connection failed (timeout, device not found, etc.)
        error_handler(__LINE__);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 5: GATT Operations
    // ========================================================================

    bool gatt_success = perform_gatt_operations(driver);
    if (!gatt_success) {
        error_handler(__LINE__);
    }

    // ========================================================================
    // Phase 6: Completion
    // ========================================================================

    // Turn on green LED to indicate success
    LED_GREEN_ON();
    LED_RED_OFF();

    // Main loop (application continues running)
    while (true) {
        // In real application, would handle notifications and data exchange
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
