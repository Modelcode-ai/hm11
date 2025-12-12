/**
 * @file test_hm11_driver_security.cpp
 * @brief Unit tests for HM11Driver security and authentication operations
 *
 * Tests cover:
 * - Set_Bond_Mode for all 4 bond mode values
 * - Get_Bond_Mode parsing
 * - Set_PIN_Code with 6-digit PIN validation
 * - Get_PIN_Code parsing
 * - Set_Work_Type for both work type values
 * - Get_Work_Type parsing (critical for Central scanning)
 * - Set_White_List_Switch enable/disable
 * - Get_White_List_Switch parsing
 * - Set_White_List_MAC_Address for indices 1-3
 * - Get_White_List_MAC_Address parsing
 * - MacWhiteListIndex RangedInteger validation
 * - Remove_Bond_Information command
 * - Set_Service_UUID with 4-hex UUID
 * - Get_Service_UUID parsing with "0x" prefix
 * - Set_UART_Sleep_Type for both enum values
 * - Get_UART_Sleep_Type parsing
 * - PinType FixedString validation
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
#include "hm11/types/type_aliases.hpp"
#include "hm11/util/enum_mapping.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// ============================================================================
// Mock UART Port for Testing Security Operations
// ============================================================================

class SecurityTestMockUARTPort {
  public:
    SecurityTestMockUARTPort() = default;

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
        SecurityTestMockUARTPort& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        // Forward to port's receive method
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload WITHOUT timeout parameter (uses default)
    void operator()(SecurityTestMockUARTPort& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        // Forward to port's receive method with default timeout
        port.receive(buffer, status, 1000, as_stream);
    }
};

// Concept satisfaction checks
static_assert(hal::IS_UART_PORT_V<SecurityTestMockUARTPort>, "SecurityTestMockUARTPort must satisfy IS_UART_PORT_V");
static_assert(
    hal::IS_RECEIVE_HANDLER_V<MockReceiveHandler, SecurityTestMockUARTPort>,
    "MockReceiveHandler must satisfy IS_RECEIVE_HANDLER_V");

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverSecurityTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // GoogleTest convention: test fixture members should be protected
    SecurityTestMockUARTPort uart_port;
    MockReceiveHandler receive_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

// ============================================================================
// Enum Mapping Tests
// ============================================================================

TEST(EnumMappingTest, BondMode_AllValues) {
    // Test all 4 bond mode values map correctly
    EXPECT_EQ(util::enum_to_string(BondMode::NotNeedPin), "0");
    EXPECT_EQ(util::enum_to_string(BondMode::AuthNotNeedPin), "1");
    EXPECT_EQ(util::enum_to_string(BondMode::AuthWithPin), "2");
    EXPECT_EQ(util::enum_to_string(BondMode::AuthAndBonded), "3");
}

TEST(EnumMappingTest, WorkType_AllValues) {
    // Test both work type values map correctly
    EXPECT_EQ(util::enum_to_string(WorkType::StartImmediately), "0");
    EXPECT_EQ(util::enum_to_string(WorkType::RespondAtCommand), "1");
}

TEST(EnumMappingTest, UartSleepType_AllValues) {
    // Test both UART sleep type values map correctly
    EXPECT_EQ(util::enum_to_string(UartSleepType::CanWakeUpThroughUart), "0");
    EXPECT_EQ(util::enum_to_string(UartSleepType::ShutdownUart), "1");
}

// ============================================================================
// Bond Mode Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetBondMode_NotNeedPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::NotNeedPin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TYPE0");
}

TEST_F(HM11DriverSecurityTest, SetBondMode_AuthNotNeedPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::AuthNotNeedPin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TYPE1");
}

TEST_F(HM11DriverSecurityTest, SetBondMode_AuthWithPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:2");

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::AuthWithPin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TYPE2");
}

TEST_F(HM11DriverSecurityTest, SetBondMode_AuthAndBonded) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:3");

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::AuthAndBonded, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+TYPE3");
}

TEST_F(HM11DriverSecurityTest, GetBondMode_NotNeedPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    BondMode mode{BondMode::AuthAndBonded}; // Initialize to different value
    UartStatus status = UartStatus::Ok;
    driver.get_bond_mode(mode, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mode, BondMode::NotNeedPin);
}

TEST_F(HM11DriverSecurityTest, GetBondMode_AuthWithPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:2");

    BondMode mode{BondMode::NotNeedPin};
    UartStatus status = UartStatus::Ok;
    driver.get_bond_mode(mode, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mode, BondMode::AuthWithPin);
}

// ============================================================================
// PIN Code Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetPinCode_DefaultPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:000000");

    PinType pin{"000000"};
    UartStatus status = UartStatus::Ok;
    driver.set_pin_code(pin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PASS000000");
}

TEST_F(HM11DriverSecurityTest, SetPinCode_CustomPin) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:123456");

    PinType pin{"123456"};
    UartStatus status = UartStatus::Ok;
    driver.set_pin_code(pin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+PASS123456");
}

TEST_F(HM11DriverSecurityTest, GetPinCode) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:123456");

    PinType pin{"000000"}; // Initialize to different value
    UartStatus status = UartStatus::Ok;
    driver.get_pin_code(pin, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(pin.view(), "123456");
}

TEST(PinTypeTest, SixDigitValidation) {
    // Valid 6-digit PINs
    EXPECT_NO_THROW(PinType("000000"));
    EXPECT_NO_THROW(PinType("123456"));
    EXPECT_NO_THROW(PinType("999999"));
}

// ============================================================================
// Work Type Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetWorkType_StartImmediately) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::Ok;
    driver.set_work_type(WorkType::StartImmediately, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+IMME0");
}

TEST_F(HM11DriverSecurityTest, SetWorkType_RespondAtCommand) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::Ok;
    driver.set_work_type(WorkType::RespondAtCommand, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+IMME1");
}

TEST_F(HM11DriverSecurityTest, GetWorkType_StartImmediately) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    WorkType work{WorkType::RespondAtCommand};
    UartStatus status = UartStatus::Ok;
    driver.get_work_type(work, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(work, WorkType::StartImmediately);
}

TEST_F(HM11DriverSecurityTest, GetWorkType_RespondAtCommand) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:1");

    WorkType work{WorkType::StartImmediately};
    UartStatus status = UartStatus::Ok;
    driver.get_work_type(work, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(work, WorkType::RespondAtCommand);
}

// ============================================================================
// White List Switch Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetWhiteListSwitch_Enable) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::Ok;
    driver.set_white_list_switch(true, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+ALLO1");
}

TEST_F(HM11DriverSecurityTest, SetWhiteListSwitch_Disable) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::Ok;
    driver.set_white_list_switch(false, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+ALLO0");
}

TEST_F(HM11DriverSecurityTest, GetWhiteListSwitch_Enabled) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:1");

    bool enabled = false;
    UartStatus status = UartStatus::Ok;
    driver.get_white_list_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(enabled);
}

TEST_F(HM11DriverSecurityTest, GetWhiteListSwitch_Disabled) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    bool enabled = true;
    UartStatus status = UartStatus::Ok;
    driver.get_white_list_switch(enabled, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_FALSE(enabled);
}

// ============================================================================
// White List MAC Address Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetWhiteListMacAddress_Index1) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:B4994C123456");

    MacWhiteListIndex index{1};
    MacAddress mac{"B4994C123456"};
    UartStatus status = UartStatus::Ok;
    driver.set_white_list_mac_address(index, mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+AD1B4994C123456");
}

TEST_F(HM11DriverSecurityTest, SetWhiteListMacAddress_Index2) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:AABBCCDDEEFF");

    MacWhiteListIndex index{2};
    MacAddress mac{"AABBCCDDEEFF"};
    UartStatus status = UartStatus::Ok;
    driver.set_white_list_mac_address(index, mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+AD2AABBCCDDEEFF");
}

TEST_F(HM11DriverSecurityTest, SetWhiteListMacAddress_Index3) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:112233445566");

    MacWhiteListIndex index{3};
    MacAddress mac{"112233445566"};
    UartStatus status = UartStatus::Ok;
    driver.set_white_list_mac_address(index, mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+AD3112233445566");
}

TEST_F(HM11DriverSecurityTest, GetWhiteListMacAddress_Index1) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+AD1?:B4994C123456");

    MacWhiteListIndex index{1};
    MacAddress mac{"000000000000"};
    UartStatus status = UartStatus::Ok;
    driver.get_white_list_mac_address(index, mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mac.view(), "B4994C123456");
}

TEST_F(HM11DriverSecurityTest, GetWhiteListMacAddress_Index3) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+AD3?:FFFFFFFFFFFF");

    MacWhiteListIndex index{3};
    MacAddress mac{"000000000000"};
    UartStatus status = UartStatus::Ok;
    driver.get_white_list_mac_address(index, mac, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mac.view(), "FFFFFFFFFFFF");
}

TEST(MacWhiteListIndexTest, RangedIntegerValidation) {
    // Valid indices (1-3)
    EXPECT_NO_THROW(MacWhiteListIndex(1));
    EXPECT_NO_THROW(MacWhiteListIndex(2));
    EXPECT_NO_THROW(MacWhiteListIndex(3));
}

// ============================================================================
// Remove Bond Information Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, RemoveBondInformation) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+ERASE");

    UartStatus status = UartStatus::Ok;
    driver.remove_bond_information(status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+ERASE");
}

// ============================================================================
// Service UUID Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetServiceUuid_BatteryService) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0x180F");

    UUID service_uuid{"180F"};
    UartStatus status = UartStatus::Ok;
    driver.set_service_uuid(service_uuid, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+UUID0x180F");
}

TEST_F(HM11DriverSecurityTest, SetServiceUuid_CustomService) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0xABCD");

    UUID service_uuid{"ABCD"};
    UartStatus status = UartStatus::Ok;
    driver.set_service_uuid(service_uuid, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+UUID0xABCD");
}

TEST_F(HM11DriverSecurityTest, GetServiceUuid) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0x180F");

    UUID service_uuid{"0000"};
    UartStatus status = UartStatus::Ok;
    driver.get_service_uuid(service_uuid, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(service_uuid.view(), "180F");
}

// ============================================================================
// UART Sleep Type Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, SetUartSleepType_CanWakeUp) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:0");

    UartStatus status = UartStatus::Ok;
    driver.set_uart_sleep_type(UartSleepType::CanWakeUpThroughUart, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+UART0");
}

TEST_F(HM11DriverSecurityTest, SetUartSleepType_Shutdown) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Set:1");

    UartStatus status = UartStatus::Ok;
    driver.set_uart_sleep_type(UartSleepType::ShutdownUart, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart_port.get_last_transmitted(), "AT+UART1");
}

TEST_F(HM11DriverSecurityTest, GetUartSleepType_CanWakeUp) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:0");

    UartSleepType sleep_type{UartSleepType::ShutdownUart};
    UartStatus status = UartStatus::Ok;
    driver.get_uart_sleep_type(sleep_type, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(sleep_type, UartSleepType::CanWakeUpThroughUart);
}

TEST_F(HM11DriverSecurityTest, GetUartSleepType_Shutdown) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("OK+Get:1");

    UartSleepType sleep_type{UartSleepType::CanWakeUpThroughUart};
    UartStatus status = UartStatus::Ok;
    driver.get_uart_sleep_type(sleep_type, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(sleep_type, UartSleepType::ShutdownUart);
}

// ============================================================================
// Error Propagation Tests
// ============================================================================

TEST_F(HM11DriverSecurityTest, ErrorPropagation_TransmitFails) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_transmit_fails();

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::NotNeedPin, status);

    EXPECT_NE(status, UartStatus::Ok);
}

TEST_F(HM11DriverSecurityTest, ErrorPropagation_ReceiveFails) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_receive_fails();

    BondMode mode{BondMode::NotNeedPin};
    UartStatus status = UartStatus::Ok;
    driver.get_bond_mode(mode, status);

    EXPECT_NE(status, UartStatus::Ok);
}

TEST_F(HM11DriverSecurityTest, ErrorPropagation_ResponseMismatch) {
    HM11Driver<SecurityTestMockUARTPort, MockReceiveHandler> driver(uart_port, receive_handler);
    uart_port.set_response("ERROR");

    UartStatus status = UartStatus::Ok;
    driver.set_bond_mode(BondMode::NotNeedPin, status);

    EXPECT_NE(status, UartStatus::Ok);
}
