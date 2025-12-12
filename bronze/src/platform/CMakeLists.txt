# Platform module source directory

# Define platform-specific preprocessor definitions
if(IS_EMBEDDED_TARGET)
    add_compile_definitions(HM11_TARGET_ARM_CORTEX_M)
else()
    add_compile_definitions(HM11_TARGET_LINUX)
endif()

# Define source files based on target platform
set(PLATFORM_SOURCES
    cache.cpp
)

# Add STM32-specific sources for embedded targets
if(IS_EMBEDDED_TARGET)
    list(APPEND PLATFORM_SOURCES
        stm32/dma.cpp
        stm32/device.cpp
        stm32/gpio_config.cpp
        stm32/uart.cpp
    )
endif()

# Add platform-specific WaitEvent implementation
if(IS_EMBEDDED_TARGET)
    list(APPEND PLATFORM_SOURCES wait_event_arm.cpp)
else()
    list(APPEND PLATFORM_SOURCES wait_event_linux.cpp)
endif()

# Platform module
add_library(hm11_platform STATIC
    ${PLATFORM_SOURCES}
)

target_include_directories(hm11_platform
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

# Conditionally compile STM32-specific code
# These flags will be set by the platform-specific CMake presets
if(DEFINED STM32F7)
    target_compile_definitions(hm11_platform PUBLIC STM32F7)
elseif(DEFINED STM32H7)
    target_compile_definitions(hm11_platform PUBLIC STM32H7)
endif()

# For embedded targets, add CMSIS include paths
if(IS_EMBEDDED_TARGET AND DEFINED CMSIS_PATH)
    target_include_directories(hm11_platform
        PRIVATE
        ${CMSIS_PATH}/Include
        ${CMSIS_PATH}/Device/ST/STM32F4xx/Include
    )
endif()

# For Linux targets, add threading support
if(NOT IS_EMBEDDED_TARGET)
    find_package(Threads REQUIRED)
    target_link_libraries(hm11_platform PUBLIC Threads::Threads)
endif()

# Installation rules
install(TARGETS hm11_platform
    EXPORT hm11-platform-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT hm11-platform-targets
    FILE hm11-platform-targets.cmake
    NAMESPACE hm11::
    DESTINATION lib/cmake/hm11
)