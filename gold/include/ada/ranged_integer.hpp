// ranged_integer.hpp - Ada range-constrained integer types for C++20.
//
// Maps `type X is range Min .. Max` to `using X = ada::RangedInteger<T, Min, Max, struct XTag>;`.
// As in Ada, `+` and `-` yield X::Base (Ada X'Base), computed in a type wider than the range and not limited to
// [Min, Max]; the range is checked when the result is stored, so `Percent p = a + b - c;` checks only p.
// Base-range overflow and out-of-range stores are caught rather than wrapped (Ada raises Constraint_Error).
//
// Out-of-range values:
// - Constant evaluation: always a compile error, including with NDEBUG.
// - Runtime, default: assert (debug builds only).
// - Runtime, -DENABLE_RUNTIME_RANGE_CHECKS: saturate to [Min, Max].
// Adjust out-of-range behavior in release builds to match the project's goals.
//
// Name the type when storing arithmetic: `Percent p = a + b;`, not `auto p = a + b;`. With auto, p is an unchecked
// Percent::Base until it is stored or passed as a Percent; Ada declarations always name the subtype, so always check.
//
// Adding additional operators:
// - Return Base with an overflow check (like detail::base_add); compound assignments validate() immediately.
// - Make binary operators hidden friends, not members, so ada::IntegerSubtype operands convert and work;
//   provide each RangedInteger/Base operand pairing.
// - Test in all three build modes (default, ENABLE_RUNTIME_RANGE_CHECKS, NDEBUG); write constant-evaluation checks as
//   concepts.
//
// Looping (Ada `for X in T loop`): add a range() view that counts in Wide. The naive translation
// `for (T x{}; x <= T{T::MAX_VALUE}; ++x)` increments past Max: asserts in debug, never ends when saturating.
//
// Ada floating-point (`digits`) and fixed-point (`delta`) range types would follow this template's pattern
// as RangedFloat / RangedFixed; floats need a NaN-safe check (`v >= Min && v <= Max`).

#ifndef ADA_RANGED_INTEGER_HPP
#define ADA_RANGED_INTEGER_HPP

#include <cassert>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "ada/detail/constraint_check.hpp"

