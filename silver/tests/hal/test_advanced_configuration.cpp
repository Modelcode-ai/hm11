// Test Advanced Configuration Operations for HM11 Driver
// Comprehensive unit tests for power management, advertising, UART configuration,
// connection parameters, white list management, and PIO control

#include <gtest/gtest.h>

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types.hpp"

using namespace hm11;
using namespace hm11::hal;

class AdvancedConfigurationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Each test will create its own driver with pre-configured mock
    }

    void TearDown() override {
        // Nothing needed - each test manages its own driver
    }

    // Helper to create driver with mock response for set commands
    static auto create_driver_with_set_response(const std::string& value) {
        MockUART mock_uart;
        std::string response = "OK+Set:" + value;
        std::vector<uint8_t> response_data(response.begin(), response.end());
        mock_uart.SetReceiveData(response_data);
        return make_hm11_driver(std::move(mock_uart));
    }

    // Helper to create driver with mock response for get commands
    static auto create_driver_with_get_response(const std::string& value) {
        MockUART mock_uart;
        std::string response = "OK+Get:" + value;
        std::vector<uint8_t> response_data(response.begin(), response.end());
        mock_uart.SetReceiveData(response_data);
        return make_hm11_driver(std::move(mock_uart));
    }

    // Helper to create driver with custom response
    static auto create_driver_with_response(const std::vector<uint8_t>& response) {
        MockUART mock_uart;
        mock_uart.SetReceiveData(response);
        return make_hm11_driver(std::move(mock_uart));
    }

    // Helper to verify transmitted command
    template <typename DriverType>
    bool verify_transmitted_command(const DriverType& driver, const std::string& expected_command) {
        const auto& transmitted = driver.GetUart().GetTransmitted8b();
        if (transmitted.size() != expected_command.size()) {
            return false;
        }

        for (size_t i = 0; i < expected_command.size(); ++i) {
            if (transmitted[i] != static_cast<uint8_t>(expected_command[i])) {
                return false;
            }
        }
        return true;
    }
};

// ============================================================================
// Power Management Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetModulePower_Success) {
    auto driver = create_driver_with_set_response("2"); // Dbm0 = "2"

    UARTStatus status = UARTStatus::ErrError;
    driver.set_module_power(ModulePower::Dbm0, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+POWE2"));
}

