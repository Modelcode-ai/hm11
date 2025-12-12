#include <gtest/gtest.h>

#include "hm11/ATResponseParser.hpp"

using namespace hm11;

TEST(ATResponseParserTest, ExtractUintSuccess) {
    unsigned int value = 0;
    EXPECT_EQ(extract_uint("12345", value), ParseResult::Ok);
    EXPECT_EQ(value, 12345U);
}

TEST(ATResponseParserTest, ExtractUintInvalid) {
    unsigned int value = 0;
    EXPECT_EQ(extract_uint("12a34", value), ParseResult::Invalid);
}

TEST(ATResponseParserTest, ExtractHexSuccess) {
    unsigned int value = 0;
    EXPECT_EQ(extract_hex("1A2b", value), ParseResult::Ok);
    EXPECT_EQ(value, 0x1A2BU);
}

TEST(ATResponseParserTest, ExtractHexInvalid) {
    unsigned int value = 0;
    EXPECT_EQ(extract_hex("1G2", value), ParseResult::Invalid);
}

TEST(ATResponseParserTest, SplitKeyValue) {
    std::string_view key;
    std::string_view val;
    EXPECT_EQ(split_key_value("+SET:123", ':', key, val), ParseResult::Ok);

    EXPECT_EQ(key, "+SET");
    EXPECT_EQ(val, "123");
}

TEST(ATResponseParserTest, ParseUUID) {
    std::array<uint8_t, 16> uuid{};
    EXPECT_EQ(parse_uuid("E2C56DB5-DFFB-48D2-B060-D0F5A71096E0", uuid), ParseResult::Ok);
    // Check first byte
    EXPECT_EQ(uuid[0], 0xE2);
}

TEST(ATResponseParserTest, ParseMAC) {
    std::array<uint8_t, 6> mac{};
    EXPECT_EQ(parse_mac("A1:B2:C3:D4:E5:F6", mac), ParseResult::Ok);
    EXPECT_EQ(mac[0], 0xA1);
    EXPECT_EQ(mac[5], 0xF6);
}

TEST(ATResponseParserTest, ParseSensorReadings) {
    uint8_t temp = 0;
    uint8_t hum = 0;
    EXPECT_EQ(parse_sensor_readings("25,60", temp, hum), ParseResult::Ok);
    EXPECT_EQ(temp, 25);
    EXPECT_EQ(hum, 60);
}
