# Examples directory CMakeLists.txt
# Contains example applications demonstrating HM11 driver features

message(STATUS "Configuring HM11 examples")

# Create examples output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# Basic type system example (from earlier tasks)
# Demonstrates RangedInteger, FixedString, and Ada type mapping  
add_subdirectory(basic)

# Basic driver initialization and testing example (Task 6)
# Demonstrates essential HM11 driver functionality equivalent to Ada example core
add_subdirectory(01_basic_init)

# STM32 Peripheral Echo example (advanced hardware integration)
# Demonstrates complete STM32 hardware driver with HM11 BLE module
# Equivalent to Ada's hm11_example.adb peripheral echo functionality
add_subdirectory(02_stm32_peripheral_echo)

# Central Device Discovery example (Task 3)
# Demonstrates central mode device discovery and streaming response parsing
# Showcases discovery state machine and callback infrastructure
add_subdirectory(03_central_scan)

# GATT Client example (Task 5)
# Demonstrates discovery-to-connection workflow with device indexing
# Showcases Connect by index, device storage, and disconnect functionality
add_subdirectory(04_gatt_client)

# iBeacon Discovery example (Task 4)
# Demonstrates iBeacon discovery with specialized parsing
# Showcases AT+DISI? command and 66-byte iBeacon data extraction
add_subdirectory(05_ibeacon)

# HMSensor Integration example (Task 4)
# Demonstrates sensor type configuration, work interval management,
# and temperature/humidity reading with numeric parsing
add_subdirectory(06_hmsensor)

# Platform-specific configuration
if(IS_EMBEDDED_TARGET)
    message(STATUS "Examples configured for embedded target:")
    message(STATUS "  - Examples will be built for hardware validation")
    message(STATUS "  - No integration tests will be run (hardware required)")
    message(STATUS "  - Binary outputs: ${CMAKE_BINARY_DIR}/examples/")
else()
    message(STATUS "Examples configured for native target:")
    message(STATUS "  - Examples can be executed for validation")
    message(STATUS "  - Integration tests will verify functionality")
    message(STATUS "  - Use 'ctest --label-regex example' to run example tests")
endif()

# Example execution instructions
if(NOT IS_EMBEDDED_TARGET)
    message(STATUS "")
    message(STATUS "To run examples:")
    message(STATUS "  cmake --build --preset <preset-name>")
    message(STATUS "  ./build/examples/basic_example")
    message(STATUS "  # Or run via test: ctest -R basic_example_test")
endif()