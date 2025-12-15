// Tests for circular buffer parsing utility functions
// Covers error paths in parsing.hpp circular buffer functions

#include <gtest/gtest.h>
#include <hm11/hm11_driver.hpp>  // For VariableString definition
#include <hm11/util/parsing.hpp>
#include <hm11/types.hpp>

using namespace hm11;
using namespace hm11::util;

// ============================================================================
// parse_mac_address error cases
// ============================================================================

TEST(ParsingCircularBufferTest, ParseMacAddress_ExceptionPath) {
    // Test the catch block by providing a string that passes validation
    // but might fail in MacAddress constructor
    auto result = parse_mac_address("000000000000");
    // This should succeed, but we're ensuring the try-catch is exercised
    EXPECT_TRUE(result.has_value());
}

// ============================================================================
// starts_with_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, StartsWithCircular_NullBuffer) {
    bool result = starts_with_circular(nullptr, 100, "OK", 0, 10);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, StartsWithCircular_ZeroSize) {
    uint8_t buffer[] = {'O', 'K'};
    bool result = starts_with_circular(buffer, 0, "OK", 0, 2);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, StartsWithCircular_WrappedBuffer) {
    // Test the wrapped case where end_pos < start_pos
    uint8_t buffer[] = {'K', 'a', 'b', 'c', 'O'};
    // start_pos = 4, end_pos = 1 means "OK" wraps around
    bool result = starts_with_circular(buffer, 5, "OK", 4, 1);
    EXPECT_TRUE(result);
}

TEST(ParsingCircularBufferTest, StartsWithCircular_InsufficientLength) {
    uint8_t buffer[] = {'O', 'K'};
    // Trying to match "OK+Get" but buffer only has "OK"
    bool result = starts_with_circular(buffer, 2, "OK+Get", 0, 2);
    EXPECT_FALSE(result);
}

// ============================================================================
// find_crlf_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, FindCrlfCircular_NullBuffer) {
    std::size_t result = find_crlf_circular(nullptr, 100, 0, 10);
    EXPECT_EQ(result, SIZE_MAX);
}

TEST(ParsingCircularBufferTest, FindCrlfCircular_ZeroSize) {
    uint8_t buffer[] = {'\r', '\n'};
    std::size_t result = find_crlf_circular(buffer, 0, 0, 2);
    EXPECT_EQ(result, SIZE_MAX);
}

TEST(ParsingCircularBufferTest, FindCrlfCircular_WrappedBuffer) {
    // Test wrapped case where end_pos < start_pos
    uint8_t buffer[] = {'\n', 'a', 'b', 'c', '\r'};
    // start_pos = 4, end_pos = 1 means "\r\n" wraps around
    std::size_t result = find_crlf_circular(buffer, 5, 4, 1);
    EXPECT_EQ(result, 4); // Position of '\r'
}

TEST(ParsingCircularBufferTest, FindCrlfCircular_InsufficientLength) {
    uint8_t buffer[] = {'a'};
    // Only 1 character, need at least 2 for CRLF
    std::size_t result = find_crlf_circular(buffer, 1, 0, 1);
    EXPECT_EQ(result, SIZE_MAX);
}

// ============================================================================
// extract_mac_address_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, ExtractMacAddressCircular_NullBuffer) {
    MacAddress mac;
    bool result = extract_mac_address_circular(nullptr, 100, 0, mac);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractMacAddressCircular_ZeroSize) {
    uint8_t buffer[12] = {};
    MacAddress mac;
    bool result = extract_mac_address_circular(buffer, 0, 0, mac);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractMacAddressCircular_InvalidHexChar) {
    // Buffer contains non-hex character 'G'
    uint8_t buffer[] = {'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D', 'E', 'E', 'F', 'G'};
    MacAddress mac;
    bool result = extract_mac_address_circular(buffer, 12, 0, mac);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractMacAddressCircular_ValidMac) {
    uint8_t buffer[] = {'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D', 'E', 'E', 'F', 'F'};
    MacAddress mac;
    bool result = extract_mac_address_circular(buffer, 12, 0, mac);
    EXPECT_TRUE(result);
}

