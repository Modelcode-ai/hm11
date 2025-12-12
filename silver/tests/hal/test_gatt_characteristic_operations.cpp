// GATT Characteristic Operations Test File
// Tests for HM11Driver characteristic discovery and operations
// Validates characteristic discovery, read/write, and notification functionality

#include <gtest/gtest.h>

#include <array>
#include <hm11/hal/mock_uart.hpp>
#include <hm11/hm11_driver.hpp>
#include <hm11/types.hpp>
#include <hm11/util.hpp>
#include <string_view>
#include <vector>

using namespace hm11;
using namespace hm11::hal;

// ============================================================================
// GATT Characteristic Discovery Tests
// ============================================================================

class GattCharacteristicDiscoveryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        uart.ClearBuffers();
        uart.SetForcedStatus(UARTStatus::Ok);

        // Set up mock response for characteristic discovery
        // Format: header + CRLF + characteristic_data + header (end)
        const std::string header(56, '*');
        const std::string crlf = "\r\n";
        const std::string char1 = "0001:RD|WR|--|NO|--:1234"; // 26 chars including colons
        const std::string char2 = "0002:--|--|WN|--|IN:5678"; // 26 chars including colons

        std::string response = header + crlf + char1 + crlf + char2 + crlf + header;

        std::vector<uint8_t> response_data;
        for (char c : response) {
            response_data.push_back(static_cast<uint8_t>(c));
        }

        uart.SetReceiveData(response_data);
    }

    void TearDown() override { uart.ClearBuffers(); }

    MockUART uart;
};

TEST_F(GattCharacteristicDiscoveryTest, FindAllCharacteristicsSuccess) {
    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_characteristics;

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
        characteristics->emplace_back(characteristic);
    };

    UARTStatus status;
    driver.find_all_characteristics_uuid(callback, &discovered_characteristics, 1000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_characteristics.size(), 2);

    if (discovered_characteristics.size() >= 2) {
        EXPECT_EQ(discovered_characteristics[0], "0001:RD|WR|--|NO|--:1234");
        EXPECT_EQ(discovered_characteristics[1], "0002:--|--|WN|--|IN:5678");
    }

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+FINDALLCHARS?");
}

TEST_F(GattCharacteristicDiscoveryTest, FindCharacteristicsWithRangeSuccess) {
    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_characteristics;

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
        characteristics->emplace_back(characteristic);
    };

    HandleType from_handle("0001");
    HandleType to_handle("0010");

    UARTStatus status;
    driver.find_characteristics_uuid(from_handle, to_handle, callback, &discovered_characteristics, 1000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_characteristics.size(), 2);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+CHAR00010010?");
}

TEST_F(GattCharacteristicDiscoveryTest, FindCharacteristicsNullCallback) {
    auto driver = make_hm11_driver(std::move(uart));

    UARTStatus status;
    driver.find_all_characteristics_uuid(nullptr, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(GattCharacteristicDiscoveryTest, FindCharacteristicsUartError) {
    uart.SetForcedStatus(UARTStatus::ErrTimeout);
    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_characteristics;

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
        characteristics->emplace_back(characteristic);
    };

    UARTStatus status;
    driver.find_all_characteristics_uuid(callback, &discovered_characteristics, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrTimeout);
    EXPECT_EQ(discovered_characteristics.size(), 0);
}

// ============================================================================
// Characteristic Notification Operations Tests
// ============================================================================

class GattCharacteristicOperationsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        uart.ClearBuffers();
        uart.SetForcedStatus(UARTStatus::Ok);
    }

    void TearDown() override { uart.ClearBuffers(); }

    MockUART uart;
};

TEST_F(GattCharacteristicOperationsTest, EnableNotificationSuccess) {
    // Set up mock response for successful notification enable
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0001");
    UARTStatus status;
    NotifyResponse response_value;

    driver.enable_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(response_value, NotifyResponse::SendOk);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+NOTIFY_ON0001");
}

TEST_F(GattCharacteristicOperationsTest, EnableNotificationDataError) {
    // Set up mock response for data error (characteristic doesn't have notify property)
    const std::string response = "OK+DATA-ER";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0002");
    UARTStatus status;
    NotifyResponse response_value;

    driver.enable_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(response_value, NotifyResponse::DataEr);
}

TEST_F(GattCharacteristicOperationsTest, EnableNotificationSendError) {
    // Set up mock response for send error (unknown response)
    const std::string response = "OK+ERRXXXX";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0003");
    UARTStatus status;
    NotifyResponse response_value;

    driver.enable_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(response_value, NotifyResponse::SendEr);
}

