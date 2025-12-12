#pragma once

// HM11 Synchronization Primitives - Main Header
// ISR-safe synchronization primitives for embedded HM11 driver
// Replaces Ada's protected objects with C++ atomic operations and platform wait primitives

/**
 * @file sync.hpp
 * @brief ISR-safe synchronization primitives for HM11 driver
 *
 * This module provides thread-safe, ISR-safe synchronization primitives that replace
 * Ada's protected objects with equivalent C++ implementations using atomic operations
 * and platform-specific wait/signal mechanisms.
 *
 * ## Key Design Principles
 *
 * ### Memory Ordering Strategy
 * - **ISR Context (Producer)**: Uses std::memory_order_release for all signaling operations
 *   to ensure visibility of data writes before signaling completion
 * - **Thread Context (Consumer)**: Uses std::memory_order_acquire for all wait operations
 *   to ensure visibility of ISR writes after being signaled
 * - **State Clearing**: Uses std::memory_order_relaxed for single-consumer clearing operations
 *
 * ### ISR Safety Requirements
 * - All ISR-callable functions are marked with "FromISR" suffix
 * - Minimal latency in ISR context (<10µs typical, <50µs worst-case)
 * - No dynamic memory allocation in any ISR path
 * - No blocking operations in ISR context
 * - Exception-safe (noexcept) for all ISR operations
 *
 * ### Platform Abstraction
 * - ARM Cortex-M: WFE/SEV instructions for µA-level power consumption
 * - Linux x86: futex system calls for development/testing
 * - Fallback: yield-based spinning for broad compatibility
 *
 * ## Usage Patterns
 *
 * ### Basic Event Notification (BinarySemaphore)
 * ```cpp
 * hm11::sync::BinarySemaphore completion;
 *
 * // Thread context - wait for event
 * auto status = completion.TakeWithTimeout(std::chrono::milliseconds(1000));
 *
 * // ISR context - signal event completion
 * completion.GiveFromISR(); // or GiveFromISRWithStatus(error_code)
 * ```
 *
 * ### Streaming Data Buffer (CircularBuffer)
 * ```cpp
 * hm11::sync::UARTCircularBuffer uart_buffer;
 *
 * // ISR context - write received data
 * uart_buffer.WriteFromISR(received_byte);
 *
 * // Thread context - read available data
 * auto data = uart_buffer.ReadAvailable();
 * // Process data...
 * uart_buffer.MarkConsumed(data.size());
 * ```
 *
 * ### Timeout Management (Watchdog)
 * ```cpp
 * hm11::sync::Watchdog operation_timer;
 *
 * // Thread context - start operation with timeout
 * operation_timer.Start(std::chrono::milliseconds(1000));
 * auto result = operation_timer.AwaitCompletion();
 *
 * // ISR context - check for timeout during other interrupts
 * if (operation_timer.CheckTimeoutFromISR()) {
 *     // Timeout occurred, operation cancelled
 * }
 * ```
 *
 * ## Memory Layout Considerations
 *
 * All buffer types use 32-byte alignment for DMA compatibility and cache coherency
 * on ARM Cortex-M7 processors. This ensures optimal performance for UART DMA operations
 * and prevents cache line conflicts.
 *
 * ## Thread Safety Guarantees
 *
 * - **BinarySemaphore**: Single producer (ISR), single consumer (thread)
 * - **CircularBuffer**: Single producer (ISR), single consumer (thread)
 * - **Watchdog**: ISR and thread coordination with atomic operations
 *
 * All classes follow the non-copyable, non-movable ownership model to prevent
 * resource duplication and maintain clear lifetime semantics.
 */

// Core synchronization primitives
#include "sync/binary_semaphore.hpp"
#include "sync/circular_buffer.hpp"
#include "sync/watchdog.hpp"

// Platform abstraction layer
#include "platform/wait_event.hpp"

// Note: Namespace convenience aliases are not needed since we directly use hm11::sync
// The sync and platform namespaces are already available within hm11

namespace hm11::sync {

/// Error propagation utilities for ISR context
/// Provides standardized patterns for error handling matching Ada's exception propagation

/// Convert platform-specific error codes to UART status
/// Used by platform abstraction layer to normalize error reporting
[[nodiscard]] constexpr hal::UARTStatus PlatformErrorToStatus(int platform_error) noexcept {
    // Platform-specific error code mapping
    // Implementation depends on target platform
    if (platform_error == 0) {
        return hal::UARTStatus::Ok;
    } else {
        return hal::UARTStatus::ErrError; // Generic error for unrecognized codes
    }
}

/// ISR-safe assertion for debug builds
/// Provides runtime verification without exception throwing
/// In release builds, compiles to no-op for zero overhead
#if defined(NDEBUG)
#define HM11_ISR_ASSERT(condition) ((void)0)
#else
#define HM11_ISR_ASSERT(condition)                                                                                     \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            /* In embedded context, would trigger debugger breakpoint */                                               \
            /* For now, use compiler trap */                                                                           \
            __builtin_trap();                                                                                          \
        }                                                                                                              \
    } while (0)
#endif

} // namespace hm11::sync