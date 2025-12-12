/**
 * @file types.hpp
 * @brief Common HAL type definitions for HM11 driver
 *
 * This file contains the basic type definitions that are used throughout the HAL
 * layer of the HM11 driver. These types are primarily numeric types with specific
 * bit widths, matching the Ada HAL package.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_HAL_TYPES_HPP
#define HM11_HAL_TYPES_HPP

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace hm11::hal {

/**
 * @brief Basic unsigned integer types with specific bit widths
 *
 * These types match the Ada HAL package's UIntX types.
 * C++ standard types are used where possible for compatibility and performance.
 */

// 1-bit unsigned integer (Bit in Ada)
using Bit = bool;

// Standard sized unsigned integers (match Ada UInt8, UInt16, UInt32, UInt64)
using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;

// Non-standard sized unsigned integers
using UInt9 = std::uint16_t; // Smallest standard C++ type that can hold 9 bits

// Array types
template <typename T, std::size_t N> using Array = std::array<T, N>;

/**
 * @brief Dynamically-sized array of UInt8 values
 *
 * Corresponds to the Ada HAL.UInt8_Array type. Uses std::vector for
 * dynamic sizing.
 */
using UInt8Array = std::vector<UInt8>;

/**
 * @brief Dynamically-sized array of UInt16 values
 *
 * Corresponds to the Ada HAL.UInt16_Array type. Uses std::vector for
 * dynamic sizing.
 */
using UInt16Array = std::vector<UInt16>;

/**
 * @brief Dynamically-sized array of UInt32 values
 *
 * Corresponds to the Ada HAL.UInt32_Array type. Uses std::vector for
 * dynamic sizing.
 */
using UInt32Array = std::vector<UInt32>;

/**
 * @brief Non-owning view of a range of UInt8 values
 *
 * Provides a view over a contiguous sequence of UInt8 values.
 * Used for passing data to functions without transferring ownership.
 */
using UInt8Span = std::span<UInt8>;

/**
 * @brief Non-owning view of a range of UInt16 values
 *
 * Provides a view over a contiguous sequence of UInt16 values.
 * Used for passing data to functions without transferring ownership.
 */
using UInt16Span = std::span<UInt16>;

/**
 * @brief Non-owning view of a range of UInt32 values
 *
 * Provides a view over a contiguous sequence of UInt32 values.
 * Used for passing data to functions without transferring ownership.
 */
using UInt32Span = std::span<UInt32>;

/**
 * @brief Non-owning view of a range of const UInt8 values
 *
 * Provides a read-only view over a contiguous sequence of UInt8 values.
 * Used for passing read-only data to functions without transferring ownership.
 */
using UInt8ConstSpan = std::span<const UInt8>;

/**
 * @brief Non-owning view of a range of const UInt16 values
 *
 * Provides a read-only view over a contiguous sequence of UInt16 values.
 * Used for passing read-only data to functions without transferring ownership.
 */
using UInt16ConstSpan = std::span<const UInt16>;

/**
 * @brief Non-owning view of a range of const UInt32 values
 *
 * Provides a read-only view over a contiguous sequence of UInt32 values.
 * Used for passing read-only data to functions without transferring ownership.
 */
using UInt32ConstSpan = std::span<const UInt32>;

} // namespace hm11::hal

#endif // HM11_HAL_TYPES_HPP