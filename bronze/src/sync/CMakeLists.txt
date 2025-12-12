# Sync module source directory

# Define source files
set(SYNC_SOURCES
    binary_semaphore.cpp
    watchdog.cpp
)

# Sync module (now a static library with implementation files)
add_library(hm11_sync STATIC
    ${SYNC_SOURCES}
)

target_include_directories(hm11_sync
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

# Link against the platform library for platform-specific features
target_link_libraries(hm11_sync
    PUBLIC
    hm11_platform
)

# Installation rules
install(TARGETS hm11_sync
    EXPORT hm11-sync-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT hm11-sync-targets
    FILE hm11-sync-targets.cmake
    NAMESPACE hm11::
    DESTINATION lib/cmake/hm11
)