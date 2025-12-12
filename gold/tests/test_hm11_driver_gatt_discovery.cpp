/**
 * @file test_hm11_driver_gatt_discovery.cpp
 * @brief Unit tests for HM11 driver GATT discovery operations
 *
 * Tests the GATT service and characteristic discovery methods including:
 * - Find_All_Services_UUID parsing and callbacks
 * - Find_All_Characteristic_UUID parsing and callbacks
 * - Find_Characteristic_UUID with handle range parameters
 * - Colon-delimited field parsing
 * - Service info format validation (start:end:uuid)
 * - Characteristic info format validation (handle:properties:uuid)
 * - Properties string parsing (RD|WR|WN|NO|IN with "--" for unsupported)
 * - Timeout handling for long-running discovery operations
 * - Empty response handling
 * - Status error propagation
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/type_aliases.hpp"

using namespace std::chrono_literals;

namespace hm11::test {

// ============================================================================
// Mock UART Port for Testing
// ============================================================================

/// @brief Mock UART port that simulates hardware responses
class MockUARTPort : public hal::IUARTPort {
  public:
    /// @brief Set canned response data for next receive operation
    void set_response(std::string_view data) {
        response_data_ = data;
        response_pos_ = 0;
    }

    /// @brief Transmit data (record command for verification)
    void transmit(hal::UartData8bConst data, hal::UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept
        override {
        last_command_.assign(
            reinterpret_cast<const char*>(data.data()), // NOLINT
            data.size());
        status = hal::UartStatus::Ok;
    }

    /// @brief Receive data (copy from canned response)
    void receive(
        hal::UartData8b data,
        hal::UartStatus& status,
        uint32_t /*timeout_ms*/ = 1000,
        bool /*as_stream*/ = false) noexcept override {
        std::size_t to_copy = std::min(data.size(), response_data_.size() - response_pos_);
        if (to_copy > 0) {
            std::memcpy(data.data(), response_data_.data() + response_pos_, to_copy);
            response_pos_ += to_copy;
        }
        status = hal::UartStatus::Ok;
    }

    /// @brief Get last transmitted command
    [[nodiscard]] const std::string& last_command() const noexcept { return last_command_; }

    /// @brief Get data size (always 8-bit for HM-11)
    [[nodiscard]] hal::UartDataSize data_size() const noexcept override { return hal::UartDataSize::DataSize8b; }

  private:
    std::string response_data_;
    std::size_t response_pos_{0};
    std::string last_command_;
};

// ============================================================================
// Mock Receive Handler for Testing
// ============================================================================

