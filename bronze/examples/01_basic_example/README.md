# HM11 Basic Example

This example demonstrates the basic functionality of the HM11 BLE driver. It implements a simple "echo" server that can be connected from a phone, similar to the original Ada example in `hm11_example.adb` but implemented in C++.

## Overview

The example demonstrates the following key operations:

1. Initializing the HM11 driver with a UART implementation
2. Testing connectivity with the HM11 module
3. Getting the module's MAC address
4. Configuring various settings (advertising type, module name, work type, role)
5. Setting the module to Peripheral role
6. Waiting for connections
7. Echoing back received data
8. Handling disconnection

## Mock Implementation

This example uses a mock UART implementation for testing purposes, allowing it to run without actual hardware. The mock:

- Provides predefined responses to AT commands
- Simulates connection and disconnection events
- Tracks transmitted data to verify echo functionality

In a real-world scenario, you would replace the mock with an actual hardware-connected UART implementation.

## Building and Running the Example

The example can be built using CMake. For a debug build:

```bash
# Create a build directory and generate build files
cmake --preset clang-debug

# Build the example
cmake --build --preset clang-debug --target hm11_basic_example
```

For a release build:

```bash
# Create a build directory and generate build files
cmake --preset clang-release

# Build the example
cmake --build --preset clang-release --target hm11_basic_example
```

Once built, the example can be run directly:

```bash
# Navigate to the build directory
cd build/clang-debug

# Run the example
./examples/01_basic_example/hm11_basic_example
```

## Expected Output

When running the example, you should see output similar to:

```
Starting initialization
Initialized UART and driver
Test OK
MAC_Address: 123456789ABC
Set_Advertising_Type OK
Set_Bond_Mode (skipped - not in API)
Set_Notify_Information (skipped - not in API)
Set_Module_Name OK
Set_PIN_Code (skipped - not in API)
Set_Work_Type OK
Set_Role OK
Restart OK
Waiting connection
Connected
Disconnected
Done
Success - Green LED would toggle here
```

## Connecting with a Real Device

In a real deployment with actual hardware, you would:

1. Connect the HM11 module to your microcontroller:
   - Connect HM11 RX pin to UART TX pin (PC12 on STM32F429)
   - Connect HM11 TX pin to UART RX pin (PD2 on STM32F429)
   - Connect power and ground pins

2. Install a BLE terminal app on your phone (like "BLE Terminal" or "nRF Connect")

3. Use the app to scan for BLE devices and connect to "HMTest" (the name set in this example)

4. Once connected, any text you send will be echoed back by the HM11 module

## Key Components

- **basic_example.cpp**: Main example implementation demonstrating HM11 driver usage
- **uart_mock.hpp**: Mock UART implementation for testing without real hardware
- **hm11_driver_mock.cpp**: Support file for the mock implementation

## Design Decisions Implemented

The example implements several key design decisions from the modernization specification:

- **Decision 1 (Dependency Injection)**: The UART implementation is injected into the HM11Driver via templating
- **Decision 2.1 (Error Handling)**: Consistent status checking and visual feedback for errors
- **Decision 3.2 (User Feedback)**: Visual indicators for operations and status (LED simulation)
- **Decision 18 (Example Structure)**: Linear single-file structure with clear sections
- **Decision 20 (DMA Buffer Management)**: Proper buffer handling in dma_receive_handler
- **Decision 21 (Testing Strategy)**: Mock-based testing that simulates hardware responses

## Notes

- The example uses a simplified error handling approach that matches the original Ada implementation
- Some functions from the original Ada example are not available in the current API and are skipped
- In real hardware, you would replace the simulated LED toggling with actual LED control
- The mock UART provides a good starting point for understanding how to interface with real hardware