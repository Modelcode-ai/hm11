#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hm11/streaming_parser.hpp"

using namespace hm11;

// Helper to convert string to uint8_t*
std::vector<std::uint8_t> to_bytes(const std::string& str) {
    return {str.begin(), str.end()};
}

// Helper callback for tests
static void collect_device(const DiscoveredDevice& dev, void* user_data) {
    auto* devices = static_cast<std::vector<DiscoveredDevice>*>(user_data);
    devices->push_back(dev);
}

// ========== Single Device Tests ==========

TEST(StreamingParserTest, SingleDeviceComplete) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    std::string response =
        "OK+DISCS"
        "OK+DISC:001122334455"
        "OK+NAME:TestDevice\r\n"
        "OK+RSSI:-65\r\n"
        "OK+DISCE";

    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_TRUE(complete);
    EXPECT_TRUE(parser.is_complete());
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name_view(), "TestDevice");
    EXPECT_EQ(devices[0].rssi_view(), "-65");
}

TEST(StreamingParserTest, SingleDeviceFragmented) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Fragment 1: Start + MAC prefix
    std::string frag1 = "OK+DISCSOK+DISC:00112233";
    auto bytes1 = to_bytes(frag1);
    EXPECT_FALSE(parser.parse(bytes1.data(), bytes1.size()));

    // Fragment 2: MAC suffix + NAME prefix
    std::string frag2 = "4455OK+NAME:Test";
    auto bytes2 = to_bytes(frag2);
    EXPECT_FALSE(parser.parse(bytes2.data(), bytes2.size()));

    // Fragment 3: NAME suffix + RSSI + END
    std::string frag3 = "Device\r\nOK+RSSI:-65\r\nOK+DISCE";
    auto bytes3 = to_bytes(frag3);
    EXPECT_TRUE(parser.parse(bytes3.data(), bytes3.size()));

    EXPECT_TRUE(parser.is_complete());
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name_view(), "TestDevice");
    EXPECT_EQ(devices[0].rssi_view(), "-65");
}

TEST(StreamingParserTest, DeviceWithLongName) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    std::string response =
        "OK+DISCS"
        "OK+DISC:AABBCCDDEEFF"
        "OK+NAME:VeryLongDeviceNameThatSpansMultipleBytes\r\n"
        "OK+RSSI:-72\r\n"
        "OK+DISCE";

    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_TRUE(complete);
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "AABBCCDDEEFF");
    EXPECT_EQ(devices[0].name_view(), "VeryLongDeviceNameThatSpansMultipleBytes");
    EXPECT_EQ(devices[0].rssi_view(), "-72");
}

TEST(StreamingParserTest, DeviceWithNoName) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    std::string response =
        "OK+DISCS"
        "OK+DISC:112233445566"
        "OK+NAME:\r\n"  // Empty name
        "OK+RSSI:-80\r\n"
        "OK+DISCE";

    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_TRUE(complete);
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "112233445566");
    EXPECT_EQ(devices[0].name_view(), "");
    EXPECT_EQ(devices[0].rssi_view(), "-80");
}

// ========== Multiple Devices Tests ==========

TEST(StreamingParserTest, MultipleDevices) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    std::string response =
        "OK+DISCS"
        "OK+DISC:001122334455"
        "OK+NAME:Device1\r\n"
        "OK+RSSI:-65\r\n"
        "OK+DISC:AABBCCDDEEFF"
        "OK+NAME:Device2\r\n"
        "OK+RSSI:-70\r\n"
        "OK+DISC:112233AABBCC"
        "OK+NAME:Device3\r\n"
        "OK+RSSI:-75\r\n"
        "OK+DISCE";

    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_TRUE(complete);
    EXPECT_EQ(devices.size(), 3);

    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name_view(), "Device1");
    EXPECT_EQ(devices[0].rssi_view(), "-65");

    EXPECT_EQ(std::string(devices[1].mac.data(), 12), "AABBCCDDEEFF");
    EXPECT_EQ(devices[1].name_view(), "Device2");
    EXPECT_EQ(devices[1].rssi_view(), "-70");

    EXPECT_EQ(std::string(devices[2].mac.data(), 12), "112233AABBCC");
    EXPECT_EQ(devices[2].name_view(), "Device3");
    EXPECT_EQ(devices[2].rssi_view(), "-75");
}

