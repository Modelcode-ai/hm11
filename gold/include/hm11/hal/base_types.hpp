/**
 * @file base_types.hpp
 * @brief Hardware Abstraction Layer (HAL) base type definitions
 *
 * This header provides fundamental unsigned integer types and array type
 * aliases used throughout the HM11 driver for type-safe hardware register
 * manipulation and buffer operations.
 *
 * These types correspond to the Ada HAL package (hal.ads) from the
 * Ada_Drivers_Library, providing a consistent interface for low-level
 * hardware access.
 *
 * ## Design Rationale
 *
 * While Ada's HAL defines 64 different bit-width unsigned integer types
 * (Bit, UInt2, UInt3, ..., UInt64), this C++ implementation focuses on
 * the commonly used byte-aligned types (UInt8, UInt16, UInt32, UInt64).
 * Additional bit-width types can be added as needed for specific register
 * field manipulation.
 *
 * ## Usage Patterns
 *
 * Users can choose from three usage patterns:
 *
 * 1. Fully qualified names (verbose but explicit):
 *    @code
 *    hm11::hal::UInt8 data = 0x42;
 *    @endcode
 *
 * 2. Using declarations in function scope (concise and safe):
 *    @code
 *    void process() {
 *        using hm11::hal::UInt8;
 *        UInt8 data = 0x42;
 *    }
 *    @endcode
 *
 * 3. Using namespace in .cpp files only (never in headers):
 *    @code
 *    using namespace hm11::hal;
 *    UInt8 data = 0x42;
 *    @endcode
 *
 * ## Adding Additional Bit-Width Types
 *
 * If you need non-byte-aligned types for specific register fields (e.g.,
 * UInt9, UInt10, UInt12), you can add them using bit-field structs or
 * custom wrapper classes with appropriate bit masking. For example:
 *
 * @code
 * // Option 1: Using standard types with masking
 * constexpr UInt16 mask_to_9_bits(UInt16 value) {
 *     return value & 0x01FFU;  // 9-bit mask
 * }
 *
 * // Option 2: Strong typedef wrapper (more type-safe)
 * struct UInt9 {
 *     UInt16 value : 9;
 *     constexpr UInt9(UInt16 v) : value(v & 0x01FFU) {}
 *     constexpr operator UInt16() const { return value; }
 * };
 * @endcode
 *
 * For production use, consider a template-based approach for consistency
 * across different bit widths.
 *
 * ## Namespace Organization
 *
 * This header establishes the hm11::hal namespace hierarchy, which mirrors
 * the directory structure:
 *   - Directory: include/hm11/hal/
 *   - Namespace: hm11::hal
 *
 * This organization follows Decision 24 (Namespace Organization) from the
 * migration specification.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

/**
 * @namespace hm11
 * @brief Top-level namespace for HM11 Bluetooth LE driver
 */

/**
 * @namespace hm11::hal
 * @brief Hardware Abstraction Layer types and interfaces
 *
 * The hal namespace provides low-level type definitions for hardware register
 * access and buffer manipulation. These types ensure consistent sizing and
 * portable code across different platforms.
 */
