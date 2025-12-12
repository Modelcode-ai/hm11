# Integration tests for HM11 driver components

# Create test executables for integration tests
add_hm11_test(hal_types_integration 
    hal_types_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

add_hm11_test(error_handling_integration 
    error_handling_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

add_hm11_test(memory_alignment_integration 
    memory_alignment_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

add_hm11_test(basic_driver_integration 
    basic_driver_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

add_hm11_test(dummy_test 
    dummy_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

add_hm11_test(driver_integration_test 
    driver_integration_test.cpp
    ../mocks/hm11_driver_mock.cpp
    ../mocks/hm11_driver_test_stubs.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

# Add dependencies to include directories
target_include_directories(hal_types_integration
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_include_directories(error_handling_integration
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_include_directories(memory_alignment_integration
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_include_directories(basic_driver_integration
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_include_directories(dummy_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_include_directories(driver_integration_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

# Instead of linking to libraries, add the source files directly
target_sources(driver_integration_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/util/circular_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/cache.cpp
)

# All tests will be automatically included in run_tests by the gtest_discover_tests function