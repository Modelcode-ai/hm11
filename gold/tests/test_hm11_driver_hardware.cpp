/**
 * @file test_hm11_driver_hardware.cpp
 * @brief Unit tests for HM11Driver hardware control operations
 *
 * Tests cover:
 * - Individual PIO pin control (Set/Get_PIO_Output_Status)
 * - Multiple PIO pin control (Set/Get_PIOs_Output_Status)
 * - Connect/Power pin output configuration
 * - PIO collection rate configuration
 * - Output power control (Normal/Max)
 * - Module power control (-23/-6/0/+6 dBm)
 * - RX gain control
 * - Auto sleep mode
 * - Notification settings (information, mode)
 * - Work mode configuration
 * - RangedInteger and enum validation
 * - Status error propagation
 */

#include <gtest/gtest.h>

#include <string>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// ============================================================================
// Mock UART Port Implementation (reuse from core tests)
// ============================================================================

class HM11MockUARTPort {
  public:
    std::vector<UInt8> transmitted_data;
    std::vector<UInt8> receive_buffer;
    UartStatus next_status{UartStatus::Ok};
    int transmit_call_count{0};
    int receive_call_count{0};

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        ++transmit_call_count;
        status = next_status;
        if (status == UartStatus::Ok) {
            transmitted_data.insert(transmitted_data.end(), data.begin(), data.end());
        }
    }

    void
    receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool /*as_stream*/ = false) noexcept {
        ++receive_call_count;
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
        return {reinterpret_cast<const char*>(transmitted_data.data()), transmitted_data.size()}; // NOLINT
    }

    void clear() {
        transmitted_data.clear();
        receive_buffer.clear();
        next_status = UartStatus::Ok;
        transmit_call_count = 0;
        receive_call_count = 0;
    }
};

// ============================================================================
// Mock Receive Handler
// ============================================================================

class MockReceiveHandler {
  public:
    template <typename UARTImpl>
    void operator()(UARTImpl& uart_port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        uart_port.receive(buffer, status, 1000, as_stream);
    }

    template <typename UARTImpl>
    void operator()(
        UARTImpl& uart_port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        uart_port.receive(buffer, status, timeout_ms, as_stream);
    }
};

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverHardwareTest : public ::testing::Test {
  protected:
    HM11MockUARTPort mock_uart;
    MockReceiveHandler mock_handler;

    void SetUp() override { mock_uart.clear(); }
};

// ============================================================================
// Individual PIO Pin Control Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetPioOutputStatusPio1Low) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_pio_output_status(PioNumber(1), PioOutput::Low, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO10");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPioOutputStatusPio1High) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_pio_output_status(PioNumber(1), PioOutput::High, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO11");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPioOutputStatusPio2Low) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+PIO2:0");

    UartStatus status{UartStatus::Busy};
    driver.set_pio_output_status(PioNumber(2), PioOutput::Low, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO20");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPioOutputStatusPio3High) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+PIO3:1");

    UartStatus status{UartStatus::Busy};
    driver.set_pio_output_status(PioNumber(3), PioOutput::High, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO31");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetPioOutputStatusPio1) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    PioOutput output{PioOutput::Low};
    UartStatus status{UartStatus::Busy};
    driver.get_pio_output_status(PioNumber(1), output, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO1?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(output, PioOutput::High);
}

TEST_F(HM11DriverHardwareTest, GetPioOutputStatusPio2) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+PIO2:0");

    PioOutput output{PioOutput::High};
    UartStatus status{UartStatus::Busy};
    driver.get_pio_output_status(PioNumber(2), output, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO2?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(output, PioOutput::Low);
}

// ============================================================================
// Multiple PIO Pin Control Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetPiosOutputStatusAllLow) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:000");

    PioNumbers pios("000");
    UartStatus status{UartStatus::Busy};
    driver.set_pios_output_status(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MPIO000");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPiosOutputStatusPio2And3High) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:00C");

    PioNumbers pios("00C"); // 0x00C = 0b001100 = PIO2 and PIO3 high
    UartStatus status{UartStatus::Busy};
    driver.set_pios_output_status(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MPIO00C");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPiosOutputStatusMax) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:3FF");

    PioNumbers pios("3FF"); // All PIOs high
    UartStatus status{UartStatus::Busy};
    driver.set_pios_output_status(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MPIO3FF");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetPiosOutputStatus) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+PIO?00C");

    PioNumbers pios("000");
    UartStatus status{UartStatus::Busy};
    driver.get_pios_output_status(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PIO??");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(pios.view(), "00C");
}

// ============================================================================
// Connect/Power Pin Output Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetConnectPinOutput) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:004");

    PioNumbers pios("004");
    UartStatus status{UartStatus::Busy};
    driver.set_connect_pin_output(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+AFTC004");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetConnectPinOutput) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:008");

    PioNumbers result("000");
    UartStatus status{UartStatus::Busy};
    driver.get_connect_pin_output(result, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+AFTC?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.view(), "008");
}