namespace hm11::hal {

// ============================================================================
// Unsigned Integer Types (Byte-Aligned)
// ============================================================================

/**
 * @brief 8-bit unsigned integer type
 *
 * Corresponds to Ada's UInt8 (Interfaces.Unsigned_8).
 * Used for byte-level operations, register access, and buffer manipulation.
 *
 * Size: 1 byte, Range: [0, 255]
 */
using UInt8 = std::uint8_t;

/**
 * @brief 16-bit unsigned integer type
 *
 * Corresponds to Ada's UInt16 (Interfaces.Unsigned_16).
 * Used for 16-bit register access and multi-byte values.
 *
 * Size: 2 bytes, Range: [0, 65535]
 */
using UInt16 = std::uint16_t;

/**
 * @brief 32-bit unsigned integer type
 *
 * Corresponds to Ada's UInt32 (Interfaces.Unsigned_32).
 * Used for 32-bit register access, addresses, and larger values.
 *
 * Size: 4 bytes, Range: [0, 4294967295]
 */
using UInt32 = std::uint32_t;

/**
 * @brief 64-bit unsigned integer type
 *
 * Corresponds to Ada's UInt64 (Interfaces.Unsigned_64).
 * Used for 64-bit values, timestamps, and large counters.
 *
 * Size: 8 bytes, Range: [0, 18446744073709551615]
 */
using UInt64 = std::uint64_t;

// ============================================================================
// Array Types (Non-Owning Views)
// ============================================================================

/**
 * @brief Non-owning view over a sequence of UInt8 values
 *
 * Corresponds to Ada's UInt8_Array (unconstrained array).
 *
 * std::span provides a non-owning view similar to Ada's unconstrained array
 * parameters. It stores a pointer and size, enabling bounds-checked access
 * without ownership semantics.
 *
 * Usage notes:
 * - For function parameters that view existing data, use UInt8Array
 * - For owned storage, use std::vector<UInt8> or std::array<UInt8, N>
 * - For fixed-size buffers, use std::array<UInt8, N> directly
 *
 * @code
 * void process_buffer(UInt8Array data) {
 *     for (UInt8 byte : data) {
 *         // Process each byte
 *     }
 * }
 *
 * std::vector<UInt8> buffer = {0x01, 0x02, 0x03};
 * process_buffer(buffer);  // Implicit conversion to span
 *
 * std::array<UInt8, 3> fixed = {0xAA, 0xBB, 0xCC};
 * process_buffer(fixed);   // Implicit conversion to span
 * @endcode
 */
using UInt8Array = std::span<UInt8>;

/**
 * @brief Non-owning view over a constant sequence of UInt8 values
 *
 * Read-only variant of UInt8Array, corresponding to Ada's "in" mode parameter.
 *
 * @code
 * void read_buffer(UInt8ArrayConst data) {
 *     // Can read but not modify data
 * }
 * @endcode
 */
using UInt8ArrayConst = std::span<const UInt8>;

/**
 * @brief Non-owning view over a sequence of UInt16 values
 *
 * Corresponds to Ada's UInt16_Array (unconstrained array).
 *
 * @code
 * void process_words(UInt16Array data) {
 *     for (UInt16 word : data) {
 *         // Process each 16-bit word
 *     }
 * }
 * @endcode
 */
using UInt16Array = std::span<UInt16>;

/**
 * @brief Non-owning view over a constant sequence of UInt16 values
 *
 * Read-only variant of UInt16Array.
 */
using UInt16ArrayConst = std::span<const UInt16>;

/**
 * @brief Non-owning view over a sequence of UInt32 values
 *
 * Corresponds to Ada's UInt32_Array (unconstrained array).
 *
 * @code
 * void process_dwords(UInt32Array data) {
 *     for (UInt32 dword : data) {
 *         // Process each 32-bit double word
 *     }
 * }
 * @endcode
 */
using UInt32Array = std::span<UInt32>;

/**
 * @brief Non-owning view over a constant sequence of UInt32 values
 *
 * Read-only variant of UInt32Array.
 */
using UInt32ArrayConst = std::span<const UInt32>;

/**
 * @brief Non-owning view over a sequence of UInt64 values
 *
 * Not present in original Ada HAL but provided for completeness and
 * consistency with other array types.
 *
 * @code
 * void process_qwords(UInt64Array data) {
 *     for (UInt64 qword : data) {
 *         // Process each 64-bit quad word
 *     }
 * }
 * @endcode
 */
using UInt64Array = std::span<UInt64>;

/**
 * @brief Non-owning view over a constant sequence of UInt64 values
 *
 * Read-only variant of UInt64Array.
 */
using UInt64ArrayConst = std::span<const UInt64>;

// ============================================================================
// UART Status Enumeration
// ============================================================================

/**
 * @brief UART operation status codes
 *
 * Corresponds to Ada's HAL.UART.UART_Status enumeration type, which
 * represents the result of UART transmit and receive operations.
 *
 * These status codes are used to report operation completion, errors,
 * and timeouts in blocking UART operations throughout the driver.
 *
 * ## Usage in Watchdog Pattern
 *
 * The Watchdog timeout manager (Decision 26) uses std::atomic<UartStatus>
 * for error propagation from ISR to application context:
 * - ISR stores status with memory_order_release before signaling
 * - Application reads status with memory_order_acquire after waking
 *
 * @note This enum is designed to be used with std::atomic for lock-free
 *       status propagation in ISR-safe code
 */
enum class UartStatus : uint8_t {
    /**
     * @brief Operation completed successfully
     *
     * Indicates that the UART transmit or receive operation completed
     * without errors. All requested data was transferred successfully.
     */
    Ok = 0,

