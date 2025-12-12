/**
 * @file hm11_response_builder.hpp
 * @brief Helper class for constructing realistic HM-11 response strings
 *
 * This header provides utilities for building properly formatted HM-11 AT command
 * responses matching the actual hardware protocol format. Useful for programming
 * realistic responses in EnhancedMockUART.
 *
 * ## Features
 *
 * - Discovery response formatting (MAC, Name, RSSI)
 * - GATT service/characteristic discovery responses
 * - Connection notification responses
 * - Sensor reading responses
 * - Error response generation
 *
 * ## Usage Example
 *
 * @code
 * #include "tests/util/hm11_response_builder.hpp"
 *
 * using namespace hm11::test;
 *
 * // Build discovery response
 * auto response = HM11ResponseBuilder::discovery_start()
 *     .add_device("001122334455", "Device1", "-65")
 *     .add_device("AABBCCDDEEFF", "Device2", "-72")
 *     .discovery_end()
 *     .build();
 *
 * // Build GATT service response
 * auto services = HM11ResponseBuilder::gatt_services({"1800", "1801", "180A"});
 *
 * // Build sensor reading
 * auto sensor = HM11ResponseBuilder::sensor_reading(25, 60); // 25°C, 60%
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hm11::test {

/**
 * @brief Builder for HM-11 AT command responses
 *
 * Provides fluent interface for constructing realistic HM-11 protocol responses.
 * Ensures correct formatting matching actual hardware behavior.
 */
class HM11ResponseBuilder {
  public:
    /**
     * @brief Build discovery start response
     * @return Builder instance for chaining
     */
    static HM11ResponseBuilder discovery_start();

    /**
     * @brief Add a discovered device to the response
     *
     * @param mac MAC address (12 hex characters)
     * @param name Device name (up to 248 characters)
     * @param rssi RSSI value (typically "-XX" format)
     * @return Builder reference for chaining
     */
    HM11ResponseBuilder& add_device(std::string_view mac, std::string_view name, std::string_view rssi);

    /**
     * @brief Add discovery end marker
     * @return Builder reference for chaining
     */
    HM11ResponseBuilder& discovery_end();

    /**
     * @brief Build and return the complete response string
     * @return Formatted response string
     */
    [[nodiscard]] std::string build() const;

    // ========================================================================
    // Static Helper Methods for Common Responses
    // ========================================================================

    /**
     * @brief Build simple OK response
     * @return "OK"
     */
    [[nodiscard]] static std::string ok();

    /**
     * @brief Build OK+Set response
     * @param value Value that was set
     * @return "OK+Set:<value>"
     */
    [[nodiscard]] static std::string ok_set(std::string_view value);

    /**
     * @brief Build OK+Get response
     * @param value Value that was retrieved
     * @return "OK+Get:<value>"
     */
    [[nodiscard]] static std::string ok_get(std::string_view value);

    /**
     * @brief Build connection success response
     * @return "OK+CONN"
     */
    [[nodiscard]] static std::string connection_success();

    /**
     * @brief Build connection lost response
     * @return "OK+LOST"
     */
    [[nodiscard]] static std::string connection_lost();

    /**
     * @brief Build error response
     * @return "OK+ERR"
     */
    [[nodiscard]] static std::string error();

    /**
     * @brief Build GATT services response
     *
     * @param service_uuids Vector of 16-bit service UUIDs (4 hex chars each)
     * @return "OK+DISC:<uuid1>,<uuid2>,..."
     */
    [[nodiscard]] static std::string gatt_services(const std::vector<std::string>& service_uuids);

    /**
     * @brief Build GATT characteristics response
     *
     * @param char_uuids Vector of characteristic UUIDs
     * @return "OK+CHAR:<uuid1>,<uuid2>,..."
     */
    [[nodiscard]] static std::string gatt_characteristics(const std::vector<std::string>& char_uuids);

    /**
     * @brief Build sensor reading response (HMSensor)
     *
     * @param temperature Temperature value (0-120)
     * @param humidity Humidity value (0-100)
     * @return "OK+Get:<temp>C <hum>%"
     */
    [[nodiscard]] static std::string sensor_reading(uint8_t temperature, uint8_t humidity);

    /**
     * @brief Build version response
     *
     * @param version Version string (e.g., "V540")
     * @return "OK+Get:<version>"
     */
    [[nodiscard]] static std::string version(std::string_view version);

    /**
     * @brief Build role response
     *
     * @param role 0 for Peripheral, 1 for Central
     * @return "OK+Get:<role>"
     */
    [[nodiscard]] static std::string role(uint8_t role);

    /**
     * @brief Build name response
     *
     * @param name Device name
     * @return "OK+NAME:<name>"
     */
    [[nodiscard]] static std::string name(std::string_view name);

    /**
     * @brief Build MAC address response
     *
     * @param mac MAC address (12 hex characters)
     * @return "OK+ADDR:<mac>"
     */
    [[nodiscard]] static std::string mac_address(std::string_view mac);

    /**
     * @brief Build RSSI response
     *
     * @param rssi RSSI value (e.g., -65)
     * @return "OK+RSSI:<rssi>dBm"
     */
    [[nodiscard]] static std::string rssi(int8_t rssi);

  private:
    std::string response_; ///< Accumulated response string

    /// @brief Private constructor for builder pattern
    HM11ResponseBuilder() = default;
};

} // namespace hm11::test
