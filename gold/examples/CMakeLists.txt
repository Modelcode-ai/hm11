# ==============================================================================
# Top-level CMakeLists.txt for HM-11 Driver Examples
# ==============================================================================
#
# This file manages the build configuration for all HM-11 driver examples.
# Examples demonstrate various driver features and serve as integration tests
# for hardware validation.
#
# ## Build Target Behavior
#
# **Embedded Target (STM32F429):**
# - Builds all examples as standalone executables (.elf, .bin, .hex)
# - Links against hm11_platform (real STM32 UART/DMA/IRQ handlers)
# - LED macros control hardware GPIO pins
# - Examples are flashed to hardware and run on bare metal
#
# **Native Target (x86 Linux):**
# - Examples are NOT built (no hardware available)
# - Test versions are built in tests/examples/ with mocked hardware
# - Test versions use EnhancedMockUART and MockLED
# - Tests validate example logic without real hardware
#
# ## Examples Overview
#
# 1. **01_basic_init**: Minimal setup, initialize driver, basic AT commands
# 2. **02_peripheral_echo**: Complete peripheral application, echo server
# 3. **03_central_scan**: Central role, device discovery with callbacks
# 4. **04_gatt_client**: GATT client operations, read/write characteristics
# 5. **05_ibeacon**: iBeacon configuration and advertising
# 6. **06_hmsensor**: HMSensor-specific features (temperature/humidity)
#
# ## Shared Infrastructure
#
# All examples use common infrastructure from examples/common/:
# - **led_control.hpp**: LED abstraction macros (hardware or mock)
# - **error_handling.hpp**: CHECK_STATUS macro and error handlers
# - **initialization.hpp**: Common init sequences (init_hardware, verify_module_ready)
#
# ## Usage
#
# Build for embedded target:
# ```bash
# cmake --preset stm32f429-debug
# cmake --build --preset stm32f429-debug
# # Flash any example: examples/XX_name/XX_name.bin
# ```
#
# Build and run tests (includes example tests):
# ```bash
# cmake --preset default
# cmake --build --preset default
# ctest --preset default -R examples
# ```
#
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

# Examples are only built for embedded targets
# Native targets use test versions in tests/examples/
if(NOT IS_EMBEDDED_TARGET)
    message(STATUS "Skipping examples directory (native target uses test versions)")
    return()
endif()

message(STATUS "Building examples for embedded target")

# Ensure required variables are set (should be set by root CMakeLists.txt)
if(NOT DEFINED CMAKE_OBJCOPY)
    message(FATAL_ERROR "CMAKE_OBJCOPY not defined - required for binary generation")
endif()

if(NOT DEFINED CMAKE_SIZE)
    message(FATAL_ERROR "CMAKE_SIZE not defined - required for size reporting")
endif()

# ==============================================================================
# Example Subdirectories
# ==============================================================================
#
# Note: Each example uses target_include_directories() to explicitly
# specify its include paths. This provides better build isolation and
# follows modern CMake best practices.

# Each subdirectory contains a complete example with its own CMakeLists.txt
# Examples are built conditionally based on IS_EMBEDDED_TARGET

add_subdirectory(01_basic_init)
add_subdirectory(02_peripheral_echo)
add_subdirectory(03_central_scan)
add_subdirectory(04_gatt_client)
add_subdirectory(05_ibeacon)
add_subdirectory(06_hmsensor)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Example targets configured:")
message(STATUS "  - 01_basic_init: Basic initialization and AT commands")
message(STATUS "  - 02_peripheral_echo: Peripheral echo server")
message(STATUS "  - 03_central_scan: Central role device discovery")
message(STATUS "  - 04_gatt_client: GATT client operations")
message(STATUS "  - 05_ibeacon: iBeacon configuration")
message(STATUS "  - 06_hmsensor: HMSensor temperature/humidity")
message(STATUS "")
message(STATUS "After building, flash examples with:")
message(STATUS "  examples/<name>/<name>.bin -> STM32F429")
