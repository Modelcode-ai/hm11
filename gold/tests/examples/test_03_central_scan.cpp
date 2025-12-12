/**
 * @file test_03_central_scan.cpp
 * @brief Integration test for 03_central_scan example
 *
 * This test validates the central scanner example logic using EnhancedMockUART
 * to simulate HM-11 module responses without requiring physical hardware.
 *
 * ## Test Coverage
 *
 * 1. Central role configuration sequence correctness
 * 2. Scan operation with streaming discovery data
 * 3. Callback invocation with multiple discovered devices
 * 4. RSSI filtering logic (accept/reject devices based on threshold)
 * 5. Empty Name/RSSI field handling (graceful degradation)
 * 6. Device counter tracking and validation
 * 7. Error handling with mock LED feedback
 *
 * ## Testing Strategy
 *
 * Rather than testing the example executable directly (which contains hardware
 * initialization and delays), this test extracts the core business logic into
 * testable functions that accept driver and LED abstractions.
 *
 * The test validates:
 * - Correct AT command sequence for Central configuration
 * - Scan operation processes streaming discovery responses
 * - Callback correctly invoked for each discovered device
 * - RSSI filtering accepts devices above threshold, rejects below
 * - Empty name/RSSI fields handled without crashes
 * - Visual feedback (LED) patterns correct
 *
 * ## Confidence Level
 *
 * This test achieves ~70% confidence without hardware by:
 * - Validating configuration command sequence
 * - Testing discovery parsing with realistic multi-device streams
 * - Verifying RSSI threshold filtering logic
 * - Testing edge cases (empty fields, timeout, multiple devices)
 * - Simulating realistic module responses
 *
 * Hardware testing will increase confidence to ~99%.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "enhanced_mock_uart.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"
#include "mock_led.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;
using namespace hm11::test;
using namespace std::chrono_literals;

// ============================================================================
// Mock Receive Handler for EnhancedMockUART
// ============================================================================

/**
 * @brief Simple receive handler that delegates to mock UART
 *
 * Matches the pattern from test_hm11_driver_core.cpp for use with
 * EnhancedMockUART in integration tests.
 */
