/**
 * @file types.hpp
 * @brief Core types and enums for GATT (Generic Attribute Profile) operations
 *
 * This file contains the definitions of core types and enums needed for GATT
 * operations in the HM11 driver. It includes enumeration types for notification
 * response, data methods, characteristic count, and the implementation of
 * HandleType using the FixedString template.
 *
 * Implementation follows Design Decision 2 (Strong Type System Translation) and
 * Design Decision 3 (Notification Callback Mechanism).
 */

#ifndef HM11_GATT_TYPES_HPP
#define HM11_GATT_TYPES_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "hm11/util/fixed_string.hpp"
#include "hm11/util/validators.hpp"

namespace hm11::gatt {

/**
 * @brief Response status for notification operations
 *
 * This enumeration represents the possible response statuses for notification operations.
 * Equivalent to the Ada Notify_Responce type.
 */
enum class NotifyResponse : std::uint8_t {
    SendOk,    ///< Send command successful
    SendError, ///< Send command error
    DataError  ///< This characteristic doesn't have notify property
};

/**
 * @brief Methods for sending data to a characteristic
 *
 * This enumeration represents the different methods for sending data to a characteristic.
 * Equivalent to the Ada Send_Data_Method type.
 */
enum class SendDataMethod : std::uint8_t {
    Write,                ///< Write with response
    WriteWithoutResponse, ///< Write without response
    Indicate,             ///< Indication (with acknowledgment)
    Notify                ///< Notification (without acknowledgment)
};

/**
 * @brief Characteristic write methods
 *
 * This enumeration represents the methods for writing to a characteristic.
 * Equivalent to the Ada Send_Data_Characteristic type.
 */
enum class SendDataCharacteristic : std::uint8_t {
    Write,               ///< Write with response
    WriteWithoutResponse ///< Write without response
};

/**
 * @brief Options for retrieving characteristic UUIDs
 *
 * This enumeration represents the different options for retrieving characteristic UUIDs.
 * Equivalent to the Ada Characteristic_UUID_Count type.
 */
enum class CharacteristicUUIDCount : std::uint8_t {
    Query,      ///< Query for characteristics
    One,        ///< Only use one characteristic (set with Set_Characteristic)
    OneAndNext, ///< Use two characteristics (first + next)
    OneAndPrev  ///< Use two characteristics (first + previous)
};

/**
 * @brief Handle type for GATT operations
 *
 * This type represents a 4-character hexadecimal handle for GATT operations.
 * Equivalent to the Ada Handle_Type.
 *
 * Following Design Decision 2, this is implemented using the FixedString template
 * with HexCharValidator to ensure all characters are valid hexadecimal digits.
 */
using HandleType = util::FixedString<4, util::HexCharValidator>;

/**
 * @brief Callback type for service UUID discovery
 *
 * This type represents a callback function for service UUID discovery.
 * Equivalent to the Ada Service_UUID_Callback type.
 *
 * Following Design Decision 3, this uses a C-style function pointer with
 * an optional void* user_data parameter for stateful callbacks.
 *
 * The service string format is [P1]:[P2]:[P3] where:
 *   - P1: 4 Bytes, Services start handle
 *   - P2: 4 Bytes, Services end handle
 *   - P3: 4 Bytes, Services UUID
 */
using ServiceUUIDCallback = void (*)(const std::string& service, void* user_data);

/**
 * @brief Callback type for characteristic UUID discovery
 *
 * This type represents a callback function for characteristic UUID discovery.
 * Equivalent to the Ada Characteristic_UUID_Callback type.
 *
 * Following Design Decision 3, this uses a C-style function pointer with
 * an optional void* user_data parameter for stateful callbacks.
 *
 * The characteristic string format is [P1]:[P2]:[P3] where:
 *   - P1: 4 Bytes, Characteristic handle
 *   - P2: 14 Bytes, "RD|WR|WN|NO|IN" (properties)
 *   - P3: 4 Bytes, Characteristic UUID
 *
 * Characteristic properties length is 14 Bytes, full string is "RD|WR|WN|NO|IN":
 *   - WR: Write
 *   - WN: Write_Without_Response
 *   - IN: Indicate
 *   - NO: Notify
 *   - RD: Read
 *
 * Usually, a characteristic may only have one or two properties.
 * Unsupported property parts are replaced by "--"
 */
using CharacteristicUUIDCallback = void (*)(const std::string& characteristic, void* user_data);

} // namespace hm11::gatt

#endif // HM11_GATT_TYPES_HPP