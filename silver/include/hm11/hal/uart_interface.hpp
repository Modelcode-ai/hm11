#pragma once

// HM11 Hardware Abstraction Layer - UART Interface Concept
// Implements Design Decision #1 (Template Constraint Design) and
// Design Decision #4 (C++20 Concept-based Interface)

#include <concepts>
#include <type_traits>

#include "uart_types.hpp"

namespace hm11::hal {

/// C++20 concept defining the UART interface requirements
/// Equivalent to Ada's HAL.UART.UART_Port abstract interface
///
/// This concept enforces the interface contract at compile time while
/// enabling zero-overhead template-based dependency injection for embedded systems.
///
/// Required methods:
/// - DataSize() const -> UARTDataSize
/// - Transmit(UARTData8bConst, UARTStatus&, uint32_t) -> void
/// - Transmit(UARTData9bConst, UARTStatus&, uint32_t) -> void
/// - Receive(UARTData8b, UARTStatus&, uint32_t) -> void
/// - Receive(UARTData9b, UARTStatus&, uint32_t) -> void
template <typename T>
concept UARTInterface = requires(
    T uart,
    UARTData8bConst tx_data_8b,
    UARTData9bConst tx_data_9b,
    UARTData8b rx_data_8b,
    UARTData9b rx_data_9b,
    UARTStatus& status,
    std::uint32_t timeout) {
    // Type must be move-constructible for template instantiation
    std::is_move_constructible_v<T>;

    // Query data size configuration (equivalent to Ada's Data_Size function)
    { uart.DataSize() } -> std::same_as<UARTDataSize>;

    // Transmit operations (equivalent to Ada's overloaded Transmit procedures)
    { uart.Transmit(tx_data_8b, status, timeout) } -> std::same_as<void>;
    { uart.Transmit(tx_data_9b, status, timeout) } -> std::same_as<void>;

    // Transmit with default timeout
    { uart.Transmit(tx_data_8b, status) } -> std::same_as<void>;
    { uart.Transmit(tx_data_9b, status) } -> std::same_as<void>;

    // Receive operations (equivalent to Ada's overloaded Receive procedures)
    { uart.Receive(rx_data_8b, status, timeout) } -> std::same_as<void>;
    { uart.Receive(rx_data_9b, status, timeout) } -> std::same_as<void>;

    // Receive with default timeout
    { uart.Receive(rx_data_8b, status) } -> std::same_as<void>;
    { uart.Receive(rx_data_9b, status) } -> std::same_as<void>;
};

} // namespace hm11::hal