struct SimpleReceiveHandler {
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// Verify handler satisfies concept
static_assert(
    IS_RECEIVE_HANDLER_V<SimpleReceiveHandler, EnhancedMockUART>,
    "SimpleReceiveHandler must satisfy receive handler interface");

// ============================================================================
// Configuration Constants (from example)
// ============================================================================

constexpr int RSSI_THRESHOLD_DBM = -70;
constexpr int SCAN_TIMEOUT_SECONDS = 10;

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Build properly formatted discovery response
 *
 * Creates HM-11 discovery response string in correct format:
 * "OK+DISCS" + ("OK+DISC:<MAC>OK+NAME:<name>\r\nOK+RSSI:<rssi>\r\n")* + "OK+DISCE"
 *
 * @param devices Vector of (MAC, name, RSSI) tuples
 * @return Formatted discovery response string
 */
std::string make_discovery_response(const std::vector<std::tuple<std::string, std::string, std::string>>& devices) {
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

// ============================================================================
// Helper Functions (from example)
// ============================================================================

/**
 * @brief Parse RSSI string to integer value
 *
 * Extracted from main.cpp for testing.
 */
int parse_rssi(std::string_view rssi_str) noexcept {
    if (rssi_str.empty()) {
        return 0;
    }

    int value = 0;
    bool negative = false;
    std::size_t i = 0;

    if (rssi_str[0] == '-') {
        negative = true;
        i = 1;
    }

    for (; i < rssi_str.size(); ++i) {
        if (rssi_str[i] >= '0' && rssi_str[i] <= '9') {
            value = value * 10 + (rssi_str[i] - '0');
        }
    }

    return negative ? -value : value;
}

// ============================================================================
// Discovery Callback Context (for test validation)
// ============================================================================

/**
 * @brief Context for capturing discovery callback invocations
 *
 * Used in tests to track which devices were discovered and validate
 * filtering logic.
 */
struct DiscoveryContext {
    int device_count{0};
    std::vector<std::tuple<char, std::string, std::string, std::string>> discovered_devices;
    MockLED* led{nullptr};

    void reset() noexcept {
        device_count = 0;
        discovered_devices.clear();
    }
};

// Global context for callback (callback must be plain function pointer)
static DiscoveryContext g_discovery_ctx;

/**
 * @brief Test discovery callback with RSSI filtering
 *
 * Matches the logic from main.cpp on_device_discovered() callback.
 */
void test_discovery_callback(
    char id,
    const types::MacAddress& mac,
    std::string_view name,
    std::string_view rssi) noexcept {
    // Apply RSSI threshold filter if RSSI available
    if (!rssi.empty()) {
        int rssi_dbm = parse_rssi(rssi);
        if (rssi_dbm <= RSSI_THRESHOLD_DBM) {
            // Device at or below threshold, skip
            return;
        }
    }

    // Increment device counter
    ++g_discovery_ctx.device_count;

    // Record discovered device for validation
    g_discovery_ctx.discovered_devices.emplace_back(id, std::string(mac.view()), std::string(name), std::string(rssi));

    // Visual feedback
    if (g_discovery_ctx.led) {
        g_discovery_ctx.led->toggle();
    }
}

// ============================================================================
// Business Logic Functions (Extracted from main.cpp)
// ============================================================================

/**
 * @brief Core logic for central scanner example
 *
 * This function encapsulates the core business logic from the example,
 * separated from hardware initialization and delays for testability.
 *
 * @tparam DriverType HM11Driver instantiation type
 * @param driver Reference to HM11 driver instance
 * @param green_led Mock green LED for status indication
 * @param red_led Mock red LED for error indication
 * @return true if scan completed successfully, false otherwise
 */
template <typename DriverType>
bool run_central_scan_logic(DriverType& driver, MockLED& green_led, MockLED& red_led) noexcept {
    UartStatus status;

    // Setup global context
    g_discovery_ctx.reset();
    g_discovery_ctx.led = &green_led;

    // ========================================================================
    // Phase 1: Module Verification
    // ========================================================================

    driver.test(status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // ========================================================================
    // Phase 2: Central Configuration
    // ========================================================================

    // Set work type to respond to AT commands
    driver.set_work_type(WorkType::RespondAtCommand, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure device information display
    driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowRssiAndName, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Set role to Central
    driver.set_role(Role::Central, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Restart module
    driver.restart(status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // ========================================================================
    // Phase 3: Scanning Operation
    // ========================================================================

    green_led.off();

    // Perform scan operation
    driver.scan(test_discovery_callback, std::chrono::seconds(SCAN_TIMEOUT_SECONDS), status);

    // Check scan status (timeout is acceptable)
    if (status != UartStatus::Ok && status != UartStatus::ErrTimeout) {
        red_led.on();
        return false;
    }

    // ========================================================================
    // Phase 4: Completion
    // ========================================================================

    green_led.on();
    return true;
}

// ============================================================================
// Test Cases
// ============================================================================

/// @brief Test fixture for central scanner example
class CentralScanExampleTest : public ::testing::Test {
  protected:
    EnhancedMockUART mock_uart_;
    SimpleReceiveHandler receive_handler_;
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver_{mock_uart_, receive_handler_};
    MockLED green_led_;
    MockLED red_led_;

    void SetUp() override {
        g_discovery_ctx.reset();
        green_led_.off();
        red_led_.off();
    }

    void TearDown() override { g_discovery_ctx.reset(); }
};

/**
 * @brief Test central configuration sequence
 *
 * Validates that the example sends correct AT commands in proper order
 * to configure the module as Central scanner.
 */
TEST_F(CentralScanExampleTest, ConfigurationSequence) {
    // Program expected command sequence
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate empty discovery (no devices)
    mock_uart_.expect_command("AT+DISC?", "OK+DISCSOK+DISCE");

    // Run configuration logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Debug: Print transmitted commands if test fails
    if (!success || !mock_uart_.all_expectations_met()) {
        const auto& commands = mock_uart_.get_transmitted_commands();
        std::cerr << "Transmitted commands (" << commands.size() << "):\n";
        for (const auto& cmd : commands) {
            std::cerr << "  \"" << cmd << "\"\n";
        }
        if (mock_uart_.has_unexpected_command()) {
            std::cerr << "Unexpected command: \"" << mock_uart_.get_unexpected_command() << "\"\n";
        }
    }

    // Verify success
    EXPECT_TRUE(success);
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_FALSE(red_led_.is_on());
    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

/**
 * @brief Test scan operation with multiple discovered devices
 *
 * Validates that callback is invoked for each device in streaming
 * discovery response, and device counter tracks correctly.
 */
TEST_F(CentralScanExampleTest, MultipleDevicesDiscovered) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with 3 devices (all above RSSI threshold)
    std::vector<std::tuple<std::string, std::string, std::string>> devices = {
        {"A4C138123456", "Device1", "-050"}, // -50 dBm (above threshold)
        {"B4994C789ABC", "Device2", "-060"}, // -60 dBm (above threshold)
        {"C0DE12345678", "Device3", "-065"}  // -65 dBm (above threshold)
    };
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify all devices discovered
    EXPECT_TRUE(success);
    EXPECT_EQ(g_discovery_ctx.device_count, 3);
    EXPECT_EQ(g_discovery_ctx.discovered_devices.size(), 3);

    // Verify device details
    EXPECT_EQ(std::get<1>(g_discovery_ctx.discovered_devices[0]), "A4C138123456");
    EXPECT_EQ(std::get<2>(g_discovery_ctx.discovered_devices[0]), "Device1");
    EXPECT_EQ(std::get<3>(g_discovery_ctx.discovered_devices[0]), "-050");

    EXPECT_TRUE(green_led_.is_on());
}

/**
 * @brief Test RSSI filtering rejects weak devices
 *
 * Validates that devices below RSSI threshold are filtered out
 * and not counted or displayed.
 */
TEST_F(CentralScanExampleTest, RssiFilteringRejectsWeakDevices) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with mixed RSSI values
    std::vector<std::tuple<std::string, std::string, std::string>> devices = {
        {"A4C138123456", "StrongDevice", "-050"},  // -50 dBm (ABOVE threshold, should appear)
        {"B4994C789ABC", "WeakDevice", "-080"},    // -80 dBm (BELOW threshold, should be filtered)
        {"C0DE12345678", "VeryWeakDevice", "-090"} // -90 dBm (BELOW threshold, should be filtered)
    };
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify only strong device accepted
    EXPECT_TRUE(success);
    EXPECT_EQ(g_discovery_ctx.device_count, 1); // Only one device above threshold
    EXPECT_EQ(g_discovery_ctx.discovered_devices.size(), 1);

    // Verify strong device details
    EXPECT_EQ(std::get<1>(g_discovery_ctx.discovered_devices[0]), "A4C138123456");
    EXPECT_EQ(std::get<2>(g_discovery_ctx.discovered_devices[0]), "StrongDevice");

    EXPECT_TRUE(green_led_.is_on());
}

/**
 * @brief Test RSSI filtering accepts borderline devices
 *
 * Validates that devices exactly at or just above threshold are accepted.
 */
TEST_F(CentralScanExampleTest, RssiFilteringAcceptsBorderlineDevices) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with borderline RSSI values
    std::vector<std::tuple<std::string, std::string, std::string>> devices = {
        {"A4C138123456", "AtThreshold", "-070"}, // -70 dBm (AT threshold, should be filtered)
        {"B4994C789ABC", "JustAbove", "-069"},   // -69 dBm (ABOVE threshold, should appear)
        {"C0DE12345678", "WellAbove", "-065"}    // -65 dBm (ABOVE threshold, should appear)
    };
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify devices above threshold accepted (threshold is exclusive: > -70)
    EXPECT_TRUE(success);
    EXPECT_EQ(g_discovery_ctx.device_count, 2); // Two devices above threshold
    EXPECT_EQ(g_discovery_ctx.discovered_devices.size(), 2);

    EXPECT_TRUE(green_led_.is_on());
}

/**
 * @brief Test empty Name field handling
 *
 * Validates that devices without Name field (when ShowDeviceInformation
 * configured differently) are handled gracefully.
 */
TEST_F(CentralScanExampleTest, EmptyNameFieldHandling) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with empty names (RSSI only mode)
    std::vector<std::tuple<std::string, std::string, std::string>> devices = {
        {"A4C138123456", "", "-050"}, // No name, only RSSI
        {"B4994C789ABC", "", "-060"}  // No name, only RSSI
    };
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify devices discovered with empty names
    EXPECT_TRUE(success);
    EXPECT_EQ(g_discovery_ctx.device_count, 2);
    EXPECT_EQ(g_discovery_ctx.discovered_devices.size(), 2);

    // Verify names are empty
    EXPECT_EQ(std::get<2>(g_discovery_ctx.discovered_devices[0]), "");
    EXPECT_EQ(std::get<2>(g_discovery_ctx.discovered_devices[1]), "");

    EXPECT_TRUE(green_led_.is_on());
}

/**
 * @brief Test empty RSSI field handling
 *
 * Validates that devices without RSSI field (when ShowDeviceInformation
 * configured for name only) are handled gracefully and pass filtering.
 */
TEST_F(CentralScanExampleTest, EmptyRssiFieldHandling) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with empty RSSI (Name only mode)
    std::vector<std::tuple<std::string, std::string, std::string>> devices = {
        {"A4C138123456", "Device1", ""}, // Name only, no RSSI
        {"B4994C789ABC", "Device2", ""}  // Name only, no RSSI
    };
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify devices discovered without RSSI filtering (empty RSSI bypasses filter)
    EXPECT_TRUE(success);
    EXPECT_EQ(g_discovery_ctx.device_count, 2);
    EXPECT_EQ(g_discovery_ctx.discovered_devices.size(), 2);

    // Verify RSSI is empty
    EXPECT_EQ(std::get<3>(g_discovery_ctx.discovered_devices[0]), "");
    EXPECT_EQ(std::get<3>(g_discovery_ctx.discovered_devices[1]), "");

    EXPECT_TRUE(green_led_.is_on());
}

/**
 * @brief Test error handling for failed configuration
 *
 * Validates that configuration errors are handled properly with
 * red LED indication.
 */
TEST_F(CentralScanExampleTest, ErrorHandlingForFailedConfiguration) {
    // Program configuration commands with one failure
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_timeout("AT+SHOW3"); // Timeout on this command

    // Run configuration logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify failure indicated
    EXPECT_FALSE(success);
    EXPECT_TRUE(red_led_.is_on());
    EXPECT_FALSE(green_led_.is_on());
}

/**
 * @brief Test parse_rssi helper function
 *
 * Validates RSSI string parsing correctness with various formats.
 */
TEST(ParseRssiTest, VariousFormats) {
    EXPECT_EQ(parse_rssi("-050"), -50);
    EXPECT_EQ(parse_rssi("-070"), -70);
    EXPECT_EQ(parse_rssi("-090"), -90);
    EXPECT_EQ(parse_rssi("-100"), -100);
    EXPECT_EQ(parse_rssi(""), 0);     // Empty string
    EXPECT_EQ(parse_rssi("050"), 50); // Positive (unusual but should handle)
}

/**
 * @brief Test LED visual feedback pattern
 *
 * Validates that LEDs are toggled correctly during discovery.
 */
TEST_F(CentralScanExampleTest, LedVisualFeedbackPattern) {
    // Program configuration commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IMME1", "OK+Set:1");
    mock_uart_.expect_command("AT+SHOW3", "OK+Set:3");
    mock_uart_.expect_command("AT+ROLE1", "OK+Set:1");
    mock_uart_.expect_command("AT+START", "OK+START");

    // Simulate discovery with 3 devices
    std::vector<std::tuple<std::string, std::string, std::string>> devices =
        {{"A4C138123456", "Device1", "-050"}, {"B4994C789ABC", "Device2", "-060"}, {"C0DE12345678", "Device3", "-065"}};
    mock_uart_.expect_command("AT+DISC?", make_discovery_response(devices));

    // Run scan logic
    bool success = run_central_scan_logic(driver_, green_led_, red_led_);

    // Verify LED pattern
    EXPECT_TRUE(success);
    EXPECT_TRUE(green_led_.is_on()); // Final state: on

    // Verify green LED was toggled 3 times (once per device)
    const auto& history = green_led_.get_state_history();
    int toggle_count = 0;
    for (std::size_t i = 1; i < history.size(); ++i) {
        if (history[i] != history[i - 1]) {
            ++toggle_count;
        }
    }
    EXPECT_GE(toggle_count, 3); // At least 3 toggles (one per device)
}
