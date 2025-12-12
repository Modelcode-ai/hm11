# HAL module source directory

# HAL module (header-only for now)
add_library(hm11_hal INTERFACE)

target_include_directories(hm11_hal
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

# Installation rules
install(TARGETS hm11_hal
    EXPORT hm11-hal-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT hm11-hal-targets
    FILE hm11-hal-targets.cmake
    NAMESPACE hm11::
    DESTINATION lib/cmake/hm11
)