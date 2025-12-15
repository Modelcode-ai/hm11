#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "mock_uart_port.hpp"

using namespace hm11;

// ========== Helper Functions ==========

// iBeacon data format: FactoryID(8) + UUID(32) + MajorMinorPower(10) + MAC(12) + RSSI(4) = 66 bytes
std::string make_ibeacon_data(
    const std::string& factory_id,   // 8 chars
    const std::string& uuid,         // 32 chars
    const std::string& major_minor_power,  // 10 chars
    const std::string& mac,          // 12 chars
    const std::string& rssi) {       // 4 chars
    return factory_id + uuid + major_minor_power + mac + rssi;
}

// C-style callback for collecting iBeacon data
static void ibeacon_callback(std::string_view device_data, void* user_data) {
    auto* devices = static_cast<std::vector<std::string>*>(user_data);
    devices->push_back(std::string(device_data));
}

// C-style callback for extracting factory IDs (first 8 chars)
static void factory_id_callback(std::string_view device_data, void* user_data) {
    auto* ids = static_cast<std::vector<std::string>*>(user_data);
    ids->push_back(std::string(device_data.substr(0, 8)));
}

// ========== Scan iBeacon Tests ==========

TEST(HM11DriverScaniBeaconTest, ScanSingleiBeacon) {
    MockUARTPort uart;

    // Create test iBeacon data
    std::string ibeacon_data = make_ibeacon_data(
        "4C000215",  // Factory ID (8 chars)
        "74278BDAB6444520" + std::string("8F0C720EAF059935"),  // UUID (32 chars)
        "FFE0FFE1C5",  // Major (FFE0) + Minor (FFE1) + Power (C5) = 10 chars
        "001122334455",  // MAC (12 chars)
        "-065"  // RSSI (4 chars)
    );

    std::string response = "OK+DISCSOK+DISC" + ibeacon_data + "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Collect discovered iBeacons
    std::vector<std::string> devices;

    // Execute scan
    EXPECT_TRUE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify transmitted command
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISI?\r\n");

    // Verify discovered device
    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(devices[0], ibeacon_data);
}

TEST(HM11DriverScaniBeaconTest, ScanMultipleiBeacons) {
    MockUARTPort uart;

    // Create multiple test iBeacon data
    std::string ibeacon1 = make_ibeacon_data(
        "4C000215",
        "74278BDAB64445208F0C720EAF059935",
        "FFE0FFE1C5",
        "001122334455",
        "-065"
    );

    std::string ibeacon2 = make_ibeacon_data(
        "4C000216",
        "AABBCCDDEEFF00112233445566778899",
        "00010002C3",
        "AABBCCDDEEFF",
        "-070"
    );

    std::string ibeacon3 = make_ibeacon_data(
        "4C000217",
        "11223344556677889900AABBCCDDEEFF",
        "12341234C7",
        "112233AABBCC",
        "-075"
    );

    std::string response = "OK+DISCS" +
                          std::string("OK+DISC") + ibeacon1 +
                          std::string("OK+DISC") + ibeacon2 +
                          std::string("OK+DISC") + ibeacon3 +
                          "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    EXPECT_TRUE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify three devices discovered
    ASSERT_EQ(devices.size(), 3);
    EXPECT_EQ(devices[0], ibeacon1);
    EXPECT_EQ(devices[1], ibeacon2);
    EXPECT_EQ(devices[2], ibeacon3);
}

TEST(HM11DriverScaniBeaconTest, ScanFragmentedResponse) {
    MockUARTPort uart;

    // Create test iBeacon data
    std::string ibeacon_data = make_ibeacon_data(
        "4C000215",
        "74278BDAB64445208F0C720EAF059935",
        "FFE0FFE1C5",
        "001122334455",
        "-065"
    );

    // Fragment the response across multiple receive calls
    uart.add_receive_fragment("OK+DISCS");
    uart.add_receive_fragment("OK+DISC" + ibeacon_data.substr(0, 20));
    uart.add_receive_fragment(ibeacon_data.substr(20, 30));
    uart.add_receive_fragment(ibeacon_data.substr(50));
    uart.add_receive_fragment("OK+DISCE");

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    EXPECT_TRUE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify device was correctly assembled from fragments
    ASSERT_EQ(devices.size(), 1);
    EXPECT_EQ(devices[0], ibeacon_data);
}

