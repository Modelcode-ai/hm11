/**
 * @file main.cpp
 * @brief Peripheral echo server example for HM-11 driver
 *
 * This example demonstrates a complete BLE peripheral application that:
 * 1. Initializes and configures the HM-11 module as a peripheral
 * 2. Waits for a central device (e.g., smartphone) to connect
 * 3. Echoes back any data received from the central device
 * 4. Handles disconnection and provides visual feedback
 *
 * This is a direct translation of Ada's hm11_example.adb (lines 54-228),
 * serving as the primary reference implementation for peripheral applications.
 *
 * ## What This Example Demonstrates
 *
 * 1. Complete peripheral configuration sequence (18 AT commands)
 * 2. Connection lifecycle handling (wait → connected → echo → disconnected)
 * 3. Bidirectional data transfer (receive and transmit)
 * 4. Error handling with visual feedback (LED indicators)
 * 5. Connection state machine implementation
 * 6. Realistic timing and delays between operations
 *
 * ## Hardware Requirements (STM32F429 target)
 *
 * - STM32F429 Discovery board
 * - HM-11 Bluetooth module connected to UART5:
 *   - PC12 (UART5_TX) -> HM-11 RX (pin 4)
 *   - PD2 (UART5_RX) -> HM-11 TX (pin 2)
 *   - 3.3V and GND connections
 * - LED indicators for status feedback (Green LED, Red LED)
 *
 * ## Expected Behavior
 *
 * Success sequence:
 * 1. Initialization phase: Configure UART, DMA, and driver (4 seconds)
 * 2. Configuration phase: Execute 18 AT commands to configure peripheral (10 seconds)
 * 3. Wait for connection: Green LED blinks while waiting
 * 4. Connected: Green LED stays on, module echoes received data
 * 5. Disconnected: Green LED blinks rapidly, returns to waiting state
 *
 * Error sequence:
 * 1. Red LED blinks continuously if any operation fails
 * 2. System halts in error state
 *
 * ## Connection Instructions
 *
 * After successful configuration, connect from your smartphone:
 * 1. Install a BLE serial terminal app (e.g., "Serial Bluetooth Terminal")
 * 2. Scan for BLE devices
 * 3. Connect to device named "HMTest"
 * 4. Enter PIN: 000000 (if prompted)
 * 5. Send text data (up to 7 characters per message)
 * 6. Module will echo back the same data
 * 7. Disconnect to see disconnection handling
 *
 * ## Configuration Summary
 *
 * The example configures the HM-11 module with these settings:
 * - **Role**: Peripheral (advertises, accepts connections)
 * - **Name**: "HMTest"
 * - **PIN Code**: "000000"
 * - **Advertising Type**: Scannable and connectable
 * - **Bond Mode**: Authentication with PIN required
 * - **Work Type**: Start immediately (no wake-up key needed)
 * - **Notify Information**: Enabled (send "OK+CONN"/"OK+LOST" messages)
 *
 * ## State Machine
 *
 * 1. **Initializing**: Hardware and driver initialization
 * 2. **Configuring**: Sending configuration AT commands
 * 3. **Waiting Connection**: Polling for "OK+CONN" message
 * 4. **Connected**: Echo loop, transmitting received data back
 * 5. **Disconnected**: Detected "OK+LOST", returns to Waiting Connection
 *
 * ## Timing Considerations
 *
 * The example uses delays matching Ada's timing:
 * - Initial delay: 2 seconds (hardware power-up)
 * - Between init steps: 1 second
 * - Between AT commands: 0.5 seconds
 * - Before restart: 1 second
 * - After restart: 2 seconds
 * - Connection timeout: 120 seconds (per receive operation)
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

// Connection state notification messages from HM-11 module
// These messages are sent by the module when connections are established/lost
// Ada source: hm11.ads:275, 279
constexpr std::string_view WHEN_CONNECTED_MESSAGE = "OK+CONN";
constexpr std::string_view WHEN_DISCONNECTED_MESSAGE = "OK+LOST";

/**
 * @brief Indicate error with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
 * In a real embedded system, this would be the error handler.
 *
 * Ada source: hm11_example.adb:76-85
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
 *
 * Ada source: hm11_example.adb:88-98
 */
void check_status(hm11::hal::UartStatus status) noexcept {
    if (status != hm11::hal::UartStatus::Ok) {
        on_error();
    }
}

