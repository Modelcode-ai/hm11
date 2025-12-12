/**
 * @file hal.hpp
 * @brief Hardware Abstraction Layer base header
 * @details Core definitions for the HAL module
 *
 * This file provides common constants and utilities for the HAL layer.
 * It includes forward declarations and common typedefs that are used
 * across various HAL components.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_HAL_HAL_HPP
#define HM11_HAL_HAL_HPP

#include <chrono>

#include "types.hpp"

namespace hm11::hal {

// Forward declarations
enum class UartStatus : std::uint8_t;
enum class UartDataSize : std::uint8_t;
class UartData8b;
class UartData9b;
class UARTPort;

// Common HAL constants
constexpr std::size_t DEFAULT_BUFFER_SIZE = 256;
constexpr std::chrono::milliseconds DEFAULT_TIMEOUT = std::chrono::milliseconds(1000);

} // namespace hm11::hal

#endif // HM11_HAL_HAL_HPP