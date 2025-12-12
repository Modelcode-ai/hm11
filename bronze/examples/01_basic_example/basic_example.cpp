/**
 * @file basic_example.cpp
 * @brief Basic example application for the HM11 BLE driver
 *
 * This example demonstrates the basic functionality of the HM11 BLE driver.
 * It is similar to the original Ada example in hm11_example.adb but implemented
 * in C++. It shows how to initialize the driver, configure basic settings,
 * and perform simple operations.
 *
 * The example implements a simple "echo" server that can be connected from a phone.
 * This example demonstrates:
 * 1. Driver initialization with a UART mock implementation
 * 2. Basic module configuration (advertising type, module name, work type, role)
 * 3. Status checking and error handling (following Decision 2.1)
 * 4. Connection state management
 * 5. Data echo functionality (bidirectional communication)
 * 6. Visual feedback for operations (following Decision 3.2)
 * 
 * For a real-world implementation, replace the mock UART with an actual
 * hardware-connected UART implementation. The pattern of initialization,
 * configuration, and operation would remain the same.
 */

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "include/hm11/hal/uart.hpp"
#include "include/hm11/hm11.hpp"
#include "include/hm11/util/fixed_string.hpp"

// For testing, we use the mock UART implementation
#include "uart_mock.hpp"

using namespace std::chrono_literals;

