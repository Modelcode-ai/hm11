/**
 * @file test_hm11_driver_advertising.cpp
 * @brief Unit tests for HM11Driver advertising configuration operations
 *
 * Tests cover advertising interval, type, mode, flags, data, and show device information operations.
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

// Mock UART Port for advertising tests
class AdvertisingMockUARTPort {
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

// Mock Receive Handler (matches core tests)
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
class HM11DriverAdvertisingTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    AdvertisingMockUARTPort mock_uart;
    MockReceiveHandler mock_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void SetUp() override { mock_uart.clear(); }
};

// ============================================================================
// Advertising Interval Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingInterval_Ms100) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_advertising_interval(AdvertisingInterval::Ms100, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+ADVI0");
}

TEST_F(HM11DriverAdvertisingTest, GetAdvertisingInterval_Ms1285) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:9");

    AdvertisingInterval result{};
    UartStatus status{UartStatus::Busy};
    driver.get_advertising_interval(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, AdvertisingInterval::Ms1285);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+ADVI?");
}

// ============================================================================
// Advertising Type Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingType_Advertising) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:3");

    UartStatus status{UartStatus::Busy};
    driver.set_advertising_type(AdvertisingType::Advertising, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+ADTY3");
}

TEST_F(HM11DriverAdvertisingTest, GetAdvertisingType_ScanResponseConnectable) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:0");

    AdvertisingType result{};
    UartStatus status{UartStatus::Busy};
    driver.get_advertising_type(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, AdvertisingType::AdvertisingScanResponseConnectable);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+ADTY?");
}

// ============================================================================
// Advertising Mode Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingMode_Reliable) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_advertising_mode(AdvertisingMode::ReliableAdvertising, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RELI1");
}

TEST_F(HM11DriverAdvertisingTest, GetAdvertisingMode_Normal) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:0");

    AdvertisingMode result{};
    UartStatus status{UartStatus::Busy};
    driver.get_advertising_mode(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, AdvertisingMode::NormalAdvertising);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RELI?");
}

// ============================================================================
// Advertising FLAG Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingFlag_StandardValue) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:06");

    AdvertisingFlag flag{"06"};
    UartStatus status{UartStatus::Busy};
    driver.set_advertising_flag(flag, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+FLAG06");
}

// ============================================================================
// Advertising Data Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingData_CustomPayload) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:48454C4C4F00");

    AdvertisementData data{"48454C4C4F00"};
    UartStatus status{UartStatus::Busy};
    driver.set_advertising_data(data, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PACK48454C4C4F00");
}

// ============================================================================
// Show Device Information Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetShowDeviceInformation_ShowRssiAndName) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Set:3");

    UartStatus status{UartStatus::Busy};
    driver.set_show_device_information_when_discovery(ShowDeviceInformation::ShowRssiAndName, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SHOW3");
}

TEST_F(HM11DriverAdvertisingTest, GetShowDeviceInformation_DontShow) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+Get:0");

    ShowDeviceInformation result{};
    UartStatus status{UartStatus::Busy};
    driver.get_show_device_information_when_discovery(result, status);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result, ShowDeviceInformation::DontShow);
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SHOW?");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(HM11DriverAdvertisingTest, SetAdvertisingInterval_TransmitError) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.next_status = UartStatus::ErrError;

    UartStatus status{UartStatus::Busy};
    driver.set_advertising_interval(AdvertisingInterval::Ms1285, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverAdvertisingTest, GetAdvertisingInterval_ResponseMismatch) {
    HM11Driver<AdvertisingMockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
    mock_uart.set_receive_response("OK+WRONG");

    AdvertisingInterval result{};
    UartStatus status{UartStatus::Busy};
    driver.get_advertising_interval(result, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}
