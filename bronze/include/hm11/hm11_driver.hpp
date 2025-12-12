#ifndef HM11_DRIVER_HPP
#define HM11_DRIVER_HPP

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "enum_types.hpp"
#include "gatt/gatt.hpp"
#include "uart_interface.hpp"
#include "util/fixed_string.hpp"
#include "util/ranged_integer.hpp"
#include "utils.hpp"

namespace hm11 {

// These enum classes are already declared in enum_types.hpp
// No forward declarations needed here

// Callback function types
using DiscoveryCallback =
    void (*)(char id, std::string_view mac_address, std::string_view name, std::string_view rssi, void* user_data);

using IBeaconDiscoveryCallback = void (*)(std::string_view device, void* user_data);

// Constants
constexpr std::size_t MAX_NAME_LENGTH = 248;
constexpr std::size_t MAX_RSII_LENGTH = 4;
constexpr std::size_t MAX_MESSAGE_LENGTH = 1413; // Defined in the Ada code

/**
 * @brief Variable string class for storing strings with a fixed capacity
 *
 * This class provides a way to store strings with a fixed maximum capacity,
 * similar to Ada's Variable_String type.
 *
 * @tparam Capacity The maximum capacity of the string
 */
template <std::size_t Capacity> class VariableString {
  public:
    /**
     * @brief Default constructor
     */
    VariableString() = default;

    /**
     * @brief Get the current length of the string
     * @return The length of the string
     */
    std::size_t length() const { return m_length; }

    /**
     * @brief Get the maximum capacity of the string
     * @return The capacity of the string
     */
    constexpr std::size_t capacity() const { return Capacity; }

    /**
     * @brief Get the string value
     * @return The string value
     */
    std::string_view value() const { return std::string_view(m_value.data(), m_length); }

    /**
     * @brief Append a string to this string
     *
     * @param str The string to append
     */
    void append(std::string_view str) {
        const std::size_t COPY_LEN = std::min(str.length(), Capacity - m_length);
        std::copy_n(str.data(), COPY_LEN, m_value.data() + m_length);
        m_length += COPY_LEN;
    }

    /**
     * @brief Clear the string
     */
    void clear() { m_length = 0; }

  private:
    std::array<char, Capacity> m_value{};
    std::size_t m_length{0};
};

/**
 * @brief Class to store information about a discovered device
 *
 * This class stores information about a device discovered during scanning,
 * similar to Ada's Discovered_Info_Type.
 */
class DiscoveredInfo {
  public:
    /**
     * @brief Default constructor
     */
    DiscoveredInfo() = default;

    /**
     * @brief Get the ID of the device
     * @return The ID character
     */
    char id() const { return m_id; }

    /**
     * @brief Set the ID of the device
     * @param id The ID character to set
     */
    void set_id(char id) { m_id = id; }

    /**
     * @brief Get the MAC address of the device
     * @return The MAC address as a string view
     */
    std::string_view mac() const { return {m_mac.data(), m_mac.size()}; }

    /**
     * @brief Set the MAC address of the device
     * @param mac The MAC address to set
     */
    void set_mac(std::string_view mac) {
        const std::size_t COPY_LEN = std::min(mac.length(), m_mac.size());
        // Create a safer copy with explicit size information
        std::string mac_str(mac);
        std::copy_n(mac_str.data(), COPY_LEN, m_mac.data());
    }

    /**
     * @brief Get the name of the device
     * @return The name as a string view
     */
    std::string_view name() const { return m_name.value(); }

    /**
     * @brief Get the RSSI of the device
     * @return The RSSI as a string view
     */
    std::string_view rssi() const { return m_rssi.value(); }

    /**
     * @brief Get the name VariableString object
     * @return Reference to the name VariableString
     */
    VariableString<MAX_NAME_LENGTH>& name_var() { return m_name; }

    /**
     * @brief Get the RSSI VariableString object
     * @return Reference to the RSSI VariableString
     */
    VariableString<MAX_RSII_LENGTH>& rssi_var() { return m_rssi; }

    /**
     * @brief Reset the device information
     */
    void reset() {
        m_id = ' ';
        std::ranges::fill(m_mac, '0');
        m_name.clear();
        m_rssi.clear();
    }

  private:
    char m_id{' '};
    std::array<char, 12> m_mac{}; // MAC_Address length is 12
    VariableString<MAX_NAME_LENGTH> m_name;
    VariableString<MAX_RSII_LENGTH> m_rssi;
};

/**
 * @brief Stages of the scanning process
 *
 * This enum class represents the different stages of the scanning process,
 * similar to Ada's Scan_Stage_Kind.
 */
enum class ScanStage : std::uint8_t {
    Selection, ///< Looking for prefixes
    MAC,       ///< Parsing MAC address
    Name,      ///< Parsing device name
    RSSI       ///< Parsing RSSI value
};

/**
 * @brief Main driver class for the HM-11/cc2541 BLE module
 *
 * This class provides the interface to communicate with and control an
 * HM-11 Bluetooth Low Energy module. It is templated on the UART implementation
 * to allow for different hardware interfaces.
 *
 * @tparam Port The UART port implementation type (must satisfy UartPort concept)
 * @tparam ReceiveHandlerImpl The receive handler implementation type (must satisfy ReceiveHandler concept)
 * @tparam ReadPositionHandlerImpl The read position handler implementation type (must satisfy LastReadPositionHandler
 * concept)
 */
template <
    UartPort Port,
    ReceiveHandler ReceiveHandlerImpl = decltype(default_receive_handler<Port>),
    LastReadPositionHandler ReadPositionHandlerImpl = std::function<void(bool&, std::size_t&)>>
class HM11Driver {
  public:
    /**
     * @brief Construct a new HM11Driver
     *
     * @param port Reference to the UART port to use for communication
     * @param receive_handler The handler function to use for receiving data
     * @param read_position_handler The handler function to use for tracking read position
     */
    HM11Driver(Port& port, ReceiveHandlerImpl receive_handler, ReadPositionHandlerImpl read_position_handler)
        : m_port(port), m_receive_handler(std::move(receive_handler)),
          m_read_position_handler(std::move(read_position_handler)) {
        // Initialize response buffer to all zeros
        std::ranges::fill(m_response, 0);
    }

