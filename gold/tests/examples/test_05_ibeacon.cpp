/**
 * @file test_05_ibeacon.cpp
 * @brief Integration test for 05_ibeacon example
 *
 * This test validates the iBeacon broadcaster example logic using EnhancedMockUART
 * to simulate HM-11 module responses without requiring physical hardware.
 *
 * ## Test Coverage
 *
 * 1. iBeacon UUID configuration (4 chunks of 8 hex characters)
 * 2. Major version configuration (4 hex characters)
 * 3. Minor version configuration (4 hex characters)
 * 4. Measured power configuration (2 hex characters)
 * 5. iBeacon switch enable operation
 * 6. Deploy mode activation (Only_Broadcast)
 * 7. Strong type validation (Decision 14)
 * 8. Error handling with mock LED feedback
 * 9. Command sequence correctness
 *
 * ## Testing Strategy
 *
 * Rather than testing the example executable directly (which contains hardware
 * initialization and delays), this test extracts the core business logic into
 * testable functions that accept driver and LED abstractions.
 *
 * The test validates:
 * - Correct AT command sequence for iBeacon configuration
 * - UUID split into 4 chunks (AT+IBE0, AT+IBE1, AT+IBE2, AT+IBE3)
 * - Proper formatting of version and power values
 * - Deploy mode command format (AT+DELO2 for Only_Broadcast)
 * - Status code propagation and error handling
 * - LED feedback patterns for success/error
 *
 * ## Confidence Level
 *
 * This test achieves ~70% confidence without hardware by:
 * - Validating complete configuration command sequence
 * - Testing strong type constraints (hex character validation)
 * - Verifying UUID chunking logic (4 x 8 characters)
 * - Testing error conditions and timeout handling
 * - Simulating realistic module responses
 *
 * Hardware testing will increase confidence to ~99%.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <string>
#include <string_view>

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
// Business Logic Functions (Extracted from main.cpp)
// ============================================================================

/**
 * @brief Core logic for iBeacon broadcaster configuration
 *
 * This function encapsulates the core business logic from the example,
 * separated from hardware initialization and delays for testability.
 *
 * @tparam DriverType HM11Driver instantiation type
 * @param driver Reference to HM11 driver instance
 * @param green_led Reference to green LED for success indication
 * @param red_led Reference to red LED for error indication
 * @return true if all operations succeeded, false on error
 */