TEST(StreamingParserTest, MultipleDevicesFragmented) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Send in small fragments
    std::string response =
        "OK+DISCS"
        "OK+DISC:001122334455"
        "OK+NAME:Dev1\r\n"
        "OK+RSSI:-65\r\n"
        "OK+DISC:AABBCCDDEEFF"
        "OK+NAME:Dev2\r\n"
        "OK+RSSI:-70\r\n"
        "OK+DISCE";

    // Fragment every 10 bytes
    for (std::size_t i = 0; i < response.size(); i += 10) {
        std::size_t len = std::min(std::size_t(10), response.size() - i);
        auto bytes = to_bytes(response.substr(i, len));
        parser.parse(bytes.data(), bytes.size());
    }

    EXPECT_TRUE(parser.is_complete());
    EXPECT_EQ(devices.size(), 2);
}

// ========== Edge Cases Tests ==========

TEST(StreamingParserTest, EmptyScan) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    std::string response = "OK+DISCSOK+DISCE";
    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_TRUE(complete);
    EXPECT_EQ(devices.size(), 0);
}

TEST(StreamingParserTest, IncompleteData) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Send incomplete MAC
    std::string response = "OK+DISCSOK+DISC:00112233";
    auto bytes = to_bytes(response);
    bool complete = parser.parse(bytes.data(), bytes.size());

    EXPECT_FALSE(complete);
    EXPECT_FALSE(parser.is_complete());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::MAC);
    EXPECT_EQ(devices.size(), 0);
}

TEST(StreamingParserTest, NameSplitAcrossMultipleReads) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Fragment 1: MAC + start of name
    std::string frag1 = "OK+DISCSOK+DISC:001122334455OK+NAME:Very";
    auto bytes1 = to_bytes(frag1);
    EXPECT_FALSE(parser.parse(bytes1.data(), bytes1.size()));

    // Fragment 2: middle of name
    std::string frag2 = "Long";
    auto bytes2 = to_bytes(frag2);
    EXPECT_FALSE(parser.parse(bytes2.data(), bytes2.size()));

    // Fragment 3: end of name + RSSI + END
    std::string frag3 = "Name\r\nOK+RSSI:-50\r\nOK+DISCE";
    auto bytes3 = to_bytes(frag3);
    EXPECT_TRUE(parser.parse(bytes3.data(), bytes3.size()));

    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(devices[0].name_view(), "VeryLongName");
}

TEST(StreamingParserTest, ResetParser) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Parse first scan
    std::string response1 = "OK+DISCSOK+DISC:001122334455OK+NAME:Dev1\r\nOK+RSSI:-65\r\nOK+DISCE";
    auto bytes1 = to_bytes(response1);
    parser.parse(bytes1.data(), bytes1.size());
    EXPECT_EQ(devices.size(), 1);

    // Reset and parse second scan
    parser.reset();
    devices.clear();

    std::string response2 = "OK+DISCSOK+DISC:AABBCCDDEEFF OK+NAME:Dev2\r\nOK+RSSI:-70\r\nOK+DISCE";
    auto bytes2 = to_bytes(response2);
    parser.parse(bytes2.data(), bytes2.size());

    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "AABBCCDDEEFF");
}

TEST(StreamingParserTest, FlushPendingDevice) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    // Send device without terminating scan
    std::string response = "OK+DISCSOK+DISC:001122334455OK+NAME:Dev1\r\nOK+RSSI:-65\r\n";
    auto bytes = to_bytes(response);
    parser.parse(bytes.data(), bytes.size());

    EXPECT_EQ(devices.size(), 0);  // Not called yet

    // Flush should invoke callback
    parser.flush();
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
}

// ========== State Machine Tests ==========

TEST(StreamingParserTest, StateTransitions) {
    std::vector<DiscoveredDevice> devices;
    StreamingParser parser(collect_device, &devices);

    EXPECT_EQ(parser.get_state(), StreamingParser::State::Selection);

    // After OK+DISC:, should transition to MAC state
    std::string frag1 = "OK+DISCSOK+DISC:001122";
    auto bytes1 = to_bytes(frag1);
    parser.parse(bytes1.data(), bytes1.size());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::MAC);

    // After complete MAC, should return to Selection
    std::string frag2 = "334455";
    auto bytes2 = to_bytes(frag2);
    parser.parse(bytes2.data(), bytes2.size());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::Selection);

    // After OK+NAME:, should transition to Name state
    std::string frag3 = "OK+NAME:Test";
    auto bytes3 = to_bytes(frag3);
    parser.parse(bytes3.data(), bytes3.size());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::Name);

    // After \r\n, should return to Selection
    std::string frag4 = "Device\r\n";
    auto bytes4 = to_bytes(frag4);
    parser.parse(bytes4.data(), bytes4.size());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::Selection);

    // After OK+DISCE, should be Complete
    std::string frag5 = "OK+RSSI:-65\r\nOK+DISCE";
    auto bytes5 = to_bytes(frag5);
    parser.parse(bytes5.data(), bytes5.size());
    EXPECT_EQ(parser.get_state(), StreamingParser::State::Complete);
}
