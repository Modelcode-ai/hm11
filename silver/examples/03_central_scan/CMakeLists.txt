# HM11 Central Device Discovery Example

# Create the example executable
add_executable(03_central_scan main.cpp)

# Link required libraries
target_link_libraries(03_central_scan
    PRIVATE
        hm11::driver
        hm11::hal
        hm11::types
)

# Set target properties
set_target_properties(03_central_scan PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Enable compiler warnings
target_compile_options(03_central_scan PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Add clang-tidy if enabled
if(ENABLE_CLANG_TIDY AND CMAKE_CXX_CLANG_TIDY)
    set_target_properties(03_central_scan PROPERTIES
        CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
    )
endif()

# Create an integration test for the example
if(BUILD_TESTS)
    add_test(NAME central_scan_example_test
        COMMAND 03_central_scan
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(central_scan_example_test PROPERTIES
        LABELS "example;integration"
        TIMEOUT 30
    )
endif()