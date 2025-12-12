/**
 * @file driver.hpp
 * @brief HM11 BLE driver class implementation
 *
 * This file defines the HM11Driver template class that provides an interface for
 * communicating with the HM-11/cc2541 Bluetooth Low Energy module.
 *
 * This implementation follows Decision 1 (Dependency Injection Mechanism) by making
 * the driver a class template parameterized by UARTImpl type, as well as
 * Decision 22 (RAII Ownership Model) with deleted copy/move operations.
 */

#ifndef HM11_DRIVER_HPP
#define HM11_DRIVER_HPP

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "gatt/types.hpp"
#include "hal/uart.hpp"
#include "hm11.hpp"
#include "util/fixed_string.hpp"
#include "util/ranged_integer.hpp"

namespace hm11 {

/**
 * @brief Types of discovered Bluetooth devices
 */
enum class DiscoveredType : std::uint8_t {
    Normal,  ///< Standard BLE device
    IBeacon, ///< Apple iBeacon device
    HMSensor ///< HMSensor device
};

/**
 * @brief Maximum response message length
 */
constexpr std::size_t MAX_MESSAGE_LENGTH = 256;

/**
 * @brief Maximum device name length
 */
constexpr std::size_t MAX_NAME_LENGTH = 248;

/**
 * @brief Maximum RSSI info length
 */
constexpr std::size_t MAX_RSII_LENGTH = 4;

/**
 * @brief Type definition for receive handler callback
 *
 * This function is called when data is received from the UART port.
 * It matches the Ada access to procedure type for the receive handler.
 *
 * @param port UART port that received the data
 * @param received Buffer containing the received data
 * @param length Length of the received data
 * @param status Output parameter for operation status
 * @param timeout_ms Timeout in milliseconds (typically 1000ms)
 * @param as_stream Whether the reception is part of a stream (typically false)
 */
using ReceiveHandler = std::function<void(
    hal::AnyUartPort port,
    void* received,
    std::size_t length,
    hal::UartStatus& status,
    std::chrono::milliseconds timeout_ms,
    bool as_stream)>;

/**
 * @brief Type definition for last read position handler callback
 *
 * This function is called to determine the last read position.
 * It matches the Ada access to procedure type for the last read position handler.
 *
 * @param closed Output parameter indicating if the stream is closed
 * @param zero Output parameter for the zero position
 */
using LastReadPositionHandler = std::function<void(bool& closed, std::size_t& zero)>;

// NOTE: The DiscoveryCallback is now defined in hm11.hpp as a C-style function pointer
// to implement Decision 10 (plain C-style function pointers for callbacks)

/**
 * @brief Structure for storing discovered device information
 *
 * This structure implements Decision 27 regarding discovery data lifetime and storage.
 * It uses fixed-capacity arrays with length tracking for Name and RSSI fields, similar
 * to Ada's Variable_String type. During discovery, a single temporary instance of this
 * structure is used to accumulate data while parsing the streaming data.
 *
 * When a complete device is parsed (MAC + Name + RSSI), the callback is invoked
 * with the data, and then the temporary structure is reset for the next device.
 */
struct DiscoveredInfo {
    char id = '\0';                           ///< Identifier character for device type
    util::MacAddress mac;                     ///< MAC address (12 hex characters)
    std::array<char, MAX_NAME_LENGTH> name{}; ///< Device name buffer (fixed capacity)
    std::size_t name_length = 0;              ///< Current length of the device name
    std::array<char, MAX_RSII_LENGTH> rssi{}; ///< RSSI value buffer (fixed capacity)
    std::size_t rssi_length = 0;              ///< Current length of the RSSI value

    /**
     * @brief Resets the structure for a new device
     *
     * Clears all fields to prepare for parsing a new device.
     */
    void reset() {
        id = '\0';
        mac = util::MacAddress();
        name.fill('\0');
        name_length = 0;
        rssi.fill('\0');
        rssi_length = 0;
    }

    /**
     * @brief Gets a string_view of the current name
     *
     * @return std::string_view View of the current name with actual length
     */
    std::string_view get_name_view() const { return {name.data(), name_length}; }

