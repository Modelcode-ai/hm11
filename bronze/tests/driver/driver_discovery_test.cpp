/**
 * @file driver_discovery_test.cpp
 * @brief Tests for HM11Driver device discovery functionality
 *
 * This file contains tests for the device discovery functionality of the HM11Driver
 * class, including the Scan and Scan_iBeacon methods.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "hm11/driver.hpp"
#include "hm11/enum_types.hpp"
#include "hm11/hal/uart.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for device discovery tests
 */
class HM11DriverDiscoveryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create and configure the mock UART
        uart_mock_ = std::make_shared<mocks::EnhancedUARTMock>();

        // Set up the mock receive handler
        mocks::MockReceiveHandler::SetInstance(&receive_handler_);

        // Set up the mock last read position handler
        mocks::MockLastReadPositionHandler::SetInstance(&last_read_handler_);

        // Set default behavior for the receive handler
        ON_CALL(
            receive_handler_,
            call(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::Invoke([this](
                                      hal::AnyUartPort port [[maybe_unused]],
                                      void* received,
                                      std::size_t length,
                                      hal::UartStatus& status,
                                      std::chrono::milliseconds timeout_ms,
                                      bool as_stream [[maybe_unused]]) {
                    // Default implementation that delegates to the UART mock
                    hal::UartData8b rx_data(static_cast<hal::UInt8*>(received), length);
                    uart_mock_->receive(rx_data, status, timeout_ms);
                }));

        // Create the driver instance
        driver_ = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
            uart_mock_,
            mocks::MockReceiveHandler::ReceiveWrapper,
            mocks::MockLastReadPositionHandler::LastReadPositionWrapper);
    }

    void TearDown() override {
        // Clean up the driver
        driver_.reset();

        // Clean up the mock handlers
        mocks::MockReceiveHandler::SetInstance(nullptr);
        mocks::MockLastReadPositionHandler::SetInstance(nullptr);
    }

    // Helper method to queue a response in the UART mock
    void QueueResponse(const std::string& response) { uart_mock_->queue_receive_data(response); }

    // Helper method to verify transmitted data contains a specific string
    bool VerifyTransmittedData(const std::string& expected) {
        const auto& data = uart_mock_->transmitted_data_8b();
        std::string transmitted(data.begin(), data.end());
        return transmitted.find(expected) != std::string::npos;
    }

    std::shared_ptr<mocks::EnhancedUARTMock> uart_mock_;
    std::unique_ptr<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>> driver_;
    ::testing::NiceMock<mocks::MockReceiveHandler> receive_handler_;
    ::testing::NiceMock<mocks::MockLastReadPositionHandler> last_read_handler_;

    // Vectors to store callback results
    std::vector<std::string> mac_addresses_;
    std::vector<std::string> names_;
    std::vector<std::string> rssi_values_;
    std::vector<std::string> ibeacon_devices_;

    // Callback functions for discovery
    DiscoveryCallback discovery_callback = [](char id,
                                              std::string_view mac_address,
                                              std::string_view name,
                                              std::string_view rssi,
                                              void* user_data) -> void {
        auto test = reinterpret_cast<HM11DriverDiscoveryTest*>(user_data);
        if (test) {
            test->mac_addresses_.emplace_back(mac_address);
            test->names_.emplace_back(name);
            test->rssi_values_.emplace_back(rssi);
        }
    };

    IBeaconDiscoveryCallback ibeacon_callback = [](std::string_view device, void* user_data) -> void {
        auto test = reinterpret_cast<HM11DriverDiscoveryTest*>(user_data);
        if (test) {
            test->ibeacon_devices_.emplace_back(device);
        }
    };
};

