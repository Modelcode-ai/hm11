# HM11 STM32F429 Peripheral Echo Example
# Demonstrates complete STM32 hardware driver with HM11 BLE module
# Equivalent to Ada's hm11_example.adb peripheral echo functionality

# Only build this example for embedded STM32 targets
if(NOT IS_EMBEDDED_TARGET)
    message(STATUS "STM32 Peripheral Echo example skipped - requires embedded target")
    return()
endif()

# Verify we have the required STM32 configuration
if(NOT DEFINED STM32_DEVICE OR NOT STM32_DEVICE MATCHES "STM32F4")
    message(STATUS "STM32 Peripheral Echo example skipped - requires STM32F4xx target")
    return()
endif()

# Create the executable
add_executable(stm32_peripheral_echo main.cpp)

# Link against HM11 libraries
target_link_libraries(stm32_peripheral_echo
    PRIVATE
        hm11::stm32_hal    # STM32-specific UART implementation
        hm11::driver       # HM11 driver templates
        hm11::sync         # Synchronization primitives
        hm11::types        # Type system
)

# Set C++20 standard
target_compile_features(stm32_peripheral_echo PRIVATE cxx_std_20)

# Add optimization flags for embedded target
target_compile_options(stm32_peripheral_echo PRIVATE
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os -g1>
)

# Create additional build outputs for embedded development
if(CMAKE_OBJCOPY)
    # Generate Intel HEX file for programming
    add_custom_command(TARGET stm32_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32_peripheral_echo> 
                ${CMAKE_BINARY_DIR}/stm32_peripheral_echo.hex
        COMMENT "Generating Intel HEX file"
    )
    
    # Generate binary file
    add_custom_command(TARGET stm32_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32_peripheral_echo> 
                ${CMAKE_BINARY_DIR}/stm32_peripheral_echo.bin
        COMMENT "Generating binary file"
    )
    
    # Show memory usage
    add_custom_command(TARGET stm32_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32_peripheral_echo>
        COMMENT "Memory usage:"
    )
endif()

# Set properties for IDE support
set_target_properties(stm32_peripheral_echo PROPERTIES
    DESCRIPTION "HM11 STM32F429 Peripheral Echo Example - Complete hardware driver demonstration"
    FOLDER "Examples/STM32"
)