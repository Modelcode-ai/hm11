/**
 * @file test_enhanced_mock_uart.cpp
 * @brief Unit tests for EnhancedMockUART infrastructure
 *
 * Tests cover:
 * - Basic UART interface satisfaction
 * - Response queueing and matching
 * - Command recording and validation
 * - Timeout simulation
 * - State management
 * - HM-11 protocol simulation helpers
 * - Response builder utilities
 */

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <string>
#include <vector>

#include "enhanced_mock_uart.hpp"
#include "hm11/hal/base_types.hpp"
#include "hm11/types/enums.hpp"
#include "hm11_response_builder.hpp"
#include "mock_led.hpp"
#include "mock_timer.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;
using namespace hm11::test;

// ============================================================================
// Basic Interface Tests
// ============================================================================

TEST(EnhancedMockUARTTest, SatisfiesUARTPortInterface) {
    // Verify mock satisfies UARTPort concept
    static_assert(IS_UART_PORT_V<EnhancedMockUART>, "EnhancedMockUART must satisfy UART port interface");

    EnhancedMockUART mock;
    EXPECT_EQ(mock.data_size(), UartDataSize::DataSize8b);
}

TEST(EnhancedMockUARTTest, BasicTransmitReceive) {
    EnhancedMockUART mock;
    UartStatus status;

    // Program expected command/response
    mock.expect_command("AT", "OK");

    // Transmit command
    std::array<UInt8, 2> tx_data = {'A', 'T'};
    mock.transmit(tx_data, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Receive response
    std::array<UInt8, 2> rx_data{};
    mock.receive(rx_data, status);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'O');
    EXPECT_EQ(rx_data[1], 'K');
}

// ============================================================================
// Response Queueing Tests
// ============================================================================