// Test the scan method with a single device
TEST_F(HM11DriverDiscoveryTest, ScanSingleDevice) {
    // Set up the last read position handler to indicate stream closed after receiving data
    bool stream_closed_called = false;
    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](bool& closed, std::size_t& zero) {
            closed = false;
            zero = 100; // Some position after the response data
        }))
        .WillOnce(::testing::Invoke([&stream_closed_called](bool& closed, std::size_t& zero) {
            closed = true; // Indicate stream closed
            zero = 100;
            stream_closed_called = true;
        }));

    // Queue responses for the scan command
    std::string scan_response = "OK+DISCS\r\n";
    scan_response += "OK+DISC:112233445566\r\n";
    scan_response += "OK+NAME:TestDevice\r\n";
    scan_response += "OK+RSSI:-75\r\n";
    scan_response += "OK+DISCE\r\n";

    QueueResponse(scan_response);

    // Call the scan method
    hal::UartStatus status;
    driver_->scan(discovery_callback, std::chrono::milliseconds(1000), status);

    // Verify that the scan command was transmitted
    EXPECT_TRUE(VerifyTransmittedData("AT+DISC?"));

    // Verify the stream closed callback was called
    EXPECT_TRUE(stream_closed_called);

    // Verify that the callback was called with the correct data
    ASSERT_EQ(mac_addresses_.size(), 1);
    EXPECT_EQ(mac_addresses_[0], "112233445566");
    ASSERT_EQ(names_.size(), 1);
    EXPECT_EQ(names_[0], "TestDevice");
    ASSERT_EQ(rssi_values_.size(), 1);
    EXPECT_EQ(rssi_values_[0], "-75");

    // Verify status is OK
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test the scan method with multiple devices
TEST_F(HM11DriverDiscoveryTest, ScanMultipleDevices) {
    // Set up the last read position handler
    bool stream_closed_called = false;
    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](bool& closed, std::size_t& zero) {
            closed = false;
            zero = 200; // Some position after the response data
        }))
        .WillOnce(::testing::Invoke([&stream_closed_called](bool& closed, std::size_t& zero) {
            closed = true; // Indicate stream closed
            zero = 200;
            stream_closed_called = true;
        }));

    // Queue responses for the scan command
    std::string scan_response = "OK+DISCS\r\n";
    scan_response += "OK+DISC:112233445566\r\n";
    scan_response += "OK+NAME:DeviceOne\r\n";
    scan_response += "OK+RSSI:-75\r\n";
    scan_response += "OK+DISC:AABBCCDDEEFF\r\n";
    scan_response += "OK+NAME:DeviceTwo\r\n";
    scan_response += "OK+RSSI:-80\r\n";
    scan_response += "OK+DISC:123456789ABC\r\n";
    scan_response += "OK+NAME:DeviceThree\r\n";
    scan_response += "OK+RSSI:-65\r\n";
    scan_response += "OK+DISCE\r\n";

    QueueResponse(scan_response);

    // Call the scan method
    hal::UartStatus status;
    driver_->scan(discovery_callback, std::chrono::milliseconds(1000), status);

    // Verify that the scan command was transmitted
    EXPECT_TRUE(VerifyTransmittedData("AT+DISC?"));

    // Verify the stream closed callback was called
    EXPECT_TRUE(stream_closed_called);

    // Verify that the callback was called with the correct data
    ASSERT_EQ(mac_addresses_.size(), 3);
    EXPECT_EQ(mac_addresses_[0], "112233445566");
    EXPECT_EQ(mac_addresses_[1], "AABBCCDDEEFF");
    EXPECT_EQ(mac_addresses_[2], "123456789ABC");

    ASSERT_EQ(names_.size(), 3);
    EXPECT_EQ(names_[0], "DeviceOne");
    EXPECT_EQ(names_[1], "DeviceTwo");
    EXPECT_EQ(names_[2], "DeviceThree");

    ASSERT_EQ(rssi_values_.size(), 3);
    EXPECT_EQ(rssi_values_[0], "-75");
    EXPECT_EQ(rssi_values_[1], "-80");
    EXPECT_EQ(rssi_values_[2], "-65");

    // Verify status is OK
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test the scan_ibeacon method
TEST_F(HM11DriverDiscoveryTest, ScanIBeacon) {
    // Set up the last read position handler
    bool stream_closed_called = false;
    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](bool& closed, std::size_t& zero) {
            closed = false;
            zero = 200; // Some position after the response data
        }))
        .WillOnce(::testing::Invoke([&stream_closed_called](bool& closed, std::size_t& zero) {
            closed = true; // Indicate stream closed
            zero = 200;
            stream_closed_called = true;
        }));

    // Queue responses for the scan_ibeacon command
    std::string scan_response = "OK+DISCS\r\n";
    scan_response += "OK+DISC74278BDAB64445208F0C720EAF059935FFE0FFE1C5112233445566-75";
    scan_response += "OK+DISCE\r\n";

    QueueResponse(scan_response);

    // Call the scan_ibeacon method
    hal::UartStatus status;
    driver_->scan_ibeacon(ibeacon_callback, std::chrono::milliseconds(1000), status);

    // Verify that the scan command was transmitted
    EXPECT_TRUE(VerifyTransmittedData("AT+DISI?"));

    // Verify the stream closed callback was called
    EXPECT_TRUE(stream_closed_called);

    // Verify that the callback was called with the correct data
    ASSERT_EQ(ibeacon_devices_.size(), 1);
    // The iBeacon data includes Factory ID (8), UUID (32), Major/Minor/Power (10), MAC (12), RSSI (4)
    EXPECT_EQ(ibeacon_devices_[0].length(), 66);
    EXPECT_EQ(ibeacon_devices_[0], "74278BDAB64445208F0C720EAF059935FFE0FFE1C5112233445566-75");

    // Verify status is OK
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

// Test error handling in scan method
TEST_F(HM11DriverDiscoveryTest, ScanErrorHandling) {
    // Configure the UART mock to return an error for transmit
    uart_mock_->set_default_status(hal::UartStatus::Error);

    // Call the scan method
    hal::UartStatus status;
    driver_->scan(discovery_callback, std::chrono::milliseconds(1000), status);

    // Verify that the callback wasn't called
    EXPECT_TRUE(mac_addresses_.empty());
    EXPECT_TRUE(names_.empty());
    EXPECT_TRUE(rssi_values_.empty());

    // Verify status shows error
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}

// Test the set_ibeacon_deploy_mode method
TEST_F(HM11DriverDiscoveryTest, SetIBeaconDeployMode) {
    // Call the set_ibeacon_deploy_mode method with BroadcastScanning mode
    hal::UartStatus status;
    driver_->set_ibeacon_deploy_mode(IBeaconDeployMode::BroadcastScanning, status);

    // Verify that the command was transmitted
    EXPECT_TRUE(VerifyTransmittedData("AT+DELO1"));

    // Verify status is OK
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);

    // Reset transmitted data
    uart_mock_->clear_transmitted_data();

    // Call the set_ibeacon_deploy_mode method with OnlyBroadcast mode
    driver_->set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);

    // Verify that the command was transmitted
    EXPECT_TRUE(VerifyTransmittedData("AT+DELO2"));

    // Verify status is OK
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

} // namespace hm11::tests