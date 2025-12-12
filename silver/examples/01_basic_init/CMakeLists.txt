cmake_minimum_required(VERSION 3.20)

# Basic initialization example demonstrating HM11 driver usage patterns
# Equivalent to the core functionality in Ada's hm11_example.adb

add_executable(01_basic_init main.cpp)

# Link against driver library with proper dependencies
target_link_libraries(01_basic_init 
    PRIVATE 
        hm11::driver
        hm11::hal
        hm11::types
)

# Include directories for header access
target_include_directories(01_basic_init 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

# Set C++ standard for this example
target_compile_features(01_basic_init PRIVATE cxx_std_20)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(01_basic_init PRIVATE -O2 -g)
endif()

# Install example (optional)
if(NOT IS_EMBEDDED_TARGET)
    install(TARGETS 01_basic_init
        RUNTIME DESTINATION examples
    )
endif()