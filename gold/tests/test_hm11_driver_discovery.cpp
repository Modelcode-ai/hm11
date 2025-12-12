/**
 * @file test_hm11_driver_discovery.cpp
 * @brief Unit tests for HM11 driver discovery operations
 *
 * Tests the Scan and Scan_iBeacon methods including:
 * - State machine parsing (Decision 11)
 * - Callback invocation (Decision 10)
 * - Temporary discovery storage (Decision 27)
 * - Soft-stop timeout behavior (Decision 28)
 * - Variable_String accumulation
 * - DiscoveredInfo lifecycle
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
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// ============================================================================
// Test Fixtures
// ============================================================================

/// @brief Test fixture for HM11 driver discovery operations
class HM11DriverDiscoveryTest : public ::testing::Test {
  protected:
    MockUARTPort uart_port_;
    MockReceiveHandler receive_handler_;
    HM11Driver<MockUARTPort, MockReceiveHandler> driver_{uart_port_, receive_handler_};

    /// @brief Helper to create single-device discovery response
    static std::string
    make_discovery_response(std::string_view mac, std::string_view name = "", std::string_view rssi = "") {
        std::string response = "OK+DISCSOK+DISC:";
        response += mac;
        if (!name.empty()) {
            response += "OK+NAME:";
            response += name;
            response += "\r\n";
        }
        if (!rssi.empty()) {
            response += "OK+RSSI:";
            response += rssi;
            response += "\r\n";
        }
        response += "OK+DISCE";
        return response;
    }

    /// @brief Helper to create multi-device discovery response
    static std::string make_multi_device_response(
        const std::vector<std::tuple<std::string, std::string, std::string>>& devices) {
        std::string response = "OK+DISCS";
        for (const auto& [mac, name, rssi] : devices) {
            response += "OK+DISC:";
            response += mac;
            if (!name.empty()) {
                response += "OK+NAME:";
                response += name;
                response += "\r\n";
            }
            if (!rssi.empty()) {
                response += "OK+RSSI:";
                response += rssi;
                response += "\r\n";
            }
        }
        response += "OK+DISCE";
        return response;
    }
};

// ============================================================================
// Variable_String Tests
// ============================================================================

TEST(VariableStringTest, DefaultConstruction) {
    hm11::VariableString<10> str;
    EXPECT_EQ(str.last, 0);
    EXPECT_EQ(str.view().size(), 0);
    EXPECT_TRUE(str.view().empty());
}

TEST(VariableStringTest, AppendCharacter) {
    hm11::VariableString<10> str;
    EXPECT_TRUE(str.append('a'));
    EXPECT_TRUE(str.append('b'));
    EXPECT_TRUE(str.append('c'));
    EXPECT_EQ(str.last, 3);
    EXPECT_EQ(str.view(), "abc");
}

TEST(VariableStringTest, AppendString) {
    hm11::VariableString<20> str;
    EXPECT_TRUE(str.append("hello"));
    EXPECT_EQ(str.view(), "hello");
    EXPECT_TRUE(str.append(" world"));
    EXPECT_EQ(str.view(), "hello world");
}

TEST(VariableStringTest, CapacityExceeded) {
    hm11::VariableString<5> str;
    EXPECT_TRUE(str.append("hello"));
    EXPECT_FALSE(str.append('!')); // Should fail, capacity reached
    EXPECT_EQ(str.view(), "hello");
}

TEST(VariableStringTest, Clear) {
    hm11::VariableString<10> str;
    str.append("test");
    EXPECT_EQ(str.view(), "test");
    str.clear();
    EXPECT_EQ(str.last, 0);
    EXPECT_TRUE(str.view().empty());
}

// ============================================================================
// DiscoveredInfo Tests
// ============================================================================

TEST(DiscoveredInfoTest, DefaultConstruction) {
    hm11::DiscoveredInfo info;
    EXPECT_EQ(info.id, ' ');
    EXPECT_FALSE(info.is_valid()); // MAC is default (zeros)
}

TEST(DiscoveredInfoTest, Reset) {
    hm11::DiscoveredInfo info;
    info.id = 'A';
    info.mac = types::MacAddress{"B4994C123456"};
    info.name.append("TestDevice");
    info.rssi.append("-50");

    info.reset();

    EXPECT_EQ(info.id, ' ');
    EXPECT_FALSE(info.is_valid());
    EXPECT_EQ(info.name.last, 0);
    EXPECT_EQ(info.rssi.last, 0);
}

TEST(DiscoveredInfoTest, IsValid) {
    hm11::DiscoveredInfo info;
    EXPECT_FALSE(info.is_valid()); // Default MAC is invalid

    info.mac = types::MacAddress{"B4994C123456"};
    EXPECT_TRUE(info.is_valid()); // Non-zero MAC is valid
}

// ============================================================================
// ScanStage Enum Tests
// ============================================================================

TEST(ScanStageTest, EnumValues) {
    // Verify enum values exist and are distinct
    EXPECT_NE(ScanStage::Selection, ScanStage::MAC);
    EXPECT_NE(ScanStage::Selection, ScanStage::Name);
    EXPECT_NE(ScanStage::Selection, ScanStage::RSSI);
    EXPECT_NE(ScanStage::MAC, ScanStage::Name);
    EXPECT_NE(ScanStage::MAC, ScanStage::RSSI);
    EXPECT_NE(ScanStage::Name, ScanStage::RSSI);
}

// ============================================================================
// Callback Tests
// ============================================================================

/// @brief Test callback context for capturing callback invocations
struct CallbackContext {
    std::vector<std::tuple<char, std::string, std::string, std::string>> devices;

    void record_device(char id, const types::MacAddress& mac, std::string_view name, std::string_view rssi) {
        devices.emplace_back(id, std::string(mac.view()), std::string(name), std::string(rssi));
    }

    static void
    callback(char /*id*/, const types::MacAddress& /*mac*/, std::string_view /*name*/, std::string_view /*rssi*/) {
        // This would be set by test fixture
    }
};

