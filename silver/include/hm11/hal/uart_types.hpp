#pragma once

// HM11 Hardware Abstraction Layer - UART Types
// Equivalent to Ada's HAL.UART types with C++ strong typing

#include <cstdint>
#include <span>
#include <type_traits>

namespace hm11::hal {

/// Status returned by UART operations
/// Equivalent to Ada's HAL.UART.UART_Status
enum class UARTStatus : std::uint8_t {
    Ok = 0,         ///< Operation completed successfully
    ErrError = 1,   ///< General error occurred
    ErrTimeout = 2, ///< Operation timed out
    Busy = 3        ///< UART is busy with another operation
};

/// UART data size configuration
/// Equivalent to Ada's HAL.UART.UART_Data_Size
enum class UARTDataSize : std::uint8_t {
    DataSize8b = 8, ///< 8-bit data mode
    DataSize9b = 9  ///< 9-bit data mode (extended addressing)
};

/// 8-bit UART data buffer type
/// Equivalent to Ada's HAL.UART.UART_Data_8b
using UARTData8b = std::span<std::uint8_t>;

/// Const version for transmit operations
using UARTData8bConst = std::span<const std::uint8_t>;

/// 9-bit UART data buffer type
/// Equivalent to Ada's HAL.UART.UART_Data_9b (UInt9 = mod 2**9)
using UARTData9b = std::span<std::uint16_t>;

/// Const version for transmit operations
using UARTData9bConst = std::span<const std::uint16_t>;

/// Default timeout value in milliseconds
/// Equivalent to Ada's default timeout parameter
constexpr std::uint32_t DEFAULT_TIMEOUT_MS = 1000;

} // namespace hm11::hal