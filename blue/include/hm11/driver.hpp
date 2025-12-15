#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/util/type_traits.hpp"

namespace hm11 {

/**
 * @brief Stub driver class for the HM11 Bluetooth module.
 */
class HM11Driver {
  public:
    // C-style callback typedefs (no heap allocation)
    using ServiceUuidCallback = void (*)(std::string_view service_info, void* user_data);
    using CharacteristicUuidCallback = void (*)(std::string_view characteristic_info, void* user_data);
    using DiscoveredCallback = void (*)(const std::array<char, 12>& mac, std::string_view name, std::string_view rssi, void* user_data);
    using DiscoveredIBeaconCallback = void (*)(std::string_view device_data, void* user_data);

    /**
     * @brief Construct driver with optional UARTPort implementation.
     *
     * Allows injection of a custom UARTPort (e.g., blocking, DMA‑based, or
     * interrupt‑driven) as described in Architecture Decision #3.
     */
    explicit HM11Driver(UARTPort& uart_port) : uart(uart_port) {}

    ~HM11Driver() = default;

    static void run();

    // API methods
    enum class IBeaconDeployMode : uint8_t { BroadcastScanning = 0, OnlyBroadcast = 1 };

    /**
     * @brief Test device connectivity.
     * @param status Output status of the operation.
     * @return true if device responded.
     */
    bool test(Status& status);

    /**
     * @brief Get firmware version.
     * @param version Output firmware version string view (points into internal buffer).
     * @param status Output status of the operation.
     * @return true if status is Ok.
     */
    bool software_version(std::string_view& version, Status& status);

    /**
     * @brief Set the role of the HM11 module (0 = peripheral, 1 = central).
     * @param role Role value (0 or 1).
     * @return true on success, false on invalid role or error.
     */
    bool set_role(unsigned int role);

    /**
     * @brief Get the current role of the HM11 module.
     * @param role Output role value (0 = peripheral, 1 = central).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool get_role(unsigned int& role, Status& status);

    /**
     * @brief Get module MAC address.
     * @param mac_address Output MAC address (12 hex characters).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool get_mac_address(std::array<char, 12>& mac_address, Status& status);

    /**
     * @brief Factory reset the module.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool reset(Status& status);

    /**
     * @brief Restart the module.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool restart(Status& status);

    /**
     * @brief Disconnect active connection.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool disconnect(Status& status);

    /**
     * @brief Start module operation (used when Work_Type = Respond_AT_Command).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool start_working(Status& status);

    /**
     * @brief Enter sleep mode (Peripheral role only).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool sleep(Status& status);

    /**
     * @brief Wake up from sleep mode.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool wake_up(Status& status);

    /**
     * @brief Set advertising interval.
     * @param interval Advertising interval value.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_advertising_interval(AdvertisingInterval interval, Status& status);

    /**
     * @brief Get advertising interval.
     * @param interval Output advertising interval value.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool get_advertising_interval(AdvertisingInterval& interval, Status& status);

    /**
     * @brief Set advertising type.
     * @param type Advertising type.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_advertising_type(AdvertisingType type, Status& status);

    /**
     * @brief Get advertising type.
     * @param type Output advertising type.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool get_advertising_type(AdvertisingType& type, Status& status);

    /**
     * @brief Set advertising flag.
     * @param flag Advertising flag (2 hex characters).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_advertising_flag(const util::AdvertisingFlag& flag, Status& status);

    /**
     * @brief Set temperature information byte in advertising data.
     * @param temp Temperature info (2 hex characters).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_temperature_information_byte_in_advertising(const util::AdvertisingTemperatureType& temp, Status& status);

    /**
     * @brief Set humidity information byte in advertising data.
     * @param humi Humidity info (2 hex characters).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_humi_information_byte_in_advertising(const util::HumiInformation& humi, Status& status);

    /**
     * @brief Set module advertisement data.
     * @param data Advertisement data (12 hex characters).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_module_advertisement_data(const util::AdvertisementData& data, Status& status);

    /**
     * @brief Set reliable advertising mode.
     * @param mode Advertising mode (normal or reliable).
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool set_reliable_advertising_mode(AdvertisingMode mode, Status& status);

    /**
     * @brief Get reliable advertising mode.
     * @param mode Output advertising mode.
     * @param status Output status of the operation.
     * @return true on success.
     */
    bool get_reliable_advertising_mode(AdvertisingMode& mode, Status& status);

