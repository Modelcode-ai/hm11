/**
 * @file peripheral_echo.cpp
 * @brief Peripheral echo server example for the HM11 BLE driver
 *
 * This example demonstrates how to configure an HM-11 module as a peripheral device,
 * establish a connection, and echo back data received from a central device (like a mobile phone).
 * It directly corresponds to the Ada example in hm11_example.adb.
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
constexpr std::string_view WHEN_CONNECTED_MESSAGE = "OK+CONN";
constexpr std::string_view WHEN_DISCONNECTED_MESSAGE = "OK+LOST";

/**
 * @brief Prints a message to the console
 *
 * @param msg Message to print
 */
void print(const std::string& msg) {
    std::cout << msg << std::endl;
}

/**
 * @brief Handles errors
 *
 * @param msg Error message
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
 * @brief Checks the status and prints a message
 *
 * @param msg Message prefix
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
 *
 * @param port UART port
 * @param received Received data buffer
 * @param length Length of the buffer
 * @param status Status output parameter
 * @param timeout_ms Timeout in milliseconds
 * @param as_stream Whether this is a streaming receive
 */
void dma_receive_handler(
    hm11::hal::AnyUartPort port,
    void* received,
    std::size_t length,
    hm11::hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms = 1000ms,
    [[maybe_unused]] bool as_stream = false) {
    
    // Create a buffer view to receive into
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
 * It's used to get the current read position in a stream.
 *
 * @param closed Whether the stream is closed
 * @param zero Current read position
 */
void last_read_handler(bool& closed, std::size_t& zero) {
    // For this simple example, we always report the stream as closed
    // and the position as 1
    closed = true;
    zero = 1;
}

/**
 * @brief Main function
 *
 * @return int Exit code
 */
int main() {
    // Simulate initialization delay (like in the Ada example)
    std::this_thread::sleep_for(2s);
    print("Starting initialization");
    
    // Create a mock UART for testing
    auto uart = hm11::tests::mocks::create_uart_mock();
    
    // Set up the expected responses for the UART mock
    auto uart_mock = std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart);
    
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
    uart_mock->queue_receive_data(std::string(WHEN_CONNECTED_MESSAGE));
    
    // Create the HM11 driver instance
    // We need to explicitly specify the template parameter as UARTMock
    hm11::HM11Driver<hm11::tests::mocks::UARTMock> driver(
        *std::dynamic_pointer_cast<hm11::tests::mocks::UARTMock>(uart), 
        dma_receive_handler, 
        last_read_handler);
    
    print("Initialized UART and driver");
    
    // Test connectivity
    hm11::hal::UartStatus status;
    driver.test(status);
    check_status("Test", status);
    
    // Get MAC address
    hm11::util::MacAddress mac;
    driver.get_mac_address(mac, status);
    if (status != hm11::hal::UartStatus::Ok) {
        on_error("Get_MAC_Address failed");
    }
    print("MAC_Address: " + std::string(mac.c_str()));
    
    // Configure the HM11 module - setup as peripheral echo server
    std::this_thread::sleep_for(500ms); // Delay between operations
    driver.set_advertising_type(hm11::AdvertisingType::AdvertisingScanResponseConnectable, status);
    check_status("Set_Advertising_Type", status);
    
    std::this_thread::sleep_for(500ms);
    driver.set_bond_mode(hm11::BondMode::AuthWithPin, status);
    check_status("Set_Bond_Mode", status);
    
    std::this_thread::sleep_for(500ms);
    driver.set_notify_information(true, status);
    check_status("Set_Notify_Information", status);
    
    std::this_thread::sleep_for(500ms);
    driver.set_module_name("HMTest", status);
    check_status("Set_Module_Name", status);
    
    std::this_thread::sleep_for(500ms);
    // Set PIN code - using PinType from util/fixed_string.hpp
    hm11::util::PinType pin("000000");
    driver.set_pin_code(pin, status);
    check_status("Set_PIN_Code", status);
    
    std::this_thread::sleep_for(500ms);
    driver.set_work_type(hm11::WorkType::StartImmediately, status);
    check_status("Set_Work_Type", status);
    
    std::this_thread::sleep_for(500ms);
    driver.set_role(hm11::Role::Peripheral, status);
    check_status("Set_Role", status);
    
    std::this_thread::sleep_for(1s);
    driver.restart(status);
    check_status("Restart", status);
    
    std::this_thread::sleep_for(2s);
    print("Waiting connection");
    
    // Wait for connection from a central device (like a phone)
    std::array<char, 20> connection_message{};
    bool connected = false;
    
    while (!connected) {
        std::fill(connection_message.begin(), connection_message.end(), 0);
        dma_receive_handler(
            uart, 
            connection_message.data(), 
            connection_message.size(),
            status,
            120s);
            
        // Check for the connection message "OK+CONN"
        if (std::strncmp(connection_message.data(), WHEN_CONNECTED_MESSAGE.data(), 
                          WHEN_CONNECTED_MESSAGE.length()) == 0) {
            print("Connected");
            connected = true;
        } else {
            check_status("Connect loop", status);
        }
    }
    
    // Waiting for data or disconnect
    // Echo back any data received
    std::array<char, 20> message_buffer{};
    bool disconnected = false;
    
    print("Starting echo server - waiting for data to echo");
    
    // Queue some example data to be echoed back in the mock
    uart_mock->queue_receive_data("Hello");
    uart_mock->queue_receive_data("World");
    
    // Finally, queue the disconnect message
    uart_mock->queue_receive_data(std::string(WHEN_DISCONNECTED_MESSAGE));
    
    while (!disconnected) {
        std::fill(message_buffer.begin(), message_buffer.end(), 0);
        dma_receive_handler(
            uart, 
            message_buffer.data(), 
            message_buffer.size(),
            status,
            120s);
            
        // Check for the disconnection message "OK+LOST"
        if (std::strncmp(message_buffer.data(), WHEN_DISCONNECTED_MESSAGE.data(), 
                         WHEN_DISCONNECTED_MESSAGE.length()) == 0) {
            print("Disconnected");
            disconnected = true;
        } else {
            // Echo back the received data
            print("Received data: " + std::string(message_buffer.data()));
            
            // Prepare data for transmission
            std::vector<hm11::hal::UInt8> data(std::strlen(message_buffer.data()));
            std::copy(message_buffer.begin(), 
                    message_buffer.begin() + std::strlen(message_buffer.data()), 
                    data.begin());
            hm11::hal::UartData8b data_view(data);
            
            // Transmit the data
            uart->transmit(data_view, status);
            
            print("Echoed back: " + std::string(message_buffer.data()));
            check_status("Transmit loop", status);
        }
    }
    
    // All is OK
    print("Done");
    
    // In the Ada example, this would toggle the green LED
    // Here we'll just print a message
    while (true) {
        std::cout << "Success - LED would toggle here" << std::endl;
        std::this_thread::sleep_for(PERIOD);
    }
    
    return 0;
}