/**
 * @file enhanced_mock_uart.hpp
 * @brief Enhanced mock UART for realistic HM-11 protocol simulation
 *
 * This header provides an advanced mock UART implementation that goes beyond
 * simple command/response mocking to simulate realistic HM-11 module behavior.
 * It enables comprehensive integration testing without physical hardware.
 *
 * ## Features
 *
 * - **Programmable Response Engine**: Queue expected commands with responses
 * - **HM-11 Protocol Simulation**: Realistic timing, connection sequences, streaming data
 * - **Behavioral Validation**: Record commands, detect unexpected sequences
 * - **State Management**: Track simulated module state (role, connected, config)
 * - **Timeout Simulation**: Selectively not respond to commands for timeout testing
 *
 * ## Testing Coverage
 *
 * This mock enables validation of:
 * - AT command formatting and sequencing
 * - Response parsing logic for all command types
 * - State machine behavior (connection lifecycle, scanning, GATT operations)
 * - Timeout handling and error conditions
 * - Streaming data parsing (device discovery, GATT characteristics)
 * - Driver API correctness and usage patterns
 *
 * This mock does NOT validate:
 * - Real HM-11 hardware timing and responses
 * - Bluetooth radio behavior and connectivity
 * - Physical hardware integration (UART, DMA, interrupts)
 * - RF characteristics, range, or interference handling
 *
 * See TESTING.md for detailed test coverage analysis and confidence methodology.
 *
 * ## Usage Example
 *
 * @code
 * #include "tests/util/enhanced_mock_uart.hpp"
 *
 * // Create mock with protocol simulation
 * EnhancedMockUART mock_uart;
 *
 * // Program expected command sequence
 * mock_uart.expect_command("AT", "OK");
 * mock_uart.expect_command("AT+ROLE1", "OK+Set:1");
 *
 * // Use with driver
 * DefaultReceiveHandler handler;
 * HM11Driver<EnhancedMockUART, DefaultReceiveHandler> driver(mock_uart, handler);
 *
 * UartStatus status;
 * driver.test(status);  // Sends "AT", receives "OK"
 * driver.set_role(Role::Central, status);  // Sends "AT+ROLE1", receives "OK+Set:1"
 *
 * // Verify all expected commands were sent
 * EXPECT_TRUE(mock_uart.all_expectations_met());
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/types/enums.hpp"

namespace hm11::test {

/**
 * @brief Response specification for programmed UART behavior
 *
 * Defines how the mock should respond to a specific AT command,
 * including timing delays and conditional state requirements.
 */
struct ResponseSpec {
    std::string expected_command;          ///< Command string to match (e.g., "AT+ROLE1")
    std::string response;                  ///< Response to return (e.g., "OK+Set:1")
    std::chrono::milliseconds delay{0};    ///< Delay before responding (simulates processing time)
    bool timeout{false};                   ///< If true, don't respond (simulate timeout)
    std::function<bool()> condition;       ///< Optional condition check (state validation)

    /// @brief Construct a simple response spec with command and response
    ResponseSpec(std::string_view cmd, std::string_view resp)
        : expected_command(cmd), response(resp) {}

    /// @brief Construct response spec with delay
    ResponseSpec(std::string_view cmd, std::string_view resp, std::chrono::milliseconds d)
        : expected_command(cmd), response(resp), delay(d) {}

    /// @brief Construct timeout response spec
    static ResponseSpec timeout_response(std::string_view cmd) {
        ResponseSpec spec(cmd, "");
        spec.timeout = true;
        return spec;
    }
};

/**
 * @brief Enhanced mock UART for HM-11 protocol simulation
 *
 * Provides sophisticated mock behavior for testing HM-11 driver logic
 * without physical hardware. Simulates realistic protocol patterns,
 * timing, state management, and error conditions.
 *
 * ## Design
 *
 * The mock operates in two modes:
 * 1. **Programmed Mode**: Responses queued via expect_command() are returned in order
 * 2. **Auto Mode**: Automatic responses based on simulated module state (future enhancement)
 *
 * ## Thread Safety
 *
 * This mock is designed for single-threaded testing. For ISR simulation tests,
 * use appropriate synchronization mechanisms.
 *
 * @note Satisfies IS_UART_PORT_V concept for use with HM11Driver
 */
class EnhancedMockUART {
  public:
    // ========================================================================
    // UART Port Interface (IS_UART_PORT_V)
    // ========================================================================

    /**
     * @brief Get UART data size (always 8-bit for HM-11)
     * @return UartDataSize::DataSize8b
     */
    [[nodiscard]] static hal::UartDataSize data_size() noexcept { return hal::UartDataSize::DataSize8b; }

    /**
     * @brief Transmit data over UART
     *
     * Captures transmitted command for validation and prepares
     * corresponding response based on programmed expectations.
     *
     * @param data Buffer containing data to transmit
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds (unused in mock)
     */
    void transmit(hal::UartData8bConst data, hal::UartStatus& status, uint32_t timeout_ms = 1000) noexcept;

    /**
     * @brief Receive data from UART
     *
     * Returns programmed response data for the last transmitted command.
     * Supports both blocking and streaming modes.
     *
     * @param data Buffer to store received data
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds
     * @param as_stream Enable streaming mode (for discovery, GATT operations)
     */
    void receive(hal::UartData8b data, hal::UartStatus& status, uint32_t timeout_ms = 1000, bool as_stream = false) noexcept;

    // ========================================================================
    // Response Programming Interface
    // ========================================================================

    /**
     * @brief Program a simple command/response expectation
     *
     * @param command Expected AT command string (e.g., "AT", "AT+ROLE1")
     * @param response Response to return (e.g., "OK", "OK+Set:1")
     */
    void expect_command(std::string_view command, std::string_view response);

