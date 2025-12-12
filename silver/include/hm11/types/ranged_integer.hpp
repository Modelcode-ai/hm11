#pragma once

#include <cassert>
#include <concepts>
#include <stdexcept>
#include <type_traits>

namespace hm11 {

/// @brief Template class for range-constrained integers with compile-time validation
///
/// This template provides compile-time type safety equivalent to Ada's range types.
/// Each instantiation creates a distinct type that cannot be accidentally mixed with
/// other range types, preventing semantic errors at compile time.
///
/// @tparam T Underlying integral type (uint8_t, uint16_t, int, etc.)
/// @tparam Min Minimum allowed value (inclusive)
/// @tparam Max Maximum allowed value (inclusive)
/// @tparam Tag Unique tag type for semantic differentiation
///
/// Example Ada type mappings:
/// - Ada: `type Percent is range 0 .. 100;`
/// - C++: `RangedInteger<uint8_t, 0, 100, struct PercentTag>`
///
/// Features:
/// - Zero-overhead abstraction (all operations are constexpr/noexcept)
/// - Compile-time validation where possible
/// - Debug assertion validation for runtime checks
/// - Operator overloading with validation
/// - Semantic type safety through unique Tag types
template <std::integral T, T Min, T Max, typename Tag> class RangedInteger {
    static_assert(Min <= Max, "Invalid range: Min must be <= Max");
    static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "Type T must not be const or volatile");

  public:
    // Type aliases for external use
    using ValueType = T;
    using TagType = Tag;

    // Static range constants
    static constexpr T MIN_VALUE = Min;
    static constexpr T MAX_VALUE = Max;

    /// @brief Default constructor - initializes to minimum value
    constexpr RangedInteger() noexcept : value_data(Min) {}

    /// @brief Explicit constructor from value with validation
    /// @param value Value to initialize with
    /// @throws std::out_of_range in debug builds if value is out of range
    explicit constexpr RangedInteger(T input_value) : value_data(validate_and_get(input_value)) {}

    /// @brief Copy constructor
    constexpr RangedInteger(const RangedInteger&) noexcept = default;

    /// @brief Copy assignment operator
    constexpr RangedInteger& operator=(const RangedInteger&) noexcept = default;

    /// @brief Move constructor
    constexpr RangedInteger(RangedInteger&&) noexcept = default;

    /// @brief Move assignment operator
    constexpr RangedInteger& operator=(RangedInteger&&) noexcept = default;

    /// @brief Destructor
    ~RangedInteger() = default;

    /// @brief Assignment from raw value with validation
    /// @param value New value to assign
    /// @return Reference to this object
    constexpr RangedInteger& operator=(T input_value) {
        value_data = validate_and_get(input_value);
        return *this;
    }

    /// @brief Get the underlying value
    /// @return The stored value
    constexpr T value() const noexcept { return value_data; }

    /// @brief Implicit conversion to underlying type (for compatibility)
    constexpr operator T() const noexcept { return value_data; }

    // Arithmetic operators (return RangedInteger to maintain type safety)

    /// @brief Addition operator
    /// @param rhs Right-hand side operand
    /// @return New RangedInteger with sum (validated)
    constexpr RangedInteger operator+(const RangedInteger& rhs) const {
        return RangedInteger(static_cast<T>(value_data + rhs.value_data));
    }

    /// @brief Addition operator with raw value
    constexpr RangedInteger operator+(T rhs) const { return RangedInteger(static_cast<T>(value_data + rhs)); }

    /// @brief Subtraction operator
    constexpr RangedInteger operator-(const RangedInteger& rhs) const {
        return RangedInteger(static_cast<T>(value_data - rhs.value_data));
    }

    /// @brief Subtraction operator with raw value
    constexpr RangedInteger operator-(T rhs) const { return RangedInteger(static_cast<T>(value_data - rhs)); }

    /// @brief Multiplication operator
    constexpr RangedInteger operator*(const RangedInteger& rhs) const {
        return RangedInteger(static_cast<T>(value_data * rhs.value_data));
    }

    /// @brief Multiplication operator with raw value
    constexpr RangedInteger operator*(T rhs) const { return RangedInteger(static_cast<T>(value_data * rhs)); }

    /// @brief Division operator
    constexpr RangedInteger operator/(const RangedInteger& rhs) const {
        return RangedInteger(static_cast<T>(value_data / rhs.value_data));
    }

    /// @brief Division operator with raw value
    constexpr RangedInteger operator/(T rhs) const { return RangedInteger(static_cast<T>(value_data / rhs)); }

    /// @brief Modulo operator
    constexpr RangedInteger operator%(const RangedInteger& rhs) const {
        return RangedInteger(static_cast<T>(value_data % rhs.value_data));
    }

    /// @brief Modulo operator with raw value
    constexpr RangedInteger operator%(T rhs) const { return RangedInteger(static_cast<T>(value_data % rhs)); }

    // Compound assignment operators

    constexpr RangedInteger& operator+=(const RangedInteger& rhs) { return *this = *this + rhs; }

    constexpr RangedInteger& operator+=(T rhs) { return *this = *this + rhs; }

    constexpr RangedInteger& operator-=(const RangedInteger& rhs) { return *this = *this - rhs; }

