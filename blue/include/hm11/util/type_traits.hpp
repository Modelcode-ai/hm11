// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

#ifndef HM11_UTIL_TYPE_TRAITS_HPP
#define HM11_UTIL_TYPE_TRAITS_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace hm11::util {

// ---------- Validator utilities ----------
namespace validators {
struct HexValidator {
    static constexpr bool validate(char c) noexcept {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }
};

struct DigitValidator {
    static constexpr bool validate(char c) noexcept { return c >= '0' && c <= '9'; }
};
} // namespace validators

// expose validators under utils namespace as required by the specification
namespace utils {
using HexValidator = validators::HexValidator;
using DigitValidator = validators::DigitValidator;
} // namespace utils

// ---------- RangedInteger ----------
/**
 * @brief Strongly typed integer with compile‑time range constraints.
 */
template <typename T, T Min, T Max, typename Tag> class RangedInteger {
    static_assert(std::is_integral_v<T>, "RangedInteger requires an integral type");
    static_assert(Min <= Max, "Min must be less than or equal to Max");

  public:
    using ValueType = T;

    constexpr RangedInteger() noexcept = default;
    constexpr ~RangedInteger() = default;

    constexpr explicit RangedInteger(T v) noexcept { set(v); }

    constexpr RangedInteger(const RangedInteger&) = default;
    constexpr RangedInteger(RangedInteger&&) noexcept = default;
    constexpr RangedInteger& operator=(const RangedInteger&) = default;
    constexpr RangedInteger& operator=(RangedInteger&&) noexcept = default;

    constexpr RangedInteger& operator=(T v) noexcept {
        set(v);
        return *this;
    }

    constexpr operator T() const noexcept { return value; }

    // arithmetic operators (range checking performed via set in the resulting object)
    constexpr RangedInteger operator+(const RangedInteger& other) const noexcept {
        return RangedInteger(static_cast<T>(value + other.value));
    }
    constexpr RangedInteger operator-(const RangedInteger& other) const noexcept {
        return RangedInteger(static_cast<T>(value - other.value));
    }
    constexpr RangedInteger operator*(const RangedInteger& other) const noexcept {
        return RangedInteger(static_cast<T>(value * other.value));
    }
    constexpr RangedInteger operator/(const RangedInteger& other) const noexcept {
        assert(other.value != 0 && "Division by zero");
        return RangedInteger(static_cast<T>(value / other.value));
    }
    constexpr RangedInteger operator%(const RangedInteger& other) const noexcept {
        assert(other.value != 0 && "Modulo by zero");
        return RangedInteger(static_cast<T>(value % other.value));
    }

    // comparison operators
    constexpr bool operator==(const RangedInteger& other) const noexcept { return value == other.value; }
    constexpr bool operator!=(const RangedInteger& other) const noexcept { return value != other.value; }
    constexpr bool operator<(const RangedInteger& other) const noexcept { return value < other.value; }
    constexpr bool operator<=(const RangedInteger& other) const noexcept { return value <= other.value; }
    constexpr bool operator>(const RangedInteger& other) const noexcept { return value > other.value; }
    constexpr bool operator>=(const RangedInteger& other) const noexcept { return value >= other.value; }

  private:
    T value{};
    constexpr void set(T v) noexcept {
#if defined(ENABLE_RUNTIME_RANGE_CHECKS)
        if (v < Min)
            v = Min;
        if (v > Max)
            v = Max;
        value = v;
#else
        assert(v >= Min && v <= Max && "RangedInteger value out of bounds");
        value = v;
#endif
    }
};

// ---------- FixedString ----------
/**
 * @brief Fixed‑size character array with compile‑time validation.
 */
