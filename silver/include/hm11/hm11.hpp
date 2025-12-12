#pragma once

// HM11 Bluetooth Low Energy Driver - Main Header
// Complete driver interface including HAL and type system

#include "hal.hpp"
#include "hm11_driver.hpp"
#include "types.hpp"

/// Main namespace for HM11 Bluetooth Low Energy driver
///
/// This namespace provides:
/// - Complete driver template class with concept-based interface validation
/// - Hardware abstraction layer for UART operations
/// - Strong type system for embedded safety-critical applications
/// - Zero-overhead template-based dependency injection
///
/// Usage Example:
/// ```cpp
/// #include <hm11/hm11.hpp>
///
/// auto uart = hm11::MockUART{hm11::UARTDataSize::Data_Size_8b};
/// auto driver = hm11::MakeHM11Driver(std::move(uart));
///
/// hm11::UARTStatus status;
/// driver.Test(status);
/// ```
namespace hm11 {
// All components are available through individual headers
// This header provides convenient single-include access
}