# CMake toolchain file for bare-metal ARM Cortex-M (arm-none-eabi GCC).
# Set ARM_TOOLCHAIN_PATH (env or -D) to the toolchain's bin/ directory if it is not on PATH.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

if(NOT DEFINED ARM_TOOLCHAIN_PATH AND DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(ARM_TOOLCHAIN_PATH "$ENV{ARM_TOOLCHAIN_PATH}")
endif()
if(DEFINED ARM_TOOLCHAIN_PATH)
    set(_ARM_PREFIX "${ARM_TOOLCHAIN_PATH}/arm-none-eabi-")
else()
    set(_ARM_PREFIX "arm-none-eabi-")
endif()

set(CMAKE_C_COMPILER   "${_ARM_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${_ARM_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${_ARM_PREFIX}gcc")
set(CMAKE_OBJCOPY      "${_ARM_PREFIX}objcopy" CACHE FILEPATH "objcopy")
set(CMAKE_OBJDUMP      "${_ARM_PREFIX}objdump" CACHE FILEPATH "objdump")
set(CMAKE_SIZE         "${_ARM_PREFIX}size"    CACHE FILEPATH "size")

# Compiler checks can't link a bare-metal executable without startup code and a linker script.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search host for programs only; libraries and headers come from the target sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