template <std::size_t N, typename Validator> class FixedString {
  public:
    constexpr FixedString() noexcept = default;
    constexpr ~FixedString() = default;

    constexpr explicit FixedString(std::string_view sv) noexcept {
        assert(sv.size() == N && "FixedString requires exactly N characters");
        for (std::size_t i = 0; i < N; ++i) {
            char c = sv[i];
            assert(Validator::validate(c) && "FixedString character validation failed");
            p_data[i] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        p_data[N] = '\0'; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    constexpr FixedString(const FixedString&) = default;
    constexpr FixedString(FixedString&&) noexcept = default;
    constexpr FixedString& operator=(const FixedString&) = default;
    constexpr FixedString& operator=(FixedString&&) noexcept = default;

    constexpr const std::array<char, N>& data() const noexcept { return p_data; }
    constexpr std::string_view view() const noexcept { return std::string_view(p_data.data(), N); }
    constexpr operator std::string_view() const noexcept { return view(); }
    constexpr char operator[](std::size_t idx) const noexcept { return p_data[idx]; }

  private:
    std::array<char, N + 1> p_data{}; // includes terminating null for convenience
};

// ---------- Tag structs for strong typing ----------
struct PercentTag {};
struct TemperatureTag {};
struct LatencyTag {};
struct MACAddressTag {};
struct UUIDTag {};
struct PINTag {};
struct LayerConnectionLatencyTag {};
struct PioCollectionRateTag {};
struct MacWhiteListIndexTag {};
struct DiscoveryTimeTag {};
struct DiscoveryIndexTag {};
struct PioNumberTag {};

// ---------- Specification‑compliant type aliases ----------
using Percent = RangedInteger<std::uint8_t, 0, 100, PercentTag>;
using Temperature = RangedInteger<std::uint8_t, 0, 120, TemperatureTag>;
using Latency = RangedInteger<std::uint16_t, 0, 65535, LatencyTag>;
using LayerConnectionLatency = RangedInteger<std::uint8_t, 0, 4, LayerConnectionLatencyTag>;
using PioCollectionRate = RangedInteger<std::uint8_t, 0, 99, PioCollectionRateTag>;
using MacWhiteListIndex = RangedInteger<std::uint8_t, 1, 3, MacWhiteListIndexTag>;
using DiscoveryTime = RangedInteger<std::uint8_t, 1, 9, DiscoveryTimeTag>;
using DiscoveryIndex = RangedInteger<std::uint8_t, 0, 5, DiscoveryIndexTag>;
using PioNumber = RangedInteger<std::uint8_t, 1, 3, PioNumberTag>;
using UUID = FixedString<4, utils::HexValidator>;
using MACAddress = FixedString<12, utils::HexValidator>;
using PINType = FixedString<6, utils::DigitValidator>;
using IBeaconUuid = FixedString<32, utils::HexValidator>;
using HandleType = FixedString<4, utils::HexValidator>;
using VersionType = FixedString<4, utils::HexValidator>;
using MeasuredPower = FixedString<2, utils::HexValidator>;
using AdvertisingFlag = FixedString<2, utils::HexValidator>;
using AdvertisingTemperatureType = FixedString<2, utils::HexValidator>;
using HumiInformation = FixedString<2, utils::HexValidator>;
using PIONumbers = FixedString<3, utils::HexValidator>;
using ConnectTimeout = FixedString<6, utils::DigitValidator>;
using AdvertisementData = FixedString<12, utils::HexValidator>;
using CharacteristicType = FixedString<4, utils::HexValidator>;
using InternalTemperatureType = FixedString<7, utils::DigitValidator>;

// ---------- Static assertions for compile‑time verification ----------
static_assert(std::is_same_v<Percent::ValueType, std::uint8_t>, "Percent underlying type must be uint8_t");
static_assert(std::is_same_v<Temperature::ValueType, std::uint8_t>, "Temperature underlying type must be uint8_t");
static_assert(std::is_same_v<Latency::ValueType, std::uint16_t>, "Latency underlying type must be uint16_t");
static_assert(std::is_same_v<MACAddress, FixedString<12, utils::HexValidator>>, "MACAddress alias mismatch");
static_assert(std::is_same_v<PINType, FixedString<6, utils::DigitValidator>>, "PINType alias mismatch");

} // namespace hm11::util

#endif // HM11_UTIL_TYPE_TRAITS_HPP
