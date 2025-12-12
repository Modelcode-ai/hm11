#include "hm11/hm11_example.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "hm11/driver.hpp"
#include "hm11/stm32/uart_port.hpp"
#include "hm11/tests/utils/mock_uart_port.hpp"

using namespace hm11;

// Simple wrapper for delays (seconds)
static void delay(double seconds) {
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}

void run_hm11_example(HM11Driver& driver) {
    // Simulated initialization (mirrors Ada example).
    std::cout << "Initializing UART..." << std::endl;
    // In real hardware Drivers.Init_UART and Drivers.Initialize_DMA would be called.
    delay(1.0);
    std::cout << "Initialized" << std::endl;

    // Example configuration: set role to peripheral (0).
    if (driver.set_role(0)) {
        std::cout << "Set role (Peripheral) OK" << std::endl;
    } else {
        std::cout << "Failed to set role" << std::endl;
    }

    // Echo loop: read from stdin and echo back (simulates UART echo).
    std::cout << "Enter text to echo (empty line to quit):" << std::endl;
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line.empty())
            break;
        // In a real system you would transmit via driver.transmit(...).
        std::cout << "Echo: " << line << std::endl;
    }
    std::cout << "Done" << std::endl;
}

int main() {
    // Create a mock UART port for demonstration purposes.
    MockUARTPort mock_uart;
    HM11Driver driver(mock_uart);

    // Simulated initialization sequence (mirrors Ada example).
    std::cout << "Initializing UART..." << std::endl;
    // In real hardware, Drivers.Init_UART and Drivers.Initialize_DMA would be called.
    // Here we assume UART is ready.
    delay(1.0);

    std::cout << "Initialized" << std::endl;

    // Example configuration steps (role, bonding, etc.) using driver methods.
    if (driver.set_role(0)) {
        std::cout << "Set role (Peripheral) OK" << std::endl;
    }
    // Additional configuration calls can be added similarly using driver API.

    // Echo server loop (simplified): read line from stdin and send back.
    std::cout << "Enter text to echo (empty line to quit):" << std::endl;
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line.empty())
            break;
        // In real driver we would transmit over UART; here we just echo.
        std::cout << "Echo: " << line << std::endl;
    }

    std::cout << "Done" << std::endl;
    return 0;
}
