cmake_minimum_required(VERSION 3.20)
project(HM11Driver VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect if we're cross-compiling for embedded target
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(IS_EMBEDDED_TARGET TRUE)
    message(STATUS "Configuring for embedded target: ${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(IS_EMBEDDED_TARGET FALSE)
    message(STATUS "Configuring for native target")
endif()

# Prefer Clang for development (can be overridden by toolchain)
if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT CMAKE_TOOLCHAIN_FILE)
    find_program(CLANG_CXX NAMES clang++-20 clang++)
    if(CLANG_CXX)
        set(CMAKE_CXX_COMPILER ${CLANG_CXX})
    endif()
endif()

# Compiler-specific warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )
elseif(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# Treat warnings as errors in CI or when explicitly requested
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(WARNINGS_AS_ERRORS)
    if(MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

# Optional clang-tidy integration
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" ON)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy-20 clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# Sanitizer support: Enabled by default in Debug builds for safety-critical code
# Provides opt-out via -DENABLE_SANITIZERS=OFF for fast iteration when needed
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" ON)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(IS_EMBEDDED_TARGET)
        message(WARNING "Sanitizers are not available for embedded targets, skipping")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        message(STATUS "  (disable with -DENABLE_SANITIZERS=OFF for fast iteration)")
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(WARNING "Sanitizers requested but not supported by compiler")
    endif()
endif()

# Optional code coverage support (requires GCC or Clang)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    if(IS_EMBEDDED_TARGET)
        message(WARNING "Coverage is not available for embedded targets, skipping")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "Enabling code coverage with GCC (gcov)")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Enabling code coverage with Clang (llvm-cov)")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    else()
        message(WARNING "Coverage requested but not supported by compiler")
    endif()
endif()

# STM32-specific configuration for embedded targets
if(IS_EMBEDDED_TARGET)
    # Verify required variables are set
    if(NOT DEFINED STM32_DEVICE)
        message(FATAL_ERROR "STM32_DEVICE must be defined (e.g., STM32F429xx)")
    endif()

    if(NOT DEFINED LINKER_SCRIPT)
        message(FATAL_ERROR "LINKER_SCRIPT must be defined")
    endif()

    # Set CPU-specific compiler flags for ARM Cortex-M4
    add_compile_options(
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
    )

    # Define STM32 device preprocessor symbol
    add_compile_definitions(${STM32_DEVICE})
    # Note: USE_HAL_DRIVER intentionally NOT defined - we use direct CMSIS register access
    # per Decision 30 (STM32 Platform Implementation Approach) which mandates
    # "direct register access via CMSIS device headers WITHOUT STM32 HAL library dependency"
    add_compile_definitions(ARM_MATH_CM4)
    add_compile_definitions(__FPU_PRESENT=1)

    # Set linker flags
    add_link_options(
        -T${LINKER_SCRIPT}
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -specs=nano.specs
        -specs=nosys.specs
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
        -Wl,--print-memory-usage
    )

    # Add CMSIS include path if specified
    if(DEFINED CMSIS_PATH)
        if(EXISTS ${CMSIS_PATH})
            message(STATUS "Using CMSIS from: ${CMSIS_PATH}")
            include_directories(SYSTEM
                ${CMSIS_PATH}/Include
                ${CMSIS_PATH}/Device/ST/STM32F4xx/Include
            )
        else()
            message(WARNING "CMSIS_PATH specified but not found: ${CMSIS_PATH}")
            message(WARNING "Please install CMSIS headers or update CMSIS_PATH")
            message(WARNING "You can download CMSIS from: https://github.com/ARM-software/CMSIS_5")
        endif()
    endif()

    message(STATUS "STM32 Device: ${STM32_DEVICE}")
    message(STATUS "Linker Script: ${LINKER_SCRIPT}")
endif()

# Find dependencies (managed by Conan, only for native builds)
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_RUNTIME_RANGE_CHECKS "Enable runtime range validation in RangedInteger/FixedString" OFF)

if(NOT IS_EMBEDDED_TARGET)
    # Optional dependencies - only required if features need them
    find_package(Boost QUIET)
    find_package(fmt QUIET)
    find_package(spdlog QUIET)

    if(BUILD_TESTS)
        find_package(GTest REQUIRED)
    endif()
else()
    # For embedded targets, dependencies will be managed differently
    message(STATUS "Skipping Conan dependencies for embedded target")
endif()

# ============================================================================
# HM11 Type System - Header-Only Library
# ============================================================================

# Configure header-only library for type system (Decision #2: Strong Type System Translation)
add_subdirectory(include/hm11)

# ============================================================================
# Directory structure placeholders
# ============================================================================

# The following directories are expected to be populated with source files:
# - src/         : Main source files (driver implementation)
# - include/     : Public header files (type system - COMPLETED)
# - tests/       : Unit tests (type system tests - COMPLETED)
# - fuzz/        : Fuzzing tests (to be implemented)
# - examples/    : Example applications (to be implemented)

# Platform abstraction library - ISR-safe primitives and DMA operations
set(PLATFORM_SOURCES
    src/platform/wait_event.cpp
    src/platform/dma_cache.cpp
)

# Add STM32 startup code for embedded targets
if(IS_EMBEDDED_TARGET AND DEFINED STM32_DEVICE AND STM32_DEVICE MATCHES "STM32F429")
    list(APPEND PLATFORM_SOURCES src/platform/stm32f429_startup.cpp)
    message(STATUS "Including STM32F429 startup code in platform library")
endif()

add_library(hm11_platform STATIC ${PLATFORM_SOURCES})
add_library(hm11::platform ALIAS hm11_platform)

target_include_directories(hm11_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(hm11_platform 
    PUBLIC 
        hm11::types
)

target_compile_features(hm11_platform PUBLIC cxx_std_20)

# Platform-specific compiler definitions
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "cortex")
    target_compile_definitions(hm11_platform PRIVATE __ARM_ARCH__)
endif()

# STM32-specific definitions based on target chip
if(DEFINED STM32_CHIP)
    if(STM32_CHIP MATCHES "STM32F4")
        target_compile_definitions(hm11_platform PRIVATE STM32F4xx)
        message(STATUS "Platform: STM32F4xx (Cortex-M4, no data cache)")
    elseif(STM32_CHIP MATCHES "STM32F7")
        target_compile_definitions(hm11_platform PRIVATE STM32F7xx)
        message(STATUS "Platform: STM32F7xx (Cortex-M7, with data cache)")
    elseif(STM32_CHIP MATCHES "STM32H7")
        target_compile_definitions(hm11_platform PRIVATE STM32H7xx)
        message(STATUS "Platform: STM32H7xx (Cortex-M7, with data cache)")
    endif()
else()
    message(STATUS "Platform: Generic (using fallback implementations)")
endif()

# Platform library installation (separate from include/hm11 libs for scope reasons)
if(NOT IS_EMBEDDED_TARGET)
    install(TARGETS hm11_platform
        EXPORT hm11Targets
        INCLUDES DESTINATION include
    )
    
    # Install platform headers
    install(FILES
        include/hm11/platform/dma_cache.hpp
        include/hm11/platform/timing.hpp  
        include/hm11/platform/platform.hpp
        include/hm11/platform/wait_event.hpp
        DESTINATION include/hm11/platform
    )
endif()

# Note: Synchronization primitives library (hm11::sync) is defined in include/hm11/CMakeLists.txt
# It already includes the platform abstraction dependencies

# Main executable (to be implemented)
# if(NOT IS_EMBEDDED_TARGET AND fmt_FOUND AND spdlog_FOUND)
#     add_executable(hm11_driver
#         src/main.cpp
#     )
#     target_include_directories(hm11_driver
#         PRIVATE
#             ${CMAKE_CURRENT_SOURCE_DIR}/include
#     )
#     target_link_libraries(hm11_driver PRIVATE
#         fmt::fmt
#         spdlog::spdlog
#     )
# endif()

# Enable testing (only for native builds)
if(BUILD_TESTS AND NOT IS_EMBEDDED_TARGET)
    enable_testing()
    message(STATUS "Testing enabled - building type system tests")

    # Type system tests - now using the header-only library target
    add_executable(hm11_type_tests
        tests/types/test_ranged_integer.cpp
        tests/types/test_fixed_string.cpp
        tests/types/test_type_aliases.cpp
    )
    target_link_libraries(hm11_type_tests
        PRIVATE
            hm11::types
            GTest::gtest_main
    )
    
    # HAL and driver tests - using HAL and driver library targets
    add_executable(hm11_hal_tests
        tests/hal/test_uart_interface.cpp
        tests/hal/test_hm11_driver.cpp
        tests/hal/test_basic_operations.cpp
        tests/hal/test_gatt_service_discovery.cpp
        tests/hal/test_ibeacon_operations.cpp
        tests/hal/test_sensor_operations.cpp
        tests/hal/test_core_operations.cpp  # Core operations and fundamental driver functionality
        tests/hal/test_configuration_api.cpp   # Configuration API tests
        tests/hal/test_error_handling.cpp # Driver error handling tests
        tests/hal/test_coverage_recovery.cpp # Coverage recovery tests for previously deleted tests
        tests/hal/test_utility_types.cpp      # Utility types and parsing tests
        tests/hal/test_enum_conversions.cpp # Comprehensive enum conversion tests
        tests/hal/test_driver_gatt_and_battery.cpp # GATT and battery query tests
        tests/hal/test_buffer_capacity.cpp # Buffer capacity and invalid character validation tests
    )
    
    # Performance and stress tests - separate executable for performance testing
    add_executable(hm11_performance_tests
        tests/hal/test_performance_streaming.cpp
    )
    target_link_libraries(hm11_performance_tests
        PRIVATE
            hm11::driver
            GTest::gtest_main
    )
    
    # Fuzzing tests - separate executable for security testing
    add_executable(hm11_fuzzing_tests
        tests/util/test_parsing.cpp  # Parsing tests (comprehensive and parameterized)
        tests/util/test_parsing_circular_buffer.cpp  # Circular buffer parsing tests
        tests/util/test_string_builder.cpp  # String builder tests
    )
    target_link_libraries(hm11_fuzzing_tests
        PRIVATE
            hm11::types
            GTest::gtest_main
    )
    target_link_libraries(hm11_hal_tests
        PRIVATE
            hm11::driver  # This transitively includes hm11::hal and hm11::types
            GTest::gtest_main
    )
    
    # Synchronization primitives tests - using sync library target
    add_executable(hm11_sync_tests
        tests/sync/test_binary_semaphore.cpp
        tests/sync/test_circular_buffer.cpp
        tests/sync/test_watchdog.cpp
    )
    target_link_libraries(hm11_sync_tests
        PRIVATE
            hm11::sync  # This includes the compiled sync primitives
            GTest::gtest_main
    )
    
    # Platform abstraction tests - platform functionality and feature detection
    add_executable(hm11_platform_tests
        tests/platform/test_dma_cache.cpp
        tests/platform/test_timing.cpp
        tests/platform/test_platform.cpp
    )
    target_link_libraries(hm11_platform_tests
        PRIVATE
            hm11::platform  # Platform abstraction layer
            GTest::gtest_main
    )
    
    # Add individual test suites
    add_test(NAME ranged_integer_tests COMMAND hm11_type_tests --gtest_filter="RangedIntegerTest.*")
    add_test(NAME fixed_string_tests COMMAND hm11_type_tests --gtest_filter="FixedStringTest.*") 
    add_test(NAME type_aliases_tests COMMAND hm11_type_tests --gtest_filter="TypeAliasesTest.*")
    add_test(NAME all_type_tests COMMAND hm11_type_tests)
    
    # HAL test suites
    add_test(NAME uart_interface_tests COMMAND hm11_hal_tests --gtest_filter="UARTInterfaceTest.*")
    add_test(NAME hm11_driver_tests COMMAND hm11_hal_tests --gtest_filter="HM11DriverTest.*")
    add_test(NAME basic_operations_tests COMMAND hm11_hal_tests --gtest_filter="BasicOperationsTest.*")
    add_test(NAME gatt_service_discovery_tests COMMAND hm11_hal_tests --gtest_filter="GattServiceDiscoveryTest.*")
    add_test(NAME gatt_characteristic_operations_tests COMMAND hm11_hal_tests --gtest_filter="GattCharacteristic*Test.*")
    add_test(NAME ibeacon_operations_tests COMMAND hm11_hal_tests --gtest_filter="IBeaconOperationsTest.*")
    add_test(NAME sensor_operations_tests COMMAND hm11_hal_tests --gtest_filter="SensorOperationsTest.*")
    add_test(NAME advanced_configuration_tests COMMAND hm11_hal_tests --gtest_filter="AdvancedConfigurationTest.*")
    add_test(NAME integration_workflow_tests COMMAND hm11_hal_tests --gtest_filter="IntegrationWorkflowTest.*")
    add_test(NAME all_hal_tests COMMAND hm11_hal_tests)
    
    # Performance test suites
    add_test(NAME performance_streaming_tests COMMAND hm11_performance_tests --gtest_filter="PerformanceStreamingTest.*")
    add_test(NAME all_performance_tests COMMAND hm11_performance_tests)
    
    # Fuzzing test suites
    add_test(NAME fuzzing_parsing_tests COMMAND hm11_fuzzing_tests --gtest_filter="FuzzingParsingTest.*")
    add_test(NAME all_fuzzing_tests COMMAND hm11_fuzzing_tests)
    
    # Sync test suites
    add_test(NAME binary_semaphore_tests COMMAND hm11_sync_tests --gtest_filter="BinarySemaphoreTest.*")
    add_test(NAME circular_buffer_tests COMMAND hm11_sync_tests --gtest_filter="CircularBufferTest.*")
    add_test(NAME watchdog_tests COMMAND hm11_sync_tests --gtest_filter="WatchdogTest.*")
    add_test(NAME all_sync_tests COMMAND hm11_sync_tests)
    
    # Platform test suites
    add_test(NAME dma_cache_tests COMMAND hm11_platform_tests --gtest_filter="DmaCacheTest.*")
    add_test(NAME timing_tests COMMAND hm11_platform_tests --gtest_filter="TimingTest.*")
    add_test(NAME platform_tests COMMAND hm11_platform_tests --gtest_filter="PlatformTest.*")
    add_test(NAME all_platform_tests COMMAND hm11_platform_tests)
    
    # Combined test suite for convenience
    add_test(NAME all_tests COMMAND ${CMAKE_CTEST_COMMAND} -R "all_.*_tests")

else()
    message(STATUS "Tests disabled for embedded target")
endif()

# Custom target for running clang-tidy manually
if(CLANG_TIDY_EXE)
    # Find all source files to analyze
    file(GLOB_RECURSE CLANG_TIDY_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    )
    
    if(CLANG_TIDY_SOURCES)
        add_custom_target(run-clang-tidy
            COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy on ${CMAKE_PROJECT_NAME} files..."
            COMMAND ${CMAKE_COMMAND} -E echo "Files already processed during compilation with CMAKE_CXX_CLANG_TIDY"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-tidy (integrated with build system)"
        )

        # Alias for CI/CD compatibility
        add_custom_target(clang-tidy-20
            COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy on ${CMAKE_PROJECT_NAME} files..."
            COMMAND ${CMAKE_COMMAND} -E echo "Files already processed during compilation with CMAKE_CXX_CLANG_TIDY"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-tidy (integrated with build system, CI/CD alias)"
        )
    else()
        message(STATUS "No source files found for clang-tidy analysis")
    endif()
endif()

# Custom targets for running clang-format
find_program(CLANG_FORMAT_EXE NAMES clang-format-20 clang-format)
if(CLANG_FORMAT_EXE)
    # Collect all source files
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    # Only create targets if source files exist
    if(ALL_SOURCE_FILES)
        # Target to format all files in-place
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Formatting all source files with clang-format"
        )

        # Target to check formatting without modifying files
        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Checking code formatting (no changes made)"
        )

        message(STATUS "clang-format enabled: ${CLANG_FORMAT_EXE}")
        message(STATUS "  Run 'cmake --build --preset <preset> --target format' to format code")
        message(STATUS "  Run 'cmake --build --preset <preset> --target format-check' to check formatting")
    else()
        message(STATUS "clang-format found but no source files to format yet")
    endif()
