# HM11 Type System - Header-Only Library Configuration
# This file defines the header-only library target for the HM11 type system
# Per Decision #2: Strong Type System Translation with RangedInteger and FixedString templates

# Create header-only interface library for the HM11 type system
add_library(hm11_types INTERFACE)

# Define the public include directories
target_include_directories(hm11_types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Set C++20 standard requirement for the library
target_compile_features(hm11_types INTERFACE cxx_std_20)

# Add compiler-specific warnings for the library
target_compile_options(hm11_types INTERFACE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Define compile-time feature macros
target_compile_definitions(hm11_types INTERFACE
    # Enable constexpr validation in debug builds
    $<$<CONFIG:Debug>:HM11_ENABLE_CONSTEXPR_VALIDATION>
    # Enable range checks in safety-critical builds if requested
    $<$<BOOL:${ENABLE_RUNTIME_RANGE_CHECKS}>:HM11_ENABLE_RUNTIME_RANGE_CHECKS>
)

# Alias for easier linking
add_library(hm11::types ALIAS hm11_types)

# =============================================================================
# HM11 Hardware Abstraction Layer - Header-Only Library
# =============================================================================

# Create header-only interface library for the HM11 Hardware Abstraction Layer
add_library(hm11_hal INTERFACE)

# Define the public include directories
target_include_directories(hm11_hal
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Set C++20 standard requirement for the HAL library  
target_compile_features(hm11_hal INTERFACE cxx_std_20)

# HAL depends on the type system
target_link_libraries(hm11_hal INTERFACE hm11::types)

# Apply same compiler warnings as type system
target_compile_options(hm11_hal INTERFACE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Alias for easier linking
add_library(hm11::hal ALIAS hm11_hal)

# =============================================================================
# HM11 STM32 HAL Implementation - Static Library (For Embedded Targets)
# =============================================================================

# Create static library for STM32 HAL implementation (only for embedded targets)
if(IS_EMBEDDED_TARGET)
    add_library(hm11_stm32_hal STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/hal/stm32_uart_port.cpp
    )

    # Define the public include directories
    target_include_directories(hm11_stm32_hal
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
            $<INSTALL_INTERFACE:include>
    )

    # Set C++20 standard requirement
    target_compile_features(hm11_stm32_hal PUBLIC cxx_std_20)

    # STM32 HAL depends on sync primitives and platform abstraction
    target_link_libraries(hm11_stm32_hal PUBLIC hm11::sync hm11::hal)

    # Apply same compiler warnings
    target_compile_options(hm11_stm32_hal PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
            -Wunused -Woverloaded-virtual -Wnull-dereference
            -Wdouble-promotion -Wformat=2
        >
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
            -Wunused -Woverloaded-virtual -Wnull-dereference
            -Wdouble-promotion -Wformat=2
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /permissive-
        >
    )

    # Add CMSIS include directory for STM32 register access
    if(DEFINED CMSIS_PATH AND EXISTS ${CMSIS_PATH})
        target_include_directories(hm11_stm32_hal PRIVATE
            ${CMSIS_PATH}/Include
            ${CMSIS_PATH}/Device/ST/STM32F4xx/Include
        )
    else()
        # Use our minimal CMSIS headers
        target_include_directories(hm11_stm32_hal PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/CMSIS
        )
    endif()

    # Alias for easier linking
    add_library(hm11::stm32_hal ALIAS hm11_stm32_hal)

    message(STATUS "HM11 STM32 HAL library configured for embedded target")
    message(STATUS "  Use target: hm11::stm32_hal")
endif()

# =============================================================================
# HM11 Main Driver - Header-Only Library  
# =============================================================================

# Create header-only interface library for the main HM11 driver
add_library(hm11_driver INTERFACE)

# Define the public include directories
target_include_directories(hm11_driver
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Set C++20 standard requirement
target_compile_features(hm11_driver INTERFACE cxx_std_20)

# Driver depends on HAL and type system
target_link_libraries(hm11_driver INTERFACE hm11::hal hm11::types)

# Apply same compiler warnings
target_compile_options(hm11_driver INTERFACE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Alias for easier linking
add_library(hm11::driver ALIAS hm11_driver)

# =============================================================================
# HM11 Synchronization Primitives - Static Library
# =============================================================================

# Create static library for synchronization primitives (requires compiled sources)
add_library(hm11_sync STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/sync/binary_semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/sync/watchdog.cpp
)

# Define the public include directories
target_include_directories(hm11_sync
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Set C++20 standard requirement
target_compile_features(hm11_sync PUBLIC cxx_std_20)

# Sync library depends on platform abstraction and HAL types
target_link_libraries(hm11_sync PUBLIC hm11::platform hm11::hal)

# Link required system libraries for platform abstraction
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux: Need pthread for std::thread and atomic operations
    target_link_libraries(hm11_sync PRIVATE pthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    # Embedded: Platform-specific libraries would be linked here
    # For now, no additional libraries needed
endif()

# Apply same compiler warnings
target_compile_options(hm11_sync PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Define platform-specific compile definitions
target_compile_definitions(hm11_sync PRIVATE
    $<$<PLATFORM_ID:Linux>:HM11_PLATFORM_LINUX>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Generic>:HM11_PLATFORM_EMBEDDED>
)

# Alias for easier linking
add_library(hm11::sync ALIAS hm11_sync)

# =============================================================================
# Header file collections for installation
# =============================================================================

# Collect all header files for IDE support and installation
set(HM11_TYPE_HEADERS
    types.hpp
    types/fixed_string.hpp
    types/ranged_integer.hpp
    types/type_aliases.hpp
    types/validators.hpp
    util.hpp
    util/string_builder.hpp
    util/parsing.hpp
)

set(HM11_HAL_HEADERS
    hal.hpp
    hal/uart_types.hpp
    hal/uart_interface.hpp
    hal/mock_uart.hpp
    hal/stm32_uart_port.hpp
)

set(HM11_DRIVER_HEADERS
    hm11.hpp
    hm11_driver.hpp
)

set(HM11_SYNC_HEADERS
    sync.hpp
    sync/binary_semaphore.hpp
    sync/circular_buffer.hpp
    sync/circular_buffer_impl.hpp
    sync/watchdog.hpp
)

set(HM11_PLATFORM_HEADERS
    platform/wait_event.hpp
    platform/dma_cache.hpp
    platform/timing.hpp
    platform/platform.hpp
)

# Installation rules (for future package distribution)
if(NOT IS_EMBEDDED_TARGET)
    # Install all library targets (platform library installed from parent CMakeLists.txt)
    install(TARGETS hm11_types hm11_hal hm11_driver hm11_sync
        EXPORT hm11Targets
        INCLUDES DESTINATION include
    )
    
    # Install type system headers
    install(FILES ${HM11_TYPE_HEADERS}
        DESTINATION include/hm11
    )
    
    install(DIRECTORY types/
        DESTINATION include/hm11/types
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # Install util headers
    install(DIRECTORY util/
        DESTINATION include/hm11/util
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # Install HAL headers
    install(FILES ${HM11_HAL_HEADERS}
        DESTINATION include/hm11
    )
    
    install(DIRECTORY hal/
        DESTINATION include/hm11/hal
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # Install driver headers
    install(FILES ${HM11_DRIVER_HEADERS}
        DESTINATION include/hm11
    )
    
    # Install sync headers
    install(FILES ${HM11_SYNC_HEADERS}
        DESTINATION include/hm11
    )
    
    install(DIRECTORY sync/
        DESTINATION include/hm11/sync
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # Install platform headers  
    install(FILES ${HM11_PLATFORM_HEADERS}
        DESTINATION include/hm11
    )
    
    install(DIRECTORY platform/
        DESTINATION include/hm11/platform
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # Install CMake configuration
    install(EXPORT hm11Targets
        FILE hm11Targets.cmake
        NAMESPACE hm11::
        DESTINATION lib/cmake/hm11
    )
endif()

# Documentation for the library targets
set_target_properties(hm11_types PROPERTIES
    INTERFACE_DESCRIPTION "HM11 Driver Type System - Header-only library providing RangedInteger and FixedString templates"
    INTERFACE_URL "https://github.com/your-repo/hm11-driver"
)

set_target_properties(hm11_hal PROPERTIES
    INTERFACE_DESCRIPTION "HM11 Hardware Abstraction Layer - UART interface with C++20 concepts"
    INTERFACE_URL "https://github.com/your-repo/hm11-driver"
)

set_target_properties(hm11_driver PROPERTIES
    INTERFACE_DESCRIPTION "HM11 BLE Driver - Template-based driver with dependency injection"
    INTERFACE_URL "https://github.com/your-repo/hm11-driver"
)

message(STATUS "HM11 Type System library configured as header-only interface")
message(STATUS "  Headers: ${HM11_TYPE_HEADERS}")
message(STATUS "  Use target: hm11::types")
message(STATUS "HM11 HAL library configured as header-only interface")
message(STATUS "  Headers: ${HM11_HAL_HEADERS}")  
message(STATUS "  Use target: hm11::hal")
message(STATUS "HM11 Driver library configured as header-only interface")
message(STATUS "  Headers: ${HM11_DRIVER_HEADERS}")
message(STATUS "  Use target: hm11::driver")