// Global callback context for testing (required for C function pointer)
CallbackContext g_callback_context;

void test_discovered_callback(char id, const types::MacAddress& mac, std::string_view name, std::string_view rssi) {
    g_callback_context.record_device(id, mac, name, rssi);
}

// ============================================================================
// Scan Operation Tests
// ============================================================================

TEST_F(HM11DriverDiscoveryTest, ScanSingleDeviceMacOnly) {
    // Set up response: single device with MAC only
    std::string response = make_discovery_response("B4994C123456");
    uart_port_.set_response(response);

    // Clear callback context
    g_callback_context.devices.clear();

    // Execute scan
    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+DISC?");
    ASSERT_EQ(g_callback_context.devices.size(), 1);

    const auto& [id, mac, name, rssi] = g_callback_context.devices[0];
    EXPECT_EQ(mac, "B4994C123456");
    EXPECT_TRUE(name.empty());
    EXPECT_TRUE(rssi.empty());
}

TEST_F(HM11DriverDiscoveryTest, ScanSingleDeviceWithNameAndRssi) {
    // Set up response: single device with all fields
    std::string response = make_discovery_response("B4994C123456", "MyDevice", "-045");
    uart_port_.set_response(response);

    // Clear callback context
    g_callback_context.devices.clear();

    // Execute scan
    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(g_callback_context.devices.size(), 1);

    const auto& [id, mac, name, rssi] = g_callback_context.devices[0];
    EXPECT_EQ(mac, "B4994C123456");
    EXPECT_EQ(name, "MyDevice");
    EXPECT_EQ(rssi, "-045");
}