else()
    message(WARNING "clang-format not found, formatting targets disabled")
endif()

# ============================================================================
# Fuzzing Tests (LibFuzzer)
# ============================================================================
option(ENABLE_FUZZING "Build fuzzing tests with LibFuzzer" OFF)

if(ENABLE_FUZZING AND NOT IS_EMBEDDED_TARGET)
    # Check if compiler supports fuzzing
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "Fuzzing requires Clang compiler, skipping fuzzing targets")
        set(ENABLE_FUZZING OFF)
    else()
        message(STATUS "Fuzzing enabled - add fuzzing targets in fuzz/ directory")

        # Create corpus directories
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus)

        # Common fuzzing flags
        set(FUZZING_FLAGS -fsanitize=fuzzer,address,undefined -g -O1)

        # Example fuzzing target structure (uncomment when fuzz tests are added):
        # add_executable(fuzz_target
        #     fuzz/fuzz_target.cpp
        # )
        # target_include_directories(fuzz_target PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        # target_compile_options(fuzz_target PRIVATE ${FUZZING_FLAGS})
        # target_link_options(fuzz_target PRIVATE ${FUZZING_FLAGS})
        # set_target_properties(fuzz_target PROPERTIES
        #     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz
        #     CXX_CLANG_TIDY ""
        # )
    endif()
