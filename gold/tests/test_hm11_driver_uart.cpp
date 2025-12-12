/**
 * @file test_hm11_driver_uart.cpp
 * @brief Unit tests for HM11Driver UART configuration operations
 *
 * Tests cover:
 * - Set_UART_Baud_Rate for all 9 baud rate values
 * - Get_UART_Baud_Rate parsing for all valid responses
 * - Set_Flow_Control_Switch enable/disable
 * - Get_Flow_Control_Switch boolean parsing
 * - Set_Parity_Bit for None, Odd, Even
 * - Get_Parity_Bit parsing
 * - Set_Stop_Bit for One and Two stop bits
 * - Get_Stop_Bit parsing
 * - Enum-to-string conversions for all UART enums
 * - Status error propagation
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/util/enum_mapping.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// ============================================================================
// Mock UART Port for Testing UART Configuration Operations
// ============================================================================

class UartTestMockUARTPort {
  public:
    UartTestMockUARTPort() = default;

    // Configure mock to return specific response
    void set_response(std::string_view response) {
        response_data.clear();
        response_data.insert(response_data.end(), response.begin(), response.end());
        next_transmit_fails = false;
        next_receive_fails = false;
    }

    void set_transmit_fails() { next_transmit_fails = true; }

    void set_receive_fails() { next_receive_fails = true; }

    std::string get_last_transmitted() const {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        return {reinterpret_cast<const char*>(last_transmitted.data()), last_transmitted.size()};
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    // UART interface methods matching IS_UART_PORT_V concept
    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        if (next_transmit_fails) {
            status = UartStatus::ErrError;
            return;
        }
        last_transmitted.clear();
        last_transmitted.insert(last_transmitted.end(), data.begin(), data.end());
        status = UartStatus::Ok;
    }

    void receive(
        UartData8b buffer,
        UartStatus& status,
        uint32_t /*timeout_ms*/ = 1000,
        bool /*as_stream*/ = false) noexcept {
        if (next_receive_fails) {
            status = UartStatus::ErrTimeout;
            return;
        }

        const std::size_t COPY_SIZE = std::min(buffer.size(), response_data.size());
        std::copy_n(response_data.begin(), COPY_SIZE, buffer.begin());
        status = UartStatus::Ok;
    }

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

  private:
    std::vector<uint8_t> response_data;
    std::vector<uint8_t> last_transmitted;
    bool next_transmit_fails{false};
    bool next_receive_fails{false};
};

// ============================================================================
// Mock Receive Handler for Testing
// ============================================================================

class MockReceiveHandler {
  public:
    // Overload WITH explicit timeout parameter
    void operator()(
        UartTestMockUARTPort& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        // Forward to port's receive method
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload WITHOUT timeout parameter (uses default)
    void operator()(UartTestMockUARTPort& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        // Forward to port's receive method with default timeout
        port.receive(buffer, status, 1000, as_stream);
    }
};

// Concept satisfaction checks
static_assert(hal::IS_UART_PORT_V<UartTestMockUARTPort>, "UartTestMockUARTPort must satisfy IS_UART_PORT_V");
static_assert(
    hal::IS_RECEIVE_HANDLER_V<MockReceiveHandler, UartTestMockUARTPort>,
    "MockReceiveHandler must satisfy IS_RECEIVE_HANDLER_V");

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverUartTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // GoogleTest convention: test fixture members should be protected
    UartTestMockUARTPort uart_port;
    MockReceiveHandler receive_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

// ============================================================================
// Enum Mapping Tests
// ============================================================================

TEST(EnumMappingTest, UartBaudRate_AllValues) {
    // Test all 9 baud rate values map correctly
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br9600), "0");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br19200), "1");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br38400), "2");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br57600), "3");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br115200), "4");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br4800), "5");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br2400), "6");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br1200), "7");
    EXPECT_EQ(util::enum_to_string(UartBaudRate::Br230400), "8");
}

TEST(EnumMappingTest, ParityBit_AllValues) {
    EXPECT_EQ(util::enum_to_string(ParityBit::None), "0");
    EXPECT_EQ(util::enum_to_string(ParityBit::Odd), "1");
    EXPECT_EQ(util::enum_to_string(ParityBit::Even), "2");
}

TEST(EnumMappingTest, StopBit_AllValues) {
    EXPECT_EQ(util::enum_to_string(StopBit::OneStopBit), "0");
    EXPECT_EQ(util::enum_to_string(StopBit::TwoStopBit), "1");
}

// ============================================================================
// Set_UART_Baud_Rate Tests
// ============================================================================

