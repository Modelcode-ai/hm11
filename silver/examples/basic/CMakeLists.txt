# CMakeLists.txt for basic type system example
# This example demonstrates the core type system templates from Task 1

# Require the basic example executable
add_executable(basic_example main.cpp)

# Link against the header-only type system library
target_link_libraries(basic_example 
    PRIVATE 
        hm11::types
)

# Set target properties
set_target_properties(basic_example PROPERTIES
    # Output to examples directory for organization
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    # Enable compiler warnings
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Platform-specific configuration
if(IS_EMBEDDED_TARGET)
    # For embedded targets, this example serves as validation
    # that the type system compiles for the target platform
    message(STATUS "Building basic example for embedded target: ${CMAKE_SYSTEM_PROCESSOR}")
    
    # Add embedded-specific compile definitions if needed
    target_compile_definitions(basic_example PRIVATE EMBEDDED_TARGET=1)
else()
    # For native targets, this is a full demonstration application
    message(STATUS "Building basic example for native target")
    
    # Add native-specific compile definitions if needed
    target_compile_definitions(basic_example PRIVATE NATIVE_TARGET=1)
endif()

# Add example-specific compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(basic_example PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

# Create a test target for the example (runs it to verify it doesn't crash)
if(NOT IS_EMBEDDED_TARGET)
    add_test(
        NAME basic_example_test
        COMMAND basic_example
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    
    # Set test properties
    set_tests_properties(basic_example_test PROPERTIES
        LABELS "example;integration"
        TIMEOUT 30
    )
endif()

# Documentation
message(STATUS "Basic example configured:")
message(STATUS "  - Demonstrates RangedInteger and FixedString templates")
message(STATUS "  - Validates type safety and Ada mapping")
message(STATUS "  - Tests compile-time and runtime validation")
message(STATUS "  - Measures performance characteristics")
if(NOT IS_EMBEDDED_TARGET)
    message(STATUS "  - Includes integration test (basic_example_test)")
endif()