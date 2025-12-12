# CMakeLists.txt for the configuration example application

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.20)

# Add the executable for the configuration example
add_executable(hm11_configuration_example
    configuration_example.cpp
    hm11_driver_mock.cpp
)

# Set C++ standard to C++20
set_target_properties(hm11_configuration_example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # Disable clang-tidy for this example since it's for demonstration purposes
    CXX_CLANG_TIDY ""
)

# We don't need to link against the HM11 driver library
# since we're providing our own mock implementation
#target_link_libraries(hm11_configuration_example
#    PRIVATE
#    hm11
#)

# Add the include directories
target_include_directories(hm11_configuration_example
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)