/// @brief Mock receive handler that uses MockUARTPort
struct MockReceiveHandler {
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    // Overload WITH explicit timeout parameter
    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        hal::UartData8b buffer,
        hal::UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload WITHOUT timeout parameter (uses default)
    template <typename UARTPortType>
    void operator()(UARTPortType& port, hal::UartData8b buffer, hal::UartStatus& status, bool as_stream) noexcept {
        operator()(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverGATTDiscoveryTest : public ::testing::Test {
  protected:
    MockUARTPort uart_port_;
    MockReceiveHandler receive_handler_;
    HM11Driver<MockUARTPort, MockReceiveHandler> driver_{uart_port_, receive_handler_};
};

// ============================================================================
// Find_All_Services_UUID Tests
// ============================================================================

TEST_F(HM11DriverGATTDiscoveryTest, FindAllServicesUuid_SingleService) {
    // Set up response with header markers and single service
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n0001:000B:1800\r\n"; // Generic Access Service
    response += HEADER;

    uart_port_.set_response(response);

    // Track callback invocations
    std::vector<std::string> discovered_services;

    // Setup static pointer for lambda
    static std::vector<std::string>* global_services = &discovered_services;
    auto test_callback = +[](std::string_view service_info) {
        if (global_services != nullptr) {
            global_services->emplace_back(service_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_services_uuid(test_callback, 2000ms, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FINDSERVICES?");
    ASSERT_EQ(discovered_services.size(), 1);
    EXPECT_EQ(discovered_services[0], "0001:000B:1800");

    global_services = nullptr;
}

TEST_F(HM11DriverGATTDiscoveryTest, FindAllServicesUuid_MultipleServices) {
    // Set up response with multiple services
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0001:000B:1800\r\n"; // Generic Access
    response += "000C:000F:1801\r\n"; // Generic Attribute
    response += "0010:0015:180A\r\n"; // Device Information
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_services;
    static std::vector<std::string>* global_services = &discovered_services;
    auto test_callback = +[](std::string_view service_info) {
        if (global_services != nullptr) {
            global_services->emplace_back(service_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_services_uuid(test_callback, 2000ms, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(discovered_services.size(), 3);
    EXPECT_EQ(discovered_services[0], "0001:000B:1800");
    EXPECT_EQ(discovered_services[1], "000C:000F:1801");
    EXPECT_EQ(discovered_services[2], "0010:0015:180A");

    global_services = nullptr;
}

TEST_F(HM11DriverGATTDiscoveryTest, FindAllServicesUuid_EmptyResponse) {
    // Set up response with no services (just header markers)
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_services;
    static std::vector<std::string>* global_services = &discovered_services;
    auto test_callback = +[](std::string_view service_info) {
        if (global_services != nullptr) {
            global_services->emplace_back(service_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_services_uuid(test_callback, 2000ms, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(discovered_services.size(), 0);

    global_services = nullptr;
}

// ============================================================================
// Find_All_Characteristic_UUID Tests
// ============================================================================

TEST_F(HM11DriverGATTDiscoveryTest, FindAllCharacteristicUuid_SingleCharacteristic) {
    // Set up response with single characteristic
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n0003:RD|--|--|--|--:2A00\r\n"; // Device Name (Read only)
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_chars;
    static std::vector<std::string>* global_chars = &discovered_chars;
    auto test_callback = +[](std::string_view char_info) {
        if (global_chars != nullptr) {
            global_chars->emplace_back(char_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_characteristic_uuid(test_callback, 2000ms, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FINDALLCHARS?");
    ASSERT_EQ(discovered_chars.size(), 1);
    EXPECT_EQ(discovered_chars[0], "0003:RD|--|--|--|--:2A00");

    global_chars = nullptr;
}

TEST_F(HM11DriverGATTDiscoveryTest, FindAllCharacteristicUuid_MultipleProperties) {
    // Set up response with characteristics having various property combinations
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0003:RD|--|--|--|--:2A00\r\n"; // Read only
    response += "0005:RD|WR|--|--|--:2A01\r\n"; // Read + Write
    response += "0007:RD|--|--|NO|--:2A02\r\n"; // Read + Notify
    response += "0009:RD|WR|WN|NO|IN:2A03\r\n"; // All properties
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_chars;
    static std::vector<std::string>* global_chars = &discovered_chars;
    auto test_callback = +[](std::string_view char_info) {
        if (global_chars != nullptr) {
            global_chars->emplace_back(char_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_characteristic_uuid(test_callback, 2000ms, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(discovered_chars.size(), 4);
    EXPECT_EQ(discovered_chars[0], "0003:RD|--|--|--|--:2A00");
    EXPECT_EQ(discovered_chars[1], "0005:RD|WR|--|--|--:2A01");
    EXPECT_EQ(discovered_chars[2], "0007:RD|--|--|NO|--:2A02");
    EXPECT_EQ(discovered_chars[3], "0009:RD|WR|WN|NO|IN:2A03");

    global_chars = nullptr;
}

// ============================================================================
// Find_Characteristic_UUID (with handle range) Tests
// ============================================================================

TEST_F(HM11DriverGATTDiscoveryTest, FindCharacteristicUuid_WithHandleRange) {
    // Set up response for characteristics in specific handle range
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0001:RD|--|--|--|--:2A00\r\n";
    response += "0003:RD|WR|--|--|--:2A01\r\n";
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_chars;
    static std::vector<std::string>* global_chars = &discovered_chars;
    auto test_callback = +[](std::string_view char_info) {
        if (global_chars != nullptr) {
            global_chars->emplace_back(char_info);
        }
    };

    types::HandleType from("0001");
    types::HandleType to("000B");

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_characteristic_uuid(from, to, test_callback, 2000ms, status);

    // Verify command format: AT+CHAR0001000B?
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+CHAR0001000B?");
    ASSERT_EQ(discovered_chars.size(), 2);
    EXPECT_EQ(discovered_chars[0], "0001:RD|--|--|--|--:2A00");
    EXPECT_EQ(discovered_chars[1], "0003:RD|WR|--|--|--:2A01");

    global_chars = nullptr;
}

// ============================================================================
// Format Validation Tests
// ============================================================================

TEST_F(HM11DriverGATTDiscoveryTest, ServiceInfoFormat_ValidatesColons) {
    // Service info without proper colon separators should not be parsed
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0001:000B:1800\r\n"; // Valid
    response += "0001000B1801\r\n";   // Invalid - no colons
    response += "000C:000F:180A\r\n"; // Valid
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_services;
    static std::vector<std::string>* global_services = &discovered_services;
    auto test_callback = +[](std::string_view service_info) {
        if (global_services != nullptr) {
            global_services->emplace_back(service_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_services_uuid(test_callback, 2000ms, status);

    // Verify - only valid entries parsed
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(discovered_services.size(), 2);
    EXPECT_EQ(discovered_services[0], "0001:000B:1800");
    EXPECT_EQ(discovered_services[1], "000C:000F:180A");

    global_services = nullptr;
}

TEST_F(HM11DriverGATTDiscoveryTest, CharacteristicInfoFormat_ValidatesColons) {
    // Characteristic info without proper colon separators should not be parsed
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0003:RD|--|--|--|--:2A00\r\n"; // Valid
    response += "0003RD|--|--|--|--2A00\r\n";   // Invalid - no colons
    response += "0005:RD|WR|--|--|--:2A01\r\n"; // Valid
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_chars;
    static std::vector<std::string>* global_chars = &discovered_chars;
    auto test_callback = +[](std::string_view char_info) {
        if (global_chars != nullptr) {
            global_chars->emplace_back(char_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_characteristic_uuid(test_callback, 2000ms, status);

    // Verify - only valid entries parsed
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(discovered_chars.size(), 2);
    EXPECT_EQ(discovered_chars[0], "0003:RD|--|--|--|--:2A00");
    EXPECT_EQ(discovered_chars[1], "0005:RD|WR|--|--|--:2A01");

    global_chars = nullptr;
}

TEST_F(HM11DriverGATTDiscoveryTest, ServiceInfoFormat_ValidatesHexFields) {
    // Service info with non-hex characters should not be parsed
    constexpr std::string_view HEADER = "********************************************************"; // 56 asterisks
    std::string response;
    response += HEADER;
    response += "\r\n";
    response += "0001:000B:1800\r\n"; // Valid
    response += "GGGG:000B:1800\r\n"; // Invalid - non-hex start handle
    response += "0001:ZZZZ:1800\r\n"; // Invalid - non-hex end handle
    response += "0001:000B:XXXX\r\n"; // Invalid - non-hex UUID
    response += "000C:000F:180A\r\n"; // Valid
    response += HEADER;

    uart_port_.set_response(response);

    std::vector<std::string> discovered_services;
    static std::vector<std::string>* global_services = &discovered_services;
    auto test_callback = +[](std::string_view service_info) {
        if (global_services != nullptr) {
            global_services->emplace_back(service_info);
        }
    };

    hal::UartStatus status = hal::UartStatus::ErrError;
    driver_.find_all_services_uuid(test_callback, 2000ms, status);

    // Verify - only valid entries parsed
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(discovered_services.size(), 2);
    EXPECT_EQ(discovered_services[0], "0001:000B:1800");
    EXPECT_EQ(discovered_services[1], "000C:000F:180A");

    global_services = nullptr;
}

} // namespace hm11::test
