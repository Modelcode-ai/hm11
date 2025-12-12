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

# Platform abstraction library
# This library provides platform-specific implementations for both native and embedded targets
set(PLATFORM_SOURCES "")

if(IS_EMBEDDED_TARGET)
    # ARM Cortex-M implementation using WFE/SEV
    list(APPEND PLATFORM_SOURCES
        src/platform/wait_event_arm.cpp
    )
    # STM32 interrupt handlers (Decision 16: Interrupt Handler Registration)
    list(APPEND PLATFORM_SOURCES
        src/stm32/irq_handlers.cpp
        src/stm32/dma_receive_handler.cpp
    )
    # STM32 hardware initialization and driver instance
    list(APPEND PLATFORM_SOURCES
        src/stm32/drivers.cpp
    )
    message(STATUS "Using ARM Cortex-M platform implementation (WFE/SEV)")
else()
    # Linux x86 implementation using futex
    list(APPEND PLATFORM_SOURCES
        src/platform/wait_event_linux.cpp
    )
    message(STATUS "Using Linux x86 platform implementation (futex)")
endif()

# Add synchronization primitives that depend on platform abstractions
list(APPEND PLATFORM_SOURCES
    src/sync/binary_semaphore.cpp
    src/sync/watchdog.cpp
)

# Create platform library (header-only interface with platform-specific implementation)
add_library(hm11_platform STATIC ${PLATFORM_SOURCES})

