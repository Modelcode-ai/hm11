// Comprehensive Integration Tests for HM11 Driver
// Tests complete workflows combining GATT, iBeacon, sensor, and configuration functionality
// Validates end-to-end operation scenarios and cross-feature interactions

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types.hpp"

using namespace hm11;
using namespace hm11::hal;

class IntegrationWorkflowTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Each test will create its own driver with specific mock setup
    }

    void TearDown() override {
        // Nothing needed - each test manages its own driver
    }

    // Helper to create response data from string
    std::vector<uint8_t> create_response(const std::string& response) {
        return std::vector<uint8_t>(response.begin(), response.end());
    }

    // Helper to create driver with specific mock response
    template <typename UARTType> auto make_test_driver(UARTType uart) { return make_hm11_driver(std::move(uart)); }

    // Verify transmitted command matches expected
    template <typename DriverType> void verify_command(const DriverType& driver, const std::string& expected) {
        const auto& transmitted = driver.GetUart().GetTransmitted8b();
        EXPECT_EQ(transmitted.size(), expected.size());
        if (transmitted.size() == expected.size()) {
            std::string actual(transmitted.begin(), transmitted.end());
            EXPECT_EQ(actual, expected);
        }
    }
};

// ============================================================================
// Complete GATT Client Workflow Integration Test
// ============================================================================

TEST_F(IntegrationWorkflowTest, CompleteGattClientWorkflow) {
    // Integration test demonstrating complete GATT client workflow:
    // 1. Set Central role
    // 2. Discover devices
    // 3. Connect to device
    // 4. Discover services
    // 5. Discover characteristics
    // 6. Enable notifications
    // 7. Read/Write characteristics
    // 8. Disconnect

    // Step 1: Set Central role
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_role(Role::Central, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+ROLE1");
    }

    // Step 2: Device discovery
    {
        MockUART uart;
        const std::string discovery_response = "OK+DISCS"
                                               "OK+DISC:001122334455"
                                               "OK+NAME:Heart Rate Monitor\r\n"
                                               "OK+RSSI:-45\r\n"
                                               "OK+DISCE";

        uart.SetReceiveData(create_response(discovery_response));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> discovered_devices;
        auto callback =
            [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
                auto* devices = static_cast<std::vector<std::string>*>(user_data);
                devices->push_back(std::string(mac) + "|" + std::string(name));
            };

        UARTStatus status = UARTStatus::ErrError;
        driver.scan(callback, &discovered_devices, 5000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(discovered_devices.size(), 1);
        if (!discovered_devices.empty()) {
            EXPECT_TRUE(discovered_devices[0].find("001122334455") != std::string::npos);
            EXPECT_TRUE(discovered_devices[0].find("Heart Rate Monitor") != std::string::npos);
        }
    }

    // Step 3: Connect to discovered device
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+CONN1")); // Connected
        auto driver = make_test_driver(std::move(uart));

        ConnectResult result = ConnectResult::OtherError;
        UARTStatus status = UARTStatus::ErrError;
        DiscoveredIndex index{0};
        driver.connect(index, result, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(result, ConnectResult::Connected);
    }

    // Step 4: Service discovery
    {
        MockUART uart;
        const std::string service_response = std::string(56, '*') + "\r\n" +
                                             "0001:0010:180D\r\n" + // Heart Rate Service
                                             "0011:0020:1800\r\n" + // Generic Access Service
                                             std::string(56, '*');

        uart.SetReceiveData(create_response(service_response));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> discovered_services;
        auto service_callback = [](std::string_view service, void* user_data) {
            auto* services = static_cast<std::vector<std::string>*>(user_data);
            services->emplace_back(service);
        };

        UARTStatus status = UARTStatus::ErrError;
        driver.find_all_services_uuid(service_callback, &discovered_services, 3000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(discovered_services.size(), 2);
        if (discovered_services.size() >= 2) {
            EXPECT_EQ(discovered_services[0], "0001:0010:180D");
            EXPECT_EQ(discovered_services[1], "0011:0020:1800");
        }
    }

    // Step 5: Characteristic discovery
    {
        MockUART uart;
        const std::string char_response = std::string(56, '*') + "\r\n" +
                                          "0002:RD|--|--|NO|--:2A37\r\n" + // Heart Rate Measurement (Read, Notify)
                                          "0003:RD|WR|--|--|--:2A38\r\n" + // Body Sensor Location (Read, Write)
                                          std::string(56, '*');

        uart.SetReceiveData(create_response(char_response));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> discovered_chars;
        auto char_callback = [](std::string_view characteristic, void* user_data) {
            auto* chars = static_cast<std::vector<std::string>*>(user_data);
            chars->emplace_back(characteristic);
        };

        UARTStatus status = UARTStatus::ErrError;
        HandleType from("0001");
        HandleType to("0010");
        driver.find_characteristics_uuid(from, to, char_callback, &discovered_chars, 3000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(discovered_chars.size(), 2);
        if (discovered_chars.size() >= 2) {
            EXPECT_EQ(discovered_chars[0], "0002:RD|--|--|NO|--:2A37");
            EXPECT_EQ(discovered_chars[1], "0003:RD|WR|--|--|--:2A38");
        }
    }

    // Step 6: Enable notifications on heart rate measurement
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType hr_handle("0002");
        UARTStatus status = UARTStatus::ErrError;
        NotifyResponse response;
        driver.enable_characteristic_notify(hr_handle, status, response);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(response, NotifyResponse::SendOk);
        verify_command(driver, "AT+NOTIFY_ON0002");
    }

    // Step 7: Read body sensor location
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType sensor_handle("0003");
        UARTStatus status = UARTStatus::ErrError;
        NotifyResponse response;
        driver.read_characteristic_notify(sensor_handle, status, response);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(response, NotifyResponse::SendOk);
        verify_command(driver, "AT+READDATA0003");
    }

    // Step 8: Configure data method for efficient communication
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType data_handle("0002");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_method_and_characteristic_handle(data_handle, SendDataMethod::Notify, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+SET_WAYNO0002");
    }

    // Step 9: Disconnect gracefully
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+LOST"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.disconnect(status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT");
    }
}