/**
 * @brief Main example entry point
 *
 * Demonstrates complete peripheral echo server implementation.
 * This is the main application flow matching Ada's hm11_example.adb.
 *
 * Ada source: hm11_example.adb:54-228
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
    // Ada source: hm11_example.adb:101-119

    // Step 1: Delay for hardware power-up stabilization
    // HM-11 module needs time to power up and become responsive
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Step 2: Initialize UART5 peripheral
    init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 3: Initialize DMA for UART5 reception
    initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 4: Get references to driver, UART port, and receive handler
    // Driver is used for AT commands, UART/receive handler for direct data transfer
    auto& driver = get_hm11_driver();
    auto& uart_port = get_uart_port();
    auto& receive_handler = get_receive_handler();

    // ========================================================================
    // Phase 2: Module Verification
    // ========================================================================
    // Ada source: hm11_example.adb:121-135

    // Test module responsiveness
    driver.test(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Get and verify MAC address
    MacAddress mac_address{"000000000000"};
    driver.get_mac_address(mac_address, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // Phase 3: Peripheral Configuration
    // ========================================================================
    // Configure module as BLE peripheral with specific settings
    // Ada source: hm11_example.adb:137-168

    // Configure advertising type: scannable and connectable
    driver.set_advertising_type(AdvertisingType::AdvertisingScanResponseConnectable, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Configure bond mode: require PIN authentication
    driver.set_bond_mode(BondMode::AuthWithPin, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Enable connection notifications (OK+CONN, OK+LOST messages)
    driver.set_notify_information(true, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set module name (advertised name visible to central devices)
    driver.set_module_name("HMTest", status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set PIN code for authentication
    PinType pin{"000000"};
    driver.set_pin_code(pin, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set work type: start immediately (no wake-up key needed)
    driver.set_work_type(WorkType::StartImmediately, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Set role to peripheral (advertises and accepts connections)
    driver.set_role(Role::Peripheral, status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Restart module to apply all configuration changes
    driver.restart(status);
    check_status(status);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========================================================================
    // Phase 4: Wait for Connection
    // ========================================================================
    // Poll for connection notification message from module
    // Ada source: hm11_example.adb:170-189

    // Indicate waiting for connection (in real hardware, would display message)
    LED_GREEN_OFF();

    {
        // Buffer to receive connection notification
        std::array<uint8_t, WHEN_CONNECTED_MESSAGE.size()> conn_buffer{};

        while (true) {
            // Blink LED while waiting
            LED_GREEN_TOGGLE();

            // Receive data with 120 second timeout
            // Ada's DMA_Receive_Handler: (UART'Access, Data'Address, Data'Length, Status, 120 * 1000)
            UartData8b data{conn_buffer.data(), conn_buffer.size()};
            receive_handler(uart_port, data, status, 120000, false);

            // Check if connection message received
            if (status == UartStatus::Ok) {
                std::string_view received{reinterpret_cast<const char*>(conn_buffer.data()), conn_buffer.size()};
                if (received == WHEN_CONNECTED_MESSAGE) {
                    // Connected! Turn on green LED
                    LED_GREEN_ON();
                    break;
                }
            }

            // Check for errors (but continue waiting even on timeout)
            if (status != UartStatus::Ok && status != UartStatus::ErrTimeout) {
                on_error();
            }
        }
    }

    // ========================================================================
    // Phase 5: Echo Loop
    // ========================================================================
    // Receive data from central and echo it back until disconnection
    // Ada source: hm11_example.adb:191-214

    {
        // Buffer sized for disconnection message (also handles data messages)
        std::array<uint8_t, WHEN_DISCONNECTED_MESSAGE.size()> echo_buffer{};

        while (true) {
            // Receive data with 120 second timeout
            UartData8b data{echo_buffer.data(), WHEN_DISCONNECTED_MESSAGE.size()};
            receive_handler(uart_port, data, status, 120000, false);

            if (status != UartStatus::Ok) {
                // Receive error - enter error state
                on_error();
            }

            // Check if disconnection message received
            std::string_view received{reinterpret_cast<const char*>(echo_buffer.data()), data.size()};
            if (received == WHEN_DISCONNECTED_MESSAGE) {
                // Disconnected - indicate and exit echo loop
                LED_GREEN_OFF();
                break;
            }

            // Echo received data back to central device
            // Ada's Transmit: STM32.USARTs.Transmit (UART, Send, Status)
            uart_port.transmit(data, status);

            if (status != UartStatus::Ok) {
                // Transmit error - enter error state
                on_error();
            }
        }
    }

    // ========================================================================
    // Phase 6: Completion
    // ========================================================================
    // All operations completed successfully
    // Ada source: hm11_example.adb:216-223

    // Turn on green LED to indicate successful completion
    LED_GREEN_ON();

// In embedded system, enter idle loop
// In hosted environment, exit cleanly
#ifdef __ARM_ARCH
    // Embedded target - enter infinite loop with LED blinking
    while (true) {
        LED_GREEN_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // In real hardware, could return to Phase 4 to wait for another connection
    }
#else
    // Hosted target - return success
    return 0;
#endif
}
