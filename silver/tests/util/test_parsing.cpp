// Tests for HM11 Parsing Utilities
// Validates iBeacon parsing functions and discovery utilities

#include <gtest/gtest.h>

#include <array>
#include <hm11/types.hpp>
#include <hm11/util/parsing.hpp>
#include <string_view>

using namespace hm11;
using namespace hm11::util;

// ============================================================================
// iBeacon Parsing Tests
// ============================================================================

class IBeaconParsingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Valid 66-character iBeacon device string for testing
        valid_ibeacon_data = "12345678"                         // Factory ID (8 chars)
                             "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4" // UUID (32 chars)
                             "0001"                             // Major (4 chars)
                             "0002"                             // Minor (4 chars)
                             "C5"                               // Measured Power (2 chars)
                             "AABBCCDDEEFF"                     // MAC (12 chars)
                             "-055";                            // RSSI (4 chars)
    }

    std::string valid_ibeacon_data;
};

TEST_F(IBeaconParsingTest, ParseValidIBeaconDevice) {
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;

    bool result = parse_ibeacon_device(valid_ibeacon_data, factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_TRUE(result);

    // Verify each extracted field
    EXPECT_EQ(factory_id, "12345678");
    EXPECT_EQ(uuid, "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4");
    EXPECT_EQ(major, "0001");
    EXPECT_EQ(minor, "0002");
    EXPECT_EQ(measured_power, "C5");
    EXPECT_EQ(mac, "AABBCCDDEEFF");
    EXPECT_EQ(rssi, "-055");

    // Verify field lengths
    EXPECT_EQ(factory_id.length(), 8);
    EXPECT_EQ(uuid.length(), 32);
    EXPECT_EQ(major.length(), 4);
    EXPECT_EQ(minor.length(), 4);
    EXPECT_EQ(measured_power.length(), 2);
    EXPECT_EQ(mac.length(), 12);
    EXPECT_EQ(rssi.length(), 4);
}

TEST_F(IBeaconParsingTest, ParseIBeaconDeviceWrongLength) {
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;

    // Test with string that's too short
    const std::string short_data = "1234567812345678901234567890123456789012345678901234567890123456";
    EXPECT_EQ(short_data.length(), 64); // 2 chars short

    bool result = parse_ibeacon_device(short_data, factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_FALSE(result);
}

TEST_F(IBeaconParsingTest, ParseIBeaconDeviceEmpty) {
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;

    bool result = parse_ibeacon_device("", factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_FALSE(result);
}

TEST_F(IBeaconParsingTest, ParseIBeaconDeviceTooLong) {
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;

    // Test with string that's too long
    const std::string long_data = valid_ibeacon_data + "EXTRA";

    bool result = parse_ibeacon_device(long_data, factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_FALSE(result);
}

TEST_F(IBeaconParsingTest, ParseIBeaconDisabledDevice) {
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;

    // Test with disabled iBeacon (all zeros in factory, UUID, and P2 as per Ada spec)
    const std::string disabled_ibeacon = "00000000"                         // Factory ID (8 zeros)
                                         "00000000000000000000000000000000" // UUID (32 zeros)
                                         "0000"                             // Major (4 zeros)
                                         "0000"                             // Minor (4 zeros)
                                         "00"                               // Measured Power (2 zeros)
                                         "112233445566"                     // MAC (still valid)
                                         "-040";                            // RSSI (still valid)

    bool result = parse_ibeacon_device(disabled_ibeacon, factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_TRUE(result); // Parsing should succeed even for disabled beacons

    // Verify that disabled fields are zeros
    EXPECT_EQ(factory_id, "00000000");
    EXPECT_EQ(uuid, "00000000000000000000000000000000");
    EXPECT_EQ(major, "0000");
    EXPECT_EQ(minor, "0000");
    EXPECT_EQ(measured_power, "00");

    // MAC and RSSI should still be valid
    EXPECT_EQ(mac, "112233445566");
    EXPECT_EQ(rssi, "-040");
}

// ============================================================================
// Circular Buffer iBeacon Extraction Tests
// ============================================================================

class IBeaconCircularTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a test buffer with iBeacon data
        test_buffer = {
            // Some prefix data
            0x41,
            0x42,
            0x43,

            // iBeacon data (66 bytes)
            '1',
            '2',
            '3',
            '4',
            '5',
            '6',
            '7',
            '8', // Factory ID (8)
            'A',
            '1',
            'B',
            '2',
            'C',
            '3',
            'D',
            '4', // UUID start
            'E',
            '5',
            'F',
            '6',
            'A',
            '1',
            'B',
            '2',
            'C',
            '3',
            'D',
            '4',
            'E',
            '5',
            'F',
            '6',
            'A',
            '1',
            'B',
            '2',
            'C',
            '3',
            'D',
            '4', // UUID end (32)
            '0',
            '0',
            '0',
            '1', // Major (4)
            '0',
            '0',
            '0',
            '2', // Minor (4)
            '2',
            '5', // Measured Power (2)
            'A',
            'A',
            'B',
            'B',
            'C',
            'C',
            'D',
            'D',
            'E',
            'E',
            'F',
            'F', // MAC (12)
            '-',
            '0',
            '5',
            '5', // RSSI (4)

            // Some suffix data
            0x44,
            0x45,
            0x46};
    }

    std::array<uint8_t, 200> test_buffer;
};

TEST_F(IBeaconCircularTest, ExtractIBeaconDataBasic) {
    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH> device_data;

    // Extract iBeacon data starting at position 3 (after prefix)
    bool result = extract_ibeacon_data_circular(
        test_buffer.data(),
        test_buffer.size(),
        3, // Start position (after 3-byte prefix)
        device_data);

    EXPECT_TRUE(result);

    // Verify the extracted data matches expected iBeacon content
    const std::string_view extracted(device_data.data(), device_data.size());
    const std::string expected = "12345678A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D40001000225AABBCCDDEEFF-055";

    EXPECT_EQ(extracted, expected);
    EXPECT_EQ(extracted.length(), 66);
}

TEST_F(IBeaconCircularTest, ExtractIBeaconDataNullBuffer) {
    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH> device_data;

    // Test with null buffer
    bool result = extract_ibeacon_data_circular(nullptr, 100, 0, device_data);

    EXPECT_FALSE(result);
}

TEST_F(IBeaconCircularTest, ExtractIBeaconDataZeroSize) {
    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH> device_data;

    // Test with zero buffer size
    bool result = extract_ibeacon_data_circular(test_buffer.data(), 0, 0, device_data);

    EXPECT_FALSE(result);
}

TEST_F(IBeaconCircularTest, ExtractIBeaconDataCircularWrap) {
    // Create smaller buffer to test circular wrapping
    std::array<uint8_t, 80> small_buffer;

    // Fill with test data that will wrap around
    const std::string ibeacon_data = "87654321F1E2D3C4B5A6F1E2D3C4B5A6F1E2D3C4001000020C899887766554-065";
    EXPECT_EQ(ibeacon_data.length(), 66);

    // Place data starting near the end to force wrapping
    const std::size_t start_pos = 20;
    for (std::size_t i = 0; i < ibeacon_data.length(); ++i) {
        small_buffer[(start_pos + i) % small_buffer.size()] = static_cast<uint8_t>(ibeacon_data[i]);
    }

    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH> device_data;

    bool result = extract_ibeacon_data_circular(small_buffer.data(), small_buffer.size(), start_pos, device_data);

    EXPECT_TRUE(result);

    // Verify the extracted data matches original despite wrapping
    const std::string_view extracted(device_data.data(), device_data.size());
    EXPECT_EQ(extracted, ibeacon_data);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IBeaconIntegrationTest, ParseExtractedCircularData) {
    // Test the complete flow: extract from circular buffer, then parse fields

    // Create buffer with iBeacon data
    const std::string ibeacon_str = "DEADBEEF"                         // Factory ID
                                    "0F1E2D3C4B5A6F1E2D3C4B5A6F1E2D3C" // UUID
                                    "1234"                             // Major
                                    "5678"                             // Minor
                                    "FF"                               // Power
                                    "112233445566"                     // MAC
                                    "-080";                            // RSSI

    std::vector<uint8_t> buffer(ibeacon_str.begin(), ibeacon_str.end());

    // Extract using circular buffer function
    std::array<char, ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH> device_data;
    bool extract_result = extract_ibeacon_data_circular(buffer.data(), buffer.size(), 0, device_data);

    EXPECT_TRUE(extract_result);

    // Parse the extracted data
    std::string_view factory_id, uuid, major, minor, measured_power, mac, rssi;
    const std::string_view device_view(device_data.data(), device_data.size());

    bool parse_result = parse_ibeacon_device(device_view, factory_id, uuid, major, minor, measured_power, mac, rssi);

    EXPECT_TRUE(parse_result);

    // Verify all fields
    EXPECT_EQ(factory_id, "DEADBEEF");
    EXPECT_EQ(uuid, "0F1E2D3C4B5A6F1E2D3C4B5A6F1E2D3C");
    EXPECT_EQ(major, "1234");
    EXPECT_EQ(minor, "5678");
    EXPECT_EQ(measured_power, "FF");
    EXPECT_EQ(mac, "112233445566");
    EXPECT_EQ(rssi, "-080");
}

// ============================================================================
// Constants Validation Tests
// ============================================================================

TEST(IBeaconConstantsTest, ValidateDataLength) {
    // Verify that the iBeacon data length constant is correct
    // According to Ada: 8 + 32 + 10 + 12 + 4 = 66
    const std::size_t expected_length = 8 + 32 + 4 + 4 + 2 + 12 + 4; // 66
    EXPECT_EQ(ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH, expected_length);
    EXPECT_EQ(ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH, 66);
}

TEST(IBeaconConstantsTest, ValidatePrefixes) {
    // Verify that the iBeacon prefixes match Ada constants
    EXPECT_EQ(ibeacon_discovery_prefixes::OK_DISCS, "OK+DISCS");
    EXPECT_EQ(ibeacon_discovery_prefixes::OK_DISC, "OK+DISC");
    EXPECT_EQ(ibeacon_discovery_prefixes::OK_DISCE, "OK+DISCE");
}

// ============================================================================
// Characteristic Property Parsing Tests
// ============================================================================

class CharacteristicPropertiesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Set up various property string combinations for testing
    }
};

TEST_F(CharacteristicPropertiesTest, ParseAllProperties) {
    // Test parsing all properties enabled
    const std::string properties_str = "RD|WR|WN|NO|IN";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseReadWriteOnly) {
    // Test parsing with only Read and Write enabled
    const std::string properties_str = "RD|WR|--|--|--";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseNotifyOnly) {
    // Test parsing with only Notify enabled
    const std::string properties_str = "--|--|--|NO|--";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseIndicateOnly) {
    // Test parsing with only Indicate enabled
    const std::string properties_str = "--|--|--|--|IN";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseWriteWithoutResponseOnly) {
    // Test parsing with only Write Without Response enabled
    const std::string properties_str = "--|--|WN|--|--";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseNoProperties) {
    // Test parsing with no properties enabled (all dashes)
    const std::string properties_str = "--|--|--|--|--";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_TRUE(result.has_value());

    auto properties = *result;
    EXPECT_EQ(properties, CharacteristicProperty::None);
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicPropertiesTest, ParseInvalidLength) {
    // Test parsing with invalid length (too short)
    const std::string properties_str = "RD|WR";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_FALSE(result.has_value());
}

TEST_F(CharacteristicPropertiesTest, ParseInvalidLength_TooLong) {
    // Test parsing with invalid length (too long)
    const std::string properties_str = "RD|WR|WN|NO|IN|EXTRA";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_FALSE(result.has_value());
}

TEST_F(CharacteristicPropertiesTest, ParseInvalidSeparators) {
    // Test parsing with invalid separators
    const std::string properties_str = "RD:WR:WN:NO:IN";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_FALSE(result.has_value());
}

TEST_F(CharacteristicPropertiesTest, ParseMissingSeparators) {
    // Test parsing with missing separators
    const std::string properties_str = "RDWRWNNOINDX";
    auto result = parse_characteristic_properties(properties_str);

    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Characteristic UUID Component Parsing Tests
// ============================================================================

class CharacteristicUuidComponentsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Valid characteristic UUID string: handle:properties:uuid (4:14:4 + 2 colons = 26 chars)
        valid_characteristic_str = "0001:RD|WR|--|NO|--:1234";
        all_properties_str = "0002:RD|WR|WN|NO|IN:5678";
        no_properties_str = "0003:--|--|--|--|--:9ABC";
    }

    std::string valid_characteristic_str;
    std::string all_properties_str;
    std::string no_properties_str;
};

