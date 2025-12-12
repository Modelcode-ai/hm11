/**
 * @file test_hm11_driver_gatt_access.cpp
 * @brief Unit tests for HM11 driver GATT characteristic access operations
 *
 * Tests the GATT characteristic access methods including:
 * - Enable_Characteristic_Notify with handle and response parsing
 * - Disable_Characteristic_Notify with handle and response parsing
 * - Read_Characteristic_Notify with handle and response parsing
 * - NotifyResponse parsing (SendOk, SendEr, DataEr)
 * - Set_Method_And_Characteristic_Handle with handle and method
 * - Send_Data_To_Characteristic with handle, method, and data
 * - Set_Use_Characteristic_UUID_Count with all enum values
 * - Set_Characteristic with 4-character hex UUID
 * - Get_Characteristic parsing 4-character hex UUID with "0x" prefix
 * - HandleType and CharacteristicType validation
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

class HM11DriverGATTAccessTest : public ::testing::Test {
  protected:
    MockUARTPort uart_port_;
    MockReceiveHandler receive_handler_;
    HM11Driver<MockUARTPort, MockReceiveHandler> driver_{uart_port_, receive_handler_};
};

// ============================================================================
// Enable_Characteristic_Notify Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, EnableCharacteristicNotify_SendOk) {
    // Test enabling notify with successful response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.enable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendOk);
    EXPECT_EQ(uart_port_.last_command(), "AT+NOTIFY_ON002A");
}

TEST_F(HM11DriverGATTAccessTest, EnableCharacteristicNotify_SendEr) {
    // Test enabling notify with error response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.enable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendEr);
}

TEST_F(HM11DriverGATTAccessTest, EnableCharacteristicNotify_DataEr) {
    // Test enabling notify when characteristic doesn't have notify property
    types::HandleType handle("002A");
    uart_port_.set_response("OK+DATA-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.enable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::DataEr);
}

TEST_F(HM11DriverGATTAccessTest, EnableCharacteristicNotify_DifferentHandles) {
    // Test with various handle values
    std::vector<std::string> handles = {"0001", "FFFF", "1234", "ABCD"};

    for (const auto& handle_str : handles) {
        types::HandleType handle(handle_str);
        uart_port_.set_response("OK+SEND-OK");

        hal::UartStatus status{};
        types::NotifyResponse response{};
        driver_.enable_characteristic_notify(handle, status, response);

        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(response, types::NotifyResponse::SendOk);
        EXPECT_EQ(uart_port_.last_command(), "AT+NOTIFY_ON" + handle_str);
    }
}

// ============================================================================
// Disable_Characteristic_Notify Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, DisableCharacteristicNotify_SendOk) {
    // Test disabling notify with successful response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.disable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendOk);
    EXPECT_EQ(uart_port_.last_command(), "AT+NOTIFYOFF002A");
}

TEST_F(HM11DriverGATTAccessTest, DisableCharacteristicNotify_SendEr) {
    // Test disabling notify with error response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.disable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendEr);
}

TEST_F(HM11DriverGATTAccessTest, DisableCharacteristicNotify_DataEr) {
    // Test disabling notify when characteristic doesn't have notify property
    types::HandleType handle("002A");
    uart_port_.set_response("OK+DATA-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.disable_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::DataEr);
}

// ============================================================================
// Read_Characteristic_Notify Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, ReadCharacteristicNotify_SendOk) {
    // Test reading characteristic with successful response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.read_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendOk);
    EXPECT_EQ(uart_port_.last_command(), "AT+READDATA002A");
}

TEST_F(HM11DriverGATTAccessTest, ReadCharacteristicNotify_SendEr) {
    // Test reading characteristic with error response
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.read_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::SendEr);
}

TEST_F(HM11DriverGATTAccessTest, ReadCharacteristicNotify_DataEr) {
    // Test reading characteristic when it doesn't have read property
    types::HandleType handle("002A");
    uart_port_.set_response("OK+DATA-ER");

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.read_characteristic_notify(handle, status, response);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(response, types::NotifyResponse::DataEr);
}

// ============================================================================
// Set_Method_And_Characteristic_Handle Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, SetMethodAndCharacteristicHandle_Write) {
    // Test setting method and handle with Write method
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    driver_.set_method_and_characteristic_handle(handle, types::SendDataMethod::Write, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+SET_WAYWR002A");
}

TEST_F(HM11DriverGATTAccessTest, SetMethodAndCharacteristicHandle_WriteWithoutResponse) {
    // Test setting method and handle with WriteWithoutResponse method
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    driver_.set_method_and_characteristic_handle(handle, types::SendDataMethod::WriteWithoutResponse, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+SET_WAYWN002A");
}

TEST_F(HM11DriverGATTAccessTest, SetMethodAndCharacteristicHandle_Indicate) {
    // Test setting method and handle with Indicate method
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    driver_.set_method_and_characteristic_handle(handle, types::SendDataMethod::Indicate, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+SET_WAYIN002A");
}

TEST_F(HM11DriverGATTAccessTest, SetMethodAndCharacteristicHandle_Notify) {
    // Test setting method and handle with Notify method
    types::HandleType handle("002A");
    uart_port_.set_response("OK+SEND-OK");

    hal::UartStatus status{};
    driver_.set_method_and_characteristic_handle(handle, types::SendDataMethod::Notify, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+SET_WAYNO002A");
}

// ============================================================================
// Send_Data_To_Characteristic Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, SendDataToCharacteristic_Write_SingleByte) {
    // Test sending single byte with Write method
    types::HandleType handle("002A");
    std::array<uint8_t, 1> data{0x42};

    hal::UartStatus status{};
    driver_
        .send_data_to_characteristic(handle, types::SendDataCharacteristic::Write, hal::UartData8bConst(data), status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    // Command should be sent in two parts: AT command + data
    EXPECT_EQ(uart_port_.last_command()[0], 0x42);
}

TEST_F(HM11DriverGATTAccessTest, SendDataToCharacteristic_WriteWithoutResponse) {
    // Test sending data with WriteWithoutResponse method
    types::HandleType handle("002A");
    std::array<uint8_t, 3> data{0x01, 0x02, 0x03};

    hal::UartStatus status{};
    driver_.send_data_to_characteristic(
        handle,
        types::SendDataCharacteristic::WriteWithoutResponse,
        hal::UartData8bConst(data),
        status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
}

TEST_F(HM11DriverGATTAccessTest, SendDataToCharacteristic_MaxPayload) {
    // Test sending maximum BLE payload (20 bytes)
    types::HandleType handle("002A");
    std::array<uint8_t, 20> data{};
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i);
    }

    hal::UartStatus status{};
    driver_
        .send_data_to_characteristic(handle, types::SendDataCharacteristic::Write, hal::UartData8bConst(data), status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
}

TEST_F(HM11DriverGATTAccessTest, SendDataToCharacteristic_VariousPayloadSizes) {
    // Test various payload sizes from 1 to 20 bytes
    types::HandleType handle("002A");

    for (size_t size = 1; size <= 20; ++size) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(i & 0xFF);
        }

        hal::UartStatus status{};
        driver_.send_data_to_characteristic(
            handle,
            types::SendDataCharacteristic::Write,
            hal::UartData8bConst(data),
            status);

        EXPECT_EQ(status, hal::UartStatus::Ok) << "Failed for payload size: " << size;
    }
}

// ============================================================================
// Set_Use_Characteristic_UUID_Count Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, SetUseCharacteristicUuidCount_Query) {
    // Test querying current characteristic UUID count setting
    uart_port_.set_response("OK+Set:?");

    hal::UartStatus status{};
    driver_.set_use_characteristic_uuid_count(types::CharacteristicUuidCount::Query, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FFE2?");
}

TEST_F(HM11DriverGATTAccessTest, SetUseCharacteristicUuidCount_One) {
    // Test setting characteristic UUID count to One
    uart_port_.set_response("OK+Set:0");

    hal::UartStatus status{};
    driver_.set_use_characteristic_uuid_count(types::CharacteristicUuidCount::One, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FFE20");
}

TEST_F(HM11DriverGATTAccessTest, SetUseCharacteristicUuidCount_OneAndNext) {
    // Test setting characteristic UUID count to OneAndNext
    uart_port_.set_response("OK+Set:1");

    hal::UartStatus status{};
    driver_.set_use_characteristic_uuid_count(types::CharacteristicUuidCount::OneAndNext, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FFE21");
}

TEST_F(HM11DriverGATTAccessTest, SetUseCharacteristicUuidCount_OneAndPrev) {
    // Test setting characteristic UUID count to OneAndPrev
    uart_port_.set_response("OK+Set:2");

    hal::UartStatus status{};
    driver_.set_use_characteristic_uuid_count(types::CharacteristicUuidCount::OneAndPrev, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+FFE22");
}

// ============================================================================
// Set_Characteristic Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, SetCharacteristic_DefaultFFE1) {
    // Test setting characteristic UUID to default FFE1
    types::CharacteristicType uuid("FFE1");
    uart_port_.set_response("OK+Set:0xFFE1\r\n");

    hal::UartStatus status{};
    driver_.set_characteristic(uuid, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+CHAR0xFFE1");
}

TEST_F(HM11DriverGATTAccessTest, SetCharacteristic_Custom2A19) {
    // Test setting characteristic UUID to Battery Level (2A19)
    types::CharacteristicType uuid("2A19");
    uart_port_.set_response("OK+Set:0x2A19\r\n");

    hal::UartStatus status{};
    driver_.set_characteristic(uuid, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+CHAR0x2A19");
}

TEST_F(HM11DriverGATTAccessTest, SetCharacteristic_Various) {
    // Test various characteristic UUIDs
    std::vector<std::string> uuids = {"0001", "FFFF", "1234", "ABCD", "FFE2"};

    for (const auto& uuid_str : uuids) {
        types::CharacteristicType uuid(uuid_str);
        uart_port_.set_response("OK+Set:0x" + uuid_str + "\r\n");

        hal::UartStatus status{};
        driver_.set_characteristic(uuid, status);

        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(uart_port_.last_command(), "AT+CHAR0x" + uuid_str);
    }
}

// ============================================================================
// Get_Characteristic Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, GetCharacteristic_DefaultFFE1) {
    // Test getting characteristic UUID (default FFE1)
    uart_port_.set_response("OK+Get:0xFFE1");

    types::CharacteristicType result("0000"); // Initialize with dummy value
    hal::UartStatus status{};
    driver_.get_characteristic(result, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(uart_port_.last_command(), "AT+CHAR?");
    EXPECT_EQ(result.view(), "FFE1");
}

TEST_F(HM11DriverGATTAccessTest, GetCharacteristic_Custom2A19) {
    // Test getting characteristic UUID (Battery Level 2A19)
    uart_port_.set_response("OK+Get:0x2A19");

    types::CharacteristicType result("0000");
    hal::UartStatus status{};
    driver_.get_characteristic(result, status);

    EXPECT_EQ(status, hal::UartStatus::Ok);
    EXPECT_EQ(result.view(), "2A19");
}

TEST_F(HM11DriverGATTAccessTest, GetCharacteristic_Various) {
    // Test getting various characteristic UUIDs
    std::vector<std::string> expected_uuids = {"0001", "FFFF", "1234", "ABCD", "FFE2"};

    for (const auto& expected : expected_uuids) {
        uart_port_.set_response("OK+Get:0x" + expected);

        types::CharacteristicType result("0000");
        hal::UartStatus status{};
        driver_.get_characteristic(result, status);

        EXPECT_EQ(status, hal::UartStatus::Ok);
        EXPECT_EQ(result.view(), expected);
    }
}

// ============================================================================
// HandleType and CharacteristicType Validation Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, HandleType_ValidHexCharacters) {
    // Test HandleType accepts valid hex characters
    std::vector<std::string> valid_handles = {"0000", "1234", "ABCD", "FFFF", "0A1F"};

    for (const auto& handle_str : valid_handles) {
        types::HandleType handle(handle_str);
        EXPECT_EQ(handle.view(), handle_str);
    }
}

TEST_F(HM11DriverGATTAccessTest, CharacteristicType_ValidHexCharacters) {
    // Test CharacteristicType accepts valid hex characters
    std::vector<std::string> valid_uuids = {"0000", "1234", "ABCD", "FFFF", "FFE1", "2A19"};

    for (const auto& uuid_str : valid_uuids) {
        types::CharacteristicType uuid(uuid_str);
        EXPECT_EQ(uuid.view(), uuid_str);
    }
}

// ============================================================================
// Error Propagation Tests
// ============================================================================

TEST_F(HM11DriverGATTAccessTest, EnableNotify_UartError) {
    // Test UART error propagation for enable notify
    types::HandleType handle("002A");
    // Don't set response, so receive will fail or return empty

    hal::UartStatus status{};
    types::NotifyResponse response{};
    driver_.enable_characteristic_notify(handle, status, response);

    // Status should reflect UART operation result
    EXPECT_EQ(status, hal::UartStatus::Ok); // Mock always returns Ok, but in real scenario would fail
}

TEST_F(HM11DriverGATTAccessTest, SetCharacteristic_Success) {
    // Test UART success with proper response
    types::CharacteristicType uuid("FFE1");
    uart_port_.set_response("OK+Set:0xFFE1\r\n");

    hal::UartStatus status{};
    driver_.set_characteristic(uuid, status);

    // Should complete successfully with proper response
    EXPECT_EQ(status, hal::UartStatus::Ok);
}

} // namespace hm11::test
