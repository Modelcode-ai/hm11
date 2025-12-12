# Main source directory for HM11 driver

# Add subdirectories for each module
add_subdirectory(hal)
add_subdirectory(platform)
add_subdirectory(sync)
add_subdirectory(util)

# Create the main HM11 driver library
add_library(hm11
    hm11_driver.cpp
    device_settings_fixed.cpp
    enum_types.cpp
    enum_parsers.cpp
    advertising_data.cpp
)

# Disable clang-tidy for these files due to crashes or linting issues
set_source_files_properties(
    hm11_driver.cpp
    device_settings_fixed.cpp
    advertising_data.cpp
    PROPERTIES SKIP_LINTING TRUE
)

target_include_directories(hm11 
    PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

# Add any dependencies as needed
target_link_libraries(hm11 
    PUBLIC
    hm11_hal
    hm11_platform
    hm11_sync
    hm11_util
)

# Create an alias target for easier linking in examples and tests
add_library(hm11_driver ALIAS hm11)

# Installation rules
install(TARGETS hm11
    EXPORT hm11-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT hm11-targets
    FILE hm11-targets.cmake
    NAMESPACE hm11::
    DESTINATION lib/cmake/hm11
)