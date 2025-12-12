# HM11 Configuration Example

This example demonstrates the comprehensive configuration capabilities of the HM11 BLE driver. It provides a systematic walkthrough of all available configuration options, showing how to query current settings, modify them, and handle errors properly.

## Overview

The example is organized into several logical sections, each focusing on a specific aspect of the HM11 module's configuration:

1. **Basic Configuration**:
   - Testing connectivity
   - Getting software version
   - Getting MAC address
   - Resetting to factory defaults

2. **Role and Work Mode Configuration**:
   - Getting and setting BLE role (Peripheral/Central)
   - Getting and setting work type (startup behavior)

3. **Advertising Configuration**:
   - Getting and setting advertising type
   - Getting and setting advertising interval
   - Getting and setting reliable advertising mode

4. **Security Configuration**:
   - Getting device identity information

5. **Power Management**:
   - Getting and setting module power (TX power)
   - Getting and setting auto-sleep mode
   - Getting and setting output power level
   - Battery monitoring configuration
   - UART sleep settings
   - Sleep/wake operations

6. **Connection Settings**:
   - Getting and setting update connection flag
   - Getting and setting connection timeout

7. **Device Settings**:
   - Getting and setting module name
   - Getting and setting UART parameters (baud rate, flow control, parity, stop bits)
   - Getting and setting RX gain
   - Getting and setting service UUID and characteristic

8. **Module Restart**:
   - Properly restarting the module after configuration

## Mock Implementation

Like the basic example, this example uses a mock UART implementation to allow testing without actual hardware. The mock:

- Returns appropriate responses for each configuration command
- Simulates the behavior of a real HM11 module
- Provides default "OK" responses when no specific response is queued

In a real-world implementation, you would replace the mock with a real UART connected to an HM11 module, but the API usage pattern would remain the same.

## Building and Running the Example

The example can be built using CMake. For a debug build:

```bash
# Create a build directory and generate build files
cmake --preset clang-debug

# Build the example
cmake --build --preset clang-debug --target hm11_configuration_example
```

For a release build:

```bash
# Create a build directory and generate build files
cmake --preset clang-release

# Build the example
cmake --build --preset clang-release --target hm11_configuration_example
```

Once built, the example can be run directly:

```bash
# Navigate to the build directory
cd build/clang-debug

# Run the example
./examples/02_configuration_example/hm11_configuration_example
```

## Expected Output

When running the example, you'll see a comprehensive output showing all the configuration operations performed. Each operation is marked with a check mark (✓) when successful. Here's a sample of the output:

```
Starting HM-11 Configuration Example
====================================

------ 1. Basic Configuration ------

✓ Test [OK]
✓ Software version: V600
✓ MAC Address: 123456789ABC
✓ Reset [OK]

------ 2. Role and Work Mode Configuration ------

✓ Current role: Peripheral
✓ Set role to Central [OK]
✓ Updated role: Central
✓ Set role to Peripheral [OK]
✓ Current work type: StartImmediately
✓ Set work type to StartImmediately [OK]

...

------ 8. Restart ------

✓ Restart [OK]
Module has been restarted successfully

------ All Configuration Operations Completed Successfully ------
Success - Green LED would toggle here
```

## Real Hardware Integration

When using this example with real hardware:

1. Replace the mock UART with your hardware UART implementation:
   ```cpp
   // Instead of mock UART:
   auto uart = my_hardware::create_uart(UART_PORT, BAUD_RATE);
   
   hm11::HM11Driver<my_hardware::UARTImpl> driver(
       *uart, dma_receive_handler, last_read_handler);
   ```

2. Implement the appropriate DMA receive handler for your hardware

3. Configure hardware-specific timeouts and delays based on your module's behavior

## Key Components

- **configuration_example.cpp**: Main example implementation demonstrating all configuration operations
- **uart_mock.hpp**: Mock UART implementation for testing without real hardware
- **hm11_driver_mock.cpp**: Support file for the mock implementation

## Design Decisions Implemented

The example implements several key design decisions from the modernization specification:

- **Decision 1 (Dependency Injection)**: The UART implementation is injected into the HM11Driver via templating
- **Decision 2 (Strong Type System)**: Uses strongly-typed parameters for configuration values
- **Decision 2.1 (Error Handling)**: Consistent status checking and visual feedback for errors
- **Decision 3.2 (User Feedback)**: Visual indicators for operations and status
- **Decision 4.1 (Configuration Approach)**: Demonstrates comprehensive configuration capabilities
- **Decision 18 (Example Structure)**: Organized into logical sections for clarity
- **Decision 21 (Testing Strategy)**: Mock-based testing that simulates hardware responses

## Notes

- This example is designed to be comprehensive, showing all configuration operations
- Some operations may be hardware-specific; consult your HM11 module documentation
- In a real application, you typically only need to configure the specific settings relevant to your use case
- The example shows how to properly check status after each operation, which is essential for reliable operation