/**
 * @file CMakeLists.txt
 * @brief Build configuration for 03_central_scan example
 *
 * This CMakeLists.txt builds the central scanner example for both
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
 * # Flash: examples/03_central_scan/03_central_scan.bin
 * ```
 *
 * Build tests (native target):
 * ```bash
 * cmake --preset default
 * cmake --build --preset default
 * ctest --preset default -R test_03_central_scan
 * ```
 */

# Only build examples for embedded targets
# Native targets use test versions in tests/examples/
if(IS_EMBEDDED_TARGET)
    # Create executable for central scanner example
    add_executable(03_central_scan
        main.cpp
    )

    # Include HM11 driver headers and examples common infrastructure
    target_include_directories(03_central_scan
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}  # For examples/common/*.hpp
    )

    # Link against platform library (STM32 drivers, UART, DMA, IRQ handlers)
    target_link_libraries(03_central_scan
        PRIVATE
            hm11_platform
    )

    # Generate binary file for flashing (STM32-specific)
    add_custom_command(TARGET 03_central_scan POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:03_central_scan>
            ${CMAKE_CURRENT_BINARY_DIR}/03_central_scan.bin
        COMMENT "Generating binary file: 03_central_scan.bin"
    )

    # Generate hex file for flashing (optional, some tools prefer hex)
    add_custom_command(TARGET 03_central_scan POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:03_central_scan>
            ${CMAKE_CURRENT_BINARY_DIR}/03_central_scan.hex
        COMMENT "Generating hex file: 03_central_scan.hex"
    )

    # Print binary size information
    add_custom_command(TARGET 03_central_scan POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:03_central_scan>
        COMMENT "Binary size information:"
    )

    message(STATUS "Building 03_central_scan example for embedded target")
else()
    message(STATUS "Skipping 03_central_scan example (native target uses test version)")
endif()
