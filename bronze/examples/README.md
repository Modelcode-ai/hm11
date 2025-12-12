# HM11 Driver Examples

This directory contains a collection of examples demonstrating the usage of the HM11 Bluetooth Low Energy driver. These examples are designed to progressively introduce the features and capabilities of the HM11 driver, from basic initialization to complex communication patterns.

## Overview

The examples are organized in order of increasing complexity, each focusing on a specific aspect of the driver's functionality:

1. **Basic Example (01_basic_example)**: Demonstrates core initialization, basic commands, and simple communication with the HM11 module.
2. **Configuration Example (02_configuration_example)**: Shows comprehensive configuration options and settings for the HM11 module.
3. **Peripheral Echo Example (03_peripheral_echo)**: Implements a complete echo server as a peripheral device, matching the functionality of the original Ada example.

Additional examples and utilities:
- **alignment_usage_example.cpp**: Demonstrates proper usage of memory alignment utilities for DMA operations.

## Example Organization

### 01_basic_example

The basic example demonstrates the fundamental usage of the HM11 driver:

- Initializing the driver with a UART implementation
- Testing connectivity with the HM11 module
- Getting the module's MAC address
- Basic configuration (advertising type, module name, work type, role)
- Setting up peripheral mode
- Waiting for connection events
- Simple data exchange (echo functionality)

This example is the best starting point for understanding the driver's core functionality.

### 02_configuration_example

The configuration example provides a comprehensive overview of all available configuration options:

- Basic initialization and testing
- Role and work mode configuration
- Advertising settings
- Security configuration
- Power management options
- Connection parameters
- Device settings (name, UART parameters, etc.)
- Module restart procedures

Use this example when you need to understand how to configure specific aspects of the HM11 module.

### 03_peripheral_echo

The peripheral echo example implements a complete peripheral device that:

- Configures the module as a peripheral with specific settings
- Sets up proper security and connection parameters
- Waits for a connection from a central device
- Receives data from the connected device
- Echoes back any received data
- Handles disconnection events

This example directly corresponds to the original Ada example in `hm11_example.adb` and demonstrates a complete application using the HM11 driver.

### Alignment Usage Example

The alignment usage example (`alignment_usage_example.cpp`) demonstrates:

- Proper DMA buffer alignment for efficient and safe memory transfers
- Usage of the platform alignment utilities
- Cache handling for DMA operations
- Fixed-size and dynamic DMA-aligned buffer usage

This specialized example is useful when working with DMA operations in embedded systems.

## Building and Running the Examples

All examples can be built using the CMake build system. For convenience, CMake presets are provided for various build configurations.

### Using CMake Presets (Recommended)

```bash
# Configure for debug build with Clang
cmake --preset clang-debug

# Build all examples in parallel
cmake --build --preset clang-debug --parallel

# Alternatively, build a specific example
cmake --build --preset clang-debug --target hm11_basic_example
cmake --build --preset clang-debug --target hm11_configuration_example
cmake --build --preset clang-debug --target hm11_peripheral_echo
```

### Running the Examples

After building, the examples can be found in the build directory:

```bash
# Navigate to the build directory
cd build/clang-debug

# Run the basic example
./examples/01_basic_example/hm11_basic_example

# Run the configuration example
./examples/02_configuration_example/hm11_configuration_example

# Run the peripheral echo example
./examples/03_peripheral_echo/hm11_peripheral_echo
```

## Mock Implementation vs. Real Hardware

All examples include a mock UART implementation that simulates the behavior of an actual HM11 module. This allows the examples to be run without physical hardware for demonstration and testing purposes.

### Mock Implementation

The mock implementation:
- Provides predefined responses to AT commands
- Simulates connection and disconnection events
- Tracks transmitted data to verify functionality
- Allows testing without actual hardware

### Real Hardware Integration

To use these examples with real hardware:

1. Replace the mock UART implementation with your hardware-specific UART driver:
   ```cpp
   // Instead of mock UART:
   // auto uart = hm11::tests::mocks::create_uart_mock();
   
   // Use your hardware UART implementation:
   auto uart = my_hardware::create_uart(UART_PORT, BAUD_RATE);
   
   hm11::HM11Driver<my_hardware::UARTImpl> driver(
       *uart, dma_receive_handler, last_read_handler);
   ```

2. Implement the appropriate DMA receive handler and last read handler for your hardware

3. Connect the HM11 module to your microcontroller:
   - Connect HM11 RX pin to UART TX pin
   - Connect HM11 TX pin to UART RX pin
   - Connect power and ground pins
   
4. To test the peripheral example with a real device:
   - Install a BLE terminal app on your phone (like "BLE Terminal" or "nRF Connect")
   - Use the app to scan for and connect to "HMTest" (the name set in the examples)
   - Send data via the app to test the echo functionality

## Key Design Decisions Implemented

The examples implement several key design decisions from the modernization specification:

### Decision 1: Dependency Injection Mechanism

All examples demonstrate template-based dependency injection where the UART implementation is passed to the HM11Driver as a template parameter:

```cpp
hm11::HM11Driver<UARTImplementation> driver(
    uart_instance, dma_receive_handler, last_read_handler);
```

This allows for easy substitution of different UART implementations without changing the driver code.

### Decision 2: Strong Type System Translation

The examples use strongly-typed parameters for configuration values:

```cpp
hm11::util::MacAddress mac;                         // Fixed-length string with validation
hm11::util::PinType pin("000000");                  // Fixed-length PIN with validation
driver.set_role(hm11::Role::Peripheral, status);    // Type-safe enum
```

### Decision 2.1: Error Handling

All examples implement consistent status checking and error handling:

```cpp
hm11::hal::UartStatus status;
driver.operation(param, status);
if (status != hm11::hal::UartStatus::Ok) {
    // Handle error with clear feedback
}
```

### Decision 3.2: User Feedback

The examples provide clear visual feedback for operations (simulated LEDs in the examples):

```cpp
// Success indication
std::cout << "Success - Green LED would toggle here" << std::endl;

// Error indication
std::cout << "Error - Red LED would toggle here" << std::endl;
```

### Decision 4.1: Configuration Approach

The configuration example demonstrates comprehensive configuration of all module parameters, with individual getter/setter methods for each parameter.

### Decision 18: Example Application Structure

Each example follows a linear single-file structure with clear sections, proper error handling with visual feedback, and comprehensive validation of the demonstrated features.

### Decision 20: DMA Buffer Management

The examples demonstrate proper DMA buffer alignment and management:

```cpp
std::vector<hm11::hal::UInt8> receive_buffer(length);
hm11::hal::UartData8b data_view(receive_buffer);
port->receive(data_view, status, timeout_ms);
```

### Decision 21: Testing Strategy

The mock-based testing approach allows for development and testing without hardware dependence.

## Future Examples

Planned future examples include:

1. **Central Scanner Example**: Demonstrates device discovery and scanning functionality
2. **GATT Client Example**: Shows how to connect to a peripheral and interact with its GATT services
3. **iBeacon Configuration Example**: Demonstrates how to configure an HM11 module as an iBeacon
4. **HMSensor Integration Example**: Shows how to work with HM11 modules that include sensor functionality
5. **Low Power Example**: Demonstrates power management features of the HM11 module
