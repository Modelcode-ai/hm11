# HM-11 Driver Examples

This directory contains example applications demonstrating the HM-11 Bluetooth driver features. Each example showcases specific driver capabilities and provides learning resources for users integrating the driver into their applications.

## Overview

The examples serve dual purposes:
1. **Learning Resources**: Progressive tutorials from basic initialization to advanced features
2. **Integration Tests**: Hardware validation ensuring driver functionality on real STM32F429 hardware

## Example Applications

### 01_basic_init - Basic Initialization
**What it demonstrates:**
- Minimal setup required to initialize the HM-11 driver
- Hardware initialization sequence (UART + DMA)
- Basic AT command execution (Test, Software_Version)
- Error handling with visual feedback

**Expected behavior:**
- Green LED blinks: Initialization successful
- Green LED blinks: Test command succeeded
- Green LED blinks: Software version retrieved
- Green LED stays on: All operations completed

**Time to complete:** ~4 seconds

### 02_peripheral_echo - Peripheral Echo Server
**What it demonstrates:**
- Complete BLE peripheral application
- Full peripheral configuration (18 AT commands)
- Connection lifecycle handling
- Bidirectional data transfer
- Connection/disconnection detection

**Expected behavior:**
1. Configuration phase: 10 seconds
2. Wait for connection: Green LED blinks
3. Connected: Green LED stays on, echoes received data
4. Disconnected: Green LED blinks rapidly

**Connection instructions:**
1. Install BLE serial terminal app on smartphone
2. Scan for "HMTest" device
3. Connect with PIN: 000000
4. Send text data (up to 7 characters)
5. Module echoes back received data

**Time to complete:** ~10 seconds configuration + waiting for connection

### 03_central_scan - Central Scanner
**What it demonstrates:**
- Central role configuration (scanner/initiator)
- Device discovery with streaming response parsing
- Callback-based device notification
- RSSI-based proximity filtering
- Handling variable-length device names

**Expected behavior:**
1. Configuration: ~5 seconds
2. Scanning: 10 seconds, discovers nearby BLE devices
3. For each device: displays MAC, name, RSSI
4. Applies RSSI threshold filter (-70 dBm)
5. Green LED indicates completion

**Time to complete:** ~15 seconds

### 04_gatt_client - GATT Client Operations
**What it demonstrates:**
- Central role with connection establishment
- GATT service/characteristic discovery
- Reading characteristic values
- Writing characteristic values
- Notification subscription
- Connection teardown

**Expected behavior:**
1. Configuration: ~5 seconds
2. Connect to target peripheral
3. Discover services and characteristics
4. Read/write operations with validation
5. Subscribe to notifications
6. Disconnect gracefully

**Time to complete:** ~30 seconds (depends on GATT operations)

### 05_ibeacon - iBeacon Configuration
**What it demonstrates:**
- iBeacon mode configuration
- UUID, Major, Minor value setting
- Measured power configuration
- Advertising as iBeacon
- iBeacon visibility to mobile apps

**Expected behavior:**
1. Configuration: ~8 seconds
2. Module advertises as iBeacon
3. Visible to iBeacon scanner apps
4. Green LED indicates successful configuration

**Time to complete:** ~10 seconds

### 06_hmsensor - HMSensor Features
**What it demonstrates:**
- HMSensor-specific features (temperature/humidity)
- Sensor type configuration
- Sensor reading retrieval
- Work interval configuration
- Sensor data parsing

**Expected behavior:**
1. Configuration: ~5 seconds
2. Read temperature and humidity values
3. Display parsed numeric values
4. Configure sensor work interval
5. Green LED indicates success

**Time to complete:** ~8 seconds

## Shared Infrastructure

All examples use common infrastructure from `examples/common/`:

### led_control.hpp
LED abstraction macros for platform independence:
```cpp
INIT_LEDS()              // Initialize LED hardware
LED_GREEN_ON()           // Turn on green LED
LED_GREEN_OFF()          // Turn off green LED
LED_GREEN_TOGGLE()       // Toggle green LED
LED_RED_ON()             // Turn on red LED
LED_RED_OFF()            // Turn off red LED
LED_RED_TOGGLE()         // Toggle red LED
```

Helper functions:
```cpp
indicate_success()       // Blink green LED 3 times
indicate_error()         // Enter infinite red LED blinking loop
```

### error_handling.hpp
Consistent error handling patterns:
```cpp
CHECK_STATUS(status)     // Check UartStatus, enter error loop if not Ok
check_status(status)     // Function version of CHECK_STATUS
on_error()               // Terminal error handler (never returns)
```

### initialization.hpp
Common initialization sequences:
```cpp
init_hardware()                              // UART + DMA + LED initialization (4s)
verify_module_ready(driver)                  // Test + MAC verification (1s)
configure_as_peripheral(driver, name, pin)   // Full peripheral config (10s)
configure_as_central(driver)                 // Central role config (1.5s)
```

## Hardware Requirements

### STM32F429 Discovery Board
- **MCU**: STM32F429ZIT6 (Cortex-M4, 180MHz)
- **Flash**: 2MB
- **RAM**: 256KB
- **LEDs**: Green and Red LEDs for status feedback

### HM-11 Bluetooth Module
- **UART Connection**:
  - PC12 (UART5_TX) -> HM-11 RX (pin 4)
  - PD2 (UART5_RX) -> HM-11 TX (pin 2)
  - 3.3V -> HM-11 VCC (pin 1)
  - GND -> HM-11 GND (pin 3)
- **Baud Rate**: 9600 (default)
- **Format**: 8N1, no flow control

### Power Supply
- USB power for STM32F429 board
- 3.3V regulated supply for HM-11 module (from board)

