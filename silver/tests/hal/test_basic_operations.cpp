// Test file for basic HM11 device operations: Test, Set_Role, Get_Role
// This file specifically validates Task 4 implementation requirements

#include <gtest/gtest.h>

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"

using namespace hm11;
using namespace hm11::hal;

class BasicOperationsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Reset mock UARTs for each test
        mock_uart_.ClearBuffers();
        mock_uart_.SetForcedStatus(UARTStatus::Ok);
        mock_uart_.SetForcedStatusForReceive(UARTStatus::Ok);
    }

    MockUART mock_uart_{UARTDataSize::DataSize8b};
};

// ============================================================================
// Test Operation Tests
// ============================================================================

TEST_F(BasicOperationsTest, TestOperationSuccess) {
    // Setup mock to return "OK" when "AT" is sent
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute test operation
    driver.Test(status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 2);
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
}

TEST_F(BasicOperationsTest, TestOperationFailureNoResponse) {
    // Setup mock to return different response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'E', 'R'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;

    // Execute test operation
    driver.Test(status);

    // Verify failure
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(BasicOperationsTest, TestOperationUARTError) {
    // Setup mock to return UART error
    mock_uart_.SetForcedStatus(UARTStatus::ErrTimeout);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;

    // Execute test operation
    driver.Test(status);

    // Verify UART error is propagated
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

// ============================================================================
// Set_Role Operation Tests
// ============================================================================

TEST_F(BasicOperationsTest, SetRolePeripheralSuccess) {
    // Setup mock to return expected "OK+Set:0" response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute set_role operation
    driver.SetRole(Role::Peripheral, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8); // "AT+ROLE0"
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
    EXPECT_EQ(transmitted[2], '+');
    EXPECT_EQ(transmitted[3], 'R');
    EXPECT_EQ(transmitted[4], 'O');
    EXPECT_EQ(transmitted[5], 'L');
    EXPECT_EQ(transmitted[6], 'E');
    EXPECT_EQ(transmitted[7], '0'); // Peripheral = 0
}

TEST_F(BasicOperationsTest, SetRoleCentralSuccess) {
    // Setup mock to return expected "OK+Set:1" response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;

    // Execute set_role operation
    driver.SetRole(Role::Central, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the correct command was sent
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8); // "AT+ROLE1"
    EXPECT_EQ(transmitted[7], '1');   // Central = 1
}

TEST_F(BasicOperationsTest, SetRoleFailureWrongResponse) {
    // Setup mock to return wrong response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'}); // Expected 0

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;

    // Execute set_role operation with Peripheral (expects response with 0)
    driver.SetRole(Role::Peripheral, status);

    // Verify failure due to response mismatch
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// ============================================================================
// Get_Role Operation Tests
// ============================================================================

TEST_F(BasicOperationsTest, GetRolePeripheralSuccess) {
    // Setup mock to return "OK+Get:0" response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '0'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    Role result = Role::Central; // Initialize to opposite value

    // Execute get_role operation
    driver.GetRole(result, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, Role::Peripheral);

    // Verify the correct command was sent
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8); // "AT+ROLE?"
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
    EXPECT_EQ(transmitted[2], '+');
    EXPECT_EQ(transmitted[3], 'R');
    EXPECT_EQ(transmitted[4], 'O');
    EXPECT_EQ(transmitted[5], 'L');
    EXPECT_EQ(transmitted[6], 'E');
    EXPECT_EQ(transmitted[7], '?');
}

TEST_F(BasicOperationsTest, GetRoleCentralSuccess) {
    // Setup mock to return "OK+Get:1" response
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Busy;
    Role result = Role::Peripheral; // Initialize to opposite value

    // Execute get_role operation
    driver.GetRole(result, status);

    // Verify success
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, Role::Central);
}

TEST_F(BasicOperationsTest, GetRoleInvalidResponse) {
    // Setup mock to return invalid role character
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', 'X'});

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;
    Role result = Role::Peripheral;

    // Execute get_role operation
    driver.GetRole(result, status);

    // Verify failure
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(BasicOperationsTest, GetRoleWrongResponsePrefix) {
    // Setup mock to return wrong response prefix
    mock_uart_.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'}); // Wrong: "OK+Set"

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;
    Role result = Role::Peripheral;

    // Execute get_role operation
    driver.GetRole(result, status);

    // Verify failure
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(BasicOperationsTest, GetRoleReceiveError) {
    // Setup mock to return receive error
    mock_uart_.SetForcedStatusForReceive(UARTStatus::ErrTimeout);

    auto driver = make_hm11_driver(std::move(mock_uart_));
    UARTStatus status = UARTStatus::Ok;
    Role result = Role::Peripheral;

    // Execute get_role operation
    driver.GetRole(result, status);

    // Verify UART error is propagated
    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(BasicOperationsTest, SetAndGetRoleIntegration) {
    auto mock_uart = MockUART{UARTDataSize::DataSize8b};

    // First set role to Central
    mock_uart.SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'S', 'e', 't', ':', '1'});
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Busy;

    driver.SetRole(Role::Central, status);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Clear buffers and set up for get operation
    driver.GetUart().ClearTransmittedData();
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'O', 'K', '+', 'G', 'e', 't', ':', '1'});

    // Now get the role
    Role result = Role::Peripheral; // Initialize to opposite
    driver.GetRole(result, status);

    // Verify both operations succeeded
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, Role::Central);
}

// ============================================================================
// Enum Conversion Validation Tests
// ============================================================================

TEST_F(BasicOperationsTest, RoleEnumConversions) {
    using namespace hm11::util;

    // Test to_string conversions
    EXPECT_EQ(to_string(Role::Peripheral), "0");
    EXPECT_EQ(to_string(Role::Central), "1");

    // Test parse_role conversions
    auto peripheral_result = parse_role('0');
    EXPECT_TRUE(peripheral_result.has_value());
    EXPECT_EQ(*peripheral_result, Role::Peripheral);

    auto central_result = parse_role('1');
    EXPECT_TRUE(central_result.has_value());
    EXPECT_EQ(*central_result, Role::Central);

    // Test invalid character
    auto invalid_result = parse_role('X');
    EXPECT_FALSE(invalid_result.has_value());
}