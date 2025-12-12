/**
 * @file configuration_example.cpp
 * @brief Comprehensive example of HM11 BLE driver configuration operations
 *
 * This example demonstrates all the configuration operations of the HM11 BLE driver.
 * It shows how to initialize the driver, query current configurations, modify
 * settings, and handle errors properly. Each section focuses on a specific aspect
 * of the HM11 module's configuration capabilities.
 * 
 * The example is organized into several logical sections:
 * 1. Basic configuration and initialization
 * 2. Role and Work Mode configuration
 * 3. Advertising configuration
 * 4. Security configuration
 * 5. Power management
 * 6. Connection settings
 * 7. Device settings
 * 8. Module restart
 * 
 * This example follows several key design decisions:
 * - Decision 1: Dependency injection for the UART implementation
 * - Decision 2.1: Consistent error handling with visual feedback
 * - Decision 3.2: Clear user feedback for all operations
 * - Decision 4.1: Comprehensive configuration approach
 * - Decision 18: Linear, well-structured example with clear sections
 */

#include <chrono>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>

#include "include/hm11/hal/uart.hpp"
#include "include/hm11/hm11.hpp"
#include "include/hm11/util/fixed_string.hpp"
#include "include/hm11/util/ranged_integer.hpp"

// For testing, we use the mock UART implementation
#include "uart_mock.hpp"

using namespace std::chrono_literals;

// Constants
constexpr auto PERIOD = 200ms;
constexpr auto OPERATION_DELAY = 500ms;
constexpr int SUCCESS_SYMBOL_WIDTH = 12;

/**
 * @brief Prints a message to the console
 *
 * In a real implementation, this would update a display or log system.
 * This function implements the user feedback mechanism described in Decision 3.2.
 *
 * @param msg Message to print
 */
void print(const std::string& msg) {
    std::cout << msg << std::endl;
}

/**
 * @brief Handles errors by displaying a message and entering a failure state
 *
 * This function implements the error handling approach from Decision 2.1:
 * - Clear visual feedback (error message + simulated LED)
 * - Non-recoverable error state (infinite loop)
 * - Consistent error reporting pattern
 *
 * In a real implementation, this would toggle a red LED to indicate error
 * and potentially log the error to a persistent storage.
 *
 * @param msg Error message describing what failed
 */
[[noreturn]] void on_error(const std::string& msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    
    // Simulate LED toggling as in the Ada example
    // This implements the visual feedback approach from Decision 3.2
    while (true) {
        std::cout << "Error state - Red LED would toggle here" << std::endl;
        std::this_thread::sleep_for(PERIOD);
    }
}

/**
 * @brief Checks operation status and provides user feedback
 *
 * This function implements the error handling pattern from Decision 2.1:
 * - Check status after each operation
 * - Provide immediate feedback on success/failure
 * - Forward to error handler on failure
 *
 * In a real implementation, this would display status on an LCD or LED.
 *
 * @param msg Message prefix describing the operation
 * @param status Status to check
 */
void check_status(const std::string& msg, hm11::hal::UartStatus status) {
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << std::left << std::setw(SUCCESS_SYMBOL_WIDTH) << "✓ " + msg;
        std::cout << " [OK]" << std::endl;
    } else {
        on_error(msg + " failed");
    }
}

/**
 * @brief DMA receive handler for the HM11 driver
 *
 * This function mimics the Ada DMA_Receive_Handler but is simplified for the example.
 * It handles the UART receive operation and copies the data to the provided buffer.
 *
 * This implements Decision 19 for protected object translation by providing a
 * consistent interface for receiving data from the UART that matches the Ada pattern.
 *
 * @param port UART port to receive data from
 * @param received Pointer to buffer where received data will be stored
 * @param length Length of the buffer
 * @param status Status output parameter to report operation result
 * @param timeout_ms Timeout in milliseconds (default: 1000ms)
 * @param as_stream Whether this is a streaming receive (default: false)
 */