    /**
     * @brief Program a command/response with delay
     *
     * @param command Expected AT command string
     * @param response Response to return
     * @param delay Processing delay before response
     */
    void expect_command(std::string_view command, std::string_view response, std::chrono::milliseconds delay);

    /**
     * @brief Program a timeout response (command with no response)
     *
     * @param command Expected AT command that will timeout
     */
    void expect_timeout(std::string_view command);

    /**
     * @brief Add a response spec to the queue
     *
     * @param spec Response specification to program
     */
    void add_response_spec(const ResponseSpec& spec);

    // ========================================================================
    // HM-11 Protocol Simulation Helpers
    // ========================================================================

    /**
     * @brief Simulate connection establishment sequence
     *
     * Programs responses for:
     * - AT+CON<address> -> "OK+CONN"
     * - Delay to simulate connection handshake
     */
    void simulate_connection_sequence(std::string_view mac_address);

    /**
     * @brief Simulate device discovery streaming data
     *
     * Programs streaming response with multiple discovered devices
     * in HM-11 format: "OK+DISC:<MAC>\r\n<Name>\r\n<RSSI>\r\n"
     *
     * @param devices Vector of device info (MAC, name, RSSI)
     */
    void simulate_discovery_scan(const std::vector<std::tuple<std::string, std::string, std::string>>& devices);

    /**
     * @brief Simulate GATT service discovery response
     *
     * Programs response for AT+DISC? command with service UUIDs
     *
     * @param service_uuids Vector of 16-bit service UUID strings (4 hex chars)
     */
    void simulate_gatt_services(const std::vector<std::string>& service_uuids);

    /**
     * @brief Simulate GATT characteristic discovery response
     *
     * Programs response for characteristic query with UUIDs and handles
     *
     * @param char_uuids Vector of characteristic UUID strings
     */
    void simulate_gatt_characteristics(const std::vector<std::string>& char_uuids);

    /**
     * @brief Simulate sensor reading response
     *
     * Programs response for HMSensor temperature/humidity query
     *
     * @param temperature Temperature value (0-120)
     * @param humidity Humidity value (0-100)
     */
    void simulate_sensor_reading(uint8_t temperature, uint8_t humidity);

    // ========================================================================
    // Behavioral Validation Interface
    // ========================================================================

    /**
     * @brief Get all transmitted commands for verification
     * @return Vector of command strings sent via transmit()
     */
    [[nodiscard]] const std::vector<std::string>& get_transmitted_commands() const noexcept { return transmitted_commands_; }

    /**
     * @brief Get the last transmitted command
     * @return Last command string, or empty if none transmitted
     */
    [[nodiscard]] std::string_view get_last_command() const noexcept;

    /**
     * @brief Check if all programmed expectations were met
     * @return true if all expected commands were sent and responses consumed
     */
    [[nodiscard]] bool all_expectations_met() const noexcept { return response_queue_.empty(); }

    /**
     * @brief Get number of remaining expectations
     * @return Count of programmed responses not yet consumed
     */
    [[nodiscard]] std::size_t remaining_expectations() const noexcept { return response_queue_.size(); }

    /**
     * @brief Check if an unexpected command was received
     * @return true if a command was sent that didn't match expectations
     */
    [[nodiscard]] bool has_unexpected_command() const noexcept { return has_unexpected_command_; }

    /**
     * @brief Get the unexpected command that was received
     * @return Command string that didn't match expectations
     */
    [[nodiscard]] std::string_view get_unexpected_command() const noexcept { return unexpected_command_; }

    // ========================================================================
    // State Management
    // ========================================================================

    /**
     * @brief Set simulated module role
     * @param role Role to simulate (Peripheral or Central)
     */
    void set_simulated_role(types::Role role) noexcept { simulated_role_ = role; }

    /**
     * @brief Get simulated module role
     * @return Current simulated role
     */
    [[nodiscard]] types::Role get_simulated_role() const noexcept { return simulated_role_; }

    /**
     * @brief Set simulated connection state
     * @param connected true if simulating connected state
     */
    void set_simulated_connected(bool connected) noexcept { simulated_connected_ = connected; }

    /**
     * @brief Get simulated connection state
     * @return true if simulating connected state
     */
    [[nodiscard]] bool is_simulated_connected() const noexcept { return simulated_connected_; }

    // ========================================================================
    // Mock Control
    // ========================================================================

    /**
     * @brief Clear all mock state and expectations
     *
     * Resets:
     * - Response queue
     * - Transmitted command history
     * - Unexpected command flags
     * - Simulated state (role, connection)
     */
    void clear() noexcept;

    /**
     * @brief Reset transmitted command history only
     *
     * Keeps response queue and simulated state intact.
     * Useful for multi-step test scenarios.
     */
    void clear_transmitted_commands() noexcept;

  private:
    // Response programming
    std::deque<ResponseSpec> response_queue_;
    std::optional<ResponseSpec> pending_response_;

    // Command recording
    std::vector<std::string> transmitted_commands_;
    std::string last_command_;

    // Validation tracking
    bool has_unexpected_command_{false};
    std::string unexpected_command_;

    // Simulated module state
    types::Role simulated_role_{types::Role::Peripheral};
    bool simulated_connected_{false};

    // Helper methods
    std::optional<ResponseSpec> find_matching_response(std::string_view command);
    void record_unexpected_command(std::string_view command);
};

// Verify EnhancedMockUART satisfies UART port concept
static_assert(hal::IS_UART_PORT_V<EnhancedMockUART>, "EnhancedMockUART must satisfy UART port interface");

} // namespace hm11::test