TEST_F(HM11DriverHardwareTest, SetPowerPinOutput) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:00C");

    PioNumbers pios("00C");
    UartStatus status{UartStatus::Busy};
    driver.set_power_pin_output(pios, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+BEFC00C");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetPowerPinOutput) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:004");

    PioNumbers result("000");
    UartStatus status{UartStatus::Busy};
    driver.get_power_pin_output(result, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+BEFC?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.view(), "004");
}

// ============================================================================
// PIO Collection Rate Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetPioCollectionRateSingleDigit) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:05");

    PioCollectionRate rate(5);
    UartStatus status{UartStatus::Busy};
    driver.set_pio_collection_rate(rate, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+CYC05");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPioCollectionRateDoubleDigit) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:42");

    PioCollectionRate rate(42);
    UartStatus status{UartStatus::Busy};
    driver.set_pio_collection_rate(rate, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+CYC42");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetPioCollectionRateMax) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:99");

    PioCollectionRate rate(99);
    UartStatus status{UartStatus::Busy};
    driver.set_pio_collection_rate(rate, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+CYC99");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetPioCollectionRate) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:15");

    PioCollectionRate result(0);
    UartStatus status{UartStatus::Busy};
    driver.get_pio_collection_rate(result, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+CYC??");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.get(), 15);
}