    // === White List Methods ===
    bool set_white_list_switch(bool enable, Status& status);
    bool get_white_list_switch(bool& enabled, Status& status);
    bool set_white_list_mac_addresses(util::MacWhiteListIndex index, const util::MACAddress& mac, Status& status);
    bool get_white_list_mac_address(util::MacWhiteListIndex index, util::MACAddress& mac, Status& status);

    // === Battery Methods ===
    bool set_battery_monitor_switch(bool enable, Status& status);
    bool get_battery_monitor_switch(bool& enabled, Status& status);
    bool set_battery_information(util::Percent value, Status& status);
    bool query_battery_information(util::Percent& value, Status& status);

    // === UART Config Methods ===
    bool set_uart_baud_rate(BaudRate rate, Status& status);
    bool get_uart_baud_rate(BaudRate& rate, Status& status);
    bool set_uart_flow_control_switch(bool enable, Status& status);
    bool get_uart_flow_control_switch(bool& enabled, Status& status);
    bool set_parity_bit(ParityBit parity, Status& status);
    bool get_parity_bit(ParityBit& parity, Status& status);
    bool set_stop_bit(StopBit stop, Status& status);
    bool get_stop_bit(StopBit& stop, Status& status);
    bool set_uart_sleep_type(UARTSleepType type, Status& status);
    bool get_uart_sleep_type(UARTSleepType& type, Status& status);