void dma_receive_handler(
    hm11::hal::AnyUartPort port,
    void* received,
    std::size_t length,
    hm11::hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms = 1000ms,
    [[maybe_unused]] bool as_stream = false) {
    
    // Create a buffer view to receive into
    // This approach matches Decision 20 regarding DMA buffer management
    std::vector<hm11::hal::UInt8> receive_buffer(length);
    hm11::hal::UartData8b data_view(receive_buffer);
    
    // Perform the receive operation
    port->receive(data_view, status, timeout_ms);
    
    // If successful, copy the data to the received buffer
    if (status == hm11::hal::UartStatus::Ok) {
        std::memcpy(received, data_view.data(), length);
    }
}

/**
 * @brief Last read position handler for the HM11 driver
 *
 * This function mimics the Ada Last_Read_Handler but is simplified for the example.
 * It's used to get the current read position in a stream and determine if the
 * stream is closed.
 *
 * This implements Decision 25 regarding timeout implementation by providing
 * a mechanism to track stream state that matches the Ada pattern.
 *
 * @param closed Output parameter set to true if the stream is closed
 * @param zero Output parameter set to the current read position
 */
void last_read_handler(bool& closed, std::size_t& zero) {
    // For this simple example, we always report the stream as closed
    // and the position as 1
    closed = true;
    zero = 1;
}

