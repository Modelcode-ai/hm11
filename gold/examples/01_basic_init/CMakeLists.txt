/**
 * @file CMakeLists.txt
 * @brief Build configuration for 01_basic_init example
 *
 * This CMakeLists.txt builds the basic initialization example for both
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
 * # Flash: examples/01_basic_init/01_basic_init.bin
 * ```
 *
 * Build tests (native target):
 * ```bash
 * cmake --preset default
 * cmake --build --preset default
 * ctest --preset default -R test_01_basic_init
 * ```
 */

# Only build examples for embedded targets
# Native targets use test versions in tests/examples/
if(IS_EMBEDDED_TARGET)
    # Create executable for basic initialization example
    add_executable(01_basic_init
        main.cpp
    )

    # Include HM11 driver headers and examples common infrastructure
    target_include_directories(01_basic_init
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}  # For examples/common/*.hpp
    )

    # Link against platform library (STM32 drivers, UART, DMA, IRQ handlers)
    target_link_libraries(01_basic_init
        PRIVATE
            hm11_platform
    )

    # Generate binary file for flashing (STM32-specific)
    add_custom_command(TARGET 01_basic_init POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:01_basic_init>
            ${CMAKE_CURRENT_BINARY_DIR}/01_basic_init.bin
        COMMENT "Generating binary file: 01_basic_init.bin"
    )

    # Generate hex file for flashing (optional, some tools prefer hex)
    add_custom_command(TARGET 01_basic_init POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:01_basic_init>
            ${CMAKE_CURRENT_BINARY_DIR}/01_basic_init.hex
        COMMENT "Generating hex file: 01_basic_init.hex"
    )

    # Print binary size information
    add_custom_command(TARGET 01_basic_init POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:01_basic_init>
        COMMENT "Binary size information:"
    )

    message(STATUS "Building 01_basic_init example for embedded target")
else()
    message(STATUS "Skipping 01_basic_init example (native target uses test version)")
endif()
