/**
 * @file test_hm11_driver_linklayer.cpp
 * @brief Unit tests for HM11Driver link layer configuration operations
 *
 * Tests cover connection interval (minimum/maximum), slave latency, supervision timeout,
 * update connection, and discovery time operations.
 */

#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// Mock UART Port for link layer tests
class LinkLayerMockUARTPort {
  public:
    std::vector<UInt8> transmitted_data;
    std::vector<UInt8> receive_buffer;
    UartStatus next_status{UartStatus::Ok};

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        status = next_status;
        if (status == UartStatus::Ok) {
            transmitted_data.insert(transmitted_data.end(), data.begin(), data.end());
        }
    }

    void
    receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool /*as_stream*/ = false) noexcept {
        status = next_status;
        if (status == UartStatus::Ok) {
            const std::size_t TO_COPY = std::min(data.size(), receive_buffer.size());
            std::copy_n(receive_buffer.begin(), TO_COPY, data.begin());
        }
    }

    void set_receive_response(std::string_view response) {
        receive_buffer.clear();
        receive_buffer.insert(receive_buffer.end(), response.begin(), response.end());
    }

    [[nodiscard]] std::string get_transmitted_string() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return {reinterpret_cast<const char*>(transmitted_data.data()), transmitted_data.size()};
    }

    void clear() {
        transmitted_data.clear();
        receive_buffer.clear();
        next_status = UartStatus::Ok;
    }
};

// Mock Receive Handler
struct MockReceiveHandler {
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// Test fixture
class HM11DriverLinkLayerTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    LinkLayerMockUARTPort mock_uart;
    MockReceiveHandler mock_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void SetUp() override { mock_uart.clear(); }
};

// ============================================================================
// Minimum Link Layer Connection Interval Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetMinimumLinkLayerConnectionInterval_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test all valid enum values
    const std::vector<std::pair<LinkLayerConnectionInterval, char>> test_cases = {
        {LinkLayerConnectionInterval::Ms7, '0'},
        {LinkLayerConnectionInterval::Ms10, '1'},
        {LinkLayerConnectionInterval::Ms15, '2'},
        {LinkLayerConnectionInterval::Ms20, '3'}, // Default minimum
        {LinkLayerConnectionInterval::Ms25, '4'},
        {LinkLayerConnectionInterval::Ms30, '5'},
        {LinkLayerConnectionInterval::Ms35, '6'},
        {LinkLayerConnectionInterval::Ms40, '7'}, // Default maximum
        {LinkLayerConnectionInterval::Ms45, '8'},
        {LinkLayerConnectionInterval::Ms4000, '9'},
    };

    for (const auto& [expected_interval, response_char] : test_cases) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += response_char;
        mock_uart.set_receive_response(response);

        LinkLayerConnectionInterval result{LinkLayerConnectionInterval::Ms20};
        UartStatus status{UartStatus::ErrError};

        driver.get_minimum_link_layer_connection_interval(result, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Get failed for interval char: " << response_char;
        EXPECT_EQ(result, expected_interval) << "Incorrect interval for char: " << response_char;
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COMI?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetMinimumLinkLayerConnectionInterval_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    const std::vector<std::pair<LinkLayerConnectionInterval, std::string>> test_cases = {
        {LinkLayerConnectionInterval::Ms7, "0"},
        {LinkLayerConnectionInterval::Ms10, "1"},
        {LinkLayerConnectionInterval::Ms15, "2"},
        {LinkLayerConnectionInterval::Ms20, "3"},
        {LinkLayerConnectionInterval::Ms25, "4"},
        {LinkLayerConnectionInterval::Ms30, "5"},
        {LinkLayerConnectionInterval::Ms35, "6"},
        {LinkLayerConnectionInterval::Ms40, "7"},
        {LinkLayerConnectionInterval::Ms45, "8"},
        {LinkLayerConnectionInterval::Ms4000, "9"},
    };

    for (const auto& [interval, param_str] : test_cases) {
        mock_uart.clear();
        mock_uart.set_receive_response("OK+Set:" + param_str);

        UartStatus status{UartStatus::ErrError};
        driver.set_minimum_link_layer_connection_interval(interval, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Set failed for interval: " << param_str;
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COMI" + param_str);
    }
}

// ============================================================================
// Maximum Link Layer Connection Interval Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetMaximumLinkLayerConnectionInterval_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    const std::vector<std::pair<LinkLayerConnectionInterval, char>> test_cases = {
        {LinkLayerConnectionInterval::Ms7, '0'},
        {LinkLayerConnectionInterval::Ms40, '7'}, // Default maximum
        {LinkLayerConnectionInterval::Ms4000, '9'},
    };

    for (const auto& [expected_interval, response_char] : test_cases) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += response_char;
        mock_uart.set_receive_response(response);

        LinkLayerConnectionInterval result{LinkLayerConnectionInterval::Ms20};
        UartStatus status{UartStatus::ErrError};

        driver.get_maximum_link_layer_connection_interval(result, status);

        EXPECT_EQ(status, UartStatus::Ok);
        EXPECT_EQ(result, expected_interval);
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COMA?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetMaximumLinkLayerConnectionInterval_DefaultValue) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:7");

    UartStatus status{UartStatus::ErrError};
    driver.set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval::Ms40, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COMA7");
}

