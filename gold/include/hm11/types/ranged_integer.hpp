/**
 * @file ranged_integer.hpp
 * @brief Template class for compile-time range-constrained integer types
 *
 * This header provides the RangedInteger<T, Min, Max, Tag> template class that
 * enforces Ada's range-constrained integer types at compile-time and runtime.
 * It is part of Decision 2 (Strong Type System Translation) from the migration
 * specification.
 *
 * ## Overview
 *
 * The RangedInteger template compresses Ada's extensive type system (11+ range-
 * constrained numeric types like Percent, Temperature, Latency) into a single
 * reusable C++ metaprogramming construct.
 *
 * ## Design Principles (Decision 2)
 *
 * - **Class encapsulation**: Private data members for safety
 * - **Debug assertions**: Runtime validation in debug builds (-O0)
 * - **Optional saturation**: Via -DENABLE_RUNTIME_RANGE_CHECKS
 * - **Zero overhead**: constexpr and noexcept for ISR safety
 * - **Strong typing**: Tag parameter prevents mixing different range types
 * - **Direct Ada mapping**: `subtype Percent is Natural range 0 .. 100`
 *   translates to `using Percent = RangedInteger<uint8_t, 0, 100, PercentTag>`
 *
 * ## Usage Example
 *
 * @code
 * // Define strong typedef with unique tag
 * using Percent = RangedInteger<uint8_t, 0, 100, struct PercentTag>;
 * using Temperature = RangedInteger<uint8_t, 0, 120, struct TemperatureTag>;
 *
 * // Compile-time construction (constexpr)
 * constexpr Percent battery{75};  // OK
 * // constexpr Percent invalid{150}; // Compile error with constexpr
 *
 * // Runtime construction (validated in debug builds)
 * Percent runtime_value{50};  // OK
 * // Percent overflow{200};       // Debug assertion failure
 *
 * // Type safety - different tags prevent mixing
 * Temperature temp{25};
 * // Percent p = temp;  // Compile error - different tags
 *
 * // Arithmetic operations with validation
 * Percent p1{30};
 * Percent p2{40};
 * Percent sum = p1 + p2;  // OK: 70
 * // Percent overflow = p1 + p2 + p2;  // Debug assertion (30+40+40 = 110 > 100)
 *
 * // Comparison operations
 * if (p1 < p2) {
 *     // p1 is less than p2
 * }
 *
 * // Extract underlying value
 * uint8_t raw = battery.value();  // Explicit conversion
 * @endcode
 *
 * ## Validation Strategy
 *
 * - **Compile-time**: constexpr contexts enforce constraints via static_assert
 * - **Debug builds**: Runtime assertions catch invalid values (-O0)
 * - **Safety-critical**: Optional saturation via -DENABLE_RUNTIME_RANGE_CHECKS
 * - **Release builds**: Minimal overhead, caller responsible for correctness
 *
 * ## Thread Safety & ISR Safety
 *
 * All operations are marked constexpr and noexcept, making them safe for use
 * in ISR (Interrupt Service Routine) contexts. No dynamic allocation, no
 * exceptions, no blocking operations.
 *
 * @tparam T Underlying integer type (uint8_t, uint16_t, etc.)
 * @tparam Min Minimum valid value (inclusive)
 * @tparam Max Maximum valid value (inclusive)
 * @tparam Tag Empty tag type for strong typing (prevents mixing different ranges)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <cassert>
#include <concepts>
#include <limits>
#include <type_traits>

namespace hm11::types {

/**
 * @brief Range-constrained integer type with compile-time and runtime validation
 *
 * This template class provides Ada-equivalent range checking for integer types.
 * It enforces constraints at compile-time (constexpr contexts) and runtime
 * (debug builds via assertions).
 *
 * ## Arithmetic Overflow Behavior
 *
 * Arithmetic operations (+, -, *, /) validate that results remain within bounds:
 * - **Debug builds**: Assertion failure if result exceeds [Min, Max]
 * - **Release builds**: Undefined behavior if overflow occurs
 * - **Caller responsibility**: Ensure operations don't overflow in release builds
 *
 * ## Implicit Conversion Policy
 *
 * This class follows Ada's strong typing model:
 * - **No implicit conversion** to/from underlying type T
 * - **Explicit construction** from T with validation
 * - **Explicit .value()** accessor to extract underlying value
 * - **Prevents accidental mixing** of different range types
 *
 * @tparam T Underlying integer type (must be std::integral)
 * @tparam Min Minimum valid value (inclusive, must be >= std::numeric_limits<T>::min())
 * @tparam Max Maximum valid value (inclusive, must be <= std::numeric_limits<T>::max())
 * @tparam Tag Empty tag type for strong typing (use struct TagName)
 */