// Constants matching the original Ada example
constexpr auto PERIOD = 200ms;
constexpr auto OPERATION_DELAY = 500ms;
constexpr auto CONNECTION_TIMEOUT = 120s;

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
    print(msg);
    
    // In the original Ada example, this would toggle the red LED
    // Here we just print an error message and loop forever
    while (true) {
        std::cout << "Error: " << msg << " - LED would toggle here" << std::endl;
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
        print(msg + " OK");
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
 * @brief Main function implementing the HM11 basic example
 *
 * This function follows the structure specified in Decision 18:
 * - Linear single-file structure with clear sections
 * - Proper error handling with visual feedback
 * - Comprehensive validation of demonstrated features
 *
 * @return int Exit code (0 for success, non-zero for failure)
 */
int main() {
    try {
        // ------------------------------------------------------------
        // Initialization Phase
        // ------------------------------------------------------------
        
        // Simulate initialization delay (like in the Ada example)
        std::this_thread::sleep_for(2s);
        print("Starting initialization");
        
        // Create a mock UART for testing
        // This demonstrates dependency injection as described in Decision 1
        auto uart = hm11::tests::mocks::create_uart_mock();
        auto uart_mock = std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart);
        
        // Set up the expected responses for the UART mock
        // In a real implementation, these would be actual responses from hardware
        // Add test response for AT command
        uart_mock->queue_receive_data("OK");
        
        // Add response for MAC address query
        uart_mock->queue_receive_data("123456789ABC");
        
        // Add responses for other commands
        uart_mock->queue_receive_data("OK"); // Set_Advertising_Type
        uart_mock->queue_receive_data("OK"); // Set_Bond_Mode
        uart_mock->queue_receive_data("OK"); // Set_Notify_Information
        uart_mock->queue_receive_data("OK"); // Set_Module_Name
        uart_mock->queue_receive_data("OK"); // Set_PIN_Code
        uart_mock->queue_receive_data("OK"); // Set_Work_Type
        uart_mock->queue_receive_data("OK"); // Set_Role
        uart_mock->queue_receive_data("OK"); // Restart
        
        // Simulate "When_Connected_Message" after waiting
        uart_mock->queue_receive_data("OK+CONN");
        
        // Create the HM11 driver instance
        // This demonstrates the template-based design from Decision 1
        hm11::HM11Driver<hm11::tests::mocks::UARTMock> driver(
            *std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart), 
            dma_receive_handler, 
            last_read_handler);
        
        print("Initialized UART and driver");
        
        // ------------------------------------------------------------
        // Basic Configuration Phase
        // ------------------------------------------------------------
        
        // Test connectivity
        hm11::hal::UartStatus status;
        driver.test(status);
        check_status("Test", status);
        
        // Get MAC address using the strong type system from Decision 2
        hm11::util::MacAddress mac;
        driver.get_mac_address(mac, status);
        if (status != hm11::hal::UartStatus::Ok) {
            on_error("Get_MAC_Address failed");
        }
        print("MAC_Address: " + std::string(mac.c_str()));
        
        // Configure the HM11 module - setting advertising type
        std::this_thread::sleep_for(OPERATION_DELAY); 
        driver.set_advertising_type(hm11::AdvertisingType::AdvertisingScanResponseConnectable, status);
        check_status("Set_Advertising_Type", status);
        
        // Note: set_bond_mode is not in the available API
        // We'll skip this step for the example
        std::this_thread::sleep_for(OPERATION_DELAY);
        // driver.set_bond_mode(hm11::BondMode::AuthWithPin, status);
        // check_status("Set_Bond_Mode", status);
        print("Set_Bond_Mode (skipped - not in API)");
        
        std::this_thread::sleep_for(OPERATION_DELAY);
        // Note: set_notify_information is not in the available API
        // We'll skip this step for the example
        print("Set_Notify_Information (skipped - not in API)");
        
        // Set module name
        std::this_thread::sleep_for(OPERATION_DELAY);
        driver.set_module_name("HMTest", status);
        check_status("Set_Module_Name", status);
        
        std::this_thread::sleep_for(OPERATION_DELAY);
        // Set PIN code - using PinType from util/fixed_string.hpp (Decision 2)
        hm11::util::PinType pin("000000");
        // Note: set_pin_code is not in the available API
        // We'll skip this step for the example
        print("Set_PIN_Code (skipped - not in API)");
        
        // Set work type to start immediately (matches Ada example)
        std::this_thread::sleep_for(OPERATION_DELAY);
        driver.set_work_type(hm11::WorkType::StartImmediately, status);
        check_status("Set_Work_Type", status);
        
        // Set role to Peripheral (acts as a server for connections)
        std::this_thread::sleep_for(OPERATION_DELAY);
        driver.set_role(hm11::Role::Peripheral, status);
        check_status("Set_Role", status);
        
        // Restart the module to apply settings
        std::this_thread::sleep_for(1s);
        driver.restart(status);
        check_status("Restart", status);
        
        // ------------------------------------------------------------
        // Connection Handling Phase
        // ------------------------------------------------------------
        
        std::this_thread::sleep_for(2s);
        print("Waiting connection");
        
        // Simulate waiting for connection (similar to the Ada example)
        // This demonstrates the connection handling approach from the original Ada code
        std::array<char, 20> connection_message{};
        bool connected = false;
        
        while (!connected) {
            std::fill(connection_message.begin(), connection_message.end(), 0);
            dma_receive_handler(
                uart, 
                connection_message.data(), 
                connection_message.size(),
                status,
                CONNECTION_TIMEOUT);
                
            // Check for the connection message "OK+CONN"
            // This message format matches the hardware protocol
            if (std::strncmp(connection_message.data(), "OK+CONN", 7) == 0) {
                print("Connected");
                connected = true;
            } else {
                check_status("Connect loop", status);
            }
        }
        
        // ------------------------------------------------------------
        // Data Echo Phase
        // ------------------------------------------------------------
        
        // Simulate waiting for data or disconnect
        // In the original Ada example, this would echo back any data received
        // Here we simulate it with a disconnection after a delay
        
        std::this_thread::sleep_for(5s);
        
        // Simulate receiving disconnect message
        uart_mock->queue_receive_data("OK+LOST");
        
        std::array<char, 20> disconnect_message{};
        bool disconnected = false;
        
        while (!disconnected) {
            std::fill(disconnect_message.begin(), disconnect_message.end(), 0);
            dma_receive_handler(
                uart, 
                disconnect_message.data(), 
                disconnect_message.size(),
                status,
                CONNECTION_TIMEOUT);
                
            // Check for the disconnection message "OK+LOST"
            if (std::strncmp(disconnect_message.data(), "OK+LOST", 7) == 0) {
                print("Disconnected");
                disconnected = true;
            } else {
                // In a real implementation, we would echo back the received data
                // This demonstrates the data echo functionality
                print("Received data: " + std::string(disconnect_message.data()));
                
                // Echo back the received data
                // This implements the echo server functionality from the Ada example
                std::vector<hm11::hal::UInt8> data(std::strlen(disconnect_message.data()));
                std::copy(disconnect_message.begin(), 
                         disconnect_message.begin() + std::strlen(disconnect_message.data()), 
                         data.begin());
                hm11::hal::UartData8b data_view(data);
                uart->transmit(data_view, status);
                
                check_status("Transmit loop", status);
            }
        }
        
        // ------------------------------------------------------------
        // Success Indication Phase
        // ------------------------------------------------------------
        
        // All operations completed successfully
        print("Done");
        
        // In the Ada example, this would toggle the green LED
        // Here we just print a message to simulate the visual feedback
        // This implements the user feedback approach from Decision 3.2
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