/**
 * @brief Demonstrates basic HM11 configuration operations
 * 
 * This section demonstrates the core functionality needed to initialize and
 * reset the HM11 module:
 * - Testing connectivity (AT command)
 * - Getting software version
 * - Getting MAC address
 * - Resetting to factory defaults
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_basic_configuration(auto& driver) {
    print("\n------ 1. Basic Configuration ------\n");
    
    // Test connectivity using simple AT command
    // This verifies the UART connection is working properly
    hm11::hal::UartStatus status;
    driver.test(status);
    check_status("Test", status);
    
    // Get software version to identify module capabilities
    // This demonstrates using the util::VersionType from Decision 2 (strong types)
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::util::VersionType version;
    driver.software_version(version, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Software version: " << version.c_str() << std::endl;
    } else {
        on_error("Get software version failed");
    }
    
    // Get MAC address for device identification
    // This demonstrates using the util::MacAddress type from Decision 2
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::util::MacAddress mac;
    driver.get_mac_address(mac, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ MAC Address: " << mac.c_str() << std::endl;
    } else {
        on_error("Get MAC address failed");
    }
    
    // Demonstrate reset (factory defaults)
    // This is useful when you need to start from a known state
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.reset(status);
    check_status("Reset", status);
}

/**
 * @brief Demonstrates role and work mode configuration
 * 
 * This section shows how to configure the fundamental operation modes of the HM11:
 * - Getting and setting the BLE role (Peripheral vs Central)
 * - Getting and setting the work type (how the module starts up)
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_role_and_work_mode(auto& driver) {
    print("\n------ 2. Role and Work Mode Configuration ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get current role to see if the module is in Peripheral or Central mode
    // This is critical to understand the current operation mode
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::Role currentRole;
    driver.get_role(currentRole, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current role: " 
                  << (currentRole == hm11::Role::Peripheral ? "Peripheral" : "Central")
                  << std::endl;
    } else {
        on_error("Get role failed");
    }
    
    // Set role to Central (can scan and connect to other devices)
    // This demonstrates changing the device's fundamental operation mode
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_role(hm11::Role::Central, status);
    check_status("Set role to Central", status);
    
    // Get role again to verify the change was successful
    // This demonstrates the pattern of setting and then verifying configuration
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.get_role(currentRole, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Updated role: " 
                  << (currentRole == hm11::Role::Peripheral ? "Peripheral" : "Central")
                  << std::endl;
    } else {
        on_error("Get role failed");
    }
    
    // Set role back to Peripheral (accepts connections)
    // Return to default mode to ensure other examples work correctly
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_role(hm11::Role::Peripheral, status);
    check_status("Set role to Peripheral", status);
    
    // Get current work type to see startup behavior
    // Work type controls whether the module starts normally or waits for commands
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::WorkType currentWorkType;
    driver.get_work_type(currentWorkType, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current work type: " 
                  << (currentWorkType == hm11::WorkType::StartImmediately ? 
                     "StartImmediately" : "RespondAtCommand")
                  << std::endl;
    } else {
        on_error("Get work type failed");
    }
    
    // Set work type to start immediately (no AT+MODE needed after power-up)
    // This is typically what you want for embedded applications
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_work_type(hm11::WorkType::StartImmediately, status);
    check_status("Set work type to StartImmediately", status);
}

/**
 * @brief Demonstrates advertising configuration
 * 
 * This section shows how to configure advertising parameters:
 * - Getting and setting advertising type (how devices can discover and connect)
 * - Getting and setting advertising interval (power/discovery tradeoff)
 * - Getting and setting reliable advertising mode (redundancy vs efficiency)
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_advertising_configuration(auto& driver) {
    print("\n------ 3. Advertising Configuration ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get current advertising type to understand how device is discoverable
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::AdvertisingType currentAdvType;
    driver.get_advertising_type(currentAdvType, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current advertising type: ";
        switch (currentAdvType) {
            case hm11::AdvertisingType::AdvertisingScanResponseConnectable:
                std::cout << "AdvertisingScanResponseConnectable";
                break;
            case hm11::AdvertisingType::LastDeviceConnect:
                std::cout << "LastDeviceConnect";
                break;
            case hm11::AdvertisingType::AdvertisingScanResponse:
                std::cout << "AdvertisingScanResponse";
                break;
            case hm11::AdvertisingType::Advertising:
                std::cout << "Advertising";
                break;
            default:
                std::cout << "Unknown";
        }
        std::cout << std::endl;
    } else {
        on_error("Get advertising type failed");
    }
    
    // Set advertising type to be both scannable and connectable
    // This is the most common mode that allows both discovery and connections
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_advertising_type(hm11::AdvertisingType::AdvertisingScanResponseConnectable, status);
    check_status("Set advertising type to AdvertisingScanResponseConnectable", status);
    
    // Get current advertising interval to see power/discovery tradeoff
    // Shorter intervals mean faster discovery but higher power consumption
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::AdvertisingInterval currentAdvInterval;
    driver.get_advertising_interval(currentAdvInterval, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current advertising interval: ";
        switch (currentAdvInterval) {
            case hm11::AdvertisingInterval::Ms100:  std::cout << "100ms"; break;
            case hm11::AdvertisingInterval::Ms211:  std::cout << "211ms"; break;
            case hm11::AdvertisingInterval::Ms252:  std::cout << "252ms"; break;
            case hm11::AdvertisingInterval::Ms318:  std::cout << "318ms"; break;
            case hm11::AdvertisingInterval::Ms417:  std::cout << "417ms"; break;
            case hm11::AdvertisingInterval::Ms546:  std::cout << "546ms"; break;
            case hm11::AdvertisingInterval::Ms760:  std::cout << "760ms"; break;
            case hm11::AdvertisingInterval::Ms852:  std::cout << "852ms"; break;
            case hm11::AdvertisingInterval::Ms1022: std::cout << "1022ms"; break;
            case hm11::AdvertisingInterval::Ms1285: std::cout << "1285ms"; break;
            case hm11::AdvertisingInterval::Ms2000: std::cout << "2000ms"; break;
            case hm11::AdvertisingInterval::Ms3000: std::cout << "3000ms"; break;
            case hm11::AdvertisingInterval::Ms4000: std::cout << "4000ms"; break;
            case hm11::AdvertisingInterval::Ms5000: std::cout << "5000ms"; break;
            case hm11::AdvertisingInterval::Ms6000: std::cout << "6000ms"; break;
            case hm11::AdvertisingInterval::Ms7000: std::cout << "7000ms"; break;
            default: std::cout << "Unknown";
        }
        std::cout << std::endl;
    } else {
        on_error("Get advertising interval failed");
    }
    
    // Set advertising interval to a moderate value
    // 1022ms provides a good balance of discovery speed and power consumption
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_advertising_interval(hm11::AdvertisingInterval::Ms1022, status);
    check_status("Set advertising interval to 1022ms", status);
    
    // Get current reliable advertising mode to see redundancy level
    // Reliable mode sends extra packets to increase chance of discovery
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::AdvertisingMode currentAdvMode;
    driver.get_reliable_advertising_mode(currentAdvMode, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current reliable advertising mode: " 
                  << (currentAdvMode == hm11::AdvertisingMode::NormalAdvertising ? 
                     "NormalAdvertising" : "ReliableAdvertising")
                  << std::endl;
    } else {
        on_error("Get reliable advertising mode failed");
    }
    
    // Set reliable advertising mode to normal (standard behavior)
    // This is suitable for most environments with good RF conditions
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_reliable_advertising_mode(hm11::AdvertisingMode::NormalAdvertising, status);
    check_status("Set reliable advertising mode to NormalAdvertising", status);
}

/**
 * @brief Demonstrates security configuration
 * 
 * This section shows how to get device identity information
 * for security purposes. In a full implementation, this would
 * include pairing and authentication settings.
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_security_configuration(auto& driver) {
    print("\n------ 4. Security Configuration ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get MAC address (for identification)
    // The MAC address is the unique identifier for the device
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::util::MacAddress mac;
    driver.get_mac_address(mac, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Module MAC address: " << mac.c_str() << std::endl;
    } else {
        on_error("Get MAC address failed");
    }
    
    // Note: Full security configuration would include:
    // - Authentication requirements
    // - Encryption settings
    // - Bonding/pairing modes
    // - PIN or passkey configuration
}

/**
 * @brief Demonstrates power management configuration
 * 
 * This section shows how to configure power-related settings:
 * - Getting and setting module power (TX power)
 * - Getting and setting auto-sleep mode
 * - Getting and setting output power level
 * - Battery monitoring configuration
 * - UART sleep settings
 * - Sleep/wake operations
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_power_management(auto& driver) {
    print("\n------ 5. Power Management ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get current module power to see transmission strength
    // Higher power increases range but consumes more energy
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::ModulePower currentPower;
    driver.get_module_power(currentPower, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current module power: ";
        switch (currentPower) {
            case hm11::ModulePower::DbmMinus23: std::cout << "-23 dBm"; break;
            case hm11::ModulePower::DbmMinus6:  std::cout << "-6 dBm"; break;
            case hm11::ModulePower::Dbm0:       std::cout << "0 dBm"; break;
            case hm11::ModulePower::Dbm6:       std::cout << "6 dBm"; break;
            default: std::cout << "Unknown";
        }
        std::cout << std::endl;
    } else {
        on_error("Get module power failed");
    }
    
    // Set module power to maximum for best range
    // This demonstrates setting TX power to 6 dBm (highest available)
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_module_power(hm11::ModulePower::Dbm6, status);
    check_status("Set module power to 6 dBm", status);
    
    // Get auto-sleep mode to see if power saving is enabled
    // Auto-sleep allows the module to enter low power mode when idle
    std::this_thread::sleep_for(OPERATION_DELAY);
    bool autoSleepEnabled;
    driver.get_module_auto_sleep(autoSleepEnabled, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Auto-sleep mode: " 
                  << (autoSleepEnabled ? "Enabled" : "Disabled")
                  << std::endl;
    } else {
        on_error("Get auto-sleep mode failed");
    }
    
    // Set auto-sleep mode to enable power saving
    // This is important for battery-powered applications
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_module_auto_sleep(true, status);
    check_status("Enable auto-sleep mode", status);
    
    // Get output power setting (normal vs maximum)
    // This is a higher-level setting than the specific dBm value
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::OutputPower outputPower;
    driver.get_output_power(outputPower, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Output power: " 
                  << (outputPower == hm11::OutputPower::NormalPower ? "Normal" : "Maximum")
                  << std::endl;
    } else {
        on_error("Get output power failed");
    }
    
    // Set output power to maximum for best range
    // This is a higher-level alternative to setting specific dBm
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_output_power(hm11::OutputPower::MaxPower, status);
    check_status("Set output power to maximum", status);
    
    // Get battery monitor switch to see if battery reporting is enabled
    // This allows the module to report battery level in advertisements
    std::this_thread::sleep_for(OPERATION_DELAY);
    bool batteryMonitorEnabled;
    driver.get_battery_monitor_switch(batteryMonitorEnabled, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Battery monitor: " 
                  << (batteryMonitorEnabled ? "Enabled" : "Disabled")
                  << std::endl;
    } else {
        on_error("Get battery monitor switch failed");
    }
    
    // Set battery monitor switch to enable battery level reporting
    // This is useful for device health monitoring
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_battery_monitor_switch(true, status);
    check_status("Enable battery monitor", status);
    
    // Set battery information to report current battery level
    // This demonstrates using the RangedInteger type from Decision 2
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_battery_information(hm11::util::Percent(80), status);
    check_status("Set battery level to 80%", status);
    
    // Query battery information to verify the setting
    // This shows how to retrieve the value we just set
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::util::Percent batteryLevel(0);
    driver.query_battery_information(batteryLevel, status);
    if (status == hm11::hal::UartStatus::Ok) {
        // Use get_value() to get the underlying value from the RangedInteger
        std::cout << "✓ Battery level: " << static_cast<int>(batteryLevel.get_value()) << "%" << std::endl;
    } else {
        on_error("Query battery information failed");
    }
    
    // Get UART sleep type to see if UART remains responsive during sleep
    // This determines if the module can be woken via UART
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::UartSleepType uartSleepType;
    driver.get_uart_sleep_type(uartSleepType, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ UART sleep type: " 
                  << (uartSleepType == hm11::UartSleepType::CanWakeUpThroughUart ? 
                     "Can wake up through UART" : "UART shutdown during sleep")
                  << std::endl;
    } else {
        on_error("Get UART sleep type failed");
    }
    
    // Set UART sleep type to allow wakeup via UART
    // This is important if you need to wake the module from the host MCU
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_uart_sleep_type(hm11::UartSleepType::CanWakeUpThroughUart, status);
    check_status("Set UART sleep type to wake up through UART", status);
    
    // Sleep and wake up demonstration
    // This shows how to manually control power states
    std::this_thread::sleep_for(OPERATION_DELAY);
    std::cout << "Putting module to sleep..." << std::endl;
    driver.sleep(status);
    check_status("Sleep", status);
    
    std::this_thread::sleep_for(1000ms);
    std::cout << "Waking up module..." << std::endl;
    driver.wake_up(status);
    check_status("Wake up", status);
}

/**
 * @brief Demonstrates connection settings configuration
 * 
 * This section shows how to configure connection-related parameters:
 * - Getting and setting update connection flag
 * - Getting and setting connection timeout
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_connection_settings(auto& driver) {
    print("\n------ 6. Connection Settings ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get update connection flag to see if parameters can be updated
    // This controls whether connection parameters can be renegotiated
    std::this_thread::sleep_for(OPERATION_DELAY);
    bool updateConnection;
    driver.get_update_connection(updateConnection, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Update connection flag: " 
                  << (updateConnection ? "Enabled" : "Disabled")
                  << std::endl;
    } else {
        on_error("Get update connection flag failed");
    }
    
    // Set update connection flag to enable parameter negotiation
    // This allows connection parameters to be changed during a connection
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_update_connection(true, status);
    check_status("Enable update connection", status);
    
    // Get connect timeout to see how long connection attempts can take
    // This determines how long the module will try to connect before giving up
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::util::ConnectTimeout connectTimeout("000000");
    driver.get_connect_remote_device_timeout(connectTimeout, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Connect timeout: " << connectTimeout.c_str() << " ms" << std::endl;
    } else {
        on_error("Get connect timeout failed");
    }
    
    // Set connect timeout to 10 seconds (10000ms)
    // A longer timeout improves connection reliability but delays error reporting
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_connect_remote_device_timeout(hm11::util::ConnectTimeout("010000"), status);
    check_status("Set connect timeout to 10000ms", status);
}

/**
 * @brief Demonstrates device settings configuration
 * 
 * This section shows how to configure device-specific settings:
 * - Getting and setting module name
 * - Getting and setting UART parameters (baud rate, flow control, parity, stop bits)
 * - Getting and setting RX gain
 * - Getting and setting service UUID and characteristic
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_device_settings(auto& driver) {
    print("\n------ 7. Device Settings ------\n");
    
    hm11::hal::UartStatus status;
    
    // Get current module name to see how it appears to scanning devices
    // This is the name that will appear in BLE scanner apps
    std::this_thread::sleep_for(OPERATION_DELAY);
    std::string currentName = driver.get_module_name(status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current module name: " << currentName << std::endl;
    } else {
        on_error("Get module name failed");
    }
    
    // Set module name to a custom value
    // This is typically one of the first configurations you want to set
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_module_name("HM11Example", status);
    check_status("Set module name to HM11Example", status);
    
    // Get current UART baud rate to see communication speed
    // This must match your host MCU's UART configuration
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::UartBaudRate currentBaudRate;
    driver.get_uart_baud_rate(currentBaudRate, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Current UART baud rate: ";
        switch (currentBaudRate) {
            case hm11::UartBaudRate::Br9600:   std::cout << "9600"; break;
            case hm11::UartBaudRate::Br19200:  std::cout << "19200"; break;
            case hm11::UartBaudRate::Br38400:  std::cout << "38400"; break;
            case hm11::UartBaudRate::Br57600:  std::cout << "57600"; break;
            case hm11::UartBaudRate::Br115200: std::cout << "115200"; break;
            case hm11::UartBaudRate::Br4800:   std::cout << "4800"; break;
            case hm11::UartBaudRate::Br2400:   std::cout << "2400"; break;
            case hm11::UartBaudRate::Br1200:   std::cout << "1200"; break;
            case hm11::UartBaudRate::Br230400: std::cout << "230400"; break;
            default: std::cout << "Unknown";
        }
        std::cout << std::endl;
    } else {
        on_error("Get UART baud rate failed");
    }
    
    // Set UART baud rate to a higher speed
    // 115200 provides good balance of speed and reliability
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_uart_baud_rate(hm11::UartBaudRate::Br115200, status);
    check_status("Set UART baud rate to 115200", status);
    
    // Get UART flow control setting
    // Flow control prevents buffer overruns on high-speed connections
    std::this_thread::sleep_for(OPERATION_DELAY);
    bool flowControlEnabled;
    driver.get_uart_flow_control_switch(flowControlEnabled, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ UART flow control: " 
                  << (flowControlEnabled ? "Enabled" : "Disabled")
                  << std::endl;
    } else {
        on_error("Get UART flow control failed");
    }
    
    // Set UART flow control to enable
    // This requires additional pins but improves reliability
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_uart_flow_control_switch(true, status);
    check_status("Enable UART flow control", status);
    
    // Get parity bit setting
    // Parity provides basic error detection for UART communication
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::ParityBit parityBit;
    driver.get_parity_bit(parityBit, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Parity bit: ";
        switch (parityBit) {
            case hm11::ParityBit::None: std::cout << "None"; break;
            case hm11::ParityBit::Odd:  std::cout << "Odd"; break;
            case hm11::ParityBit::Even: std::cout << "Even"; break;
            default: std::cout << "Unknown";
        }
        std::cout << std::endl;
    } else {
        on_error("Get parity bit failed");
    }
    
    // Set parity bit to even
    // Even parity is a common choice for error detection
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_parity_bit(hm11::ParityBit::Even, status);
    check_status("Set parity bit to Even", status);
    
    // Get stop bit configuration
    // Stop bits mark the end of a UART data frame
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::StopBit stopBit;
    driver.get_stop_bit(stopBit, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Stop bit: " 
                  << (stopBit == hm11::StopBit::OneStopBit ? "One stop bit" : "Two stop bits")
                  << std::endl;
    } else {
        on_error("Get stop bit failed");
    }
    
    // Set stop bit to one (most common configuration)
    // One stop bit is standard for most UART configurations
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_stop_bit(hm11::StopBit::OneStopBit, status);
    check_status("Set stop bit to OneStopBit", status);
    
    // Get RX gain setting (receiver sensitivity)
    // Higher gain improves reception range but may increase noise
    std::this_thread::sleep_for(OPERATION_DELAY);
    hm11::RxGain rxGain;
    driver.get_module_rx_gain(rxGain, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ RX gain: " 
                  << (rxGain == hm11::RxGain::No ? "Disabled" : "Enabled")
                  << std::endl;
    } else {
        on_error("Get RX gain failed");
    }
    
    // Set RX gain to enabled for better reception
    // This improves sensitivity at the cost of potential noise
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_module_rx_gain(hm11::RxGain::Open, status);
    check_status("Enable RX gain", status);
    
    // Get service UUID for GATT configuration
    // This defines the primary service offered by the device
    std::this_thread::sleep_for(OPERATION_DELAY);
    std::string serviceUuid;
    driver.get_service_uuid(serviceUuid, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Service UUID: " << serviceUuid << std::endl;
    } else {
        on_error("Get service UUID failed");
    }
    
    // Set service UUID to standard UART service
    // FFE0 is a common UUID for UART-over-BLE services
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_service_uuid("FFE0", status);
    check_status("Set service UUID to FFE0", status);
    
    // Get characteristic UUID for GATT configuration
    // This defines the specific data point within the service
    std::this_thread::sleep_for(OPERATION_DELAY);
    std::string characteristic;
    driver.get_characteristic(characteristic, status);
    if (status == hm11::hal::UartStatus::Ok) {
        std::cout << "✓ Characteristic: " << characteristic << std::endl;
    } else {
        on_error("Get characteristic failed");
    }
    
    // Set characteristic UUID to standard UART characteristic
    // FFE1 is a common UUID for UART-over-BLE characteristics
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.set_characteristic("FFE1", status);
    check_status("Set characteristic to FFE1", status);
}

/**
 * @brief Demonstrate restarting the HM-11 module
 * 
 * This section shows how to restart the module to apply configuration changes.
 * A restart is typically required after changing multiple settings.
 * 
 * @param driver Reference to the HM11 driver
 */
