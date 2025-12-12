#pragma once

/// @file types.hpp
/// @brief Main header for HM11 type system
///
/// This header provides the complete type system for the HM11 driver,
/// implementing Ada's strong type safety through C++ templates and enums.
///
/// The type system consists of:
/// - RangedInteger<T, Min, Max, Tag> template for range-constrained integers
/// - FixedString<N, Validator> template for character-validated strings
/// - Type aliases mapping all Ada types to C++ equivalents
/// - Enum classes for all Ada enumeration types
///
/// Key features:
/// - Zero-overhead abstractions (constexpr/noexcept)
/// - Compile-time validation where possible
/// - Runtime validation in debug builds
/// - Semantic type safety through unique tag types
/// - Direct Ada-to-C++ type mapping preservation

#include "types/fixed_string.hpp"
#include "types/ranged_integer.hpp"
#include "types/type_aliases.hpp"
#include "types/validators.hpp"

/// @namespace hm11
/// @brief Root namespace for HM11 driver components
///
/// All HM11 driver types, classes, and functions are defined within this namespace
/// to avoid conflicts with other libraries and provide clear organization.
///
/// Sub-namespaces:
/// - hm11::sync - Synchronization primitives (future)
/// - hm11::platform - Platform abstraction (future)
/// - hm11::hal - Hardware abstraction (future)
/// - hm11::util - Utilities and helpers (future)
namespace hm11 {

/// @brief Type system version for compatibility checking
constexpr int TYPE_SYSTEM_VERSION_MAJOR = 1;
constexpr int TYPE_SYSTEM_VERSION_MINOR = 0;
constexpr int TYPE_SYSTEM_VERSION_PATCH = 0;

/// @brief Compile-time check that templates are working correctly
/// This ensures the type system is functioning during compilation
namespace detail {
// Test basic RangedInteger functionality
static_assert(RangedInteger<uint8_t, 0, 100, struct TestTag>(50).value() == 50);
static_assert(RangedInteger<uint8_t, 0, 100, struct TestTag>::MIN_VALUE == 0);
static_assert(RangedInteger<uint8_t, 0, 100, struct TestTag>::MAX_VALUE == 100);

// Test FixedString functionality
static_assert(FixedString<12, HexCharValidator>::MAX_SIZE == 12);
static_assert(std::is_same_v<FixedString<12, HexCharValidator>::ValidatorType, HexCharValidator>);

// Test type alias correctness
static_assert(std::is_same_v<Percent, RangedInteger<uint8_t, 0, 100, PercentTag>>);
static_assert(std::is_same_v<MacAddress, FixedString<12, HexCharValidator>>);
static_assert(std::is_same_v<PinType, FixedString<6, DigitCharValidator>>);

// Test enum sizes for memory efficiency
static_assert(sizeof(Role) == 1);
static_assert(sizeof(AdvertisingInterval) == 1);
static_assert(sizeof(UartBaudRate) == 1);
} // namespace detail

} // namespace hm11