/**
 * @file hm11_response_builder.cpp
 * @brief Implementation of HM-11 response builder
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "hm11_response_builder.hpp"

namespace hm11::test {

// ============================================================================
// Builder Pattern Methods
// ============================================================================

HM11ResponseBuilder HM11ResponseBuilder::discovery_start() {
    HM11ResponseBuilder builder;
    builder.response_ = "OK+DISCS"; // Start of discovery
    return builder;
}

HM11ResponseBuilder&
HM11ResponseBuilder::add_device(std::string_view mac, std::string_view name, std::string_view rssi) {
    // Format: "OK+DISC:<MAC>\r\n<Name>\r\n<RSSI>\r\n"
    response_ += "OK+DISC:";
    response_ += mac;
    response_ += "\r\n";
    response_ += name;
    response_ += "\r\n";
    response_ += rssi;
    response_ += "\r\n";
    return *this;
}

HM11ResponseBuilder& HM11ResponseBuilder::discovery_end() {
    response_ += "OK+DISCE"; // End of discovery
    return *this;
}

std::string HM11ResponseBuilder::build() const {
    return response_;
}

// ============================================================================
// Static Helper Methods
// ============================================================================

std::string HM11ResponseBuilder::ok() {
    return "OK";
}

std::string HM11ResponseBuilder::ok_set(std::string_view value) {
    std::string response = "OK+Set:";
    response += value;
    return response;
}

std::string HM11ResponseBuilder::ok_get(std::string_view value) {
    std::string response = "OK+Get:";
    response += value;
    return response;
}

std::string HM11ResponseBuilder::connection_success() {
    return "OK+CONN";
}

std::string HM11ResponseBuilder::connection_lost() {
    return "OK+LOST";
}

std::string HM11ResponseBuilder::error() {
    return "OK+ERR";
}

std::string HM11ResponseBuilder::gatt_services(const std::vector<std::string>& service_uuids) {
    std::string response = "OK+DISC:";

    for (std::size_t i = 0; i < service_uuids.size(); ++i) {
        if (i > 0) {
            response += ",";
        }
        response += service_uuids[i];
    }

    return response;
}

std::string HM11ResponseBuilder::gatt_characteristics(const std::vector<std::string>& char_uuids) {
    std::string response = "OK+CHAR:";

    for (std::size_t i = 0; i < char_uuids.size(); ++i) {
        if (i > 0) {
            response += ",";
        }
        response += char_uuids[i];
    }

    return response;
}

std::string HM11ResponseBuilder::sensor_reading(uint8_t temperature, uint8_t humidity) {
    std::string response = "OK+Get:";
    response += std::to_string(temperature);
    response += "C ";
    response += std::to_string(humidity);
    response += "%";
    return response;
}

std::string HM11ResponseBuilder::version(std::string_view version) {
    return ok_get(version);
}

std::string HM11ResponseBuilder::role(uint8_t role) {
    return ok_get(std::to_string(role));
}

std::string HM11ResponseBuilder::name(std::string_view name) {
    std::string response = "OK+NAME:";
    response += name;
    return response;
}

std::string HM11ResponseBuilder::mac_address(std::string_view mac) {
    std::string response = "OK+ADDR:";
    response += mac;
    return response;
}

std::string HM11ResponseBuilder::rssi(int8_t rssi) {
    std::string response = "OK+RSSI:";
    response += std::to_string(rssi);
    response += "dBm";
    return response;
}

} // namespace hm11::test