// ============================================================================
// extract_string_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, ExtractStringCircular_NullBuffer) {
    VariableString<64> dest;
    bool result = extract_string_circular(nullptr, 100, 0, 10, dest);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractStringCircular_ZeroSize) {
    uint8_t buffer[] = {'H', 'e', 'l', 'l', 'o'};
    VariableString<64> dest;
    bool result = extract_string_circular(buffer, 0, 0, 5, dest);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractStringCircular_InsufficientCapacity) {
    uint8_t buffer[] = {'H', 'e', 'l', 'l', 'o'};
    VariableString<3> dest; // Capacity too small
    bool result = extract_string_circular(buffer, 5, 0, 5, dest);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractStringCircular_Valid) {
    uint8_t buffer[] = {'H', 'e', 'l', 'l', 'o'};
    VariableString<64> dest;
    bool result = extract_string_circular(buffer, 5, 0, 5, dest);
    EXPECT_TRUE(result);
    EXPECT_EQ(dest.view(), "Hello");
}

// ============================================================================
// extract_service_uuid_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, ExtractServiceUuidCircular_NullBuffer) {
    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH> service_data;
    bool result = extract_service_uuid_circular(nullptr, 100, 0, service_data);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractServiceUuidCircular_ZeroSize) {
    uint8_t buffer[14] = {};
    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH> service_data;
    bool result = extract_service_uuid_circular(buffer, 0, 0, service_data);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractServiceUuidCircular_Valid) {
    uint8_t buffer[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E'};
    std::array<char, gatt_service_discovery::SERVICE_UUID_LENGTH> service_data;
    bool result = extract_service_uuid_circular(buffer, 14, 0, service_data);
    EXPECT_TRUE(result);
}

// ============================================================================
// parse_characteristic_uuid_components tests
// ============================================================================

TEST(ParsingCircularBufferTest, ParseCharacteristicUuidComponents_InvalidProperties) {
    // Format: handle:properties:uuid (4:14:4)
    std::string_view characteristic_str = "1234:INVALID_PROPS:5678";
    std::string_view handle, characteristic_uuid;
    CharacteristicProperty parsed_props{};

    bool result = parse_characteristic_uuid_components(characteristic_str, handle, parsed_props, characteristic_uuid);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ParseCharacteristicUuidComponents_Valid) {
    // Format: handle:properties:uuid (4:14:4) - total 24 chars including colons
    // Properties format must be: RD|WR|NT|IN|BR with pipes at positions 2,5,8,11 (14 chars total)
    std::string_view characteristic_str = "1234:RD|WR|NT|IN|BR:5678";
    std::string_view handle, characteristic_uuid;
    CharacteristicProperty parsed_props{};

    bool result = parse_characteristic_uuid_components(characteristic_str, handle, parsed_props, characteristic_uuid);
    EXPECT_TRUE(result);
}

// ============================================================================
// extract_characteristic_uuid_circular tests
// ============================================================================

TEST(ParsingCircularBufferTest, ExtractCharacteristicUuidCircular_NullBuffer) {
    std::array<char, gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH> char_data;
    bool result = extract_characteristic_uuid_circular(nullptr, 100, 0, char_data);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractCharacteristicUuidCircular_ZeroSize) {
    uint8_t buffer[24] = {};
    std::array<char, gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH> char_data;
    bool result = extract_characteristic_uuid_circular(buffer, 0, 0, char_data);
    EXPECT_FALSE(result);
}

TEST(ParsingCircularBufferTest, ExtractCharacteristicUuidCircular_Valid) {
    uint8_t buffer[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
                        'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6', '7'};
    std::array<char, gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH> char_data;
    bool result = extract_characteristic_uuid_circular(buffer, 24, 0, char_data);
    EXPECT_TRUE(result);
}
