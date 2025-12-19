# ==============================================================================
# CMakeLists.txt - Build configuration for 06_hmsensor example
# ==============================================================================
#
# This CMakeLists.txt builds the HMSensor reader example for both
# embedded (STM32F429) and native (x86 Linux) targets.
#
# ## Embedded Target (STM32F429)
#
# When building with an embedded preset (e.g., stm32f429-debug):
# - Links against hm11_platform (STM32 drivers)
# - Uses real UART5 and DMA hardware
# - Produces .elf and .bin files for flashing
# - LED macros control hardware GPIO pins
#
# ## Native Target (Testing)
#
# When building for native target (default preset):
# - Example is NOT built (no hardware available)
# - Test version is built separately in tests/examples/
# - Uses EnhancedMockUART for simulation
# - LED macros are mocked for validation
#
# ## Usage
#
# Build for embedded target:
#   cmake --preset stm32f429-debug
#   cmake --build --preset stm32f429-debug
#   # Flash: examples/06_hmsensor/06_hmsensor.bin
#
# Build tests (native target):
#   cmake --preset default
#   cmake --build --preset default
#   ctest --preset default -R test_06_hmsensor
#
# ==============================================================================

# Only build examples for embedded targets
# Native targets use test versions in tests/examples/
if(IS_EMBEDDED_TARGET)
    # Create executable for HMSensor reader example
    add_executable(06_hmsensor
        main.cpp
    )

    # Include HM11 driver headers and examples common infrastructure
    target_include_directories(06_hmsensor
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}  # For examples/common/*.hpp
    )

    # Link against platform library (STM32 drivers, UART, DMA, IRQ handlers)
    target_link_libraries(06_hmsensor
        PRIVATE
            hm11_platform
    )

    # Generate binary file for flashing (STM32-specific)
    add_custom_command(TARGET 06_hmsensor POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:06_hmsensor>
            ${CMAKE_CURRENT_BINARY_DIR}/06_hmsensor.bin
        COMMENT "Generating binary file: 06_hmsensor.bin"
    )

    # Generate hex file for flashing (optional, some tools prefer hex)
    add_custom_command(TARGET 06_hmsensor POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:06_hmsensor>
            ${CMAKE_CURRENT_BINARY_DIR}/06_hmsensor.hex
        COMMENT "Generating hex file: 06_hmsensor.hex"
    )

    # Print binary size information
    add_custom_command(TARGET 06_hmsensor POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:06_hmsensor>
        COMMENT "Binary size information:"
    )

    message(STATUS "Building 06_hmsensor example for embedded target")
else()
    message(STATUS "Skipping 06_hmsensor example (native target uses test version)")
endif()