    /**
     * @brief Destroy the HM11Driver
     *
     * The destructor is responsible for cleaning up resources.
     * It implements RAII principles by releasing resources on scope exit.
     */
    ~HM11Driver() = default;

    // Delete copy constructor and assignment operator (Design Decision #22)
    HM11Driver(const HM11Driver&) = delete;
    HM11Driver& operator=(const HM11Driver&) = delete;

    // Delete move constructor and assignment operator (Design Decision #22)
    HM11Driver(HM11Driver&&) = delete;
    HM11Driver& operator=(HM11Driver&&) = delete;

    /**
     * @brief Test if the HM-11 module is responsive
     *
     * This method sends a basic AT command to verify that the HM-11 module
     * is connected and responsive.
     *
     * @param status Output parameter that will be set to the operation status
     */
    void test(UartStatus& status) { transmit_and_check("AT", "OK", status); }

    /**
     * @brief Get the software version of the HM-11 module
     *
     * This method queries the HM-11 module for its software version and
     * returns it as a string.
     *
     * @param result Output parameter that will be set to the software version string
     * @param status Output parameter that will be set to the operation status
     */
    void software_version(std::string& result, UartStatus& status);

    /**
     * @brief Reset the HM-11 module to factory settings
     *
     * This method restores all setup values to factory defaults.
     *
     * @param status Output parameter that will be set to the operation status
     */
    void reset(UartStatus& status);

    /**
     * @brief Restart the HM-11 module
     *
     * This method reboots the HM-11 module.
     *
     * @param status Output parameter that will be set to the operation status
     */
    void restart(UartStatus& status);

    /**
     * @brief Put the HM-11 module into sleep mode
     *
     * This method puts the module into low power consumption mode.
     * Only supported in Peripheral role.
     *
     * @param status Output parameter that will be set to the operation status
     */
    void sleep(UartStatus& status);

    /**
     * @brief Wake up the HM-11 module from sleep mode
     *
     * This method wakes up the module from sleep mode.
     *
     * @param status Output parameter that will be set to the operation status
     */
    void wake_up(UartStatus& status);

    /**
     * @brief Get the role of the HM-11 module
     *
     * @param result Output parameter that will be set to the current role
     * @param status Output parameter that will be set to the operation status
     */
    void get_role(Role& result, UartStatus& status);

    /**
     * @brief Set the role of the HM-11 module
     *
     * @param value The role to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_role(Role value, UartStatus& status);

    /**
     * @brief Set the power level of the HM-11 module
     *
     * This method sets the transmission power level of the module.
     * Higher power levels increase range but consume more battery.
     *
     * @param power The power level to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_module_power(ModulePower power, UartStatus& status);

    /**
     * @brief Get the current power level of the HM-11 module
     *
     * @param power Output parameter that will be set to the current power level
     * @param status Output parameter that will be set to the operation status
     */
    void get_module_power(ModulePower& power, UartStatus& status);

    /**
     * @brief Set the auto-sleep mode of the HM-11 module
     *
     * This method enables or disables the module's automatic sleep mode.
     * When enabled, the module will automatically enter a low-power state when idle.
     * Only available in Peripheral role.
     *
     * @param enable True to enable auto-sleep, false to disable
     * @param status Output parameter that will be set to the operation status
     */
    void set_module_auto_sleep(bool enable, UartStatus& status);

    /**
     * @brief Get the current auto-sleep mode of the HM-11 module
     *
     * @param enabled Output parameter that will be set to true if auto-sleep is enabled, false otherwise
     * @param status Output parameter that will be set to the operation status
     */
    void get_module_auto_sleep(bool& enabled, UartStatus& status);

    /**
     * @brief Set the output power mode of the HM-11 module
     *
     * This method configures the output power mode of the module.
     * Added in V527 firmware.
     *
     * @param power The output power mode to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_output_power(OutputPower power, UartStatus& status);

    /**
     * @brief Get the current output power mode of the HM-11 module
     *
     * @param power Output parameter that will be set to the current output power mode
     * @param status Output parameter that will be set to the operation status
     */
    void get_output_power(OutputPower& power, UartStatus& status);

    /**
     * @brief Set the battery monitor switch
     *
     * This method enables or disables the battery monitoring feature.
     * When enabled, battery information is included in scan response data package.
     * Added in V520 firmware.
     *
     * @param enable True to enable battery monitoring, false to disable
     * @param status Output parameter that will be set to the operation status
     */
    void set_battery_monitor_switch(bool enable, UartStatus& status);

    /**
     * @brief Get the current battery monitor switch state
     *
     * @param enabled Output parameter that will be set to true if battery monitoring is enabled, false otherwise
     * @param status Output parameter that will be set to the operation status
     */
    void get_battery_monitor_switch(bool& enabled, UartStatus& status);

    /**
     * @brief Set the battery information
     *
     * This method manually sets the battery level information.
     * Only used when battery monitoring is disabled.
     * Added in V520 firmware.
     *
     * @param level The battery level percentage (0-100)
     * @param status Output parameter that will be set to the operation status
     */
    void set_battery_information(util::Percent level, UartStatus& status);

    /**
     * @brief Query the current battery information
     *
     * This method retrieves the current battery level.
     * Added in V520 firmware.
     *
     * @param level Output parameter that will be set to the battery level percentage (0-100)
     * @param status Output parameter that will be set to the operation status
     */
    void query_battery_information(util::Percent& level, UartStatus& status);

    /**
     * @brief Set the UART sleep type
     *
     * This method configures the behavior of the UART interface when the module enters sleep mode.
     * Only used for HMSensor version.
     *
     * @param type The UART sleep type to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_uart_sleep_type(UartSleepType type, UartStatus& status);

    /**
     * @brief Get the current UART sleep type
     *
     * @param type Output parameter that will be set to the current UART sleep type
     * @param status Output parameter that will be set to the operation status
     */
    void get_uart_sleep_type(UartSleepType& type, UartStatus& status);

