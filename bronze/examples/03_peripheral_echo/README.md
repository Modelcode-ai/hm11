# HM11 Peripheral Echo Server Example

This example demonstrates how to configure an HM-11 Bluetooth Low Energy (BLE) module as a peripheral device, establish a connection with a central device (like a smartphone), and echo back data received from that central device.

## Overview

The example implements the following functionality:

1. Initializes the HM-11 module with the UART driver
2. Configures the module as a peripheral device with the following settings:
   - Advertising type: Scannable and connectable
   - Bond mode: Authentication with PIN
   - Notifications enabled for connection events
   - Module name set to "HMTest"
   - PIN code set to "000000"
   - Work type set to start immediately
   - Role set to peripheral
3. Waits for a connection from a central device (like a smartphone)
4. Once connected, it echoes back any data received
5. Detects and handles disconnection events

This example directly corresponds to the original Ada example in `hm11_example.adb` but is implemented in C++.

## Mock Implementation

For testing and demonstration purposes, this example uses a mock implementation of the UART driver. The mock simulates the behavior of an actual HM-11 module by:

1. Providing expected responses to AT commands
2. Simulating connection and disconnection events
3. Handling data transmission and reception

## Running the Example

To build and run this example:

```bash
# Navigate to the build directory
cd build

# Build the example
cmake --build . --target hm11_peripheral_echo

# Run the example
./examples/03_peripheral_echo/hm11_peripheral_echo
```

## Expected Output

When running the example, you should see output similar to this:

```
Starting initialization
Initialized UART and driver
Test OK
MAC_Address: 123456789ABC
Set_Advertising_Type OK
Set_Bond_Mode OK
Set_Notify_Information OK
Set_Module_Name OK
Set_PIN_Code OK
Set_Work_Type OK
Set_Role OK
Restart OK
Waiting connection
Connected
Starting echo server - waiting for data to echo
Received data: Hello
Echoed back: Hello
Transmit loop OK
Received data: World
Echoed back: World
Transmit loop OK
Disconnected
Done
Success - LED would toggle here
Success - LED would toggle here
...
```

## Connecting to the Example with a Real Device

On a real hardware implementation (not the mock), you would follow these steps:

1. Run the example on your hardware with the HM-11 module connected
2. Use a BLE scanner app on your smartphone to find the "HMTest" device
3. Connect to the device (PIN code: 000000)
4. Use a BLE terminal app to send data to the device
5. Observe that any data sent is echoed back by the device

## Key Components

- **peripheral_echo.cpp**: Main example implementation
- **uart_mock.hpp**: Mock implementation of the UART interface
- **hm11_driver_mock.cpp**: Mock implementation of the HM11 driver

## Notes

- In a real hardware implementation, LED indicators would be used to show status (e.g., red LED for errors, green LED for success)
- The example uses console output to show status information instead of physical LEDs
- The mock simulates a connection and sends predefined test data before simulating a disconnection