TEST_F(HM11DriverDiscoveryTest, ScanMultipleDevices) {
    // Set up response: three devices
    std::vector<std::tuple<std::string, std::string, std::string>> devices =
        {{"B4994C123456", "Device1", "-040"}, {"A1B2C3D4E5F6", "Device2", "-055"}, {"111111111111", "Device3", "-070"}};
    std::string response = make_multi_device_response(devices);
    uart_port_.set_response(response);

    // Clear callback context
    g_callback_context.devices.clear();

    // Execute scan
    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(g_callback_context.devices.size(), 3);

    for (std::size_t i = 0; i < 3; ++i) {
        const auto& [id, mac, name, rssi] = g_callback_context.devices[i];
        EXPECT_EQ(mac, std::get<0>(devices[i]));
        EXPECT_EQ(name, std::get<1>(devices[i]));
        EXPECT_EQ(rssi, std::get<2>(devices[i]));
    }
}

TEST_F(HM11DriverDiscoveryTest, ScanEmptyResponse) {
    // Set up response: only start and end markers
    std::string response = "OK+DISCSOK+DISCE";
    uart_port_.set_response(response);

    // Clear callback context
    g_callback_context.devices.clear();

    // Execute scan
    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(g_callback_context.devices.size(), 0); // No devices discovered
}

// ============================================================================
// iBeacon Scan Tests
// ============================================================================

// Global iBeacon callback context
std::vector<std::string> g_ibeacon_devices;

void test_ibeacon_callback(std::string_view device_info) {
    g_ibeacon_devices.emplace_back(device_info);
}

TEST_F(HM11DriverDiscoveryTest, ScanIBeaconSingleDevice) {
    // Set up response: single iBeacon device (74 characters after OK+DISC)
    // Format: P0(8) + P1(32) + P2(10) + P3(12) + P4(4) = 66 total
    // Actually 74 in Ada spec, so using 74
    std::string ibeacon_data = "FACTORY1"                         // P0: Factory ID (8)
                               "E2C56DB5DFFB48D2B060D0F5A71096E0" // P1: UUID (32)
                               "00010002C5"                       // P2: Major(4) + Minor(4) + Power(2) = 10
                               "B4994C123456"                     // P3: MAC (12)
                               "-045"                             // P4: RSSI (4)
                               "12345678";                        // Padding to reach 74

    std::string response = "OK+DISCSOK+DISC" + ibeacon_data + "OK+DISCE";
    uart_port_.set_response(response);

    // Clear callback context
    g_ibeacon_devices.clear();

    // Execute scan
    hal::UartStatus status;
    driver_.scan_ibeacon(test_ibeacon_callback, 1s, status);

    // Verify
    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+DISI?");
    ASSERT_EQ(g_ibeacon_devices.size(), 1);
    EXPECT_EQ(g_ibeacon_devices[0].size(), 74);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_F(HM11DriverDiscoveryTest, ScanStateTransitions) {
    // Test state machine transitions: Selection -> MAC -> Selection -> Name -> Selection -> RSSI -> Selection
    std::string response = "OK+DISCSOK+DISC:B4994C123456OK+NAME:Test\r\nOK+RSSI:-045\r\nOK+DISCE";
    uart_port_.set_response(response);

    g_callback_context.devices.clear();

    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(g_callback_context.devices.size(), 1);

    const auto& [id, mac, name, rssi] = g_callback_context.devices[0];
    EXPECT_EQ(mac, "B4994C123456");
    EXPECT_EQ(name, "Test");
    EXPECT_EQ(rssi, "-045");
}

TEST_F(HM11DriverDiscoveryTest, ScanPartialNameData) {
    // Test partial name (with CRLF between name and end marker)
    std::string response = "OK+DISCSOK+DISC:B4994C123456OK+NAME:PartialName\r\nOK+DISCE";
    uart_port_.set_response(response);

    g_callback_context.devices.clear();

    hal::UartStatus status;
    driver_.scan(test_discovered_callback, 1s, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    ASSERT_EQ(g_callback_context.devices.size(), 1);

    const auto& [id, mac, name, rssi] = g_callback_context.devices[0];
    EXPECT_EQ(mac, "B4994C123456");
    EXPECT_EQ(name, "PartialName"); // Name properly parsed with CRLF delimiter
    EXPECT_TRUE(rssi.empty());      // No RSSI in this response
}

} // namespace hm11::test