TEST_F(HM11DriverUartTest, SetUartBaudRate_Br9600) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::ErrError;
    driver.set_uart_baud_rate(UartBaudRate::Br9600, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BAUD0");
}

TEST_F(HM11DriverUartTest, SetUartBaudRate_Br115200) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:4");

    UartStatus status = UartStatus::ErrError;
    driver.set_uart_baud_rate(UartBaudRate::Br115200, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BAUD4");
}

TEST_F(HM11DriverUartTest, SetUartBaudRate_Br1200_Warning) {
    // Test BR_1200 (7) - this value disables AT commands after power cycle
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:7");

    UartStatus status = UartStatus::ErrError;
    driver.set_uart_baud_rate(UartBaudRate::Br1200, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BAUD7");
}

TEST_F(HM11DriverUartTest, SetUartBaudRate_Br230400) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:8");

    UartStatus status = UartStatus::ErrError;
    driver.set_uart_baud_rate(UartBaudRate::Br230400, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BAUD8");
}

TEST_F(HM11DriverUartTest, SetUartBaudRate_TransmitError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_transmit_fails();

    UartStatus status = UartStatus::Ok;
    driver.set_uart_baud_rate(UartBaudRate::Br9600, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Get_UART_Baud_Rate Tests
// ============================================================================

TEST_F(HM11DriverUartTest, GetUartBaudRate_Br9600) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    UartStatus status = UartStatus::ErrError;
    UartBaudRate result = UartBaudRate::Br115200; // Initialize to different value
    driver.get_uart_baud_rate(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, UartBaudRate::Br9600);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+BAUD?");
}

TEST_F(HM11DriverUartTest, GetUartBaudRate_AllValues) {
    HM11Driver driver(uart_port, receive_handler);
    UartStatus status;
    UartBaudRate result;

    // Test all 9 baud rate values
    const std::array<std::pair<std::string_view, UartBaudRate>, 9> test_cases = {{
        {"OK+Get:0", UartBaudRate::Br9600},
        {"OK+Get:1", UartBaudRate::Br19200},
        {"OK+Get:2", UartBaudRate::Br38400},
        {"OK+Get:3", UartBaudRate::Br57600},
        {"OK+Get:4", UartBaudRate::Br115200},
        {"OK+Get:5", UartBaudRate::Br4800},
        {"OK+Get:6", UartBaudRate::Br2400},
        {"OK+Get:7", UartBaudRate::Br1200},
        {"OK+Get:8", UartBaudRate::Br230400},
    }};

    for (const auto& [response, expected_baud] : test_cases) {
        uart_port.set_response(response);
        driver.get_uart_baud_rate(result, status);
        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(result, expected_baud);
    }
}

TEST_F(HM11DriverUartTest, GetUartBaudRate_InvalidResponse) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:9"); // Invalid: only 0-8 are valid

    UartStatus status = UartStatus::Ok;
    UartBaudRate result = UartBaudRate::Br9600;
    driver.get_uart_baud_rate(result, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverUartTest, GetUartBaudRate_ReceiveError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_receive_fails();

    UartStatus status = UartStatus::Ok;
    UartBaudRate result = UartBaudRate::Br9600;
    driver.get_uart_baud_rate(result, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

// ============================================================================
// Set_Flow_Control_Switch Tests
// ============================================================================

TEST_F(HM11DriverUartTest, SetFlowControlSwitch_Enable) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::ErrError;
    driver.set_flow_control_switch(true, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+FIOW1");
}

TEST_F(HM11DriverUartTest, SetFlowControlSwitch_Disable) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::ErrError;
    driver.set_flow_control_switch(false, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+FIOW0");
}

TEST_F(HM11DriverUartTest, SetFlowControlSwitch_TransmitError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_transmit_fails();

    UartStatus status = UartStatus::Ok;
    driver.set_flow_control_switch(true, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Get_Flow_Control_Switch Tests
// ============================================================================

TEST_F(HM11DriverUartTest, GetFlowControlSwitch_Enabled) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:1");

    UartStatus status = UartStatus::ErrError;
    bool enabled = false; // Initialize to opposite value
    driver.get_flow_control_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(enabled);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+FIOW?");
}

TEST_F(HM11DriverUartTest, GetFlowControlSwitch_Disabled) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    UartStatus status = UartStatus::ErrError;
    bool enabled = true; // Initialize to opposite value
    driver.get_flow_control_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_FALSE(enabled);
}

TEST_F(HM11DriverUartTest, GetFlowControlSwitch_InvalidResponse) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:2"); // Invalid: only 0 or 1 valid

    UartStatus status = UartStatus::Ok;
    bool enabled = false;
    driver.get_flow_control_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverUartTest, GetFlowControlSwitch_ReceiveError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_receive_fails();

    UartStatus status = UartStatus::Ok;
    bool enabled = false;
    driver.get_flow_control_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

// ============================================================================
// Set_Parity_Bit Tests
// ============================================================================

TEST_F(HM11DriverUartTest, SetParityBit_None) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::ErrError;
    driver.set_parity_bit(ParityBit::None, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PARI0");
}

TEST_F(HM11DriverUartTest, SetParityBit_Odd) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::ErrError;
    driver.set_parity_bit(ParityBit::Odd, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PARI1");
}

TEST_F(HM11DriverUartTest, SetParityBit_Even) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:2");

    UartStatus status = UartStatus::ErrError;
    driver.set_parity_bit(ParityBit::Even, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PARI2");
}

TEST_F(HM11DriverUartTest, SetParityBit_TransmitError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_transmit_fails();

    UartStatus status = UartStatus::Ok;
    driver.set_parity_bit(ParityBit::None, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Get_Parity_Bit Tests
// ============================================================================

TEST_F(HM11DriverUartTest, GetParityBit_None) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    UartStatus status = UartStatus::ErrError;
    ParityBit parity = ParityBit::Even; // Initialize to different value
    driver.get_parity_bit(parity, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(parity, ParityBit::None);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PARI?");
}

TEST_F(HM11DriverUartTest, GetParityBit_AllValues) {
    HM11Driver driver(uart_port, receive_handler);
    UartStatus status;
    ParityBit parity;

    // Test all 3 parity bit values
    const std::array<std::pair<std::string_view, ParityBit>, 3> test_cases = {{
        {"OK+Get:0", ParityBit::None},
        {"OK+Get:1", ParityBit::Odd},
        {"OK+Get:2", ParityBit::Even},
    }};

    for (const auto& [response, expected_parity] : test_cases) {
        uart_port.set_response(response);
        driver.get_parity_bit(parity, status);
        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(parity, expected_parity);
    }
}

TEST_F(HM11DriverUartTest, GetParityBit_InvalidResponse) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:3"); // Invalid: only 0-2 are valid

    UartStatus status = UartStatus::Ok;
    ParityBit parity = ParityBit::None;
    driver.get_parity_bit(parity, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverUartTest, GetParityBit_ReceiveError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_receive_fails();

    UartStatus status = UartStatus::Ok;
    ParityBit parity = ParityBit::None;
    driver.get_parity_bit(parity, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

// ============================================================================
// Set_Stop_Bit Tests
// ============================================================================

TEST_F(HM11DriverUartTest, SetStopBit_One) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::ErrError;
    driver.set_stop_bit(StopBit::OneStopBit, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+STOP0");
}

TEST_F(HM11DriverUartTest, SetStopBit_Two) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::ErrError;
    driver.set_stop_bit(StopBit::TwoStopBit, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+STOP1");
}

TEST_F(HM11DriverUartTest, SetStopBit_TransmitError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_transmit_fails();

    UartStatus status = UartStatus::Ok;
    driver.set_stop_bit(StopBit::OneStopBit, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Get_Stop_Bit Tests
// ============================================================================

TEST_F(HM11DriverUartTest, GetStopBit_One) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    UartStatus status = UartStatus::ErrError;
    StopBit stop_bit = StopBit::TwoStopBit; // Initialize to different value
    driver.get_stop_bit(stop_bit, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(stop_bit, StopBit::OneStopBit);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+STOP?");
}

TEST_F(HM11DriverUartTest, GetStopBit_Two) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:1");

    UartStatus status = UartStatus::ErrError;
    StopBit stop_bit = StopBit::OneStopBit; // Initialize to different value
    driver.get_stop_bit(stop_bit, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(stop_bit, StopBit::TwoStopBit);
}

TEST_F(HM11DriverUartTest, GetStopBit_InvalidResponse) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:2"); // Invalid: only 0-1 are valid

    UartStatus status = UartStatus::Ok;
    StopBit stop_bit = StopBit::OneStopBit;
    driver.get_stop_bit(stop_bit, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverUartTest, GetStopBit_ReceiveError) {
    HM11Driver driver(uart_port, receive_handler);
    uart_port.set_receive_fails();

    UartStatus status = UartStatus::Ok;
    StopBit stop_bit = StopBit::OneStopBit;
    driver.get_stop_bit(stop_bit, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}
