// Tests for GATT Service Discovery Implementation
// Validates GATT service discovery callbacks, parsing, and state machine

#include <gtest/gtest.h>

#include <array>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11.hpp>
#include <hm11/util/parsing.hpp>
#include <string_view>
#include <vector>

using namespace hm11;
using namespace hm11::util;

// ============================================================================
// GATT Service Discovery Parsing Tests
// ============================================================================

class GattServiceDiscoveryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Valid service UUID string for testing (format: start:end:uuid)
        valid_service_uuid = "0001:0004:1800";

        // Mock UART for driver testing
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);

        // Reset callback capture
        captured_services.clear();
        callback_call_count = 0;
    }

    // Callback capture for testing
    static void capture_service_callback(std::string_view service, void* user_data) {
        auto* test_instance = static_cast<GattServiceDiscoveryTest*>(user_data);
        test_instance->captured_services.emplace_back(service);
        test_instance->callback_call_count++;
    }

    std::string valid_service_uuid;
    MockUART mock_uart{UARTDataSize::DataSize8b};
    std::vector<std::string> captured_services;
    int callback_call_count = 0;
};

// ============================================================================
// Service UUID Parsing Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, ParseValidServiceUuidComponents) {
    std::string_view start_handle, end_handle, service_uuid;

    bool result = parse_service_uuid_components(valid_service_uuid, start_handle, end_handle, service_uuid);

    EXPECT_TRUE(result);

    // Verify each extracted component
    EXPECT_EQ(start_handle, "0001");
    EXPECT_EQ(end_handle, "0004");
    EXPECT_EQ(service_uuid, "1800");
}

TEST_F(GattServiceDiscoveryTest, ParseServiceUuidInvalidLength) {
    std::string_view start_handle, end_handle, service_uuid;

    // Too short
    bool result1 = parse_service_uuid_components("0001:0004", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result1);

    // Too long
    bool result2 = parse_service_uuid_components("0001:0004:1800:extra", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result2);
}

TEST_F(GattServiceDiscoveryTest, ParseServiceUuidInvalidFormat) {
    std::string_view start_handle, end_handle, service_uuid;

    // Missing first colon
    bool result1 = parse_service_uuid_components("0001 0004:1800", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result1);

    // Missing second colon
    bool result2 = parse_service_uuid_components("0001:0004 1800", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result2);

    // Wrong colon positions
    bool result3 = parse_service_uuid_components("001:00041:800", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result3);
}

TEST_F(GattServiceDiscoveryTest, ParseServiceUuidInvalidHexCharacters) {
    std::string_view start_handle, end_handle, service_uuid;

    // Invalid characters in start handle
    bool result1 = parse_service_uuid_components("000G:0004:1800", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result1);

    // Invalid characters in end handle
    bool result2 = parse_service_uuid_components("0001:000Z:1800", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result2);

    // Invalid characters in service UUID
    bool result3 = parse_service_uuid_components("0001:0004:180X", start_handle, end_handle, service_uuid);
    EXPECT_FALSE(result3);
}

TEST_F(GattServiceDiscoveryTest, ParseServiceUuidValidHexCases) {
    std::string_view start_handle, end_handle, service_uuid;

    // Mixed case hex characters
    bool result = parse_service_uuid_components("00A1:0bC4:1eFf", start_handle, end_handle, service_uuid);

    EXPECT_TRUE(result);
    EXPECT_EQ(start_handle, "00A1");
    EXPECT_EQ(end_handle, "0bC4");
    EXPECT_EQ(service_uuid, "1eFf");
}

// ============================================================================
// Service Discovery Header Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, ServiceHeaderConstantsCorrect) {
    // Verify service header constant matches Ada implementation
    constexpr auto expected_header = "********************************************************";
    EXPECT_EQ(gatt_service_discovery::SERVICE_HEADER, expected_header);
    EXPECT_EQ(gatt_service_discovery::SERVICE_HEADER_LENGTH, 56);
    EXPECT_EQ(gatt_service_discovery::SERVICE_UUID_LENGTH, 14);
}

TEST_F(GattServiceDiscoveryTest, ServiceHeaderCircularBufferCheck) {
    // Create test buffer with service header
    constexpr std::string_view header = gatt_service_discovery::SERVICE_HEADER;
    std::array<uint8_t, 100> buffer{};

    // Copy header to buffer
    for (std::size_t i = 0; i < header.size(); ++i) {
        buffer[i] = static_cast<uint8_t>(header[i]);
    }

    bool result = has_service_header_circular(buffer.data(), buffer.size(), 0, header.size());

    EXPECT_TRUE(result);
}

TEST_F(GattServiceDiscoveryTest, ServiceHeaderCircularBufferNoMatch) {
    // Create test buffer without service header
    std::array<uint8_t, 100> buffer{};
    buffer.fill(static_cast<uint8_t>('X'));

    bool result = has_service_header_circular(buffer.data(), buffer.size(), 0, 56);

    EXPECT_FALSE(result);
}

