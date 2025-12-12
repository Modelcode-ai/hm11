#pragma once

// HM11 Platform Abstraction Layer - Main Platform Header
// Unified access to all platform-specific functionality
// Supports ARM Cortex-M, Linux x86, and other platforms

#include "hm11/platform/dma_cache.hpp"
#include "hm11/platform/timing.hpp"
#include "hm11/platform/wait_event.hpp"

namespace hm11::platform {

/// Platform detection and feature availability
namespace Platform {

/// Platform identification
enum class Type {
    ARM_CORTEX_M4, ///< ARM Cortex-M4 (STM32F4xx series)
    ARM_CORTEX_M7, ///< ARM Cortex-M7 (STM32F7xx/H7xx series)
    LINUX_X86_64,  ///< Linux x86_64 (development/testing)
    OTHER          ///< Other/unknown platform
};

/// Get current platform type at compile time
constexpr Type GetPlatformType() noexcept {
#if defined(STM32F7xx) || defined(STM32H7xx)
    return Type::ARM_CORTEX_M7;
#elif defined(STM32F4xx)
    return Type::ARM_CORTEX_M4;
#elif defined(__linux__) && defined(__x86_64__)
    return Type::LINUX_X86_64;
#else
    return Type::OTHER;
#endif
}

/// Check if platform has data cache
constexpr bool HasDataCache() noexcept {
    constexpr auto platform = GetPlatformType();
    return platform == Type::ARM_CORTEX_M7;
}

/// Check if platform supports WFE/SEV instructions
constexpr bool HasWaitForEvent() noexcept {
    constexpr auto platform = GetPlatformType();
    return platform == Type::ARM_CORTEX_M4 || platform == Type::ARM_CORTEX_M7;
}

/// Check if platform supports futex operations (Linux)
constexpr bool HasFutex() noexcept {
    return GetPlatformType() == Type::LINUX_X86_64;
}

/// Check if we're building for embedded target
constexpr bool IsEmbedded() noexcept {
    constexpr auto platform = GetPlatformType();
    return platform == Type::ARM_CORTEX_M4 || platform == Type::ARM_CORTEX_M7;
}

/// Get platform name as string (for debugging/logging)
constexpr const char* GetPlatformName() noexcept {
    switch (GetPlatformType()) {
        case Type::ARM_CORTEX_M4:
            return "ARM Cortex-M4";
        case Type::ARM_CORTEX_M7:
            return "ARM Cortex-M7";
        case Type::LINUX_X86_64:
            return "Linux x86_64";
        case Type::OTHER:
        default:
            return "Other/Unknown";
    }
}

} // namespace Platform

/// Memory barrier operations for different platforms
namespace MemoryBarrier {

/// Data memory barrier (ensures memory operations complete before proceeding)
/// Maps to appropriate barrier for each platform
inline void DataMemoryBarrier() noexcept {
#if defined(__arm__) || defined(__aarch64__)
    // ARM: Use data memory barrier instruction with "sy" (system) scope
    __asm__ volatile("dmb sy" ::: "memory");
#else
    // Other platforms: Compiler memory barrier
    __asm__ volatile("" ::: "memory");
#endif
}

/// Data synchronization barrier (stronger than DMB)
/// Ensures all memory operations complete and all cache operations complete
inline void DataSynchronizationBarrier() noexcept {
#if defined(__arm__) || defined(__aarch64__)
    // ARM: Use data synchronization barrier instruction with "sy" (system) scope
    __asm__ volatile("dsb sy" ::: "memory");
#else
    // Other platforms: Compiler memory barrier
    __asm__ volatile("" ::: "memory");
#endif
}

/// Instruction synchronization barrier
/// Ensures instruction pipeline is flushed after memory mapping changes
inline void InstructionSynchronizationBarrier() noexcept {
#if defined(__arm__) || defined(__aarch64__)
    // ARM: Use instruction synchronization barrier
    __asm__ volatile("isb" ::: "memory");
#else
    // Other platforms: Compiler memory barrier
    __asm__ volatile("" ::: "memory");
#endif
}

} // namespace MemoryBarrier

/// Platform initialization and configuration
namespace Initialize {

/// Initialize platform-specific features
/// Should be called once at startup before using platform features
/// Currently a no-op but reserved for future platform setup
inline void PlatformFeatures() noexcept {
    // Future platform initialization code goes here
    // Examples:
    // - Enable instruction/data caches on Cortex-M7
    // - Configure memory protection unit (MPU)
    // - Set up platform-specific clock sources
    // - Initialize debug interfaces
}

} // namespace Initialize

} // namespace hm11::platform