    /**
     * @brief Gets a string_view of the current RSSI
     *
     * @return std::string_view View of the current RSSI with actual length
     */
    std::string_view get_rssi_view() const { return {rssi.data(), rssi_length}; }
};

/**
 * @brief HM11 Bluetooth Low Energy module driver class
 *
 * This class provides an interface for communicating with the HM-11/cc2541
 * Bluetooth Low Energy module. It is a template class that is parameterized
 * by the UART implementation type.
 *
 * The driver implements a complete API for configuring and operating the HM-11 module,
 * including support for both peripheral and central modes, GATT operations,
 * and iBeacon functionality.
 *
 * This implementation follows the same API design as the original Ada driver,
 * making it easy to migrate existing code. It also provides additional features
 * such as strong type safety and ISR-safe synchronization mechanisms.
 *
 * Thread safety: This class is not thread-safe and should be accessed from a single thread.
 * However, it does support ISR-to-thread communication via specific mechanisms.
 *
 * @tparam UARTImpl UART implementation that satisfies the UART_Implementation concept
 *
 * @see hal::UART_Implementation for the requirements of the UART implementation
 * @see docs/thread_safety.md for thread safety and ISR handling details
 * @see docs/architecture.md for design decisions and architecture
 */
template <hal::UART_Implementation UARTImpl> class HM11Driver {
  public:
    /**
     * @brief Construct a new HM11Driver object
     *
     * Creates a new HM11 driver instance with the provided UART implementation and handlers.
     * The driver takes ownership of the UART implementation and will use it for all
     * communication with the HM-11 module.
     *
     * The receive_handler is used to handle data reception from the module, while the
     * last_read_handler is used to track the last read position in streaming operations.
     *
     * Note that this follows Decision 22 (RAII Ownership Model), making the driver
     * non-copyable and non-movable. Instances should be allocated on the stack and
     * will automatically clean up resources when destroyed.
     *
     * @param uart_impl UART implementation for communication
     * @param handler Handler for receiving data
     * @param read_handler Handler for last read position
     *
     * @pre uart_impl must satisfy the UART_Implementation concept
     * @pre handler must be a valid function that can receive data
     * @pre read_handler must be a valid function that can track position
     *
     * Thread safety: This constructor is not thread-safe and should only be called
     * from a single thread, typically during initialization.
     */
    HM11Driver(UARTImpl uart_impl, ReceiveHandler handler, LastReadPositionHandler read_handler)
        : uart(std::move(uart_impl)), receive_handler(std::move(handler)), last_read_handler(std::move(read_handler)),
          discovered_info{} {}

    /**
     * @brief Destructor
     *
     * Handles graceful cleanup of resources. This includes:
     * 1. Attempting to gracefully stop any ongoing operations
     * 2. Waiting up to 100ms for operations to complete
     * 3. Forcing operations to stop if timeout is exceeded
     * 4. Unregistering any IRQ handlers if needed
     * 5. Releasing resources owned by the driver
     *
     * Thread safety: The destructor is not thread-safe and should only be called
     * from a single thread, typically when the owning scope exits.
     */
    ~HM11Driver() = default;

    // Delete copy constructor and assignment operator
    HM11Driver(const HM11Driver&) = delete;
    HM11Driver& operator=(const HM11Driver&) = delete;

    // Delete move constructor and assignment operator
    HM11Driver(HM11Driver&&) noexcept = delete;
    HM11Driver& operator=(HM11Driver&&) noexcept = delete;

    /**
     * @brief Test the HM11 module connection
     *
     * Sends the AT command to check if the module responds. This is typically
     * the first operation performed with the driver to verify that the module
     * is correctly connected and responsive.
     *
     * The function sends "AT" and expects "OK" as a response. If the module
     * responds correctly, the status will be set to UartStatus::Ok.
     *
     * @param status Output parameter for operation status
     *
     * @post If status is UartStatus::Ok, the module is properly connected
     * @post If status is not UartStatus::Ok, the module is not responsive
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     */
    void test(hal::UartStatus& status);

    /**
     * @brief Get the software version of the HM11 module
     *
     * This method retrieves the firmware version from the module by sending
     * the "AT+VERS?" command. The version is returned as a VersionType object,
     * which is a fixed-size string of 4 hexadecimal characters.
     *
     * The version information is important for determining which features
     * are supported, as different firmware versions have different capabilities.
     *
     * @param version Output parameter for version information (4 hex characters)
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, version contains valid version information
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::VersionType for the version format
     * @see hal::UartStatus for possible status values
     */
    void software_version(util::VersionType& version, hal::UartStatus& status);

    /**
     * @brief Restart the HM11 module
     *
     * This method reboots the device without changing any settings. It sends
     * the "AT+RESET" command to the module, which causes it to restart with
     * the current configuration.
     *
     * After calling this method, it's recommended to wait at least 500ms before
     * sending any other commands to allow the module to complete its restart
     * sequence.
     *
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, the module has been restarted
     * @post Existing configuration settings are preserved
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see reset() for resetting to factory settings
     */
    void restart(hal::UartStatus& status);

    /**
     * @brief Set the role of the HM11 module
     *
     * Sets the BLE role of the module to either Peripheral (server) or Central (client).
     * In Peripheral role, the module advertises itself and can be connected to by
     * other BLE devices. In Central role, the module can scan for and connect to
     * other BLE devices.
     *
     * This method sends the "AT+ROLE" command with the appropriate parameter.
     * The default role for the module is Peripheral.
     *
     * @param role Role to set (Peripheral or Central)
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, the role has been set
     * @post The module may need to be restarted for the change to take effect
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see Role for available roles
     * @see hal::UartStatus for possible status values
     * @see set_work_type() to configure the behavior after role change
     * @see restart() to apply the new role
     */
    void set_role(Role role, hal::UartStatus& status);

    /**
     * @brief Get the current role of the HM11 module
     *
     * Retrieves the current BLE role of the module (Peripheral or Central)
     * by sending the "AT+ROLE?" command.
     *
     * @param role Output parameter for current role
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, role contains the current module role
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see Role for possible role values
     * @see hal::UartStatus for possible status values
     * @see set_role() to change the module role
     */
    void get_role(Role& role, hal::UartStatus& status);

    /**
     * @brief Reset the HM11 module to factory settings
     *
     * This method restores all setup values to factory defaults by sending
     * the "AT+RENEW" command. All configuration parameters will be reset to
     * their original values.
     *
     * Factory defaults typically include:
     * - Name: HMSoft
     * - Baud rate: 9600, N, 8, 1
     * - PIN code: 000000
     * - Role: Peripheral
     * - Work mode: Remote-Control mode
     * - iBeacon: Disabled
     *
     * After calling this method, it's recommended to wait at least 500ms before
     * sending any other commands to allow the module to complete its reset.
     *
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, the module has been reset to factory settings
     * @post All custom configuration will be lost
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see restart() for rebooting without changing settings
     */
    void reset(hal::UartStatus& status);

    /**
     * @brief Put the HM11 module into sleep mode
     *
     * This method puts the module into sleep mode to conserve power by sending
     * the "AT+SLEEP" command. In sleep mode, the module consumes significantly
     * less power but is still able to maintain any active connection and respond
     * to events.
     *
     * The module can be woken up using the wake_up() method or by receiving
     * data on its UART interface (if configured to wake on UART activity).
     *
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre The module must be in Peripheral role
     * @post If status is UartStatus::Ok, the module is in sleep mode
     * @post Power consumption is reduced
     * @post Module remains in sleep mode until explicitly woken up
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see wake_up() for waking up the module
     * @see set_module_auto_sleep() for automatic sleep functionality
     *
     * @note Only supported in Peripheral role.
     */
    void sleep(hal::UartStatus& status);

    /**
     * @brief Wake up the HM11 module from sleep mode
     *
     * This method wakes up the module from sleep mode by sending the
     * appropriate wake-up signal. After waking up, the module resumes
     * normal operation and responds to commands.
     *
     * The wake-up process typically involves sending a dummy byte to
     * the UART to trigger the module to exit sleep mode. After waking up,
     * it's recommended to wait a short time (10-20ms) before sending
     * any commands.
     *
     * @param status Output parameter for operation status
     *
     * @pre The module must be in sleep mode
     * @post If status is UartStatus::Ok, the module is awake and ready
     * @post Normal power consumption resumes
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see sleep() for putting the module to sleep
     */
    void wake_up(hal::UartStatus& status);

    /**
     * @brief Get the MAC address of the module
     *
     * This method retrieves the MAC address of the HM11 module by sending
     * the "AT+ADDR?" command. The MAC address is returned as a MacAddress
     * object, which is a fixed-size string of 12 hexadecimal characters.
     *
     * The MAC address uniquely identifies the module and is useful for
     * distinguishing between multiple devices. In iOS, the MAC address cannot
     * be retrieved directly but can be included in the advertising packet.
     *
     * @param mac Output parameter for the MAC address (12 hex characters)
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, mac contains the module's MAC address
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::MacAddress for the MAC address format
     * @see hal::UartStatus for possible status values
     */
    void get_mac_address(util::MacAddress& mac, hal::UartStatus& status);

    /**
     * @brief Set the advertising interval of the module
     *
     * This method sets the advertising interval for the HM11 module by sending
     * the "AT+ADVI" command. The advertising interval determines how frequently
     * the module broadcasts its presence to nearby devices.
     *
     * The advertising interval affects power consumption and visibility:
     * - Shorter intervals (e.g., Ms100) consume more power but improve visibility
     * - Longer intervals (e.g., Ms1285) save power but may reduce visibility
     *
     * The maximum recommended value for iOS compatibility is Ms1285 (1285ms).
     * This feature was added in firmware version V517 and enhanced in V522.
     *
     * @param interval The advertising interval to set
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Module firmware version must be V517 or newer
     * @post If status is UartStatus::Ok, the advertising interval has been set
     * @post The module may need to be restarted for the change to take effect
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see AdvertisingInterval for available interval values
     * @see hal::UartStatus for possible status values
     * @see restart() to apply the new interval
     */
    void set_advertising_interval(AdvertisingInterval interval, hal::UartStatus& status);

    /**
     * @brief Get the current advertising interval of the module
     *
     * @param interval Output parameter for the advertising interval
     * @param status Output parameter for operation status
     */
    void get_advertising_interval(AdvertisingInterval& interval, hal::UartStatus& status);

    /**
     * @brief Set the advertising type of the module
     *
     * This method sets the advertising type for the HM11 module.
     * The advertising type affects how the module advertises its presence.
     *
     * @param type The advertising type to set
     * @param status Output parameter for operation status
     */
    void set_advertising_type(AdvertisingType type, hal::UartStatus& status);

    /**
     * @brief Get the current advertising type of the module
     *
     * @param type Output parameter for the advertising type
     * @param status Output parameter for operation status
     */
    void get_advertising_type(AdvertisingType& type, hal::UartStatus& status);

    /**
     * @brief Set the sensor type connected to the HM11 module
     *
     * This method configures the type of sensor connected to the HM11 module.
     * This command is only used for HMSensor version.
     * Default is None.
     *
     * @param type The sensor type to set
     * @param status Output parameter for operation status
     */
    void set_sensor_type(SensorType type, hal::UartStatus& status);

    /**
     * @brief Get the current sensor type connected to the HM11 module
     *
     * This method retrieves the type of sensor currently configured on the HM11 module.
     * This command is only used for HMSensor version.
     *
     * @param type Output parameter that will be set to the current sensor type
     * @param status Output parameter for operation status
     */
    void get_sensor_type(SensorType& type, hal::UartStatus& status);

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
     * @param status Output parameter for operation status
     */
    void get_sensor_temperature_and_humidity(
        util::TemperatureType& temperature,
        util::HumidityType& humidity,
        hal::UartStatus& status);

    /**
     * @brief Set the work interval for the sensor
     *
     * This method configures the work interval for the sensor connected to the HM11 module.
     * The interval determines how often sensor readings are taken.
     * This command is only used for HMSensor version.
     * Default is "00" (0 minutes).
     *
     * @param interval The work interval to set (00-99 minutes)
     * @param status Output parameter for operation status
     */
    void set_sensor_work_interval(const util::WorkInterval& interval, hal::UartStatus& status);

    /**
     * @brief Get the current work interval for the sensor
     *
     * This method retrieves the current work interval configured for the sensor
     * connected to the HM11 module.
     * This command is only used for HMSensor version.
     *
     * @param interval Output parameter that will be set to the current work interval
     * @param status Output parameter for operation status
     */
    void get_sensor_work_interval(util::WorkInterval& interval, hal::UartStatus& status);

    /**
     * @brief Set the reliable advertising mode of the module
     *
     * This method configures whether the module uses normal or reliable advertising.
     * Reliable advertising helps ensure visibility during long-time standby.
     * Only applicable in Peripheral role.
     *
     * @param mode The advertising mode to set
     * @param status Output parameter for operation status
     */
    void set_reliable_advertising_mode(AdvertisingMode mode, hal::UartStatus& status);

    /**
     * @brief Get the current reliable advertising mode of the module
     *
     * @param mode Output parameter for the current advertising mode
     * @param status Output parameter for operation status
     */
    void get_reliable_advertising_mode(AdvertisingMode& mode, hal::UartStatus& status);

    /**
     * @brief Set the advertising FLAG for the module
     *
     * This method sets the FLAG field in advertising packets, which controls
     * certain advertising behaviors and capabilities.
     * Only applicable in Peripheral role.
     *
     * @param flag The advertising FLAG to set (2 hex characters)
     * @param status Output parameter for operation status
     */
    void set_advertising_flag(const util::AdvertisingFlag& flag, hal::UartStatus& status);

    /**
     * @brief Set the module name
     *
     * This method sets the name of the HM11 module that will be visible
     * to other Bluetooth devices.
     *
     * @param name The name to set (max 12 characters)
     * @param status Output parameter for operation status
     */
    void set_module_name(std::string_view name, hal::UartStatus& status);

    /**
     * @brief Get the current module name
     *
     * @param name Output buffer for the module name
     * @param max_length Maximum length of the name buffer
     * @param status Output parameter for operation status
     * @return The length of the returned name, or 0 if an error occurred
     */
    std::size_t get_module_name(char* name, std::size_t max_length, hal::UartStatus& status);

    /**
     * @brief Get the current module name as a string
     *
     * @param status Output parameter for operation status
     * @return The module name as a string, or empty string if an error occurred
     */
    std::string get_module_name(hal::UartStatus& status);

    /**
     * @brief Set the module's output power
     *
     * This method sets the output power level of the HM11 module.
     * Higher power levels increase range but consume more energy.
     *
     * @param power The power level to set
     * @param status Output parameter for operation status
     */
    void set_module_power(ModulePower power, hal::UartStatus& status);

    /**
     * @brief Get the current module power setting
     *
     * @param power Output parameter for the power setting
     * @param status Output parameter for operation status
     */
    void get_module_power(ModulePower& power, hal::UartStatus& status);

    /**
     * @brief Set the work mode of the module
     *
     * This method sets the operational work mode for the HM11 module.
     * Different work modes determine how the module behaves in terms of
     * data transmission, PIO collection, and remote control functionality.
     *
     * @param mode The work mode to set
     * @param status Output parameter for operation status
     */
    void set_work_mode(WorkMode mode, hal::UartStatus& status);

    /**
     * @brief Get the current work mode of the module
     *
     * @param mode Output parameter for the work mode
     * @param status Output parameter for operation status
     */
    void get_work_mode(WorkMode& mode, hal::UartStatus& status);

    /**
     * @brief Set the work type of the module
     *
     * This method sets whether the module starts working immediately
     * or responds to AT commands first.
     *
     * @param type The work type to set
     * @param status Output parameter for operation status
     */
    void set_work_type(WorkType type, hal::UartStatus& status);

    /**
     * @brief Get the current work type of the module
     *
     * @param type Output parameter for the work type
     * @param status Output parameter for operation status
     */
    void get_work_type(WorkType& type, hal::UartStatus& status);

    /**
     * @brief Set the UART baud rate for communication
     *
     * This method sets the baud rate used by the module for UART communication.
     * The default is typically 9600 baud.
     *
     * @param baud_rate The baud rate to set
     * @param status Output parameter for operation status
     */
    void set_uart_baud_rate(UartBaudRate baud_rate, hal::UartStatus& status);

    /**
     * @brief Get the current UART baud rate
     *
     * @param baud_rate Output parameter for the baud rate
     * @param status Output parameter for operation status
     */
    void get_uart_baud_rate(UartBaudRate& baud_rate, hal::UartStatus& status);

    /**
     * @brief Set the UART flow control switch
     *
     * @param enable Whether to enable (true) or disable (false) flow control
     * @param status Output parameter for operation status
     */
    void set_uart_flow_control_switch(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current UART flow control switch setting
     *
     * @param enabled Output parameter indicating if flow control is enabled
     * @param status Output parameter for operation status
     */
    void get_uart_flow_control_switch(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Set the parity bit
     *
     * @param parity The parity bit setting to set
     * @param status Output parameter for operation status
     */
    void set_parity_bit(ParityBit parity, hal::UartStatus& status);

    /**
     * @brief Get the current parity bit setting
     *
     * @param parity Output parameter for the parity bit setting
     * @param status Output parameter for operation status
     */
    void get_parity_bit(ParityBit& parity, hal::UartStatus& status);

    /**
     * @brief Set the stop bit
     *
     * @param stop_bit The stop bit setting to set
     * @param status Output parameter for operation status
     */
    void set_stop_bit(StopBit stop_bit, hal::UartStatus& status);

    /**
     * @brief Get the current stop bit setting
     *
     * @param stop_bit Output parameter for the stop bit setting
     * @param status Output parameter for operation status
     */
    void get_stop_bit(StopBit& stop_bit, hal::UartStatus& status);

    /**
     * @brief Set the module RX gain
     *
     * @param gain The RX gain setting to set
     * @param status Output parameter for operation status
     */
    void set_module_rx_gain(RxGain gain, hal::UartStatus& status);

    /**
     * @brief Get the current module RX gain setting
     *
     * @param gain Output parameter for the RX gain setting
     * @param status Output parameter for operation status
     */
    void get_module_rx_gain(RxGain& gain, hal::UartStatus& status);

    /**
     * @brief Set the service UUID
     *
     * @param uuid The service UUID to set
     * @param status Output parameter for operation status
     */
    void set_service_uuid(std::string_view uuid, hal::UartStatus& status);

    /**
     * @brief Get the current service UUID
     *
     * @param uuid Output parameter for the service UUID
     * @param status Output parameter for operation status
     */
    void get_service_uuid(std::string& uuid, hal::UartStatus& status);

    /**
     * @brief Set the characteristic
     *
     * @param characteristic The characteristic to set
     * @param status Output parameter for operation status
     */
    void set_characteristic(std::string_view characteristic, hal::UartStatus& status);

    /**
     * @brief Get the current characteristic
     *
     * @param characteristic Output parameter for the characteristic
     * @param status Output parameter for operation status
     */
    void get_characteristic(std::string& characteristic, hal::UartStatus& status);

    /**
     * @brief Set the output power level
     *
     * @param power The output power level to set
     * @param status Output parameter for operation status
     */
    void set_output_power(OutputPower power, hal::UartStatus& status);

    /**
     * @brief Get the current output power level
     *
     * @param power Output parameter for the power level
     * @param status Output parameter for operation status
     */
    void get_output_power(OutputPower& power, hal::UartStatus& status);

    /**
     * @brief Enable or disable the auto-sleep feature
     *
     * This method enables or disables the module's auto-sleep functionality.
     * When enabled, the module will automatically enter sleep mode to conserve power.
     * Only available in Peripheral role.
     *
     * @param enable Whether to enable (true) or disable (false) auto-sleep
     * @param status Output parameter for operation status
     */
    void set_module_auto_sleep(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current auto-sleep setting
     *
     * @param enabled Output parameter indicating if auto-sleep is enabled
     * @param status Output parameter for operation status
     */
    void get_module_auto_sleep(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Enable or disable the battery monitor
     *
     * @param enable Whether to enable (true) or disable (false) the battery monitor
     * @param status Output parameter for operation status
     */
    void set_battery_monitor_switch(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current battery monitor setting
     *
     * @param enabled Output parameter indicating if battery monitor is enabled
     * @param status Output parameter for operation status
     */
    void get_battery_monitor_switch(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Set battery information
     *
     * @param level Battery level as a percentage
     * @param status Output parameter for operation status
     */
    void set_battery_information(util::Percent level, hal::UartStatus& status);

    /**
     * @brief Query battery information
     *
     * @param level Output parameter for battery level as a percentage
     * @param status Output parameter for operation status
     */
    void query_battery_information(util::Percent& level, hal::UartStatus& status);

    /**
     * @brief Set the UART sleep type
     *
     * @param type The UART sleep type to set
     * @param status Output parameter for operation status
     */
    void set_uart_sleep_type(UartSleepType type, hal::UartStatus& status);

    /**
     * @brief Get the current UART sleep type
     *
     * @param type Output parameter for the UART sleep type
     * @param status Output parameter for operation status
     */
    void get_uart_sleep_type(UartSleepType& type, hal::UartStatus& status);

    /**
     * @brief Enable or disable iBeacon functionality
     *
     * This method enables or disables the iBeacon functionality of the module.
     * When enabled, the module will operate as an iBeacon.
     *
     * @param enable Whether to enable (true) or disable (false) iBeacon mode
     * @param status Output parameter for operation status
     */
    void set_ibeacon_switch(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current iBeacon setting
     *
     * @param enabled Output parameter indicating if iBeacon is enabled
     * @param status Output parameter for operation status
     */
    void get_ibeacon_switch(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Set the iBeacon UUID
     *
     * This method sets the UUID used for iBeacon functionality.
     * The UUID is a 32-character hexadecimal string.
     *
     * @param uuid The UUID to set
     * @param status Output parameter for operation status
     */
    void set_ibeacon_uuid(const util::IBeaconUuid& uuid, hal::UartStatus& status);

    /**
     * @brief Get the current iBeacon UUID
     *
     * @param uuid Output parameter for the UUID
     * @param status Output parameter for operation status
     */
    void get_ibeacon_uuid(util::IBeaconUuid& uuid, hal::UartStatus& status);

    /**
     * @brief Set the iBeacon major version
     *
     * This method sets the major version identifier for iBeacon functionality.
     *
     * @param version The major version to set
     * @param status Output parameter for operation status
     */
    void set_ibeacon_major_version(const util::VersionType& version, hal::UartStatus& status);

    /**
     * @brief Get the current iBeacon major version
     *
     * @param version Output parameter for the major version
     * @param status Output parameter for operation status
     */
    void get_ibeacon_major_version(util::VersionType& version, hal::UartStatus& status);

    /**
     * @brief Set the iBeacon minor version
     *
     * This method sets the minor version identifier for iBeacon functionality.
     *
     * @param version The minor version to set
     * @param status Output parameter for operation status
     */
    void set_ibeacon_minor_version(const util::VersionType& version, hal::UartStatus& status);

    /**
     * @brief Get the current iBeacon minor version
     *
     * @param version Output parameter for the minor version
     * @param status Output parameter for operation status
     */
    void get_ibeacon_minor_version(util::VersionType& version, hal::UartStatus& status);

    /**
     * @brief Set the iBeacon measured power
     *
     * This method sets the measured power value for iBeacon functionality.
     * This value is used for distance estimation.
     *
     * @param power The measured power to set
     * @param status Output parameter for operation status
     */
    void set_ibeacon_measured_power(const util::MeasuredPower& power, hal::UartStatus& status);

    /**
     * @brief Get the current iBeacon measured power
     *
     * @param power Output parameter for the measured power
     * @param status Output parameter for operation status
     */
    void get_ibeacon_measured_power(util::MeasuredPower& power, hal::UartStatus& status);

    /**
     * @brief Start working
     *
     * This method starts the module working according to its configured
     * role and work mode. This is only used when work type is set to
     * Respond_AT_Command.
     *
     * @param status Output parameter for operation status
     */
    void start_working(hal::UartStatus& status);

    /**
     * @brief Disconnect the current connection
     *
     * This method disconnects the current Bluetooth connection by sending
     * the "AT+DISC" command. It can be used in both Peripheral and Central
     * roles to terminate an active connection.
     *
     * After disconnection, the module returns to its previous state (advertising
     * in Peripheral mode or scanning in Central mode, depending on configuration).
     *
     * @param status Output parameter for operation status
     *
     * @pre A connection must be established
     * @post If status is UartStatus::Ok, the connection has been terminated
     * @post The module returns to its previous state
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see connect() for establishing a connection in Central mode
     * @see scan() for discovering devices in Central mode
     *
     * @note Should be connected before calling this method.
     */
    void disconnect(hal::UartStatus& status);

    /**
     * @brief Connect to a device by MAC address
     *
     * This method attempts to connect to a Bluetooth device with the specified MAC address.
     * It sends the "AT+CON" command followed by the MAC address.
     *
     * @param mac The MAC address of the device to connect to
     * @param result Output parameter for the connection result
     * @param status Output parameter for operation status
     *
     * @pre The module must be in Central role
     * @post If successful, a connection is established
     */
    void connect(const util::MacAddress& mac, ConnectResult& result, hal::UartStatus& status);

    /**
     * @brief Connect to a device by MAC address string
     *
     * This method attempts to connect to a Bluetooth device with the specified MAC address string.
     * The MAC address string should be 12 hexadecimal characters.
     *
     * @param mac_str The MAC address string (12 hex characters)
     * @param result Output parameter for the connection result
     * @param status Output parameter for operation status
     */
    void connect(std::string_view mac_str, ConnectResult& result, hal::UartStatus& status);

    /**
     * @brief Connect to the last connected device
     *
     * This method attempts to reconnect to the last device that was successfully connected.
     * It sends the "AT+CONNL" command.
     *
     * @param result Output parameter for the connection result
     * @param status Output parameter for operation status
     */
    void connect_last_device(ConnectResult& result, hal::UartStatus& status);

    /**
     * @brief Set the PIN code for the HM11 module
     *
     * Sets the PIN code used for authentication. The PIN code is a 6-digit number
     * used during the pairing process when security is enabled. The default PIN
     * code is 000000.
     *
     * @param pin The PIN code to set (6 digits)
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, the PIN code has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::PinType for the PIN code format
     * @see hal::UartStatus for possible status values
     * @see set_bond_mode() to configure authentication requirements
     */
    void set_pin_code(const util::PinType& pin, hal::UartStatus& status);

    /**
     * @brief Get the current PIN code of the HM11 module
     *
     * Retrieves the current PIN code set in the module.
     *
     * @param pin Output parameter for the PIN code
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, pin contains the current PIN code
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::PinType for the PIN code format
     * @see hal::UartStatus for possible status values
     */
    void get_pin_code(util::PinType& pin, hal::UartStatus& status);

    /**
     * @brief Set the bond mode of the HM11 module
     *
     * Configures the bond and authentication mode used for connections.
     * Different bond modes determine the level of security required for
     * connections, ranging from no authentication to full bonding with PIN.
     *
     * This setting affects how the module authenticates with other devices.
     * It was added in firmware version V515 and enhanced in V524.
     *
     * @param mode The bond mode to set
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Module firmware version must be V515 or newer
     * @post If status is UartStatus::Ok, the bond mode has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see BondMode for available bond modes
     * @see hal::UartStatus for possible status values
     * @see set_pin_code() to set the PIN used for authentication
     *
     * @note In Android, AuthNotNeedPin is treated the same as AuthWithPin.
     * @note Important: If your module version is less than V515, please don't use this command.
     */
    void set_bond_mode(BondMode mode, hal::UartStatus& status);

    /**
     * @brief Get the current bond mode of the HM11 module
     *
     * Retrieves the current bond mode setting of the module.
     *
     * @param mode Output parameter for the bond mode
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, mode contains the current bond mode
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see BondMode for possible bond mode values
     * @see hal::UartStatus for possible status values
     */
    void get_bond_mode(BondMode& mode, hal::UartStatus& status);

    /**
     * @brief Enable or disable the white list feature
     *
     * Enables or disables the white list functionality, which restricts
     * connections to only those devices whose MAC addresses are in the
     * white list. This feature is useful for creating secure connections
     * by allowing only authorized devices to connect.
     *
     * This feature was added in firmware version V523.
     *
     * @param enable Whether to enable (true) or disable (false) the white list
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Module firmware version must be V523 or newer
     * @post If status is UartStatus::Ok, the white list setting has been applied
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see set_white_list_mac_address() to add MAC addresses to the white list
     */
    void set_white_list_switch(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current white list switch setting
     *
     * Retrieves the current state of the white list feature (enabled or disabled).
     *
     * @param enabled Output parameter indicating if the white list is enabled
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, enabled contains the current white list state
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     */
    void get_white_list_switch(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Add a MAC address to the white list
     *
     * Adds a specific device's MAC address to the white list at the specified index.
     * The white list can store up to 3 MAC addresses. When the white list feature is
     * enabled, only devices with MAC addresses in this list can connect to the module.
     *
     * This feature was added in firmware version V523.
     *
     * @param index The index in the white list (1 to 3)
     * @param mac The MAC address to add (12 hex characters)
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Module firmware version must be V523 or newer
     * @pre Index must be between 1 and 3 inclusive
     * @post If status is UartStatus::Ok, the MAC address has been added to the white list
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::MacWhiteListIndex for the index type
     * @see util::MacAddress for the MAC address format
     * @see hal::UartStatus for possible status values
     * @see set_white_list_switch() to enable the white list feature
     */
    void set_white_list_mac_address(
        const util::MacWhiteListIndex& index,
        const util::MacAddress& mac,
        hal::UartStatus& status);

    /**
     * @brief Get a MAC address from the white list
     *
     * Retrieves the MAC address stored at the specified index in the white list.
     *
     * @param index The index in the white list (1 to 3)
     * @param mac Output parameter for the MAC address
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Index must be between 1 and 3 inclusive
     * @post If status is UartStatus::Ok, mac contains the MAC address from the white list
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see util::MacWhiteListIndex for the index type
     * @see util::MacAddress for the MAC address format
     * @see hal::UartStatus for possible status values
     */
    void
    get_white_list_mac_address(const util::MacWhiteListIndex& index, util::MacAddress& mac, hal::UartStatus& status);

    /**
     * @brief Enable or disable connection notification messages
     *
     * When enabled, the module sends "OK+CONN" or "OK+LOST" messages through the UART
     * when a Bluetooth connection is established or lost. This is useful for monitoring
     * connection state changes without polling.
     *
     * @param enable Whether to enable (true) or disable (false) notifications
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, the notification setting has been applied
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     * @see set_notify_mode() to configure notification format
     */
    void set_notify_information(bool enable, hal::UartStatus& status);

    /**
     * @brief Get the current connection notification setting
     *
     * Retrieves whether connection notification messages are enabled or disabled.
     *
     * @param enabled Output parameter indicating if notifications are enabled
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, enabled contains the current notification state
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see hal::UartStatus for possible status values
     */
    void get_notify_information(bool& enabled, hal::UartStatus& status);

    /**
     * @brief Set the notification mode for connection events
     *
     * Configures whether MAC addresses are included in connection and disconnection
     * notification messages. When set to WithAddress, notifications include the
     * remote device's MAC address (e.g., "OK+CONN:001122334455").
     *
     * This feature was added in firmware version V534.
     *
     * @param mode The notification mode to set
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @pre Module firmware version must be V534 or newer
     * @pre Notifications must be enabled via set_notify_information(true)
     * @post If status is UartStatus::Ok, the notification mode has been set
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see NotifyMode for available notification modes
     * @see hal::UartStatus for possible status values
     * @see set_notify_information() to enable notifications
     */
    void set_notify_mode(NotifyMode mode, hal::UartStatus& status);

    /**
     * @brief Get the current notification mode
     *
     * Retrieves the current notification mode setting for connection events.
     *
     * @param mode Output parameter for the notification mode
     * @param status Output parameter for operation status
     *
     * @pre The module must be responsive (test() returns Ok)
     * @post If status is UartStatus::Ok, mode contains the current notification mode
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see NotifyMode for possible notification modes
     * @see hal::UartStatus for possible status values
     */
    void get_notify_mode(NotifyMode& mode, hal::UartStatus& status);

    /**
     * @brief Set the connection supervision timeout
     *
     * @param value The connection supervision timeout to set
     * @param status Output parameter for operation status
     */
    void set_connection_supervision_timeout(ConnectionSupervisionTimeout value, hal::UartStatus& status);

    /**
     * @brief Get the current connection supervision timeout
     *
     * @param result Output parameter for the connection supervision timeout
     * @param status Output parameter for operation status
     */
    void get_connection_supervision_timeout(ConnectionSupervisionTimeout& result, hal::UartStatus& status);

    /**
     * @brief Set the update connection setting
     *
     * @param value Whether to enable (true) or disable (false) update connection
     * @param status Output parameter for operation status
     */
    void set_update_connection(bool value, hal::UartStatus& status);

    /**
     * @brief Get the current update connection setting
     *
     * @param result Output parameter indicating if update connection is enabled
     * @param status Output parameter for operation status
     */
    void get_update_connection(bool& result, hal::UartStatus& status);

    /**
     * @brief Set the connect remote device timeout
     *
     * @param timeout The timeout value to set
     * @param status Output parameter for operation status
     */
    void set_connect_remote_device_timeout(util::ConnectTimeout timeout, hal::UartStatus& status);

    /**
     * @brief Get the current connect remote device timeout
     *
     * @param timeout Output parameter for the timeout value
     * @param status Output parameter for operation status
     */
    void get_connect_remote_device_timeout(util::ConnectTimeout& timeout, hal::UartStatus& status);

    /**
     * @brief Scan for nearby BLE devices
     *
     * This method performs a scan for nearby Bluetooth Low Energy devices
     * by sending the "AT+DISC?" command. When devices are discovered, the
     * provided callback function is invoked with the discovered device information.
     *
     * @param callback Function to be called when a device is discovered
     * @param timeout_ms Maximum time to scan in milliseconds
     * @param status Output parameter for operation status
     */
    void scan(DiscoveryCallback callback, std::chrono::milliseconds timeout_ms, hal::UartStatus& status);

    /**
     * @brief Scan for nearby iBeacon devices
     *
     * This method performs a scan specifically for iBeacon devices
     * by sending the "AT+DISI?" command. When iBeacon devices are discovered,
     * the provided callback function is invoked with the discovered device information.
     *
     * @param callback Function to be called when an iBeacon device is discovered
     * @param timeout_ms Maximum time to scan in milliseconds
     * @param status Output parameter for operation status
     */
    void scan_ibeacon(IBeaconDiscoveryCallback callback, std::chrono::milliseconds timeout_ms, hal::UartStatus& status);

    /**
     * @brief Find all service UUIDs on a connected device
     *
     * This method sends the "AT+FINDSERVICES?" command to discover all service UUIDs
     * available on the currently connected peripheral device. When services are discovered,
     * the provided callback function is invoked with the service information.
     *
     * The service string format is [P1]:[P2]:[P3] where:
     *   - P1: 4 bytes, Service start handle
     *   - P2: 4 bytes, Service end handle
     *   - P3: 4 bytes, Service UUID
     *
     * This command was added in firmware version V700.
     *
     * @param callback Function to be called for each discovered service
     * @param user_data Optional user data to be passed to the callback
     * @param timeout_ms Maximum time to wait for service discovery in milliseconds
     * @param status Output parameter for operation status
     *
     * @pre The module must be in Central role (set_role(Role::Central))
     * @pre A connection must be established with a peripheral device
     * @post If status is UartStatus::Ok, all services have been discovered
     *
     * Thread safety: This method is not ISR-safe and should only be called
     * from a single thread.
     *
     * @see gatt::ServiceUUIDCallback for the callback signature
     * @see hal::UartStatus for possible status values
     */
    void find_all_services_uuid(
        gatt::ServiceUUIDCallback callback,
        void* user_data,
        std::chrono::milliseconds timeout_ms,
        hal::UartStatus& status);

  private:
    UARTImpl uart;                                                ///< UART implementation for communication
    ReceiveHandler receive_handler;                               ///< Handler for receiving data
    LastReadPositionHandler last_read_handler;                    ///< Handler for last read position
    std::array<hal::UInt8, MAX_MESSAGE_LENGTH * 5> response = {}; ///< Buffer for received responses
    DiscoveredInfo discovered_info;                               ///< Information about discovered devices

    /**
     * @brief Transmit a command to the HM11 module
     *
     * @param command Command to transmit
     * @param status Output parameter for operation status
     */
    /**
     * @brief Transmit a command to the HM11 module
     *
     * @param command Command to transmit
     * @param status Output parameter for operation status
     */
    void transmit(std::string_view command, hal::UartStatus& status);

    /**
     * @brief Transmit a command with additional data to the HM11 module
     *
     * @param command Command to transmit
     * @param data Additional binary data to transmit
     * @param status Output parameter for operation status
     */
    void transmit(std::string_view command, const hal::UartData8b& data, hal::UartStatus& status);

    /**
     * @brief Check if a received response contains an expected value
     *
     * @param received Pointer to received data
     * @param length Length of received data
     * @param expect Expected response string
     * @param status Output parameter for operation status
     */
    void check_response(void* received, std::size_t length, std::string_view expect, hal::UartStatus& status);

    /**
     * @brief Transmit a command and check for an expected response
     *
     * @param command Command to transmit
     * @param expect Expected response
     * @param status Output parameter for operation status
     * @return true If the expected response was received
     * @return false If the expected response was not received
     */
    bool transmit_and_check(std::string_view command, std::string_view expect, hal::UartStatus& status);

    /**
     * @brief Transmit a command and check for an expected response with detailed response access
     *
     * @param command Command to transmit
     * @param expect Expected response
     * @param received Pointer to received data
     * @param length Length of received data
     * @param status Output parameter for operation status
     * @return true If the expected response was received
     * @return false If the expected response was not received
     */
    bool transmit_and_check(
        std::string_view command,
        std::string_view expect,
        void* received,
        std::size_t length,
        hal::UartStatus& status);
};

} // namespace hm11

#endif // HM11_DRIVER_HPP