## Building Examples

### Prerequisites

**For Embedded Target (STM32F429):**
- ARM GCC toolchain (arm-none-eabi-gcc)
- CMake 3.20 or later
- STM32 CMSIS headers
- OpenOCD or ST-Link tools for flashing

**For Testing (Native Target):**
- Clang 20+ or GCC 14+
- CMake 3.20 or later
- Google Test
- Examples are tested via test versions in `tests/examples/`

### Build for Embedded Target

```bash
# Configure for STM32F429
cmake --preset stm32f429-debug

# Build all examples
cmake --build --preset stm32f429-debug

# Build specific example
cmake --build --preset stm32f429-debug --target 01_basic_init

# Example outputs (in build directory):
# - examples/01_basic_init/01_basic_init.elf  (ELF with debug symbols)
# - examples/01_basic_init/01_basic_init.bin  (Raw binary for flashing)
# - examples/01_basic_init/01_basic_init.hex  (Intel HEX format)
```

### Build for Testing (Native Target)

```bash
# Configure for native target
cmake --preset default

# Build all tests (includes example tests)
cmake --build --preset default

# Run example tests
ctest --preset default -R examples

# Run specific example test
ctest --preset default -R test_01_basic_init
```

## Flashing to Hardware

### Using OpenOCD (Recommended)

```bash
# Flash example binary
openocd -f board/stm32f429discovery.cfg \
    -c "program examples/01_basic_init/01_basic_init.elf verify reset exit"
```

### Using ST-Link Utility

```bash
# Flash with ST-Link command line
st-flash write examples/01_basic_init/01_basic_init.bin 0x08000000
```

### Using STM32CubeProgrammer

1. Open STM32CubeProgrammer
2. Connect to ST-Link
3. Load .hex or .bin file
4. Set start address: 0x08000000 (for .bin)
5. Click "Start Programming"

## Debugging

### OpenOCD + GDB

```bash
# Terminal 1: Start OpenOCD
openocd -f board/stm32f429discovery.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb examples/01_basic_init/01_basic_init.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

### Serial Output (Optional)

If you add printf debugging (redirected to UART):
```bash
# Monitor serial output (adjust /dev/ttyUSBx)
screen /dev/ttyUSB0 115200
```

## Troubleshooting

### Build Issues

**Problem**: `CMSIS headers not found`
```bash
# Solution: Install CMSIS or set CMSIS_PATH
cmake --preset stm32f429-debug -DCMSIS_PATH=/path/to/CMSIS_5
```

**Problem**: `arm-none-eabi-gcc not found`
```bash
# Solution: Install ARM GCC toolchain
# Ubuntu: sudo apt-get install gcc-arm-none-eabi
# Or download from ARM website
```

**Problem**: `Linker script not found`
```bash
# Solution: Ensure preset defines LINKER_SCRIPT
# Check CMakePresets.json for stm32f429 preset
```

### Hardware Issues

**Problem**: Module not responding (Red LED blinking)
- Check UART connections (TX/RX crossed)
- Verify 3.3V power supply to HM-11
- Ensure ground connection between STM32 and HM-11
- Try power-cycling both boards

**Problem**: Cannot flash to STM32
- Verify ST-Link connection
- Check USB cable (data cable, not power-only)
- Update ST-Link firmware
- Try mass erase: `st-flash erase`

**Problem**: Example runs but no BLE activity
- Verify HM-11 is powered (check with multimeter)
- Test HM-11 separately with USB-to-serial adapter
- Check baud rate (default: 9600)
- Verify AT command mode (send "AT", expect "OK")

## Testing Strategy

Examples are validated through two approaches:

### 1. Hardware Integration Tests
- Build for STM32F429 embedded target
- Flash to hardware and run on real HM-11 module
- Manual verification of LED patterns and BLE behavior
- Used for final validation before release

### 2. Automated Test Versions
- Built in `tests/examples/` for native target
- Use EnhancedMockUART to simulate HM-11 responses
- MockLED captures LED state changes
- Run automatically in CI/CD pipeline
- Validate example logic without hardware

Run automated tests:
```bash
cmake --preset default
cmake --build --preset default
ctest --preset default -R examples --output-on-failure
```

## Example Progression

Recommended learning order:

1. **01_basic_init**: Start here - understand initialization sequence
2. **02_peripheral_echo**: Learn peripheral role and data transfer
3. **03_central_scan**: Understand central role and discovery
4. **04_gatt_client**: Learn GATT operations (most complex)
5. **05_ibeacon**: Understand iBeacon configuration
6. **06_hmsensor**: Learn sensor-specific features

## Code Style

All examples follow these conventions:
- C++20 standard
- Clear comments documenting each phase
- Ada source references where applicable
- Timing matching Ada implementation
- Error handling with CHECK_STATUS
- LED feedback for visual status

## Contributing

When adding new examples:
1. Follow existing structure (main.cpp + CMakeLists.txt)
2. Include comprehensive header documentation
3. Use common infrastructure (led_control.hpp, error_handling.hpp)
4. Add corresponding test in tests/examples/
5. Update this README with example description
6. Document hardware requirements and expected behavior

## License

Copyright (c) 2025. Licensed under BSD-3-Clause.

## References

- **Ada Source**: `/l2l/src/hm11/examples/shared/hm11/src/hm11_example.adb`
- **Driver Documentation**: `../CMAKE_USAGE.md`, `../TESTING.md`
- **Migration Spec**: Root directory modernization specifications
- **STM32F429 Reference**: ST RM0090 Reference Manual
- **HM-11 Datasheet**: JNHuaMao HM-11 Bluetooth 4.0 Module