// ============================================================================
// Complete iBeacon Broadcasting Workflow Integration Test
// ============================================================================

TEST_F(IntegrationWorkflowTest, CompleteIBeaconBroadcastingWorkflow) {
    // Integration test for complete iBeacon broadcasting setup:
    // 1. Configure device as peripheral
    // 2. Set custom device name
    // 3. Enable iBeacon mode
    // 4. Configure iBeacon parameters (UUID, Major, Minor, Power)
    // 5. Set advertising parameters
    // 6. Deploy as broadcast-only beacon

    // Step 1: Set Peripheral role
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_role(Role::Peripheral, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+ROLE0");
    }

    // Step 2: Set custom device name
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:MyBeacon"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        DeviceName name("MyBeacon");
        driver.set_device_name(name, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+NAMEMyBeacon");
    }

    // Step 3: Enable iBeacon mode
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_switch(true, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+IBEA1");
    }

    // Step 4a: Configure iBeacon UUID (multi-part)
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:00x74278BDA")); // First part response
        auto driver = make_test_driver(std::move(uart));

        const IBeaconUuid beacon_uuid("74278BDAB644452080FC720EAF059935");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_uuid(beacon_uuid, status);

        // Verify first part of UUID setting was attempted
        const auto& transmitted = driver.GetUart().GetTransmitted8b();
        EXPECT_GT(transmitted.size(), 6);
        if (transmitted.size() >= 6) {
            std::string command_start(transmitted.begin(), transmitted.begin() + 6);
            EXPECT_EQ(command_start, "AT+IBE");
        }
    }

    // Step 4b: Configure Major version
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0xFFE0"));
        auto driver = make_test_driver(std::move(uart));

        const VersionType major("FFE0");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_major_version(major, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+MARJ0xFFE0");
    }

    // Step 4c: Configure Minor version
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0xFFE1"));
        auto driver = make_test_driver(std::move(uart));

        const VersionType minor("FFE1");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_minor_version(minor, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+MINO0xFFE1");
    }

    // Step 4d: Configure Measured Power
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0xC5"));
        auto driver = make_test_driver(std::move(uart));

        const MeasuredPower power("C5");
        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_measured_power(power, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+MEAS0xC5");
    }

    // Step 5: Configure advertising interval for optimal battery life
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:9"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_advertising_interval(AdvertisingInterval::Ms2000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+ADVI9");
    }

    // Step 6: Deploy as broadcast-only beacon (will reset device)
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_ibeacon_deploy_mode(IBeaconDeployMode::OnlyBroadcast, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+DELO1");
    }
}

