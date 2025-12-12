#pragma once

// HM11 Driver String Construction Utilities
// Implements stack-based AT command construction without heap allocation
// Resolves Design Decision #1 (AT Command Construction Strategy)

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "../types/type_aliases.hpp"

namespace hm11::util {

/// @brief Fixed-capacity string builder for stack-based AT command construction
///
/// This class provides efficient string concatenation without heap allocation,
/// matching Ada's stack-based string building approach. All operations are
/// constexpr where possible for compile-time optimization.
///
/// @tparam Capacity Maximum number of characters that can be stored
template <std::size_t Capacity> class StringBuilder {
    static_assert(Capacity > 0, "StringBuilder capacity must be greater than 0");
    static_assert(Capacity <= 512, "StringBuilder capacity should be reasonable (max 512 chars for AT commands)");

  public:
    using SizeType = std::size_t;
    static constexpr SizeType MAX_CAPACITY = Capacity;

    /// @brief Default constructor - creates empty string builder
    constexpr StringBuilder() noexcept : buffer{}, length(0) {}

    /// @brief Constructor with initial string
    /// @param str Initial string content
    explicit StringBuilder(std::string_view str) : buffer{}, length(0) { append(str); }

    /// @brief Copy constructor
    constexpr StringBuilder(const StringBuilder&) = default;

    /// @brief Copy assignment
    constexpr StringBuilder& operator=(const StringBuilder&) = default;

    /// @brief Move constructor
    constexpr StringBuilder(StringBuilder&&) noexcept = default;

    /// @brief Move assignment
    constexpr StringBuilder& operator=(StringBuilder&&) noexcept = default;

    /// @brief Destructor
    ~StringBuilder() = default;

    /// @brief Get current length
    constexpr SizeType size() const noexcept { return length; }

    /// @brief Check if empty
    constexpr bool empty() const noexcept { return length == 0; }

    /// @brief Get remaining capacity
    constexpr SizeType remaining_capacity() const noexcept { return Capacity - length; }

    /// @brief Check if at maximum capacity
    constexpr bool full() const noexcept { return length == Capacity; }

    /// @brief Clear the buffer
    constexpr void clear() noexcept {
        length = 0;
        // Optional: clear buffer for security (disabled for performance)
        // buffer.fill('\0');
    }

    /// @brief Get string view of current content
    constexpr std::string_view view() const noexcept { return std::string_view(buffer.data(), length); }

    /// @brief Get C string (null-terminated)
    /// Note: This method ensures null-termination by using a separate buffer
    const char* c_str() const {
        static thread_local std::array<char, Capacity + 1> temp_buffer;
        std::copy_n(buffer.data(), length, temp_buffer.data());
        temp_buffer[length] = '\0';
        return temp_buffer.data();
    }

    /// @brief Get pointer to underlying data
    constexpr const char* data() const noexcept { return buffer.data(); }

    /// @brief Append string view
    /// @param str String to append
    /// @throws std::length_error if would exceed capacity
    StringBuilder& append(std::string_view str) {
        if (length + str.size() > Capacity) {
            throw std::length_error("StringBuilder: append would exceed capacity");
        }

        for (SizeType i = 0; i < str.size(); ++i) {
            buffer[length + i] = str[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        length += str.size();
        return *this;
    }

    /// @brief Append single character
    /// @param c Character to append
    /// @throws std::length_error if at capacity
    StringBuilder& append(char c) {
        if (length >= Capacity) {
            throw std::length_error("StringBuilder: append would exceed capacity");
        }

        buffer[length++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        return *this;
    }

    /// @brief Append integer as decimal string
    /// @param value Integer value to append
    template <std::integral T> StringBuilder& append_int(T value) {
        if constexpr (std::is_signed_v<T>) {
            if (value < 0) {
                append('-');
                return append_int(static_cast<std::make_unsigned_t<T>>(-value));
            }
        }

        return append_int_positive(static_cast<std::make_unsigned_t<T>>(value));
    }

    /// @brief Append hex character representation of integer
    /// @param value Integer value (0-15)
    StringBuilder& append_hex_char(uint8_t value) {
        assert(value <= 15 && "Value must be 0-15 for hex character");
        if (value < 10) {
            return append(static_cast<char>('0' + value));
        }
        return append(static_cast<char>('A' + (value - 10)));
    }

    /// @brief Append integer as hex string (uppercase)
    /// @param value Integer value to append as hex
    template <std::integral T> StringBuilder& append_hex(T value) {
        constexpr int HEX_DIGITS = sizeof(T) * 2;
        for (int i = HEX_DIGITS - 1; i >= 0; --i) {
            const auto NIBBLE = static_cast<uint8_t>((value >> (i * 4)) & 0xF);
            append_hex_char(NIBBLE);
        }
        return *this;
    }

    /// @brief Append single character (alias for append for compatibility)
    /// @param c Character to append
    /// @throws std::length_error if at capacity
    StringBuilder& append_char(char c) { return append(c); }

    /// @brief Operator += for string_view
    StringBuilder& operator+=(std::string_view str) { return append(str); }

    /// @brief Operator += for character
    StringBuilder& operator+=(char c) { return append(c); }

    /// @brief Operator += for another StringBuilder
    template <SizeType OtherCapacity> StringBuilder& operator+=(const StringBuilder<OtherCapacity>& other) {
        return append(other.view());
    }

  private:
    std::array<char, Capacity> buffer;
    SizeType length;

    /// @brief Helper to append positive integer
    template <std::unsigned_integral T> StringBuilder& append_int_positive(T value) {
        if (value == 0) {
            return append('0');
        }

        // Calculate number of digits
        T temp = value;
        SizeType digit_count = 0;
        while (temp > 0) {
            temp /= 10;
            ++digit_count;
        }

        if (length + digit_count > Capacity) {
            throw std::length_error("StringBuilder: integer append would exceed capacity");
        }

        // Build digits in reverse order
        SizeType start_pos = length + digit_count - 1;
        length += digit_count;

        while (value > 0) {
            buffer[start_pos--] = static_cast<char>(
                '0' + (value % 10)); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            value /= 10;
        }

        return *this;
    }
};

// ============================================================================
// String conversion utilities for HM11 types
// ============================================================================

/// @brief Convert Role enum to string representation
/// Maps to Ada: Image (Role'Pos (Value))
constexpr std::string_view to_string(Role role) noexcept {
    switch (role) {
        case Role::Peripheral:
            return "0";
        case Role::Central:
            return "1";
    }
    return "0"; // Default fallback
}

/// @brief Convert boolean to string representation
/// Maps to Ada: Image (Value) -> "1" if True else "0"
constexpr std::string_view to_string(bool value) noexcept {
    return value ? "1" : "0";
}

/// @brief Convert AdvertisingInterval to character representation
/// Maps to Ada: From_Advertising_Interval array lookup
constexpr char to_char(AdvertisingInterval interval) noexcept {
    switch (interval) {
        case AdvertisingInterval::Ms100:
            return '0';
        case AdvertisingInterval::Ms211:
            return '1';
        case AdvertisingInterval::Ms252:
            return '2';
        case AdvertisingInterval::Ms318:
            return '3';
        case AdvertisingInterval::Ms417:
            return '4';
        case AdvertisingInterval::Ms546:
            return '5';
        case AdvertisingInterval::Ms760:
            return '6';
        case AdvertisingInterval::Ms852:
            return '7';
        case AdvertisingInterval::Ms1022:
            return '8';
        case AdvertisingInterval::Ms1285:
            return '9';
        case AdvertisingInterval::Ms2000:
            return 'A';
        case AdvertisingInterval::Ms3000:
            return 'B';
        case AdvertisingInterval::Ms4000:
            return 'C';
        case AdvertisingInterval::Ms5000:
            return 'D';
        case AdvertisingInterval::Ms6000:
            return 'E';
        case AdvertisingInterval::Ms7000:
            return 'F';
    }
    return '0'; // Default fallback
}

/// @brief Convert AdvertisingType to string representation
constexpr std::string_view to_string(AdvertisingType type) noexcept {
    switch (type) {
        case AdvertisingType::AdvertisingScanResponseConnectable:
            return "0";
        case AdvertisingType::LastDeviceConnect:
            return "1";
        case AdvertisingType::AdvertisingScanResponse:
            return "2";
        case AdvertisingType::Advertising:
            return "3";
    }
    return "0"; // Default fallback
}

/// @brief Convert MacAddressType to string representation
constexpr std::string_view to_string(MacAddressType type) noexcept {
    switch (type) {
        case MacAddressType::NormalAddress:
            return "0";
        case MacAddressType::StaticMac:
            return "1";
        case MacAddressType::StaticRandomMac:
            return "2";
        case MacAddressType::RandomMac:
            return "3";
    }
    return "0"; // Default fallback
}

/// @brief Convert UartBaudRate to string representation
constexpr std::string_view to_string(UartBaudRate rate) noexcept {
    switch (rate) {
        case UartBaudRate::Br9600:
            return "0";
        case UartBaudRate::Br19200:
            return "1";
        case UartBaudRate::Br38400:
            return "2";
        case UartBaudRate::Br57600:
            return "3";
        case UartBaudRate::Br115200:
            return "4";
        case UartBaudRate::Br4800:
            return "5";
        case UartBaudRate::Br2400:
            return "6";
        case UartBaudRate::Br1200:
            return "7";
        case UartBaudRate::Br230400:
            return "8";
    }
    return "0"; // Default fallback
}

/// @brief Convert SendDataMethod to string representation
constexpr std::string_view to_string(SendDataMethod method) noexcept {
    switch (method) {
        case SendDataMethod::Write:
            return "WR";
        case SendDataMethod::WriteWithoutResponse:
            return "WN";
        case SendDataMethod::Indicate:
            return "IN";
        case SendDataMethod::Notify:
            return "NO";
    }
    return "WR"; // Default fallback
}

/// @brief Convert SendDataCharacteristic to string representation
constexpr std::string_view to_string(SendDataCharacteristic method) noexcept {
    switch (method) {
        case SendDataCharacteristic::Write:
            return "WR";
        case SendDataCharacteristic::WriteWithoutResponse:
            return "WN";
    }
    return "WR"; // Default fallback
}

/// @brief Convert SensorType to string representation
/// Maps to Ada: Image (Sensor_Type'Pos (Value))
constexpr std::string_view to_string(SensorType type) noexcept {
    switch (type) {
        case SensorType::None:
            return "0";
        case SensorType::DHT11:
            return "1";
        case SensorType::DS18B20:
            return "2";
    }
    return "0"; // Default fallback
}

/// @brief Convert ModulePower to string representation
constexpr std::string_view to_string(ModulePower power) noexcept {
    switch (power) {
        case ModulePower::DbmMinus23:
            return "0";
        case ModulePower::DbmMinus6:
            return "1";
        case ModulePower::Dbm0:
            return "2";
        case ModulePower::Dbm6:
            return "3";
    }
    return "2"; // Default fallback
}

/// @brief Convert OutputPower to string representation
constexpr std::string_view to_string(OutputPower power) noexcept {
    switch (power) {
        case OutputPower::NormalPower:
            return "0";
        case OutputPower::MaxPower:
            return "1";
    }
    return "1"; // Default fallback
}

/// @brief Convert AdvertisingMode to string representation
constexpr std::string_view to_string(AdvertisingMode mode) noexcept {
    switch (mode) {
        case AdvertisingMode::NormalAdvertising:
            return "0";
        case AdvertisingMode::ReliableAdvertising:
            return "1";
    }
    return "0"; // Default fallback
}

/// @brief Convert ParityBit to string representation
constexpr std::string_view to_string(ParityBit parity) noexcept {
    switch (parity) {
        case ParityBit::None:
            return "0";
        case ParityBit::Odd:
            return "1";
        case ParityBit::Even:
            return "2";
    }
    return "0"; // Default fallback
}

/// @brief Convert StopBit to string representation
constexpr std::string_view to_string(StopBit bit) noexcept {
    switch (bit) {
        case StopBit::OneStopBit:
            return "0";
        case StopBit::TwoStopBit:
            return "1";
    }
    return "0"; // Default fallback
}

/// @brief Convert LinkLayerConnectionInterval to string representation
constexpr std::string_view to_string(LinkLayerConnectionInterval interval) noexcept {
    switch (interval) {
        case LinkLayerConnectionInterval::Ms7:
            return "0";
        case LinkLayerConnectionInterval::Ms10:
            return "1";
        case LinkLayerConnectionInterval::Ms15:
            return "2";
        case LinkLayerConnectionInterval::Ms20:
            return "3";
        case LinkLayerConnectionInterval::Ms25:
            return "4";
        case LinkLayerConnectionInterval::Ms30:
            return "5";
        case LinkLayerConnectionInterval::Ms35:
            return "6";
        case LinkLayerConnectionInterval::Ms40:
            return "7";
        case LinkLayerConnectionInterval::Ms45:
            return "8";
        case LinkLayerConnectionInterval::Ms4000:
            return "9";
    }
    return "3"; // Default fallback (20ms)
}

/// @brief Convert ConnectionSupervisionTimeout to string representation
constexpr std::string_view to_string(ConnectionSupervisionTimeout timeout) noexcept {
    switch (timeout) {
        case ConnectionSupervisionTimeout::Ms100:
            return "0";
        case ConnectionSupervisionTimeout::Ms1000:
            return "1";
        case ConnectionSupervisionTimeout::Ms2000:
            return "2";
        case ConnectionSupervisionTimeout::Ms3000:
            return "3";
        case ConnectionSupervisionTimeout::Ms4000:
            return "4";
        case ConnectionSupervisionTimeout::Ms5000:
            return "5";
        case ConnectionSupervisionTimeout::Ms6000:
            return "6";
    }
    return "6"; // Default fallback (6000ms)
}

/// @brief Convert PioOutput to string representation
constexpr std::string_view to_string(PioOutput output) noexcept {
    switch (output) {
        case PioOutput::Low:
            return "0";
        case PioOutput::High:
            return "1";
    }
    return "0"; // Default fallback
}

// ============================================================================
// Type aliases for common AT command string builders
// ============================================================================

/// @brief Standard AT command string builder (64 chars should be sufficient for most commands)
using ATCommandBuilder = StringBuilder<64>;

/// @brief Large AT command string builder for complex commands with data
using LargeATCommandBuilder = StringBuilder<128>;

/// @brief Small string builder for short parameter strings
using SmallStringBuilder = StringBuilder<32>;

} // namespace hm11::util