TEST_F(GattCharacteristicOperationsTest, DisableNotificationSuccess) {
    // Set up mock response for successful notification disable
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0004");
    UARTStatus status;
    NotifyResponse response_value;

    driver.disable_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(response_value, NotifyResponse::SendOk);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+NOTIFYOFF0004");
}

TEST_F(GattCharacteristicOperationsTest, ReadCharacteristicSuccess) {
    // Set up mock response for successful characteristic read
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0005");
    UARTStatus status;
    NotifyResponse response_value;

    driver.read_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(response_value, NotifyResponse::SendOk);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+READDATA0005");
}

// ============================================================================
// Characteristic Data Operations Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, SetMethodAndHandleSuccess) {
    // Set up mock response for successful method and handle setting
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0006");
    UARTStatus status;

    driver.set_method_and_characteristic_handle(handle, SendDataMethod::Write, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+SET_WAYWR0006");
}

TEST_F(GattCharacteristicOperationsTest, SetMethodAndHandleWithNotify) {
    // Set up mock response for successful method and handle setting with Notify
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0007");
    UARTStatus status;

    driver.set_method_and_characteristic_handle(handle, SendDataMethod::Notify, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+SET_WAYNO0007");
}

TEST_F(GattCharacteristicOperationsTest, SetMethodAndHandleWithIndicate) {
    // Set up mock response for successful method and handle setting with Indicate
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0008");
    UARTStatus status;

    driver.set_method_and_characteristic_handle(handle, SendDataMethod::Indicate, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+SET_WAYIN0008");
}

TEST_F(GattCharacteristicOperationsTest, SetMethodAndHandleWithWriteWithoutResponse) {
    // Set up mock response for successful method and handle setting with Write Without Response
    const std::string response = "OK+SEND-OK";
    std::vector<uint8_t> response_data;
    for (char c : response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0009");
    UARTStatus status;

    driver.set_method_and_characteristic_handle(handle, SendDataMethod::WriteWithoutResponse, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+SET_WAYWN0009");
}

TEST_F(GattCharacteristicOperationsTest, SendDataToCharacteristicWrite) {
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000A");
    const std::string test_data = "Hello";
    std::array<uint8_t, 5> data_array;
    for (size_t i = 0; i < test_data.size(); ++i) {
        data_array[i] = static_cast<uint8_t>(test_data[i]);
    }
    UARTData8bConst data{data_array.data(), data_array.size()};

    UARTStatus status;

    driver.send_data_to_characteristic(handle, SendDataCharacteristic::Write, data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_GE(transmitted.size(), 18); // "AT+SEND_DATAWR000A" + data

    // Extract command part (first 18 bytes)
    const std::string command(transmitted.begin(), transmitted.begin() + 18);
    EXPECT_EQ(command, "AT+SEND_DATAWR000A");

    // Extract data part (remaining bytes)
    const std::string sent_data(transmitted.begin() + 18, transmitted.end());
    EXPECT_EQ(sent_data, test_data);
}

TEST_F(GattCharacteristicOperationsTest, SendDataToCharacteristicWriteWithoutResponse) {
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000B");
    const std::string test_data = "World";
    std::array<uint8_t, 5> data_array;
    for (size_t i = 0; i < test_data.size(); ++i) {
        data_array[i] = static_cast<uint8_t>(test_data[i]);
    }
    UARTData8bConst data{data_array.data(), data_array.size()};

    UARTStatus status;

    driver.send_data_to_characteristic(handle, SendDataCharacteristic::WriteWithoutResponse, data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_GE(transmitted.size(), 18); // "AT+SEND_DATAWN000B" + data

    // Extract command part (first 18 bytes)
    const std::string command(transmitted.begin(), transmitted.begin() + 18);
    EXPECT_EQ(command, "AT+SEND_DATAWN000B");

    // Extract data part (remaining bytes)
    const std::string sent_data(transmitted.begin() + 18, transmitted.end());
    EXPECT_EQ(sent_data, test_data);
}

// ============================================================================
// Comprehensive GATT Write Operations Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, WriteCharacteristicWithResponseSuccess) {
    // Test writing data to characteristic with response
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000E");
    const std::string test_data = "TestData123";
    std::array<uint8_t, 11> data_array;
    for (size_t i = 0; i < test_data.size(); ++i) {
        data_array[i] = static_cast<uint8_t>(test_data[i]);
    }
    UARTData8bConst data{data_array.data(), data_array.size()};

    UARTStatus status;

    driver.send_data_to_characteristic(handle, SendDataCharacteristic::Write, data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_GE(transmitted.size(), 18); // "AT+SEND_DATAWR000E" + data

    // Extract command part (first 18 bytes)
    const std::string command(transmitted.begin(), transmitted.begin() + 18);
    EXPECT_EQ(command, "AT+SEND_DATAWR000E");

    // Extract data part (remaining bytes)
    const std::string sent_data(transmitted.begin() + 18, transmitted.end());
    EXPECT_EQ(sent_data, test_data);
}

TEST_F(GattCharacteristicOperationsTest, WriteCharacteristicLargeData) {
    // Test writing larger data payload (max GATT payload size)
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000F");
    const std::string large_data(244, 'X'); // 244 bytes of 'X' (near GATT max)
    std::vector<uint8_t> data_array(large_data.begin(), large_data.end());
    UARTData8bConst data{data_array.data(), data_array.size()};

    UARTStatus status;

    driver.send_data_to_characteristic(handle, SendDataCharacteristic::WriteWithoutResponse, data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 18 + large_data.size()); // Command + data

    // Extract command part
    const std::string command(transmitted.begin(), transmitted.begin() + 18);
    EXPECT_EQ(command, "AT+SEND_DATAWN000F");
}

TEST_F(GattCharacteristicOperationsTest, WriteCharacteristicEmptyData) {
    // Test writing empty data payload
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0010");
    // Create empty span for empty data test
    std::array<uint8_t, 1> dummy_array = {0};
    UARTData8bConst empty_data{dummy_array.data(), 0};

    UARTStatus status;

    driver.send_data_to_characteristic(handle, SendDataCharacteristic::Write, empty_data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was sent correctly (just command part)
    const auto& transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 18); // Just command, no data

    const std::string command(transmitted.begin(), transmitted.end());
    EXPECT_EQ(command, "AT+SEND_DATAWR0010");
}

// ============================================================================
// GATT Streaming Operations Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, StreamingNotificationParsing) {
    // Test parsing of streaming notification data
    // Set up mock response with streaming notification data
    const std::string streaming_response = "OK+RECV:48656C6C6F" // "Hello" in hex
                                           "OK+RECV:576F726C64" // "World" in hex
                                           "OK+RECV:21";        // "!" in hex

    std::vector<uint8_t> response_data;
    for (char c : streaming_response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }
    uart.SetReceiveData(response_data);

    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("0011");
    UARTStatus status;
    NotifyResponse response_value;

    driver.enable_characteristic_notify(handle, status, response_value);

    // For streaming, we would expect Ok status even with additional data
    EXPECT_EQ(status, UARTStatus::Ok);
}

// ============================================================================
// GATT Service Integration Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, CompleteGattWorkflow) {
    // Integration test demonstrating complete GATT client workflow:
    // 1. Discover services
    // 2. Discover characteristics
    // 3. Enable notifications
    // 4. Read characteristic
    // 5. Write characteristic

    // Step 1: Service discovery (simulated)
    std::vector<std::string> discovered_services;

    // Step 2: Characteristic discovery
    // Set up comprehensive characteristic discovery response
    const std::string header(56, '*');
    const std::string crlf = "\r\n";
    const std::string char1 = "0001:RD|WR|--|NO|--:180F"; // Battery Service characteristic
    const std::string char2 = "0002:RD|--|WN|--|IN:1800"; // Generic Access characteristic
    const std::string char3 = "0003:--|WR|--|NO|--|1801"; // Generic Attribute characteristic

    std::string discovery_response = header + crlf + char1 + crlf + char2 + crlf + char3 + crlf + header;

    std::vector<uint8_t> discovery_data;
    for (char c : discovery_response) {
        discovery_data.push_back(static_cast<uint8_t>(c));
    }

    uart.SetReceiveData(discovery_data);
    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_characteristics;

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
        characteristics->emplace_back(characteristic);
    };

    UARTStatus status;
    driver.find_all_characteristics_uuid(callback, &discovered_characteristics, 1000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_characteristics.size(), 3);

    if (discovered_characteristics.size() >= 3) {
        EXPECT_EQ(discovered_characteristics[0], "0001:RD|WR|--|NO|--:180F");
        EXPECT_EQ(discovered_characteristics[1], "0002:RD|--|WN|--|IN:1800");
        EXPECT_EQ(discovered_characteristics[2], "0003:--|WR|--|NO|--|1801");
    }

    // Step 3: Test characteristic operations on discovered characteristics
    // Reset UART for next operation
    uart.ClearBuffers();
    uart.SetForcedStatus(UARTStatus::Ok);
    uart.SetReceiveData(std::vector<uint8_t>{'O', 'K', '+', 'S', 'E', 'N', 'D', '-', 'O', 'K'});

    HandleType battery_handle("0001"); // Battery level characteristic
    NotifyResponse notify_response;
    status = UARTStatus::Busy;

    driver.enable_characteristic_notify(battery_handle, status, notify_response);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(notify_response, NotifyResponse::SendOk);
}

// ============================================================================
// GATT Properties Validation Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, ValidateCharacteristicProperties) {
    // Test parsing and validation of GATT characteristic properties

    // Test all property combinations
    const std::vector<std::string> property_tests = {
        "RD|--|--|--|--", // Read only
        "--|WR|--|--|--", // Write only
        "--|--|WN|--|--", // Write without response only
        "--|--|--|NO|--", // Notify only
        "--|--|--|--|IN", // Indicate only
        "RD|WR|--|--|--", // Read + Write
        "RD|--|--|NO|--", // Read + Notify
        "--|WR|WN|--|--", // Write + Write without response
        "RD|WR|WN|NO|IN", // All properties
    };

    for (size_t i = 0; i < property_tests.size(); ++i) {
        const std::string header(56, '*');
        const std::string crlf = "\r\n";
        const std::string handle = "000" + std::to_string(i + 1);
        const std::string characteristic = handle + ":" + property_tests[i] + ":1234";

        std::string response = header + crlf + characteristic + crlf + header;

        std::vector<uint8_t> response_data;
        for (char c : response) {
            response_data.push_back(static_cast<uint8_t>(c));
        }

        auto test_uart = MockUART{UARTDataSize::DataSize8b};
        test_uart.SetForcedStatus(UARTStatus::Ok);
        test_uart.SetReceiveData(response_data);

        auto test_driver = make_hm11_driver(std::move(test_uart));

        std::vector<std::string> test_characteristics;

        auto callback = [](std::string_view characteristic, void* user_data) {
            auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
            characteristics->emplace_back(characteristic);
        };

        UARTStatus status;
        test_driver.find_all_characteristics_uuid(callback, &test_characteristics, 1000, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Failed for properties: " << property_tests[i];
        EXPECT_EQ(test_characteristics.size(), 1) << "Failed for properties: " << property_tests[i];

        if (!test_characteristics.empty()) {
            EXPECT_EQ(test_characteristics[0], characteristic) << "Failed for properties: " << property_tests[i];
        }
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(GattCharacteristicOperationsTest, UartTransmitError) {
    uart.SetForcedStatus(UARTStatus::ErrError);
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000C");
    UARTStatus status;
    NotifyResponse response_value;

    driver.enable_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(response_value, NotifyResponse::SendEr);
}

TEST_F(GattCharacteristicOperationsTest, UartReceiveError) {
    uart.SetForcedStatusForReceive(UARTStatus::ErrTimeout);
    auto driver = make_hm11_driver(std::move(uart));

    HandleType handle("000D");
    UARTStatus status;
    NotifyResponse response_value;

    driver.read_characteristic_notify(handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(GattCharacteristicOperationsTest, InvalidCharacteristicHandle) {
    // Test with invalid handle format
    uart.SetReceiveData(std::vector<uint8_t>{'O', 'K', '+', 'D', 'A', 'T', 'A', '-', 'E', 'R'});
    auto driver = make_hm11_driver(std::move(uart));

    HandleType invalid_handle("GGGG"); // Invalid hex characters
    UARTStatus status;
    NotifyResponse response_value;

    // Note: HandleType validation happens at construction time
    // The driver should still attempt the operation but hardware will reject it
    driver.enable_characteristic_notify(invalid_handle, status, response_value);

    EXPECT_EQ(status, UARTStatus::Ok);                 // Command sent successfully
    EXPECT_EQ(response_value, NotifyResponse::DataEr); // Hardware returns data error
}

TEST_F(GattCharacteristicOperationsTest, MalformedCharacteristicResponse) {
    // Test handling of malformed characteristic discovery response
    const std::string malformed_response = "******************************************"
                                           "\r\n"
                                           "0001:INVALID_PROPS:1234" // Invalid properties format
                                           "\r\n"
                                           "******************************************";

    std::vector<uint8_t> response_data;
    for (char c : malformed_response) {
        response_data.push_back(static_cast<uint8_t>(c));
    }

    uart.SetReceiveData(response_data);
    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> characteristics;

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* characteristics = static_cast<std::vector<std::string>*>(user_data);
        characteristics->emplace_back(characteristic);
    };

    UARTStatus status;
    driver.find_all_characteristics_uuid(callback, &characteristics, 1000, status);

    // Should handle malformed response gracefully
    EXPECT_EQ(status, UARTStatus::Ok); // Parsing completed
    // The malformed characteristic should still be passed to callback
    // (callback decides how to handle invalid formats)
    EXPECT_EQ(characteristics.size(), 1);
    if (!characteristics.empty()) {
        EXPECT_EQ(characteristics[0], "0001:INVALID_PROPS:1234");
    }
}