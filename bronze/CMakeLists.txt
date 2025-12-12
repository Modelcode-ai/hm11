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

    # Load STM32 family-specific configuration
    if(DEFINED STM32F7)
        message(STATUS "Including STM32F7 family configuration")
        include(${CMAKE_SOURCE_DIR}/cmake/stm32f7.cmake)
    elseif(DEFINED STM32H7)
        message(STATUS "Including STM32H7 family configuration")
        include(${CMAKE_SOURCE_DIR}/cmake/stm32h7.cmake)
    else()
        # Default to STM32F4 family
        message(STATUS "Including STM32F4 family configuration (default)")
        include(${CMAKE_SOURCE_DIR}/cmake/stm32f4.cmake)
    endif()

    # Define STM32 device preprocessor symbol (common to all families)
    add_compile_definitions(${STM32_DEVICE})
    # Note: USE_HAL_DRIVER intentionally NOT defined - we use direct CMSIS register access
    # per Decision 30 (STM32 Platform Implementation Approach) which mandates
    # "direct register access via CMSIS device headers WITHOUT STM32 HAL library dependency"

    # Common map file output for all STM32 targets
    add_link_options(
        -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
    )

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

# ============================================================================
# Directory structure
# ============================================================================

# The following directories are expected to be populated with source files:
# - src/         : Main source files
# - include/     : Public header files
# - tests/       : Unit tests
# - fuzz/        : Fuzzing tests
# - examples/    : Example applications

# Add subdirectories for platform and sync modules
add_subdirectory(src)

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
    message(STATUS "Testing enabled - building test executables")
    
    # Add define for build tests
    add_compile_definitions(BUILD_TESTS)

    # Include tests directory
    add_subdirectory(tests)
else()
    message(STATUS "Tests disabled for embedded target")
endif()

# Custom target for running clang-tidy manually
if(CLANG_TIDY_EXE)
    # Check if src directory exists and has .cpp files
    file(GLOB_RECURSE SRC_CPP_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
    )

    if(SRC_CPP_FILES)
        add_custom_target(run-clang-tidy
            COMMAND ${CLANG_TIDY_EXE}
                -p ${CMAKE_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-tidy manually on source files"
        )

        # Alias for CI/CD compatibility
        add_custom_target(clang-tidy-20
            COMMAND ${CLANG_TIDY_EXE}
                -p ${CMAKE_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-tidy manually on source files (CI/CD alias)"
        )

        message(STATUS "clang-tidy targets enabled: run-clang-tidy, clang-tidy-20")
    else()
        # Create no-op targets when no source files exist
        add_custom_target(run-clang-tidy
            COMMAND ${CMAKE_COMMAND} -E echo "No source files to analyze"
            COMMENT "clang-tidy: no source files in src/ directory"
        )

        add_custom_target(clang-tidy-20
            COMMAND ${CMAKE_COMMAND} -E echo "No source files to analyze"
            COMMENT "clang-tidy: no source files in src/ directory"
        )

        message(STATUS "clang-tidy found but no source files in src/ directory yet")
    endif()
endif()

# Custom targets for running clang-format
find_program(CLANG_FORMAT_EXE NAMES clang-format-20 clang-format)
if(CLANG_FORMAT_EXE)
    # Collect all source files
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
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
        # Create no-op targets when no source files exist
        add_custom_target(format
            COMMAND ${CMAKE_COMMAND} -E echo "No source files to format"
            COMMENT "clang-format: no source files found"
        )

        add_custom_target(format-check
            COMMAND ${CMAKE_COMMAND} -E echo "No source files to check"
            COMMENT "clang-format: no source files found"
        )

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

    # Example stress test structure (uncomment when stress tests are added):
    # add_executable(stress_test_example
    #     tests/stress_test_example.cpp
    # )
    # target_include_directories(stress_test_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    # target_link_libraries(stress_test_example PRIVATE GTest::gtest_main pthread)
    # if(TSAN_FLAGS)
    #     target_compile_options(stress_test_example PRIVATE ${TSAN_FLAGS})
    #     target_link_options(stress_test_example PRIVATE ${TSAN_FLAGS})
    # endif()
    # set_target_properties(stress_test_example PROPERTIES
    #     CXX_CLANG_TIDY ""
    # )
    # add_test(NAME stress_example COMMAND stress_test_example)
endif()

# ============================================================================
# Examples (embedded targets only)
# ============================================================================

# Examples are only built for embedded targets
# Native targets use test versions in tests/examples/
# Add the examples directory
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

# Standalone test for array utilities
add_executable(run_array_utils_test
    run_array_utils_test.cpp
)

target_include_directories(run_array_utils_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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