TEST_F(AdvancedConfigurationTest, SetModulePower_AllValues) {
    // Test all module power values
    const std::vector<std::pair<ModulePower, std::string>> test_cases = {
        {ModulePower::DbmMinus23, "0"},
        {ModulePower::DbmMinus6, "1"},
        {ModulePower::Dbm0, "2"},
        {ModulePower::Dbm6, "3"}};

    for (const auto& test_case : test_cases) {
        auto driver = create_driver_with_set_response(test_case.second);

        UARTStatus status = UARTStatus::ErrError;
        driver.set_module_power(test_case.first, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Failed for power level: " << test_case.second;
        EXPECT_TRUE(verify_transmitted_command(driver, "AT+POWE" + test_case.second));
    }
}

TEST_F(AdvancedConfigurationTest, GetModulePower_Success) {
    auto driver = create_driver_with_get_response("2"); // Dbm0

    ModulePower power = ModulePower::DbmMinus23;
    UARTStatus status = UARTStatus::ErrError;
    driver.get_module_power(power, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(power, ModulePower::Dbm0);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+POWE?"));
}

TEST_F(AdvancedConfigurationTest, SetOutputPower_Success) {
    auto driver = create_driver_with_set_response("1"); // MaxPower = "1"

    UARTStatus status = UARTStatus::ErrError;
    driver.set_output_power(OutputPower::MaxPower, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+POWR1"));
}

TEST_F(AdvancedConfigurationTest, GetOutputPower_Success) {
    auto driver = create_driver_with_get_response("1"); // MaxPower

    OutputPower power = OutputPower::NormalPower;
    UARTStatus status = UARTStatus::ErrError;
    driver.get_output_power(power, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+POWR?"));
}

TEST_F(AdvancedConfigurationTest, SetModuleAutoSleep_Success) {
    auto driver = create_driver_with_set_response("1"); // Enabled

    UARTStatus status = UARTStatus::ErrError;
    driver.set_module_auto_sleep(true, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+PWRM1"));
}

TEST_F(AdvancedConfigurationTest, Sleep_Success) {
    std::vector<uint8_t> response_data{'O', 'K', '+', 'S', 'L', 'E', 'E', 'P'};
    auto driver = create_driver_with_response(response_data);

    UARTStatus status = UARTStatus::ErrError;
    driver.sleep(status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+SLEEP"));
}

TEST_F(AdvancedConfigurationTest, WakeUp_Success) {
    std::vector<uint8_t> response_data{'O', 'K'};
    auto driver = create_driver_with_response(response_data);

    UARTStatus status = UARTStatus::ErrError;
    driver.wake_up(status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT"));
}

// ============================================================================
// Advertising Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetAdvertisingInterval_Success) {
    auto driver = create_driver_with_set_response("5"); // Ms546 = "5"

    UARTStatus status = UARTStatus::ErrError;
    driver.set_advertising_interval(AdvertisingInterval::Ms546, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+ADVI5"));
}

TEST_F(AdvancedConfigurationTest, GetAdvertisingInterval_Success) {
    auto driver = create_driver_with_get_response("5"); // Ms546 = "5"

    AdvertisingInterval interval = AdvertisingInterval::Ms100;
    UARTStatus status = UARTStatus::ErrError;
    driver.get_advertising_interval(interval, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(interval, AdvertisingInterval::Ms546);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+ADVI?"));
}

TEST_F(AdvancedConfigurationTest, SetAdvertisingType_Success) {
    auto driver = create_driver_with_set_response("2"); // AdvertisingScanResponse = "2"

    UARTStatus status = UARTStatus::ErrError;
    driver.set_advertising_type(AdvertisingType::AdvertisingScanResponse, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+ADTY2"));
}

TEST_F(AdvancedConfigurationTest, SetAdvertisingFlag_Success) {
    auto driver = create_driver_with_set_response("1F"); // AdvertisingFlag = "1F"

    UARTStatus status = UARTStatus::ErrError;
    AdvertisingFlag flag("1F");
    driver.set_advertising_flag(flag, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+FLAG1F"));
}

// ============================================================================
// UART Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetUartBaudRate_Success) {
    auto driver = create_driver_with_set_response("3"); // Br57600 = "3"

    UARTStatus status = UARTStatus::ErrError;
    driver.set_uart_baud_rate(UartBaudRate::Br57600, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+BAUD3"));
}

// ============================================================================
// General Operation Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, Reset_Success) {
    std::vector<uint8_t> response_data{'O', 'K', '+', 'R', 'E', 'N', 'E', 'W'};
    auto driver = create_driver_with_response(response_data);

    UARTStatus status = UARTStatus::ErrError;
    driver.reset(status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+RENEW"));
}

TEST_F(AdvancedConfigurationTest, Restart_Success) {
    std::vector<uint8_t> response_data{'O', 'K', '+', 'R', 'E', 'S', 'E', 'T'};
    auto driver = create_driver_with_response(response_data);

    UARTStatus status = UARTStatus::ErrError;
    driver.restart(status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+RESET"));
}

// ============================================================================
// Connection Parameter Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetConnectTimeout_Success) {
    auto driver = create_driver_with_set_response("03E8"); // 10000ms = "03E8"

    UARTStatus status = UARTStatus::ErrError;
    ConnectTimeout timeout("03E8");
    driver.set_connect_timeout(timeout, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+COSU03E8"));
}

// ============================================================================
// White List Management Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetSaveConnectedMac_Enable) {
    auto driver = create_driver_with_set_response("1"); // Enabled

    UARTStatus status = UARTStatus::ErrError;
    driver.set_save_connected_mac(true, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+SAVE1"));
}

// ============================================================================
// PIO (GPIO) Control Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetPioCollectionRate_Success) {
    auto driver = create_driver_with_set_response("1"); // 100ms rate

    UARTStatus status = UARTStatus::ErrError;
    PioCollectionRate rate(1);
    driver.set_pio_collection_rate(rate, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+RATE1"));
}

// ============================================================================
// MAC Address Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetMacAddress_Success) {
    auto driver = create_driver_with_set_response("001122334455"); // Custom MAC

    UARTStatus status = UARTStatus::ErrError;
    MacAddress mac("001122334455");
    driver.set_mac_address(mac, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+ADDR001122334455"));
}

// ============================================================================
// Service and Characteristic UUID Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetServiceUuid_Success) {
    auto driver = create_driver_with_set_response("FFE0"); // Custom service UUID

    UARTStatus status = UARTStatus::ErrError;
    ServiceUuid uuid("FFE0");
    driver.set_service_uuid(uuid, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+UUIDFFE0"));
}

// ============================================================================
// Discovery and Scanning Configuration Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, SetDiscoveryMode_Success) {
    auto driver = create_driver_with_set_response("1"); // Auto discovery enabled

    UARTStatus status = UARTStatus::ErrError;
    driver.set_discovery_mode(DiscoveryMode::AutoDiscovery, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_TRUE(verify_transmitted_command(driver, "AT+DISC1"));
}

// ============================================================================
// Comprehensive Configuration Workflow Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, CompletePeripheralConfigurationWorkflow) {
    // Integration test for complete peripheral device configuration

    // Step 1: Set device name
    {
        auto driver = create_driver_with_set_response("MyDevice");
        UARTStatus status = UARTStatus::ErrError;
        DeviceName name("MyDevice");
        driver.set_device_name(name, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 2: Set peripheral role
    {
        auto driver = create_driver_with_set_response("0");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_role(Role::Peripheral, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 3: Configure advertising parameters
    {
        auto driver = create_driver_with_set_response("5");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_advertising_interval(AdvertisingInterval::Ms546, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 4: Configure connection timeout
    {
        auto driver = create_driver_with_set_response("000600");
        UARTStatus status = UARTStatus::ErrError;
        ConnectTimeout timeout("000600");
        driver.set_connect_timeout(timeout, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 5: Configure custom service UUID
    {
        auto driver = create_driver_with_set_response("FFE0");
        UARTStatus status = UARTStatus::ErrError;
        ServiceUUID uuid("FFE0");
        driver.set_service_uuid(uuid, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }
}

TEST_F(AdvancedConfigurationTest, CompleteCentralConfigurationWorkflow) {
    // Integration test for complete central device configuration

    // Step 1: Set central role
    {
        auto driver = create_driver_with_set_response("1");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_role(Role::Central, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 2: Enable MAC address saving
    {
        auto driver = create_driver_with_set_response("1");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_save_connected_mac(true, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 3: Configure discovery mode
    {
        auto driver = create_driver_with_set_response("1");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_discovery_mode(DiscoveryMode::AutoDiscovery, status);
        EXPECT_EQ(status, UARTStatus::Ok);
    }
}

// ============================================================================
// Error Handling and Edge Cases
// ============================================================================

TEST_F(AdvancedConfigurationTest, ConfigurationErrorHandling) {
    // Test error handling for various configuration failures

    // Test UART transmit error
    {
        MockUART error_uart;
        error_uart.SetForcedStatus(UARTStatus::ErrError);
        auto driver = make_hm11_driver(std::move(error_uart));

        UARTStatus status = UARTStatus::Ok;
        driver.set_module_power(ModulePower::Dbm0, status);

        EXPECT_EQ(status, UARTStatus::ErrError);
    }

    // Test UART receive timeout
    {
        MockUART timeout_uart;
        timeout_uart.SetForcedStatusForReceive(UARTStatus::ErrTimeout);
        auto driver = make_hm11_driver(std::move(timeout_uart));

        ModulePower power = ModulePower::DbmMinus23;
        UARTStatus status = UARTStatus::Ok;
        driver.get_module_power(power, status);

        EXPECT_EQ(status, UARTStatus::ErrTimeout);
    }

    // Test malformed response
    {
        std::vector<uint8_t> malformed_response{'O', 'K', '+', 'X', 'X', 'X'};
        auto driver = create_driver_with_response(malformed_response);

        ModulePower power = ModulePower::DbmMinus23;
        UARTStatus status = UARTStatus::Ok;
        driver.get_module_power(power, status);

        EXPECT_EQ(status, UARTStatus::ErrError);
    }
}

TEST_F(AdvancedConfigurationTest, ConfigurationParameterValidation) {
    // Test parameter validation at the type level

    // Valid parameters should not throw
    EXPECT_NO_THROW({
        ConnectTimeout valid_timeout("000300");
        PioCollectionRate valid_rate(10);
        ServiceUuid valid_service("FFE0");
        MacAddress valid_mac("001122334455");
    });

    // Invalid parameters should throw
    EXPECT_THROW(ConnectTimeout("GGGG"), std::exception); // Invalid hex
    EXPECT_THROW(ServiceUuid("GGG"), std::exception);     // Invalid hex in UUID
    EXPECT_THROW(MacAddress("INVALID"), std::exception);  // Invalid MAC
}

// ============================================================================
// Type Safety Tests
// ============================================================================

TEST_F(AdvancedConfigurationTest, TypeSafety_AdvertisingFlag) {
    // Test that AdvertisingFlag only accepts valid hex characters
    EXPECT_NO_THROW(AdvertisingFlag flag("1F"));
    EXPECT_NO_THROW(AdvertisingFlag flag("AB"));

    // These should throw due to validation (exact length and hex chars required)
    EXPECT_THROW(AdvertisingFlag flag("1"), std::exception);   // Too short
    EXPECT_THROW(AdvertisingFlag flag("123"), std::exception); // Too long
    EXPECT_THROW(AdvertisingFlag flag("GH"), std::exception);  // Invalid hex
}

TEST_F(AdvancedConfigurationTest, TypeSafety_AdvertisementData) {
    // Test that AdvertisementData requires exactly 12 hex characters
    EXPECT_NO_THROW(AdvertisementData data("123456789ABC"));

    // These should throw due to validation
    EXPECT_THROW(AdvertisementData data("12345"), std::exception);           // Too short
    EXPECT_THROW(AdvertisementData data("123456789ABCDEF"), std::exception); // Too long
    EXPECT_THROW(AdvertisementData data("123456789AGH"), std::exception);    // Invalid hex
}

TEST_F(AdvancedConfigurationTest, TypeSafety_MacAddress) {
    // Test that MacAddress requires exactly 12 hex characters
    EXPECT_NO_THROW(MacAddress mac("001122334455"));
    EXPECT_NO_THROW(MacAddress mac("AABBCCDDEEFF"));

    // These should throw due to validation
    EXPECT_THROW(MacAddress mac("00112233"), std::exception);       // Too short
    EXPECT_THROW(MacAddress mac("00112233445566"), std::exception); // Too long
    EXPECT_THROW(MacAddress mac("00112233GGFF"), std::exception);   // Invalid hex
}