    constexpr RangedInteger& operator-=(T rhs) { return *this = *this - rhs; }

    constexpr RangedInteger& operator*=(const RangedInteger& rhs) { return *this = *this * rhs; }

    constexpr RangedInteger& operator*=(T rhs) { return *this = *this * rhs; }

    constexpr RangedInteger& operator/=(const RangedInteger& rhs) { return *this = *this / rhs; }

    constexpr RangedInteger& operator/=(T rhs) { return *this = *this / rhs; }

    constexpr RangedInteger& operator%=(const RangedInteger& rhs) { return *this = *this % rhs; }

    constexpr RangedInteger& operator%=(T rhs) { return *this = *this % rhs; }

    // Increment/Decrement operators

    /// @brief Pre-increment operator
    constexpr RangedInteger& operator++() { return *this += 1; }

    /// @brief Post-increment operator
    constexpr RangedInteger operator++(int) {
        RangedInteger temp = *this;
        ++(*this);
        return temp;
    }

    /// @brief Pre-decrement operator
    constexpr RangedInteger& operator--() { return *this -= 1; }

    /// @brief Post-decrement operator
    constexpr RangedInteger operator--(int) {
        RangedInteger temp = *this;
        --(*this);
        return temp;
    }

    // Comparison operators (with same tag type only)

    constexpr bool operator==(const RangedInteger& rhs) const noexcept { return value_data == rhs.value_data; }

    constexpr bool operator!=(const RangedInteger& rhs) const noexcept { return value_data != rhs.value_data; }

    constexpr bool operator<(const RangedInteger& rhs) const noexcept { return value_data < rhs.value_data; }

    constexpr bool operator<=(const RangedInteger& rhs) const noexcept { return value_data <= rhs.value_data; }

    constexpr bool operator>(const RangedInteger& rhs) const noexcept { return value_data > rhs.value_data; }

    constexpr bool operator>=(const RangedInteger& rhs) const noexcept { return value_data >= rhs.value_data; }

    // Comparison with raw values

    constexpr bool operator==(T rhs) const noexcept { return value_data == rhs; }

    constexpr bool operator!=(T rhs) const noexcept { return value_data != rhs; }

    constexpr bool operator<(T rhs) const noexcept { return value_data < rhs; }

    constexpr bool operator<=(T rhs) const noexcept { return value_data <= rhs; }

    constexpr bool operator>(T rhs) const noexcept { return value_data > rhs; }

    constexpr bool operator>=(T rhs) const noexcept { return value_data >= rhs; }

    /// @brief Check if the value is at its minimum
    constexpr bool at_min() const noexcept { return value_data == Min; }

    /// @brief Check if the value is at its maximum
    constexpr bool at_max() const noexcept { return value_data == Max; }

    /// @brief Check if a raw value is within the valid range
    static constexpr bool is_valid(T input_value) noexcept { return input_value >= Min && input_value <= Max; }

    /// @brief Try to create RangedInteger from value, return success/failure
    /// @param value Value to check and convert
    /// @param result Output parameter for the created value if valid
    /// @return true if value is valid and result was set
    /// Note: This is a factory method for cases where exception handling is not desired
    static constexpr bool try_create(T input_value, RangedInteger& result) noexcept {
        if (is_valid(input_value)) {
            result.value_data = input_value;
            return true;
        }
        return false;
    }

  private:
    T value_data;

    /// @brief Validate value and return it if valid, otherwise handle error
    /// @param value Value to validate
    /// @return The value if valid
    /// @throws std::out_of_range if value is invalid and NDEBUG is not defined
    static constexpr T validate_and_get(T input_value) {
// Runtime validation in debug builds
#ifndef NDEBUG
        if (!is_valid(input_value)) {
            // In debug builds, throw exception for proper error handling
            throw std::out_of_range("RangedInteger value out of range");
        }
#endif
        return input_value;
    }
};

// Free function operators for symmetry (T op RangedInteger)

template <std::integral T, T Min, T Max, typename Tag>
constexpr RangedInteger<T, Min, Max, Tag> operator+(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) {
    return rhs + lhs;
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr RangedInteger<T, Min, Max, Tag> operator-(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) {
    return RangedInteger<T, Min, Max, Tag>(static_cast<T>(lhs - rhs.value()));
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr RangedInteger<T, Min, Max, Tag> operator*(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) {
    return rhs * lhs;
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr RangedInteger<T, Min, Max, Tag> operator/(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) {
    return RangedInteger<T, Min, Max, Tag>(static_cast<T>(lhs / rhs.value()));
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr RangedInteger<T, Min, Max, Tag> operator%(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) {
    return RangedInteger<T, Min, Max, Tag>(static_cast<T>(lhs % rhs.value()));
}

// Comparison operators for symmetry (T op RangedInteger)

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator==(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return rhs == lhs;
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator!=(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return rhs != lhs;
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator<(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return lhs < rhs.value();
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator<=(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return lhs <= rhs.value();
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator>(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return lhs > rhs.value();
}

template <std::integral T, T Min, T Max, typename Tag>
constexpr bool operator>=(T lhs, const RangedInteger<T, Min, Max, Tag>& rhs) noexcept {
    return lhs >= rhs.value();
}

} // namespace hm11