    /**
     * @brief UART hardware error occurred
     *
     * Indicates a hardware-level error during the operation, such as:
     * - Parity error
     * - Framing error
     * - Noise error
     * - Overrun error
     *
     * The specific error details may be available through platform-specific
     * error status registers.
     */
    ErrError = 1,

    /**
     * @brief Operation timed out
     *
     * Indicates that the operation did not complete within the specified
     * timeout period. This typically means:
     * - For receive: No data arrived within timeout
     * - For transmit: Hardware did not accept data within timeout
     *
     * The timeout mechanism is implemented using ISR-side deadline polling
     * (Decision 25), where the ISR checks Clock > Until_Time opportunistically.
     */
    ErrTimeout = 2,

    /**
     * @brief UART peripheral is busy
     *
     * Indicates that the UART peripheral is currently handling another
     * operation and cannot start a new one. This status is typically
     * returned when attempting to start a new operation while one is
     * already in progress.
     */
    Busy = 3,
};

// ============================================================================
// UART Data Size Enumeration
// ============================================================================

/**
 * @brief UART data size configuration
 *
 * Corresponds to Ada's HAL.UART.UART_Data_Size enumeration type, which
 * specifies the number of data bits used in UART transmission.
 *
 * The HM-11 Bluetooth module operates exclusively in 8-bit data mode
 * (factory default configuration). While this enum provides both 8-bit
 * and 9-bit options for completeness and compatibility with the Ada HAL
 * interface, only DataSize8b is used in the HM-11 driver implementation.
 *
 * ## Hardware Configuration
 *
 * The HM-11 hardware specification requires:
 * - 8 data bits (DataSize8b)
 * - 1 stop bit
 * - No parity
 * - Baud rates: 9600, 19200, 38400, 57600, or 115200 (default: 9600)
 *
 * The 9-bit mode (DataSize9b) is provided for interface completeness but
 * is not supported by HM-11 hardware. Attempting to configure 9-bit mode
 * will result in undefined behavior.
 *
 * ## Usage in UART Interface
 *
 * This enum would typically be used in a full UART abstraction layer to
 * configure the data width. In the HM-11 driver, the data size is fixed
 * at 8 bits and does not need runtime configuration.
 *
 * @note The 9-bit mode exists in the Ada HAL for compatibility with other
 *       UART peripherals but is not used in the HM-11 driver
 */
enum class UartDataSize : uint8_t {
    /**
     * @brief 8-bit data mode (standard)
     *
     * Corresponds to Ada's Data_Size_8b.
     * This is the standard UART data size used by HM-11 and most UART
     * devices. Each transmitted character consists of 8 data bits.
     */
    DataSize8b = 0,