endif()

# ============================================================================
# Stress Tests (ThreadSanitizer)
# ============================================================================
option(ENABLE_STRESS_TESTS "Build stress tests with ThreadSanitizer" OFF)

if(ENABLE_STRESS_TESTS AND BUILD_TESTS AND NOT IS_EMBEDDED_TARGET)
    message(STATUS "Stress tests enabled - building with ThreadSanitizer")

    # Check if compiler supports ThreadSanitizer
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(WARNING "ThreadSanitizer requires Clang or GCC, building without TSan")
        set(TSAN_FLAGS "")
    else()
        set(TSAN_FLAGS -fsanitize=thread -g -O1)
        message(STATUS "Using ThreadSanitizer for race condition detection")
    endif()

    # Synchronization stress tests for race condition detection
    add_executable(hm11_sync_stress_tests
        tests/sync/test_stress.cpp
    )
    target_link_libraries(hm11_sync_stress_tests
        PRIVATE
            hm11::sync
            GTest::gtest_main
    )
    if(TSAN_FLAGS)
        target_compile_options(hm11_sync_stress_tests PRIVATE ${TSAN_FLAGS})
        target_link_options(hm11_sync_stress_tests PRIVATE ${TSAN_FLAGS})
    endif()
    set_target_properties(hm11_sync_stress_tests PROPERTIES
        CXX_CLANG_TIDY ""  # Disable clang-tidy for stress tests (performance focus)
    )
    add_test(NAME sync_stress_tests COMMAND hm11_sync_stress_tests)