// ============================================================================
// Complete HMSensor Monitoring Workflow Integration Test
// ============================================================================

TEST_F(IntegrationWorkflowTest, CompleteHMSensorMonitoringWorkflow) {
    // Integration test for complete HMSensor monitoring setup:
    // 1. Configure device as peripheral with custom name
    // 2. Set sensor type to DHT11
    // 3. Configure work interval
    // 4. Set up sensor service UUID
    // 5. Perform multiple sensor readings
    // 6. Test sensor range validation

    // Step 1: Set device name for sensor application
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:TempSensor"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        DeviceName name("TempSensor");
        driver.set_device_name(name, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+NAMETempSensor");
    }

    // Step 2: Configure sensor type as DHT11
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_sensor_type(SensorType::DHT11, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+SENS1");
    }

    // Step 3: Set work interval to 10 seconds
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:10"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        WorkInterval interval("10");
        driver.set_sensor_work_interval(interval, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+RAT10");
    }

    // Step 4: Configure custom service UUID for sensor data
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1810"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        ServiceUUID uuid("1810"); // Environmental Sensing Service
        driver.set_service_uuid(uuid, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+UUID1810");
    }

    // Step 5: Perform multiple sensor readings over time
    const std::vector<std::tuple<std::string, int, int>> sensor_readings = {
        {"023055", 23, 55}, // Morning reading
        {"025060", 25, 60}, // Midday reading
        {"027065", 27, 65}, // Afternoon reading
        {"024058", 24, 58}, // Evening reading
    };

    for (size_t i = 0; i < sensor_readings.size(); ++i) {
        const auto& [response_data, expected_temp, expected_humidity] = sensor_readings[i];

        MockUART uart;
        std::string full_response = "OK+Get:" + response_data;
        uart.SetReceiveData(create_response(full_response));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        TemperatureType temperature(0);
        HumidityType humidity(0);
        driver.get_sensor_temperature_and_humidity(temperature, humidity, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Reading " << i + 1 << " failed";
        EXPECT_EQ(temperature.value(), expected_temp) << "Temperature wrong for reading " << i + 1;
        EXPECT_EQ(humidity.value(), expected_humidity) << "Humidity wrong for reading " << i + 1;
        verify_command(driver, "AT+TEHU?");
    }

    // Step 6: Test sensor configuration retrieval
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Get:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        SensorType retrieved_type = SensorType::None;
        driver.get_sensor_type(retrieved_type, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(retrieved_type, SensorType::DHT11);
        verify_command(driver, "AT+SENS?");
    }

    // Step 7: Test work interval retrieval
    {
        MockUART uart;
        std::vector<uint8_t> interval_response = {'O', 'K', '+', 'G', 'e', 't', ':', '1', '0'};
        uart.SetReceiveData(interval_response);
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        WorkInterval retrieved_interval("00");
        driver.get_sensor_work_interval(retrieved_interval, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(retrieved_interval.view(), "10");
        verify_command(driver, "AT+RAT??");
    }
}

// ============================================================================
// Cross-Feature Integration Test (GATT + Sensors)
// ============================================================================

TEST_F(IntegrationWorkflowTest, GattSensorIntegrationWorkflow) {
    // Integration test combining GATT operations with sensor functionality:
    // Scenario: Central device connecting to HMSensor and reading temperature/humidity via GATT

    // Step 1: Set Central role for client device
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_role(Role::Central, status);

        EXPECT_EQ(status, UARTStatus::Ok);
    }

    // Step 2: Discover HMSensor devices
    {
        MockUART uart;
        const std::string sensor_discovery = "OK+DISCS"
                                             "OK+DISC:AABBCCDDEEFF"
                                             "OK+NAME:TempSensor\r\n"
                                             "OK+RSSI:-35\r\n"
                                             "OK+DISC:112233445566"
                                             "OK+NAME:HMSensor-01\r\n"
                                             "OK+RSSI:-50\r\n"
                                             "OK+DISCE";

        uart.SetReceiveData(create_response(sensor_discovery));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> sensor_devices;
        auto discovery_callback =
            [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
                auto* devices = static_cast<std::vector<std::string>*>(user_data);
                if (name.find("Sensor") != std::string_view::npos) {
                    devices->push_back(std::string(mac) + "|" + std::string(name));
                }
            };

        UARTStatus status = UARTStatus::ErrError;
        driver.scan(discovery_callback, &sensor_devices, 5000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(sensor_devices.size(), 2); // Should find both sensor devices
    }

    // Step 3: Connect to primary sensor device
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+CONN1"));
        auto driver = make_test_driver(std::move(uart));

        ConnectResult result = ConnectResult::OtherError;
        UARTStatus status = UARTStatus::ErrError;
        DiscoveredIndex sensor_index{0}; // Connect to first discovered sensor
        driver.connect(sensor_index, result, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(result, ConnectResult::Connected);
    }

    // Step 4: Discover environmental sensing service
    {
        MockUART uart;
        const std::string service_discovery = std::string(56, '*') + "\r\n" +
                                              "0001:0010:1810\r\n" + // Environmental Sensing Service
                                              "0011:0020:180F\r\n" + // Battery Service
                                              std::string(56, '*');

        uart.SetReceiveData(create_response(service_discovery));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> environmental_services;
        auto service_callback = [](std::string_view service, void* user_data) {
            auto* services = static_cast<std::vector<std::string>*>(user_data);
            if (service.find("1810") != std::string_view::npos) {
                services->emplace_back(service);
            }
        };

        UARTStatus status = UARTStatus::ErrError;
        driver.find_all_services_uuid(service_callback, &environmental_services, 3000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(environmental_services.size(), 1);
    }

    // Step 5: Discover temperature and humidity characteristics
    {
        MockUART uart;
        const std::string char_discovery = std::string(56, '*') + "\r\n" +
                                           "0002:RD|--|--|NO|--:2A6E\r\n" + // Temperature characteristic
                                           "0003:RD|--|--|NO|--:2A6F\r\n" + // Humidity characteristic
                                           "0004:RD|WR|--|--|--:2A7A\r\n" + // Heat Index characteristic
                                           std::string(56, '*');

        uart.SetReceiveData(create_response(char_discovery));
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> sensor_characteristics;
        auto char_callback = [](std::string_view characteristic, void* user_data) {
            auto* chars = static_cast<std::vector<std::string>*>(user_data);
            chars->emplace_back(characteristic);
        };

        HandleType from("0001");
        HandleType to("0010");
        UARTStatus status = UARTStatus::ErrError;
        driver.find_characteristics_uuid(from, to, char_callback, &sensor_characteristics, 3000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_GE(sensor_characteristics.size(), 3);
    }

    // Step 6: Enable notifications on temperature characteristic
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType temp_handle("0002");
        UARTStatus status = UARTStatus::ErrError;
        NotifyResponse response;
        driver.enable_characteristic_notify(temp_handle, status, response);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(response, NotifyResponse::SendOk);
    }

    // Step 7: Enable notifications on humidity characteristic
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType humidity_handle("0003");
        UARTStatus status = UARTStatus::ErrError;
        NotifyResponse response;
        driver.enable_characteristic_notify(humidity_handle, status, response);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(response, NotifyResponse::SendOk);
    }

    // Step 8: Read current sensor values via GATT
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SEND-OK"));
        auto driver = make_test_driver(std::move(uart));

        HandleType temp_handle("0002");
        UARTStatus status = UARTStatus::ErrError;
        NotifyResponse response;
        driver.read_characteristic_notify(temp_handle, status, response);

        EXPECT_EQ(status, UARTStatus::Ok);
        EXPECT_EQ(response, NotifyResponse::SendOk);
    }
}

// ============================================================================
// Power Management and Configuration Integration Test
// ============================================================================

TEST_F(IntegrationWorkflowTest, PowerManagementConfigurationWorkflow) {
    // Integration test for power-optimized device configuration:
    // Configure device for long-term battery operation

    // Step 1: Set optimal power level
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0")); // -23dBm for minimum power
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_module_power(ModulePower::DbmMinus23, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+POWE0");
    }

    // Step 2: Enable auto-sleep mode
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:1"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_module_auto_sleep(true, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+PWRM1");
    }

    // Step 3: Set long advertising interval for battery savings
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:9"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.set_advertising_interval(AdvertisingInterval::Ms2000, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+ADVI9");
    }

    // Step 4: Configure connection parameters for power optimization
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+Set:0050"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        ConnectionIntervalMin interval("0050"); // 100ms minimum for power savings
        driver.set_connection_interval_min(interval, status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+COMI0050");
    }

    // Step 5: Test sleep and wake functionality
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+SLEEP"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.sleep(status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT+SLEEP");
    }

    // Step 6: Wake up device
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK"));
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::ErrError;
        driver.wake_up(status);

        EXPECT_EQ(status, UARTStatus::Ok);
        verify_command(driver, "AT");
    }
}

