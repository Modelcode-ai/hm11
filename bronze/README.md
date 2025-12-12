# HM11 Bluetooth LE Driver

A modern C++ driver for the HM-11/CC2541 Bluetooth Low Energy module. This driver is designed to be efficient, typesafe, and suitable for resource-constrained embedded systems.

## Overview

The HM11 driver provides a complete interface for interacting with HM-11 (CC2541-based) Bluetooth Low Energy modules. It features a modular design with a focus on:

- **Memory Safety**: Strong typing and bounds checking
- **ISR Safety**: Interrupt-safe synchronization mechanisms
- **Efficiency**: Minimal overhead and memory footprint
- **Compatibility**: Direct port of the original Ada driver API
- **Flexibility**: Template-based design for dependency injection

This implementation was carefully migrated from the original Ada codebase, preserving the same functionality while leveraging modern C++ (C++20) features.

## Features

- Complete HM-11 AT command support
- Template-based dependency injection for UART implementation
- Strong typing with compile-time validation
- Memory-safe buffer handling
- ISR-safe synchronization primitives
- Support for peripheral and central modes
- Full support for iBeacon functionality
- Comprehensive testing and error handling
- Cross-platform support (Linux/macOS and STM32 targets)

## Quick Start

### Prerequisites

- Clang 20+ or GCC 14+ compiler with C++20 support
- CMake 3.20+
- For embedded targets: ARM GCC Toolchain

### Building the Driver

```bash
# Configure for debug build with Clang
cmake --preset clang-debug

# Build in parallel
cmake --build --preset clang-debug --parallel

# Run tests
ctest --preset clang-debug
```

For detailed build options, see [Build System Documentation](docs/build_system.md).

### Basic Usage

```cpp
#include <hm11/hm11.hpp>
#include <hm11/hal/uart.hpp>
#include <chrono>

// Create UART implementation
auto uart = create_uart(...);  // Your UART implementation

// Define handlers for receive operations and position tracking
void dma_receive_handler(...) { /* implementation */ }
void last_read_handler(...) { /* implementation */ }

// Create HM11 driver instance
hm11::HM11Driver<YourUARTType> driver(
    uart, 
    dma_receive_handler, 
    last_read_handler
);

// Test connection
hm11::hal::UartStatus status;
driver.test(status);
if (status != hm11::hal::UartStatus::Ok) {
    // Handle error
}

// Configure device
driver.set_role(hm11::Role::Peripheral, status);
driver.set_module_name("MyDevice", status);

// Set advertising interval (417ms)
driver.set_advertising_interval(hm11::AdvertisingInterval::Ms417, status);
```

See the [examples directory](examples/) for complete working examples.

## Peripheral Mode Example

```cpp
// Set peripheral mode
driver.set_role(hm11::Role::Peripheral, status);

// Configure device
driver.set_module_name("HM11Device", status);
driver.set_advertising_interval(hm11::AdvertisingInterval::Ms417, status);
driver.set_module_power(hm11::ModulePower::Dbm0, status);

// Start working
driver.set_work_type(hm11::WorkType::StartImmediately, status);
driver.restart(status);

// Wait for connection
// When connected, the module will send "OK+CONN"
// When disconnected, the module will send "OK+LOST"
```

## Central Mode Example

```cpp
// Set central mode
driver.set_role(hm11::Role::Central, status);

// Set scan parameters
driver.set_work_type(hm11::WorkType::RespondATCommand, status);

// Define callback for discovered devices
auto callback = [](char id, const hm11::util::MacAddress& mac, 
                   std::string_view name, std::string_view rssi) {
    // Process discovered device
    std::cout << "Found device: " << name << " [" << mac.to_string() 
              << "] RSSI: " << rssi << std::endl;
};

// Scan for devices (5 seconds timeout)
driver.scan(callback, 5000, status);

// Connect to device by index
hm11::ConnectResult result;
driver.connect(0, result, status);  // Connect to the first device found

// Or connect by MAC address
hm11::util::MacAddress target_mac("123456789ABC");
driver.connect(hm11::MACAddressType::NormalAddress, target_mac, result, status);
```

## Memory Safety Features

The driver implements various safety features to ensure memory safety:

- **RangedInteger**: Compile-time and runtime range checking for integer values
- **FixedString**: Validated fixed-size strings with character constraints
- **DMA alignment**: Utilities for proper DMA buffer alignment
- **Safe buffer handling**: No raw pointers, protection against buffer overflows
- **RAII principles**: Resource management with proper ownership semantics

## ISR Safety

The driver is designed to be used in interrupt-driven environments:

- All functions explicitly document their ISR safety
- BinarySemaphore for ISR-to-thread signaling
- Watchdog pattern for timeout handling
- Lock-free data structures for concurrency

## Architecture

The driver is structured into several modules:

- **Core Driver**: Main HM11Driver template class
- **HAL**: Hardware Abstraction Layer for UART and platform-specific operations
- **Utilities**: Type-safe wrappers, parsing utilities, string handling
- **Synchronization**: ISR-safe primitives for thread synchronization
- **Platform**: Platform-specific implementations

## License

Copyright (C) 2025, AdaCore

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the license conditions are met. See the source code for the complete license details.