endif()

# ============================================================================
# Examples (embedded targets only)
# ============================================================================

# Examples are built for both embedded and native targets
# Native targets can execute examples for validation
# Embedded targets build examples for hardware testing
add_subdirectory(examples)

# ============================================================================
# SonarQube Analysis
# ============================================================================

# SonarQube scanner configuration
set(SONAR_SCANNER_EXECUTABLE "" CACHE STRING "Path to sonar-scanner executable")
set(SONAR_HOST_URL "http://localhost:9000" CACHE STRING "SonarQube server URL")
set(SONAR_PROJECT_KEY "hm11-driver" CACHE STRING "SonarQube project key")

# Find sonar-scanner executable
if(NOT SONAR_SCANNER_EXECUTABLE)
    find_program(SONAR_SCANNER_FOUND
        NAMES sonar-scanner
        PATHS /usr/local/bin /opt/sonar-scanner/bin
    )
    if(SONAR_SCANNER_FOUND)
        set(SONAR_SCANNER_EXECUTABLE "${SONAR_SCANNER_FOUND}")
    endif()
endif()

# Create SonarQube analysis target
if(SONAR_SCANNER_EXECUTABLE)
    # Detect if gosu is available for running as non-root
    find_program(GOSU_EXECUTABLE NAMES gosu)

    # Use helper script to handle gosu logic cleanly
    set(SONAR_RUNNER_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/run-sonar-scanner.sh")

    if(EXISTS "${SONAR_RUNNER_SCRIPT}")
        if(GOSU_EXECUTABLE)
            # Run with gosu support
            add_custom_target(sonar
                COMMAND ${CMAKE_COMMAND} -E echo "Running SonarQube analysis..."
                COMMAND ${CMAKE_COMMAND} -E echo "Scanner: ${SONAR_SCANNER_EXECUTABLE}"
                COMMAND ${CMAKE_COMMAND} -E echo "Host: ${SONAR_HOST_URL}"
                COMMAND ${CMAKE_COMMAND} -E echo "Project: ${SONAR_PROJECT_KEY}"
                COMMAND ${CMAKE_COMMAND} -E echo ""
                COMMAND ${SONAR_RUNNER_SCRIPT}
                    ${SONAR_SCANNER_EXECUTABLE}
                    ${SONAR_HOST_URL}
                    ${SONAR_PROJECT_KEY}
                    ${CMAKE_SOURCE_DIR}
                    ${GOSU_EXECUTABLE}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running SonarQube analysis (with gosu for root user)"
                USES_TERMINAL
            )
            message(STATUS "SonarQube scanner found: ${SONAR_SCANNER_EXECUTABLE}")
            message(STATUS "gosu found: ${GOSU_EXECUTABLE} (will drop privileges if running as root)")
        else()
            # Run without gosu
            add_custom_target(sonar
                COMMAND ${CMAKE_COMMAND} -E echo "Running SonarQube analysis..."
                COMMAND ${CMAKE_COMMAND} -E echo "Scanner: ${SONAR_SCANNER_EXECUTABLE}"
                COMMAND ${CMAKE_COMMAND} -E echo "Host: ${SONAR_HOST_URL}"
                COMMAND ${CMAKE_COMMAND} -E echo "Project: ${SONAR_PROJECT_KEY}"
                COMMAND ${CMAKE_COMMAND} -E echo ""
                COMMAND ${SONAR_RUNNER_SCRIPT}
                    ${SONAR_SCANNER_EXECUTABLE}
                    ${SONAR_HOST_URL}
                    ${SONAR_PROJECT_KEY}
                    ${CMAKE_SOURCE_DIR}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running SonarQube analysis"
                USES_TERMINAL
            )
            message(STATUS "SonarQube scanner found: ${SONAR_SCANNER_EXECUTABLE}")
            message(STATUS "gosu not found - will run scanner directly (may fail if running as root)")
        endif()

        message(STATUS "  Run 'cmake --build build/sonarqube --target sonar' to analyze")
        message(STATUS "  Set SONAR_TOKEN environment variable before running")
        message(STATUS "    export SONAR_TOKEN=squ_your_token_here")
    else()
        message(STATUS "SonarQube scanner found but helper script missing: ${SONAR_RUNNER_SCRIPT}")
    endif()
else()
    message(STATUS "SonarQube scanner not found (optional)")
    message(STATUS "  Install: brew install sonar-scanner")
    message(STATUS "  Or set SONAR_SCANNER_EXECUTABLE=/path/to/sonar-scanner")
endif()

# ============================================================================
# Convenience targets
# ============================================================================

# Run all tests (unit + stress if enabled)
add_custom_target(run-all-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running all tests (unit + stress)"
)

# Installation rules (for future use)
# Uncomment when main executable is built:
# install(TARGETS hm11_driver
#     RUNTIME DESTINATION bin
# )

# install(DIRECTORY include/hm11
#     DESTINATION include
# )
