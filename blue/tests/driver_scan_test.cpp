#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Helper Structures ==========

struct ScannedDevice {
    std::array<char, 12> mac;
    std::string name;
    std::string rssi;

    ScannedDevice(const std::array<char, 12>& m, const std::string& n, const std::string& r)
        : mac(m), name(n), rssi(r) {}
};

// ========== Scan Tests ==========

TEST(HM11DriverScanTest, ScanSingleDevice) {
    MockUARTPort uart;

    // Set up scan response with one device
    std::string response =
        "OK+DISCS"
        "OK+DISC:001122334455"
        "OK+NAME:TestDevice\r\n"
        "OK+RSSI:-65\r\n"
        "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Collect discovered devices
    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    // Execute scan
    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify transmitted command
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISC?\r\n");

    // Verify discovered devices
    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name, "TestDevice");
    EXPECT_EQ(devices[0].rssi, "-65");
}

TEST(HM11DriverScanTest, ScanMultipleDevices) {
    MockUARTPort uart;

    // Set up scan response with three devices
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
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify three devices discovered
    ASSERT_EQ(devices.size(), 3);

    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name, "Device1");
    EXPECT_EQ(devices[0].rssi, "-65");

    EXPECT_EQ(std::string(devices[1].mac.data(), 12), "AABBCCDDEEFF");
    EXPECT_EQ(devices[1].name, "Device2");
    EXPECT_EQ(devices[1].rssi, "-70");

    EXPECT_EQ(std::string(devices[2].mac.data(), 12), "112233AABBCC");
    EXPECT_EQ(devices[2].name, "Device3");
    EXPECT_EQ(devices[2].rssi, "-75");
}

TEST(HM11DriverScanTest, ScanFragmentedResponse) {
    MockUARTPort uart;

    // Set up fragmented scan response (simulates multiple UART reads)
    uart.add_receive_fragment("OK+DISCS");
    uart.add_receive_fragment("OK+DISC:001122");
    uart.add_receive_fragment("334455");
    uart.add_receive_fragment("OK+NAME:Test");
    uart.add_receive_fragment("Device\r\n");
    uart.add_receive_fragment("OK+RSSI:-65");
    uart.add_receive_fragment("\r\n");
    uart.add_receive_fragment("OK+DISCE");

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify device was correctly assembled from fragments
    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "001122334455");
    EXPECT_EQ(devices[0].name, "TestDevice");
    EXPECT_EQ(devices[0].rssi, "-65");
}

TEST(HM11DriverScanTest, ScanEmptyResults) {
    MockUARTPort uart;

    // Set up scan response with no devices
    std::string response = "OK+DISCSOK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify no devices discovered
    EXPECT_EQ(devices.size(), 0);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISC?\r\n");
}

TEST(HM11DriverScanTest, ScanDeviceWithLongName) {
    MockUARTPort uart;

    // Device with a very long name
    std::string response =
        "OK+DISCS"
        "OK+DISC:AABBCCDDEEFF"
        "OK+NAME:VeryLongDeviceNameThatCouldSpanMultipleBytesAndTestBuffering\r\n"
        "OK+RSSI:-72\r\n"
        "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "AABBCCDDEEFF");
    EXPECT_EQ(devices[0].name, "VeryLongDeviceNameThatCouldSpanMultipleBytesAndTestBuffering");
    EXPECT_EQ(devices[0].rssi, "-72");
}

TEST(HM11DriverScanTest, ScanDeviceWithEmptyName) {
    MockUARTPort uart;

    // Device with empty name
    std::string response =
        "OK+DISCS"
        "OK+DISC:112233445566"
        "OK+NAME:\r\n"
        "OK+RSSI:-80\r\n"
        "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(std::string(devices[0].mac.data(), 12), "112233445566");
    EXPECT_EQ(devices[0].name, "");
    EXPECT_EQ(devices[0].rssi, "-80");
}

TEST(HM11DriverScanTest, ScanTransmitError) {
    MockUARTPort uart;
    uart.set_transmit_error(true);  // Simulate UART transmit error

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_FALSE(driver.scan(callback, status, 5000));
    EXPECT_NE(status, Status::Ok);
    EXPECT_EQ(devices.size(), 0);
}

TEST(HM11DriverScanTest, ScanCallbackInvocation) {
    MockUARTPort uart;

    // Set up scan with two devices
    std::string response =
        "OK+DISCS"
        "OK+DISC:001122334455"
        "OK+NAME:First\r\n"
        "OK+RSSI:-60\r\n"
        "OK+DISC:AABBCCDDEEFF"
        "OK+NAME:Second\r\n"
        "OK+RSSI:-70\r\n"
        "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Track callback invocation order
    std::vector<std::string> invocation_order;
    auto callback = [&invocation_order](const std::array<char, 12>& mac,
                                        const std::string& name,
                                        const std::string& rssi) {
        invocation_order.push_back(name);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));

    // Verify callbacks were invoked in order
    ASSERT_EQ(invocation_order.size(), 2);
    EXPECT_EQ(invocation_order[0], "First");
    EXPECT_EQ(invocation_order[1], "Second");
}

TEST(HM11DriverScanTest, ScanRSSIVariations) {
    MockUARTPort uart;

    // Test various RSSI value formats
    std::string response =
        "OK+DISCS"
        "OK+DISC:111111111111"
        "OK+NAME:Strong\r\n"
        "OK+RSSI:-30\r\n"
        "OK+DISC:222222222222"
        "OK+NAME:Weak\r\n"
        "OK+RSSI:-95\r\n"
        "OK+DISC:333333333333"
        "OK+NAME:Medium\r\n"
        "OK+RSSI:-67\r\n"
        "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<ScannedDevice> devices;
    auto callback = [&devices](const std::array<char, 12>& mac, const std::string& name, const std::string& rssi) {
        devices.emplace_back(mac, name, rssi);
    };

    EXPECT_TRUE(driver.scan(callback, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    ASSERT_EQ(devices.size(), 3);
    EXPECT_EQ(devices[0].rssi, "-30");
    EXPECT_EQ(devices[1].rssi, "-95");
    EXPECT_EQ(devices[2].rssi, "-67");
}
