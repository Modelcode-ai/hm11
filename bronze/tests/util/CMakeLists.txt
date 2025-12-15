# Util tests

# Create a test executable for utility tests
add_hm11_test(util_tests
    status_test.cpp
    error_test.cpp
    utils_test.cpp
    array_utils_test.cpp
    circular_buffer_test.cpp
    validators_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

target_include_directories(util_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# util_tests will be automatically included in run_tests by the gtest_discover_tests function