target_include_directories(hm11_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform library only needs standard library (atomic, thread support)
# No additional dependencies required
if(NOT IS_EMBEDDED_TARGET)
    # Linux implementation needs pthread for std::thread in tests
    target_link_libraries(hm11_platform PUBLIC pthread)
endif()

# Main executable (minimal placeholder for now)
# Only build if fmt and spdlog are available (main.cpp uses them)
if(NOT IS_EMBEDDED_TARGET AND fmt_FOUND AND spdlog_FOUND)
    add_executable(hm11_driver
        src/main.cpp
    )

    target_include_directories(hm11_driver
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # Link required dependencies
    target_link_libraries(hm11_driver PRIVATE
        fmt::fmt
        spdlog::spdlog
    )

    # Link optional dependencies if found
    if(Boost_FOUND)
        target_link_libraries(hm11_driver PRIVATE Boost::boost)
    endif()

    # Ensure proper RTTI and exception handling settings
    target_compile_options(hm11_driver PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-rtti>
    )
else()
    if(NOT IS_EMBEDDED_TARGET)
        message(STATUS "Skipping hm11_driver executable (requires fmt and spdlog)")
    else()
        message(STATUS "Embedded target: main executable may be added as needed")
    endif()
endif()

# Enable testing (only for native builds)
if(BUILD_TESTS AND NOT IS_EMBEDDED_TARGET)
    enable_testing()

    # Main test executable with all unit tests
    add_executable(hm11_tests
        tests/test_main.cpp
        tests/test_validators.cpp
        tests/test_fixed_string.cpp
        tests/test_ranged_integer.cpp
        tests/test_uart_types.cpp
        tests/test_uart_port.cpp
        tests/test_stm32_uart_port.cpp
        tests/test_wait_event.cpp
        tests/test_cache_ops.cpp
        tests/test_circular_buffer.cpp
        tests/test_binary_semaphore.cpp
        tests/test_watchdog.cpp
        tests/test_enum_mapping.cpp
        tests/test_parsing_utils.cpp
        tests/util/command_builder_test.cpp
        tests/test_hm11_driver_core.cpp
        tests/test_hm11_driver_advertising.cpp
        tests/test_hm11_driver_device.cpp
        tests/test_hm11_driver_discovery.cpp
        tests/test_hm11_driver_gatt_discovery.cpp
        tests/test_hm11_driver_gatt_access.cpp
        tests/test_hm11_driver_ibeacon.cpp
        tests/test_hm11_driver_linklayer.cpp
        tests/test_hm11_driver_security.cpp
        tests/test_hm11_driver_uart.cpp
        tests/test_hm11_driver_sensor.cpp
        tests/test_hm11_driver_hardware.cpp
        tests/integration_test.cpp
        # Enhanced mock utilities
        tests/util/enhanced_mock_uart.cpp
        tests/util/mock_led.cpp
        tests/util/mock_timer.cpp
        tests/util/hm11_response_builder.cpp
        tests/test_enhanced_mock_uart.cpp
        # Example tests
        tests/examples/test_01_basic_init.cpp
        tests/examples/test_02_peripheral_echo.cpp
        tests/examples/test_03_central_scan.cpp
        tests/examples/test_04_gatt_client.cpp
        tests/examples/test_05_ibeacon.cpp
        tests/examples/test_06_hmsensor.cpp
    )

    target_include_directories(hm11_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/util
    )

    target_compile_definitions(hm11_tests
        PRIVATE
            HM11_ENABLE_TEST_INSTANTIATIONS
    )

    # Disable clang-tidy for test files to avoid timeout on template instantiations
    # Test files can have millions of template instantiations which causes clang-tidy to timeout
    set_target_properties(hm11_tests PROPERTIES
        CXX_CLANG_TIDY ""
    )

    target_link_libraries(hm11_tests
        PRIVATE
            hm11_platform
            GTest::gtest_main
    )

    # Only link optional dependencies if they are found
    if(Boost_FOUND)
        target_link_libraries(hm11_tests PRIVATE Boost::boost)
    endif()
    if(fmt_FOUND)
        target_link_libraries(hm11_tests PRIVATE fmt::fmt)
    endif()
    if(spdlog_FOUND)
        target_link_libraries(hm11_tests PRIVATE spdlog::spdlog)
    endif()

    # Register test with CTest
    add_test(NAME unit_tests COMMAND hm11_tests)
else()
    message(STATUS "Tests disabled for embedded target")
endif()

# Custom target for running clang-tidy manually
# Note: Test files are excluded to prevent timeout from template instantiations
if(CLANG_TIDY_EXE)
    add_custom_target(run-clang-tidy
        COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy manually on source files (tests excluded)"
    )

    # Alias for CI/CD compatibility
    add_custom_target(clang-tidy-20
        COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy manually on source files (CI/CD alias, tests excluded)"
    )
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
        message(STATUS "Fuzzing enabled - building LibFuzzer targets")

        # Create corpus directories
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus/parsing)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus/fixed_string)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus/ranged_integer)

        # Common fuzzing flags
        set(FUZZING_FLAGS -fsanitize=fuzzer,address,undefined -g -O1)

        # Fuzzing target 1: Parsing utilities
        add_executable(fuzz_parsing_utils
            fuzz/fuzz_parsing_utils.cpp
        )
        target_include_directories(fuzz_parsing_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_compile_options(fuzz_parsing_utils PRIVATE ${FUZZING_FLAGS})
        target_link_options(fuzz_parsing_utils PRIVATE ${FUZZING_FLAGS})
        set_target_properties(fuzz_parsing_utils PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz
            CXX_CLANG_TIDY ""  # Disable clang-tidy for fuzz targets
        )

        # Fuzzing target 2: Fixed string
        add_executable(fuzz_fixed_string
            fuzz/fuzz_fixed_string.cpp
        )
        target_include_directories(fuzz_fixed_string PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_compile_options(fuzz_fixed_string PRIVATE ${FUZZING_FLAGS})
        target_link_options(fuzz_fixed_string PRIVATE ${FUZZING_FLAGS})
        set_target_properties(fuzz_fixed_string PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz
            CXX_CLANG_TIDY ""  # Disable clang-tidy for fuzz targets
        )

        # Fuzzing target 3: Ranged integer
        add_executable(fuzz_ranged_integer
            fuzz/fuzz_ranged_integer.cpp
        )
        target_include_directories(fuzz_ranged_integer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_compile_options(fuzz_ranged_integer PRIVATE ${FUZZING_FLAGS})
        target_link_options(fuzz_ranged_integer PRIVATE ${FUZZING_FLAGS})
        set_target_properties(fuzz_ranged_integer PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz
            CXX_CLANG_TIDY ""  # Disable clang-tidy for fuzz targets
        )

        # Custom target to run all fuzzing tests (5 minutes each)
        add_custom_target(run-fuzz
            COMMAND echo "Running fuzz_parsing_utils for 5 minutes..."
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_parsing_utils
                -max_len=1000 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/parsing || true
            COMMAND echo ""
            COMMAND echo "Running fuzz_fixed_string for 5 minutes..."
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_fixed_string
                -max_len=100 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/fixed_string || true
            COMMAND echo ""
            COMMAND echo "Running fuzz_ranged_integer for 5 minutes..."
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_ranged_integer
                -max_len=100 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/ranged_integer || true
            DEPENDS fuzz_parsing_utils fuzz_fixed_string fuzz_ranged_integer
            COMMENT "Running all fuzzing tests (15 minutes total)"
        )

        # Individual run targets for each fuzzer
        add_custom_target(run-fuzz-parsing
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_parsing_utils
                -max_len=1000 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/parsing
            DEPENDS fuzz_parsing_utils
            COMMENT "Running parsing utilities fuzzer (5 minutes)"
        )

        add_custom_target(run-fuzz-fixed-string
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_fixed_string
                -max_len=100 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/fixed_string
            DEPENDS fuzz_fixed_string
            COMMENT "Running fixed string fuzzer (5 minutes)"
        )

        add_custom_target(run-fuzz-ranged-integer
            COMMAND ${CMAKE_BINARY_DIR}/fuzz/fuzz_ranged_integer
                -max_len=100 -max_total_time=300
                ${CMAKE_BINARY_DIR}/corpus/ranged_integer
            DEPENDS fuzz_ranged_integer
            COMMENT "Running ranged integer fuzzer (5 minutes)"
        )

        message(STATUS "Fuzzing targets created:")
        message(STATUS "  - Run 'make fuzz_parsing_utils' to build")
        message(STATUS "  - Run 'make run-fuzz' to run all fuzzers (15 min)")
        message(STATUS "  - Run 'make run-fuzz-parsing' for individual fuzzer")
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

    # Stress test 1: Circular buffer
    add_executable(stress_test_circular_buffer
        tests/stress_test_circular_buffer.cpp
    )
    target_include_directories(stress_test_circular_buffer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(stress_test_circular_buffer PRIVATE GTest::gtest_main pthread)
    if(TSAN_FLAGS)
        target_compile_options(stress_test_circular_buffer PRIVATE ${TSAN_FLAGS})
        target_link_options(stress_test_circular_buffer PRIVATE ${TSAN_FLAGS})
    endif()
    set_target_properties(stress_test_circular_buffer PROPERTIES
        CXX_CLANG_TIDY ""  # Disable clang-tidy for stress tests
    )

    # Stress test 2: Watchdog
    add_executable(stress_test_watchdog
        tests/stress_test_watchdog.cpp
        src/sync/watchdog.cpp
        src/sync/binary_semaphore.cpp
        src/platform/wait_event_linux.cpp
    )
    target_include_directories(stress_test_watchdog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(stress_test_watchdog PRIVATE GTest::gtest_main pthread)
    if(TSAN_FLAGS)
        target_compile_options(stress_test_watchdog PRIVATE ${TSAN_FLAGS})
        target_link_options(stress_test_watchdog PRIVATE ${TSAN_FLAGS})
    endif()
    set_target_properties(stress_test_watchdog PROPERTIES
        CXX_CLANG_TIDY ""  # Disable clang-tidy for stress tests
    )

    # Stress test 3: Binary semaphore
    add_executable(stress_test_binary_semaphore
        tests/stress_test_binary_semaphore.cpp
        src/sync/binary_semaphore.cpp
        src/platform/wait_event_linux.cpp
    )
    target_include_directories(stress_test_binary_semaphore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(stress_test_binary_semaphore PRIVATE GTest::gtest_main pthread)
    if(TSAN_FLAGS)
        target_compile_options(stress_test_binary_semaphore PRIVATE ${TSAN_FLAGS})
        target_link_options(stress_test_binary_semaphore PRIVATE ${TSAN_FLAGS})
    endif()
    set_target_properties(stress_test_binary_semaphore PROPERTIES
        CXX_CLANG_TIDY ""  # Disable clang-tidy for stress tests
    )

    # Register stress tests with CTest
    add_test(NAME stress_circular_buffer COMMAND stress_test_circular_buffer)
    add_test(NAME stress_watchdog COMMAND stress_test_watchdog)
    add_test(NAME stress_binary_semaphore COMMAND stress_test_binary_semaphore)

    # Set test properties for stress tests
    set_tests_properties(stress_circular_buffer stress_watchdog stress_binary_semaphore
        PROPERTIES
            LABELS "stress"
            TIMEOUT 300  # 5 minutes timeout for stress tests
            ENVIRONMENT "TSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/tsan.supp"
    )

    # Custom target to run all stress tests
    add_custom_target(run-stress
        COMMAND ${CMAKE_CTEST_COMMAND} -L stress --output-on-failure
        DEPENDS stress_test_circular_buffer stress_test_watchdog stress_test_binary_semaphore
        COMMENT "Running all stress tests with ThreadSanitizer"
    )

    message(STATUS "Stress test targets created:")
    message(STATUS "  - Run 'make stress_test_circular_buffer' to build")
    message(STATUS "  - Run 'ctest -L stress' to run all stress tests")
    message(STATUS "  - Run 'make run-stress' to run stress tests")
endif()

# ============================================================================
# Examples (embedded targets only)
# ============================================================================

# Examples are only built for embedded targets
# Native targets use test versions in tests/examples/
# The examples/CMakeLists.txt manages all example subdirectories
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
install(TARGETS hm11_driver
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/hm11
    DESTINATION include
)