// ============================================================================
// Slave Latency Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetLinkLayerConnectionSlaveLatency_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test all valid values 0-4
    for (uint8_t latency_value = 0; latency_value <= 4; ++latency_value) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += static_cast<char>('0' + latency_value);
        mock_uart.set_receive_response(response);

        LayerConnectionLatency result{0};
        UartStatus status{UartStatus::ErrError};

        driver.get_link_layer_connection_slave_latency(result, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Get failed for latency: " << static_cast<int>(latency_value);
        EXPECT_EQ(result.get(), latency_value) << "Incorrect latency value";
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COLA?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetLinkLayerConnectionSlaveLatency_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    for (uint8_t latency_value = 0; latency_value <= 4; ++latency_value) {
        mock_uart.clear();
        std::string param_str(1, static_cast<char>('0' + latency_value));
        mock_uart.set_receive_response("OK+Set:" + param_str);

        LayerConnectionLatency latency{latency_value};
        UartStatus status{UartStatus::ErrError};
        driver.set_link_layer_connection_slave_latency(latency, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Set failed for latency: " << static_cast<int>(latency_value);
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COLA" + param_str);
    }
}

TEST_F(HM11DriverLinkLayerTest, SetLinkLayerConnectionSlaveLatency_DefaultValue) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    LayerConnectionLatency latency{0}; // Default value
    UartStatus status{UartStatus::ErrError};
    driver.set_link_layer_connection_slave_latency(latency, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COLA0");
}

// ============================================================================
// Connection Supervision Timeout Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetConnectionSupervisionTimeout_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    const std::vector<std::pair<ConnectionSupervisionTimeout, char>> test_cases = {
        {ConnectionSupervisionTimeout::Ms100, '0'},
        {ConnectionSupervisionTimeout::Ms1000, '1'},
        {ConnectionSupervisionTimeout::Ms2000, '2'},
        {ConnectionSupervisionTimeout::Ms3000, '3'},
        {ConnectionSupervisionTimeout::Ms4000, '4'},
        {ConnectionSupervisionTimeout::Ms5000, '5'},
        {ConnectionSupervisionTimeout::Ms6000, '6'}, // Default
    };

    for (const auto& [expected_timeout, response_char] : test_cases) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += response_char;
        mock_uart.set_receive_response(response);

        ConnectionSupervisionTimeout result{ConnectionSupervisionTimeout::Ms100};
        UartStatus status{UartStatus::ErrError};

        driver.get_connection_supervision_timeout(result, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Get failed for timeout char: " << response_char;
        EXPECT_EQ(result, expected_timeout) << "Incorrect timeout for char: " << response_char;
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COSU?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetConnectionSupervisionTimeout_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    const std::vector<std::pair<ConnectionSupervisionTimeout, std::string>> test_cases = {
        {ConnectionSupervisionTimeout::Ms100, "0"},
        {ConnectionSupervisionTimeout::Ms1000, "1"},
        {ConnectionSupervisionTimeout::Ms2000, "2"},
        {ConnectionSupervisionTimeout::Ms3000, "3"},
        {ConnectionSupervisionTimeout::Ms4000, "4"},
        {ConnectionSupervisionTimeout::Ms5000, "5"},
        {ConnectionSupervisionTimeout::Ms6000, "6"},
    };

    for (const auto& [timeout, param_str] : test_cases) {
        mock_uart.clear();
        mock_uart.set_receive_response("OK+Set:" + param_str);

        UartStatus status{UartStatus::ErrError};
        driver.set_connection_supervision_timeout(timeout, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Set failed for timeout: " << param_str;
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COSU" + param_str);
    }
}

TEST_F(HM11DriverLinkLayerTest, SetConnectionSupervisionTimeout_DefaultValue) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:6");

    UartStatus status{UartStatus::ErrError};
    driver.set_connection_supervision_timeout(ConnectionSupervisionTimeout::Ms6000, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COSU6");
}

// ============================================================================
// Update Connection Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetUpdateConnection_Enabled) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    bool result = false;
    UartStatus status{UartStatus::ErrError};
    driver.get_update_connection(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(result);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COUP?");
}

TEST_F(HM11DriverLinkLayerTest, GetUpdateConnection_Disabled) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:0");

    bool result = true;
    UartStatus status{UartStatus::ErrError};
    driver.get_update_connection(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_FALSE(result);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COUP?");
}

TEST_F(HM11DriverLinkLayerTest, SetUpdateConnection_Enable) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::ErrError};
    driver.set_update_connection(true, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COUP1");
}