namespace ada {

namespace detail {

// True if every sum/difference of two values in [Min, Max] (and Max + 1, Min - 1) fits in W.
template <typename W, auto Min, auto Max>
inline constexpr bool RANGE_FITS_IN = std::cmp_greater_equal(Min, std::numeric_limits<W>::min() / 2) &&
                                      std::cmp_less_equal(Max, std::numeric_limits<W>::max() / 2);

// Narrowest signed type (32 or 64 bits) that holds all intermediate results for [Min, Max]; void if none.
template <auto Min, auto Max>
using WideFor = std::conditional_t<
    RANGE_FITS_IN<std::int32_t, Min, Max>,
    std::int32_t,
    std::conditional_t<RANGE_FITS_IN<std::int64_t, Min, Max>, std::int64_t, void>>;

// Checks val against [Min, Max] before narrowing to T.
template <std::integral T, auto Min, auto Max, std::integral U> constexpr T check_range(U val) noexcept {
    if (std::is_constant_evaluated() && (std::cmp_less(val, Min) || std::cmp_greater(val, Max))) {
        constraint_error_in_constant_expression();
    }
#ifdef ENABLE_RUNTIME_RANGE_CHECKS
    if (std::cmp_less(val, Min)) {
        return Min;
    }
    if (std::cmp_greater(val, Max)) {
        return Max;
    }
#else
    assert(std::cmp_greater_equal(val, Min) && "Value below minimum bound");
    assert(std::cmp_less_equal(val, Max) && "Value above maximum bound");
#endif
    return static_cast<T>(val);
}

// Reports a sum/difference outside W (Ada's Constraint_Error on base-range overflow); returns the nearest limit,
// so release builds never perform signed overflow.
template <std::signed_integral W> constexpr W base_overflow(bool positive) noexcept {
    if (std::is_constant_evaluated()) {
        constraint_error_in_constant_expression();
    }
#ifndef ENABLE_RUNTIME_RANGE_CHECKS
    assert(false && "Base range overflow");
#endif
    return positive ? std::numeric_limits<W>::max() : std::numeric_limits<W>::min();
}

template <std::signed_integral W> constexpr W base_add(W a, W b) noexcept {
    if (b > 0 && a > std::numeric_limits<W>::max() - b) {
        return base_overflow<W>(true);
    }
    if (b < 0 && a < std::numeric_limits<W>::min() - b) {
        return base_overflow<W>(false);
    }
    return a + b;
}

template <std::signed_integral W> constexpr W base_sub(W a, W b) noexcept {
    if (b < 0 && a > std::numeric_limits<W>::max() + b) {
        return base_overflow<W>(true);
    }
    if (b > 0 && a < std::numeric_limits<W>::min() + b) {
        return base_overflow<W>(false);
    }
    return a - b;
}

} // namespace detail

/// Range-constrained integer, equivalent to Ada `type X is range Min .. Max`.
///
/// @tparam T   Storage type
/// @tparam Min Lowest valid value (inclusive)
/// @tparam Max Highest valid value (inclusive)
/// @tparam Tag Distinct empty type per Ada type; prevents mixing types with equal ranges
template <std::integral T, T Min, T Max, typename Tag> class RangedInteger {
    static_assert(Min <= Max, "Min must be <= Max");

  public:
    using ValueType = T;
    using TagType = Tag;
    using Wide = detail::WideFor<Min, Max>;
    static_assert(!std::is_void_v<Wide>, "Range too wide: sums/differences of two values must fit in int64_t");

    static constexpr T MIN_VALUE = Min;
    static constexpr T MAX_VALUE = Max;

    /// Ada X'Base: the result of + and -, not limited to [Min, Max]; checked when converted back to RangedInteger.
    /// No get(): reading the value means storing it in RangedInteger, which is where the range check happens.
    class Base {
      public:
        [[nodiscard]] friend constexpr bool operator==(const Base&, const Base&) noexcept = default;
        [[nodiscard]] friend constexpr auto operator<=>(const Base&, const Base&) noexcept = default;

      private:
        friend class RangedInteger; // includes the hidden friends defined inside RangedInteger ([class.friend]/2)
        explicit constexpr Base(Wide val) noexcept : value_{val} {}

        Wide value_;
    };

    /// Initializes to Min.
    constexpr RangedInteger() noexcept : value_{Min} {}

    /// Accepts any integer type so the range check sees the caller's real value, not one already narrowed to T.
    template <std::integral U>
        requires(!std::same_as<U, bool>)
    explicit constexpr RangedInteger(U val) noexcept : value_{validate(val)} {}

    /// Implicit, like Ada assignment of a base-type expression: `Percent p = a + b - c;` checks the final value only.
    constexpr RangedInteger(const Base& val) noexcept // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        : value_{validate(val.value_)} {}

    [[nodiscard]] constexpr T get() const noexcept { return value_; }

    [[nodiscard]] friend constexpr bool operator==(const RangedInteger&, const RangedInteger&) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(const RangedInteger&, const RangedInteger&) noexcept = default;

    // Comparing a base-type result does not range-check it, as in Ada.
    [[nodiscard]] friend constexpr bool operator==(const Base& lhs, const RangedInteger& rhs) noexcept {
        return lhs.value_ == wide(rhs.value_);
    }
    [[nodiscard]] friend constexpr auto operator<=>(const Base& lhs, const RangedInteger& rhs) noexcept {
        return lhs.value_ <=> wide(rhs.value_);
    }

    // Hidden friends rather than members so operands that convert to RangedInteger (e.g. ada::IntegerSubtype) work.
    // One overload per operand pairing: an IntegerSubtype cannot convert to Base (two user-defined conversions).
    [[nodiscard]] friend constexpr Base operator+(const RangedInteger& lhs, const RangedInteger& rhs) noexcept {
        return Base{detail::base_add(wide(lhs.value_), wide(rhs.value_))};
    }
    [[nodiscard]] friend constexpr Base operator+(const Base& lhs, const RangedInteger& rhs) noexcept {
        return Base{detail::base_add(lhs.value_, wide(rhs.value_))};
    }
    [[nodiscard]] friend constexpr Base operator+(const RangedInteger& lhs, const Base& rhs) noexcept {
        return Base{detail::base_add(wide(lhs.value_), rhs.value_)};
    }
    [[nodiscard]] friend constexpr Base operator+(const Base& lhs, const Base& rhs) noexcept {
        return Base{detail::base_add(lhs.value_, rhs.value_)};
    }

    [[nodiscard]] friend constexpr Base operator-(const RangedInteger& lhs, const RangedInteger& rhs) noexcept {
        return Base{detail::base_sub(wide(lhs.value_), wide(rhs.value_))};
    }
    [[nodiscard]] friend constexpr Base operator-(const Base& lhs, const RangedInteger& rhs) noexcept {
        return Base{detail::base_sub(lhs.value_, wide(rhs.value_))};
    }
    [[nodiscard]] friend constexpr Base operator-(const RangedInteger& lhs, const Base& rhs) noexcept {
        return Base{detail::base_sub(wide(lhs.value_), rhs.value_)};
    }
    [[nodiscard]] friend constexpr Base operator-(const Base& lhs, const Base& rhs) noexcept {
        return Base{detail::base_sub(lhs.value_, rhs.value_)};
    }

    // Compound assignment stores into the object, so the result is checked immediately.
    constexpr RangedInteger& operator+=(const RangedInteger& rhs) noexcept {
        value_ = validate(wide(value_) + wide(rhs.value_));
        return *this;
    }
    constexpr RangedInteger& operator+=(const Base& rhs) noexcept {
        value_ = validate(detail::base_add(wide(value_), rhs.value_));
        return *this;
    }

    constexpr RangedInteger& operator-=(const RangedInteger& rhs) noexcept {
        value_ = validate(wide(value_) - wide(rhs.value_));
        return *this;
    }
    constexpr RangedInteger& operator-=(const Base& rhs) noexcept {
        value_ = validate(detail::base_sub(wide(value_), rhs.value_));
        return *this;
    }

    constexpr RangedInteger& operator++() noexcept {
        value_ = validate(wide(value_) + 1);
        return *this;
    }

    constexpr RangedInteger operator++(int) noexcept {
        RangedInteger old = *this;
        ++(*this);
        return old;
    }

    constexpr RangedInteger& operator--() noexcept {
        value_ = validate(wide(value_) - 1);
        return *this;
    }

    constexpr RangedInteger operator--(int) noexcept {
        RangedInteger old = *this;
        --(*this);
        return old;
    }

  private:
    // Safe for any T: RANGE_FITS_IN guarantees every value in [Min, Max] fits in Wide.
    static constexpr Wide wide(T v) noexcept { return static_cast<Wide>(v); }

    template <std::integral U> static constexpr T validate(U val) noexcept {
        return detail::check_range<T, Min, Max>(val);
    }

    T value_;
};

} // namespace ada

#endif // ADA_RANGED_INTEGER_HPP
