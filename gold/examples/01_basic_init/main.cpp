/**
 * @file main.cpp
 * @brief Basic initialization example for HM-11 driver
 *
 * This example demonstrates the minimal setup required to initialize the
 * HM-11 driver and execute basic AT commands. It serves as the entry point
 * for users learning the driver API.
 *
 * ## What This Example Demonstrates
 *
 * 1. Hardware initialization sequence (UART + DMA)
 * 2. Driver instance access
 * 3. Basic AT command execution (Test)
 * 4. Information retrieval (Software_Version)
 * 5. Error handling with visual feedback
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
 * ## Expected Output
 *
 * Success sequence:
 * 1. Green LED blinks: Initialization successful
 * 2. Green LED blinks: Test command succeeded
 * 3. Green LED blinks: Software version retrieved
 * 4. Green LED stays on: All operations completed successfully
 *
 * Error sequence:
 * 1. Red LED blinks continuously: Operation failed
 *
 * ## Initialization Sequence
 *
 * The initialization follows the pattern from Ada's hm11_example.adb:
 * 1. Delay 2 seconds for hardware power-up stabilization
 * 2. Initialize UART5 peripheral (GPIO + UART configuration)
 * 3. Delay 1 second for UART stabilization
 * 4. Initialize DMA for UART5 RX
 * 5. Delay 1 second for DMA initialization
 * 6. Execute basic commands to verify module responsiveness
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <chrono>
#include <cstdint>
#include <string_view>
#include <thread>

#include "examples/common/led_control.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/stm32/drivers.hpp"

/**
 * @brief Indicate successful operation with green LED blink
 *
 * Provides visual feedback that an operation completed successfully.
 * Blinks green LED 3 times with 200ms period.
 */
void indicate_success() noexcept {
    for (int i = 0; i < 3; ++i) {
        LED_GREEN_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Indicate error with continuous red LED blinking
 *
 * Provides visual feedback that an operation failed.
 * Enters infinite loop blinking red LED with 200ms period.
 * In a real embedded system, this would be the error handler.
 */
[[noreturn]] void indicate_error() noexcept {
    while (true) {
        LED_RED_TOGGLE();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

/**
 * @brief Main example entry point
 *
 * Demonstrates basic HM-11 driver initialization and usage.
 * This is the "Hello World" equivalent for the HM-11 driver.
 */
int main() {
    using namespace hm11;
    using namespace hm11::hal;
    using namespace hm11::stm32;

    // Step 1: Delay for hardware power-up stabilization
    // HM-11 module needs time to power up and become responsive
    // Ada example uses 2 second delay (hm11_example.adb:101)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Step 2: Initialize UART5 peripheral
    // Configures GPIO pins (PC12=TX, PD2=RX) and UART5 registers
    // Baud rate: 9600, format: 8N1, no flow control
    init_uart();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 3: Initialize DMA for UART5 reception
    // Configures DMA1 Stream 0 Channel 4 for UART5_RX
    // Also enables UART5 and DMA interrupts in NVIC
    initialize_dma();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Step 4: Get reference to HM-11 driver instance
    // The driver uses dependency injection with STM32_UART_DMA port
    auto& driver = get_hm11_driver();

    // Initialization complete - indicate success
    indicate_success();

    // Step 5: Test module responsiveness with AT command
    // Sends "AT" command, expects "OK" response
    // This verifies basic UART communication and module responsiveness
    UartStatus status;
    driver.test(status);

    if (status != UartStatus::Ok) {
        // Test command failed - module not responding
        indicate_error();
    }

    // Test succeeded
    indicate_success();

    // Step 6: Retrieve software version from module
    // Sends "AT+VERS?" command, expects "OK+Get:<version>" response
    // This demonstrates information retrieval and response parsing
    std::string_view version;
    driver.software_version(version, status);

    if (status != UartStatus::Ok) {
        // Version query failed
        indicate_error();
    }

    // Version retrieved successfully
    // In a real application, you would display or log the version string
    // For this minimal example, we just verify it succeeded
    indicate_success();

    // All operations completed successfully
    // Turn on green LED to indicate success
    LED_GREEN_ON();

// In embedded system, enter idle loop
// In hosted environment, exit cleanly
#ifdef __ARM_ARCH
    // Embedded target - enter infinite loop
    while (true) {
        // Wait for interrupts or other events
        __WFI(); // Wait For Interrupt (ARM instruction)
    }
#else
    // Hosted target - return success
    return 0;
#endif
}
