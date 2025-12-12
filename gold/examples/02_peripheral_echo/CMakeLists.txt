/**
 * @file CMakeLists.txt
 * @brief Build configuration for 02_peripheral_echo example
 *
 * This CMakeLists.txt builds the peripheral echo server example for both
 * embedded (STM32F429) and native (x86 Linux) targets.
 *
 * ## Embedded Target (STM32F429)
 *
 * When building with an embedded preset (e.g., stm32f429-debug):
 * - Links against hm11_platform (STM32 drivers)
 * - Uses real UART5 and DMA hardware
 * - Produces .elf and .bin files for flashing
 * - LED macros control hardware GPIO pins
 *
 * ## Native Target (Testing)
 *
 * When building for native target (default preset):
 * - Example is NOT built (no hardware available)
 * - Test version is built separately in tests/examples/
 * - Uses EnhancedMockUART for simulation
 * - LED macros are mocked for validation
 *
 * ## Usage
 *
 * Build for embedded target:
 * ```bash
 * cmake --preset stm32f429-debug
 * cmake --build --preset stm32f429-debug
 * # Flash: examples/02_peripheral_echo/02_peripheral_echo.bin
 * ```
 *
 * Build tests (native target):
 * ```bash
 * cmake --preset default
 * cmake --build --preset default
 * ctest --preset default -R test_02_peripheral_echo
 * ```
 */

# Only build examples for embedded targets
# Native targets use test versions in tests/examples/
if(IS_EMBEDDED_TARGET)
    # Create executable for peripheral echo server example
    add_executable(02_peripheral_echo
        main.cpp
    )

    # Include HM11 driver headers and examples common infrastructure
    target_include_directories(02_peripheral_echo
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}  # For examples/common/*.hpp
    )

    # Link against platform library (STM32 drivers, UART, DMA, IRQ handlers)
    target_link_libraries(02_peripheral_echo
        PRIVATE
            hm11_platform
    )

    # Generate binary file for flashing (STM32-specific)
    add_custom_command(TARGET 02_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:02_peripheral_echo>
            ${CMAKE_CURRENT_BINARY_DIR}/02_peripheral_echo.bin
        COMMENT "Generating binary file: 02_peripheral_echo.bin"
    )

    # Generate hex file for flashing (optional, some tools prefer hex)
    add_custom_command(TARGET 02_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:02_peripheral_echo>
            ${CMAKE_CURRENT_BINARY_DIR}/02_peripheral_echo.hex
        COMMENT "Generating hex file: 02_peripheral_echo.hex"
    )

    # Print binary size information
    add_custom_command(TARGET 02_peripheral_echo POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:02_peripheral_echo>
        COMMENT "Binary size information:"
    )

    message(STATUS "Building 02_peripheral_echo example for embedded target")
else()
    message(STATUS "Skipping 02_peripheral_echo example (native target uses test version)")
endif()