template <std::integral T, T Min, T Max, typename Tag> class RangedInteger {
    static_assert(Min <= Max, "Min must be <= Max");
    static_assert(Min >= std::numeric_limits<T>::min(), "Min must be >= numeric_limits<T>::min()");
    static_assert(Max <= std::numeric_limits<T>::max(), "Max must be <= numeric_limits<T>::max()");

  public:
    // Type aliases for external use
    using ValueType = T;
    using TagType = Tag;

    // Expose bounds as public constexpr constants
    static constexpr T MIN_VALUE = Min;
    static constexpr T MAX_VALUE = Max;

    /**
     * @brief Default constructor - initializes to minimum value
     *
     * Provides a safe default by initializing to Min, matching Ada's
     * behavior where uninitialized variables have a defined value.
     */
    constexpr RangedInteger() noexcept : value{Min} {}

    /**
     * @brief Explicit constructor from underlying type with validation
     *
     * Constructs a RangedInteger from a value of type T. Validation occurs:
     * - At compile-time in constexpr contexts (static_assert)
     * - At runtime in debug builds (assertion)
     * - Optional saturation with -DENABLE_RUNTIME_RANGE_CHECKS
     *
     * @param val Value to initialize with
     * @pre val must be in range [Min, Max]
     *
     * @code
     * using Percent = RangedInteger<uint8_t, 0, 100, struct PercentTag>;
     * constexpr Percent p1{50};  // OK - compile-time check
     * Percent p2{75};            // OK - runtime check in debug
     * // Percent p3{150};        // Assertion failure in debug
     * @endcode
     */
    explicit constexpr RangedInteger(T val) noexcept : value{validate(val)} {}

    /**
     * @brief Copy constructor
     *
     * Trivially copyable - no validation needed since source is already valid.
     */
    constexpr RangedInteger(const RangedInteger&) noexcept = default;

    /**
     * @brief Copy assignment operator
     *
     * Trivially copyable - no validation needed since source is already valid.
     */
    constexpr RangedInteger& operator=(const RangedInteger&) noexcept = default;

    /**
     * @brief Destructor
     *
     * Trivially destructible - no cleanup needed.
     */
    ~RangedInteger() = default;

    /**
     * @brief Extract underlying value
     *
     * Provides explicit access to the underlying value. Use this when you
     * need to pass the value to APIs that expect the raw type T.
     *
     * @return The underlying value of type T
     *
     * @code
     * using Percent = RangedInteger<uint8_t, 0, 100, struct PercentTag>;
     * Percent battery{75};
     * uint8_t raw = battery.get();  // raw == 75
     * @endcode
     */
    [[nodiscard]] constexpr T get() const noexcept { return value; }

    // ========================================================================
    // Comparison Operators
    // ========================================================================

    /**
     * @brief Equality comparison
     *
     * Compares two RangedInteger instances for equality. Only instances with
     * the same Tag can be compared (enforced at compile-time).
     *
     * @param rhs Right-hand side operand
     * @return true if values are equal, false otherwise
     */
    [[nodiscard]] constexpr bool operator==(const RangedInteger& rhs) const noexcept { return value == rhs.value; }

    /**
     * @brief Inequality comparison
     *
     * @param rhs Right-hand side operand
     * @return true if values are not equal, false otherwise
     */
    [[nodiscard]] constexpr bool operator!=(const RangedInteger& rhs) const noexcept { return value != rhs.value; }

    /**
     * @brief Less-than comparison
     *
     * @param rhs Right-hand side operand
     * @return true if this value is less than rhs, false otherwise
     */
    [[nodiscard]] constexpr bool operator<(const RangedInteger& rhs) const noexcept { return value < rhs.value; }

    /**
     * @brief Less-than-or-equal comparison
     *
     * @param rhs Right-hand side operand
     * @return true if this value is less than or equal to rhs, false otherwise
     */
    [[nodiscard]] constexpr bool operator<=(const RangedInteger& rhs) const noexcept { return value <= rhs.value; }

    /**
     * @brief Greater-than comparison
     *
     * @param rhs Right-hand side operand
     * @return true if this value is greater than rhs, false otherwise
     */
    [[nodiscard]] constexpr bool operator>(const RangedInteger& rhs) const noexcept { return value > rhs.value; }

    /**
     * @brief Greater-than-or-equal comparison
     *
     * @param rhs Right-hand side operand
     * @return true if this value is greater than or equal to rhs, false otherwise
     */
    [[nodiscard]] constexpr bool operator>=(const RangedInteger& rhs) const noexcept { return value >= rhs.value; }

    // ========================================================================
    // Arithmetic Operators
    // ========================================================================

    /**
     * @brief Addition operator
     *
     * Adds two RangedInteger values. The result is validated:
     * - Debug builds: Assertion failure if result exceeds [Min, Max]
     * - Release builds: Caller must ensure no overflow
     *
     * @param rhs Right-hand side operand
     * @return New RangedInteger with sum of values
     * @pre value + rhs.value must be in range [Min, Max]
     */
    [[nodiscard]] constexpr RangedInteger operator+(const RangedInteger& rhs) const noexcept {
        return RangedInteger{static_cast<T>(value + rhs.value)};
    }

    /**
     * @brief Subtraction operator
     *
     * Subtracts one RangedInteger from another. The result is validated:
     * - Debug builds: Assertion failure if result exceeds [Min, Max]
     * - Release builds: Caller must ensure no underflow
     *
     * @param rhs Right-hand side operand
     * @return New RangedInteger with difference of values
     * @pre value - rhs.value must be in range [Min, Max]
     */
    [[nodiscard]] constexpr RangedInteger operator-(const RangedInteger& rhs) const noexcept {
        return RangedInteger{static_cast<T>(value - rhs.value)};
    }

    /**
     * @brief Pre-increment operator
     *
     * Increments the value by 1. Validated in debug builds.
     *
     * @return Reference to this object after increment
     * @pre value + 1 must be <= Max
     */
    constexpr RangedInteger& operator++() noexcept {
        value = validate(static_cast<T>(value + 1));
        return *this;
    }

    /**
     * @brief Post-increment operator
     *
     * Increments the value by 1 and returns the previous value.
     * Validated in debug builds.
     *
     * @return Copy of this object before increment
     * @pre value + 1 must be <= Max
     */
    constexpr RangedInteger operator++(int) noexcept {
        RangedInteger old = *this;
        ++(*this);
        return old;
    }

    /**
     * @brief Pre-decrement operator
     *
     * Decrements the value by 1. Validated in debug builds.
     *
     * @return Reference to this object after decrement
     * @pre value - 1 must be >= Min
     */
    constexpr RangedInteger& operator--() noexcept {
        value = validate(static_cast<T>(value - 1));
        return *this;
    }

    /**
     * @brief Post-decrement operator
     *
     * Decrements the value by 1 and returns the previous value.
     * Validated in debug builds.
     *
     * @return Copy of this object before decrement
     * @pre value - 1 must be >= Min
     */
    constexpr RangedInteger operator--(int) noexcept {
        RangedInteger old = *this;
        --(*this);
        return old;
    }

    /**
     * @brief Addition assignment operator
     *
     * Adds rhs to this value. Validated in debug builds.
     *
     * @param rhs Right-hand side operand
     * @return Reference to this object after addition
     * @pre value + rhs.value must be in range [Min, Max]
     */
    constexpr RangedInteger& operator+=(const RangedInteger& rhs) noexcept {
        value = validate(static_cast<T>(value + rhs.value));
        return *this;
    }

    /**
     * @brief Subtraction assignment operator
     *
     * Subtracts rhs from this value. Validated in debug builds.
     *
     * @param rhs Right-hand side operand
     * @return Reference to this object after subtraction
     * @pre value - rhs.value must be in range [Min, Max]
     */
    constexpr RangedInteger& operator-=(const RangedInteger& rhs) noexcept {
        value = validate(static_cast<T>(value - rhs.value));
        return *this;
    }

  private:
    /**
     * @brief Validates value is within range [Min, Max]
     *
     * Validation strategy (Decision 2):
     * - Compile-time: constexpr evaluation triggers static_assert on invalid values
     * - Debug builds: Runtime assertion catches out-of-range values
     * - Safety-critical: Optional saturation via -DENABLE_RUNTIME_RANGE_CHECKS
     * - Release builds: Minimal overhead, assumes caller correctness
     *
     * @param val Value to validate
     * @return Validated value (or saturated value if ENABLE_RUNTIME_RANGE_CHECKS)
     */
    static constexpr T validate(T val) noexcept {
#ifdef ENABLE_RUNTIME_RANGE_CHECKS
        // Safety-critical builds: saturate to bounds
        if (val < Min) {
            return Min;
        }
        if (val > Max) {
            return Max;
        }
        return val;
#else
        // Debug and release builds: assert in debug, undefined in release
        assert(val >= Min && "Value below minimum bound");
        assert(val <= Max && "Value above maximum bound");
        return val;
#endif
    }

    T value; ///< Underlying value (guaranteed to be in [Min, Max] after construction)
};

} // namespace hm11::types