    /**
     * @brief 9-bit data mode (extended)
     *
     * Corresponds to Ada's Data_Size_9b.
     * This mode is used by some UART peripherals for extended addressing
     * or parity schemes. Not supported by HM-11 hardware.
     *
     * @warning Not supported by HM-11 Bluetooth module
     */
    DataSize9b = 1,
};

// ============================================================================
// UART Data Buffer Type Aliases
// ============================================================================

/**
 * @brief Mutable view over UART 8-bit data buffer
 *
 * Corresponds to Ada's UART_Data_8b (array of UInt8).
 *
 * This type alias provides a semantic name for buffers used in UART
 * receive operations where data will be written by the hardware/driver.
 * It is semantically equivalent to UInt8Array but provides clearer intent
 * when used in UART-specific contexts.
 *
 * ## Buffer Ownership and Lifetime
 *
 * std::span is a non-owning view over contiguous data. The underlying
 * storage must be owned by the caller and remain valid for the duration
 * of the UART operation:
 *
 * @code
 * // Correct: buffer outlives the operation
 * std::array<UInt8, 128> rx_buffer;
 * UartData8b rx_view(rx_buffer);
 * uart.Receive(rx_view, status, timeout);
 * // rx_buffer contains received data
 *
 * // Incorrect: temporary buffer destroyed before operation completes
 * uart.Receive(std::array<UInt8, 128>{}, status, timeout); // DANGER!
 * @endcode
 *
 * ## DMA Alignment Requirements
 *
 * For DMA-based UART operations, caller-provided buffers may require
 * 32-byte alignment for cache coherency on Cortex-M7 platforms (Decision 20).
 * Internal driver buffers use alignas(32) for guaranteed alignment.
 * User-provided buffers should follow alignment guidelines documented in
 * the UART interface specification.
 *
 * @see UartData8bConst for read-only transmit buffers
 * @see UInt8Array for generic byte buffer views
 */
using UartData8b = std::span<UInt8>;

/**
 * @brief Read-only view over UART 8-bit data buffer
 *
 * Corresponds to Ada's UART_Data_8b used with "in" mode parameters.
 *
 * This type alias provides a semantic name for buffers used in UART
 * transmit operations where data will be read (but not modified) by
 * the hardware/driver.
 *
 * ## Usage Pattern
 *
 * Use this type for UART transmit operations where the driver only needs
 * to read the data without modifying it:
 *
 * @code
 * const std::array<UInt8, 5> tx_data = {'H', 'e', 'l', 'l', 'o'};
 * UartData8bConst tx_view(tx_data);
 * uart.Transmit(tx_view, status, timeout);
 * // tx_data remains unmodified
 * @endcode
 *
 * The const-qualified span prevents accidental modification and allows
 * passing const-qualified containers directly to UART transmit operations.
 *
 * @see UartData8b for mutable receive buffers
 * @see UInt8ArrayConst for generic read-only byte buffer views
 */
using UartData8bConst = std::span<const UInt8>;

/**
 * @brief Mutable view over UART 9-bit data buffer
 *
 * Corresponds to Ada's UART_Data_9b (array of UInt9).
 *
 * This type is provided for completeness to match the Ada HAL interface,
 * but is not used in the HM-11 driver since the HM-11 hardware only
 * supports 8-bit data mode.
 *
 * In a full UART abstraction supporting 9-bit mode, each UInt16 would
 * hold a 9-bit value (0-511 range) in its lower 9 bits.
 *
 * @warning Not used in HM-11 driver (HM-11 hardware is 8-bit only)
 * @see UartData8b for the 8-bit buffers used by HM-11
 */
using UartData9b = std::span<UInt16>;

/**
 * @brief Read-only view over UART 9-bit data buffer
 *
 * Corresponds to Ada's UART_Data_9b used with "in" mode parameters.
 *
 * This type is provided for completeness but is not used in the HM-11 driver.
 *
 * @warning Not used in HM-11 driver (HM-11 hardware is 8-bit only)
 * @see UartData8bConst for the 8-bit buffers used by HM-11
 */
using UartData9bConst = std::span<const UInt16>;

// ============================================================================
// Static Assertions for Type Safety and Atomic Compatibility
// ============================================================================

/**
 * @brief Verify UartStatus is compatible with std::atomic
 *
 * This static assertion ensures that UartStatus can be used with std::atomic
 * for lock-free ISR-to-thread status propagation (Decision 26, Decision 23).
 *
 * The assertion verifies that:
 * - UartStatus is trivially copyable (required for atomic operations)
 * - UartStatus has standard layout (ensures predictable memory representation)
 * - sizeof(UartStatus) matches underlying type (uint8_t), ensuring efficient atomic ops
 *
 * These properties guarantee that std::atomic<UartStatus> can be safely used
 * in ISR context with memory_order_release/acquire semantics for synchronization
 * between interrupt handlers and application threads.
 */
static_assert(std::is_trivially_copyable_v<UartStatus>, "UartStatus must be trivially copyable for atomic operations");
static_assert(std::is_standard_layout_v<UartStatus>, "UartStatus must have standard layout for atomic operations");
static_assert(
    sizeof(UartStatus) == sizeof(uint8_t),
    "UartStatus must be same size as underlying type for efficient atomics");

/**
 * @brief Verify UartDataSize is compatible with std::atomic
 *
 * While UartDataSize is not currently used with std::atomic in the driver,
 * this assertion ensures it has the same properties as UartStatus for
 * consistency and potential future use in configuration synchronization.
 */
static_assert(
    std::is_trivially_copyable_v<UartDataSize>,
    "UartDataSize must be trivially copyable for atomic operations");
static_assert(std::is_standard_layout_v<UartDataSize>, "UartDataSize must have standard layout for atomic operations");
static_assert(sizeof(UartDataSize) == sizeof(uint8_t), "UartDataSize must be same size as underlying type");

// ============================================================================
// Example Usage
// ============================================================================

/**
 * @example uart_types_example.cpp
 * Example usage of UART types for transmit and receive operations.
 *
 * @code
 * #include "hm11/hal/base_types.hpp"
 * #include <array>
 * #include <atomic>
 *
 * using namespace hm11::hal;
 *
 * // Example 1: UART transmit with read-only buffer
 * void transmit_example(UARTPort& uart) {
 *     // Data to transmit (const-qualified, safe from modification)
 *     const std::array<UInt8, 5> tx_data = {'H', 'e', 'l', 'l', 'o'};
 *
 *     // Create read-only view for transmit
 *     UartData8bConst tx_view(tx_data);
 *
 *     // Transmit data with timeout
 *     UartStatus status;
 *     // uart.Transmit(tx_view, status, 1000);
 *
 *     if (status == UartStatus::Ok) {
 *         // Success
 *     } else if (status == UartStatus::ErrTimeout) {
 *         // Timeout
 *     }
 * }
 *
 * // Example 2: UART receive with mutable buffer
 * void receive_example(UARTPort& uart) {
 *     // Buffer for received data (mutable for driver to write)
 *     std::array<UInt8, 128> rx_buffer;
 *
 *     // Create mutable view for receive
 *     UartData8b rx_view(rx_buffer);
 *
 *     // Receive data with timeout
 *     UartStatus status;
 *     // uart.Receive(rx_view, status, 5000);
 *
 *     if (status == UartStatus::Ok) {
 *         // Process received data in rx_buffer
 *         for (UInt8 byte : rx_buffer) {
 *             // Process byte
 *         }
 *     }
 * }
 *
 * // Example 3: Atomic status for ISR-to-thread communication
 * void isr_communication_example() {
 *     // Status variable shared between ISR and application thread
 *     std::atomic<UartStatus> uart_status{UartStatus::Busy};
 *
 *     // In ISR context (after DMA completion):
 *     // uart_status.store(UartStatus::Ok, std::memory_order_release);
 *
 *     // In application thread (after waking from wait):
 *     // UartStatus status = uart_status.load(std::memory_order_acquire);
 *     // if (status == UartStatus::Ok) { ... }
 * }
 *
 * // Example 4: Using with different container types
 * void container_flexibility_example(UARTPort& uart) {
 *     UartStatus status;
 *
 *     // With std::array
 *     std::array<UInt8, 64> array_buffer;
 *     // uart.Receive(array_buffer, status, 1000);  // Implicit conversion to span
 *
 *     // With std::vector
 *     std::vector<UInt8> vector_buffer(64);
 *     // uart.Receive(vector_buffer, status, 1000);  // Implicit conversion to span
 *
 *     // With C-style array
 *     UInt8 c_array[64];
 *     // uart.Receive(c_array, status, 1000);  // Implicit conversion to span
 *
 *     // All three patterns work seamlessly with std::span-based interface
 * }
 * @endcode
 */

} // namespace hm11::hal
