/**
 * @file enum_parsers.hpp
 * @brief Parsing functions for HM11 enumeration types
 *
 * This file defines parsing functions that convert strings to enum values
 * for HM11 driver enumeration types. These functions are used to parse
 * responses from the HM11 module.
 */

#ifndef HM11_ENUM_PARSERS_HPP
#define HM11_ENUM_PARSERS_HPP

#include <optional>
#include <string_view>

#include "hm11/enum_types.hpp"

namespace hm11 {

/**
 * @brief Parse a Role from a string
 *
 * @param str String to parse
 * @return std::optional<Role> The parsed role, or std::nullopt if parsing failed
 */
std::optional<Role> parse_role(std::string_view str);

/**
 * @brief Parse an AdvertisingInterval from a string
 *
 * @param str String to parse
 * @return std::optional<AdvertisingInterval> The parsed interval, or std::nullopt if parsing failed
 */
std::optional<AdvertisingInterval> parse_advertising_interval(std::string_view str);

/**
 * @brief Parse an AdvertisingType from a string
 *
 * @param str String to parse
 * @return std::optional<AdvertisingType> The parsed type, or std::nullopt if parsing failed
 */
std::optional<AdvertisingType> parse_advertising_type(std::string_view str);

/**
 * @brief Parse an AdvertisingMode from a string
 *
 * @param str String to parse
 * @return std::optional<AdvertisingMode> The parsed mode, or std::nullopt if parsing failed
 */
std::optional<AdvertisingMode> parse_advertising_mode(std::string_view str);

/**
 * @brief Parse a MacAddressType from a string
 *
 * @param str String to parse
 * @return std::optional<MacAddressType> The parsed type, or std::nullopt if parsing failed
 */
std::optional<MacAddressType> parse_mac_address_type(std::string_view str);

/**
 * @brief Parse a WriteMethod from a string
 *
 * @param str String to parse
 * @return std::optional<WriteMethod> The parsed method, or std::nullopt if parsing failed
 */
std::optional<WriteMethod> parse_write_method(std::string_view str);

/**
 * @brief Parse a UartBaudRate from a string
 *
 * @param str String to parse
 * @return std::optional<UartBaudRate> The parsed baud rate, or std::nullopt if parsing failed
 */
std::optional<UartBaudRate> parse_uart_baud_rate(std::string_view str);

/**
 * @brief Parse a LinkLayerConnectionInterval from a string
 *
 * @param str String to parse
 * @return std::optional<LinkLayerConnectionInterval> The parsed interval, or std::nullopt if parsing failed
 */
std::optional<LinkLayerConnectionInterval> parse_link_layer_connection_interval(std::string_view str);

/**
 * @brief Parse a ConnectionSupervisionTimeout from a string
 *
 * @param str String to parse
 * @return std::optional<ConnectionSupervisionTimeout> The parsed timeout, or std::nullopt if parsing failed
 */
std::optional<ConnectionSupervisionTimeout> parse_connection_supervision_timeout(std::string_view str);

/**
 * @brief Parse a ConnectResult from a string
 *
 * @param str String to parse
 * @return std::optional<ConnectResult> The parsed result, or std::nullopt if parsing failed
 */
std::optional<ConnectResult> parse_connect_result(std::string_view str);

/**
 * @brief Parse an IBeaconDeployMode from a string
 *
 * @param str String to parse
 * @return std::optional<IBeaconDeployMode> The parsed mode, or std::nullopt if parsing failed
 */
std::optional<IBeaconDeployMode> parse_ibeacon_deploy_mode(std::string_view str);

/**
 * @brief Parse a NotifyResponse from a string
 *
 * @param str String to parse
 * @return std::optional<NotifyResponse> The parsed response, or std::nullopt if parsing failed
 */
std::optional<NotifyResponse> parse_notify_response(std::string_view str);

/**
 * @brief Parse a SendDataMethod from a string
 *
 * @param str String to parse
 * @return std::optional<SendDataMethod> The parsed method, or std::nullopt if parsing failed
 */
std::optional<SendDataMethod> parse_send_data_method(std::string_view str);

/**
 * @brief Parse a SendDataCharacteristic from a string
 *
 * @param str String to parse
 * @return std::optional<SendDataCharacteristic> The parsed characteristic, or std::nullopt if parsing failed
 */
std::optional<SendDataCharacteristic> parse_send_data_characteristic(std::string_view str);

/**
 * @brief Parse a CharacteristicUuidCount from a string
 *
 * @param str String to parse
 * @return std::optional<CharacteristicUuidCount> The parsed count, or std::nullopt if parsing failed
 */
std::optional<CharacteristicUuidCount> parse_characteristic_uuid_count(std::string_view str);

/**
 * @brief Parse an RxGain from a string
 *
 * @param str String to parse
 * @return std::optional<RxGain> The parsed gain, or std::nullopt if parsing failed
 */
std::optional<RxGain> parse_rx_gain(std::string_view str);

/**
 * @brief Parse a WorkType from a string
 *
 * @param str String to parse
 * @return std::optional<WorkType> The parsed type, or std::nullopt if parsing failed
 */
std::optional<WorkType> parse_work_type(std::string_view str);

/**
 * @brief Parse a WorkMode from a string
 *
 * @param str String to parse
 * @return std::optional<WorkMode> The parsed mode, or std::nullopt if parsing failed
 */
std::optional<WorkMode> parse_work_mode(std::string_view str);

/**
 * @brief Parse a NotifyMode from a string
 *
 * @param str String to parse
 * @return std::optional<NotifyMode> The parsed mode, or std::nullopt if parsing failed
 */
std::optional<NotifyMode> parse_notify_mode(std::string_view str);

/**
 * @brief Parse an OutputPower from a string
 *
 * @param str String to parse
 * @return std::optional<OutputPower> The parsed power, or std::nullopt if parsing failed
 */
std::optional<OutputPower> parse_output_power(std::string_view str);

/**
 * @brief Parse a ParityBit from a string
 *
 * @param str String to parse
 * @return std::optional<ParityBit> The parsed parity, or std::nullopt if parsing failed
 */
std::optional<ParityBit> parse_parity_bit(std::string_view str);

/**
 * @brief Parse a PioOutput from a string
 *
 * @param str String to parse
 * @return std::optional<PioOutput> The parsed output, or std::nullopt if parsing failed
 */
std::optional<PioOutput> parse_pio_output(std::string_view str);

/**
 * @brief Parse a ModulePower from a string
 *
 * @param str String to parse
 * @return std::optional<ModulePower> The parsed power, or std::nullopt if parsing failed
 */
std::optional<ModulePower> parse_module_power(std::string_view str);

/**
 * @brief Parse a StopBit from a string
 *
 * @param str String to parse
 * @return std::optional<StopBit> The parsed stop bit, or std::nullopt if parsing failed
 */
std::optional<StopBit> parse_stop_bit(std::string_view str);

/**
 * @brief Parse a SensorType from a string
 *
 * @param str String to parse
 * @return std::optional<SensorType> The parsed type, or std::nullopt if parsing failed
 */
std::optional<SensorType> parse_sensor_type(std::string_view str);

/**
 * @brief Parse a ShowDeviceInformation from a string
 *
 * @param str String to parse
 * @return std::optional<ShowDeviceInformation> The parsed information, or std::nullopt if parsing failed
 */
std::optional<ShowDeviceInformation> parse_show_device_information(std::string_view str);

/**
 * @brief Parse a BondMode from a string
 *
 * @param str String to parse
 * @return std::optional<BondMode> The parsed mode, or std::nullopt if parsing failed
 */
std::optional<BondMode> parse_bond_mode(std::string_view str);

/**
 * @brief Parse a UartSleepType from a string
 *
 * @param str String to parse
 * @return std::optional<UartSleepType> The parsed type, or std::nullopt if parsing failed
 */
std::optional<UartSleepType> parse_uart_sleep_type(std::string_view str);

// ============================================================================
// Character Conversion Functions (for single-char enum representations)
// ============================================================================

/**
 * @brief Convert AdvertisingInterval enum to character
 * @param interval The advertising interval enum value
 * @return Character representation ('0'-'9', 'A'-'F')
 */
char to_char(AdvertisingInterval interval);

/**
 * @brief Convert character to AdvertisingInterval enum
 * @param c Character representation ('0'-'9', 'A'-'F')
 * @param tag Type tag for overload resolution (unused)
 * @return AdvertisingInterval enum value
 */
AdvertisingInterval from_char(char c, [[maybe_unused]] AdvertisingInterval* tag);

/**
 * @brief Convert AdvertisingType enum to character
 * @param type The advertising type enum value
 * @return Character representation ('0'-'3')
 */
char to_char(AdvertisingType type);

/**
 * @brief Convert character to AdvertisingType enum
 * @param c Character representation ('0'-'3')
 * @param tag Type tag for overload resolution (unused)
 * @return AdvertisingType enum value
 */
AdvertisingType from_char(char c, [[maybe_unused]] AdvertisingType* tag);

/**
 * @brief Convert AdvertisingMode enum to character
 * @param mode The advertising mode enum value
 * @return Character representation ('0'-'1')
 */
char to_char(AdvertisingMode mode);

/**
 * @brief Convert character to AdvertisingMode enum
 * @param c Character representation ('0'-'1')
 * @param tag Type tag for overload resolution (unused)
 * @return AdvertisingMode enum value
 */
AdvertisingMode from_char(char c, [[maybe_unused]] AdvertisingMode* tag);

/**
 * @brief Convert WorkMode enum to character
 * @param mode The work mode enum value
 * @return Character representation ('0'-'2')
 */
char to_char(WorkMode mode);

/**
 * @brief Convert character to WorkMode enum
 * @param c Character representation ('0'-'2')
 * @param tag Type tag for overload resolution (unused)
 * @return WorkMode enum value
 */
WorkMode from_char(char c, [[maybe_unused]] WorkMode* tag);

/**
 * @brief Convert WorkType enum to character
 * @param type The work type enum value
 * @return Character representation ('0'-'1')
 */
char to_char(WorkType type);

/**
 * @brief Convert character to WorkType enum
 * @param c Character representation ('0'-'1')
 * @param tag Type tag for overload resolution (unused)
 * @return WorkType enum value
 */
WorkType from_char(char c, [[maybe_unused]] WorkType* tag);

/**
 * @brief Convert NotifyMode enum to character
 * @param mode The notify mode enum value
 * @return Character representation ('0'-'1')
 */
char to_char(NotifyMode mode);

/**
 * @brief Convert character to NotifyMode enum
 * @param c Character representation ('0'-'1')
 * @param tag Type tag for overload resolution (unused)
 * @return NotifyMode enum value
 */
NotifyMode from_char(char c, [[maybe_unused]] NotifyMode* tag);

// Template overload for type deduction
template<typename T>
T from_char(char c) {
    T* tag = nullptr;
    return from_char(c, tag);
}

} // namespace hm11

#endif // HM11_ENUM_PARSERS_HPP