/**
 * @file ranged_integer.hpp
 * @brief RangedInteger template class for strong type safety
 *
 * This file contains the RangedInteger template class that provides strong
 * type safety for integer range-constrained types, similar to Ada's
 * range-constrained types. It includes compile-time validation via
 * static_assert as well as optional runtime validation for debug builds.
 */

#ifndef HM11_UTIL_RANGED_INTEGER_HPP
#define HM11_UTIL_RANGED_INTEGER_HPP

#include <cassert>
#include <concepts>
#include <stdexcept>
#include <type_traits>

#include "type_traits.hpp"

namespace hm11::util {

/**
 * @brief Provides a range-constrained integer type with strong type safety
 *
 * This template class implements a range-constrained integer type similar to Ada's
 * range types. It ensures values are always within the specified range through
 * both compile-time and runtime validation. All operations are constexpr where
 * possible for compile-time evaluation, and noexcept for ISR safety.
 *
 * The class provides:
 * - Compile-time range validation via static_assert
 * - Runtime range validation via assertions in debug builds
 * - Optional runtime saturation with ENABLE_RUNTIME_RANGE_CHECKS
 * - Arithmetic operations with validation
 * - Comparison operators
 * - Type-safe distinctions using Tag parameter
 *
 * This implementation corresponds to Decision 2: Strong Type System Translation,
 * which requires maintaining Ada's strong typing and validation in the C++ code.
 *
 * Example usage:
 * ```cpp
 * // Define a percent type (0-100)
 * using Percent = RangedInteger<uint8_t, 0, 100, struct PercentTag>;
 *
 * // Use the type
 * Percent p1(50);   // Valid
 * Percent p2(150);  // Assertion in debug, saturation in safety builds
 * Percent p3 = p1 + 20; // Valid
 * int raw = static_cast<int>(p1); // Explicit conversion required
 * ```
 *
 * Thread safety: This class has no internal state beyond the stored value.
 * All operations are thread-safe, and all methods are marked noexcept for
 * ISR safety.
 *
 * @tparam T The underlying integer type
 * @tparam Min The minimum allowed value
 * @tparam Max The maximum allowed value
 * @tparam Tag Optional tag type for distinguishing otherwise identical types
 *
 * @see ENABLE_RUNTIME_RANGE_CHECKS to enable runtime saturation
 */
template <IntegralType T, T Min, T Max, typename Tag = void> class RangedInteger {
    // Ensure Min <= Max at compile time
    static_assert(Min <= Max, "Min must be less than or equal to Max");

  private:
    T value;

    /**
     * @brief Validates a value at runtime
     *
     * This method performs runtime validation of a value against the allowed range:
     * - In debug builds, it uses assertions to verify that the value is within range
     * - If ENABLE_RUNTIME_RANGE_CHECKS is defined, it performs saturation:
     *   - Values below Min are clamped to Min
     *   - Values above Max are clamped to Max
     *
     * This approach balances safety and performance:
     * - Debug builds catch violations early with assertions
     * - Safety-critical builds can saturate values instead of failing
     * - Release builds without ENABLE_RUNTIME_RANGE_CHECKS assume values are valid
     *
     * @param value The value to validate
     * @return T The validated value, possibly saturated to the allowed range
     *
     * Thread safety: This method is thread-safe and ISR-safe.
     */
    [[nodiscard]] static constexpr T validate(T value) noexcept {
        // In debug builds, use assertions to validate
#ifndef NDEBUG
        assert(value >= Min && value <= Max && "Value out of range");
#endif

#ifdef ENABLE_RUNTIME_RANGE_CHECKS
        // With runtime checks enabled, perform saturation
        if (value < Min) {
            return Min;
        }
        if (value > Max) {
            return Max;
        }
#endif
        return value;
    }

  public:
    /**
     * @brief Default constructor initializes to Min value
     *
     * Creates a new RangedInteger instance with the minimum allowed value.
     * This provides a safe, predictable default state.
     *
     * Thread safety: This constructor is thread-safe and ISR-safe.
     */
    constexpr RangedInteger() noexcept : value(Min) {}

    /**
     * @brief Constructor with value validation
     *
     * Creates a new RangedInteger instance with the specified value.
     * The value is validated using the validate() method, which ensures
     * it is within the allowed range, possibly through saturation if
     * ENABLE_RUNTIME_RANGE_CHECKS is defined.
     *
     * @param val The value to initialize with
     *
     * @pre In debug builds, val must be within [Min, Max] or an assertion will trigger
     * @post The internal value is within [Min, Max]
     *
     * Thread safety: This constructor is thread-safe and ISR-safe.
     */
    constexpr explicit RangedInteger(T val) noexcept : value(validate(val)) {}

    /**
     * @brief Get the underlying value
     *
     * Returns the raw value stored in this RangedInteger instance.
     * This provides access to the value for calculations or interfacing
     * with code that requires the raw integer type.
     *
     * @return T The underlying value
     *
     * @post The returned value is guaranteed to be within [Min, Max]
     *
     * Thread safety: This method is thread-safe and ISR-safe.
     */
    [[nodiscard]] constexpr T get_value() const noexcept { return value; }

    /**
     * @brief Cast operator to underlying type
     *
     * Provides explicit conversion to the underlying integer type.
     * This explicit conversion helps prevent accidental type conversions
     * while still allowing deliberate conversions when needed.
     *
     * @return T The underlying value
     *
     * Usage:
     * ```cpp
     * Percent p(75);
     * uint8_t raw = static_cast<uint8_t>(p); // Explicit conversion
     * ```
     *
     * @post The returned value is guaranteed to be within [Min, Max]
     *
     * Thread safety: This method is thread-safe and ISR-safe.
     */
    [[nodiscard]] constexpr explicit operator T() const noexcept { return get_value(); }

    /**
     * @brief Minimum allowed value
     *
     * @return constexpr T
     */
    [[nodiscard]] static constexpr T min() noexcept { return Min; }

    /**
     * @brief Maximum allowed value
     *
     * @return constexpr T
     */
    [[nodiscard]] static constexpr T max() noexcept { return Max; }

    // Arithmetic operators with validation

    /**
     * @brief Addition operator
     *
     * @param other The value to add
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator+(const RangedInteger& other) const noexcept {
        return RangedInteger(validate(value + other.value));
    }

    /**
     * @brief Addition operator with raw value
     *
     * @param other The value to add
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator+(T other) const noexcept {
        return RangedInteger(validate(value + other));
    }

    /**
     * @brief Subtraction operator
     *
     * @param other The value to subtract
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator-(const RangedInteger& other) const noexcept {
        return RangedInteger(validate(value - other.value));
    }

    /**
     * @brief Subtraction operator with raw value
     *
     * @param other The value to subtract
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator-(T other) const noexcept {
        return RangedInteger(validate(value - other));
    }

    /**
     * @brief Multiplication operator
     *
     * @param other The value to multiply by
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator*(const RangedInteger& other) const noexcept {
        return RangedInteger(validate(value * other.value));
    }

    /**
     * @brief Multiplication operator with raw value
     *
     * @param other The value to multiply by
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator*(T other) const noexcept {
        return RangedInteger(validate(value * other));
    }

    /**
     * @brief Division operator
     *
     * @param other The value to divide by
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator/(const RangedInteger& other) const noexcept {
        // Avoid division by zero
        assert(other.value != 0 && "Division by zero");
        return RangedInteger(validate(value / other.value));
    }

    /**
     * @brief Division operator with raw value
     *
     * @param other The value to divide by
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator/(T other) const noexcept {
        // Avoid division by zero
        assert(other != 0 && "Division by zero");
        return RangedInteger(validate(value / other));
    }

    /**
     * @brief Modulo operator
     *
     * @param other The modulus
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator%(const RangedInteger& other) const noexcept {
        // Avoid modulo by zero
        assert(other.value != 0 && "Modulo by zero");
        return RangedInteger(validate(value % other.value));
    }

    /**
     * @brief Modulo operator with raw value
     *
     * @param other The modulus
     * @return RangedInteger The result with validated range
     */
    [[nodiscard]] constexpr RangedInteger operator%(T other) const noexcept {
        // Avoid modulo by zero
        assert(other != 0 && "Modulo by zero");
        return RangedInteger(validate(value % other));
    }

    // Compound assignment operators

    /**
     * @brief Addition assignment operator
     *
     * @param other The value to add
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator+=(const RangedInteger& other) noexcept {
        value = validate(value + other.value);
        return *this;
    }

    /**
     * @brief Addition assignment operator with raw value
     *
     * @param other The value to add
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator+=(T other) noexcept {
        value = validate(value + other);
        return *this;
    }

    /**
     * @brief Subtraction assignment operator
     *
     * @param other The value to subtract
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator-=(const RangedInteger& other) noexcept {
        value = validate(value - other.value);
        return *this;
    }

    /**
     * @brief Subtraction assignment operator with raw value
     *
     * @param other The value to subtract
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator-=(T other) noexcept {
        value = validate(value - other);
        return *this;
    }

    /**
     * @brief Multiplication assignment operator
     *
     * @param other The value to multiply by
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator*=(const RangedInteger& other) noexcept {
        value = validate(value * other.value);
        return *this;
    }

    /**
     * @brief Multiplication assignment operator with raw value
     *
     * @param other The value to multiply by
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator*=(T other) noexcept {
        value = validate(value * other);
        return *this;
    }

    /**
     * @brief Division assignment operator
     *
     * @param other The value to divide by
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator/=(const RangedInteger& other) noexcept {
        // Avoid division by zero
        assert(other.value != 0 && "Division by zero");
        value = validate(value / other.value);
        return *this;
    }

    /**
     * @brief Division assignment operator with raw value
     *
     * @param other The value to divide by
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator/=(T other) noexcept {
        // Avoid division by zero
        assert(other != 0 && "Division by zero");
        value = validate(value / other);
        return *this;
    }

    /**
     * @brief Modulo assignment operator
     *
     * @param other The modulus
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator%=(const RangedInteger& other) noexcept {
        // Avoid modulo by zero
        assert(other.value != 0 && "Modulo by zero");
        value = validate(value % other.value);
        return *this;
    }

    /**
     * @brief Modulo assignment operator with raw value
     *
     * @param other The modulus
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator%=(T other) noexcept {
        // Avoid modulo by zero
        assert(other != 0 && "Modulo by zero");
        value = validate(value % other);
        return *this;
    }

    // Increment and decrement operators

    /**
     * @brief Pre-increment operator
     *
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator++() noexcept {
        value = validate(value + 1);
        return *this;
    }

    /**
     * @brief Post-increment operator
     *
     * @return RangedInteger The previous value
     */
    [[nodiscard]] constexpr RangedInteger operator++(int) noexcept {
        RangedInteger temp(*this);
        ++(*this);
        return temp;
    }

    /**
     * @brief Pre-decrement operator
     *
     * @return RangedInteger& Reference to this object
     */
    constexpr RangedInteger& operator--() noexcept {
        value = validate(value - 1);
        return *this;
    }

    /**
     * @brief Post-decrement operator
     *
     * @return RangedInteger The previous value
     */
    [[nodiscard]] constexpr RangedInteger operator--(int) noexcept {
        RangedInteger temp(*this);
        --(*this);
        return temp;
    }

    // Comparison operators

    /**
     * @brief Equality operator
     *
     * @param other The value to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(const RangedInteger& other) const noexcept { return value == other.value; }

    /**
     * @brief Equality operator with raw value
     *
     * @param other The value to compare with
     * @return true if equal
     * @return false if not equal
     */
    [[nodiscard]] constexpr bool operator==(T other) const noexcept { return value == other; }

    /**
     * @brief Less than operator
     *
     * @param other The value to compare with
     * @return true if less than
     * @return false if not less than
     */
    [[nodiscard]] constexpr bool operator<(const RangedInteger& other) const noexcept { return value < other.value; }

    /**
     * @brief Less than operator with raw value
     *
     * @param other The value to compare with
     * @return true if less than
     * @return false if not less than
     */
    [[nodiscard]] constexpr bool operator<(T other) const noexcept { return value < other; }

    /**
     * @brief Greater than operator
     *
     * @param other The value to compare with
     * @return true if greater than
     * @return false if not greater than
     */
    [[nodiscard]] constexpr bool operator>(const RangedInteger& other) const noexcept { return value > other.value; }

    /**
     * @brief Greater than operator with raw value
     *
     * @param other The value to compare with
     * @return true if greater than
     * @return false if not greater than
     */
    [[nodiscard]] constexpr bool operator>(T other) const noexcept { return value > other; }

    /**
     * @brief Less than or equal operator
     *
     * @param other The value to compare with
     * @return true if less than or equal
     * @return false if not less than or equal
     */
    [[nodiscard]] constexpr bool operator<=(const RangedInteger& other) const noexcept { return value <= other.value; }

    /**
     * @brief Less than or equal operator with raw value
     *
     * @param other The value to compare with
     * @return true if less than or equal
     * @return false if not less than or equal
     */
    [[nodiscard]] constexpr bool operator<=(T other) const noexcept { return value <= other; }

    /**
     * @brief Greater than or equal operator
     *
     * @param other The value to compare with
     * @return true if greater than or equal
     * @return false if not greater than or equal
     */
    [[nodiscard]] constexpr bool operator>=(const RangedInteger& other) const noexcept { return value >= other.value; }

    /**
     * @brief Greater than or equal operator with raw value
     *
     * @param other The value to compare with
     * @return true if greater than or equal
     * @return false if not greater than or equal
     */
    [[nodiscard]] constexpr bool operator>=(T other) const noexcept { return value >= other; }

    /**
     * @brief Not equal operator
     *
     * @param other The value to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(const RangedInteger& other) const noexcept { return !(*this == other); }

    /**
     * @brief Not equal operator with raw value
     *
     * @param other The value to compare with
     * @return true if not equal
     * @return false if equal
     */
    [[nodiscard]] constexpr bool operator!=(T other) const noexcept { return !(*this == other); }
};

// Define the Ada range types using RangedInteger

// Tag types for distinguishing otherwise identical range types
struct PercentTag {};
struct MacWhiteListIndexTag {};
struct LayerConnectionLatencyTag {};
struct DiscoveredIndexTag {};
struct PioCollectionRateTag {};
struct PioNumberTag {};
struct DiscoveryTimeTag {};
struct TemperatureTypeTag {};
struct HumidityTypeTag {};

// Type aliases for Ada range types
using Percent = RangedInteger<uint8_t, 0, 100, PercentTag>;
using MacWhiteListIndex = RangedInteger<uint8_t, 1, 3, MacWhiteListIndexTag>;
using LayerConnectionLatency = RangedInteger<uint8_t, 0, 4, LayerConnectionLatencyTag>;
using DiscoveredIndex = RangedInteger<uint8_t, 0, 5, DiscoveredIndexTag>;
using PioCollectionRate = RangedInteger<uint8_t, 0, 99, PioCollectionRateTag>;
using PioNumber = RangedInteger<uint8_t, 1, 3, PioNumberTag>;
using DiscoveryTime = RangedInteger<uint8_t, 1, 9, DiscoveryTimeTag>;
using TemperatureType = RangedInteger<uint8_t, 0, 120, TemperatureTypeTag>;
using HumidityType = RangedInteger<uint8_t, 0, 100, HumidityTypeTag>;

} // namespace hm11::util

#endif // HM11_UTIL_RANGED_INTEGER_HPP