TEST_F(HM11DriverHardwareTest, GetPioCollectionRateSingleDigit) {
    // Test variable-length response handling for 1-digit values (0-9)
    // Hardware returns "OK+Get:5\0" for single-digit responses
    // This tests the fix for the null-terminator check in get_pio_collection_rate()
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Simulate single-digit response with null terminator
    std::string response = "OK+Get:5";
    response.push_back('\0'); // Explicitly add null terminator as hardware does
    mock_uart.set_receive_response(response);

    PioCollectionRate result(99); // Initialize to non-zero to verify change
    UartStatus status{UartStatus::Busy};
    driver.get_pio_collection_rate(result, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+CYC??");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(result.get(), 5);
}

TEST_F(HM11DriverHardwareTest, GetPioCollectionRateBoundaryValues) {
    // Test boundary values for variable-length responses
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test minimum value (single digit)
    mock_uart.clear();
    std::string response_min = "OK+Get:0";
    response_min.push_back('\0');
    mock_uart.set_receive_response(response_min);

    PioCollectionRate result_min(50);
    UartStatus status_min{UartStatus::Busy};
    driver.get_pio_collection_rate(result_min, status_min);

    EXPECT_EQ(status_min, UartStatus::Ok);
    EXPECT_EQ(result_min.get(), 0);

    // Test maximum value (two digits)
    mock_uart.clear();
    mock_uart.set_receive_response("OK+Get:99");

    PioCollectionRate result_max(0);
    UartStatus status_max{UartStatus::Busy};
    driver.get_pio_collection_rate(result_max, status_max);

    EXPECT_EQ(status_max, UartStatus::Ok);
    EXPECT_EQ(result_max.get(), 99);
}

// ============================================================================
// Output Power Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetOutputPowerNormal) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_output_power(OutputPower::NormalPower, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PCTL0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetOutputPowerMax) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_output_power(OutputPower::MaxPower, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PCTL1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetOutputPower) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    OutputPower power{OutputPower::NormalPower};
    UartStatus status{UartStatus::Busy};
    driver.get_output_power(power, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PCTL?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(power, OutputPower::MaxPower);
}

// ============================================================================
// Module Power Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetModulePowerMinus23) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_module_power(ModulePower::DbmMinus23, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+POWE0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetModulePowerMinus6) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_module_power(ModulePower::DbmMinus6, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+POWE1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetModulePower0) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:2");

    UartStatus status{UartStatus::Busy};
    driver.set_module_power(ModulePower::Dbm0, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+POWE2");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetModulePower6) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:3");

    UartStatus status{UartStatus::Busy};
    driver.set_module_power(ModulePower::Dbm6, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+POWE3");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetModulePower) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:2");

    ModulePower power{ModulePower::DbmMinus23};
    UartStatus status{UartStatus::Busy};
    driver.get_module_power(power, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+POWE?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(power, ModulePower::Dbm0);
}

// ============================================================================
// RX Gain Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetModuleRxGainNo) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_module_rx_gain(RxGain::No, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+GAIN0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetModuleRxGainOpen) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_module_rx_gain(RxGain::Open, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+GAIN1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetModuleRxGain) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    RxGain gain{RxGain::No};
    UartStatus status{UartStatus::Busy};
    driver.get_module_rx_gain(gain, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+GAIN?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(gain, RxGain::Open);
}

// ============================================================================
// Auto Sleep Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetModuleAutoSleepEnabled) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Note: Inverted - sleep=true sends "0"
    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_module_auto_sleep(true, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PWRM0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetModuleAutoSleepDisabled) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Note: Inverted - sleep=false sends "1"
    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_module_auto_sleep(false, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PWRM1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetModuleAutoSleep) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Note: Inverted - '0' means enabled
    mock_uart.set_receive_response("OK+Get:0");

    bool sleep = false;
    UartStatus status{UartStatus::Busy};
    driver.get_module_auto_sleep(sleep, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+PWRM?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(sleep);
}

// ============================================================================
// Notify Information Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetNotifyInformationEnabled) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_notify_information(true, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTI1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetNotifyInformationDisabled) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_notify_information(false, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTI0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetNotifyInformation) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    bool notify = false;
    UartStatus status{UartStatus::Busy};
    driver.get_notify_information(notify, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTI?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_TRUE(notify);
}

// ============================================================================
// Notify Mode Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetNotifyModeWithoutAddress) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_notify_mode(NotifyMode::WithoutAddress, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTP0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetNotifyModeWithAddress) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_notify_mode(NotifyMode::WithAddress, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTP1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetNotifyMode) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    NotifyMode mode{NotifyMode::WithoutAddress};
    UartStatus status{UartStatus::Busy};
    driver.get_notify_mode(mode, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+NOTP?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mode, NotifyMode::WithAddress);
}

// ============================================================================
// Work Mode Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, SetWorkModeTransmission) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:0");

    UartStatus status{UartStatus::Busy};
    driver.set_work_mode(WorkMode::TransmissionMode, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MODE0");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetWorkModePioCollection) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:1");

    UartStatus status{UartStatus::Busy};
    driver.set_work_mode(WorkMode::PioCollectionMode, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MODE1");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, SetWorkModeRemoteControl) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Set:2");

    UartStatus status{UartStatus::Busy};
    driver.set_work_mode(WorkMode::RemoteControlMode, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MODE2");
    EXPECT_EQ(status, UartStatus::Ok);
}

TEST_F(HM11DriverHardwareTest, GetWorkMode) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.set_receive_response("OK+Get:1");

    WorkMode mode{WorkMode::TransmissionMode};
    UartStatus status{UartStatus::Busy};
    driver.get_work_mode(mode, status);

    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+MODE?");
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mode, WorkMode::PioCollectionMode);
}

// ============================================================================
// Error Propagation Tests
// ============================================================================

TEST_F(HM11DriverHardwareTest, TransmitFailurePropagates) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    mock_uart.next_status = UartStatus::ErrError;

    UartStatus status{UartStatus::Busy};
    driver.set_output_power(OutputPower::MaxPower, status);

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST_F(HM11DriverHardwareTest, ReceiveTimeoutPropagates) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Transmit succeeds, receive times out
    mock_uart.next_status = UartStatus::Ok;
    UartStatus temp_status{UartStatus::Busy};
    mock_uart.transmit(UartData8bConst{}, temp_status, 1000);

    mock_uart.next_status = UartStatus::ErrTimeout;

    UartStatus status{UartStatus::Busy};
    OutputPower power{OutputPower::NormalPower};
    driver.get_output_power(power, status);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}
