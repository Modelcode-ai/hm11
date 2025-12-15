# Tests for the HM11 driver
# Requires GTest to be installed or available via find_package()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Find required packages
find_package(GTest REQUIRED)
include(GoogleTest)

# Common test settings
set(TEST_COMPILE_DEFINITIONS "")
set(TEST_COMPILE_OPTIONS "")
set(TEST_LINK_OPTIONS "")

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND TEST_COMPILE_OPTIONS --coverage -fprofile-arcs -ftest-coverage)
        list(APPEND TEST_LINK_OPTIONS --coverage)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND TEST_COMPILE_OPTIONS -fprofile-instr-generate -fcoverage-mapping)
        list(APPEND TEST_LINK_OPTIONS -fprofile-instr-generate -fcoverage-mapping)
    endif()
endif()

# Function to add a test executable with common settings
function(add_hm11_test name)
    add_executable(${name} ${ARGN})
    
    target_compile_definitions(${name} PRIVATE ${TEST_COMPILE_DEFINITIONS})
    target_compile_options(${name} PRIVATE ${TEST_COMPILE_OPTIONS})
    target_link_options(${name} PRIVATE ${TEST_LINK_OPTIONS})
    
# Link with Google Test and GMock
target_link_libraries(${name} PRIVATE 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
)
    
    # Register the test with CTest
    gtest_discover_tests(${name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES TIMEOUT 10
    )
    
    # Disable clang-tidy for tests as it can be too strict
    set_target_properties(${name} PROPERTIES 
        CXX_CLANG_TIDY ""
    )
endfunction()

# Minimal test setup to verify Google Test integration
add_executable(minimal_test minimal_test.cpp)
target_link_libraries(minimal_test PRIVATE GTest::gtest GTest::gtest_main)
add_test(NAME minimal_test COMMAND minimal_test)

# Add subdirectories for module tests - uncomment as they are implemented
add_subdirectory(hal)
add_subdirectory(platform)
add_subdirectory(sync)
add_subdirectory(util)
add_subdirectory(integration)
add_subdirectory(driver)

# Create example test
add_hm11_test(example_test example_test.cpp)

# Add target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS minimal_test example_test driver_test driver_mock_test at_command_test response_parser_test binary_semaphore_test watchdog_test
    COMMENT "Running all tests"
    USES_TERMINAL
)

# Add an alias target (use test-runner instead of test as name to avoid conflict with CTest)
add_custom_target(test-runner
    DEPENDS run_tests
)

# Message about available targets
message(STATUS "Test targets added:")
message(STATUS "  'make example_test' - Build the example test")
message(STATUS "  'make run_tests' - Run all tests")