TEST_F(CharacteristicUuidComponentsTest, ParseValidCharacteristic) {
    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(valid_characteristic_str, handle, properties, uuid);

    EXPECT_TRUE(result);
    EXPECT_EQ(handle, "0001");
    EXPECT_EQ(uuid, "1234");

    // Verify properties
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicUuidComponentsTest, ParseAllProperties) {
    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(all_properties_str, handle, properties, uuid);

    EXPECT_TRUE(result);
    EXPECT_EQ(handle, "0002");
    EXPECT_EQ(uuid, "5678");

    // Verify all properties are set
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Indicate));
}

TEST_F(CharacteristicUuidComponentsTest, ParseNoProperties) {
    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(no_properties_str, handle, properties, uuid);

    EXPECT_TRUE(result);
    EXPECT_EQ(handle, "0003");
    EXPECT_EQ(uuid, "9ABC");

    // Verify no properties are set
    EXPECT_EQ(properties, CharacteristicProperty::None);
}

TEST_F(CharacteristicUuidComponentsTest, ParseInvalidLength) {
    // Test with wrong length (too short)
    const std::string invalid_str = "001:RD|WR|--|NO|--:123";

    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(invalid_str, handle, properties, uuid);

    EXPECT_FALSE(result);
}

TEST_F(CharacteristicUuidComponentsTest, ParseInvalidColonPositions) {
    // Test with colons in wrong positions
    const std::string invalid_str = "000:1RD|WR|--|NO|--:1234";

    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(invalid_str, handle, properties, uuid);

    EXPECT_FALSE(result);
}