TEST_F(HM11DriverLinkLayerTest, SetUpdateConnection_Disable) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::ErrError};
    driver.set_update_connection(false, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+COUP0");
}

// ============================================================================
// Discovery Time Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetDiscoveryTime_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test all valid values 1-9
    for (uint8_t time_value = 1; time_value <= 9; ++time_value) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += static_cast<char>('0' + time_value);
        mock_uart.set_receive_response(response);

        DiscoveryTime result{3}; // Default is 3
        UartStatus status{UartStatus::ErrError};

        driver.get_discovery_time(result, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Get failed for discovery time: " << static_cast<int>(time_value);
        EXPECT_EQ(result.get(), time_value) << "Incorrect discovery time value";
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SCAN?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetDiscoveryTime_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    for (uint8_t time_value = 1; time_value <= 9; ++time_value) {
        mock_uart.clear();
        std::string param_str(1, static_cast<char>('0' + time_value));
        mock_uart.set_receive_response("OK+Set:" + param_str);

        DiscoveryTime time{time_value};
        UartStatus status{UartStatus::ErrError};
        driver.set_discovery_time(time, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Set failed for discovery time: " << static_cast<int>(time_value);
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SCAN" + param_str);
    }
}

TEST_F(HM11DriverLinkLayerTest, SetDiscoveryTime_DefaultValue) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:3");

    DiscoveryTime time{3}; // Default value
    UartStatus status{UartStatus::ErrError};
    driver.set_discovery_time(time, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SCAN3");
}

TEST_F(HM11DriverLinkLayerTest, GetShowDeviceInformationWhenDiscovery_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test all valid values 0-3
    const std::array<ShowDeviceInformation, 4> TEST_VALUES = {
        ShowDeviceInformation::DontShow,
        ShowDeviceInformation::ShowName,
        ShowDeviceInformation::ShowRssi,
        ShowDeviceInformation::ShowRssiAndName
    };

    for (uint8_t i = 0; i < TEST_VALUES.size(); ++i) {
        mock_uart.clear();
        std::string response = "OK+Get:";
        response += static_cast<char>('0' + i);
        mock_uart.set_receive_response(response);

        ShowDeviceInformation result{ShowDeviceInformation::DontShow};
        UartStatus status{UartStatus::ErrError};

        driver.GetShowDeviceInformationWhenDiscovery(result, status);

        EXPECT_EQ(status, UartStatus::Ok) << "Get failed for ShowDeviceInformation value: " << static_cast<int>(i);
        EXPECT_EQ(result, TEST_VALUES[i]) << "Incorrect ShowDeviceInformation value";
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SHOW?");
    }
}

TEST_F(HM11DriverLinkLayerTest, SetShowDeviceInformationWhenDiscovery_AllValues) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    const std::array<ShowDeviceInformation, 4> TEST_VALUES = {
        ShowDeviceInformation::DontShow,
        ShowDeviceInformation::ShowName,
        ShowDeviceInformation::ShowRssi,
        ShowDeviceInformation::ShowRssiAndName
    };

    for (uint8_t i = 0; i < TEST_VALUES.size(); ++i) {
        mock_uart.clear();
        std::string param_str(1, static_cast<char>('0' + i));
        mock_uart.set_receive_response("OK+Set:" + param_str);

        UartStatus status{UartStatus::ErrError};
        driver.SetShowDeviceInformationWhenDiscovery(TEST_VALUES[i], status);

        EXPECT_EQ(status, UartStatus::Ok) << "Set failed for ShowDeviceInformation value: " << static_cast<int>(i);
        EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SHOW" + param_str);
    }
}

TEST_F(HM11DriverLinkLayerTest, SetShowDeviceInformationWhenDiscovery_DefaultValue) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::ErrError};
    driver.SetShowDeviceInformationWhenDiscovery(ShowDeviceInformation::DontShow, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SHOW0");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(HM11DriverLinkLayerTest, GetMinimumInterval_TransmitError) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.next_status = UartStatus::ErrError;

    LinkLayerConnectionInterval result{LinkLayerConnectionInterval::Ms20};
    UartStatus status{UartStatus::Ok};
    driver.get_minimum_link_layer_connection_interval(result, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverLinkLayerTest, SetMaximumInterval_ReceiveError) {
    HM11Driver<LinkLayerMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.next_status = UartStatus::ErrTimeout;

    UartStatus status{UartStatus::Ok};
    driver.set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval::Ms40, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