TEST(EnhancedMockUARTTest, MultipleCommandsQueued) {
    EnhancedMockUART mock;
    UartStatus status;

    // Program sequence of commands
    mock.expect_command("AT", "OK");
    mock.expect_command("AT+ROLE1", "OK+Set:1");
    mock.expect_command("AT+RESET", "OK+RESET");

    // Execute sequence
    std::array<UInt8, 2> cmd1 = {'A', 'T'};
    mock.transmit(cmd1, status);
    EXPECT_EQ(status, UartStatus::Ok);

    std::array<UInt8, 2> resp1{};
    mock.receive(resp1, status);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(resp1[0], 'O');
    EXPECT_EQ(resp1[1], 'K');

    // Second command
    std::array<UInt8, 8> cmd2 = {'A', 'T', '+', 'R', 'O', 'L', 'E', '1'};
    mock.transmit(cmd2, status);
    std::array<UInt8, 10> resp2{};
    mock.receive(resp2, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Third command
    std::array<UInt8, 8> cmd3 = {'A', 'T', '+', 'R', 'E', 'S', 'E', 'T'};
    mock.transmit(cmd3, status);
    std::array<UInt8, 9> resp3{};
    mock.receive(resp3, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // All expectations met
    EXPECT_TRUE(mock.all_expectations_met());
    EXPECT_EQ(mock.remaining_expectations(), 0);
}

TEST(EnhancedMockUARTTest, OrderedResponseMatching) {
    EnhancedMockUART mock;
    UartStatus status;

    // Program responses in order
    mock.expect_command("CMD1", "RESP1");
    mock.expect_command("CMD2", "RESP2");

    // Send CMD2 first - should not match (FIFO order)
    std::array<UInt8, 4> cmd2 = {'C', 'M', 'D', '2'};
    mock.transmit(cmd2, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Should be marked as unexpected
    EXPECT_TRUE(mock.has_unexpected_command());
    EXPECT_EQ(mock.get_unexpected_command(), "CMD2");
}

// ============================================================================
// Command Recording Tests
// ============================================================================

TEST(EnhancedMockUARTTest, CommandRecording) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.expect_command("AT", "OK");
    mock.expect_command("AT+ROLE?", "OK+Get:0");

    std::array<UInt8, 2> cmd1 = {'A', 'T'};
    mock.transmit(cmd1, status);

    std::array<UInt8, 8> cmd2 = {'A', 'T', '+', 'R', 'O', 'L', 'E', '?'};
    mock.transmit(cmd2, status);

    // Verify recording
    const auto& commands = mock.get_transmitted_commands();
    EXPECT_EQ(commands.size(), 2);
    EXPECT_EQ(commands[0], "AT");
    EXPECT_EQ(commands[1], "AT+ROLE?");
    EXPECT_EQ(mock.get_last_command(), "AT+ROLE?");
}

TEST(EnhancedMockUARTTest, UnexpectedCommandDetection) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.expect_command("EXPECTED", "RESPONSE");

    std::array<UInt8, 10> unexpected = {'U', 'N', 'E', 'X', 'P', 'E', 'C', 'T', 'E', 'D'};
    mock.transmit(unexpected, status);

    EXPECT_TRUE(mock.has_unexpected_command());
    EXPECT_EQ(mock.get_unexpected_command(), "UNEXPECTED");
}

// ============================================================================
// Timeout Simulation Tests
// ============================================================================

TEST(EnhancedMockUARTTest, TimeoutResponse) {
    EnhancedMockUART mock;
    UartStatus status;

    // Program timeout response
    mock.expect_timeout("AT+SCAN");

    std::array<UInt8, 7> cmd = {'A', 'T', '+', 'S', 'C', 'A', 'N'};
    mock.transmit(cmd, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Receive should timeout
    std::array<UInt8, 10> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

TEST(EnhancedMockUARTTest, NoResponseAvailableTimeout) {
    EnhancedMockUART mock;
    UartStatus status;

    // Don't program any response

    std::array<UInt8, 2> cmd = {'A', 'T'};
    mock.transmit(cmd, status);

    // Receive should timeout (no pending response)
    std::array<UInt8, 2> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

// ============================================================================
// State Management Tests
// ============================================================================

TEST(EnhancedMockUARTTest, SimulatedRoleState) {
    EnhancedMockUART mock;

    // Default role
    EXPECT_EQ(mock.get_simulated_role(), Role::Peripheral);

    // Set role
    mock.set_simulated_role(Role::Central);
    EXPECT_EQ(mock.get_simulated_role(), Role::Central);
}

TEST(EnhancedMockUARTTest, SimulatedConnectionState) {
    EnhancedMockUART mock;

    // Default disconnected
    EXPECT_FALSE(mock.is_simulated_connected());

    // Set connected
    mock.set_simulated_connected(true);
    EXPECT_TRUE(mock.is_simulated_connected());
}

// ============================================================================
// HM-11 Protocol Simulation Tests
// ============================================================================

TEST(EnhancedMockUARTTest, ConnectionSequenceSimulation) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.simulate_connection_sequence("001122334455");

    // Should have programmed connection command
    std::array<UInt8, 18> cmd =
        {'A', 'T', '+', 'C', 'O', 'N', '0', '0', '1', '1', '2', '2', '3', '3', '4', '4', '5', '5'};
    mock.transmit(cmd, status);
    EXPECT_EQ(status, UartStatus::Ok);

    std::array<UInt8, 7> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Should update connected state
    EXPECT_TRUE(mock.is_simulated_connected());
}

TEST(EnhancedMockUARTTest, DiscoveryScanSimulation) {
    EnhancedMockUART mock;
    UartStatus status;

    // Simulate discovery with multiple devices
    std::vector<std::tuple<std::string, std::string, std::string>> devices =
        {{"001122334455", "Device1", "-65"}, {"AABBCCDDEEFF", "Device2", "-72"}, {"112233445566", "Device3", "-80"}};

    mock.simulate_discovery_scan(devices);

    // Send discovery command
    std::array<UInt8, 8> cmd = {'A', 'T', '+', 'D', 'I', 'S', 'C', '?'};
    mock.transmit(cmd, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Receive should contain formatted discovery data
    std::array<UInt8, 256> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Verify response contains device data (basic check)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string resp_str(reinterpret_cast<const char*>(resp.data()), resp.size());
    EXPECT_NE(resp_str.find("OK+DISCS"), std::string::npos);
    EXPECT_NE(resp_str.find("OK+DISCE"), std::string::npos);
    EXPECT_NE(resp_str.find("Device1"), std::string::npos);
}

TEST(EnhancedMockUARTTest, GATTServicesSimulation) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.simulate_gatt_services({"1800", "1801", "180A"});

    std::array<UInt8, 8> cmd = {'A', 'T', '+', 'D', 'I', 'S', 'C', '?'};
    mock.transmit(cmd, status);

    std::array<UInt8, 32> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string resp_str(reinterpret_cast<const char*>(resp.data()), resp.size());
    EXPECT_NE(resp_str.find("OK+DISC:"), std::string::npos);
    EXPECT_NE(resp_str.find("1800"), std::string::npos);
}

TEST(EnhancedMockUARTTest, SensorReadingSimulation) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.simulate_sensor_reading(25, 60);

    std::array<UInt8, 8> cmd = {'A', 'T', '+', 'T', 'E', 'M', 'P', '?'};
    mock.transmit(cmd, status);

    std::array<UInt8, 32> resp{};
    mock.receive(resp, status);
    EXPECT_EQ(status, UartStatus::Ok);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string resp_str(reinterpret_cast<const char*>(resp.data()), resp.size());
    EXPECT_NE(resp_str.find("OK+Get:"), std::string::npos);
    EXPECT_NE(resp_str.find("25"), std::string::npos);
    EXPECT_NE(resp_str.find("60"), std::string::npos);
}

// ============================================================================
// Mock Control Tests
// ============================================================================

TEST(EnhancedMockUARTTest, ClearResetsMock) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.expect_command("CMD1", "RESP1");
    mock.set_simulated_role(Role::Central);
    mock.set_simulated_connected(true);

    std::array<UInt8, 4> cmd = {'C', 'M', 'D', '1'};
    mock.transmit(cmd, status);

    // Clear everything
    mock.clear();

    EXPECT_TRUE(mock.all_expectations_met());
    EXPECT_EQ(mock.get_transmitted_commands().size(), 0);
    EXPECT_FALSE(mock.has_unexpected_command());
    EXPECT_EQ(mock.get_simulated_role(), Role::Peripheral);
    EXPECT_FALSE(mock.is_simulated_connected());
}

TEST(EnhancedMockUARTTest, ClearTransmittedCommandsOnly) {
    EnhancedMockUART mock;
    UartStatus status;

    mock.expect_command("CMD1", "RESP1");
    mock.set_simulated_role(Role::Central);

    std::array<UInt8, 4> cmd = {'C', 'M', 'D', '1'};
    mock.transmit(cmd, status);

    // Clear only transmitted commands
    mock.clear_transmitted_commands();

    // Expectations consumed but transmitted commands cleared
    // Note: transmit() consumes the expectation, so the queue is empty
    EXPECT_TRUE(mock.all_expectations_met()); // CMD1 was consumed during transmit
    EXPECT_EQ(mock.get_simulated_role(), Role::Central);
    EXPECT_EQ(mock.get_transmitted_commands().size(), 0);
}

// ============================================================================
// Response Builder Tests
// ============================================================================

TEST(HM11ResponseBuilderTest, SimpleOkResponse) {
    auto response = HM11ResponseBuilder::ok();
    EXPECT_EQ(response, "OK");
}

TEST(HM11ResponseBuilderTest, OkSetResponse) {
    auto response = HM11ResponseBuilder::ok_set("1");
    EXPECT_EQ(response, "OK+Set:1");
}

TEST(HM11ResponseBuilderTest, OkGetResponse) {
    auto response = HM11ResponseBuilder::ok_get("V540");
    EXPECT_EQ(response, "OK+Get:V540");
}

TEST(HM11ResponseBuilderTest, ConnectionResponses) {
    EXPECT_EQ(HM11ResponseBuilder::connection_success(), "OK+CONN");
    EXPECT_EQ(HM11ResponseBuilder::connection_lost(), "OK+LOST");
    EXPECT_EQ(HM11ResponseBuilder::error(), "OK+ERR");
}

TEST(HM11ResponseBuilderTest, DiscoveryBuilder) {
    auto response = HM11ResponseBuilder::discovery_start()
                        .add_device("001122334455", "TestDevice", "-65")
                        .add_device("AABBCCDDEEFF", "AnotherDevice", "-72")
                        .discovery_end()
                        .build();

    EXPECT_NE(response.find("OK+DISCS"), std::string::npos);
    EXPECT_NE(response.find("OK+DISC:001122334455"), std::string::npos);
    EXPECT_NE(response.find("TestDevice"), std::string::npos);
    EXPECT_NE(response.find("-65"), std::string::npos);
    EXPECT_NE(response.find("OK+DISCE"), std::string::npos);
}

TEST(HM11ResponseBuilderTest, GATTServicesResponse) {
    auto response = HM11ResponseBuilder::gatt_services({"1800", "1801", "180A"});
    EXPECT_EQ(response, "OK+DISC:1800,1801,180A");
}

TEST(HM11ResponseBuilderTest, GATTCharacteristicsResponse) {
    auto response = HM11ResponseBuilder::gatt_characteristics({"2A00", "2A01"});
    EXPECT_EQ(response, "OK+CHAR:2A00,2A01");
}

TEST(HM11ResponseBuilderTest, SensorReadingResponse) {
    auto response = HM11ResponseBuilder::sensor_reading(25, 60);
    EXPECT_EQ(response, "OK+Get:25C 60%");
}

TEST(HM11ResponseBuilderTest, VersionResponse) {
    auto response = HM11ResponseBuilder::version("V540");
    EXPECT_EQ(response, "OK+Get:V540");
}

TEST(HM11ResponseBuilderTest, RoleResponse) {
    auto response = HM11ResponseBuilder::role(1);
    EXPECT_EQ(response, "OK+Get:1");
}

TEST(HM11ResponseBuilderTest, NameResponse) {
    auto response = HM11ResponseBuilder::name("HMSoft");
    EXPECT_EQ(response, "OK+NAME:HMSoft");
}

TEST(HM11ResponseBuilderTest, MacAddressResponse) {
    auto response = HM11ResponseBuilder::mac_address("001122334455");
    EXPECT_EQ(response, "OK+ADDR:001122334455");
}

TEST(HM11ResponseBuilderTest, RSSIResponse) {
    auto response = HM11ResponseBuilder::rssi(-65);
    EXPECT_EQ(response, "OK+RSSI:-65dBm");
}

// ============================================================================
// MockLED Tests
// ============================================================================

TEST(MockLEDTest, BasicOnOff) {
    MockLED led;

    EXPECT_FALSE(led.is_on());

    led.on();
    EXPECT_TRUE(led.is_on());

    led.off();
    EXPECT_FALSE(led.is_on());
}

TEST(MockLEDTest, Toggle) {
    MockLED led;

    led.toggle();
    EXPECT_TRUE(led.is_on());

    led.toggle();
    EXPECT_FALSE(led.is_on());
}

TEST(MockLEDTest, StateHistory) {
    MockLED led;

    led.on();
    led.off();
    led.toggle();

    const auto& history = led.get_state_history();
    EXPECT_EQ(history.size(), 3);
    EXPECT_TRUE(history[0]);
    EXPECT_FALSE(history[1]);
    EXPECT_TRUE(history[2]);
}

TEST(MockLEDTest, Counters) {
    MockLED led;

    led.on();
    led.off();
    led.on();
    led.toggle();
    led.toggle();

    EXPECT_EQ(led.get_on_count(), 2);
    EXPECT_EQ(led.get_off_count(), 1);
    EXPECT_EQ(led.get_toggle_count(), 2);
}

TEST(MockLEDTest, Clear) {
    MockLED led;

    led.on();
    led.toggle();

    led.clear();

    EXPECT_FALSE(led.is_on());
    EXPECT_EQ(led.get_state_history().size(), 0);
    EXPECT_EQ(led.get_on_count(), 0);
    EXPECT_EQ(led.get_toggle_count(), 0);
}

// ============================================================================
// MockTimer Tests
// ============================================================================

TEST(MockTimerTest, StartStop) {
    MockTimer timer;

    EXPECT_FALSE(timer.is_running());

    timer.start();
    EXPECT_TRUE(timer.is_running());
    EXPECT_EQ(timer.elapsed_ms(), 0);

    timer.stop();
    EXPECT_FALSE(timer.is_running());
}

TEST(MockTimerTest, TimeAdvancement) {
    MockTimer timer;

    timer.start();
    timer.advance(std::chrono::milliseconds(100));
    EXPECT_EQ(timer.elapsed_ms(), 100);

    timer.advance_ms(50);
    EXPECT_EQ(timer.elapsed_ms(), 150);
}

TEST(MockTimerTest, NoAdvancementWhenStopped) {
    MockTimer timer;

    timer.start();
    timer.advance_ms(100);
    timer.stop();
    timer.advance_ms(50);

    EXPECT_EQ(timer.elapsed_ms(), 100); // Should not advance after stop
}

TEST(MockTimerTest, HasElapsedCheck) {
    MockTimer timer;

    timer.start();
    EXPECT_FALSE(timer.has_elapsed(std::chrono::milliseconds(100)));

    timer.advance_ms(50);
    EXPECT_FALSE(timer.has_elapsed(std::chrono::milliseconds(100)));

    timer.advance_ms(50);
    EXPECT_TRUE(timer.has_elapsed(std::chrono::milliseconds(100)));
}

TEST(MockTimerTest, RemainingTime) {
    MockTimer timer;

    timer.start();
    timer.advance_ms(300);

    auto remaining = timer.remaining(std::chrono::milliseconds(1000));
    EXPECT_EQ(remaining.count(), 700);

    timer.advance_ms(800);
    remaining = timer.remaining(std::chrono::milliseconds(1000));
    EXPECT_EQ(remaining.count(), 0);
}

TEST(MockTimerTest, Reset) {
    MockTimer timer;

    timer.start();
    timer.advance_ms(100);

    timer.reset();

    EXPECT_FALSE(timer.is_running());
    EXPECT_EQ(timer.elapsed_ms(), 0);
}

TEST(MockTimerTest, SetElapsed) {
    MockTimer timer;

    timer.set_elapsed(std::chrono::milliseconds(500));
    EXPECT_EQ(timer.elapsed_ms(), 500);
}

TEST(ScopedMockTimerTest, AutoStartStop) {
    {
        ScopedMockTimer timer;
        EXPECT_TRUE(timer.is_running());
        timer.advance_ms(100);
        EXPECT_EQ(timer.elapsed_ms(), 100);
    } // Timer stopped automatically
}

// ============================================================================
// Main
// ============================================================================

// Note: main() is defined in test_main.cpp