// ============================================================================
// Circular Buffer Extraction Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, ExtractServiceUuidCircular) {
    // Create test buffer with service UUID
    std::array<uint8_t, 30> buffer{};
    const std::string test_service = "0001:0004:1800";

    for (std::size_t i = 0; i < test_service.size(); ++i) {
        buffer[i] = static_cast<uint8_t>(test_service[i]);
    }

    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH> extracted{};

    bool result = extract_service_uuid_circular(buffer.data(), buffer.size(), 0, extracted);

    EXPECT_TRUE(result);

    std::string_view extracted_str{extracted.data(), extracted.size()};
    EXPECT_EQ(extracted_str, test_service);
}

TEST_F(GattServiceDiscoveryTest, ExtractServiceUuidCircularWrapping) {
    // Test circular buffer wrapping
    std::array<uint8_t, 20> buffer{};
    const std::string test_service = "0001:0004:1800";

    // Place service UUID starting at position 10, wrapping around
    for (std::size_t i = 0; i < test_service.size(); ++i) {
        buffer[(10 + i) % buffer.size()] = static_cast<uint8_t>(test_service[i]);
    }

    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH> extracted{};

    bool result = extract_service_uuid_circular(buffer.data(), buffer.size(), 10, extracted);

    EXPECT_TRUE(result);

    std::string_view extracted_str{extracted.data(), extracted.size()};
    EXPECT_EQ(extracted_str, test_service);
}

// ============================================================================
// GATT State Machine Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, GattServiceStageEnumValues) {
    // Verify enum values for state machine
    EXPECT_EQ(static_cast<uint8_t>(GattServiceStage::LookingForHeader), 0);
    EXPECT_EQ(static_cast<uint8_t>(GattServiceStage::ParsingService), 1);
    EXPECT_EQ(static_cast<uint8_t>(GattServiceStage::LookingForEnd), 2);
}

// ============================================================================
// Driver Integration Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, FindAllServicesUuidNullCallback) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Ok;

    // Test null callback handling
    driver.find_all_services_uuid(nullptr, this, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(GattServiceDiscoveryTest, FindAllServicesUuidBasicFlow) {
    // Set up mock UART to succeed
    mock_uart.SetForcedStatus(UARTStatus::Ok);

    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Ok;

    // Test basic call flow (will not parse real response without streaming implementation)
    driver.find_all_services_uuid(capture_service_callback, this, 1000, status);

    // Should complete without error even if no services found
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(GattServiceDiscoveryTest, FindAllServicesUuidTransmitError) {
    // Set up mock UART to fail on transmit
    mock_uart.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Ok;

    driver.find_all_services_uuid(capture_service_callback, this, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(callback_call_count, 0);
}

// ============================================================================
// Callback Functionality Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, ServiceUuidCallbackSignature) {
    // Test callback function pointer type
    ServiceUuidCallback callback = capture_service_callback;
    EXPECT_NE(callback, nullptr);

    // Test callback invocation
    callback("test:serv:ice", this);

    EXPECT_EQ(callback_call_count, 1);
    ASSERT_EQ(captured_services.size(), 1);
    EXPECT_EQ(captured_services[0], "test:serv:ice");
}

TEST_F(GattServiceDiscoveryTest, ServiceUuidCallbackUserData) {
    // Test user data passing
    int test_value = 42;

    auto lambda_callback = [](std::string_view /*service*/, void* user_data) {
        auto* value = static_cast<int*>(user_data);
        *value = 100;
    };

    lambda_callback("test", &test_value);

    EXPECT_EQ(test_value, 100);
}

// ============================================================================
// Driver State Initialization Tests
// ============================================================================

TEST_F(GattServiceDiscoveryTest, DriverGattStateInitialization) {
    auto driver = make_hm11_driver(std::move(mock_uart));

    // Test that driver initializes GATT state correctly
    // Note: We can't access private members directly, but we can test
    // that multiple calls work correctly, implying state is managed

    UARTStatus status = UARTStatus::Ok;

    // First call should not fail due to uninitialized state
    driver.find_all_services_uuid(capture_service_callback, this, 1000, status);
    EXPECT_NE(status, UARTStatus::ErrError); // Should be Ok if UART succeeds

    // Second call should also work, implying state reset
    driver.find_all_services_uuid(capture_service_callback, this, 1000, status);
    EXPECT_NE(status, UARTStatus::ErrError);
}

// ============================================================================
// Error Handling and Edge Cases
// ============================================================================

TEST_F(GattServiceDiscoveryTest, ZeroTimeoutHandling) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Ok;

    // Test zero timeout (should not cause crashes)
    driver.find_all_services_uuid(capture_service_callback, this, 0, status);

    // Should complete, timeout behavior depends on implementation details
    EXPECT_TRUE(status == UARTStatus::Ok || status == UARTStatus::ErrTimeout);
}

TEST_F(GattServiceDiscoveryTest, VeryLargeTimeoutHandling) {
    auto driver = make_hm11_driver(std::move(mock_uart));
    UARTStatus status = UARTStatus::Ok;

    // Test very large timeout
    constexpr uint32_t large_timeout = 0xFFFFFFFF;
    driver.find_all_services_uuid(capture_service_callback, this, large_timeout, status);

    // Should not overflow or cause issues
    EXPECT_TRUE(status == UARTStatus::Ok || status == UARTStatus::ErrTimeout);
}