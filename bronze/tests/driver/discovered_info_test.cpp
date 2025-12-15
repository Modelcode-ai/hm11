/**
 * @file discovered_info_test.cpp
 * @brief Unit tests for DiscoveredInfo structure
 *
 * This file contains tests for the DiscoveredInfo structure defined in driver.hpp.
 */

#include <gtest/gtest.h>

#include "hm11/driver.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for DiscoveredInfo tests
 */
class DiscoveredInfoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No specific setup needed
    }

    void TearDown() override {
        // No specific teardown needed
    }
};

// Test reset() function
TEST_F(DiscoveredInfoTest, ResetClearsAllFields) {
    DiscoveredInfo info;

    // Set some values
    info.id = 'A';
    info.mac = util::MacAddress("112233445566");
    std::copy_n("TestDevice", 10, info.name.begin());
    info.name_length = 10;
    std::copy_n("-50", 3, info.rssi.begin());
    info.rssi_length = 3;

    // Reset
    info.reset();

    // Verify all fields are cleared
    EXPECT_EQ(info.id, '\0');
    EXPECT_EQ(info.mac, util::MacAddress("000000000000"));
    EXPECT_EQ(info.name_length, 0);
    EXPECT_EQ(info.rssi_length, 0);
}

// Test get_name_view() function
TEST_F(DiscoveredInfoTest, GetNameViewReturnsCorrectView) {
    DiscoveredInfo info;

    // Set name
    const char* test_name = "MyDevice";
    std::copy_n(test_name, 8, info.name.begin());
    info.name_length = 8;

    // Get view
    auto view = info.get_name_view();

    EXPECT_EQ(view.size(), 8);
    EXPECT_EQ(view, "MyDevice");
}

// Test get_name_view() with empty name
TEST_F(DiscoveredInfoTest, GetNameViewEmptyName) {
    DiscoveredInfo info;

    // Name is empty by default
    info.name_length = 0;

    // Get view
    auto view = info.get_name_view();

    EXPECT_EQ(view.size(), 0);
    EXPECT_TRUE(view.empty());
}

// Test get_name_view() with maximum length name
TEST_F(DiscoveredInfoTest, GetNameViewMaxLength) {
    DiscoveredInfo info;

    // Fill name with maximum length
    std::fill(info.name.begin(), info.name.end(), 'X');
    info.name_length = MAX_NAME_LENGTH;

    // Get view
    auto view = info.get_name_view();

    EXPECT_EQ(view.size(), MAX_NAME_LENGTH);
    EXPECT_EQ(view[0], 'X');
    EXPECT_EQ(view[MAX_NAME_LENGTH - 1], 'X');
}

// Test get_rssi_view() function
TEST_F(DiscoveredInfoTest, GetRssiViewReturnsCorrectView) {
    DiscoveredInfo info;

    // Set RSSI
    const char* test_rssi = "-50";
    std::copy_n(test_rssi, 3, info.rssi.begin());
    info.rssi_length = 3;

    // Get view
    auto view = info.get_rssi_view();

    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view, "-50");
}

// Test get_rssi_view() with empty RSSI
TEST_F(DiscoveredInfoTest, GetRssiViewEmptyRssi) {
    DiscoveredInfo info;

    // RSSI is empty by default
    info.rssi_length = 0;

    // Get view
    auto view = info.get_rssi_view();

    EXPECT_EQ(view.size(), 0);
    EXPECT_TRUE(view.empty());
}

// Test get_rssi_view() with maximum length RSSI
TEST_F(DiscoveredInfoTest, GetRssiViewMaxLength) {
    DiscoveredInfo info;

    // Fill RSSI with maximum length
    std::fill(info.rssi.begin(), info.rssi.end(), '-');
    info.rssi_length = MAX_RSII_LENGTH;

    // Get view
    auto view = info.get_rssi_view();

    EXPECT_EQ(view.size(), MAX_RSII_LENGTH);
    EXPECT_EQ(view[0], '-');
}

// Test default initialization
TEST_F(DiscoveredInfoTest, DefaultInitialization) {
    DiscoveredInfo info;

    // Verify default values
    EXPECT_EQ(info.id, '\0');
    EXPECT_EQ(info.name_length, 0);
    EXPECT_EQ(info.rssi_length, 0);
}

// Test setting and getting MAC address
TEST_F(DiscoveredInfoTest, MacAddressSetting) {
    DiscoveredInfo info;

    // Set MAC address
    info.mac = util::MacAddress("AABBCCDDEEFF");

    // Verify
    EXPECT_EQ(info.mac, util::MacAddress("AABBCCDDEEFF"));
}

// Test setting and getting ID
TEST_F(DiscoveredInfoTest, IdSetting) {
    DiscoveredInfo info;

    // Set ID
    info.id = 'F';

    // Verify
    EXPECT_EQ(info.id, 'F');

    // Set different ID
    info.id = 'B';
    EXPECT_EQ(info.id, 'B');
}

// Test name buffer manipulation
TEST_F(DiscoveredInfoTest, NameBufferManipulation) {
    DiscoveredInfo info;

    // Write to name buffer
    const char* name1 = "Device1";
    std::copy_n(name1, 7, info.name.begin());
    info.name_length = 7;

    EXPECT_EQ(info.get_name_view(), "Device1");

    // Overwrite with different name
    const char* name2 = "NewDeviceName";
    std::copy_n(name2, 13, info.name.begin());
    info.name_length = 13;

    EXPECT_EQ(info.get_name_view(), "NewDeviceName");
}

// Test RSSI buffer manipulation
TEST_F(DiscoveredInfoTest, RssiBufferManipulation) {
    DiscoveredInfo info;

    // Write to RSSI buffer
    const char* rssi1 = "-45";
    std::copy_n(rssi1, 3, info.rssi.begin());
    info.rssi_length = 3;

    EXPECT_EQ(info.get_rssi_view(), "-45");

    // Overwrite with different RSSI
    const char* rssi2 = "-80";
    std::copy_n(rssi2, 3, info.rssi.begin());
    info.rssi_length = 3;

    EXPECT_EQ(info.get_rssi_view(), "-80");
}

// Test reset after setting all fields
TEST_F(DiscoveredInfoTest, ResetAfterFullyPopulated) {
    DiscoveredInfo info;

    // Populate all fields
    info.id = 'T';
    info.mac = util::MacAddress("123456789ABC");
    const char* name = "FullDevice";
    std::copy_n(name, 10, info.name.begin());
    info.name_length = 10;
    const char* rssi = "-60";
    std::copy_n(rssi, 3, info.rssi.begin());
    info.rssi_length = 3;

    // Verify populated
    EXPECT_EQ(info.id, 'T');
    EXPECT_EQ(info.get_name_view(), "FullDevice");
    EXPECT_EQ(info.get_rssi_view(), "-60");

    // Reset
    info.reset();

    // Verify cleared
    EXPECT_EQ(info.id, '\0');
    EXPECT_EQ(info.name_length, 0);
    EXPECT_EQ(info.rssi_length, 0);
    EXPECT_EQ(info.get_name_view(), "");
    EXPECT_EQ(info.get_rssi_view(), "");
}

} // namespace hm11::tests