    /**
     * @brief Set the minimum link layer connection interval
     *
     * This command is available since firmware V538.
     * Default value is 20ms.
     *
     * @param value The connection interval to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval value, UartStatus& status);

    /**
     * @brief Get the minimum link layer connection interval
     *
     * @param result Output parameter that will be set to the current minimum connection interval
     * @param status Output parameter that will be set to the operation status
     */
    void get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval& result, UartStatus& status);

    /**
     * @brief Set the maximum link layer connection interval
     *
     * This command is available since firmware V538.
     * Default value is 40ms.
     *
     * @param value The connection interval to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval value, UartStatus& status);

    /**
     * @brief Get the maximum link layer connection interval
     *
     * @param result Output parameter that will be set to the current maximum connection interval
     * @param status Output parameter that will be set to the operation status
     */
    void get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval& result, UartStatus& status);

    /**
     * @brief Set the link layer connection slave latency
     *
     * This command is available since firmware V538.
     * Default value is 0.
     *
     * @param value The slave latency to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_link_layer_connection_slave_latency(util::LayerConnectionLatency value, UartStatus& status);

    /**
     * @brief Get the link layer connection slave latency
     *
     * @param result Output parameter that will be set to the current slave latency
     * @param status Output parameter that will be set to the operation status
     */
    void get_link_layer_connection_slave_latency(util::LayerConnectionLatency& result, UartStatus& status);

    /**
     * @brief Set the connection supervision timeout
     *
     * This command is available since firmware V538.
     * Default value is 6000ms.
     *
     * @param value The supervision timeout to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_connection_supervision_timeout(ConnectionSupervisionTimeout value, UartStatus& status);

    /**
     * @brief Get the connection supervision timeout
     *
     * @param result Output parameter that will be set to the current supervision timeout
     * @param status Output parameter that will be set to the operation status
     */
    void get_connection_supervision_timeout(ConnectionSupervisionTimeout& result, UartStatus& status);

    /**
     * @brief Set the update connection flag
     *
     * This command is available since firmware V538.
     * This command is only used when the module is in Peripheral role. In Central role,
     * use set_minimum_link_layer_connection_interval and set_maximum_link_layer_connection_interval
     * to change default connection parameters.
     * Default value is true.
     *
     * @param value True to enable connection parameter updates, false to disable
     * @param status Output parameter that will be set to the operation status
     */
    void set_update_connection(bool value, UartStatus& status);

    /**
     * @brief Get the update connection flag
     *
     * Only used in Peripheral role.
     *
     * @param result Output parameter that will be set to the current update connection flag
     * @param status Output parameter that will be set to the operation status
     */
    void get_update_connection(bool& result, UartStatus& status);

    /**
     * @brief Set the timeout for connecting to a remote device
     *
     * Only used in Central role.
     * Default value is 000000ms (no timeout).
     * This command is only used in central role.
     * In central role, when module powers on, if module has a saved device MAC address,
     * the module will not enter scan procedure, but will try to connect to this device.
     * This command causes the module to enter a scan procedure if the setup value is not 000000.
     *
     * @param timeout The timeout value to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_connect_remote_device_timeout(util::ConnectTimeout timeout, UartStatus& status);

    /**
     * @brief Get the timeout for connecting to a remote device
     *
     * Only used in Central role.
     *
     * @param timeout Output parameter that will be set to the current timeout
     * @param status Output parameter that will be set to the operation status
     */
    void get_connect_remote_device_timeout(util::ConnectTimeout& timeout, UartStatus& status);

    /**
     * @brief Set the module name
     *
     * This method sets the name of the HM-11 module.
     * Default name is HMSoft.
     * Maximum length is 12 characters.
     *
     * @param name The name to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_module_name(std::string_view name, UartStatus& status);

    /**
     * @brief Get the module name
     *
     * This method retrieves the current name of the HM-11 module.
     *
     * @return The module name as a string
     * @param status Output parameter that will be set to the operation status
     */
    std::string get_module_name(UartStatus& status);

    /**
     * @brief Get the MAC address of the HM-11 module
     *
     * This method retrieves the MAC address of the module.
     * In iOS systems, you can't get module MAC address directly,
     * so the MAC address information is placed in the advertisement packet.
     *
     * @param mac Output parameter that will be set to the MAC address
     * @param status Output parameter that will be set to the operation status
     */
    void get_mac_address(std::string& mac, UartStatus& status);

    /**
     * @brief Set the UART baud rate
     *
     * This method configures the baud rate for UART communication.
     * Default is 9600.
     * Note: If setup to BR_1200, after next power on, the module will not support any AT Commands.
     *
     * @param baud_rate The baud rate to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_uart_baud_rate(UartBaudRate baud_rate, UartStatus& status);

    /**
     * @brief Get the current UART baud rate
     *
     * @param baud_rate Output parameter that will be set to the current baud rate
     * @param status Output parameter that will be set to the operation status
     */
    void get_uart_baud_rate(UartBaudRate& baud_rate, UartStatus& status);

    /**
     * @brief Set the UART flow control switch
     *
     * This method enables or disables UART flow control.
     * Default is disabled (false).
     *
     * @param enable True to enable flow control, false to disable
     * @param status Output parameter that will be set to the operation status
     */
    void set_uart_flow_control_switch(bool enable, UartStatus& status);

    /**
     * @brief Get the UART flow control switch state
     *
     * @param enabled Output parameter that will be set to true if flow control is enabled, false otherwise
     * @param status Output parameter that will be set to the operation status
     */
    void get_uart_flow_control_switch(bool& enabled, UartStatus& status);

    /**
     * @brief Set the parity bit for UART communication
     *
     * This method sets the parity bit mode for error detection.
     * Default is None.
     *
     * @param parity The parity bit setting to use
     * @param status Output parameter that will be set to the operation status
     */
    void set_parity_bit(ParityBit parity, UartStatus& status);

    /**
     * @brief Get the current parity bit setting
     *
     * @param parity Output parameter that will be set to the current parity bit setting
     * @param status Output parameter that will be set to the operation status
     */
    void get_parity_bit(ParityBit& parity, UartStatus& status);

    /**
     * @brief Set the stop bit for UART communication
     *
     * This method sets the stop bit mode for framing.
     * Default is OneStopBit.
     *
     * @param stop_bit The stop bit setting to use
     * @param status Output parameter that will be set to the operation status
     */
    void set_stop_bit(StopBit stop_bit, UartStatus& status);

    /**
     * @brief Get the current stop bit setting
     *
     * @param stop_bit Output parameter that will be set to the current stop bit setting
     * @param status Output parameter that will be set to the operation status
     */
    void get_stop_bit(StopBit& stop_bit, UartStatus& status);

    /**
     * @brief Set the module RX gain
     *
     * This method sets the RX gain of the module.
     * Added in V535 firmware.
     * Default is No RX gain.
     *
     * @param gain The RX gain setting to use
     * @param status Output parameter that will be set to the operation status
     */
    void set_module_rx_gain(RxGain gain, UartStatus& status);

    /**
     * @brief Get the current module RX gain setting
     *
     * @param gain Output parameter that will be set to the current RX gain setting
     * @param status Output parameter that will be set to the operation status
     */
    void get_module_rx_gain(RxGain& gain, UartStatus& status);

    /**
     * @brief Set the service UUID
     *
     * This method sets the service UUID for GATT operations.
     *
     * @param uuid The UUID to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_service_uuid(std::string_view uuid, UartStatus& status);

    /**
     * @brief Get the current service UUID
     *
     * @param uuid Output parameter that will be set to the current service UUID
     * @param status Output parameter that will be set to the operation status
     */
    void get_service_uuid(std::string& uuid, UartStatus& status);

    /**
     * @brief Set the characteristic
     *
     * This method sets the characteristic for GATT operations.
     * Default is FFE1.
     *
     * @param characteristic The characteristic to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_characteristic(std::string_view characteristic, UartStatus& status);

    /**
     * @brief Get the current characteristic
     *
     * @param characteristic Output parameter that will be set to the current characteristic
     * @param status Output parameter that will be set to the operation status
     */
    void get_characteristic(std::string& characteristic, UartStatus& status);

    /**
     * @brief Find all characteristic UUIDs on the connected device
     *
     * This method discovers all characteristic UUIDs on the connected BLE device
     * by sending the "AT+FINDALLCHARS?" command. When characteristics are discovered,
     * the provided callback function is invoked with the discovered characteristic information.
     *
     * Required state: after connect; Required role: Central.
     *
     * @param callback Function to be called for each discovered characteristic (format: [handle]:[properties]:[UUID])
     * @param user_data Optional user data to be passed to the callback function
     * @param timeout Timeout in milliseconds for the operation
     * @param status Output parameter that will be set to the operation status
     */
    // Function declaration moved to private section

    /**
     * @brief Find characteristic UUIDs within a specific service handle range
     *
     * This method discovers characteristic UUIDs within the specified handle range
     * on the connected BLE device by sending the "AT+CHAR[from][to]?" command.
     * When characteristics are discovered, the provided callback function is
     * invoked with the discovered characteristic information.
     *
     * Required state: after connect; Required role: Central.
     * The from and to values can be obtained from the find_all_services_uuid method.
     *
     * @param from Starting handle for the service range
     * @param to Ending handle for the service range
     * @param callback Function to be called for each discovered characteristic (format: [handle]:[properties]:[UUID])
     * @param user_data Optional user data to be passed to the callback function
     * @param timeout Timeout in milliseconds for the operation
     * @param status Output parameter that will be set to the operation status
     */
    // Function declaration moved to private section

    /**
     * @brief Enable notifications on a characteristic
     *
     * This method enables notifications on a characteristic with the given handle.
     * It allows the central device to receive updates from the characteristic without
     * having to poll for changes.
     *
     * This command was added in V700 firmware.
     * Required state: after connect; Required role: Central.
     * Only central role devices can use this command. The characteristic must have
     * the notify property.
     *
     * @param handle The handle of the characteristic to enable notifications on
     * @param status Output parameter that will be set to the operation status
     * @param response Output parameter that will be set to the notification response
     *
     * @pre The module must be in Central role
     * @pre The module must be connected to a device
     * @pre The characteristic must have the notify property
     * @post If status is UartStatus::Ok, the enable notification command has been sent
     * @post response will indicate the result of the notification operation
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void
    enable_characteristic_notify(const gatt::HandleType& handle, UartStatus& status, gatt::NotifyResponse& response);

    /**
     * @brief Disable notifications on a characteristic
     *
     * This method disables notifications on a characteristic with the given handle.
     * It stops the central device from receiving updates from the characteristic.
     *
     * This command was added in V700 firmware.
     * Required state: after connect; Required role: Central.
     * Only central role devices can use this command. The characteristic must have
     * the notify property.
     *
     * @param handle The handle of the characteristic to disable notifications on
     * @param status Output parameter that will be set to the operation status
     * @param response Output parameter that will be set to the notification response
     *
     * @pre The module must be in Central role
     * @pre The module must be connected to a device
     * @pre The characteristic must have the notify property
     * @post If status is UartStatus::Ok, the disable notification command has been sent
     * @post response will indicate the result of the notification operation
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void
    disable_characteristic_notify(const gatt::HandleType& handle, UartStatus& status, gatt::NotifyResponse& response);

    /**
     * @brief Read a characteristic's value
     *
     * This method reads the value of a characteristic with the given handle.
     * It can be used to read characteristics with the read property.
     *
     * This command was added in V700 firmware.
     * Required state: after connect; Required role: Central.
     * Only central role devices can use this command. The characteristic must have
     * the read property.
     *
     * @param handle The handle of the characteristic to read
     * @param status Output parameter that will be set to the operation status
     * @param response Output parameter that will be set to the notification response
     *
     * @pre The module must be in Central role
     * @pre The module must be connected to a device
     * @pre The characteristic must have the read property
     * @post If status is UartStatus::Ok, the read command has been sent
     * @post response will indicate the result of the read operation
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void read_characteristic_notify(const gatt::HandleType& handle, UartStatus& status, gatt::NotifyResponse& response);

    /**
     * @brief Scan for nearby BLE devices
     *
     * This method performs a scan for nearby Bluetooth Low Energy devices
     * by sending the "AT+DISC?" command. When devices are discovered, the
     * provided callback function is invoked with the discovered device information.
     *
     * The scanning process follows a state machine approach as described in
     * Design Decision 11, with explicit states for parsing the different parts
     * of the discovery data (MAC address, name, RSSI). The method continues
     * scanning until either the timeout is reached or an error occurs.
     *
     * If a timeout occurs during parsing of a device's information, the method
     * will complete the parsing of the current device before returning, in line
     * with Design Decision 28 (soft-stop timeout semantics).
     *
     * @param callback Function to be called when a device is discovered
     * @param timeout_ms Maximum time to scan in milliseconds
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role and work type set to Respond_AT_Command
     * @post If status is UartStatus::Ok, scanning completed successfully
     * @post Callback will be invoked for each discovered device
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see DiscoveryCallback for the callback signature
     * @see DiscoveredInfo for the device information structure
     * @see set_role() to set the module to Central role
     * @see set_work_type() to set the module to respond to AT commands
     */
    void scan(DiscoveryCallback callback, std::chrono::milliseconds timeout_ms, UartStatus& status);

    /**
     * @brief Scan for nearby iBeacon devices
     *
     * This method performs a scan specifically for iBeacon devices
     * by sending the "AT+DISI?" command. When iBeacon devices are discovered,
     * the provided callback function is invoked with the discovered device information.
     *
     * Like the scan method, this follows Design Decision 28 for soft-stop timeout
     * semantics, ensuring that any partially parsed device information is delivered
     * via the callback before the method returns on timeout.
     *
     * iBeacon information includes factory ID, UUID, major/minor values, measured power,
     * MAC address, and RSSI.
     *
     * @param callback Function to be called when an iBeacon device is discovered
     * @param timeout_ms Maximum time to scan in milliseconds
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role and work type set to Respond_AT_Command
     * @post If status is UartStatus::Ok, scanning completed successfully
     * @post Callback will be invoked for each discovered iBeacon device
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see IBeaconDiscoveryCallback for the callback signature
     * @see set_role() to set the module to Central role
     * @see set_work_type() to set the module to respond to AT commands
     * @see set_ibeacon_uuid() to configure the module as an iBeacon
     */
    void scan_ibeacon(IBeaconDiscoveryCallback callback, std::chrono::milliseconds timeout_ms, UartStatus& status);

    /**
     * @brief Set the iBeacon deployment mode
     *
     * This method sets the deployment mode for the module when operating
     * as an iBeacon. The mode controls whether the module only broadcasts
     * iBeacon packets or also scans for other devices.
     *
     * After setting this, the module will reset after about 500ms.
     *
     * @param mode The iBeacon deployment mode to set
     * @param status Output parameter that will be set to the operation status
     */
    void set_ibeacon_deploy_mode(IBeaconDeployMode mode, UartStatus& status);

    /**
     * @brief Connect to a device by MAC address
     *
     * This method attempts to connect to a device with the specified MAC address.
     * It requires the device to be in Central role.
     *
     * @param mac The MAC address of the device to connect to
     * @param result Output parameter that will be set to the connection result
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role
     * @pre The MAC address must be valid
     * @post If status is UartStatus::Ok, the connection attempt has been initiated
     * @post result will indicate the result of the connection attempt
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void connect(const util::MacAddress& mac, ConnectResult& result, UartStatus& status);

    /**
     * @brief Connect to a device by MAC address string
     *
     * This method attempts to connect to a device with the specified MAC address.
     * It requires the device to be in Central role.
     *
     * @param mac_str The MAC address string of the device to connect to
     * @param result Output parameter that will be set to the connection result
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role
     * @pre The MAC address string must be a valid MAC address format
     * @post If status is UartStatus::Ok, the connection attempt has been initiated
     * @post result will indicate the result of the connection attempt
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void connect(std::string_view mac_str, ConnectResult& result, UartStatus& status);

    /**
     * @brief Connect to the last connected device
     *
     * This method attempts to connect to the last device that was connected to.
     * It requires the device to be in Central role and to have previously connected
     * to a device.
     *
     * @param result Output parameter that will be set to the connection result
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role
     * @pre The module must have a saved device MAC address
     * @post If status is UartStatus::Ok, the connection attempt has been initiated
     * @post result will indicate the result of the connection attempt
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void connect_last_device(ConnectResult& result, UartStatus& status);

    /**
     * @brief Disconnect from the currently connected device
     *
     * This method disconnects from the currently connected device.
     *
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be connected to a device
     * @post If status is UartStatus::Ok, the disconnect request has been sent
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void disconnect(UartStatus& status);

    /**
     * @brief Set the method and characteristic handle for GATT operations
     *
     * This command configures the method and characteristic handle for sending
     * data to a BLE device. After executing this command, data can be sent without
     * additional AT commands.
     *
     * Added since V701 firmware.
     *
     * @param handle The characteristic handle to use
     * @param method The send data method to use
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role
     * @pre The module must be connected to a device
     * @pre The handle must be a valid characteristic handle
     * @post If status is UartStatus::Ok, the method and handle have been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_method_and_characteristic_handle(
        const gatt::HandleType& handle,
        gatt::SendDataMethod method,
        UartStatus& status);

    /**
     * @brief Send data to a characteristic
     *
     * This method sends data to a characteristic on the connected BLE device.
     * The characteristic must support the specified write method.
     *
     * Added since V700 firmware.
     *
     * @param handle The characteristic handle to write to
     * @param method The write method to use
     * @param data The data to send
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Central role
     * @pre The module must be connected to a device
     * @pre The handle must be a valid characteristic handle
     * @post If status is UartStatus::Ok, the data has been sent
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * Note: Since V701 firmware, it's recommended to use set_method_and_characteristic_handle
     * instead of this method for better performance when sending multiple data packets.
     */
    void send_data_to_characteristic(
        const gatt::HandleType& handle,
        gatt::SendDataCharacteristic method,
        std::span<const std::uint8_t> data,
        UartStatus& status);

    /**
     * @brief Set the number of characteristic UUIDs to use
     *
     * This method configures how many characteristic UUIDs the module should use.
     * Added since V550 firmware.
     *
     * @param count The number of characteristic UUIDs to use
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be in Peripheral role
     * @post If status is UartStatus::Ok, the characteristic UUID count has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_use_characteristic_uuid_count(gatt::CharacteristicUUIDCount count, UartStatus& status);

    /**
     * @brief Set the sensor type connected to the HM11 module
     *
     * This method configures the type of sensor connected to the HM11 module.
     * This command is only used for HMSensor version.
     * Default is None.
     *
     * @param type The sensor type to set
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be an HMSensor version
     * @post If status is UartStatus::Ok, the sensor type has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_sensor_type(SensorType type, UartStatus& status);

    /**
     * @brief Get the current sensor type connected to the HM11 module
     *
     * This method retrieves the type of sensor currently configured on the HM11 module.
     * This command is only used for HMSensor version.
     *
     * @param type Output parameter that will be set to the current sensor type
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be an HMSensor version
     * @post If status is UartStatus::Ok, type will contain the current sensor type
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void get_sensor_type(SensorType& type, UartStatus& status);

    /**
     * @brief Get the temperature and humidity from the connected sensor
     *
     * This method retrieves the current temperature and humidity readings from
     * the sensor connected to the HM11 module. The readings are returned as
     * numeric values matching Ada's approach.
     *
     * This command is only used for HMSensor version and requires a sensor to be
     * connected and configured.
     *
     * @param temperature Output parameter that will be set to the current temperature (0-120)
     * @param humidity Output parameter that will be set to the current humidity (0-100)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be an HMSensor version
     * @pre A sensor must be connected and configured using set_sensor_type
     * @post If status is UartStatus::Ok, temperature and humidity will contain the current readings
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see set_sensor_type to configure the connected sensor
     */
    void get_sensor_temperature_and_humidity(
        util::TemperatureType& temperature,
        util::HumidityType& humidity,
        UartStatus& status);

    /**
     * @brief Set the work interval for the sensor
     *
     * This method configures the work interval for the sensor connected to the HM11 module.
     * The interval determines how often sensor readings are taken.
     * This command is only used for HMSensor version.
     * Default is "00" (0 minutes).
     *
     * @param interval The work interval to set (00-99 minutes)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be an HMSensor version
     * @post If status is UartStatus::Ok, the work interval has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_sensor_work_interval(const util::WorkInterval& interval, UartStatus& status);

    /**
     * @brief Get the current work interval for the sensor
     *
     * This method retrieves the current work interval configured for the sensor
     * connected to the HM11 module.
     * This command is only used for HMSensor version.
     *
     * @param interval Output parameter that will be set to the current work interval
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The module must be an HMSensor version
     * @post If status is UartStatus::Ok, interval will contain the current work interval
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void get_sensor_work_interval(util::WorkInterval& interval, UartStatus& status);

    /**
     * @brief Set the advertising flag
     *
     * This method sets the flag information byte in the advertising package.
     * Added in firmware V530.
     *
     * The advertising flag controls various behaviors of the BLE advertising process,
     * such as discoverability and connectability modes.
     *
     * @param flag The advertising flag to set (2 hexadecimal characters)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The flag must be a valid 2-character hexadecimal value
     * @post If status is UartStatus::Ok, the advertising flag has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_advertising_flag(const util::AdvertisingFlag& flag, UartStatus& status);

    /**
     * @brief Set the humidity information byte in advertising data
     *
     * This method sets the humidity information byte in the advertising data.
     * Added in firmware V544.
     *
     * This allows broadcasting humidity sensor readings in the advertisement packet,
     * making it visible to scanning devices without requiring a connection.
     *
     * @param info The humidity information to set (2 hexadecimal characters)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The info must be a valid 2-character hexadecimal value
     * @post If status is UartStatus::Ok, the humidity information has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_humi_information_byte_in_advertising(const util::HumiInformation& info, UartStatus& status);

    /**
     * @brief Set the temperature information byte in advertising data
     *
     * This method sets the temperature information byte in the advertising data.
     * Added in firmware V544.
     *
     * This allows broadcasting temperature sensor readings in the advertisement packet,
     * making it visible to scanning devices without requiring a connection.
     *
     * @param temperature The temperature information to set (2 hexadecimal characters)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The temperature must be a valid 2-character hexadecimal value
     * @post If status is UartStatus::Ok, the temperature information has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_temperature_information_byte_in_advertising(
        const util::AdvertisingTemperatureType& temperature,
        UartStatus& status);

    /**
     * @brief Set the module advertisement data
     *
     * This method sets the custom advertisement data for the module.
     * Added in firmware V607/V702.
     *
     * Custom advertisement data allows transmitting application-specific data
     * in the advertisement packet, enabling more sophisticated device discovery
     * and selection logic.
     *
     * @param data The advertisement data to set (12 hexadecimal characters)
     * @param status Output parameter that will be set to the operation status
     *
     * @pre The data must be a valid 12-character hexadecimal value
     * @post If status is UartStatus::Ok, the advertisement data has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     */
    void set_module_advertisement_data(const util::AdvertisementData& data, UartStatus& status);

  private:
    /**
     * @brief Helper method to read and process characteristic UUID data
     *
     * This method reads streaming characteristic UUID data from the device and
     * invokes the provided callback function for each discovered characteristic.
     * It follows a state machine approach to parse the incoming data in chunks.
     *
     * @param callback Function to be called for each discovered characteristic
     * @param user_data Optional user data to be passed to the callback function
     * @param timeout Timeout in milliseconds for the operation
     * @param status Output parameter that will be set to the operation status
     */
    // Function declaration moved to private section

    /**
     * @brief Helper method to process notification response
     *
     * This private method processes the response from the HM-11 module after sending
     * a notification command (enable, disable, or read). It determines the outcome
     * of the operation based on the response string.
     *
     * @param status Input/output parameter for UART status
     * @param response Output parameter that will be set to the notification response
     *
     * @pre The notification command has been sent and status is UartStatus::Ok
     * @post If status remains UartStatus::Ok, response will be set based on the module's response
     */
    void read_characteristic_notify_response(UartStatus& status, gatt::NotifyResponse& response);

    // Constants
    static constexpr std::string_view OK_GET = "OK+Get:";
    static constexpr std::string_view OK_SET = "OK+Set:";

    // Member variables
    Port& m_port;
    ReceiveHandlerImpl m_receive_handler;
    ReadPositionHandlerImpl m_read_position_handler;
    std::array<std::uint8_t, MAX_MESSAGE_LENGTH * 5> m_response{};
    DiscoveredInfo m_discovered_info;

    /**
     * @brief Transmit a command to the HM-11 module
     *
     * @param command The command to transmit
     * @param cmdStatus Output parameter that will be set to the operation status
     */
    void transmit(std::string_view command, UartStatus& cmd_status) {
        // Create a temporary buffer for the command that can be written to
        std::vector<std::uint8_t> buffer(command.size());
        std::copy_n(command.data(), command.size(), buffer.data());

        // Create a span over the buffer
        auto data = std::span<std::uint8_t>(buffer);

        // Transmit the data
        m_port.transmit(data, cmd_status);
    }

    /**
     * @brief Transmit data to the HM-11 module
     *
     * @param command The command to transmit
     * @param data The data to transmit
     * @param status Output parameter that will be set to the operation status
     */
    void transmit(std::string_view command, std::span<const std::uint8_t> data, UartStatus& status) {
        // Create a temporary buffer to hold the command and data
        std::vector<std::uint8_t> buffer(command.size() + data.size());

        // Copy the command and data into the buffer
        for (std::size_t i = 0; i < command.size(); ++i) {
            buffer[i] = static_cast<std::uint8_t>(command[i]);
        }

        std::copy_n(data.data(), data.size(), buffer.data() + command.size());

        // Transmit the buffer
        m_port.transmit(buffer, status);
    }

    /**
     * @brief Check if a received response matches an expected value
     *
     * @param received The received data
     * @param length The length of the received data
     * @param expected The expected response
     * @param status Output parameter that will be set to the operation status
     */
    void check_response(const void* received, std::size_t length, std::string_view expected, UartStatus& status) {
        if (status != UartStatus::Ok) {
            return;
        }

        const auto* data = static_cast<const std::uint8_t*>(received);

        if (length < expected.size() || !utils::starts_with(data, length, expected)) {
            status = UartStatus::ErrError;
        }
    }

    /**
     * @brief Transmit a command and check the response
     *
     * @param command The command to transmit
     * @param expected The expected response
     * @param received The buffer to store the received data
     * @param length The length of the buffer
     * @param status Output parameter that will be set to the operation status
     */
    void transmit_and_check(
        std::string_view command,
        std::string_view expected,
        void* received,
        std::size_t length,
        UartStatus& status) {
        transmit(command, status);

        if (status != UartStatus::Ok) {
            return;
        }

        m_receive_handler(m_port, received, length, status);

        if (status != UartStatus::Ok) {
            return;
        }

        check_response(received, length, expected, status);
    }

    /**
     * @brief Transmit a command and check the response
     *
     * @param command The command to transmit
     * @param expected The expected response
     * @param status Output parameter that will be set to the operation status
     */
    void transmit_and_check(std::string_view command, std::string_view expected, UartStatus& status) {
        transmit_and_check(command, expected, m_response.data(), expected.size(), status);
    }

    /**
     * @brief Check if the response buffer starts with a specific string at a given position
     *
     * @param value The string to check for
     * @param from The starting position
     * @param to The ending position
     * @return true if the buffer starts with the string, false otherwise
     */
    bool starts_with(std::string_view value, std::size_t from, std::size_t /*to*/) const {
        return utils::starts_with(m_response.data(), m_response.size(), value, from);
    }

    /**
     * @brief Find a string in the response buffer
     *
     * @param value The string to find
     * @param from The starting position
     * @param to The ending position
     * @return The position where the string was found, or std::nullopt if not found
     */
    std::optional<std::size_t> find(std::string_view value, std::size_t from, std::size_t /*to*/) const {
        return utils::find(m_response.data(), m_response.size(), value, from);
    }

    /**
     * @brief Find the first zero byte in the response buffer
     *
     * @param from The starting position
     * @return The position of the first zero byte, or response_.size() if not found
     */
    std::size_t find_zero(std::size_t from) const {
        auto result = utils::find_zero(m_response.data(), m_response.size(), from);
        return result.value_or(m_response.size());
    }

    /**
     * @brief Check if the response buffer starts with a given string at a specific position
     *
     * @param str The string to check for
     * @param from The starting position in the response buffer
     * @param to The ending position in the response buffer
     * @return true if the response buffer starts with the given string, false otherwise
     */
    bool starts_with_str(std::string_view str, std::size_t from, std::size_t to) const {
        if (calc_length(from, to) < str.length()) {
            return false;
        }

        for (std::size_t i = 0; i < str.length(); ++i) {
            std::size_t pos = from + i;
            if (pos >= m_response.size()) {
                pos -= m_response.size();
            }

            if (static_cast<char>(m_response.at(pos)) != str[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Calculate the length between a position and a zero byte
     *
     * @param from The starting position
     * @param zero The position of the zero byte
     * @return The length between from and zero
     */
    std::size_t calc_length(std::size_t from, std::size_t zero) const {
        if (from == zero) {
            return 0;
        }

        if (from < zero) {
            return zero - from;
        }

        return (m_response.size() - from + 1) + (zero - 1);
    }

    /**
     * @brief Move a position forward and calculate the length to a zero byte
     *
     * @param result The position to move
     * @param add The amount to add to the position
     * @param zero The position of the zero byte
     * @param length Output parameter that will be set to the length between result and zero
     */
    void move(std::size_t& result, std::size_t add, std::size_t zero, std::size_t& length) {
        result += add;
        if (result >= m_response.size()) {
            result -= m_response.size();
        }
        length = calc_length(result, zero);
    }

    /**
     * @brief Append a substring from the response buffer to a VariableString
     *
     * @param str The VariableString to append to
     * @param from The starting position in the response buffer
     * @param to The ending position in the response buffer
     */
    template <std::size_t Capacity> void append(VariableString<Capacity>& str, std::size_t from, std::size_t to) {
        // Create temporary char buffers to avoid reinterpret_cast
        if (from <= to) {
            std::vector<char> buffer(to - from);
            for (std::size_t i = 0; i < (to - from); ++i) {
                buffer[i] = static_cast<char>(m_response.at(from + i));
            }
            str.append(std::string_view(buffer.data(), buffer.size()));
        } else {
            // Handle wrap-around - first part
            std::vector<char> buffer1(m_response.size() - from);
            for (std::size_t i = 0; i < (m_response.size() - from); ++i) {
                buffer1[i] = static_cast<char>(m_response.at(from + i));
            }
            str.append(std::string_view(buffer1.data(), buffer1.size()));

            // Handle wrap-around - second part
            std::vector<char> buffer2(to);
            for (std::size_t i = 0; i < to; ++i) {
                buffer2[i] = static_cast<char>(m_response.at(i));
            }
            str.append(std::string_view(buffer2.data(), buffer2.size()));
        }
    }

    /**
     * @brief Copy a substring from the response buffer to a buffer
     *
     * @param dest The destination buffer
     * @param from The starting position in the response buffer
     */
    void copy(std::span<std::uint8_t> dest, std::size_t from) {
        if (from + dest.size() <= m_response.size()) {
            // Simple case - no wrap-around
            std::copy_n(m_response.data() + from, dest.size(), dest.data());
        } else {
            // Handle wrap-around
            const std::size_t FIRST_PART = m_response.size() - from;
            std::copy_n(m_response.data() + from, FIRST_PART, dest.data());
            std::copy_n(m_response.data(), dest.size() - FIRST_PART, dest.data() + FIRST_PART);
        }
    }

    // Implementation of GATT characteristic discovery methods

    /**
     * Implementation of GATT characteristic discovery methods
     */
    void find_all_characteristic_uuid(
        gatt::CharacteristicUUIDCallback callback,
        void* user_data,
        std::uint32_t timeout,
        UartStatus& status) {
        this->transmit("AT+FINDALLCHARS?", status);
        if (status == UartStatus::Ok) {
            this->read_characteristic_uuid(callback, user_data, timeout, status);
        }
    }

    void find_characteristic_uuid(
        const gatt::HandleType& from,
        const gatt::HandleType& to,
        gatt::CharacteristicUUIDCallback callback,
        void* user_data,
        std::uint32_t timeout,
        UartStatus& status) {
        // Convert HandleType to string representation
        std::string command = "AT+CHAR";
        command += std::string(from.get_data(), gatt::HandleType::size());
        command += std::string(to.get_data(), gatt::HandleType::size());
        command += "?";

        this->transmit(command, status);
        if (status == UartStatus::Ok) {
            this->read_characteristic_uuid(callback, user_data, timeout, status);
        }
    }

    void read_characteristic_uuid(
        gatt::CharacteristicUUIDCallback callback,
        void* user_data,
        std::uint32_t timeout,
        UartStatus& status) {
        constexpr std::string_view HEADER = "********************************************************";
        bool started = false;
        std::array<std::uint8_t, 24> uuid_data{};
        std::vector<char> char_buffer(uuid_data.size());

        // Convert to char buffer using static_cast to avoid reinterpret_cast
        auto uuid_view = [&uuid_data, &char_buffer]() -> std::string {
            for (std::size_t i = 0; i < uuid_data.size(); ++i) {
                char_buffer[i] = static_cast<char>(uuid_data.at(i));
            }
            return {char_buffer.data(), char_buffer.size()};
        };

        std::size_t pos = 1;
        std::size_t zero = 0;
        bool stream_closed = false;
        std::size_t length = 0;

        // Start streaming receive
        m_receive_handler(m_port, m_response.data(), m_response.size(), status, timeout, true);

        try {
            while (true) {
                // Get current read position
                m_read_position_handler(stream_closed, zero);
                if (stream_closed) {
                    break;
                }

                length = calc_length(pos, zero);

                while (length >= uuid_data.size()) {
                    // Check if we're at the start of a header
                    if (m_response.at(pos) == '*') {
                        if (starts_with_str(HEADER, pos, zero - 1)) {
                            if (started) {
                                // End message, exit
                                goto end_processing;
                            } else {
                                // Start message
                                started = true;
                                move(pos, HEADER.length(), zero, length);
                            }
                        }
                    } else if (starts_with_str("\r\n", pos, zero - 1)) {
                        // Skip CR+LF
                        move(pos, 2, zero, length);
                    } else {
                        // Copy the characteristic data to the buffer
                        copy(uuid_data, pos);
                        move(pos, uuid_data.size(), zero, length);

                        // Call the callback with the UUID data
                        callback(std::string(uuid_view), user_data);
                    }
                }
            }

        end_processing:
            if (!stream_closed) {
                // Stop streaming and read any remaining data
                m_receive_handler(m_port, m_response.data(), m_response.size(), status);
            }
        } catch (...) {
            // Stop stream on any exception
            m_receive_handler(m_port, m_response.data(), m_response.size(), status);
            status = UartStatus::ErrError;
            // Re-throw the exception to allow proper handling at a higher level
            throw;
        }
    }
};

} // namespace hm11

#endif // HM11_DRIVER_HPP