TEST(HM11DriverScaniBeaconTest, ScanEmptyResults) {
    MockUARTPort uart;

    // Set up scan response with no devices
    std::string response = "OK+DISCSOK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    EXPECT_TRUE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 5000));
    EXPECT_EQ(status, Status::Ok);

    // Verify no devices discovered
    EXPECT_EQ(devices.size(), 0);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISI?\r\n");
}

TEST(HM11DriverScaniBeaconTest, ReceiveBasic) {
    MockUARTPort uart;
    uart.set_next_receive("TEST_DATA");

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::string_view received;
    bool result = driver.receive(received, status, 256);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(received, "TEST_DATA");
}

TEST(HM11DriverScaniBeaconTest, TransmitThenReceive) {
    MockUARTPort uart;
    uart.set_next_receive("OK+DISCSOK+DISCE");

    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Transmit command (like scan_ibeacon does)
    std::string cmd = "AT+DISI?\r\n";
    driver.transmit(cmd, status);
    EXPECT_EQ(status, Status::Ok);

    // Then receive
    std::string_view received;
    bool result = driver.receive(received, status, 512);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(received, "OK+DISCSOK+DISCE");
}

TEST(HM11DriverScaniBeaconTest, CheckATCommand) {
    // Verify the AT command builder creates the correct command
    auto cmd = ATCommandBuilder::build(AtCommand::ScaniBeacon, {});
    EXPECT_EQ(cmd.finalize(), "AT+DISI?\r\n");
}

TEST(HM11DriverScaniBeaconTest, ScanTransmitOnly) {
    MockUARTPort uart;
    // No receive data set up

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    // This should timeout quickly since no data
    EXPECT_FALSE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 100));  // Short timeout
    EXPECT_EQ(status, Status::Timeout);

    // But transmit should have happened
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISI?\r\n");
}

TEST(HM11DriverScaniBeaconTest, ScanEmptyResponse) {
    MockUARTPort uart;
    uart.set_next_receive("OK+DISCSOK+DISCE");  // Just start and end markers

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    EXPECT_TRUE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 500));
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(devices.size(), 0);
    EXPECT_EQ(uart.get_transmitted_data(), "AT+DISI?\r\n");
}

TEST(HM11DriverScaniBeaconTest, ScanTransmitError) {
    MockUARTPort uart;
    uart.set_transmit_error(true);  // Simulate UART transmit error

    HM11Driver driver(uart);
    Status status = Status::Ok;

    std::vector<std::string> devices;

    EXPECT_FALSE(driver.scan_ibeacon(ibeacon_callback, &devices, status, 5000));
    EXPECT_NE(status, Status::Ok);
    EXPECT_EQ(devices.size(), 0);
}

TEST(HM11DriverScaniBeaconTest, ScanCallbackInvocation) {
    MockUARTPort uart;

    // Create two test iBeacons
    std::string ibeacon1 = make_ibeacon_data(
        "4C000215",
        "74278BDAB64445208F0C720EAF059935",
        "FFE0FFE1C5",
        "001122334455",
        "-060"
    );

    std::string ibeacon2 = make_ibeacon_data(
        "4C000216",
        "AABBCCDDEEFF00112233445566778899",
        "00010002C3",
        "AABBCCDDEEFF",
        "-070"
    );

    std::string response = "OK+DISCSOK+DISC" + ibeacon1 + "OK+DISC" + ibeacon2 + "OK+DISCE";
    uart.set_next_receive(response);

    HM11Driver driver(uart);
    Status status = Status::Ok;

    // Track callback invocation order
    std::vector<std::string> invocation_order;

    EXPECT_TRUE(driver.scan_ibeacon(factory_id_callback, &invocation_order, status, 5000));

    // Verify callbacks were invoked in order
    ASSERT_EQ(invocation_order.size(), 2);
    EXPECT_EQ(invocation_order[0], "4C000215");
    EXPECT_EQ(invocation_order[1], "4C000216");
}
