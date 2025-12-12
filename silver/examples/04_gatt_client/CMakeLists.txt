# HM11 GATT Client Example - Discovery to Connection Workflow

# Create the example executable
add_executable(04_gatt_client main.cpp ../../src/discovery.cpp)

# Link required libraries
target_link_libraries(04_gatt_client
    PRIVATE
        hm11::driver
        hm11::hal
        hm11::types
)

# Set target properties
set_target_properties(04_gatt_client PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Enable compiler warnings
target_compile_options(04_gatt_client PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Add clang-tidy if enabled
if(ENABLE_CLANG_TIDY AND CMAKE_CXX_CLANG_TIDY)
    set_target_properties(04_gatt_client PROPERTIES
        CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
    )
endif()

# Create an integration test for the example
if(BUILD_TESTS)
    add_test(NAME gatt_client_example_test
        COMMAND 04_gatt_client
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(gatt_client_example_test PROPERTIES
        LABELS "example;integration"
        TIMEOUT 30
    )
endif()