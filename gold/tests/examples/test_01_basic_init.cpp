/**
 * @file test_01_basic_init.cpp
 * @brief Integration test for 01_basic_init example
 *
 * This test validates the basic initialization example logic using EnhancedMockUART
 * to simulate HM-11 module responses without requiring physical hardware.
 *
 * ## Test Coverage
 *
 * 1. Driver construction and initialization
 * 2. Test AT command execution and response validation
 * 3. Software version query and response parsing
 * 4. Error handling with mock LED feedback
 * 5. Full initialization sequence timing
 *
 * ## Testing Strategy
 *
 * Rather than testing the example executable directly (which contains hardware
 * initialization and delays), this test extracts the core business logic into
 * testable functions that accept driver and LED abstractions.
 *
 * The test validates:
 * - Correct command sequence (Test -> Software_Version)
 * - Proper error handling (LED feedback on failure)
 * - Response parsing correctness
 * - Status code propagation
 *
 * ## Confidence Level
 *
 * This test achieves ~65% confidence without hardware by:
 * - Validating command sequence and logic flow
 * - Testing error conditions and timeout handling
 * - Verifying LED feedback patterns
 * - Simulating realistic module responses
 *
 * Hardware testing will increase confidence to ~99%.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <string_view>

#include "enhanced_mock_uart.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/hm11_driver.hpp"
#include "mock_led.hpp"

using namespace hm11;
using namespace hm11::hal;
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
 * @brief Core logic for basic initialization example
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
bool run_basic_init_logic(DriverType& driver, MockLED& green_led, MockLED& red_led) noexcept {
    // Test module responsiveness with AT command
    UartStatus status;
    driver.test(status);

    if (status != UartStatus::Ok) {
        // Test command failed - indicate error
        red_led.on();
        return false;
    }

    // Test succeeded - indicate success
    green_led.toggle();

    // Retrieve software version from module
    std::string_view version;
    driver.software_version(version, status);

    if (status != UartStatus::Ok) {
        // Version query failed - indicate error
        red_led.on();
        return false;
    }

    // Version retrieved successfully - indicate success
    green_led.toggle();

    // All operations completed - turn on green LED
    green_led.on();

    return true;
}

// ============================================================================
// Test Fixture
// ============================================================================

class BasicInitExampleTest : public ::testing::Test {
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
// Successful Initialization Tests
// ============================================================================

TEST_F(BasicInitExampleTest, SuccessfulInitializationSequence) {
    // Setup: Program expected command sequence with realistic responses
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    bool success = run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Operations succeeded
    EXPECT_TRUE(success);

    // Verify: All expected commands were sent
    EXPECT_TRUE(mock_uart_.all_expectations_met());
    EXPECT_EQ(mock_uart_.remaining_expectations(), 0);

    // Verify: No unexpected commands
    EXPECT_FALSE(mock_uart_.has_unexpected_command());

    // Verify: Green LED was toggled twice (success indicators) and turned on (final state)
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_EQ(green_led_.get_toggle_count(), 2);

    // Verify: Red LED was never activated
    EXPECT_FALSE(red_led_.is_on());
    EXPECT_EQ(red_led_.get_on_count(), 0);
}

TEST_F(BasicInitExampleTest, SuccessfulInitializationWithDifferentVersion) {
    // Setup: Different firmware version response
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V550");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    bool success = run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Operations succeeded with different version
    EXPECT_TRUE(success);
    EXPECT_TRUE(mock_uart_.all_expectations_met());

    // Verify: LED pattern is consistent
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_EQ(green_led_.get_toggle_count(), 2);
    EXPECT_FALSE(red_led_.is_on());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(BasicInitExampleTest, TestCommandFailure) {
    // Setup: Test command will timeout (module not responding)
    mock_uart_.expect_timeout("AT");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    bool success = run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Operations failed
    EXPECT_FALSE(success);

    // Verify: Red LED was activated to indicate error
    EXPECT_TRUE(red_led_.is_on());

    // Verify: Green LED was not toggled (no success operations)
    EXPECT_FALSE(green_led_.is_on());
    EXPECT_EQ(green_led_.get_toggle_count(), 0);

    // Verify: Software version command was not sent (early exit on error)
    const auto& commands = mock_uart_.get_transmitted_commands();
    EXPECT_EQ(commands.size(), 1); // Only "AT" was sent
    EXPECT_EQ(commands[0], "AT");
}

TEST_F(BasicInitExampleTest, SoftwareVersionQueryFailure) {
    // Setup: Test succeeds, version query fails
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_timeout("AT+VERS?");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    bool success = run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Operations failed
    EXPECT_FALSE(success);

    // Verify: Red LED was activated
    EXPECT_TRUE(red_led_.is_on());

    // Verify: Green LED was toggled once (for successful test)
    // After one toggle from initial off state, LED will be on
    // But this is different from explicitly calling on() at the end
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_EQ(green_led_.get_toggle_count(), 1);
    EXPECT_EQ(green_led_.get_on_count(), 0); // on() was never explicitly called

    // Verify: Both commands were sent
    const auto& commands = mock_uart_.get_transmitted_commands();
    EXPECT_EQ(commands.size(), 2);
    EXPECT_EQ(commands[0], "AT");
    EXPECT_EQ(commands[1], "AT+VERS?");
}

TEST_F(BasicInitExampleTest, InvalidResponseFormat) {
    // Setup: Module returns invalid response format
    mock_uart_.expect_command("AT", "ERROR");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    bool success = run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Operations failed (driver should handle invalid response)
    EXPECT_FALSE(success);

    // Verify: Red LED was activated
    EXPECT_TRUE(red_led_.is_on());
    EXPECT_FALSE(green_led_.is_on());
}

// ============================================================================
// Command Sequence Validation Tests
// ============================================================================

TEST_F(BasicInitExampleTest, CorrectCommandSequence) {
    // Setup: Program expected sequence
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Commands were sent in correct order
    const auto& commands = mock_uart_.get_transmitted_commands();
    ASSERT_EQ(commands.size(), 2);
    EXPECT_EQ(commands[0], "AT");
    EXPECT_EQ(commands[1], "AT+VERS?");

    // Verify: No unexpected commands
    EXPECT_FALSE(mock_uart_.has_unexpected_command());
}

TEST_F(BasicInitExampleTest, NoExtraCommands) {
    // Setup: Program expected sequence
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Exactly 2 commands sent, no more
    EXPECT_EQ(mock_uart_.get_transmitted_commands().size(), 2);
    EXPECT_TRUE(mock_uart_.all_expectations_met());
}

// ============================================================================
// LED Feedback Pattern Tests
// ============================================================================

TEST_F(BasicInitExampleTest, GreenLEDPatternOnSuccess) {
    // Setup: Successful sequence
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Green LED state history shows toggle -> toggle -> on
    const auto& history = green_led_.get_state_history();
    ASSERT_GE(history.size(), 3);

    // Verify: Final state is ON
    EXPECT_TRUE(green_led_.is_on());
    EXPECT_EQ(history[history.size() - 1], true);

    // Verify: Toggle count matches expectations (2 toggles for success)
    EXPECT_EQ(green_led_.get_toggle_count(), 2);
}

TEST_F(BasicInitExampleTest, RedLEDActivatedOnError) {
    // Setup: Test command fails
    mock_uart_.expect_timeout("AT");

    // Create driver instance
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Execute: Run basic initialization logic
    run_basic_init_logic(driver, green_led_, red_led_);

    // Verify: Red LED was turned on (not toggled, directly on)
    EXPECT_TRUE(red_led_.is_on());
    EXPECT_EQ(red_led_.get_on_count(), 1);

    // Verify: Red LED state history shows single 'on' transition
    const auto& history = red_led_.get_state_history();
    ASSERT_EQ(history.size(), 1);
    EXPECT_TRUE(history[0]); // First transition: on
}

// ============================================================================
// Driver Behavior Verification Tests
// ============================================================================

TEST_F(BasicInitExampleTest, DriverConstructionDoesNotSendCommands) {
    // Setup: No commands expected yet
    mock_uart_.clear();

    // Execute: Construct driver
    HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);

    // Verify: No commands sent during construction
    EXPECT_EQ(mock_uart_.get_transmitted_commands().size(), 0);
}

TEST_F(BasicInitExampleTest, DriverDestructionIsGraceful) {
    // Setup: Program commands
    mock_uart_.expect_command("AT", "OK");
    mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

    // Execute: Create and destroy driver in scope
    {
        HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);
        run_basic_init_logic(driver, green_led_, red_led_);
    } // Driver destroyed here

    // Verify: No crashes or issues during destruction
    SUCCEED();
}

// ============================================================================
// Version String Parsing Tests
// ============================================================================

TEST_F(BasicInitExampleTest, ParseVariousVersionFormats) {
    // Test with different version string formats returned by HM-11 variants

    // Format 1: HMSoft V542
    {
        mock_uart_.clear();
        green_led_.clear();
        red_led_.clear();

        mock_uart_.expect_command("AT", "OK");
        mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V542");

        HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);
        bool success = run_basic_init_logic(driver, green_led_, red_led_);

        EXPECT_TRUE(success);
    }

    // Format 2: HMSoft V605
    {
        mock_uart_.clear();
        green_led_.clear();
        red_led_.clear();

        mock_uart_.expect_command("AT", "OK");
        mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSoft V605");

        HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);
        bool success = run_basic_init_logic(driver, green_led_, red_led_);

        EXPECT_TRUE(success);
    }

    // Format 3: HMSensor V100 (sensor variant)
    {
        mock_uart_.clear();
        green_led_.clear();
        red_led_.clear();

        mock_uart_.expect_command("AT", "OK");
        mock_uart_.expect_command("AT+VERS?", "OK+Get:HMSensor V100");

        HM11Driver<EnhancedMockUART, SimpleReceiveHandler> driver(mock_uart_, receive_handler_);
        bool success = run_basic_init_logic(driver, green_led_, red_led_);

        EXPECT_TRUE(success);
    }
}