void demonstrate_restart(auto& driver) {
    print("\n------ 8. Restart ------\n");
    
    hm11::hal::UartStatus status;
    
    // Restart the module to apply all configuration changes
    // This is required after changing most settings
    std::this_thread::sleep_for(OPERATION_DELAY);
    driver.restart(status);
    check_status("Restart", status);
    
    // Allow time for the module to fully restart
    std::this_thread::sleep_for(2s);
    print("Module has been restarted successfully");
}

/**
 * @brief Main function implementing the HM11 configuration example
 *
 * This function demonstrates the comprehensive configuration capabilities
 * of the HM11 driver. It follows the structure specified in Decision 18:
 * - Linear single-file structure with clear sections
 * - Proper error handling with visual feedback
 * - Comprehensive validation of demonstrated features
 *
 * @return int Exit code (0 for success, non-zero for failure)
 */
int main() {
    try {
        // Set up a more predictable console output
        std::cout << std::fixed;
        std::cout.precision(1);
        
        // Simulate initialization delay (like in the Ada example)
        std::this_thread::sleep_for(2s);
        print("Starting HM-11 Configuration Example");
        print("====================================");
        
        // Create a mock UART for testing
        // This demonstrates dependency injection as described in Decision 1
        auto uart = hm11::tests::mocks::create_uart_mock();
        auto uart_mock = std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart);
        
        // Create the HM11 driver instance with the mock UART
        // This demonstrates the template-based design from Decision 1
        hm11::HM11Driver<hm11::tests::mocks::UARTMock> driver(
            *std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart), 
            dma_receive_handler, 
            last_read_handler);
        
        // Execute all configuration demonstrations
        // Each function focuses on a specific aspect of configuration
        demonstrate_basic_configuration(driver);
        demonstrate_role_and_work_mode(driver);
        demonstrate_advertising_configuration(driver);
        demonstrate_security_configuration(driver);
        demonstrate_power_management(driver);
        demonstrate_connection_settings(driver);
        demonstrate_device_settings(driver);
        demonstrate_restart(driver);
        
        // All operations completed successfully
        print("\n------ All Configuration Operations Completed Successfully ------");
        
        // In the Ada example, this would toggle the green LED
        // Here we simulate the visual feedback (Decision 3.2)
        while (true) {
            std::cout << "Success - Green LED would toggle here" << std::endl;
            std::this_thread::sleep_for(PERIOD);
        }
    }
    catch (const std::exception& e) {
        // Handle any unexpected exceptions
        // This implements the error handling approach from Decision 2.1
        on_error(std::string("Exception: ") + e.what());
    }
    
    return 0;
}