// ============================================================================
// Error Recovery and Resilience Integration Test
// ============================================================================

TEST_F(IntegrationWorkflowTest, ErrorRecoveryResilienceWorkflow) {
    // Integration test for error handling and recovery across operations

    // Test 1: UART communication error recovery
    {
        MockUART uart;
        uart.SetForcedStatus(UARTStatus::ErrError); // Force transmit error
        auto driver = make_test_driver(std::move(uart));

        UARTStatus status = UARTStatus::Ok;
        driver.set_role(Role::Central, status);

        EXPECT_EQ(status, UARTStatus::ErrError); // Should propagate error
    }

    // Test 2: Malformed response handling
    {
        MockUART uart;
        uart.SetReceiveData(create_response("MALFORMED_RESPONSE"));
        auto driver = make_test_driver(std::move(uart));

        ModulePower power = ModulePower::Dbm0;
        UARTStatus status = UARTStatus::Ok;
        driver.get_module_power(power, status);

        EXPECT_EQ(status, UARTStatus::ErrError); // Should detect malformed response
    }

    // Test 3: Timeout handling in streaming operations
    {
        MockUART uart;
        uart.SetForcedStatusForReceive(UARTStatus::ErrTimeout);
        auto driver = make_test_driver(std::move(uart));

        std::vector<std::string> partial_results;
        auto callback = [](std::string_view device, void* user_data) {
            auto* results = static_cast<std::vector<std::string>*>(user_data);
            results->emplace_back(device);
        };

        UARTStatus status = UARTStatus::Ok;
        driver.scan_ibeacon(callback, &partial_results, 1000, status);

        EXPECT_EQ(status, UARTStatus::ErrTimeout); // Should handle timeout gracefully
    }

    // Test 4: Invalid parameter validation
    {
        // Type system should catch invalid parameters at compile/construction time
        EXPECT_THROW({ MacAddress invalid_mac("INVALID"); }, std::exception);

        EXPECT_THROW(
            {
                TemperatureType invalid_temp(200); // Out of range
            },
            std::exception);

        EXPECT_THROW({ IBeaconUuid invalid_uuid("TOO_SHORT"); }, std::exception);
    }

    // Test 5: Connection state error handling
    {
        MockUART uart;
        uart.SetReceiveData(create_response("OK+CONN0")); // Connection failed
        auto driver = make_test_driver(std::move(uart));

        ConnectResult result = ConnectResult::Connected;
        UARTStatus status = UARTStatus::ErrError;
        DiscoveredIndex index{0};
        driver.connect(index, result, status);

        EXPECT_EQ(status, UARTStatus::Ok);             // Command succeeded
        EXPECT_EQ(result, ConnectResult::ConnectFail); // But connection failed
    }
}