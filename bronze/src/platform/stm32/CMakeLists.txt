# STM32 Platform Module

# This module is only built for STM32 embedded targets
if(NOT IS_EMBEDDED_TARGET)
    message(STATUS "Skipping STM32 platform module (native build)")
    return()
endif()

# Source files common to all STM32 families
set(STM32_COMMON_SOURCES
    device.cpp
    gpio_config.cpp
    uart.cpp
    dma.cpp
)

# Source files for STM32 with DMA receive handler
list(APPEND STM32_COMMON_SOURCES
    dma_receive_handler.cpp
)

# Source files for STM32 with hardware watchdog
list(APPEND STM32_COMMON_SOURCES
    watchdog.cpp
)

# Source files for STM32 interrupt handlers
list(APPEND STM32_COMMON_SOURCES
    interrupt_handlers.cpp
)

# Define the STM32 platform library
add_library(hm11_platform_stm32 STATIC
    ${STM32_COMMON_SOURCES}
)

# Library settings
target_include_directories(hm11_platform_stm32
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
    $<INSTALL_INTERFACE:include>
)

# Link with platform library
target_link_libraries(hm11_platform_stm32
    PUBLIC
    hm11_platform
)

# Ensure proper STM32 family is defined
if(DEFINED STM32F7)
    target_compile_definitions(hm11_platform_stm32 PUBLIC STM32F7)
elseif(DEFINED STM32H7)
    target_compile_definitions(hm11_platform_stm32 PUBLIC STM32H7)
endif()

# Family-specific configuration for cache handling
if(DEFINED STM32F7 OR DEFINED STM32H7)
    target_compile_definitions(hm11_platform_stm32 PUBLIC CACHE_MAINTENANCE_REQUIRED)
endif()

# For STM32 embedded targets, include CMSIS paths
if(DEFINED CMSIS_PATH AND EXISTS ${CMSIS_PATH})
    if(DEFINED STM32F7)
        target_include_directories(hm11_platform_stm32
            PRIVATE
            ${CMSIS_PATH}/Include
            ${CMSIS_PATH}/Device/ST/STM32F7xx/Include
        )
    elseif(DEFINED STM32H7)
        target_include_directories(hm11_platform_stm32
            PRIVATE
            ${CMSIS_PATH}/Include
            ${CMSIS_PATH}/Device/ST/STM32H7xx/Include
        )
    else()
        # Default to F4 if not specified
        target_include_directories(hm11_platform_stm32
            PRIVATE
            ${CMSIS_PATH}/Include
            ${CMSIS_PATH}/Device/ST/STM32F4xx/Include
        )
    endif()
endif()

# Installation
install(TARGETS hm11_platform_stm32
    EXPORT hm11-platform-stm32-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT hm11-platform-stm32-targets
    FILE hm11-platform-stm32-targets.cmake
    NAMESPACE hm11::
    DESTINATION lib/cmake/hm11
)

# Report setup
message(STATUS "Configured STM32 platform module")
message(STATUS "  Families supported: F4, F7, H7")
if(DEFINED STM32F7)
    message(STATUS "  Active family: STM32F7 (Cortex-M7 with FPU)")
elseif(DEFINED STM32H7)
    message(STATUS "  Active family: STM32H7 (Cortex-M7 with FPU)")
else()
    message(STATUS "  Active family: STM32F4 (Cortex-M4 with FPU)")
endif()