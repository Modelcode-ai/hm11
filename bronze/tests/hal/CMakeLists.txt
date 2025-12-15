# HAL tests

# Create test executable for UART tests
add_hm11_test(uart_test
    uart_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

# Create test executable for HAL types tests
add_hm11_test(hal_types_test
    hal_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)