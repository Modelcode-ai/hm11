/**
 * @file type_traits.hpp
 * @brief Type traits helper for the HM11 driver
 *
 * This file contains type traits helpers used by the RangedInteger template
 * and other utilities in the HM11 driver.
 */

#ifndef HM11_UTIL_TYPE_TRAITS_HPP
#define HM11_UTIL_TYPE_TRAITS_HPP

#include <concepts>
#include <type_traits>

namespace hm11::util {

/**
 * @brief Type trait to check if a value is in a specific range
 *
 * This is used for compile-time validation in RangedInteger
 *
 * @tparam T The type to check
 * @tparam Min The minimum allowed value
 * @tparam Max The maximum allowed value
 * @tparam Value The value to check
 */
template <typename T, T Min, T Max, T Value>
struct IsInRange : std::integral_constant<bool, (Value >= Min && Value <= Max)> {};

/**
 * @brief Helper variable template for is_in_range
 */
template <typename T, T Min, T Max, T Value> inline constexpr bool IS_IN_RANGE_V = IsInRange<T, Min, Max, Value>::value;

/**
 * @brief Concept to ensure a type is an integral type
 */
template <typename T>
concept IntegralType = std::is_integral_v<T>;

/**
 * @brief Concept to ensure a value is in range
 */
template <auto Value, auto Min, auto Max>
concept InRange = (Value >= Min && Value <= Max);

/**
 * @brief Concept to ensure one type can be represented by another without precision loss
 */
template <typename From, typename To>
concept RepresentableBy = std::is_convertible_v<From, To> &&
                          (sizeof(From) <= sizeof(To) || (std::is_signed_v<From> == std::is_signed_v<To>));

} // namespace hm11::util

#endif // HM11_UTIL_TYPE_TRAITS_HPP