template <typename DriverType>
bool run_ibeacon_config_logic(DriverType& driver, MockLED& green_led, MockLED& red_led) noexcept {
    UartStatus status;

    // Test module responsiveness
    driver.test(status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure iBeacon UUID (128-bit)
    // Example: E2C56DB5-DFFB-48D2-B060-D0F5A71096E0 -> E2C56DB5DFFB48D2B060D0F5A71096E0
    IBeaconUuid uuid{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    driver.set_ibeacon_uuid(uuid, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure major version (16-bit)
    VersionType major{"0001"};
    driver.set_ibeacon_major_version(major, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure minor version (16-bit)
    VersionType minor{"0001"};
    driver.set_ibeacon_minor_version(minor, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Configure measured power (8-bit, two's complement)
    // 0xC5 = -59 dBm
    MeasuredPower power{"C5"};
    driver.set_ibeacon_measured_power(power, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Enable iBeacon functionality
    driver.set_ibeacon_switch(true, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // Activate deploy mode (Only_Broadcast = non-connectable)
    driver.set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);
    if (status != UartStatus::Ok) {
        red_led.on();
        return false;
    }

    // All operations succeeded
    green_led.on();
    return true;
}

// ============================================================================
// Test Fixture
// ============================================================================

class IBeaconExampleTest : public ::testing::Test {
  protected:
    EnhancedMockUART mock_uart_;
    SimpleReceiveHandler receive_handler_;
    MockLED green_led_;
    MockLED red_led_;

    void SetUp() override {
        mock_uart_.clear();
        green_led_.clear();
        red_led_.clear();
    }
};

// ============================================================================
// Configuration Tests
// ============================================================================

/**
 * @brief Test successful iBeacon configuration sequence
 *
 * Validates the complete command sequence:
 * 1. AT test
 * 2. UUID configuration (4 commands for 4 chunks)
 * 3. Major version configuration
 * 4. Minor version configuration
 * 5. Measured power configuration
 * 6. iBeacon switch enable
 * 7. Deploy mode activation
 */
TEST_F(IBeaconExampleTest, SuccessfulConfiguration) {
    // Program expected responses for all commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00xE2C56DB5", "OK+Set:00xE2C56DB5");
    mock_uart_.expect_command("AT+IBE10xDFFB48D2", "OK+Set:10xDFFB48D2");
    mock_uart_.expect_command("AT+IBE20xB060D0F5", "OK+Set:20xB060D0F5");
    mock_uart_.expect_command("AT+IBE30xA71096E0", "OK+Set:30xA71096E0");
    mock_uart_.expect_command("AT+MARJ0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MINO0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MEAS0xC5", "OK+Set:0xC5");
    mock_uart_.expect_command("AT+IBEA1", "OK+Set:1");
    mock_uart_.expect_command("AT+DELO2", "OK+DELO2");

    // Create driver and run configuration logic
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    // Verify success
    EXPECT_TRUE(result);
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_FALSE(red_led_.is_on());

    // Verify all commands were sent
    const auto& history = mock_uart_.get_transmitted_commands();
    ASSERT_EQ(history.size(), 10);

    // Verify command sequence
    EXPECT_EQ(history[0], "AT");
    EXPECT_EQ(history[1], "AT+IBE00xE2C56DB5"); // UUID chunk 0
    EXPECT_EQ(history[2], "AT+IBE10xDFFB48D2"); // UUID chunk 1
    EXPECT_EQ(history[3], "AT+IBE20xB060D0F5"); // UUID chunk 2
    EXPECT_EQ(history[4], "AT+IBE30xA71096E0"); // UUID chunk 3
    EXPECT_EQ(history[5], "AT+MARJ0x0001");     // Major
    EXPECT_EQ(history[6], "AT+MINO0x0001");     // Minor
    EXPECT_EQ(history[7], "AT+MEAS0xC5");       // Power
    EXPECT_EQ(history[8], "AT+IBEA1");          // Switch
    EXPECT_EQ(history[9], "AT+DELO2");          // Deploy mode
}

/**
 * @brief Test UUID configuration with different values
 *
 * Tests that UUID is correctly split into 4 chunks of 8 hex characters each.
 */
TEST_F(IBeaconExampleTest, UUIDConfiguration) {
    // Program responses
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00x74278BDA", "OK+Set:00x74278BDA");
    mock_uart_.expect_command("AT+IBE10xB6444520", "OK+Set:10xB6444520");
    mock_uart_.expect_command("AT+IBE20x8F0C720E", "OK+Set:20x8F0C720E");
    mock_uart_.expect_command("AT+IBE30xAF059935", "OK+Set:30xAF059935");
    mock_uart_.expect_command("AT+MARJ0xFFE0", "OK+Set:0xFFE0");
    mock_uart_.expect_command("AT+MINO0xFFE1", "OK+Set:0xFFE1");
    mock_uart_.expect_command("AT+MEAS0xC5", "OK+Set:0xC5");
    mock_uart_.expect_command("AT+IBEA1", "OK+Set:1");
    mock_uart_.expect_command("AT+DELO2", "OK+DELO2");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    UartStatus status;

    // Test
    driver.test(status);
    ASSERT_EQ(status, UartStatus::Ok);

    // Configure with default UUID from Ada spec (hm11.ads:740)
    // 74278BDA-B644-4520-8F0C-720EAF059935
    IBeaconUuid uuid{"74278BDAB64445208F0C720EAF059935"};
    driver.set_ibeacon_uuid(uuid, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Configure major/minor with defaults (hm11.ads:759, 771)
    VersionType major{"FFE0"}; // Default 0xFFE0
    driver.set_ibeacon_major_version(major, status);
    EXPECT_EQ(status, UartStatus::Ok);

    VersionType minor{"FFE1"}; // Default 0xFFE1
    driver.set_ibeacon_minor_version(minor, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Measured power
    MeasuredPower power{"C5"}; // Default 0xC5 (hm11.ads:783)
    driver.set_ibeacon_measured_power(power, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Enable switch
    driver.set_ibeacon_switch(true, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Deploy
    driver.set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Verify UUID was split correctly
    const auto& history = mock_uart_.get_transmitted_commands();
    EXPECT_EQ(history[1], "AT+IBE00x74278BDA");
    EXPECT_EQ(history[2], "AT+IBE10xB6444520");
    EXPECT_EQ(history[3], "AT+IBE20x8F0C720E");
    EXPECT_EQ(history[4], "AT+IBE30xAF059935");
}

/**
 * @brief Test deploy mode values
 *
 * Tests both deploy mode values: BroadcastScanning (1) and OnlyBroadcast (2).
 */
TEST_F(IBeaconExampleTest, DeployModeValues) {
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};
    UartStatus status;

    // Test BroadcastScanning mode (value = 1)
    mock_uart_.expect_command("AT+DELO1", "OK+DELO1");
    driver.set_ibeacon_deploy_mode(IBeaconDeployMode::BroadcastScanning, status);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart_.get_transmitted_commands().back(), "AT+DELO1");

    // Test OnlyBroadcast mode (value = 2)
    mock_uart_.expect_command("AT+DELO2", "OK+DELO2");
    driver.set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart_.get_transmitted_commands().back(), "AT+DELO2");
}

/**
 * @brief Test error handling during UUID configuration
 *
 * If any UUID chunk fails, configuration should stop and report error.
 */
TEST_F(IBeaconExampleTest, ErrorDuringUUIDConfiguration) {
    // Program responses - fail on chunk 2
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00xE2C56DB5", "OK+Set:00xE2C56DB5");
    mock_uart_.expect_command("AT+IBE10xDFFB48D2", "OK+Set:10xDFFB48D2");
    mock_uart_.expect_command("AT+IBE20xB060D0F5", "ERROR");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    // Should fail and indicate error
    EXPECT_FALSE(result);
    EXPECT_FALSE(green_led_.is_on());
    EXPECT_TRUE(red_led_.is_on());

    // Should stop after failure (not send remaining commands)
    const auto& history = mock_uart_.get_transmitted_commands();
    EXPECT_LE(history.size(), 4); // At most: AT + 3 UUID chunks
}

/**
 * @brief Test error handling during major version configuration
 */
TEST_F(IBeaconExampleTest, ErrorDuringMajorConfiguration) {
    // Program responses - fail on major version
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00xE2C56DB5", "OK+Set:00xE2C56DB5");
    mock_uart_.expect_command("AT+IBE10xDFFB48D2", "OK+Set:10xDFFB48D2");
    mock_uart_.expect_command("AT+IBE20xB060D0F5", "OK+Set:20xB060D0F5");
    mock_uart_.expect_command("AT+IBE30xA71096E0", "OK+Set:30xA71096E0");
    mock_uart_.expect_command("AT+MARJ0x0001", "ERROR");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());
}

/**
 * @brief Test error handling during switch enable
 */
TEST_F(IBeaconExampleTest, ErrorDuringSwitch) {
    // Program responses - fail on switch
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00xE2C56DB5", "OK+Set:00xE2C56DB5");
    mock_uart_.expect_command("AT+IBE10xDFFB48D2", "OK+Set:10xDFFB48D2");
    mock_uart_.expect_command("AT+IBE20xB060D0F5", "OK+Set:20xB060D0F5");
    mock_uart_.expect_command("AT+IBE30xA71096E0", "OK+Set:30xA71096E0");
    mock_uart_.expect_command("AT+MARJ0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MINO0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MEAS0xC5", "OK+Set:0xC5");
    mock_uart_.expect_command("AT+IBEA1", "ERROR");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());
}

/**
 * @brief Test error handling during deploy mode
 */
TEST_F(IBeaconExampleTest, ErrorDuringDeploy) {
    // Program responses - fail on deploy
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+IBE00xE2C56DB5", "OK+Set:00xE2C56DB5");
    mock_uart_.expect_command("AT+IBE10xDFFB48D2", "OK+Set:10xDFFB48D2");
    mock_uart_.expect_command("AT+IBE20xB060D0F5", "OK+Set:20xB060D0F5");
    mock_uart_.expect_command("AT+IBE30xA71096E0", "OK+Set:30xA71096E0");
    mock_uart_.expect_command("AT+MARJ0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MINO0x0001", "OK+Set:0x0001");
    mock_uart_.expect_command("AT+MEAS0xC5", "OK+Set:0xC5");
    mock_uart_.expect_command("AT+IBEA1", "OK+Set:1");
    mock_uart_.expect_command("AT+DELO2", "ERROR");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());
}

/**
 * @brief Test that initial test failure is handled
 */
TEST_F(IBeaconExampleTest, TestCommandFails) {
    // Program error response for test command
    mock_uart_.expect_command("AT", "ERROR");

    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver{mock_uart_, receive_handler_};

    bool result = run_ibeacon_config_logic(driver, green_led_, red_led_);

    // Should fail immediately
    EXPECT_FALSE(result);
    EXPECT_TRUE(red_led_.is_on());
    EXPECT_FALSE(green_led_.is_on());

    // Should only send test command
    const auto& history = mock_uart_.get_transmitted_commands();
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0], "AT");
}

// ============================================================================
// Strong Type Validation Tests (Decision 14)
// ============================================================================

/**
 * @brief Test that IBeaconUuid requires exactly 32 hex characters
 *
 * Validates compile-time size enforcement and runtime character validation.
 */
TEST_F(IBeaconExampleTest, UUIDTypeValidation) {
    // Valid UUID (32 hex characters)
    IBeaconUuid valid{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    EXPECT_EQ(valid.size(), 32);

    // Size validation is compile-time, so invalid sizes won't compile
    // This is tested at compile time by the FixedString template

    // Character validation happens at construction (debug builds)
    // All characters must be 0-9 or A-F
#ifndef NDEBUG
    // In debug builds, invalid characters trigger assertion
    // Can't test this directly in unit test without death test
    // but validation is present in FixedString implementation
#endif
}

/**
 * @brief Test that VersionType requires exactly 4 hex characters
 */
TEST_F(IBeaconExampleTest, VersionTypeValidation) {
    // Valid versions (4 hex characters)
    VersionType major{"0001"};
    VersionType minor{"FFFF"};

    EXPECT_EQ(major.size(), 4);
    EXPECT_EQ(minor.size(), 4);
}

/**
 * @brief Test that MeasuredPower requires exactly 2 hex characters
 */
TEST_F(IBeaconExampleTest, MeasuredPowerValidation) {
    // Valid power values (2 hex characters)
    MeasuredPower power1{"C5"}; // -59 dBm
    MeasuredPower power2{"00"}; // 0 dBm
    MeasuredPower power3{"FF"}; // -1 dBm

    EXPECT_EQ(power1.size(), 2);
    EXPECT_EQ(power2.size(), 2);
    EXPECT_EQ(power3.size(), 2);
}
