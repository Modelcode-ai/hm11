#pragma once

// HM11 Hardware Abstraction Layer - Main Header
// Provides complete HAL interface including UART abstraction

#include "hal/mock_uart.hpp"
#include "hal/uart_interface.hpp"
#include "hal/uart_types.hpp"

/// Hardware Abstraction Layer for HM11 driver
///
/// This namespace contains all hardware abstraction components:
/// - UART interface definitions and concepts
/// - Mock implementations for testing
/// - Type definitions equivalent to Ada's HAL types
///
/// The HAL follows the modernization spec guidelines:
/// - C++20 concepts for interface constraints
/// - Zero-overhead template-based polymorphism
/// - Strong type safety equivalent to Ada
/// - Memory safety through modern C++ practices
namespace hm11::hal {
// All types and interfaces are available through individual headers
}

// Convenience aliases for common use
namespace hm11 {
/// Convenient access to HAL types
using UARTStatus = hal::UARTStatus;
using UARTDataSize = hal::UARTDataSize;
using MockUART = hal::MockUART;

/// Template alias for UART interface constraint
template <typename T>
concept UART = hal::UARTInterface<T>;
} // namespace hm11