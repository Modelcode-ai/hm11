/**
 * @file at_command_builder.hpp
 * @brief Utilities for building AT commands for the HM-11 module
 *
 * This file provides utility functions and constants for constructing AT commands
 * to be sent to the HM-11 Bluetooth Low Energy module. It implements Decision 5
 * (AT Command Construction) from the architecture specification.
 */

#ifndef HM11_UTIL_AT_COMMAND_BUILDER_HPP
#define HM11_UTIL_AT_COMMAND_BUILDER_HPP

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "../driver.hpp"
#include "../enum_types.hpp"
#include "../hm11.hpp"
#include "fixed_string.hpp"

// Forward declarations for enums
namespace hm11 {
enum class OutputPower : std::uint8_t;
enum class UartSleepType : std::uint8_t;
} // namespace hm11

/**
 * @brief Namespace for AT command construction constants and utilities
 */
namespace hm11::util::at {

/**
 * @brief Maximum AT command length
 */
constexpr std::size_t MAX_COMMAND_LENGTH = 128;

/**
 * @brief Base AT command strings
 */
namespace cmd {
constexpr std::string_view AT = "AT";
constexpr std::string_view TEST = "AT";
constexpr std::string_view RESET = "AT+RESET";
constexpr std::string_view RENEW = "AT+RENEW";
constexpr std::string_view VERSION = "AT+VERS?";

constexpr std::string_view ROLE = "AT+ROLE";
constexpr std::string_view NAME = "AT+NAME";
constexpr std::string_view PASS = "AT+PASS";
constexpr std::string_view BAUD = "AT+BAUD";
constexpr std::string_view IMME = "AT+IMME";
constexpr std::string_view MODE = "AT+MODE";
constexpr std::string_view ADTY = "AT+ADTY";
constexpr std::string_view SCAN = "AT+SCAN";
constexpr std::string_view SHOW = "AT+SHOW";
constexpr std::string_view CON = "AT+CON";
constexpr std::string_view DISC = "AT+DISC";
constexpr std::string_view MAC = "AT+MAC";
constexpr std::string_view FIND = "AT+FIND";
constexpr std::string_view ADDR = "AT+ADDR";
constexpr std::string_view AINT = "AT+AINT";
constexpr std::string_view NOTI = "AT+NOTI";
constexpr std::string_view UUID = "AT+UUID";
constexpr std::string_view IBE = "AT+IBE";
constexpr std::string_view IBEA = "AT+IBEA";
constexpr std::string_view MARJ = "AT+MARJ";
constexpr std::string_view MINO = "AT+MINO";
constexpr std::string_view MEAS = "AT+MEAS";
constexpr std::string_view PWRM = "AT+PWRM";
constexpr std::string_view POWE = "AT+POWE";
constexpr std::string_view DELO = "AT+DELO";
constexpr std::string_view PARI = "AT+PARI";
constexpr std::string_view STOP = "AT+STOP";
constexpr std::string_view ADVI = "AT+ADVI";
constexpr std::string_view FLAG = "AT+FLAG";
constexpr std::string_view RELI = "AT+RELI";
constexpr std::string_view PCTL = "AT+PCTL";
constexpr std::string_view BATC = "AT+BATC";
constexpr std::string_view BATT = "AT+BATT";
constexpr std::string_view UART = "AT+UART";
} // namespace cmd

/**
 * @brief Static mapping of Role enum to string representation
 */
constexpr std::array<std::string_view, 2> ROLE_STRINGS = {
    "0", // Role::Peripheral
    "1"  // Role::Central
};

/**
 * @brief Static mapping of AdvertisingInterval enum to string representation
 */
constexpr std::array<std::string_view, 16> ADVERTISING_INTERVAL_STRINGS = {
    "0",  // AdvertisingInterval::Ms100
    "1",  // AdvertisingInterval::Ms211
    "2",  // AdvertisingInterval::Ms252
    "3",  // AdvertisingInterval::Ms318
    "4",  // AdvertisingInterval::Ms417
    "5",  // AdvertisingInterval::Ms546
    "6",  // AdvertisingInterval::Ms760
    "7",  // AdvertisingInterval::Ms852
    "8",  // AdvertisingInterval::Ms1022
    "9",  // AdvertisingInterval::Ms1285
    "10", // AdvertisingInterval::Ms2000
    "11", // AdvertisingInterval::Ms3000
    "12", // AdvertisingInterval::Ms4000
    "13", // AdvertisingInterval::Ms5000
    "14", // AdvertisingInterval::Ms6000
    "15"  // AdvertisingInterval::Ms7000
};

/**
 * @brief Static mapping of AdvertisingType enum to string representation
 */
constexpr std::array<std::string_view, 4> ADVERTISING_TYPE_STRINGS = {
    "0", // AdvertisingType::AdvertisingScanResponseConnectable
    "1", // AdvertisingType::LastDeviceConnect
    "2", // AdvertisingType::OnlyWhiteListConnect
    "3"  // AdvertisingType::AnyDeviceConnect
};

/**
 * @brief Static mapping of AdvertisingMode enum to string representation
 */
constexpr std::array<std::string_view, 2> ADVERTISING_MODE_STRINGS = {
    "0", // AdvertisingMode::NormalAdvertising
    "1"  // AdvertisingMode::ReliableAdvertising
};

/**
 * @brief Static mapping of MacAddressType enum to string representation
 */
constexpr std::array<std::string_view, 4> MAC_ADDRESS_TYPE_STRINGS = {
    "0", // MacAddressType::NormalAddress
    "1", // MacAddressType::StaticMac
    "2", // MacAddressType::StaticRandomMac
    "3"  // MacAddressType::RandomMac
};

/**
 * @brief Static mapping of WorkMode enum to string representation
 */
constexpr std::array<std::string_view, 3> WORK_MODE_STRINGS = {
    "0", // WorkMode::TransmissionMode
    "1", // WorkMode::PioCollectionMode
    "2"  // WorkMode::RemoteControlMode
};

/**
 * @brief Static mapping of UartBaudRate enum to string representation
 */
constexpr std::array<std::string_view, 8> UART_BAUD_RATE_STRINGS = {
    "0", // UartBaudRate::Br9600
    "1", // UartBaudRate::Br19200
    "2", // UartBaudRate::Br38400
    "3", // UartBaudRate::Br57600
    "4", // UartBaudRate::Br115200
    "5", // UartBaudRate::Br4800
    "6", // UartBaudRate::Br2400
    "7"  // UartBaudRate::Br1200
};

/**
 * @brief Static mapping of BondMode enum to string representation
 */
constexpr std::array<std::string_view, 4> BOND_MODE_STRINGS = {
    "0", // BondMode::NotNeedPin
    "1", // BondMode::AuthNotNeedPin
    "2", // BondMode::AuthWithPin
    "3"  // BondMode::AuthAndBonded
};

/**
 * @brief Static mapping of SendDataMethod enum to string representation
 */
constexpr std::array<std::string_view, 4> SEND_DATA_METHOD_STRINGS = {
    "0", // SendDataMethod::Write
    "1", // SendDataMethod::WriteWithoutResponse
    "2", // SendDataMethod::Indicate
    "3"  // SendDataMethod::Notify
};

/**
 * @brief Static mapping of ModulePower enum to string representation
 */
constexpr std::array<std::string_view, 4> MODULE_POWER_STRINGS = {
    "0", // ModulePower::DbmMinus23
    "1", // ModulePower::DbmMinus6
    "2", // ModulePower::Dbm0
    "3"  // ModulePower::Dbm6
};

/**
 * @brief Static mapping of OutputPower enum to string representation
 */
constexpr std::array<std::string_view, 2> OUTPUT_POWER_STRINGS = {
    "0", // OutputPower::NormalPower
    "1"  // OutputPower::MaxPower
};

/**
 * @brief Static mapping of UartSleepType enum to string representation
 */
constexpr std::array<std::string_view, 2> UART_SLEEP_TYPE_STRINGS = {
    "0", // UartSleepType::CanWakeUpThroughUart
    "1"  // UartSleepType::ShutdownUart
};

/**
 * @brief Static mapping of ParityBit enum to string representation
 */
constexpr std::array<std::string_view, 3> PARITY_BIT_STRINGS = {
    "0", // ParityBit::None
    "1", // ParityBit::Odd
    "2"  // ParityBit::Even
};

/**
 * @brief Static mapping of StopBit enum to string representation
 */
constexpr std::array<std::string_view, 2> STOP_BIT_STRINGS = {
    "0", // StopBit::OneStopBit
    "1"  // StopBit::TwoStopBit
};

/**
 * @brief Static mapping of RxGain enum to string representation
 */
constexpr std::array<std::string_view, 2> RX_GAIN_STRINGS = {
    "0", // RxGain::No
    "1"  // RxGain::Open
};

/**
 * @brief Template function to get string representation of an enum value
 *
 * This function returns a string_view representation of an enum value using
 * the appropriate mapping table.
 *
 * @tparam EnumType The enum type
 * @tparam StringArray The type of the string array
 * @param value The enum value to convert
 * @param strings The array of string representations
 * @return constexpr std::string_view The string representation
 */
template <typename EnumType, typename StringArray>
[[nodiscard]] constexpr std::string_view to_string(EnumType value, const StringArray& strings) noexcept {
    const auto INDEX = static_cast<std::size_t>(value);
    if (INDEX < strings.size()) {
        // Use .at() to avoid the array subscript warning
        return strings.at(INDEX);
    }
    return "";
}

/**
 * @brief Get string representation of Role enum value
 *
 * @param role The role value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(Role role) noexcept {
    return to_string(role, ROLE_STRINGS);
}

/**
 * @brief Get string representation of AdvertisingInterval enum value
 *
 * @param interval The advertising interval value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(AdvertisingInterval interval) noexcept {
    return to_string(interval, ADVERTISING_INTERVAL_STRINGS);
}

/**
 * @brief Get string representation of AdvertisingType enum value
 *
 * @param type The advertising type value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(AdvertisingType type) noexcept {
    return to_string(type, ADVERTISING_TYPE_STRINGS);
}

/**
 * @brief Get string representation of AdvertisingMode enum value
 *
 * @param mode The advertising mode value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(AdvertisingMode mode) noexcept {
    return to_string(mode, ADVERTISING_MODE_STRINGS);
}

/**
 * @brief Get string representation of MacAddressType enum value
 *
 * @param type The MAC address type value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(MacAddressType type) noexcept {
    return to_string(type, MAC_ADDRESS_TYPE_STRINGS);
}

/**
 * @brief Get string representation of WorkMode enum value
 *
 * @param mode The work mode value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(WorkMode mode) noexcept {
    return to_string(mode, WORK_MODE_STRINGS);
}

/**
 * @brief Get string representation of UartBaudRate enum value
 *
 * @param baud_rate The UART baud rate value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(UartBaudRate baud_rate) noexcept {
    return to_string(baud_rate, UART_BAUD_RATE_STRINGS);
}

/**
 * @brief Get string representation of BondMode enum value
 *
 * @param mode The bond mode value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(BondMode mode) noexcept {
    return to_string(mode, BOND_MODE_STRINGS);
}

/**
 * @brief Get string representation of SendDataMethod enum value
 *
 * @param method The send data method value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(SendDataMethod method) noexcept {
    return to_string(method, SEND_DATA_METHOD_STRINGS);
}

/**
 * @brief Get string representation of ModulePower enum value
 *
 * @param power The module power value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(ModulePower power) noexcept {
    return to_string(power, MODULE_POWER_STRINGS);
}

/**
 * @brief Get string representation of OutputPower enum value
 *
 * @param power The output power value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(OutputPower power) noexcept {
    return to_string(power, OUTPUT_POWER_STRINGS);
}

/**
 * @brief Get string representation of UartSleepType enum value
 *
 * @param type The UART sleep type value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(UartSleepType type) noexcept {
    return to_string(type, UART_SLEEP_TYPE_STRINGS);
}

/**
 * @brief Get string representation of ParityBit enum value
 *
 * @param parity The parity bit value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(ParityBit parity) noexcept {
    return to_string(parity, PARITY_BIT_STRINGS);
}

/**
 * @brief Get string representation of StopBit enum value
 *
 * @param stop_bit The stop bit value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(StopBit stop_bit) noexcept {
    return to_string(stop_bit, STOP_BIT_STRINGS);
}

/**
 * @brief Get string representation of RxGain enum value
 *
 * @param gain The RX gain value
 * @return constexpr std::string_view The string representation
 */
[[nodiscard]] constexpr std::string_view to_string(RxGain gain) noexcept {
    return to_string(gain, RX_GAIN_STRINGS);
}

/**
 * @brief Build an AT command string for a query operation
 *
 * @param command The base command string (e.g., "AT+ROLE")
 * @return std::string The complete query command (e.g., "AT+ROLE?")
 */
[[nodiscard]] std::string build_query_command(std::string_view command);

/**
 * @brief Build an AT command string for a set operation
 *
 * @param command The base command string (e.g., "AT+ROLE")
 * @param value The value to set as a string_view (e.g., "1")
 * @return std::string The complete set command (e.g., "AT+ROLE1")
 */
[[nodiscard]] std::string build_set_command(std::string_view command, std::string_view value);

/**
 * @brief Build an AT command string for a set operation with an enum value
 *
 * @tparam EnumType The enum type
 * @param command The base command string (e.g., "AT+ROLE")
 * @param value The enum value to set
 * @return std::string The complete set command (e.g., "AT+ROLE1")
 */
template <typename EnumType> [[nodiscard]] std::string build_set_command(std::string_view command, EnumType value) {
    return build_set_command(command, to_string(value));
}

/**
 * @brief Build an AT command string for a set operation with a string value
 *
 * @param command The base command string (e.g., "AT+NAME")
 * @param value The string value to set
 * @return std::string The complete set command (e.g., "AT+NAMEMyDevice")
 */
[[nodiscard]] std::string build_set_command(std::string_view command, const std::string& value);

/**
 * @brief Build an AT command string for a set operation with a fixed string value
 *
 * @tparam N The size of the fixed string
 * @tparam Validator The validator type for the fixed string
 * @param command The base command string (e.g., "AT+UUID")
 * @param value The fixed string value to set
 * @return std::string The complete set command (e.g., "AT+UUID0x1234...")
 */
template <std::size_t N, CharacterValidator Validator>
[[nodiscard]] std::string build_set_command(std::string_view command, const FixedString<N, Validator>& value) {
    return build_set_command(command, std::string_view(value.data(), value.size()));
}

/**
 * @brief Build an AT command string for a set operation with a hex prefix
 *
 * @param command The base command string (e.g., "AT+UUID")
 * @param value The value to set
 * @return std::string The complete set command (e.g., "AT+UUID0x1234...")
 */
[[nodiscard]] std::string build_hex_command(std::string_view command, std::string_view value);

/**
 * @brief Build an AT command string for a set operation with a fixed string value and hex prefix
 *
 * @tparam N The size of the fixed string
 * @tparam Validator The validator type for the fixed string
 * @param command The base command string (e.g., "AT+UUID")
 * @param value The fixed string value to set
 * @return std::string The complete set command (e.g., "AT+UUID0x1234...")
 */
template <std::size_t N, CharacterValidator Validator>
[[nodiscard]] std::string build_hex_command(std::string_view command, const FixedString<N, Validator>& value) {
    return build_hex_command(command, std::string_view(value.data(), value.size()));
}

/**
 * @brief Build a command to connect to a device by MAC address
 *
 * @param mac The MAC address to connect to
 * @return std::string The complete connect command
 */
[[nodiscard]] std::string build_connect_command(const MacAddress& mac);

/**
 * @brief Build a command to scan for devices
 *
 * @param seconds The number of seconds to scan (0 for indefinite)
 * @return std::string The complete scan command
 */
[[nodiscard]] std::string build_scan_command(uint8_t seconds = 0);

/**
 * @brief Expected response patterns
 */
namespace response {
constexpr std::string_view OK = "OK";
constexpr std::string_view OK_SET_PREFIX = "OK+Set:";
constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
constexpr std::string_view ERROR = "ERROR";

// For discovery/connection responses
constexpr std::string_view OK_CONN = "OK+CONN";
constexpr std::string_view OK_LOST = "OK+LOST";
constexpr std::string_view OK_SCAN = "OK+SCAN:";
constexpr std::string_view OK_FIND = "OK+FIND:";

// Device type identifiers in scan responses
constexpr char NORMAL_DEVICE_ID = 'F';
constexpr char IBEACON_DEVICE_ID = 'B';
constexpr char HMSENSOR_DEVICE_ID = 'T';
} // namespace response

} // namespace hm11::util::at

#endif // HM11_UTIL_AT_COMMAND_BUILDER_HPP