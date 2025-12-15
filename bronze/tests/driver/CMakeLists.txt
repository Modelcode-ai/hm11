# Driver tests
# This directory contains unit tests for the HM11Driver class

# Add driver test executable
add_hm11_test(driver_test 
    driver_test.cpp
    driver_constructor_test.cpp
    driver_basic_ops_test.cpp
    driver_getset_test.cpp
    ../mocks/hm11_driver_mock.cpp
    ../mocks/hm11_driver_test_stubs.cpp
)

# Add driver mock test executable
add_hm11_test(driver_mock_test
    driver_mock_test.cpp
    ../mocks/hm11_driver_mock.cpp
    ../mocks/hm11_driver_test_stubs.cpp
)

# Add command builder test executable
add_hm11_test(at_command_test
    at_command_test.cpp
)

# Add response parser test executable
add_hm11_test(response_parser_test
    response_parser_test.cpp
)

# Add connection management test executable
add_hm11_test(driver_connection_test
    driver_connection_test.cpp
    ../mocks/hm11_driver_mock.cpp
    ../mocks/hm11_driver_test_stubs.cpp
)

# Add DiscoveredInfo test executable
add_hm11_test(discovered_info_test
    discovered_info_test.cpp
)

# Instead of linking to libraries, add the source files directly
target_sources(driver_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/util/circular_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/cache.cpp
)

target_sources(driver_mock_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/util/circular_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/cache.cpp
)

target_sources(at_command_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
)

target_sources(response_parser_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
)

target_sources(driver_connection_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/at_command_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/util/response_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/at_response_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/util/circular_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/cache.cpp
)