TEST_F(CharacteristicUuidComponentsTest, ParseInvalidHexHandle) {
    // Test with non-hex characters in handle
    const std::string invalid_str = "000G:RD|WR|--|NO|--:1234";

    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(invalid_str, handle, properties, uuid);

    EXPECT_FALSE(result);
}

TEST_F(CharacteristicUuidComponentsTest, ParseInvalidHexUuid) {
    // Test with non-hex characters in UUID
    const std::string invalid_str = "0001:RD|WR|--|NO|--:123G";

    std::string_view handle;
    CharacteristicProperty properties;
    std::string_view uuid;

    bool result = parse_characteristic_uuid_components(invalid_str, handle, properties, uuid);

    EXPECT_FALSE(result);
}

// ============================================================================
// Characteristic Property Bitfield Operations Tests
// ============================================================================

TEST(CharacteristicPropertyBitfieldTest, BitwiseOrOperations) {
    auto combined = CharacteristicProperty::Read | CharacteristicProperty::Write;

    EXPECT_TRUE(has_property(combined, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(combined, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(combined, CharacteristicProperty::Notify));
}

TEST(CharacteristicPropertyBitfieldTest, BitwiseAndOperations) {
    auto combined = CharacteristicProperty::Read | CharacteristicProperty::Write;
    auto result = combined & CharacteristicProperty::Read;

    EXPECT_EQ(result, CharacteristicProperty::Read);

    auto no_notify = combined & CharacteristicProperty::Notify;
    EXPECT_EQ(no_notify, CharacteristicProperty::None);
}

TEST(CharacteristicPropertyBitfieldTest, CompoundAssignment) {
    CharacteristicProperty properties = CharacteristicProperty::None;

    properties |= CharacteristicProperty::Read;
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));

    properties |= CharacteristicProperty::Write;
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Write));
}

TEST(CharacteristicPropertyBitfieldTest, HasPropertyHelper) {
    auto properties = CharacteristicProperty::Read | CharacteristicProperty::Notify | CharacteristicProperty::Indicate;

    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Read));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::Write));
    EXPECT_FALSE(has_property(properties, CharacteristicProperty::WriteWithoutResponse));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Notify));
    EXPECT_TRUE(has_property(properties, CharacteristicProperty::Indicate));
}