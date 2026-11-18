// integer_subtype.hpp - Ada integer subtypes for C++20.
//
// Maps `subtype S is Parent range Min .. Max` to `using S = ada::IntegerSubtype<Parent, Min, Max>;`.
// Unlike ada::RangedInteger (Ada `type X is range`, a new type), a subtype is its parent's type with a
// narrower range:
// - converts implicitly to its base type, so arithmetic is the base type's arithmetic;
// - converts implicitly from the base type and from other subtypes of the same base, with a range check.
// Parent may be a built-in integer, an ada::RangedInteger, or another ada::IntegerSubtype.
// Out-of-range values behave as for ada::RangedInteger.
//
// Float and fixed-point subtypes (e.g. `subtype Probability is Float range 0.0 .. 1.0`) would follow this
// template's pattern, ideally by generalizing it to one subtype template keyed on the parent kind.

#ifndef ADA_INTEGER_SUBTYPE_HPP
#define ADA_INTEGER_SUBTYPE_HPP

#include <concepts>
#include <limits>
#include <type_traits>

#include "ada/ranged_integer.hpp"

namespace ada {

namespace detail {

template <typename T> inline constexpr bool IS_RANGED_INTEGER = false;
template <typename T, T Min, T Max, typename Tag>
inline constexpr bool IS_RANGED_INTEGER<RangedInteger<T, Min, Max, Tag>> = true;

// Base type, value type and range of a subtype's parent.
template <typename Parent> struct ParentInfo;

template <typename Parent>
    requires std::integral<Parent>
struct ParentInfo<Parent> {
    using Base = Parent;
    using Value = Parent;
    static constexpr Value FIRST = std::numeric_limits<Parent>::min();
    static constexpr Value LAST = std::numeric_limits<Parent>::max();
};

template <typename Parent>
    requires IS_RANGED_INTEGER<Parent>
struct ParentInfo<Parent> {
    using Base = Parent;
    using Value = typename Parent::ValueType;
    static constexpr Value FIRST = Parent::MIN_VALUE;
    static constexpr Value LAST = Parent::MAX_VALUE;
};

} // namespace detail

template <
    typename Parent,
    typename detail::ParentInfo<Parent>::Value Min,
    typename detail::ParentInfo<Parent>::Value Max>
class IntegerSubtype;

namespace detail {

template <typename T> inline constexpr bool IS_INTEGER_SUBTYPE = false;
template <typename Parent, typename ParentInfo<Parent>::Value Min, typename ParentInfo<Parent>::Value Max>
inline constexpr bool IS_INTEGER_SUBTYPE<IntegerSubtype<Parent, Min, Max>> = true;

// A subtype of a subtype shares the outer parent's base type.
template <typename Parent>
    requires IS_INTEGER_SUBTYPE<Parent>
struct ParentInfo<Parent> {
    using Base = typename Parent::BaseType;
    using Value = typename Parent::ValueType;
    static constexpr Value FIRST = Parent::MIN_VALUE;
    static constexpr Value LAST = Parent::MAX_VALUE;
};

// Result type of + and - on a subtype's base type: RangedInteger::Base, or void for a built-in base.
template <typename BaseType> struct ArithmeticResult {
    using Type = void;
};
template <typename BaseType>
    requires IS_RANGED_INTEGER<BaseType>
struct ArithmeticResult<BaseType> {
    using Type = typename BaseType::Base;
};

} // namespace detail

/// Range-constrained view of a parent type, equivalent to Ada `subtype S is Parent range Min .. Max`.
///
/// @tparam Parent Built-in integer, ada::RangedInteger, or ada::IntegerSubtype
/// @tparam Min    Lowest valid value (inclusive); must lie within Parent's range
/// @tparam Max    Highest valid value (inclusive); must lie within Parent's range
template <
    typename Parent,
    typename detail::ParentInfo<Parent>::Value Min,
    typename detail::ParentInfo<Parent>::Value Max>
class IntegerSubtype {
    using Info = detail::ParentInfo<Parent>;

  public:
    using ParentType = Parent;
    using BaseType = typename Info::Base;
    using ValueType = typename Info::Value;

    static_assert(Min <= Max, "Min must be <= Max");
    static_assert(Min >= Info::FIRST && Max <= Info::LAST, "IntegerSubtype range must lie within the parent's range");

    static constexpr ValueType MIN_VALUE = Min;
    static constexpr ValueType MAX_VALUE = Max;

    /// Initializes to Min.
    constexpr IntegerSubtype() noexcept : value_{Min} {}

    /// Implicit from any integer when the base is a built-in integer; checks the caller's real value.
    template <std::integral U>
        requires(std::integral<BaseType> && !std::same_as<U, bool>)
    constexpr IntegerSubtype(U val) noexcept
        : value_{check(val)} {} // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)

    /// Explicit from an integer when the base is an ada::RangedInteger, matching RangedInteger's constructor.
    template <std::integral U>
        requires(!std::integral<BaseType> && !std::same_as<U, bool>)
    explicit constexpr IntegerSubtype(U val) noexcept : value_{check(val)} {}

    /// Implicit from the base ada::RangedInteger.
    constexpr IntegerSubtype(
        const BaseType& val) noexcept // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        requires(!std::integral<BaseType>)
        : value_{check(val.get())} {}

    /// Implicit from the base RangedInteger's arithmetic result, so `LowBattery low = a - b;` checks only low.
    /// Checked against the base range, then this subtype's (which lies within it), so the outcome is the same.
    template <typename B>
        requires std::same_as<B, typename detail::ArithmeticResult<BaseType>::Type>
    constexpr IntegerSubtype(const B& val) noexcept // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        : value_{check(BaseType{val}.get())} {}

    /// Implicit from another subtype of the same base type.
    template <typename Other>
        requires(
            detail::IS_INTEGER_SUBTYPE<Other> && !std::same_as<Other, IntegerSubtype> &&
            std::same_as<typename Other::BaseType, BaseType>)
    constexpr IntegerSubtype(
        const Other& other) noexcept // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        : value_{check(other.get())} {}

    [[nodiscard]] constexpr ValueType get() const noexcept { return value_; }

    /// Implicit: a subtype's values are values of its base type.
    constexpr operator BaseType() const noexcept { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        if constexpr (std::integral<BaseType>) {
            return value_;
        } else {
            return BaseType{value_};
        }
    }

  private:
    template <std::integral U> static constexpr ValueType check(U val) noexcept {
        return detail::check_range<ValueType, Min, Max>(val);
    }

    ValueType value_;
};

/// Ada's predefined subtypes of Integer.
using Natural = IntegerSubtype<int, 0, std::numeric_limits<int>::max()>;
using Positive = IntegerSubtype<int, 1, std::numeric_limits<int>::max()>;

} // namespace ada

#endif // ADA_INTEGER_SUBTYPE_HPP