    // === Link Layer Methods ===
    bool set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status);
    bool get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status);
    bool set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval interval, Status& status);
    bool get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval& interval, Status& status);
    bool set_link_layer_connection_slave_latency(util::LayerConnectionLatency latency, Status& status);
    bool get_link_layer_connection_slave_latency(util::LayerConnectionLatency& latency, Status& status);
    bool set_connection_supervision_timeout(ConnectionSupervisionTimeout timeout, Status& status);
    bool get_connection_supervision_timeout(ConnectionSupervisionTimeout& timeout, Status& status);
    bool set_update_connection(bool enable, Status& status);
    bool get_update_connection(bool& enabled, Status& status);

    // === Connection Methods ===
    bool clear_last_connected_address(Status& status);
    bool connect_last_device(ConnectResult& result, Status& status);
    bool connect(MACAddressType mac_type, const util::MACAddress& address, ConnectResult& result, Status& status);
    bool connect(util::DiscoveryIndex index, ConnectResult& result, Status& status);
    bool get_last_connected_device_address(util::MACAddress& mac, Status& status);
    bool set_save_connected_mac(bool save, Status& status);
    bool get_save_connected_mac(bool& save, Status& status);
    bool set_connect_remote_device_timeout(const util::ConnectTimeout& timeout, Status& status);
    bool get_connect_remote_device_timeout(util::ConnectTimeout& timeout, Status& status);

    // === Discovery Methods ===
    bool set_discovery_time(util::DiscoveryTime time, Status& status);
    bool get_discovery_time(util::DiscoveryTime& time, Status& status);
    bool set_show_device_information_when_discovery(ShowDeviceInformation show, Status& status);
    bool get_show_device_information_when_discovery(ShowDeviceInformation& show, Status& status);

    // === GATT Methods ===
    bool enable_characteristic_notify(const util::HandleType& handle, NotifyResponse& response, Status& status);
    bool disable_characteristic_notify(const util::HandleType& handle, NotifyResponse& response, Status& status);
    bool read_characteristic_notify(const util::HandleType& handle, NotifyResponse& response, Status& status);
    bool set_method_and_characteristic_handle(const util::HandleType& handle, SendDataMethod method, Status& status);
    bool set_use_characteristic_uuid_count(CharacteristicUUIDCount count, Status& status);
    bool set_characteristic(const util::CharacteristicType& value, Status& status);
    bool get_characteristic(util::CharacteristicType& result, Status& status);

    /**
     * @brief Discover all GATT services on connected peripheral.
     *
     * Sends AT+FINDALLSERVICES? command and parses the streaming response.
     * Service info format: start_handle:end_handle:uuid (e.g., "0001:000B:1800")
     *
     * @param callback C-style function pointer to call for each discovered service.
     * @param status Output status of the operation.
     * @param timeout_ms Timeout in milliseconds for the discovery operation.
     * @return true if status is Ok.
     */
    bool find_all_services_uuid(ServiceUuidCallback callback,
                                void* user_data,
                                Status& status,
                                std::uint32_t timeout_ms = 5000);

    /**
     * @brief Discover all GATT characteristics on connected peripheral.
     *
     * Sends AT+FINDALLCHARS? command and parses the streaming response.
     * Characteristic info format: handle:properties:uuid (e.g., "0002:RD|WR|--|--|--:2A00")
     *
     * @param callback C-style function pointer to call for each discovered characteristic.
     * @param status Output status of the operation.
     * @param timeout_ms Timeout in milliseconds for the discovery operation.
     * @return true if status is Ok.
     */
    bool find_all_characteristic_uuid(CharacteristicUuidCallback callback,
                                      void* user_data,
                                      Status& status,
                                      std::uint32_t timeout_ms = 5000);

    /**
     * @brief Discover GATT characteristics in a handle range.
     *
     * Sends AT+CHARxxxxYYYY? command and parses the streaming response.
     * Characteristic info format: handle:properties:uuid (e.g., "0002:RD|WR|--|--|--:2A00")
     *
     * @param from Start handle (4 hex characters).
     * @param to End handle (4 hex characters).
     * @param callback C-style function pointer to call for each discovered characteristic.
     * @param status Output status of the operation.
     * @param timeout_ms Timeout in milliseconds for the discovery operation.
     * @return true if status is Ok.
     */
    bool find_characteristic_uuid(const util::HandleType& from,
                                  const util::HandleType& to,
                                  CharacteristicUuidCallback callback,
                                  void* user_data,
                                  Status& status,
                                  std::uint32_t timeout_ms = 5000);

    /**
     * @brief Send data to a GATT characteristic.
     *
     * Sends AT+SEND_DATA[handle][method][data] command.
     *
     * @param handle Characteristic handle (4 hex characters).
     * @param method Send method (Write or WriteWithoutResponse).
     * @param data Data bytes to send.
     * @param status Output status of the operation.
     * @return true if status is Ok.
     */
    bool send_data_to_characteristic(const util::HandleType& handle,
                                     SendDataCharacteristic method,
                                     std::span<const uint8_t> data,
                                     Status& status);

    // === Service UUID Methods ===
    bool set_service_uuid(const util::UUID& value, Status& status);
    bool get_service_uuid(util::UUID& result, Status& status);

    // === Power Methods ===
    bool set_module_power(ModulePower power, Status& status);
    bool get_module_power(ModulePower& power, Status& status);
    bool set_output_power(OutputPower power, Status& status);
    bool get_output_power(OutputPower& power, Status& status);
    bool set_module_auto_sleep(bool auto_sleep, Status& status);
    bool get_module_auto_sleep(bool& auto_sleep, Status& status);

    // === Module Config Methods ===
    bool set_work_type(WorkType type, Status& status);
    bool get_work_type(WorkType& type, Status& status);
    bool set_work_mode(WorkMode mode, Status& status);
    bool get_work_mode(WorkMode& mode, Status& status);
    bool set_notify_mode(NotifyMode mode, Status& status);
    bool get_notify_mode(NotifyMode& mode, Status& status);
    bool set_module_name(std::string_view name, Status& status);
    bool get_module_name(std::string_view& name, Status& status);
    bool set_notify_information(bool notify, Status& status);
    bool get_notify_information(bool& notify, Status& status);
    bool set_module_rx_gain(RXGain gain, Status& status);
    bool get_module_rx_gain(RXGain& gain, Status& status);

    // === PIO Control Methods ===
    bool set_pio_output_status(util::PioNumber pio, PIOOutput output, Status& status);
    bool get_pio_output_status(util::PioNumber pio, PIOOutput& output, Status& status);
    bool get_pios_output_status(util::PIONumbers& pios, Status& status);
    bool set_pios_output_status(const util::PIONumbers& pios, Status& status);
    bool set_pio_collection_rate(util::PioCollectionRate rate, Status& status);
    bool get_pio_collection_rate(util::PioCollectionRate& rate, Status& status);
    bool set_power_pin_output(const util::PIONumbers& pios, Status& status);
    bool get_power_pin_output(util::PIONumbers& pios, Status& status);
    bool set_connect_pin_output(const util::PIONumbers& pios, Status& status);
    bool get_connect_pin_output(util::PIONumbers& pios, Status& status);

    // === Security Methods ===
    bool set_bond_mode(BondMode mode, Status& status);
    bool get_bond_mode(BondMode& mode, Status& status);
    bool set_pin_code(const util::PINType& pin, Status& status);
    bool get_pin_code(util::PINType& pin, Status& status);
    bool remove_bond_information(Status& status);

    /**
     * @brief Set the iBeacon UUID.
     * @param uuid UUID to set.
     * @return true on success.
     */
    bool set_ibeacon_uuid(const util::UUID& uuid);
    /**
     * @brief Set iBeacon deployment mode.
     * @param mode Deployment mode.
     */
    bool set_ibeacon_deploy_mode(IBeaconDeployMode mode);
    /**
     * @brief Set iBeacon switch (enable/disable).
     * @param enable true to enable, false to disable.
     */
    bool set_ibeacon_switch(bool enable);
    /**
     * @brief Set iBeacon major version (4 hex chars).
     */
    bool set_ibeacon_major(const std::array<char, 4>& version);
    /**
     * @brief Set iBeacon minor version (4 hex chars).
     */
    bool set_ibeacon_minor(const std::array<char, 4>& version);
    /**
     * @brief Set iBeacon measured power (2 hex chars).
     */
    bool set_ibeacon_measured_power(const std::array<char, 2>& power);
    bool get_ibeacon_switch(bool& enabled);
    bool get_ibeacon_uuid(std::array<char, 32>& uuid_out);
    bool get_ibeacon_major(util::VersionType& major, Status& status);
    bool get_ibeacon_minor(util::VersionType& minor, Status& status);
    bool get_ibeacon_measured_power(util::MeasuredPower& power, Status& status);

    /**
     * @brief Get iBeacon UUID.
     * @param uuid_out output, iBeacon UUID.
     */
    enum class SensorType : uint8_t { None = 0, DHT11 = 1, DS18B20 = 2 };
    using WorkInterval = std::array<char, 2>;

    /**
     * @brief Set HMSensor type.
     * @param value Sensor type value.
     */
    bool set_sensor_type(SensorType value);

    /**
     * @brief Get HMSensor type.
     * @param result Output sensor type.
     */
    bool get_sensor_type(SensorType& result);

    /**
     * @brief Set sensor work interval.
     * @param interval Interval string (2 chars).
     */
    bool set_sensor_work_interval(const WorkInterval& interval);

    /**
     * @brief Get sensor work interval.
     * @param interval Output interval.
     */
    bool get_sensor_work_interval(WorkInterval& interval);

    /**
     * @brief Get sensor temperature and humidity.
     * @param temperature Output temperature (0..120).
     * @param humidity Output humidity (0..100) as Percent.
     * @return true on success.
     */
    bool get_sensor_temperature_and_humidity(util::Temperature& temperature, util::Percent& humidity);
    bool get_module_temperature(util::InternalTemperatureType& temperature, Status& status);

    /**
     * @brief Send raw data over UART.
     * @param data Data to send.
     * @param status Output status of the operation.
     * @return true if status is Ok.
     */
    bool send(std::string_view data, Status& status);

    /**
     * @brief Receive data from UART into a fixed internal buffer.
     * @param out Output string_view pointing to received data in internal buffer.
     * @param status Output status of the operation.
     * @param max_len Maximum number of bytes to receive (default 256).
     * @return true if status is Ok.
     */
    bool receive(std::string_view& out, Status& status, std::size_t max_len = 256);

    /**
     * @brief Scan for nearby Bluetooth devices.
     *
     * Sends AT+DISC? command and parses the streaming response containing
     * device information (MAC, name, RSSI). The callback is invoked for each
     * discovered device.
     *
     * @param callback C-style function pointer to call for each discovered device.
     * @param status Output status of the operation.
     * @param timeout_ms Timeout in milliseconds for the scan operation.
     * @return true if status is Ok.
     */
    bool scan(DiscoveredCallback callback,
              void* user_data,
              Status& status,
              std::uint32_t timeout_ms = 5000);

    /**
     * @brief Scan for nearby iBeacon devices.
     *
     * Sends AT+DISI? command and parses the streaming response containing
     * iBeacon device information. The callback receives a string in format:
     * FactoryID(8):UUID(32):MajorMinorPower(10):MAC(12):RSSI(4)
     *
     * @param callback C-style function pointer to call for each discovered iBeacon device.
     * @param status Output status of the operation.
     * @param timeout_ms Timeout in milliseconds for the scan operation.
     * @return true if status is Ok.
     */
    bool scan_ibeacon(DiscoveredIBeaconCallback callback,
                      void* user_data,
                      Status& status,
                      std::uint32_t timeout_ms = 5000);

    // Transmit overloads
    template <std::size_t N> void transmit(const std::array<char, N>& cmd) {
        // Copy command to buffer, stopping at null terminator or buffer capacity
        last_command_size_ = 0;
        for (std::size_t i = 0; i < N && i < last_command_buffer_.size(); ++i) {
            if (cmd[i] == '\0') {
                break;
            }
            last_command_buffer_[i] = cmd[i];
            ++last_command_size_;
        }
    }
    void transmit(const std::string_view CMD, Status& status) { uart.transmit(CMD, status); }
    void receive(std::span<uint8_t> response, Status& status) { uart.receive(response, status); }
    static void check_response(std::span<uint8_t> response, const std::string_view EXPECTED, Status& status) {
        if (response.size() < EXPECTED.size()) {
            status = Status::InvalidLength;
            return;
        }
        for (std::size_t i = 0; i < EXPECTED.size(); ++i) {
            if (response[i] != static_cast<uint8_t>(EXPECTED[i])) {
                status = Status::InvalidResponse;
                return;
            }
        }
        status = Status::Ok;
    }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void transmit_and_check(
        const std::string_view CMD,
        const std::string_view EXPECTED,
        std::span<uint8_t> response,
        Status& status) {
        if (status != Status::Ok) {
            return;
        }

        transmit(CMD, status);
        if (status != Status::Ok) {
            return;
        }

        receive(response, status);
        if (status != Status::Ok) {
            return;
        }

        check_response(response, EXPECTED, status);
    }
    void transmit_and_check(const std::string_view CMD, const std::string_view EXPECTED, Status& status) {
        transmit_and_check(CMD, EXPECTED, response_buffer, status);
    }

    std::string_view get_last_command() const { return {last_command_buffer_.data(), last_command_size_}; }

    // Disallow copy/move
    HM11Driver(const HM11Driver&) = delete;
    HM11Driver& operator=(const HM11Driver&) = delete;
    HM11Driver(HM11Driver&&) = delete;
    HM11Driver& operator=(HM11Driver&&) = delete;

  private:
    UARTPort& uart;           // injected UART abstraction
    std::array<char, 128> last_command_buffer_{};  // stored command for testing
    std::size_t last_command_size_{0};             // length of last command
    bool get_ibeacon_uuid_chunk(std::size_t pos, std::array<char, 32>& uuid_out);
    std::array<uint8_t, 128> response_buffer{};
    std::array<uint8_t, 1024> receive_buffer_{};   // buffer for public receive() method
};

} // namespace hm11
