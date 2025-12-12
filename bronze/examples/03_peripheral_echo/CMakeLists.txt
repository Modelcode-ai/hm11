# CMakeLists.txt for the peripheral echo example application

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.20)

# Add the executable for the peripheral echo example
add_executable(hm11_peripheral_echo
    peripheral_echo.cpp
    hm11_driver_mock.cpp
)

# Set C++ standard to C++20
set_target_properties(hm11_peripheral_echo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # Disable clang-tidy for this example since it's for demonstration purposes
    CXX_CLANG_TIDY ""
)

# We don't need to link against the HM11 driver library
# since we're providing our own mock implementation
#target_link_libraries(hm11_peripheral_echo
#    PRIVATE
#    hm11
#)

# Add the include directories
target_include_directories(hm11_peripheral_echo
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)