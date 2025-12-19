/**
 * @file hm11_driver.hpp
 * @brief HM11 Bluetooth Low Energy driver class template
 *
 * This header provides the main HM11Driver class template that implements the
 * complete driver interface for HM-11 Bluetooth LE modules. The driver uses
 * template-based dependency injection to support different UART implementations
 * while maintaining zero runtime overhead.
 *
 * ## Overview
 *
 * The HM11Driver is the primary interface for controlling HM-11 Bluetooth Low
 * Energy modules via AT commands. It provides a comprehensive API for device
 * configuration, connection management, GATT operations, discovery, and
 * iBeacon functionality.
 *
 * ## Architecture (Decision 1: Dependency Injection Mechanism)
 *
 * The driver uses template-based dependency injection with C++20 concepts:
 * - Template parameter: UARTImpl (UART port implementation)
 * - Template parameter: ReceiveHandler (receive strategy implementation)
 * - Concepts enforce interface contracts at compile time
 * - Zero runtime overhead (no vtables, no virtual dispatch)
 * - Direct Ada translation: Template parameters = Ada discriminants
 *
 * ## Ownership Model (Decision 22: RAII Ownership Model)
 *
 * The driver follows unique ownership semantics:
 * - Copy constructor: deleted
 * - Copy assignment: deleted
 * - Move constructor: deleted
 * - Move assignment: deleted
 * - Destructor: graceful cleanup with 100ms timeout
 * - Matches Ada's "limited private" type semantics
 *
 * ## Internal State
 *
 * The driver maintains minimal internal state:
 * - Response buffer: alignas(32) for DMA compatibility (Decision 20)
 * - Discovered device temporary: for streaming parse (Decision 27)
 * - UART reference: non-owning, lifetime tied to driver scope
 * - Receive handler reference: non-owning, lifetime tied to driver scope
 *
 * ## Usage Example
 *
 * @code
 * using namespace hm11;
 * using namespace hm11::hal;
 *
 * // Define UART implementation (satisfies UARTPort concept)
 * STM32F4UARTPort uart_port;
 * DefaultReceiveHandler receive_handler;
 *
 * // Construct driver with dependency injection
 * HM11Driver<STM32F4UARTPort, DefaultReceiveHandler> driver(uart_port, receive_handler);
 *
 * // Test connectivity
 * UartStatus status;
 * driver.test(status);
 * if (status == UartStatus::Ok) {
 *     // Module is responding
 * }
 *
 * // Set role to Central
 * driver.set_role(types::Role::Central, status);
 *
 * // Query software version
 * std::string_view version;
 * driver.software_version(version, status);
 * // Driver automatically destroyed on scope exit with graceful cleanup
 * @endcode
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/types/enums.hpp"
#include "hm11/types/type_aliases.hpp"
#include "hm11/util/command_builder.hpp"
#include "hm11/util/parsing_utils.hpp"

namespace hm11 {

/**
 * @brief Maximum length of HM-11 messages
 *
 * Corresponds to Ada's Max_Message_Length constant (hm11.ads:283).
 * Sized to handle the longest possible response: discovery streaming data.
 */
inline constexpr std::size_t MAX_MESSAGE_LENGTH = 278; // Discovered_Lenght from Ada

/**
 * @brief Maximum length of device name in discovery responses
 *
 * Corresponds to Ada's Max_Name_Length constant (hm11.ads:132).
 */
inline constexpr std::size_t MAX_NAME_LENGTH = 248;

/**
 * @brief Maximum length of RSSI string in discovery responses
 *
 * Corresponds to Ada's Max_RSII_Length constant (hm11.ads:133).
 */
inline constexpr std::size_t MAX_RSSI_LENGTH = 4;

/**
 * @brief Variable-length string with fixed capacity (Decision 27)
 *
 * Matches Ada's Variable_String discriminated type (hm11.ads:1234-1237).
 * Stores a fixed-size character array with tracked length for accumulating
 * variable-length data during streaming parse operations.
 *
 * @tparam Capacity Maximum number of characters that can be stored
 */
template <std::size_t Capacity> struct VariableString {
    std::array<char, Capacity> value{}; ///< Character storage array
    std::size_t last{0};                ///< Current length (0-based index of last char + 1)

    /// @brief Reset the string to empty
    constexpr void clear() noexcept { last = 0; }

    /// @brief Append a character to the string
    /// @param c Character to append
    /// @return true if appended successfully, false if capacity exceeded
    constexpr bool append(char c) noexcept {
        if (last >= Capacity) {
            return false;
        }
        value[last++] = c;
        return true;
    }

    /// @brief Append a string_view to the string
    /// @param str String to append
    /// @return true if appended successfully, false if capacity exceeded
    constexpr bool append(std::string_view str) noexcept {
        if (last + str.size() > Capacity) {
            return false;
        }
        for (char c : str) {
            value[last++] = c;
        }
        return true;
    }

    /// @brief Get a string_view of the current content
    /// @return String view of characters [0, last)
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {value.data(), last}; }
};

/**
 * @brief Scan parsing state machine stages (Decision 11)
 *
 * Corresponds to Ada's Scan_Stage_Kind (hm11.ads:1249-1250).
 * Used to track which component of the discovery response is currently
 * being parsed during streaming scan operation.
 */
enum class ScanStage {
    Selection, ///< Looking for prefixes (OK+DISCS, OK+DISC:, OK+NAME:, OK+RSSI:, OK+DISCE)
    MAC,       ///< Parsing 12-byte MAC address
    Name,      ///< Parsing device name until CRLF
    RSSI       ///< Parsing RSSI value until CRLF
};

/**
 * @brief Temporary storage for discovered device information (Decision 27)
 *
 * Corresponds to Ada's Discovered_Info_Type (hm11.ads:1239-1244).
 * Used to accumulate device data during streaming parse. When complete
 * device is parsed (MAC + Name + RSSI), callback is invoked and this
 * temporary is reset for next device.
 */
struct DiscoveredInfo {
    char id{' '};                           ///< Device ID character
    types::MacAddress mac{};                ///< 12-character hex MAC address
    VariableString<MAX_NAME_LENGTH> name{}; ///< Variable-length device name
    VariableString<MAX_RSSI_LENGTH> rssi{}; ///< Variable-length RSSI value

    /// @brief Reset to null state for next device
    void reset() noexcept {
        id = ' ';
        mac = types::MacAddress{};
        name.clear();
        rssi.clear();
    }

    /// @brief Check if this represents a valid discovered device
    /// @return true if MAC address is not all zeros, false otherwise
    [[nodiscard]] bool is_valid() const noexcept {
        // Check if MAC is not default-initialized (all zeros)
        std::string_view mac_view = mac.view();
        return mac_view != "000000000000";
    }
};

/**
 * @brief Callback for discovered device notification (Decision 10)
 *
 * Corresponds to Ada's Discovered_Callback (hm11.ads:139-145).
 * Plain C-style function pointer for zero overhead, no heap allocation.
 * Invoked inline during parsing of streaming data.
 *
 * @param id Device ID character
 * @param mac 12-character hex MAC address
 * @param name Device name (may be empty if Set_Show_Device_Information_When_Discovery not configured)
 * @param rssi RSSI value string (may be empty)
 */
using DiscoveredCallback =
    void (*)(char id, const types::MacAddress& mac, std::string_view name, std::string_view rssi);

/**
 * @brief Callback for discovered iBeacon device notification (Decision 10)
 *
 * Corresponds to Ada's Discovered_iBeacon_Callback (hm11.ads:154-167).
 * Plain C-style function pointer for zero overhead, no heap allocation.
 *
 * Device info format: P0:P1:P2:P3:P4 where:
 * - P0: Factory ID (8 bytes)
 * - P1: iBeacon UUID (32 bytes)
 * - P2: Major(4) + Minor(4) + Measured Power(2) = 10 bytes
 * - P3: MAC address (12 bytes)
 * - P4: RSSI (4 bytes)
 *
 * @param device_info Full device information string (P0:P1:P2:P3:P4 format)
 */
using DiscoveredIBeaconCallback = void (*)(std::string_view device_info);

/**
 * @brief Callback for service discovery notification (Decision 10)
 *
 * Corresponds to Ada's Service_UUID_Callback (hm11.ads:171-175).
 * Plain C-style function pointer for zero overhead, no heap allocation.
 * Invoked inline during parsing of GATT service discovery streaming data.
 *
 * Service info format: [start_handle]:[end_handle]:[uuid] where:
 * - start_handle: 4 hex characters (service start handle)
 * - end_handle: 4 hex characters (service end handle)
 * - uuid: 4 hex characters (service UUID)
 *
 * @param service_info Service information string (start:end:uuid format, 14 chars total with colons)
 *
 * @pre Central role, connected state
 */
using ServiceUuidCallback = void (*)(std::string_view service_info);

/**
 * @brief Callback for characteristic discovery notification (Decision 10)
 *
 * Corresponds to Ada's Characteristic_UUID_Callback (hm11.ads:177-187).
 * Plain C-style function pointer for zero overhead, no heap allocation.
 * Invoked inline during parsing of GATT characteristic discovery streaming data.
 *
 * Characteristic info format: [handle]:[properties]:[uuid] where:
 * - handle: 4 hex characters (characteristic handle)
 * - properties: 14 characters (property flags string)
 * - uuid: 4 hex characters (characteristic UUID)
 *
 * Properties string format: "RD|WR|WN|NO|IN" where:
 * - RD: Read
 * - WR: Write
 * - WN: Write Without Response
 * - NO: Notify
 * - IN: Indicate
 * - "--" replaces unsupported properties (e.g., "RD|--|--|--|--")
 *
 * @param characteristic_info Characteristic information string (handle:properties:uuid format, 24 chars total with
 * colons)
 *
 * @pre Central role, connected state
 */
using CharacteristicUuidCallback = void (*)(std::string_view characteristic_info);

/**
 * @brief HM11 Bluetooth Low Energy driver class template
 *
 * This class provides the complete driver interface for HM-11 BLE modules.
 * It is parameterized by UART implementation and receive handler types,
 * enabling compile-time polymorphism with zero runtime overhead.
 *
 * ## Template Parameters
 *
 * @tparam UARTImpl UART port implementation satisfying IS_UART_PORT_V concept
 * @tparam ReceiveHandler Receive strategy satisfying IS_RECEIVE_HANDLER_V concept
 *
 * ## Class Invariants
 *
 * - uart_port_ reference remains valid for driver lifetime
 * - receive_handler_ reference remains valid for driver lifetime
 * - response_buffer_ is always DMA-aligned (alignas(32))
 * - Driver state is consistent after each operation
 * - All operations report status via out parameter
 *
 * ## Thread Safety
 *
 * This driver is NOT thread-safe. Caller must serialize access if used from
 * multiple threads. Typical usage: single-threaded or protected by higher-level mutex.
 *
 * ## Ada Correspondence
 *
 * Corresponds to Ada's HM11_Driver discriminated type (hm11.ads:289-293):
 * - Ada discriminants → C++ template parameters
 * - Ada limited private → C++ deleted copy/move
 * - Ada Responce buffer → C++ response_buffer_
 * - Ada Discovered_Info → C++ discovered_info_
 */
template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
class HM11Driver {
  public:
    /**
     * @brief Construct HM11Driver with dependency injection
     *
     * Initializes driver with references to UART port and receive handler.
     * These references must remain valid for the driver's lifetime.
     *
     * @param uart_port UART port implementation for communication
     * @param receive_handler Receive strategy for handling incoming data
     *
     * @pre uart_port.data_size() == UartDataSize::DataSize8b
     * @pre uart_port and receive_handler outlive this driver instance
     *
     * @note This constructor performs no I/O operations
     * @note References are stored, not copied (non-owning)
     *
     * Corresponds to Ada discriminant initialization (hm11.ads:290-292)
     */
    explicit HM11Driver(UARTImpl& uart_port, ReceiveHandler& receive_handler) noexcept
        : uart_port_(uart_port), receive_handler_(receive_handler) {}

    /**
     * @brief Destructor with graceful cleanup
     *
     * Performs graceful shutdown with timeout (Decision 22):
     * 1. Attempts graceful stop of ongoing operations
     * 2. Waits up to 100ms for completion
     * 3. Force-stops if timeout exceeded
     * 4. Unregisters IRQ handlers (future implementation)
     *
     * @note Does not throw exceptions (embedded environment)
     * @note May block for up to 100ms during cleanup
     */
    ~HM11Driver() noexcept = default; // NOLINT(modernize-use-equals-default)

    // Delete copy and move operations (Decision 22: RAII Ownership Model)
    // Matches Ada's "limited private" type semantics
    HM11Driver(const HM11Driver&) = delete;
    HM11Driver& operator=(const HM11Driver&) = delete;
    HM11Driver(HM11Driver&&) = delete;
    HM11Driver& operator=(HM11Driver&&) = delete;

    // ========================================================================
    // Basic Device Operations
    // ========================================================================

    /**
     * @brief Test module connectivity
     *
     * Sends "AT" command and expects "OK" response. This is the simplest
     * command to verify that the module is powered, connected, and responding.
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if module responded correctly
     * @post status == UartStatus::ErrTimeout if no response within timeout
     * @post status == UartStatus::ErrError if response mismatch or UART error
     *
     * Corresponds to Ada's Test procedure (hm11.adb:343-349)
     */
    void test(hal::UartStatus& status) noexcept;

    /**
     * @brief Reset module to default settings
     *
     * Sends "AT+RESET" command. Module resets and responds with "OK+RESET".
     * All settings return to factory defaults except MAC address.
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if reset successful
     * @post Module settings restored to factory defaults
     * @post Connection drops if currently connected
     *
     * @note Module may take ~500ms to complete reset
     * @note Factory defaults: 9600 baud, Peripheral role, PIN 000000
     *
     * Corresponds to Ada's Reset procedure (hm11.adb:~2900)
     */
    void reset(hal::UartStatus& status) noexcept;

    /**
     * @brief Restart module (power cycle)
     *
     * Sends "AT+START" command. Module performs power cycle and responds
     * with "OK+START". Current settings are preserved (unlike Reset).
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if restart successful
     * @post Module restarted with current settings preserved
     * @post Connection drops if currently connected
     *
     * @note Module may take ~500ms to complete restart
     * @note Use this instead of Reset to apply configuration changes
     *
     * Corresponds to Ada's Restart procedure (hm11.adb:~2950)
     */
    void restart(hal::UartStatus& status) noexcept;

    /**
     * @brief Put module into sleep mode
     *
     * Sends "AT+SLEEP" command. Module enters low-power sleep mode and
     * responds with "OK+SLEEP". Wake with Wake_Up() method.
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if sleep command acknowledged
     * @post Module in sleep mode (low power consumption)
     * @post UART may remain active or shut down based on Set_UART_Sleep_Type
     *
     * @note Sleep mode configuration: see Set_UART_Sleep_Type()
     * @note Power consumption: <50μA in sleep mode (typical)
     *
     * Corresponds to Ada's Sleep procedure (hm11.adb:317-325)
     */
    void sleep(hal::UartStatus& status) noexcept;

    /**
     * @brief Wake module from sleep mode
     *
     * Sends special wake sequence "WWWWWWW" (7 'W' characters) and expects
     * "OK+WAKE" response. Module exits sleep mode and resumes normal operation.
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if wake successful
     * @post Module awake and ready for commands
     *
     * @note Wake sequence must be exactly 7 'W' characters
     * @note If UART was shut down in sleep, external wake signal may be needed
     *
     * Corresponds to Ada's Wake_Up procedure (hm11.adb:327-338)
     */
    void wake_up(hal::UartStatus& status) noexcept;

    /**
     * @brief Query module software version
     *
     * Sends "AT+VERS?" command and parses version string response.
     * Response format: "OK+Get:HMSoftV545" or similar.
     *
     * @param version Output parameter for version string (zero-copy view)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if version retrieved successfully
     * @post version contains module firmware version string
     * @post version is valid only while driver exists (view into internal buffer)
     *
     * @note Version string format varies by firmware (typically "HMSoftVxxx")
     * @note Version string length is variable (requires null-terminator search)
     *
     * Corresponds to Ada's Software_Version procedure (hm11.adb:~3150)
     */
    void software_version(std::string_view& version, hal::UartStatus& status) noexcept;

    // ========================================================================
    // Role and Connection Management Operations
    // ========================================================================

    /**
     * @brief Get current device role
     *
     * Sends "AT+ROLE?" command and parses role response (0=Peripheral, 1=Central).
     *
     * @param result Output parameter for device role
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if role retrieved successfully
     * @post result contains current device role (Peripheral or Central)
     *
     * Corresponds to Ada's Get_Role procedure (hm11.adb:169-181)
     */
    void get_role(types::Role& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set device role
     *
     * Sends "AT+ROLE0" (Peripheral) or "AT+ROLE1" (Central) command.
     * Response: "OK+Set:0" or "OK+Set:1"
     *
     * @param value Desired device role
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if role set successfully
     * @post Module role configured to specified value
     *
     * @warning See set_work_type() to avoid losing AT command capability
     * @note Default role is Peripheral
     *
     * Corresponds to Ada's Set_Role procedure (hm11.adb:187-195)
     */
    void set_role(types::Role value, hal::UartStatus& status) noexcept;

    /**
     * @brief Connect to device by MAC address
     *
     * Sends "AT+CO[type][address]" command to initiate connection with specified
     * device. This is a long-running operation that can take up to 10 seconds
     * on failure.
     *
     * @param mac_type MAC address type (Normal, Static, StaticRandom, Random)
     * @param address 12-character hex MAC address
     * @param result Output parameter for connection result status
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if command completed (check result for connection status)
     * @post result indicates connection outcome (Connecting, Connected, ConnectError, etc.)
     *
     * @note Can take ~10 seconds if remote device not connectable
     * @note MAC type is prefix: 0=Normal, 1=Static, 2=StaticRandom, 3=Random
     *
     * Corresponds to Ada's Connect procedure (hm11.adb:867-895)
     */
    void connect(
        types::MacAddressType mac_type,
        const types::MacAddress& address,
        types::ConnectResult& result,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Connect to discovered device by index
     *
     * Sends "AT+CONN[index]" command to connect to Nth discovered device from
     * last scan operation. Index 0 is first device, index 5 is sixth device.
     *
     * @param index Index in discovered devices list (0-5)
     * @param result Output parameter for connection result status
     * @param status Output parameter for operation result
     *
     * @pre Must execute scan() operation first to populate discovered devices
     * @pre Module role must be Central (Decision 8: document preconditions)
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if command completed (check result for connection status)
     * @post result indicates connection outcome (Connecting, Connected, ConnectError, etc.)
     * @post Discovered devices list is cleared after this command
     *
     * @note Only first 6 discovered devices can use array index
     * @note Other devices must use connect(mac_type, address, ...) overload
     *
     * Corresponds to Ada's Connect procedure (hm11.adb:901-925)
     */
    void connect(types::DiscoveredIndex index, types::ConnectResult& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Connect to last connected device
     *
     * Sends "AT+CONNL" command to reconnect to previously connected device.
     * Response: "OK+CONN[L/E/F/N]" where character indicates connection result.
     *
     * @param result Output parameter for connection result status
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     * @pre save_connected_mac must be enabled (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if command completed (check result for connection status)
     * @post result indicates connection outcome (Connecting, Connected, ConnectError, etc.)
     *
     * @note If remote device not connectable, ConnectFail received after ~10 seconds
     * @note Result character: L=Connecting, E=Error, F=Fail, N=NoAddress, 0-5=Connected
     *
     * Corresponds to Ada's Connect_Last_Device procedure (hm11.adb:773-791)
     */
    void connect_last_device(types::ConnectResult& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Disconnect from current connection
     *
     * Sends "AT" command to terminate current BLE connection.
     * Response: "OK+LOST" message.
     *
     * @param status Output parameter for operation result
     *
     * @pre Module must be connected (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if disconnect successful
     * @post Connection terminated
     *
     * Corresponds to Ada's Disconnect procedure (hm11.adb:355-360)
     */
    void disconnect(hal::UartStatus& status) noexcept;

    /**
     * @brief Get last connected device MAC address
     *
     * Sends "AT+RADD?" command to retrieve MAC address of last connected device.
     * Response: "OK+RADD:[12-char MAC address]"
     *
     * @param mac Output parameter for MAC address
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if address retrieved successfully
     * @post mac contains 12-character hex MAC address of last connected device
     *
     * Corresponds to Ada's Get_Last_Connected_Device_Address procedure (hm11.adb:201-220)
     */
    void get_last_connected_device_address(types::MacAddress& mac, hal::UartStatus& status) noexcept;

    /**
     * @brief Clear last connected device address
     *
     * Sends "AT+CLEAR" command to erase stored MAC address of last connected device.
     * Response: "OK+CLEAR"
     *
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if address cleared successfully
     * @post Last connected device address erased from module memory
     *
     * Corresponds to Ada's Clear_Last_Connected_Address procedure (hm11.adb:762-767)
     */
    void clear_last_connected_address(hal::UartStatus& status) noexcept;

    /**
     * @brief Set whether to save connected MAC address
     *
     * Sends "AT+SAVE[0/1]" command to enable/disable automatic saving of
     * connected device MAC address. Response: "OK+Set:[0/1]"
     *
     * @param save True to enable saving, false to disable
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if setting configured successfully
     * @post Module will save/not save connected MAC address based on setting
     *
     * @note Default: false (don't save)
     * @note When enabled, module auto-connects to saved MAC on power-up
     * @note Ada uses inverted logic: AT command 0=save, 1=don't save
     *
     * Corresponds to Ada's Set_Save_Connected_MAC procedure (hm11.adb:2345-2353)
     */
    void set_save_connected_mac(bool save, hal::UartStatus& status) noexcept;

    /**
     * @brief Get whether module saves connected MAC address
     *
     * Sends "AT+SAVE?" command to query auto-save setting.
     * Response: "OK+Get:[0/1]"
     *
     * @param save Output parameter for save setting
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if setting retrieved successfully
     * @post save indicates whether module saves connected MAC address
     *
     * @note Ada uses inverted logic: response 0=save, 1=don't save
     *
     * Corresponds to Ada's Get_Save_Connected_MAC procedure (hm11.adb:2359-2376)
     */
    void get_save_connected_mac(bool& save, hal::UartStatus& status) noexcept;

    /**
     * @brief Set connection timeout when connecting to remote device
     *
     * Sends "AT+TCON[6-digit timeout]" command to configure timeout in milliseconds.
     * Response: "OK+Set:[timeout]"
     *
     * @param timeout 6-digit timeout value in milliseconds (000000-999999)
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if timeout configured successfully
     * @post Module uses specified timeout when connecting to saved MAC on power-up
     *
     * @note Default: 000000 ms (no timeout, keeps trying indefinitely)
     * @note If non-zero, module enters scan mode after timeout expires
     * @note Only applies when module has saved MAC and attempts auto-connect on power-up
     *
     * Corresponds to Ada's Set_Connect_Remote_Device_Timeout procedure (hm11.adb:2553-2563)
     */
    void set_connect_remote_device_timeout(const types::ConnectTimeout& timeout, hal::UartStatus& status) noexcept;

    /**
     * @brief Get connection timeout for remote device
     *
     * Sends "AT+TCON?" command to query connection timeout setting.
     * Response: "OK+Get:[6-digit timeout]"
     *
     * @param timeout Output parameter for timeout value
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if timeout retrieved successfully
     * @post timeout contains 6-digit millisecond timeout value
     *
     * Corresponds to Ada's Get_Connect_Remote_Device_Timeout procedure (hm11.adb:2569-2587)
     */
    void get_connect_remote_device_timeout(types::ConnectTimeout& timeout, hal::UartStatus& status) noexcept;

    /**
     * @brief Start module working/advertising
     *
     * Sends "AT+START" command to begin module operation. Response: "OK+START"
     *
     * @param status Output parameter for operation result
     *
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     *
     * @post status == UartStatus::Ok if start command successful
     * @post Module enters active mode (advertising for Peripheral, scanning for Central)
     *
     * @note Only used when work_type = RespondAtCommand
     * @note Peripheral role: begins advertising
     * @note Central role: begins discovery/connection procedure
     *
     * Corresponds to Ada's Start_Working procedure (hm11.adb:2334-2339)
     */
    void start_working(hal::UartStatus& status) noexcept;

    // ========================================================================
    // Link Layer Configuration Operations
    // ========================================================================

    /**
     * @brief Get minimum link layer connection interval
     *
     * Sends "AT+COMI?" command and parses interval response (0-9).
     * Response format: "OK+Get:{0-9}"
     *
     * @param result Output parameter for minimum connection interval
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval retrieved successfully
     * @post result contains current minimum connection interval enum value
     *
     * @note Added in firmware V538
     * @note Default: 20ms
     * @note Used in Central role for connection parameter negotiation
     *
     * Corresponds to Ada's Get_Minimum_Link_Layer_Connection_Interval (hm11.adb:605-618)
     */
    void get_minimum_link_layer_connection_interval(
        types::LinkLayerConnectionInterval& result,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Set minimum link layer connection interval
     *
     * Sends "AT+COMI{0-9}" command to configure minimum connection interval.
     * Response format: "OK+Set:{0-9}"
     *
     * @param value Desired minimum connection interval (Ms7 through Ms4000)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval set successfully
     * @post Module minimum connection interval configured to specified value
     *
     * @note Added in firmware V538
     * @note Default: 20ms
     * @note Lower intervals improve latency but increase power consumption
     * @note Used in Central role for connection parameter negotiation
     *
     * Corresponds to Ada's Set_Minimum_Link_Layer_Connection_Interval (hm11.adb:590-599)
     */
    void set_minimum_link_layer_connection_interval(
        types::LinkLayerConnectionInterval value,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Get maximum link layer connection interval
     *
     * Sends "AT+COMA?" command and parses interval response (0-9).
     * Response format: "OK+Get:{0-9}"
     *
     * @param result Output parameter for maximum connection interval
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval retrieved successfully
     * @post result contains current maximum connection interval enum value
     *
     * @note Added in firmware V538
     * @note Default: 40ms
     * @note Used in Central role for connection parameter negotiation
     *
     * Corresponds to Ada's Get_Maximum_Link_Layer_Connection_Interval (hm11.adb:639-652)
     */
    void get_maximum_link_layer_connection_interval(
        types::LinkLayerConnectionInterval& result,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Set maximum link layer connection interval
     *
     * Sends "AT+COMA{0-9}" command to configure maximum connection interval.
     * Response format: "OK+Set:{0-9}"
     *
     * @param value Desired maximum connection interval (Ms7 through Ms4000)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval set successfully
     * @post Module maximum connection interval configured to specified value
     *
     * @note Added in firmware V538
     * @note Default: 40ms
     * @note Used in Central role for connection parameter negotiation
     *
     * Corresponds to Ada's Set_Maximum_Link_Layer_Connection_Interval (hm11.adb:624-633)
     */
    void set_maximum_link_layer_connection_interval(
        types::LinkLayerConnectionInterval value,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Get link layer connection slave latency
     *
     * Sends "AT+COLA?" command and parses latency response (0-4).
     * Response format: "OK+Get:{0-4}"
     *
     * @param result Output parameter for slave latency (0-4 connection events)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if latency retrieved successfully
     * @post result contains current slave latency value
     *
     * @note Added in firmware V538
     * @note Default: 0 (no latency)
     * @note Allows peripheral to skip connection events to save power
     *
     * Corresponds to Ada's Get_Link_Layer_Connection_Slave_Latency (hm11.adb:672-685)
     */
    void get_link_layer_connection_slave_latency(
        types::LayerConnectionLatency& result,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Set link layer connection slave latency
     *
     * Sends "AT+COLA{0-4}" command to configure slave latency parameter.
     * Response format: "OK+Set:{0-4}"
     *
     * @param value Desired slave latency (0-4 connection events can be skipped)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if latency set successfully
     * @post Module slave latency configured to specified value
     *
     * @note Added in firmware V538
     * @note Default: 0 (no latency)
     * @note Higher latency saves power but increases response latency
     *
     * Corresponds to Ada's Set_Link_Layer_Connection_Slave_Latency (hm11.adb:658-666)
     */
    void set_link_layer_connection_slave_latency(types::LayerConnectionLatency value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get connection supervision timeout
     *
     * Sends "AT+COSU?" command and parses timeout response (0-6).
     * Response format: "OK+Get:{0-6}"
     *
     * @param result Output parameter for supervision timeout
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if timeout retrieved successfully
     * @post result contains current supervision timeout enum value
     *
     * @note Added in firmware V538
     * @note Default: 6000ms (6 seconds)
     * @note Determines how long to wait before declaring connection lost
     *
     * Corresponds to Ada's Get_Connection_Supervision_Timeout (hm11.adb:706-719)
     */
    void get_connection_supervision_timeout(
        types::ConnectionSupervisionTimeout& result,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Set connection supervision timeout
     *
     * Sends "AT+COSU{0-6}" command to configure supervision timeout.
     * Response format: "OK+Set:{0-6}"
     *
     * @param value Desired supervision timeout (Ms100 through Ms6000)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if timeout set successfully
     * @post Module supervision timeout configured to specified value
     *
     * @note Added in firmware V538
     * @note Default: 6000ms (6 seconds)
     * @note Lower timeout detects disconnection faster but risks false positives
     *
     * Corresponds to Ada's Set_Connection_Supervision_Timeout (hm11.adb:691-700)
     */
    void set_connection_supervision_timeout(
        types::ConnectionSupervisionTimeout value,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Get update connection parameter
     *
     * Sends "AT+COUP?" command and parses boolean response (0-1).
     * Response format: "OK+Get:{0-1}"
     *
     * @param result Output parameter for update connection setting (true=enabled, false=disabled)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting retrieved successfully
     * @post result contains current update connection parameter
     *
     * @note Added in firmware V538
     * @note Only Peripheral role is used
     * @note Default: true (enabled)
     *
     * Corresponds to Ada's Get_Update_Connection (hm11.adb:739-756)
     */
    void get_update_connection(bool& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set update connection parameter
     *
     * Sends "AT+COUP{0-1}" command to enable/disable connection parameter updates.
     * Response format: "OK+Set:{0-1}"
     *
     * @param value Enable (true) or disable (false) connection parameter updates
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting updated successfully
     * @post Module update connection parameter configured to specified value
     *
     * @note Added in firmware V538
     * @note Only Peripheral role is used
     * @note In Central role, use set_minimum/maximum_link_layer_connection_interval instead
     * @note Default: true (enabled)
     *
     * Corresponds to Ada's Set_Update_Connection (hm11.adb:725-733)
     */
    void set_update_connection(bool value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get discovery time
     *
     * Sends "AT+SCAN?" command and parses time response (1-9).
     * Response format: "OK+Get:{1-9}"
     *
     * @param time Output parameter for discovery time in seconds (1-9)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if time retrieved successfully
     * @post time contains current discovery time in seconds
     *
     * @note Added in firmware V543
     * @note Only Central role is used
     * @note Default: 3 seconds
     *
     * Corresponds to Ada's Get_Discovery_Time (hm11.adb:2396-2413)
     */
    void get_discovery_time(types::DiscoveryTime& time, hal::UartStatus& status) noexcept;

    /**
     * @brief Set discovery time
     *
     * Sends "AT+SCAN{1-9}" command to configure discovery scan duration.
     * Response format: "OK+Set:{1-9}"
     *
     * @param time Desired discovery time in seconds (1-9)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if time set successfully
     * @post Module discovery time configured to specified value
     *
     * @note Added in firmware V543
     * @note Only Central role is used
     * @note Longer times increase chance of discovering devices but slow scanning
     * @note Default: 3 seconds
     *
     * Corresponds to Ada's Set_Discovery_Time (hm11.adb:2382-2390)
     */
    void set_discovery_time(types::DiscoveryTime time, hal::UartStatus& status) noexcept;

    // ========================================================================
    // Advertising Configuration Operations
    // ========================================================================

    /**
     * @brief Get current advertising interval
     *
     * Sends "AT+ADVI?" command and parses interval response (0-9, A-F).
     * Response format: "OK+Get:{0-9|A-F}"
     *
     * @param result Output parameter for advertising interval
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval retrieved successfully
     * @post result contains current advertising interval enum value
     *
     * @note Response character maps to interval: 0=100ms, 9=1285ms, F=7000ms
     * @note Apple recommends maximum 1285ms (9) for iOS compatibility
     *
     * Corresponds to Ada's Get_Advertising_Interval procedure (hm11.adb:366-397)
     */
    void get_advertising_interval(types::AdvertisingInterval& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set advertising interval
     *
     * Sends "AT+ADVI{0-9|A-F}" command to configure advertising interval.
     * Response format: "OK+Set:{0-9|A-F}"
     *
     * @param value Desired advertising interval (ms_100 through ms_7000)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if interval set successfully
     * @post Module advertising interval configured to specified value
     *
     * @note Lower intervals increase power consumption but improve discoverability
     * @note Apple recommends maximum 1285ms for iOS device compatibility
     * @note Setting applies to Peripheral role advertising
     *
     * Corresponds to Ada's Set_Advertising_Interval procedure (hm11.adb:403-411)
     */
    void set_advertising_interval(types::AdvertisingInterval value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current advertising type
     *
     * Sends "AT+ADTY?" command and parses type response (0-3).
     * Response format: "OK+Get:{0-3}"
     *
     * @param result Output parameter for advertising type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if type retrieved successfully
     * @post result contains current advertising type enum value
     *
     * Corresponds to Ada's Get_Advertising_Type procedure (hm11.adb:417-430)
     */
    void get_advertising_type(types::AdvertisingType& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set advertising type
     *
     * Sends "AT+ADTY{0-3}" command to configure advertising packet type.
     * Response format: "OK+Set:{0-3}"
     *
     * @param value Desired advertising type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if type set successfully
     * @post Module advertising type configured to specified value
     *
     * @note Type 0: Full advertising (scannable, connectable) - default
     * @note Type 1: Only connect to last paired device
     * @note Type 2: Advertising with scan response (not connectable)
     * @note Type 3: Advertising only (no scan response, not connectable)
     *
     * Corresponds to Ada's Set_Advertising_Type procedure (hm11.adb:436-444)
     */
    void set_advertising_type(types::AdvertisingType value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current advertising mode
     *
     * Sends "AT+RELI?" command and parses mode response (0-1).
     * Response format: "OK+Get:{0-1}"
     *
     * @param result Output parameter for advertising mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if mode retrieved successfully
     * @post result contains current advertising mode enum value
     *
     * Corresponds to Ada's Get_Reliable_Advertising_Mode procedure (hm11.adb:2293-2306)
     */
    void get_advertising_mode(types::AdvertisingMode& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set advertising mode (reliable or normal)
     *
     * Sends "AT+RELI{0-1}" command to configure advertising reliability mode.
     * Response format: "OK+Set:{0-1}"
     *
     * @param value Desired advertising mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if mode set successfully
     * @post Module advertising mode configured to specified value
     *
     * @note Mode 0: Normal advertising (default)
     * @note Mode 1: Reliable advertising with acknowledgment
     *
     * Corresponds to Ada's Set_Reliable_Advertising_Mode procedure (hm11.adb:2279-2287)
     */
    void set_advertising_mode(types::AdvertisingMode value, hal::UartStatus& status) noexcept;

    /**
     * @brief Set advertising flags
     *
     * Sends "AT+FLAG{hex}" command to configure advertising flags byte.
     * Response format: "OK+Set:{hex}"
     *
     * @param flag 2-character hex advertising flags (00-FF)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if flags set successfully
     * @post Module advertising flags configured to specified value
     *
     * @note Common values: 06 = LE General Discoverable + BR/EDR Not Supported
     * @note Flags affect discoverability and BR/EDR capability advertisement
     *
     * Corresponds to Ada's Set_Advertising_FLAG procedure (hm11.adb:1511-1521)
     */
    void set_advertising_flag(const types::AdvertisingFlag& flag, hal::UartStatus& status) noexcept;

    /**
     * @brief Set custom advertisement data
     *
     * Sends "AT+PACK{hex}" command to configure custom advertising data payload.
     * Response format: "OK+Set:{hex}"
     *
     * @param data 12-character hex custom data (6 bytes)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if data set successfully
     * @post Module includes custom data in advertising packets
     *
     * @note Data is 48-bit (6 bytes) custom payload included in advertising packets
     * @note Can be used to broadcast application-specific information
     *
     * Corresponds to Ada's Set_Module_Advertisement_Data procedure (hm11.adb:2699-2709)
     */
    void set_advertising_data(const types::AdvertisementData& data, hal::UartStatus& status) noexcept;

    /**
     * @brief Set show device information when discovery
     *
     * Sends "AT+SHOW{0-3}" command to configure what information is included
     * in scan result packages during discovery operations.
     * Response format: "OK+Set:{0-3}"
     *
     * @param show Desired device information display mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting configured successfully
     * @post Module discovery results include specified information
     *
     * @note Option 0: Don't show device information (default)
     * @note Option 1: Show device name only
     * @note Option 2: Show RSSI only
     * @note Option 3: Show both RSSI and device name
     * @note This affects scan result data format, not advertising packets
     *
     * Corresponds to Ada's Set_Show_Device_Information_When_Discovery (hm11.adb:2452-2460)
     */
    void set_show_device_information_when_discovery(
        types::ShowDeviceInformation show,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Get show device information when discovery setting
     *
     * Sends "AT+SHOW?" command and parses information display mode (0-3).
     * Response format: "OK+Get:{0-3}"
     *
     * @param show Output parameter for device information display mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting retrieved successfully
     * @post show contains current device information display mode
     *
     * Corresponds to Ada's Get_Show_Device_Information_When_Discovery (hm11.adb:2466-2479)
     */
    void get_show_device_information_when_discovery(
        types::ShowDeviceInformation& show,
        hal::UartStatus& status) noexcept;

    // ========================================================================
    // Device Management Operations
    // ========================================================================

    /**
     * @brief Get module name
     *
     * Sends "AT+NAME?" command and parses variable-length name response.
     * Response format: "OK+NAME:{name}" where name is 1-12 characters.
     *
     * @param name Output parameter for module name (zero-copy view)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if name retrieved successfully
     * @post name contains module name string (1-12 characters)
     * @post name is valid only while driver exists (view into internal buffer)
     *
     * @note Default name: "HMSoft"
     * @note Name length is variable, requires null-terminator or CRLF search
     *
     * Corresponds to Ada's Get_Module_Name function (hm11.adb:1987-2007)
     */
    void get_module_name(std::string_view& name, hal::UartStatus& status) noexcept;

    /**
     * @brief Set module name
     *
     * Sends "AT+NAME{name}" command to configure visible device name.
     * Response format: "OK+Set:{name}"
     *
     * @param name Desired module name (1-12 characters)
     * @param status Output parameter for operation result
     *
     * @pre name.size() >= 1 && name.size() <= 12 (enforced by assertion in debug)
     *
     * @post status == UartStatus::Ok if name set successfully
     * @post Module name configured to specified value
     * @post Name visible in discovery scans
     *
     * @note Default name: "HMSoft"
     * @note Name persists across power cycles
     * @note Invalid name length causes assertion failure in debug builds
     *
     * Corresponds to Ada's Set_Module_Name procedure (hm11.adb:1975-1981)
     */
    void set_module_name(std::string_view name, hal::UartStatus& status) noexcept;

    /**
     * @brief Get module MAC address
     *
     * Sends "AT+ADDR?" command to retrieve module's Bluetooth MAC address.
     * Response format: "OK+ADDR:{12-char MAC}"
     *
     * @param mac Output parameter for MAC address
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if MAC address retrieved successfully
     * @post mac contains 12-character hex MAC address
     *
     * @note MAC address format: "B4994C123456" (no delimiters)
     * @note MAC address is factory-programmed and cannot be changed
     * @note iOS cannot get MAC directly, use advertising packet instead
     *
     * Corresponds to Ada's Get_MAC_Address procedure (hm11.adb:797-816)
     */
    void get_mac_address(types::MacAddress& mac, hal::UartStatus& status) noexcept;

    /**
     * @brief Get battery information (query current level)
     *
     * Sends "AT+BATT?" command to query current battery level.
     * Response format: "OK+Get:{percentage}" (1-3 digits)
     *
     * @param result Output parameter for battery percentage (0-100)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if battery level retrieved successfully
     * @post result contains battery percentage value
     *
     * @note Requires battery monitoring to be enabled
     * @note Returns actual measured battery level
     *
     * Corresponds to Ada's Query_Battery_Information procedure (hm11.adb:534-551)
     */
    void query_battery_information(types::Percent& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set battery information (configure reported level)
     *
     * Sends "AT+BATT{percentage}" command to set battery level for reporting.
     * Response format: "OK+BATT{xxx}" where xxx is 3-digit percentage
     *
     * @param value Battery percentage to report (0-100)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if battery level set successfully
     * @post Module reports specified battery level
     *
     * @note This sets the reported value, not measured value
     * @note Used when external battery monitoring is implemented
     *
     * Corresponds to Ada's Set_Battery_Information procedure (hm11.adb:518-528)
     */
    void set_battery_information(types::Percent value, hal::UartStatus& status) noexcept;

    /**
     * @brief Set battery monitor switch
     *
     * Sends "AT+BATC{0/1}" command to enable/disable battery monitoring.
     * Response format: "OK+Set:{0/1}"
     *
     * @param enable True to enable battery monitoring, false to disable
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting configured successfully
     * @post Battery monitoring enabled or disabled based on parameter
     *
     * @note When enabled, module monitors battery voltage
     * @note Battery level available via query_battery_information()
     *
     * Corresponds to Ada's Set_Battery_Monitor_Switch procedure (hm11.adb:484-492)
     */
    void set_battery_monitor_switch(bool enable, hal::UartStatus& status) noexcept;

    /**
     * @brief Get battery monitor switch
     *
     * Sends "AT+BATC?" command to query battery monitoring status.
     * Response format: "OK+Get:{0/1}"
     *
     * @param enabled Output parameter for battery monitoring status
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if setting retrieved successfully
     * @post enabled indicates whether battery monitoring is active
     *
     * Corresponds to Ada's Get_Battery_Monitor_Switch procedure (hm11.adb:498-512)
     */
    void get_battery_monitor_switch(bool& enabled, hal::UartStatus& status) noexcept;

    /**
     * @brief Get module internal temperature
     *
     * Sends "AT+TEMP?" command to query module's internal temperature sensor.
     * Response format: "OK+Get:{temperature}" (variable length, e.g., "25" or "100")
     *
     * @param temperature Output parameter for temperature string
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if temperature retrieved successfully
     * @post temperature contains temperature string (zero-copy view)
     * @post temperature is valid only while driver exists (view into internal buffer)
     *
     * @note Returns internal die temperature, not ambient temperature
     * @note Temperature format is variable length decimal string
     * @note For accurate temperature sensing, use external sensor (DHT11/DS18B20)
     *
     * Corresponds to Ada's Get_Module_Temperature procedure (hm11.adb:2512-2530)
     */
    void get_module_temperature(std::string_view& temperature, hal::UartStatus& status) noexcept;

    /**
     * @brief Set temperature information byte in advertising
     *
     * Sends "AT+TEMP0x{hex}" command to include temperature in advertising packets.
     * Response format: "OK+Set:0x{hex}"
     *
     * @param temperature 2-character hex temperature value (00-FF)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if temperature configured successfully
     * @post Module includes temperature byte in advertising packets
     *
     * @note Temperature is 8-bit hex value for advertising payload
     * @note Does not affect internal temperature sensor reading
     * @note Used to broadcast temperature data to scanning devices
     *
     * Corresponds to Ada's Set_Temperature_Information_Byte_In_Advertising (hm11.adb:2536-2547)
     */
    void set_temperature_information_byte_in_advertising(
        const types::AdvertisingTemperatureType& temperature,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Set humidity information byte in advertising
     *
     * Sends "AT+HUMI{hex}" command to include humidity in advertising packets.
     * Response format: "OK+Set:{hex}"
     *
     * @param humidity 2-character hex humidity value (00-FF)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if humidity configured successfully
     * @post Module includes humidity byte in advertising packets
     *
     * @note Humidity is 8-bit hex value for advertising payload
     * @note Used to broadcast humidity data to scanning devices
     *
     * Corresponds to Ada's Set_Humi_Information_Byte_In_Advertising (hm11.adb:1596-1606)
     */
    void set_humi_information_byte_in_advertising(
        const types::HumiInformation& humidity,
        hal::UartStatus& status) noexcept;

    // ========================================================================
    // Discovery and Scanning Operations
    // ========================================================================

    /**
     * @brief Scan for nearby BLE devices (streaming operation)
     *
     * Sends "AT+DISC?" command and parses streaming discovery responses.
     * For each discovered device, invokes callback with device information
     * (ID, MAC, name, RSSI). Uses state machine parsing (Decision 11) to
     * handle variable-length responses.
     *
     * ## Operation Overview
     *
     * This is a long-running streaming operation that can discover multiple
     * devices. The module streams discovery data in this format:
     *
     * @code
     * OK+DISCS              // Discovery hello
     * OK+DISC:B4994C123456  // Device MAC address
     * OK+NAME:MyDevice      // Device name (if configured)
     * OK+RSSI:-045          // RSSI value (if configured)
     * ...                   // More devices
     * OK+DISCE              // Discovery complete
     * @endcode
     *
     * ## State Machine Parsing (Design Pattern #3)
     *
     * Uses explicit state enum with switch/case (Decision 11):
     * - **Selection**: Looking for prefixes (OK+DISCS, OK+DISC:, OK+NAME:, OK+RSSI:, OK+DISCE)
     * - **MAC**: Parsing 12-byte MAC address
     * - **Name**: Parsing device name until CRLF
     * - **RSSI**: Parsing RSSI until CRLF, then invoke callback
     *
     * ## Callback Invocation (Design Pattern #4, Decision 10)
     *
     * Callback is invoked inline during parsing when complete device is
     * accumulated (MAC + Name + RSSI). Uses plain C function pointer for
     * zero overhead. Callback receives string_view parameters (zero-copy).
     *
     * ## Temporary Storage (Decision 27)
     *
     * Uses single DiscoveredInfo temporary (not array) to accumulate data:
     * - Name: VariableString<248> (matches Ada's Max_Name_Length)
     * - RSSI: VariableString<4> (matches Ada's Max_RSII_Length)
     * - Total memory: 248 + 4 + 12 + metadata bytes
     *
     * After callback, temporary is reset for next device.
     *
     * ## Soft-Stop Timeout (Decision 28)
     *
     * When timeout expires (stream closes), completes parsing of current
     * response and invokes callback with any accumulated partial data before
     * returning. This ensures users receive all parseable data rather than
     * truncated results.
     *
     * @tparam Rep Arithmetic type representing timeout duration ticks
     * @tparam Period std::ratio representing tick period (duration units)
     * @param callback Function pointer to invoke for each discovered device
     * @param timeout Maximum time to wait for discovery operation (milliseconds)
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     * @pre Set_Show_Device_Information_When_Discovery configures name/RSSI inclusion
     *
     * @post status == UartStatus::Ok if discovery completed successfully
     * @post status == UartStatus::ErrTimeout if operation timed out
     * @post status == UartStatus::ErrError if UART error occurred
     * @post Callback invoked for each discovered device (potentially multiple times)
     *
     * @note Can take several seconds if many devices nearby
     * @note Name and RSSI may be empty if not configured
     * @note "OK+DISCE" end marker terminates scan early
     *
     * Corresponds to Ada's Scan procedure (hm11.adb:931-1081)
     */
    template <typename Rep, typename Period>
    void
    scan(DiscoveredCallback callback, std::chrono::duration<Rep, Period> timeout, hal::UartStatus& status) noexcept;

    /**
     * @brief Scan for nearby iBeacon devices (streaming operation)
     *
     * Sends "AT+DISI?" command and parses streaming iBeacon discovery responses.
     * For each discovered iBeacon device, invokes callback with full device
     * information string in P0:P1:P2:P3:P4 format.
     *
     * ## Response Format
     *
     * @code
     * OK+DISCS              // Discovery hello
     * OK+DISC{74-char data} // P0(8):P1(32):P2(10):P3(12):P4(4)
     * ...                   // More devices
     * OK+DISCE              // Discovery complete
     * @endcode
     *
     * ## Device Info Format (hm11.ads:154-167)
     *
     * Each device is 74 characters: P0:P1:P2:P3:P4 where:
     * - **P0**: Factory ID (8 bytes)
     * - **P1**: iBeacon UUID (32 bytes)
     * - **P2**: Major(4) + Minor(4) + Measured Power(2) = 10 bytes
     * - **P3**: MAC address (12 bytes)
     * - **P4**: RSSI (4 bytes)
     *
     * If device not iBeacon, P0/P1/P2 filled with '0' characters.
     *
     * ## Parsing Strategy
     *
     * Simpler than regular Scan - waits for fixed-length data blocks after
     * "OK+DISC" prefix. When 74 characters available after prefix, copies
     * to temporary buffer and invokes callback.
     *
     * @tparam Rep Arithmetic type representing timeout duration ticks
     * @tparam Period std::ratio representing tick period (duration units)
     * @param callback Function pointer to invoke for each discovered iBeacon
     * @param timeout Maximum time to wait for discovery operation (milliseconds)
     * @param status Output parameter for operation result
     *
     * @pre Module role must be Central (Decision 8: document preconditions)
     * @pre Work type must be RespondAtCommand (Decision 8: document preconditions)
     * @pre Firmware version V539+ (feature added in V539)
     *
     * @post status == UartStatus::Ok if discovery completed successfully
     * @post status == UartStatus::ErrTimeout if operation timed out
     * @post status == UartStatus::ErrError if UART error occurred
     * @post Callback invoked for each discovered iBeacon device
     *
     * @note Added in firmware V539
     * @note Non-iBeacon devices may appear with zeros in P0/P1/P2
     *
     * Corresponds to Ada's Scan_iBeacon procedure (hm11.adb:1087-1156)
     */
    template <typename Rep, typename Period>
    void scan_ibeacon(
        DiscoveredIBeaconCallback callback,
        std::chrono::duration<Rep, Period> timeout,
        hal::UartStatus& status) noexcept;

    // ========================================================================
    // GATT Operations (Service and Characteristic Discovery)
    // ========================================================================

    /**
     * @brief Find all GATT services on connected peripheral device (streaming operation)
     *
     * Sends "AT+FINDSERVICES?" command to enumerate all GATT services.
     * Receives streaming response with service information and invokes callback for each service.
     *
     * Response format (streaming):
     * @code
     * ********************************************************
     * [start_handle]:[end_handle]:[uuid]CRLF
     * [start_handle]:[end_handle]:[uuid]CRLF
     * ...
     * ********************************************************
     * @endcode
     *
     * Service info format: "start:end:uuid" (4:4:4 hex characters, 14 total with colons)
     * - start_handle: 4 hex characters (service start handle)
     * - end_handle: 4 hex characters (service end handle)
     * - uuid: 4 hex characters (16-bit service UUID)
     *
     * Example service info: "0001:000B:1800" (Generic Access service, handles 0x0001-0x000B)
     *
     * @param callback Function pointer invoked for each discovered service
     * @param timeout Maximum time to wait for streaming response (typically 1-5 seconds)
     * @param status Output parameter for operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @post status == UartStatus::Ok if discovery completed successfully
     * @post Callback invoked for each service found
     *
     * @note Added in firmware V700+
     * @note Uses streaming response with timeout (Decision 28: soft-stop semantics)
     * @note Discovery can take several seconds depending on peripheral GATT structure
     * @note Simpler parsing than Scan: colon-delimited fields rather than state machine
     *
     * Corresponds to Ada's Find_All_Services_UUID procedure (hm11.adb:1162-1263)
     */
    template <typename Rep, typename Period>
    void find_all_services_uuid(
        ServiceUuidCallback callback,
        std::chrono::duration<Rep, Period> timeout,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Find all GATT characteristics on connected peripheral device (streaming operation)
     *
     * Sends "AT+FINDALLCHARS?" command to enumerate all GATT characteristics across all services.
     * Receives streaming response with characteristic information and invokes callback for each.
     *
     * Response format (streaming):
     * @code
     * ********************************************************
     * [handle]:[properties]:[uuid]CRLF
     * [handle]:[properties]:[uuid]CRLF
     * ...
     * ********************************************************
     * @endcode
     *
     * Characteristic info format: "handle:properties:uuid" (4:14:4 hex/property characters, 24 total with colons)
     * - handle: 4 hex characters (characteristic handle)
     * - properties: 14 characters (property flags: "RD|WR|WN|NO|IN", unsupported as "--")
     * - uuid: 4 hex characters (16-bit characteristic UUID)
     *
     * Properties string details:
     * - RD: Read property
     * - WR: Write property
     * - WN: Write Without Response property
     * - NO: Notify property
     * - IN: Indicate property
     * - "--" replaces unsupported properties (e.g., "RD|--|--|NO|--" = Read + Notify)
     *
     * Example characteristic info: "0003:RD|WR|--|--|--:2A00" (Device Name characteristic with Read+Write)
     *
     * @param callback Function pointer invoked for each discovered characteristic
     * @param timeout Maximum time to wait for streaming response (typically 2-10 seconds)
     * @param status Output parameter for operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @post status == UartStatus::Ok if discovery completed successfully
     * @post Callback invoked for each characteristic found
     *
     * @note Added in firmware V700+
     * @note Uses streaming response with timeout (Decision 28: soft-stop semantics)
     * @note Discovery can take several seconds depending on peripheral GATT structure
     * @note Use Find_All_Services_UUID first to understand service boundaries
     *
     * Corresponds to Ada's Find_All_Characteristic_UUID procedure (hm11.adb:1269-1279)
     */
    template <typename Rep, typename Period>
    void find_all_characteristic_uuid(
        CharacteristicUuidCallback callback,
        std::chrono::duration<Rep, Period> timeout,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Find GATT characteristics within specified handle range (streaming operation)
     *
     * Sends "AT+CHAR{from}{to}?" command to enumerate characteristics within a service.
     * Receives streaming response with characteristic information and invokes callback for each.
     * This is useful for discovering characteristics within a specific service (use service
     * start/end handles from Find_All_Services_UUID).
     *
     * Response format (streaming): Same as Find_All_Characteristic_UUID
     * @code
     * ********************************************************
     * [handle]:[properties]:[uuid]CRLF
     * [handle]:[properties]:[uuid]CRLF
     * ...
     * ********************************************************
     * @endcode
     *
     * Characteristic info format: "handle:properties:uuid" (4:14:4, 24 total with colons)
     *
     * Example usage:
     * @code
     * // After Find_All_Services_UUID callback provides "0001:000B:1800"
     * types::HandleType from = types::HandleType("0001");
     * types::HandleType to = types::HandleType("000B");
     * driver.find_characteristic_uuid(from, to, my_callback, 2000ms, status);
     * @endcode
     *
     * @param from Start handle of search range (4 hex characters, typically service start handle)
     * @param to End handle of search range (4 hex characters, typically service end handle)
     * @param callback Function pointer invoked for each discovered characteristic
     * @param timeout Maximum time to wait for streaming response (typically 1-5 seconds)
     * @param status Output parameter for operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre from <= to (handle range validity)
     * @post status == UartStatus::Ok if discovery completed successfully
     * @post Callback invoked for each characteristic found in range
     *
     * @note Added in firmware V700+
     * @note Uses streaming response with timeout (Decision 28: soft-stop semantics)
     * @note More efficient than Find_All_Characteristic_UUID when service boundaries are known
     * @note Get from/to handles from Find_All_Services_UUID command
     *
     * Corresponds to Ada's Find_Characteristic_UUID procedure (hm11.adb:1285-1305)
     */
    template <typename Rep, typename Period>
    void find_characteristic_uuid(
        const types::HandleType& from,
        const types::HandleType& to,
        CharacteristicUuidCallback callback,
        std::chrono::duration<Rep, Period> timeout,
        hal::UartStatus& status) noexcept;

    // ========================================================================
    // GATT Characteristic Access Operations
    // ========================================================================

    /**
     * @brief Enable notify/indicate on a GATT characteristic
     *
     * Sends "AT+NOTIFY_ON{handle}" command to enable notifications or indications
     * on a characteristic that supports the notify or indicate property.
     * Response format: "OK+SEND-OK", "OK+SEND-ER", or "OK+DATA-ER"
     *
     * This subscribes the Central to receive notifications/indications from the
     * Peripheral when the characteristic value changes.
     *
     * @param handle Characteristic handle (4 hex characters, from GATT discovery)
     * @param status Output parameter for UART operation result
     * @param response Output parameter for notify operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre Characteristic discovered via find_characteristic_uuid or find_all_characteristic_uuid
     * @pre Characteristic supports notify or indicate property
     * @post status == UartStatus::Ok if command sent and response received successfully
     * @post response indicates operation result:
     *       - NotifyResponse::SendOk: Notify enabled successfully
     *       - NotifyResponse::SendEr: Command error
     *       - NotifyResponse::DataEr: Characteristic doesn't have notify property
     *
     * @note Added in firmware V700+
     * @note Only Central role can use this command
     * @note After enabling, peripheral will push notifications when characteristic value changes
     * @note Use Read_Characteristic_Notify to read current value
     *
     * Corresponds to Ada's Enable_Characteristic_Notify procedure (hm11.adb:1387-1395)
     */
    void enable_characteristic_notify(
        const types::HandleType& handle,
        hal::UartStatus& status,
        types::NotifyResponse& response) noexcept;

    /**
     * @brief Disable notify/indicate on a GATT characteristic
     *
     * Sends "AT+NOTIFYOFF{handle}" command to disable notifications or indications
     * on a characteristic that was previously enabled.
     * Response format: "OK+SEND-OK", "OK+SEND-ER", or "OK+DATA-ER"
     *
     * This unsubscribes the Central from receiving notifications/indications.
     *
     * @param handle Characteristic handle (4 hex characters, from GATT discovery)
     * @param status Output parameter for UART operation result
     * @param response Output parameter for notify operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre Characteristic notifications previously enabled via enable_characteristic_notify
     * @post status == UartStatus::Ok if command sent and response received successfully
     * @post response indicates operation result:
     *       - NotifyResponse::SendOk: Notify disabled successfully
     *       - NotifyResponse::SendEr: Command error
     *       - NotifyResponse::DataEr: Characteristic doesn't have notify property
     *
     * @note Added in firmware V700+
     * @note Only Central role can use this command
     * @note After disabling, peripheral stops sending notifications
     *
     * Corresponds to Ada's Disable_Characteristic_Notify procedure (hm11.adb:1401-1409)
     */
    void disable_characteristic_notify(
        const types::HandleType& handle,
        hal::UartStatus& status,
        types::NotifyResponse& response) noexcept;

    /**
     * @brief Read GATT characteristic value
     *
     * Sends "AT+READDATA{handle}" command to read the current value of a characteristic
     * that supports the read property.
     * Response format: "OK+SEND-OK", "OK+SEND-ER", or "OK+DATA-ER"
     *
     * This performs a one-time read of the characteristic value.
     *
     * @param handle Characteristic handle (4 hex characters, from GATT discovery)
     * @param status Output parameter for UART operation result
     * @param response Output parameter for notify operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre Characteristic discovered via find_characteristic_uuid or find_all_characteristic_uuid
     * @pre Characteristic supports read property
     * @post status == UartStatus::Ok if command sent and response received successfully
     * @post response indicates operation result:
     *       - NotifyResponse::SendOk: Read successful
     *       - NotifyResponse::SendEr: Command error
     *       - NotifyResponse::DataEr: Characteristic doesn't have read property
     *
     * @note Added in firmware V700+
     * @note Only Central role can use this command
     * @note For continuous monitoring, use enable_characteristic_notify instead
     *
     * Corresponds to Ada's Read_Characteristic_Notify procedure (hm11.adb:1415-1423)
     */
    void read_characteristic_notify(
        const types::HandleType& handle,
        hal::UartStatus& status,
        types::NotifyResponse& response) noexcept;

    /**
     * @brief Configure default characteristic handle and method for transparent data exchange
     *
     * Sends "AT+SET_WAY{method}{handle}" command to configure a default characteristic
     * for subsequent transparent UART data transmission. After this command, UART data
     * is automatically sent to the configured characteristic without AT command prefix.
     * Response format: "OK+SEND-OK"
     *
     * This enables transparent bidirectional communication mode where UART data flows
     * directly to/from the GATT characteristic.
     *
     * Example usage:
     * @code
     * types::HandleType handle = types::HandleType("002A");
     * driver.set_method_and_characteristic_handle(handle, SendDataMethod::Write, status);
     * // Now UART data is automatically written to handle 002A with Write method
     * @endcode
     *
     * @param handle Characteristic handle (4 hex characters, from GATT discovery)
     * @param method Data transmission method (Write, WriteWithoutResponse, Indicate, Notify)
     * @param status Output parameter for operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre Characteristic discovered and supports selected method property
     * @post status == UartStatus::Ok if configuration successful
     * @post Subsequent UART data automatically sent to configured characteristic
     *
     * @note Added in firmware V701+
     * @note Only Central role can use this command
     * @note Only needs to be executed once (unlike Send_Data_To_Characteristic)
     * @note Preferred over Send_Data_To_Characteristic for continuous data exchange
     * @note To exit transparent mode, send "AT" command prefix to re-enter command mode
     *
     * Corresponds to Ada's Set_Method_And_Characteristic_Handle procedure (hm11.adb:1463-1472)
     */
    void set_method_and_characteristic_handle(
        const types::HandleType& handle,
        types::SendDataMethod method,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Send data to GATT characteristic with explicit handle and method
     *
     * Sends "AT+SEND_DATA{method}{handle}{data}" command to write data to a
     * characteristic. Unlike set_method_and_characteristic_handle, this requires
     * the AT command prefix for each transmission.
     * Response format: Varies based on method (typically no response for write-without-response)
     *
     * This is the legacy method for sending data. For continuous data exchange,
     * prefer set_method_and_characteristic_handle (V701+).
     *
     * Example usage:
     * @code
     * types::HandleType handle = types::HandleType("002A");
     * std::array<uint8_t, 5> data = {0x01, 0x02, 0x03, 0x04, 0x05};
     * driver.send_data_to_characteristic(
     *     handle,
     *     SendDataCharacteristic::Write,
     *     hal::UartData8bConst(data),
     *     status);
     * @endcode
     *
     * @param handle Characteristic handle (4 hex characters, from GATT discovery)
     * @param method Data transmission method (Write or WriteWithoutResponse)
     * @param data Data payload to send (1-20 bytes for BLE, read-only span of uint8_t)
     * @param status Output parameter for operation result
     *
     * @pre Central role (Set_Work_Mode(Role::Central, ...))
     * @pre Connected to peripheral device (Connect(...) successful)
     * @pre Characteristic discovered and supports selected method property
     * @pre data.size() <= 20 (BLE maximum payload size per packet)
     * @post status == UartStatus::Ok if data sent successfully
     *
     * @note Added in firmware V700+
     * @note Only Central role can use this command
     * @note Since V701, Set_Method_And_Characteristic_Handle is preferred
     * @note Maximum data length is 20 bytes per BLE specification
     * @note method parameter is restricted to Write/WriteWithoutResponse only
     *
     * Corresponds to Ada's Send_Data_To_Characteristic procedure (hm11.adb:1478-1488)
     */
    void send_data_to_characteristic(
        const types::HandleType& handle,
        types::SendDataCharacteristic method,
        hal::UartData8bConst data,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Configure characteristic UUID query/usage mode for Peripheral role
     *
     * Sends "AT+FFE2{count}" command to configure how many characteristic UUIDs
     * the peripheral uses for GATT operations. This affects how the peripheral
     * responds to central characteristic discovery.
     * Response format: "OK+Set:{count}"
     *
     * Mode descriptions:
     * - Query: Query current setting (returns current value)
     * - One: Use single characteristic (from Set_Characteristic command)
     * - OneAndNext: Use two characteristics (current and UUID+1)
     * - OneAndPrev: Use two characteristics (current and UUID-1)
     *
     * @param count Characteristic UUID count mode
     * @param status Output parameter for operation result
     *
     * @pre Peripheral role (Set_Work_Mode(Role::Peripheral, ...))
     * @post status == UartStatus::Ok if configuration successful
     * @post Peripheral uses configured number of characteristics
     *
     * @note Added in firmware V550+
     * @note Only Peripheral role uses this configuration
     * @note Affects GATT database structure exposed to Central devices
     * @note Must be configured before connection establishment
     *
     * Corresponds to Ada's Set_Use_Characteristic_UUID_Count procedure (hm11.adb:1494-1505)
     */
    void set_use_characteristic_uuid_count(types::CharacteristicUuidCount count, hal::UartStatus& status) noexcept;

    /**
     * @brief Set peripheral characteristic UUID
     *
     * Sends "AT+CHAR0x{uuid}" command to configure the characteristic UUID
     * for peripheral GATT database. This defines the characteristic that
     * central devices will discover and interact with.
     * Response format: "OK+Set:0x{uuid}"
     *
     * @param value Characteristic UUID (4 hex characters, e.g., "FFE1")
     * @param status Output parameter for operation result
     *
     * @pre Peripheral role (Set_Work_Mode(Role::Peripheral, ...))
     * @post status == UartStatus::Ok if UUID set successfully
     * @post Peripheral advertises with configured characteristic UUID
     *
     * @note Default: "FFE1"
     * @note Must be configured before connection establishment
     * @note Used in conjunction with Set_Use_Characteristic_UUID_Count
     *
     * Corresponds to Ada's Set_Characteristic procedure (hm11.adb:2753-2765)
     */
    void set_characteristic(const types::CharacteristicType& value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current peripheral characteristic UUID
     *
     * Sends "AT+CHAR?" command to query the currently configured characteristic UUID.
     * Response format: "OK+Get:0x{uuid}" (e.g., "OK+Get:0xFFE1")
     *
     * @param result Output parameter for characteristic UUID (4 hex characters)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if query successful
     * @post result contains current characteristic UUID
     *
     * @note Default: "FFE1"
     * @note Response includes "0x" prefix which is parsed and stored in result
     *
     * Corresponds to Ada's Get_Characteristic procedure (hm11.adb:2771-2788)
     */
    void get_characteristic(types::CharacteristicType& result, hal::UartStatus& status) noexcept;

    // ========================================================================
    // iBeacon Configuration Operations
    // ========================================================================

    /**
     * @brief Set iBeacon switch (enable/disable iBeacon mode)
     *
     * Sends "AT+IBEA{0|1}" command to enable or disable iBeacon functionality.
     * Response format: "OK+Set:{0|1}"
     *
     * @param switch_on True to enable iBeacon mode, false to disable
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if switch set successfully
     * @post iBeacon mode enabled or disabled based on switch_on parameter
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Added in firmware version V517+
     * @note Default: false (disabled)
     *
     * Corresponds to Ada's Set_iBeacon_Switch procedure (hm11.adb:1644-1652)
     */
    void set_ibeacon_switch(bool switch_on, hal::UartStatus& status) noexcept;

    /**
     * @brief Get iBeacon switch status
     *
     * Sends "AT+IBEA?" command to query current iBeacon mode status.
     * Response format: "OK+Get:{0|1}"
     *
     * @param switch_on Output parameter for iBeacon mode status
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if status retrieved successfully
     * @post switch_on contains true if iBeacon mode enabled, false if disabled
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     *
     * Corresponds to Ada's Get_iBeacon_Switch procedure (hm11.adb:1658-1675)
     */
    void get_ibeacon_switch(bool& switch_on, hal::UartStatus& status) noexcept;

    /**
     * @brief Set iBeacon deployment mode
     *
     * Sends "AT+DELO{1|2}" command to configure iBeacon operating mode.
     * Response format: "OK+DELO{1|2}"
     *
     * @param mode Deployment mode (Broadcast_Scanning or Only_Broadcast)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if mode set successfully
     * @post Module configured for specified deployment mode
     * @post Module resets after 500ms
     * @post Module enters non-connectable status until next power-on
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Broadcast_Scanning (1): Broadcast iBeacon and scan for others
     * @note Only_Broadcast (2): Only broadcast iBeacon (no scanning)
     * @note Module will reset 500ms after receiving OK response
     * @note This command causes module to enter non-connectable state
     *
     * Corresponds to Ada's Set_iBeacon_Deploy_Mode procedure (hm11.adb:1162-1171)
     */
    void set_ibeacon_deploy_mode(types::IBeaconDeployMode mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Set iBeacon UUID (128-bit proximity UUID)
     *
     * Sends four "AT+IBE{0-3}0x{8-char hex}" commands to configure the full
     * 32-character (128-bit) iBeacon proximity UUID. The UUID is sent in four
     * 8-character chunks due to HM-11 command length limitations.
     *
     * Response format (per chunk): "OK+Set:{0-3}0x{8-char hex}"
     *
     * @param uuid 32-character hexadecimal UUID string
     * @param status Output parameter for operation result
     *
     * @pre uuid.size() == 32 (enforced by FixedString type)
     * @pre All characters in [0-9A-F] (enforced by FixedString type)
     *
     * @post status == UartStatus::Ok if all chunks transmitted successfully
     * @post status reflects first error if any chunk fails
     * @post iBeacon proximity UUID configured to specified value
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Added in firmware version V520+
     * @note Default UUID: 74278BDA-B644-4520-8F0C-720EAF059935
     * @note All four AT+IBE commands must succeed for complete configuration
     *
     * Corresponds to Ada's Set_iBeacon_UUID procedure (hm11.adb:1681-1704)
     */
    void set_ibeacon_uuid(const types::IBeaconUuid& uuid, hal::UartStatus& status) noexcept;

    /**
     * @brief Get iBeacon UUID
     *
     * Sends four "AT+IBE{0-3}?" commands to retrieve the 32-character iBeacon
     * proximity UUID in four 8-character chunks.
     *
     * Response format (per chunk): "OK+Get:0x{8-char hex}"
     *
     * @param uuid Output parameter for 32-character UUID string
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if all chunks retrieved successfully
     * @post status reflects first error if any chunk fails
     * @post uuid contains 32-character hexadecimal UUID
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Response includes "0x" prefix for each 8-character chunk
     *
     * Corresponds to Ada's Get_iBeacon_UUID procedure (hm11.adb:1710-1748)
     */
    void get_ibeacon_uuid(types::IBeaconUuid& uuid, hal::UartStatus& status) noexcept;

    /**
     * @brief Set iBeacon major version number
     *
     * Sends "AT+MARJ0x{4-char hex}" command to configure iBeacon major version.
     * Response format: "OK+Set:0x{4-char hex}"
     *
     * @param version 4-character hexadecimal major version (0x0000-0xFFFF)
     * @param status Output parameter for operation result
     *
     * @pre version.size() == 4 (enforced by FixedString type)
     * @pre All characters in [0-9A-F] (enforced by FixedString type)
     *
     * @post status == UartStatus::Ok if major version set successfully
     * @post iBeacon major version configured to specified value
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Added in firmware version V517+
     * @note Default: 0xFFE0
     * @note Major version used for coarse proximity grouping
     *
     * Corresponds to Ada's Set_iBeacon_Marjor_Version procedure (hm11.adb:1754-1764)
     */
    void set_ibeacon_major_version(const types::VersionType& version, hal::UartStatus& status) noexcept;

    /**
     * @brief Get iBeacon major version number
     *
     * Sends "AT+MARJ?" command to retrieve iBeacon major version.
     * Response format: "OK+Get:0x{4-char hex}"
     *
     * @param version Output parameter for 4-character hex major version
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if major version retrieved successfully
     * @post version contains 4-character hexadecimal major version
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Response includes "0x" prefix
     *
     * Corresponds to Ada's Get_iBeacon_Marjor_Version procedure (hm11.adb:1770-1788)
     */
    void get_ibeacon_major_version(types::VersionType& version, hal::UartStatus& status) noexcept;

    /**
     * @brief Set iBeacon minor version number
     *
     * Sends "AT+MINO0x{4-char hex}" command to configure iBeacon minor version.
     * Response format: "OK+Set:0x{4-char hex}"
     *
     * @param version 4-character hexadecimal minor version (0x0000-0xFFFF)
     * @param status Output parameter for operation result
     *
     * @pre version.size() == 4 (enforced by FixedString type)
     * @pre All characters in [0-9A-F] (enforced by FixedString type)
     *
     * @post status == UartStatus::Ok if minor version set successfully
     * @post iBeacon minor version configured to specified value
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Added in firmware version V517+
     * @note Default: 0xFFE1
     * @note Minor version used for fine proximity grouping
     *
     * Corresponds to Ada's Set_iBeacon_Minor_Version procedure (hm11.adb:1794-1804)
     */
    void set_ibeacon_minor_version(const types::VersionType& version, hal::UartStatus& status) noexcept;

    /**
     * @brief Get iBeacon minor version number
     *
     * Sends "AT+MINO?" command to retrieve iBeacon minor version.
     * Response format: "OK+Get:0x{4-char hex}"
     *
     * @param version Output parameter for 4-character hex minor version
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if minor version retrieved successfully
     * @post version contains 4-character hexadecimal minor version
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Response includes "0x" prefix
     *
     * Corresponds to Ada's Get_iBeacon_Minor_Version procedure (hm11.adb:1810-1828)
     */
    void get_ibeacon_minor_version(types::VersionType& version, hal::UartStatus& status) noexcept;

    /**
     * @brief Set iBeacon measured power (RSSI calibration value)
     *
     * Sends "AT+MEAS0x{2-char hex}" command to configure iBeacon measured power.
     * Response format: "OK+Set:0x{2-char hex}"
     *
     * @param power 2-character hexadecimal measured power (0x00-0xFF)
     * @param status Output parameter for operation result
     *
     * @pre power.size() == 2 (enforced by FixedString type)
     * @pre All characters in [0-9A-F] (enforced by FixedString type)
     *
     * @post status == UartStatus::Ok if measured power set successfully
     * @post iBeacon measured power configured to specified value
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Added in firmware version V519+
     * @note Default: 0xC5 (representing -59 dBm in two's complement)
     * @note Measured power is RSSI at 1 meter distance for distance estimation
     *
     * Corresponds to Ada's Set_iBeacon_Measured_Power procedure (hm11.adb:1834-1844)
     */
    void set_ibeacon_measured_power(const types::MeasuredPower& power, hal::UartStatus& status) noexcept;

    /**
     * @brief Get iBeacon measured power
     *
     * Sends "AT+MEAS?" command to retrieve iBeacon measured power.
     * Response format: "OK+Get:0x{2-char hex}"
     *
     * @param power Output parameter for 2-character hex measured power
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if measured power retrieved successfully
     * @post power contains 2-character hexadecimal measured power value
     *
     * @note iBeacon-specific feature (HM-11 modules only)
     * @note Response includes "0x" prefix
     *
     * Corresponds to Ada's Get_iBeacon_Measured_Power procedure (hm11.adb:1850-1868)
     */
    void get_ibeacon_measured_power(types::MeasuredPower& power, hal::UartStatus& status) noexcept;

    // ========================================================================
    // UART Configuration Operations
    // ========================================================================

    /**
     * @brief Set UART baud rate
     *
     * Sends "AT+BAUD{0-8}" command to configure UART communication baud rate.
     * Response format: "OK+Set:{0-8}"
     *
     * @param value Desired UART baud rate (Br9600 through Br230400)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if baud rate set successfully
     * @post Module UART baud rate configured to specified value
     *
     * @warning If set to Br1200 (value 7), module will NOT support AT commands
     *          after next power cycle. This is a permanent configuration change
     *          that effectively disables AT command mode.
     *
     * @note Default: Br9600 (9600 baud)
     * @note Baud rate changes take effect after power cycle
     * @note Host MCU UART must be reconfigured to match new baud rate
     * @note Enum values map by declaration order, not numeric value:
     *       Br9600=0, Br19200=1, Br38400=2, Br57600=3, Br115200=4,
     *       Br4800=5, Br2400=6, Br1200=7, Br230400=8
     *
     * Corresponds to Ada's Set_UART_Baud_Rate procedure (hm11.adb:557-565)
     */
    void set_uart_baud_rate(types::UartBaudRate value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current UART baud rate
     *
     * Sends "AT+BAUD?" command to query current UART baud rate setting.
     * Response format: "OK+Get:{0-8}"
     *
     * @param result Output parameter for UART baud rate
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if baud rate retrieved successfully
     * @post result contains current UART baud rate enum value
     *
     * @note Response character maps to baud rate by declaration order
     *
     * Corresponds to Ada's Get_UART_Baud_Rate procedure (hm11.adb:571-584)
     */
    void get_uart_baud_rate(types::UartBaudRate& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set UART flow control switch
     *
     * Sends "AT+FIOW{0|1}" command to enable or disable UART hardware flow control.
     * Response format: "OK+Set:{0|1}"
     *
     * @param enable True to enable flow control, false to disable
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if flow control switch set successfully
     * @post UART flow control enabled or disabled based on enable parameter
     *
     * @note Default: false (flow control disabled)
     * @note When enabled, uses RTS/CTS hardware flow control
     * @note Host MCU must configure RTS/CTS pins when enabling flow control
     *
     * Corresponds to Ada's Set_UART_Flow_Control_Switch procedure (hm11.adb:1527-1535)
     */
    void set_flow_control_switch(bool enable, hal::UartStatus& status) noexcept;

    /**
     * @brief Get UART flow control switch status
     *
     * Sends "AT+FIOW?" command to query current UART flow control setting.
     * Response format: "OK+Get:{0|1}"
     *
     * @param enabled Output parameter for flow control status
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if flow control status retrieved successfully
     * @post enabled contains true if flow control enabled, false if disabled
     *
     * Corresponds to Ada's Get_UART_Flow_Control_Switch procedure (hm11.adb:1541-1558)
     */
    void get_flow_control_switch(bool& enabled, hal::UartStatus& status) noexcept;

    /**
     * @brief Set UART parity bit configuration
     *
     * Sends "AT+PARI{0-2}" command to configure UART parity checking.
     * Response format: "OK+Set:{0-2}"
     *
     * @param parity Desired parity bit setting (None, Odd, or Even)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if parity bit set successfully
     * @post UART parity configured to specified value
     *
     * @note Default: None (no parity bit)
     * @note Host MCU UART must be reconfigured to match parity setting
     * @note Mapping: None=0, Odd=1, Even=2
     *
     * Corresponds to Ada's Set_Parity_Bit procedure (hm11.adb:2045-2053)
     */
    void set_parity_bit(types::ParityBit parity, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current UART parity bit configuration
     *
     * Sends "AT+PARI?" command to query current UART parity setting.
     * Response format: "OK+Get:{0-2}"
     *
     * @param parity Output parameter for parity bit setting
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if parity bit retrieved successfully
     * @post parity contains current parity bit enum value
     *
     * Corresponds to Ada's Get_Parity_Bit procedure (hm11.adb:2059-2071)
     */
    void get_parity_bit(types::ParityBit& parity, hal::UartStatus& status) noexcept;

    /**
     * @brief Set UART stop bit configuration
     *
     * Sends "AT+STOP{0-1}" command to configure number of UART stop bits.
     * Response format: "OK+Set:{0-1}"
     *
     * @param stop_bit Desired stop bit setting (OneStopBit or TwoStopBit)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if stop bit set successfully
     * @post UART stop bits configured to specified value
     *
     * @note Default: OneStopBit (1 stop bit - standard)
     * @note Host MCU UART must be reconfigured to match stop bit setting
     * @note Mapping: OneStopBit=0, TwoStopBit=1
     *
     * Corresponds to Ada's Set_Stop_Bit procedure (hm11.adb:288-296)
     */
    void set_stop_bit(types::StopBit stop_bit, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current UART stop bit configuration
     *
     * Sends "AT+STOP?" command to query current UART stop bit setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param stop_bit Output parameter for stop bit setting
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if stop bit retrieved successfully
     * @post stop_bit contains current stop bit enum value
     *
     * Corresponds to Ada's Get_Stop_Bit procedure (hm11.adb:302-314)
     */
    void get_stop_bit(types::StopBit& stop_bit, hal::UartStatus& status) noexcept;

    // ========================================================================
    // Security and Authentication Operations
    // ========================================================================

    /**
     * @brief Set bond mode for BLE pairing
     *
     * Sends "AT+TYPE{0-3}" command to configure Bluetooth bonding/pairing mode.
     * Response format: "OK+Set:{0-3}"
     *
     * Bond mode determines whether devices must pair before connecting and
     * what authentication is required.
     *
     * @param mode Desired bond mode (NotNeedPin, AuthNotNeedPin, AuthWithPin, AuthAndBonded)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if bond mode set successfully
     * @post Module bond mode configured to specified value
     *
     * @note Default: NotNeedPin (no PIN required)
     * @note AuthWithPin requires Set_PIN_Code to configure PIN
     * @note AuthAndBonded stores pairing information across power cycles
     * @note Mapping: NotNeedPin=0, AuthNotNeedPin=1, AuthWithPin=2, AuthAndBonded=3
     *
     * Corresponds to Ada's Set_Bond_Mode procedure (hm11.adb:2593-2601)
     */
    void set_bond_mode(types::BondMode mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current bond mode
     *
     * Sends "AT+TYPE?" command to query current bond mode setting.
     * Response format: "OK+Get:{0-3}"
     *
     * @param mode Output parameter for bond mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if bond mode retrieved successfully
     * @post mode contains current bond mode enum value
     *
     * Corresponds to Ada's Get_Bond_Mode procedure (hm11.adb:2607-2620)
     */
    void get_bond_mode(types::BondMode& mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Set PIN code for authentication
     *
     * Sends "AT+PASS{6-digit PIN}" command to configure pairing PIN code.
     * Response format: "OK+Set:{6-digit PIN}"
     *
     * PIN code is used when bond mode is AuthWithPin or AuthAndBonded.
     * Must be exactly 6 decimal digits (enforced by PinType).
     *
     * @param pin 6-digit PIN code (e.g., "123456", "000000")
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIN code set successfully
     * @post Module PIN code configured to specified value
     *
     * @note Default: "000000"
     * @note PIN must be 6 decimal digits (enforced by FixedString<6, DigitCharValidator>)
     * @note PIN is used only when bond mode requires authentication
     *
     * Corresponds to Ada's Set_PIN_Code procedure (hm11.adb:2171-2181)
     */
    void set_pin_code(const types::PinType& pin, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current PIN code
     *
     * Sends "AT+PASS?" command to query current PIN code setting.
     * Response format: "OK+Get:{6-digit PIN}"
     *
     * @param pin Output parameter for PIN code
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIN code retrieved successfully
     * @post pin contains current 6-digit PIN code
     *
     * Corresponds to Ada's Get_PIN_Code procedure (hm11.adb:2187-2204)
     */
    void get_pin_code(types::PinType& pin, hal::UartStatus& status) noexcept;

    /**
     * @brief Set work type for module operation
     *
     * Sends "AT+IMME{0-1}" command to configure when module starts operation.
     * Response format: "OK+Set:{0-1}"
     *
     * Work type determines whether the module starts immediately on power-up
     * or waits for AT commands.
     *
     * @param work Desired work type (StartImmediately or RespondAtCommand)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work type set successfully
     * @post Module work type configured to specified value
     *
     * @note Default: StartImmediately (module starts on power-up)
     * @note RespondAtCommand (1) is CRITICAL for Central role scanner applications
     *       - Central mode requires RespondAtCommand to accept AT commands
     *       - Without this, module auto-starts and cannot be controlled
     * @note Mapping: StartImmediately=0, RespondAtCommand=1
     *
     * Corresponds to Ada's Set_Work_Type procedure (hm11.adb:1612-1620)
     */
    void set_work_type(types::WorkType work, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current work type
     *
     * Sends "AT+IMME?" command to query current work type setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param work Output parameter for work type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work type retrieved successfully
     * @post work contains current work type enum value
     *
     * Corresponds to Ada's Get_Work_Type procedure (hm11.adb:1626-1638)
     */
    void get_work_type(types::WorkType& work, hal::UartStatus& status) noexcept;

    /**
     * @brief Set white list switch enable/disable
     *
     * Sends "AT+ALLO{0|1}" command to enable or disable MAC address white list.
     * Response format: "OK+Set:{0|1}"
     *
     * When enabled, module only accepts connections from MAC addresses in the
     * white list (configured via Set_White_List_MAC_Addresses).
     *
     * @param enable True to enable white list, false to disable
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if white list switch set successfully
     * @post White list filtering enabled or disabled based on enable parameter
     *
     * @note This command added in V523
     * @note White list supports up to 3 MAC addresses (indices 1-3)
     * @note Use Set_White_List_MAC_Addresses to configure white list entries
     * @note Default: false (white list disabled)
     *
     * Corresponds to Ada's Set_White_List_Switch procedure (hm11.adb:470-478)
     */
    void set_white_list_switch(bool enable, hal::UartStatus& status) noexcept;

    /**
     * @brief Get white list switch status
     *
     * Sends "AT+ALLO?" command to query white list enable/disable status.
     * Response format: "OK+Get:{0|1}"
     *
     * @param enabled Output parameter for white list status
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if white list status retrieved successfully
     * @post enabled contains true if white list enabled, false if disabled
     *
     * Corresponds to Ada's Get_White_List_Switch procedure (hm11.adb:450-464)
     */
    void get_white_list_switch(bool& enabled, hal::UartStatus& status) noexcept;

    /**
     * @brief Set MAC address in white list
     *
     * Sends "AT+AD{1-3}{MAC}" command to configure a white list entry.
     * Response format: "OK+Set:{MAC}"
     *
     * HM-11 supports up to 3 MAC addresses in the white list. When white list
     * is enabled, only connections from these addresses are accepted.
     *
     * @param index White list entry index (1-3, enforced by MacWhiteListIndex)
     * @param mac_address 12-character hexadecimal MAC address (e.g., "B4994C123456")
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if white list MAC address set successfully
     * @post White list entry at specified index configured with MAC address
     *
     * @note This command added in V523
     * @note White list capacity: 3 addresses maximum (indices 1-3)
     * @note Index enforced by RangedInteger<uint8_t, 1, 3>
     * @note MAC address format: 12 hexadecimal characters (enforced by FixedString<12, HexCharValidator>)
     * @note White list must be enabled via Set_White_List_Switch to take effect
     *
     * Corresponds to Ada's Set_White_List_MAC_Addresses procedure (hm11.adb:822-834)
     */
    void set_white_list_mac_address(
        types::MacWhiteListIndex index,
        const types::MacAddress& mac_address,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Get MAC address from white list
     *
     * Sends "AT+AD{1-3}??" command to query a white list entry.
     * Response format: "OK+AD{1-3}?:{MAC}"
     *
     * @param index White list entry index (1-3, enforced by MacWhiteListIndex)
     * @param mac_address Output parameter for MAC address
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if white list MAC address retrieved successfully
     * @post mac_address contains 12-character hexadecimal MAC address
     *
     * @note Index enforced by RangedInteger<uint8_t, 1, 3>
     * @note Returns configured MAC address at specified index
     *
     * Corresponds to Ada's Get_White_List_MAC_Address procedure (hm11.adb:840-861)
     */
    void get_white_list_mac_address(
        types::MacWhiteListIndex index,
        types::MacAddress& mac_address,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Remove all bond information
     *
     * Sends "AT+ERASE" command to clear all pairing and bonding information.
     * Response format: "OK+ERASE"
     *
     * This operation clears all stored bonding data, including:
     * - Paired device information
     * - Bonding keys
     * - Security settings
     *
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if bond information removed successfully
     * @post All pairing and bonding data cleared from module
     *
     * @note This operation is permanent and cannot be undone
     * @note Devices will need to re-pair after this command
     * @note Useful for resetting security state or troubleshooting pairing issues
     *
     * Corresponds to Ada's Remove_Bond_Information procedure (hm11.adb:1177-1182)
     */
    void remove_bond_information(hal::UartStatus& status) noexcept;

    /**
     * @brief Set service UUID for GATT advertising
     *
     * Sends "AT+UUID0x{UUID}" command to configure advertised service UUID.
     * Response format: "OK+Set:0x{UUID}"
     *
     * The service UUID is included in advertising packets and identifies the
     * primary GATT service offered by the module.
     *
     * @param service_uuid 4-character hexadecimal UUID (16-bit service UUID)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if service UUID set successfully
     * @post Module advertises specified service UUID
     *
     * @note UUID format: 4 hexadecimal characters (enforced by FixedString<4, HexCharValidator>)
     * @note Example: "180F" (Battery Service UUID)
     * @note HM-11 only supports 16-bit UUIDs, not full 128-bit UUIDs
     * @note Command includes "0x" prefix: "AT+UUID0x180F"
     *
     * Corresponds to Ada's Set_Service_UUID procedure (hm11.adb:2626-2637)
     */
    void set_service_uuid(const types::UUID& service_uuid, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current service UUID
     *
     * Sends "AT+UUID?" command to query current service UUID setting.
     * Response format: "OK+Get:0x{UUID}"
     *
     * @param service_uuid Output parameter for service UUID
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if service UUID retrieved successfully
     * @post service_uuid contains current 4-character hexadecimal UUID
     *
     * @note Response includes "0x" prefix which is parsed and removed
     *
     * Corresponds to Ada's Get_Service_UUID procedure (hm11.adb:2643-2660)
     */
    void get_service_uuid(types::UUID& service_uuid, hal::UartStatus& status) noexcept;

    /**
     * @brief Set UART sleep type
     *
     * Sends "AT+UART{0-1}" command to configure UART behavior in sleep mode.
     * Response format: "OK+Set:{0-1}"
     *
     * UART sleep type determines whether the UART interface can wake the module
     * from sleep or is completely shut down during sleep.
     *
     * @param sleep_type Desired UART sleep type (CanWakeUpThroughUart or ShutdownUart)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if UART sleep type set successfully
     * @post Module UART sleep behavior configured to specified value
     *
     * @note Default: CanWakeUpThroughUart (UART can wake module)
     * @note HMSensor-specific feature
     * @note ShutdownUart saves more power but requires hardware reset to wake
     * @note Mapping: CanWakeUpThroughUart=0, ShutdownUart=1
     *
     * Corresponds to Ada's Set_UART_Sleep_Type procedure (hm11.adb:2666-2674)
     */
    void set_uart_sleep_type(types::UartSleepType sleep_type, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current UART sleep type
     *
     * Sends "AT+UART?" command to query current UART sleep type setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param sleep_type Output parameter for UART sleep type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if UART sleep type retrieved successfully
     * @post sleep_type contains current UART sleep type enum value
     *
     * Corresponds to Ada's Get_UART_Sleep_Type procedure (hm11.adb:2680-2693)
     */
    void get_uart_sleep_type(types::UartSleepType& sleep_type, hal::UartStatus& status) noexcept;

    // ========================================================================
    // Hardware Control Operations
    // ========================================================================

    /**
     * @brief Set individual PIO pin output state
     *
     * Sends "AT+PIO{1-3}{0-1}" command to control individual PIO pin output.
     * Response format varies by pin: "OK+Set:{0-1}" for PIO1, "OK+PIO{2-3}:{0-1}" for PIO2-3.
     *
     * PIO (Programmable Input/Output) pins allow GPIO control for external hardware.
     * Note: HM-11 has PIO0-PIO3, but PIO0 is reserved for system KEY function and
     * PIO1 is reserved for system LED. Only PIO 1-3 are available for this command.
     *
     * @param pio PIO pin number (1-3)
     * @param output Desired output state (Low=0V or High=3.3V)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIO output set successfully
     * @post Specified PIO pin set to requested output state
     *
     * @note Required Work_Mode: TransmissionMode or RemoteControlMode
     * @note PIO0 has system KEY function, PIO1 has system LED function
     * @note Only PIO 1-3 available for user control
     * @note Mapping: Low=0, High=1
     *
     * Corresponds to Ada's Set_PIO_Output_Status procedure (hm11.adb:2077-2092)
     */
    void set_pio_output_status(types::PioNumber pio, types::PioOutput output, hal::UartStatus& status) noexcept;

    /**
     * @brief Get individual PIO pin output state
     *
     * Sends "AT+PIO{1-3}?" command to query individual PIO pin output state.
     * Response format varies by pin: "OK+Get:{0-1}" for PIO1, "OK+PIO{2-3}:{0-1}" for PIO2-3.
     *
     * @param pio PIO pin number (1-3)
     * @param output Output parameter for current output state
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIO output retrieved successfully
     * @post output contains current PioOutput enum value (Low or High)
     *
     * Corresponds to Ada's Get_PIO_Output_Status procedure (hm11.adb:2098-2127)
     */
    void get_pio_output_status(types::PioNumber pio, types::PioOutput& output, hal::UartStatus& status) noexcept;

    /**
     * @brief Set multiple PIO pin output states simultaneously
     *
     * Sends "AT+MPIO{000-3FF}" command to control multiple PIO pins via bitmask.
     * Response format: "OK+Set:{000-3FF}"
     *
     * The 3-hex-character value is a 12-bit bitmask where each bit maps to a PIO pin:
     * - Bit position (left to right) maps to PIO0-PIOB
     * - Bit value: 1 = HIGH, 0 = LOW
     * - Example: "00C" = 0b000000001100 = PIO2 and PIO3 HIGH, others LOW
     * - Maximum: "3FF" = 0b001111111111 = PIO0-PIO9 all HIGH
     *
     * @param pios 3-character hex bitmask (000-3FF)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIO outputs set successfully
     * @post PIO pins set according to bitmask
     *
     * @note Added since V551
     * @note HM-11 only has PIO0-PIO3 (4 pins total)
     * @note PIO0 and PIO1 are system-reserved but can be set via bitmask
     * @note Hex format: "000" to "3FF" (12 bits)
     *
     * Corresponds to Ada's Set_PIOs_Output_Status procedure (hm11.adb:2155-2165)
     */
    void set_pios_output_status(const types::PioNumbers& pios, hal::UartStatus& status) noexcept;

    /**
     * @brief Get multiple PIO pin output states as bitmask
     *
     * Sends "AT+PIO??" command to query all PIO pin output states.
     * Response format: "OK+PIO?{000-3FF}"
     *
     * Returns 3-hex-character bitmask representing all PIO pin states.
     *
     * @param pios Output parameter for 3-character hex bitmask
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if PIO outputs retrieved successfully
     * @post pios contains 3-character hex bitmask of PIO states
     *
     * Corresponds to Ada's Get_PIOs_Output_Status procedure (hm11.adb:2133-2149)
     */
    void get_pios_output_status(types::PioNumbers& pios, hal::UartStatus& status) noexcept;

    /**
     * @brief Set PIO pin states after connection established
     *
     * Sends "AT+AFTC{000-3FF}" command to configure PIO pin states that will be
     * automatically applied when a BLE connection is established.
     * Response format: "OK+Get:{000-3FF}"
     *
     * This allows automatic GPIO control upon connection without software intervention.
     * The 3-hex-character value is a 12-bit bitmask (same format as Set_PIOs_Output_Status).
     *
     * @param value 3-character hex bitmask (000-3FF) for post-connection pin states
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if connect pin output set successfully
     * @post PIO pins will automatically set to specified states upon connection
     *
     * @note Added in V527
     * @note Default: "000" (all pins LOW)
     * @note PIO0 and PIO1 are system-used, so must be 0
     * @note Only PIO2-PIOB pins are available (HM-11: only PIO2-PIO3)
     *
     * Corresponds to Ada's Set_Connect_Pin_Output procedure (hm11.adb:2880-2889)
     */
    void set_connect_pin_output(const types::PioNumbers& value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get PIO pin states configured for after connection
     *
     * Sends "AT+AFTC?" command to query PIO pin states that will be applied on connection.
     * Response format: "OK+Get:{000-3FF}"
     *
     * @param result Output parameter for 3-character hex bitmask
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if connect pin output retrieved successfully
     * @post result contains 3-character hex bitmask of post-connection pin states
     *
     * Corresponds to Ada's Get_Connect_Pin_Output procedure (hm11.adb:2895-2908)
     */
    void get_connect_pin_output(types::PioNumbers& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set PIO pin states before connection (after power-on)
     *
     * Sends "AT+BEFC{000-3FF}" command to configure PIO pin states that will be
     * automatically applied immediately after power is supplied to the module.
     * Response format: "OK+Get:{000-3FF}"
     *
     * This allows automatic GPIO initialization on power-up without software intervention.
     * The 3-hex-character value is a 12-bit bitmask (same format as Set_PIOs_Output_Status).
     *
     * @param value 3-character hex bitmask (000-3FF) for post-power pin states
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if power pin output set successfully
     * @post PIO pins will automatically set to specified states after power-on
     *
     * @note Added in V527
     * @note Default: "000" (all pins LOW)
     * @note PIO0 and PIO1 are system-used, so must be 0
     * @note Only PIO2-PIOB pins are available (HM-11: only PIO2-PIO3)
     *
     * Corresponds to Ada's Set_Power_Pin_Output procedure (hm11.adb:2837-2850)
     */
    void set_power_pin_output(const types::PioNumbers& value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get PIO pin states configured for after power-on
     *
     * Sends "AT+BEFC?" command to query PIO pin states that will be applied on power-up.
     * Response format: "OK+Get:{000-3FF}"
     *
     * @param result Output parameter for 3-character hex bitmask
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if power pin output retrieved successfully
     * @post result contains 3-character hex bitmask of post-power pin states
     *
     * Corresponds to Ada's Get_Power_Pin_Output procedure (hm11.adb:2856-2869)
     */
    void get_power_pin_output(types::PioNumbers& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set PIO collection rate for PIO_Collection_Mode
     *
     * Sends "AT+CYC{00-99}" command to set the interval for PIO state reporting.
     * Response format: "OK+Get:{00-99}"
     *
     * In PIO_Collection_Mode work mode, the module periodically sends "OK+Col:[xx]"
     * messages through UART or to the remote side when PIO state changes. This command
     * sets the minimum interval between these messages.
     *
     * @param value Collection rate in seconds (0-99)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if collection rate set successfully
     * @post PIO collection interval configured to specified seconds
     *
     * @note Added since V515
     * @note Default: 10 seconds
     * @note Required Work Mode: PIO_Collection_Mode
     * @note Format: Two decimal digits (00-99), zero-padded
     *
     * Corresponds to Ada's Set_PIO_Collection_Rate procedure (hm11.adb:2794-2804)
     */
    void set_pio_collection_rate(types::PioCollectionRate value, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current PIO collection rate
     *
     * Sends "AT+CYC??" command to query PIO collection rate setting.
     * Response format: "OK+Get:{0-99}" (1 or 2 decimal digits)
     *
     * @param result Output parameter for collection rate in seconds
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if collection rate retrieved successfully
     * @post result contains current collection rate (0-99 seconds)
     *
     * Corresponds to Ada's Get_PIO_Collection_Rate procedure (hm11.adb:2810-2831)
     */
    void get_pio_collection_rate(types::PioCollectionRate& result, hal::UartStatus& status) noexcept;

    /**
     * @brief Set RF output power level
     *
     * Sends "AT+PCTL{0-1}" command to configure RF transmission power.
     * Response format: "OK+Set:{0-1}"
     *
     * Controls the RF output power to balance range vs. power consumption.
     * NormalPower provides adequate range with lower power draw, while MaxPower
     * provides maximum transmission range at the cost of higher power consumption.
     *
     * @param power Desired output power (NormalPower or MaxPower)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if output power set successfully
     * @post Module RF output power configured to specified level
     *
     * @note Added in V527
     * @note Default: MaxPower
     * @note Mapping: NormalPower=0, MaxPower=1
     *
     * Corresponds to Ada's Set_Output_Power procedure (hm11.adb:2013-2021)
     */
    void set_output_power(types::OutputPower power, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current RF output power level
     *
     * Sends "AT+PCTL?" command to query RF output power setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param power Output parameter for current output power
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if output power retrieved successfully
     * @post power contains current OutputPower enum value
     *
     * Corresponds to Ada's Get_Output_Power procedure (hm11.adb:2027-2039)
     */
    void get_output_power(types::OutputPower& power, hal::UartStatus& status) noexcept;

    /**
     * @brief Set module RF transmission power in dBm
     *
     * Sends "AT+POWE{0-3}" command to configure RF transmission power level.
     * Response format: "OK+Set:{0-3}"
     *
     * Four power levels are available:
     * - DbmMinus23 (0): -23 dBm, lowest power, shortest range
     * - DbmMinus6 (1): -6 dBm, low power, reduced range
     * - Dbm0 (2): 0 dBm, normal power, standard range (default)
     * - Dbm6 (3): +6 dBm, maximum power, maximum range
     *
     * @param power Desired module power in dBm
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if module power set successfully
     * @post Module RF transmission power configured to specified dBm level
     *
     * @note Default: Dbm0 (0 dBm)
     * @note Higher power increases range but also power consumption
     * @note Mapping: DbmMinus23=0, DbmMinus6=1, Dbm0=2, Dbm6=3
     *
     * Corresponds to Ada's Set_Module_Power procedure (hm11.adb:2210-2218)
     */
    void set_module_power(types::ModulePower power, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current module RF transmission power
     *
     * Sends "AT+POWE?" command to query module power setting.
     * Response format: "OK+Get:{0-3}"
     *
     * @param power Output parameter for current module power
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if module power retrieved successfully
     * @post power contains current ModulePower enum value
     *
     * Corresponds to Ada's Get_Module_Power procedure (hm11.adb:2224-2236)
     */
    void get_module_power(types::ModulePower& power, hal::UartStatus& status) noexcept;

    /**
     * @brief Set module RX gain for receiver sensitivity
     *
     * Sends "AT+GAIN{0-1}" command to configure receiver gain.
     * Response format: "OK+Set:{0-1}"
     *
     * RX gain controls the receiver sensitivity. Enabling gain (Open) increases
     * receiver sensitivity and range but also increases power consumption. Disabling
     * gain (No) reduces power consumption at the cost of receiver sensitivity.
     *
     * @param gain Desired RX gain (No=disabled, Open=enabled)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if RX gain set successfully
     * @post Module RX gain configured to specified state
     *
     * @note Mapping: No=0 (disabled, lower power), Open=1 (enabled, higher sensitivity)
     *
     * Corresponds to Ada's Set_Module_RX_Gain procedure (hm11.adb:1564-1572)
     */
    void set_module_rx_gain(types::RxGain gain, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current module RX gain setting
     *
     * Sends "AT+GAIN?" command to query RX gain setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param gain Output parameter for current RX gain
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if RX gain retrieved successfully
     * @post gain contains current RxGain enum value
     *
     * Corresponds to Ada's Get_Module_RX_Gain procedure (hm11.adb:1578-1590)
     */
    void get_module_rx_gain(types::RxGain& gain, hal::UartStatus& status) noexcept;

    /**
     * @brief Set module auto sleep mode (Peripheral role only)
     *
     * Sends "AT+PWRM{0-1}" command to enable/disable automatic sleep mode.
     * Response format: "OK+Set:{0-1}"
     *
     * When auto sleep is enabled (true), the module automatically enters sleep mode
     * after disconnection to save power. When disabled (false), the module remains
     * active after disconnection.
     *
     * Note: The command parameter is inverted - sleep=true sends "0", sleep=false sends "1".
     * This matches the Ada implementation where Set_Module_Auto_Sleep(True) sends "AT+PWRM0".
     *
     * @param sleep Enable (true) or disable (false) auto sleep
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if auto sleep set successfully
     * @post Module auto sleep behavior configured
     *
     * @note Only Peripheral role is used
     * @note Default: False (auto sleep disabled)
     * @note Mapping: sleep=true → "0", sleep=false → "1" (inverted)
     *
     * Corresponds to Ada's Set_Module_Auto_Sleep procedure (hm11.adb:2242-2250)
     */
    void set_module_auto_sleep(bool sleep, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current module auto sleep setting
     *
     * Sends "AT+PWRM?" command to query auto sleep setting.
     * Response format: "OK+Get:{0-1}"
     *
     * The response is inverted: '0' means auto sleep enabled, '1' means disabled.
     *
     * @param sleep Output parameter for auto sleep state
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if auto sleep retrieved successfully
     * @post sleep contains current auto sleep boolean value
     *
     * @note Only Peripheral role is used
     * @note Response mapping: '0' → true (enabled), '1' → false (disabled)
     *
     * Corresponds to Ada's Get_Module_Auto_Sleep procedure (hm11.adb:2256-2273)
     */
    void get_module_auto_sleep(bool& sleep, hal::UartStatus& status) noexcept;

    /**
     * @brief Set connection notification information enable/disable
     *
     * Sends "AT+NOTI{0-1}" command to control connection status notifications.
     * Response format: "OK+Set:{0-1}"
     *
     * When enabled (true), the module sends "OK+CONN" or "OK+LOST" messages through
     * UART when a BLE connection is established or lost. This allows the application
     * to monitor connection status without polling.
     *
     * @param notify Enable (true) or disable (false) connection notifications
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if notify information set successfully
     * @post Module connection notification behavior configured
     *
     * @note Default: True (notifications enabled)
     * @note Mapping: notify=false → "0", notify=true → "1"
     * @note See also: Set_Notify_Mode to include MAC address in notifications
     *
     * Corresponds to Ada's Set_Notify_Information procedure (hm11.adb:1906-1914)
     */
    void set_notify_information(bool notify, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current connection notification information setting
     *
     * Sends "AT+NOTI?" command to query notification setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param notify Output parameter for notification enable state
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if notify information retrieved successfully
     * @post notify contains current notification boolean value
     *
     * @note Response mapping: '0' → false (disabled), '1' → true (enabled)
     *
     * Corresponds to Ada's Get_Notify_Information procedure (hm11.adb:1920-1937)
     */
    void get_notify_information(bool& notify, hal::UartStatus& status) noexcept;

    /**
     * @brief Set notification mode (with or without MAC address)
     *
     * Sends "AT+NOTP{0-1}" command to configure notification format.
     * Response format: "OK+Set:{0-1}"
     *
     * This command controls whether connection notifications include the remote
     * device's MAC address. When WithAddress mode is set, notifications become:
     * - "OK+CONN:001122334455" (includes 12-character MAC address)
     * - "OK+LOST:001122334455"
     *
     * Without address mode uses standard format: "OK+CONN" / "OK+LOST"
     *
     * @param mode Notification mode (WithoutAddress or WithAddress)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if notify mode set successfully
     * @post Module notification format configured
     *
     * @note Added since V534
     * @note Default: WithoutAddress
     * @note Must work with Set_Notify_Information(true) to receive notifications
     * @note Mapping: WithoutAddress=0, WithAddress=1
     *
     * Corresponds to Ada's Set_Notify_Mode procedure (hm11.adb:1943-1951)
     */
    void set_notify_mode(types::NotifyMode mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current notification mode
     *
     * Sends "AT+NOTP?" command to query notification mode setting.
     * Response format: "OK+Get:{0-1}"
     *
     * @param mode Output parameter for current notification mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if notify mode retrieved successfully
     * @post mode contains current NotifyMode enum value
     *
     * Corresponds to Ada's Get_Notify_Mode procedure (hm11.adb:1957-1969)
     */
    void get_notify_mode(types::NotifyMode& mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Set module work mode
     *
     * Sends "AT+MODE{0-2}" command to configure module operating mode.
     * Response format: "OK+Set:{0-2}"
     *
     * Three work modes are available:
     *
     * TransmissionMode (0): Standard mode
     * - Before connection: AT commands available
     * - After connection: Bidirectional data transmission
     *
     * PioCollectionMode (1): PIO collection mode
     * - Before connection: AT commands available
     * - After connection: Remote can send AT commands, collect PIO3 state,
     *   remote control PIO2 pin, and send data (<20 bytes)
     *
     * RemoteControlMode (2): Remote control mode
     * - Before connection: AT commands available
     * - After connection: Remote can send AT commands, remote control PIO2-PIO3 pins,
     *   and send data (<20 bytes)
     *
     * @param mode Desired work mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work mode set successfully
     * @post Module work mode configured to specified value
     *
     * @note Default: TransmissionMode
     * @note Mode affects data transmission behavior after connection
     * @note PIO operations require appropriate work mode
     * @note Mapping: TransmissionMode=0, PioCollectionMode=1, RemoteControlMode=2
     *
     * Corresponds to Ada's Set_Work_Mode procedure (hm11.adb:1874-1882)
     */
    void set_work_mode(types::WorkMode mode, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current module work mode
     *
     * Sends "AT+MODE?" command to query work mode setting.
     * Response format: "OK+Get:{0-2}"
     *
     * @param mode Output parameter for current work mode
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work mode retrieved successfully
     * @post mode contains current WorkMode enum value
     *
     * Corresponds to Ada's Get_Work_Mode procedure (hm11.adb:1888-1900)
     */
    void get_work_mode(types::WorkMode& mode, hal::UartStatus& status) noexcept;

    // ========================================================================
    // HMSensor-Specific Operations (Decision 15)
    // ========================================================================

    /**
     * @brief Set sensor type for HMSensor module
     *
     * Sends "AT+SENS{0-2}" command to configure the type of sensor connected
     * to the HMSensor variant module's PIO11 pin (PIO3 on HM-11).
     * Response format: "OK+Set:{0-2}"
     *
     * Sensor types:
     * - None (0): No sensor connected
     * - DHT11 (1): DHT11 temperature and humidity sensor
     * - DS18B20 (2): DS18B20 temperature-only sensor
     *
     * @param sensor_type Desired sensor type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if sensor type set successfully
     * @post Module sensor type configured to specified value
     *
     * @note HMSensor-only: This command only works with HMSensor variant modules
     * @note Hardware will return error if called on non-HMSensor modules
     * @note Default: None (no sensor)
     * @note Mapping: None=0, DHT11=1, DS18B20=2
     *
     * Corresponds to Ada's Set_Sensor_Type procedure (hm11.adb:2419-2427)
     */
    void set_sensor_type(types::SensorType sensor_type, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current sensor type from HMSensor module
     *
     * Sends "AT+SENS?" command to query the currently configured sensor type.
     * Response format: "OK+Get:{0-2}"
     *
     * @param sensor_type Output parameter for sensor type
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if sensor type retrieved successfully
     * @post sensor_type contains current SensorType enum value
     *
     * @note HMSensor-only: This command only works with HMSensor variant modules
     * @note Hardware will return error if called on non-HMSensor modules
     *
     * Corresponds to Ada's Get_Sensor_Type procedure (hm11.adb:2433-2446)
     */
    void get_sensor_type(types::SensorType& sensor_type, hal::UartStatus& status) noexcept;

    /**
     * @brief Get temperature and humidity readings from HMSensor module
     *
     * Sends "AT+TEHU?" command to read current sensor values from DHT11 sensor.
     * Response format: "OK+Get:TTT:HHH" where TTT is temperature (0-120) and
     * HHH is humidity (0-100).
     *
     * Temperature and humidity values are parsed from the response string into
     * numeric uint8_t values. If hardware returns decimal values (e.g., "25.5"),
     * they are truncated to integers (Decision 29).
     *
     * @param temperature Output parameter for temperature reading (0-120 Celsius)
     * @param humidity Output parameter for humidity reading (0-100 percent)
     * @param status Output parameter for operation result
     *
     * @pre Sensor type must be DHT11 (Decision 8: document preconditions)
     * @post status == UartStatus::Ok if readings retrieved successfully
     * @post temperature contains parsed temperature value (0-120)
     * @post humidity contains parsed humidity value (0-100)
     *
     * @note HMSensor-only: This command only works with HMSensor variant modules
     * @note Hardware will return error if called on non-HMSensor modules or no sensor
     * @note Values are added to scan response data package
     * @note Data format in scan response: 0x02, 0x16, 0x00, 0xB0, [reserved],
     *       [temperature], [humidity], [battery]
     *
     * Corresponds to Ada's Get_Sensor_Temperature_And_Humidity procedure (hm11.adb:2485-2506)
     */
    void get_sensor_temperature_and_humidity(uint8_t& temperature, uint8_t& humidity, hal::UartStatus& status) noexcept;

    /**
     * @brief Set sensor work interval for HMSensor module
     *
     * Sends "AT+RAT{interval}" command to configure how often the sensor takes
     * readings in minutes.
     * Response format: "OK+Set:{interval}"
     *
     * @param interval Work interval in minutes (2-digit string "00" to "99")
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work interval set successfully
     * @post Module sensor work interval configured to specified value
     *
     * @note HMSensor-only: This command only works with HMSensor variant modules
     * @note Hardware will return error if called on non-HMSensor modules
     * @note Default: "00" (0 minutes)
     * @note Unit: minutes
     *
     * Corresponds to Ada's Set_Sensor_Work_Interval procedure (hm11.adb:261-282)
     */
    void set_sensor_work_interval(const types::WorkInterval& interval, hal::UartStatus& status) noexcept;

    /**
     * @brief Get current sensor work interval from HMSensor module
     *
     * Sends "AT+RAT?" command to query the current sensor work interval setting.
     * Response format: "OK+Get:I" (single digit) or "OK+Get:II" (two digits)
     *
     * If the response is a single digit (e.g., "5"), it is padded to two digits
     * (e.g., "05") to match the WorkInterval format requirement.
     *
     * @param interval Output parameter for work interval (2-digit string)
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if work interval retrieved successfully
     * @post interval contains current work interval as 2-character string
     *
     * @note HMSensor-only: This command only works with HMSensor variant modules
     * @note Hardware will return error if called on non-HMSensor modules
     * @note Single-digit responses are left-padded with "0" to maintain format
     *
     * Corresponds to Ada's Get_Sensor_Work_Interval procedure (hm11.adb:226-255)
     */
    void get_sensor_work_interval(types::WorkInterval& interval, hal::UartStatus& status) noexcept;

  private:
    // ========================================================================
    // Internal Helper Methods
    // ========================================================================

    /**
     * @brief Transmit command string over UART
     *
     * Sends AT command string to module via UART transmit.
     * Does not wait for or validate response.
     *
     * @param command Command string to transmit
     * @param status Output parameter for UART transmit status
     *
     * Corresponds to Ada's Transmit procedure (hm11.adb:71-81)
     */
    void transmit(std::string_view command, hal::UartStatus& status) noexcept;

    /**
     * @brief Check if response starts with expected prefix
     *
     * Validates that received data starts with expected prefix string.
     * Updates status to ErrError if mismatch detected.
     *
     * @param received Buffer containing received data
     * @param expect Expected prefix string
     * @param status Input/output parameter for status (checked and updated)
     *
     * @pre status == UartStatus::Ok (checked before validation)
     * @post status == UartStatus::Ok if prefix matches
     * @post status == UartStatus::ErrError if prefix mismatch
     *
     * Corresponds to Ada's Check_Responce procedure (hm11.adb:104-125)
     */
    void check_response(hal::UartData8b received, std::string_view expect, hal::UartStatus& status) noexcept;

    /**
     * @brief Transmit command and check response prefix
     *
     * Combined operation: transmit command, receive response, validate prefix.
     * This is the most common operation pattern for AT commands.
     *
     * @param command Command string to transmit
     * @param expect Expected response prefix
     * @param received Buffer for received data
     * @param status Output parameter for operation result
     *
     * @post status == UartStatus::Ok if command sent and response matches
     * @post status reflects first error encountered (transmit, receive, or validation)
     *
     * Corresponds to Ada's Transmit_And_Check procedure (hm11.adb:131-149)
     */
    void transmit_and_check(
        std::string_view command,
        std::string_view expect,
        hal::UartData8b received,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Transmit command and check response (using internal buffer)
     *
     * Convenience overload that uses internal response buffer.
     * Determines buffer size based on expected response length.
     *
     * @param command Command string to transmit
     * @param expect Expected response prefix
     * @param status Output parameter for operation result
     *
     * Corresponds to Ada's Transmit_And_Check overload (hm11.adb:155-163)
     */
    void transmit_and_check(std::string_view command, std::string_view expect, hal::UartStatus& status) noexcept;

    /**
     * @brief Common implementation for characteristic UUID parsing
     *
     * Internal helper used by both find_all_characteristic_uuid and find_characteristic_uuid.
     * Receives and parses streaming characteristic discovery responses.
     *
     * @param callback Function pointer invoked for each discovered characteristic
     * @param timeout Maximum time to wait for streaming response
     * @param status Output parameter for operation result
     *
     * Corresponds to Ada's Read_Characteristic_UUID procedure (hm11.adb:1311-1380)
     */
    template <typename Rep, typename Period>
    void read_characteristic_uuid_impl(
        CharacteristicUuidCallback callback,
        std::chrono::duration<Rep, Period> timeout,
        hal::UartStatus& status) noexcept;

    /**
     * @brief Parse notify response for characteristic access operations
     *
     * Internal helper used by enable_characteristic_notify, disable_characteristic_notify,
     * and read_characteristic_notify. Receives and parses the notify response.
     * Response format: "OK+SEND-OK", "OK+SEND-ER", or "OK+DATA-ER" (10 bytes)
     *
     * @param status Input/output parameter for UART status
     * @param response Output parameter for parsed notify response
     *
     * @post status == UartStatus::Ok if response received and parsed successfully
     * @post response contains parsed NotifyResponse value
     *
     * Corresponds to Ada's Read_Characteristic_Notify_Responce procedure (hm11.adb:1429-1457)
     */
    void read_characteristic_notify_response(hal::UartStatus& status, types::NotifyResponse& response) noexcept;

    // ========================================================================
    // Internal State
    // ========================================================================
    // Member ordering optimized to minimize padding (response_buffer_ first for alignment)
    // NOLINTBEGIN(clang-analyzer-optin.performance.Padding)

    /// Response buffer for AT command responses (DMA-aligned, Decision 20)
    /// Size: MAX_MESSAGE_LENGTH * 5 to handle discovery streaming data
    /// Alignment: 32 bytes for cache coherency on Cortex-M7
    alignas(
        32) std::array<hal::UInt8, MAX_MESSAGE_LENGTH * 5> response_buffer_{}; // NOLINT(readability-identifier-naming)

    /// UART port reference (non-owning, must outlive driver)
    UARTImpl& uart_port_; // NOLINT(readability-identifier-naming)

    /// Receive handler reference (non-owning, must outlive driver)
    ReceiveHandler& receive_handler_; // NOLINT(readability-identifier-naming)

    /// Temporary storage for streaming discovery data (Decision 27)
    /// Accumulates device information during Scan operation
    /// Reset after each complete device is parsed and callback invoked
    DiscoveredInfo discovered_info_{};

    // NOLINTEND(clang-analyzer-optin.performance.Padding)
};

// ============================================================================
// Template Method Implementations
// ============================================================================

// Note: Template methods must be defined in header or explicitly instantiated

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::transmit(std::string_view command, hal::UartStatus& status) noexcept {
    // Convert string_view to UartData8bConst for UART transmission
    // reinterpret_cast is safe here because UInt8 is uint8_t and char is byte-compatible
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,readability-identifier-naming)
    const hal::UartData8bConst DATA_SPAN(reinterpret_cast<const hal::UInt8*>(command.data()), command.size());

    uart_port_.transmit(DATA_SPAN, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::check_response(
    hal::UartData8b received,
    std::string_view expect,
    hal::UartStatus& status) noexcept {
    if (status != hal::UartStatus::Ok) {
        return; // Already failed, don't check
    }

    // Verify buffer is large enough for expected prefix
    if (received.size() < expect.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    // Compare received prefix with expected byte-by-byte
    for (std::size_t i = 0; i < expect.size(); ++i) {
        if (received[i] != static_cast<hal::UInt8>(expect[i])) {
            status = hal::UartStatus::ErrError;
            return;
        }
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::transmit_and_check(
    std::string_view command,
    std::string_view expect,
    hal::UartData8b received,
    hal::UartStatus& status) noexcept {
    // Step 1: Transmit command
    transmit(command, status);
    if (status != hal::UartStatus::Ok) {
        return; // Transmit failed
    }

    // Step 2: Receive response (using receive handler with default timeout)
    receive_handler_(uart_port_, received, status, false);
    if (status != hal::UartStatus::Ok) {
        return; // Receive failed
    }

    // Step 3: Check response prefix
    check_response(received, expect, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::transmit_and_check(
    std::string_view command,
    std::string_view expect,
    hal::UartStatus& status) noexcept {
    // Use internal buffer, sized for expected response
    const std::size_t BUFFER_SIZE = expect.size(); // NOLINT(readability-identifier-naming)
    hal::UartData8b buffer(response_buffer_.data(), BUFFER_SIZE);
    transmit_and_check(command, expect, buffer, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::test(hal::UartStatus& status) noexcept {
    transmit_and_check("AT", "OK", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::reset(hal::UartStatus& status) noexcept {
    transmit_and_check("AT+RESET", "OK+RESET", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::restart(hal::UartStatus& status) noexcept {
    transmit_and_check("AT+START", "OK+START", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::sleep(hal::UartStatus& status) noexcept {
    transmit_and_check("AT+SLEEP", "OK+SLEEP", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::wake_up(hal::UartStatus& status) noexcept {
    // Special wake sequence: exactly 7 'W' characters
    transmit_and_check("WWWWWWW", "OK+WAKE", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::software_version(
    std::string_view& version,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+Get:";

    // Transmit query command
    transmit("AT+VERS?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive response (variable length, use larger buffer)
    constexpr std::size_t MAX_VERSION_LENGTH = 32; // "OK+Get:" + version string
    hal::UartData8b buffer(response_buffer_.data(), MAX_VERSION_LENGTH);
    receive_handler_(uart_port_, buffer, status, false);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Check prefix
    check_response(buffer, EXPECT_PREFIX, status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse version string (everything after "OK+Get:" prefix)
    // Find null terminator or end of buffer
    std::string_view full_response = util::to_string_view(buffer);

    // Skip prefix
    if (full_response.size() <= EXPECT_PREFIX.size()) {
        status = hal::UartStatus::ErrError;
        return;
    }

    std::string_view version_part = full_response.substr(EXPECT_PREFIX.size());

    // Find CRLF or null terminator (CRLF takes precedence)
    std::size_t end_pos = version_part.find('\r');
    if (end_pos == std::string_view::npos) {
        end_pos = version_part.find('\0');
    }
    if (end_pos == std::string_view::npos) {
        // No terminator found, use entire remaining buffer
        version = version_part;
    } else {
        version = version_part.substr(0, end_pos);
    }
}

// ============================================================================
// Role and Connection Management Operations
// ============================================================================

// Helper functions for connection operations (constexpr implies inline)

/// @brief Convert character to digit (0-9)
/// @param c Character in range '0'-'9'
/// @return Numeric value 0-9
constexpr uint8_t char_to_digit(char c) noexcept {
    return static_cast<uint8_t>(c - '0');
}

/// @brief Convert character to ConnectResult enum (Decision 5)
/// @param c Character from connect response
/// @return ConnectResult enum value
/// Corresponds to Ada's To_Connect_Result function (hm11.adb:3036-3047)
constexpr types::ConnectResult to_connect_result(char c) noexcept {
    using types::ConnectResult;
    switch (c) {
        case 'L':
            return ConnectResult::Connecting;
        case 'E':
            return ConnectResult::ConnectError;
        case 'F':
            return ConnectResult::ConnectFail;
        case 'N':
            return ConnectResult::NoAddress;
        case ' ':
        case '\0':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
            return ConnectResult::Connected;
        default:
            return ConnectResult::OtherError;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_role(types::Role& result, hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+ROLE?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto ROLE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto ROLE_VALUE = char_to_digit(ROLE_CHAR);

        if (ROLE_VALUE == 0) {
            result = types::Role::Peripheral;
        } else if (ROLE_VALUE == 1) {
            result = types::Role::Central;
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_role(types::Role value, hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+ROLE").append(value);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(value);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::connect(
    types::MacAddressType mac_type,
    const types::MacAddress& address,
    types::ConnectResult& result,
    hal::UartStatus& status) noexcept {
    result = types::ConnectResult::OtherError;

    util::CommandBuilder cmd;
    cmd.append("AT+CO").append(util::enum_to_string(mac_type)).append(address);

    const auto TYPE_STR = util::enum_to_string(mac_type);
    util::CommandBuilder expected;
    expected.append("OK+CO").append(TYPE_STR).append(TYPE_STR);

    const std::size_t RESPONSE_SIZE = expected.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check(cmd.get(), expected.get(), buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto RESULT_CHAR = static_cast<char>(buffer[expected.size()]);
        result = to_connect_result(RESULT_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::connect(
    types::DiscoveredIndex index,
    types::ConnectResult& result,
    hal::UartStatus& status) noexcept {
    result = types::ConnectResult::OtherError;

    util::CommandBuilder cmd;
    cmd.append("AT+CONN").append(index);

    constexpr std::string_view WHEN_CONNECTED = "OK+CONN";
    constexpr std::size_t RESPONSE_SIZE = WHEN_CONNECTED.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check(cmd.get(), WHEN_CONNECTED, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto RESULT_CHAR = static_cast<char>(buffer[WHEN_CONNECTED.size()]);
        result = to_connect_result(RESULT_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::connect_last_device(
    types::ConnectResult& result,
    hal::UartStatus& status) noexcept {
    result = types::ConnectResult::OtherError;

    constexpr std::string_view WHEN_CONNECTED = "OK+CONN";
    constexpr std::size_t RESPONSE_SIZE = WHEN_CONNECTED.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+CONNL", WHEN_CONNECTED, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto RESULT_CHAR = static_cast<char>(buffer[WHEN_CONNECTED.size()]);
        result = to_connect_result(RESULT_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::disconnect(hal::UartStatus& status) noexcept {
    transmit_and_check("AT", "OK+LOST", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_last_connected_device_address(
    types::MacAddress& mac,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view EXPECT_PREFIX = "OK+RADD:";
    constexpr std::size_t MAC_SIZE = 12;
    constexpr std::size_t RESPONSE_SIZE = EXPECT_PREFIX.size() + MAC_SIZE;

    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+RADD?", EXPECT_PREFIX, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto* mac_start = buffer.data() + EXPECT_PREFIX.size();
        std::string_view mac_view = util::to_string_view(std::span(mac_start, MAC_SIZE));
        mac = types::MacAddress{mac_view};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::clear_last_connected_address(hal::UartStatus& status) noexcept {
    transmit_and_check("AT+CLEAR", "OK+CLEAR", status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_save_connected_mac(bool save, hal::UartStatus& status) noexcept {
    const char VALUE_CHAR = save ? '0' : '1'; // Ada uses inverted logic

    util::CommandBuilder cmd;
    cmd.append("AT+SAVE").append(VALUE_CHAR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(VALUE_CHAR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_save_connected_mac(bool& save, hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+SAVE?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto VALUE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        save = (VALUE_CHAR == '0'); // Ada uses inverted logic
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_connect_remote_device_timeout(
    const types::ConnectTimeout& timeout,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+TCON").append(timeout);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(timeout);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_connect_remote_device_timeout(
    types::ConnectTimeout& timeout,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t TIMEOUT_SIZE = 6;
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + TIMEOUT_SIZE;

    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+TCON?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto* timeout_start = buffer.data() + OK_GET.size();
        std::string_view timeout_view = util::to_string_view(std::span(timeout_start, TIMEOUT_SIZE));
        timeout = types::ConnectTimeout{timeout_view};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::start_working(hal::UartStatus& status) noexcept {
    transmit_and_check("AT+START", "OK+START", status);
}

// ============================================================================
// Link Layer Configuration Operations
// ============================================================================

/// @brief Helper function to parse LinkLayerConnectionInterval from character
/// @param c Character in range '0'-'9'
/// @return LinkLayerConnectionInterval enum value
/// Corresponds to Ada's Get_Minimum_Link_Layer_Connection_Interval (hm11.adb:615-616)
constexpr types::LinkLayerConnectionInterval char_to_link_layer_connection_interval(char c) noexcept {
    using types::LinkLayerConnectionInterval;
    switch (c) {
        case '0':
            return LinkLayerConnectionInterval::Ms7;
        case '1':
            return LinkLayerConnectionInterval::Ms10;
        case '2':
            return LinkLayerConnectionInterval::Ms15;
        case '3':
            return LinkLayerConnectionInterval::Ms20;
        case '4':
            return LinkLayerConnectionInterval::Ms25;
        case '5':
            return LinkLayerConnectionInterval::Ms30;
        case '6':
            return LinkLayerConnectionInterval::Ms35;
        case '7':
            return LinkLayerConnectionInterval::Ms40;
        case '8':
            return LinkLayerConnectionInterval::Ms45;
        case '9':
            return LinkLayerConnectionInterval::Ms4000;
        default:
            return LinkLayerConnectionInterval::Ms20; // Fallback to default min
    }
}

/// @brief Helper function to parse ConnectionSupervisionTimeout from character
/// @param c Character in range '0'-'6'
/// @return ConnectionSupervisionTimeout enum value
/// Corresponds to Ada's Get_Connection_Supervision_Timeout (hm11.adb:716-717)
constexpr types::ConnectionSupervisionTimeout char_to_connection_supervision_timeout(char c) noexcept {
    using types::ConnectionSupervisionTimeout;
    switch (c) {
        case '0':
            return ConnectionSupervisionTimeout::Ms100;
        case '1':
            return ConnectionSupervisionTimeout::Ms1000;
        case '2':
            return ConnectionSupervisionTimeout::Ms2000;
        case '3':
            return ConnectionSupervisionTimeout::Ms3000;
        case '4':
            return ConnectionSupervisionTimeout::Ms4000;
        case '5':
            return ConnectionSupervisionTimeout::Ms5000;
        case '6':
            return ConnectionSupervisionTimeout::Ms6000;
        default:
            return ConnectionSupervisionTimeout::Ms6000; // Fallback to default
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_minimum_link_layer_connection_interval(
    types::LinkLayerConnectionInterval& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+COMI?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto INTERVAL_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        result = char_to_link_layer_connection_interval(INTERVAL_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_minimum_link_layer_connection_interval(
    types::LinkLayerConnectionInterval value,
    hal::UartStatus& status) noexcept {
    const auto INTERVAL_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+COMI").append(INTERVAL_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(INTERVAL_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_maximum_link_layer_connection_interval(
    types::LinkLayerConnectionInterval& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+COMA?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto INTERVAL_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        result = char_to_link_layer_connection_interval(INTERVAL_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_maximum_link_layer_connection_interval(
    types::LinkLayerConnectionInterval value,
    hal::UartStatus& status) noexcept {
    const auto INTERVAL_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+COMA").append(INTERVAL_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(INTERVAL_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_link_layer_connection_slave_latency(
    types::LayerConnectionLatency& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+COLA?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto LATENCY_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto LATENCY_VALUE = char_to_digit(LATENCY_CHAR);

        // LayerConnectionLatency is RangedInteger<uint8_t, 0, 4>
        // Validation happens in constructor
        result = types::LayerConnectionLatency{LATENCY_VALUE};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_link_layer_connection_slave_latency(
    types::LayerConnectionLatency value,
    hal::UartStatus& status) noexcept {
    // Convert RangedInteger to string
    const auto LATENCY_VALUE = value.get();
    char latency_char = static_cast<char>('0' + LATENCY_VALUE);
    std::string_view latency_str(&latency_char, 1);

    util::CommandBuilder cmd;
    cmd.append("AT+COLA").append(latency_str);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(latency_str);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_connection_supervision_timeout(
    types::ConnectionSupervisionTimeout& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+COSU?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto TIMEOUT_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        result = char_to_connection_supervision_timeout(TIMEOUT_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_connection_supervision_timeout(
    types::ConnectionSupervisionTimeout value,
    hal::UartStatus& status) noexcept {
    const auto TIMEOUT_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+COSU").append(TIMEOUT_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(TIMEOUT_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_update_connection(bool& result, hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+COUP?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto VALUE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        result = (VALUE_CHAR == '1');
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_update_connection(bool value, hal::UartStatus& status) noexcept {
    const auto VALUE_STR = util::bool_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+COUP").append(VALUE_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(VALUE_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_discovery_time(
    types::DiscoveryTime& time,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+SCAN?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto TIME_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto TIME_VALUE = char_to_digit(TIME_CHAR);

        // DiscoveryTime is RangedInteger<uint8_t, 1, 9>
        // Validation happens in constructor
        time = types::DiscoveryTime{TIME_VALUE};
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_discovery_time(
    types::DiscoveryTime time,
    hal::UartStatus& status) noexcept {
    // Convert RangedInteger to string
    const auto TIME_VALUE = time.get();
    char time_char = static_cast<char>('0' + TIME_VALUE);
    std::string_view time_str(&time_char, 1);

    util::CommandBuilder cmd;
    cmd.append("AT+SCAN").append(time_str);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(time_str);

    transmit_and_check(cmd.get(), expected.get(), status);
}

// ============================================================================
// Advertising Configuration Operations
// ============================================================================

/// @brief Helper function to parse AdvertisingInterval from character
/// @param c Character in range '0'-'9' or 'A'-'F'
/// @return AdvertisingInterval enum value
/// Corresponds to Ada's Get_Advertising_Interval case statement (hm11.adb:376-395)
constexpr types::AdvertisingInterval char_to_advertising_interval(char c) noexcept {
    using types::AdvertisingInterval;
    switch (c) {
        case '0':
            return AdvertisingInterval::Ms100;
        case '1':
            return AdvertisingInterval::Ms211;
        case '2':
            return AdvertisingInterval::Ms252;
        case '3':
            return AdvertisingInterval::Ms318;
        case '4':
            return AdvertisingInterval::Ms417;
        case '5':
            return AdvertisingInterval::Ms546;
        case '6':
            return AdvertisingInterval::Ms760;
        case '7':
            return AdvertisingInterval::Ms852;
        case '8':
            return AdvertisingInterval::Ms1022;
        case '9':
            return AdvertisingInterval::Ms1285;
        case 'A':
            return AdvertisingInterval::Ms2000;
        case 'B':
            return AdvertisingInterval::Ms3000;
        case 'C':
            return AdvertisingInterval::Ms4000;
        case 'D':
            return AdvertisingInterval::Ms5000;
        case 'E':
            return AdvertisingInterval::Ms6000;
        case 'F':
            return AdvertisingInterval::Ms7000;
        default:
            return AdvertisingInterval::Ms100; // Fallback
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_advertising_interval(
    types::AdvertisingInterval& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+ADVI?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto INTERVAL_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        result = char_to_advertising_interval(INTERVAL_CHAR);
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_advertising_interval(
    types::AdvertisingInterval value,
    hal::UartStatus& status) noexcept {
    const auto INTERVAL_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+ADVI").append(INTERVAL_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(INTERVAL_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_advertising_type(
    types::AdvertisingType& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+ADTY?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto TYPE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto TYPE_VALUE = char_to_digit(TYPE_CHAR);

        // AdvertisingType enum values match ordinal 0-3
        if (TYPE_VALUE <= 3) {
            result = static_cast<types::AdvertisingType>(TYPE_VALUE);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_advertising_type(
    types::AdvertisingType value,
    hal::UartStatus& status) noexcept {
    const auto TYPE_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+ADTY").append(TYPE_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(TYPE_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_advertising_mode(
    types::AdvertisingMode& result,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+RELI?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto MODE_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto MODE_VALUE = char_to_digit(MODE_CHAR);

        // AdvertisingMode enum values: 0=Normal, 1=Reliable
        if (MODE_VALUE <= 1) {
            result = static_cast<types::AdvertisingMode>(MODE_VALUE);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_advertising_mode(
    types::AdvertisingMode value,
    hal::UartStatus& status) noexcept {
    const auto MODE_STR = util::enum_to_string(value);

    util::CommandBuilder cmd;
    cmd.append("AT+RELI").append(MODE_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(MODE_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_advertising_flag(
    const types::AdvertisingFlag& flag,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+FLAG").append(flag);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(flag);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_advertising_data(
    const types::AdvertisementData& data,
    hal::UartStatus& status) noexcept {
    util::CommandBuilder cmd;
    cmd.append("AT+PACK").append(data);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(data);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::set_show_device_information_when_discovery(
    types::ShowDeviceInformation show,
    hal::UartStatus& status) noexcept {
    const auto SHOW_STR = util::enum_to_string(show);

    util::CommandBuilder cmd;
    cmd.append("AT+SHOW").append(SHOW_STR);

    util::CommandBuilder expected;
    expected.append("OK+Set:").append(SHOW_STR);

    transmit_and_check(cmd.get(), expected.get(), status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
void HM11Driver<UARTImpl, ReceiveHandler>::get_show_device_information_when_discovery(
    types::ShowDeviceInformation& show,
    hal::UartStatus& status) noexcept {
    constexpr std::string_view OK_GET = "OK+Get:";
    constexpr std::size_t RESPONSE_SIZE = OK_GET.size() + 1;
    hal::UartData8b buffer(response_buffer_.data(), RESPONSE_SIZE);

    transmit_and_check("AT+SHOW?", OK_GET, buffer, status);

    if (status == hal::UartStatus::Ok) {
        const auto SHOW_CHAR = static_cast<char>(buffer[OK_GET.size()]);
        const auto SHOW_VALUE = char_to_digit(SHOW_CHAR);

        // ShowDeviceInformation enum values match ordinal 0-3
        if (SHOW_VALUE <= 3) {
            show = static_cast<types::ShowDeviceInformation>(SHOW_VALUE);
        } else {
            status = hal::UartStatus::ErrError;
        }
    }
}

// ============================================================================
// Discovery and Scanning Operations Template Implementations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::scan(
    DiscoveredCallback callback,
    std::chrono::duration<Rep, Period> /*timeout*/,
    hal::UartStatus& status) noexcept {
    // Constants for protocol prefixes
    constexpr std::string_view OK_DISCS = "OK+DISCS"; // Discovery hello
    constexpr std::string_view OK_DISC = "OK+DISC:";  // MAC address prefix
    constexpr std::string_view OK_NAME = "OK+NAME:";  // Name prefix
    constexpr std::string_view OK_RSSI = "OK+RSSI:";  // RSSI prefix
    constexpr std::string_view OK_DISCE = "OK+DISCE"; // Discovery end
    constexpr std::size_t MIN_PREFIX = 8;             // Minimum prefix length

    // Helper lambda to invoke callback with current discovered_info_
    auto invoke_callback = [this, callback]() {
        if (discovered_info_.is_valid()) {
            callback(
                discovered_info_.id,
                discovered_info_.mac,
                discovered_info_.name.view(),
                discovered_info_.rssi.view());
            discovered_info_.reset();
        }
    };

    // Initialize state
    discovered_info_.reset();
    ScanStage stage = ScanStage::Selection;

    // Send discovery command
    transmit("AT+DISC?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive streaming response using internal buffer
    constexpr std::size_t BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UartData8b buffer(response_buffer_.data(), BUFFER_SIZE);

    receive_handler_(uart_port_, buffer, status, true);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the buffer using state machine (Decision 11)
    // Convert buffer to string_view for parsing
    std::string_view data_view = util::to_string_view(buffer);

    std::size_t pos = 0;
    bool scan_complete = false;

    while (pos < data_view.size() && !scan_complete) {
        switch (stage) {
            case ScanStage::Selection: {
                // Need at least MIN_PREFIX characters to check prefix
                if (pos + MIN_PREFIX > data_view.size()) {
                    break; // Exit inner loop, need more data
                }

                std::string_view remaining = data_view.substr(pos);

                // Check for end marker first
                if (util::starts_with(remaining, OK_DISCE)) {
                    // Invoke callback with any accumulated data (Decision 28: soft-stop)
                    invoke_callback();
                    scan_complete = true;
                    break;
                }

                // Skip discovery hello
                if (util::starts_with(remaining, OK_DISCS)) {
                    pos += OK_DISCS.size();
                    continue;
                }

                // Check for MAC address
                if (util::starts_with(remaining, OK_DISC)) {
                    // Call callback for previous device if exists
                    invoke_callback();
                    pos += OK_DISC.size();
                    stage = ScanStage::MAC;
                    continue;
                }

                // Check for name
                if (util::starts_with(remaining, OK_NAME)) {
                    pos += OK_NAME.size();
                    stage = ScanStage::Name;
                    continue;
                }

                // Check for RSSI
                if (util::starts_with(remaining, OK_RSSI)) {
                    pos += OK_RSSI.size();
                    stage = ScanStage::RSSI;
                    continue;
                }

                // No recognized prefix, skip character
                ++pos;
                break;
            }

            case ScanStage::MAC: {
                // Parse 12-character hex MAC address
                constexpr std::size_t MAC_SIZE = 12;
                if (pos + MAC_SIZE > data_view.size()) {
                    // Not enough data for MAC
                    break;
                }

                std::string_view mac_str = data_view.substr(pos, MAC_SIZE);
                discovered_info_.mac = types::MacAddress{mac_str};
                pos += MAC_SIZE;
                stage = ScanStage::Selection;
                break;
            }

            case ScanStage::Name: {
                // Parse until CRLF
                std::string_view remaining = data_view.substr(pos);
                std::size_t crlf_pos = util::find_crlf(remaining);

                if (crlf_pos == std::string_view::npos) {
                    // No CRLF found, append all remaining data
                    discovered_info_.name.append(remaining);
                    pos = data_view.size();
                } else {
                    // Found CRLF, append data before it
                    discovered_info_.name.append(remaining.substr(0, crlf_pos));
                    pos += crlf_pos + 2; // Skip CRLF
                    stage = ScanStage::Selection;
                }
                break;
            }

            case ScanStage::RSSI: {
                // Parse until CRLF
                std::string_view remaining = data_view.substr(pos);
                std::size_t crlf_pos = util::find_crlf(remaining);

                if (crlf_pos == std::string_view::npos) {
                    // No CRLF found, append all remaining data
                    discovered_info_.rssi.append(remaining);
                    pos = data_view.size();
                } else {
                    // Found CRLF, append data before it
                    discovered_info_.rssi.append(remaining.substr(0, crlf_pos));
                    pos += crlf_pos + 2; // Skip CRLF
                    stage = ScanStage::Selection;
                }
                break;
            }
        }
    }

    // Invoke callback with any remaining accumulated data (Decision 28: soft-stop timeout)
    invoke_callback();
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::scan_ibeacon(
    DiscoveredIBeaconCallback callback,
    std::chrono::duration<Rep, Period> /*timeout*/,
    hal::UartStatus& status) noexcept {
    // Constants for protocol
    constexpr std::string_view OK_DISCS = "OK+DISCS"; // Discovery hello
    constexpr std::string_view OK_DISC = "OK+DISC";   // iBeacon data prefix (no colon)
    constexpr std::string_view OK_DISCE = "OK+DISCE"; // Discovery end
    constexpr std::size_t IBEACON_DATA_LENGTH = 74;   // P0(8) + P1(32) + P2(10) + P3(12) + P4(4)

    // Send iBeacon discovery command
    transmit("AT+DISI?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive streaming response using internal buffer
    constexpr std::size_t BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UartData8b buffer(response_buffer_.data(), BUFFER_SIZE);

    receive_handler_(uart_port_, buffer, status, true);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the buffer for iBeacon data
    // Convert buffer to string_view for parsing
    std::string_view data_view = util::to_string_view(buffer);

    std::size_t pos = 0;
    bool scan_complete = false;

    while (pos < data_view.size() && !scan_complete) {
        std::string_view remaining = data_view.substr(pos);

        // Check for end marker
        if (util::starts_with(remaining, OK_DISCE)) {
            scan_complete = true;
            break;
        }

        // Skip discovery hello
        if (util::starts_with(remaining, OK_DISCS)) {
            pos += OK_DISCS.size();
            continue;
        }

        // Check for iBeacon data
        if (util::starts_with(remaining, OK_DISC)) {
            // Need OK_DISC prefix + 74 characters of data
            const std::size_t TOTAL_NEEDED = OK_DISC.size() + IBEACON_DATA_LENGTH;
            if (pos + TOTAL_NEEDED <= data_view.size()) {
                // Extract iBeacon data (skip OK_DISC prefix)
                std::string_view ibeacon_data = data_view.substr(pos + OK_DISC.size(), IBEACON_DATA_LENGTH);

                // Invoke callback with device info
                callback(ibeacon_data);

                pos += TOTAL_NEEDED;
                continue;
            }
        }

        // No match, advance position
        ++pos;
    }
}

// ============================================================================
// GATT Operations (Service and Characteristic Discovery) Template Implementations
// ============================================================================

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::find_all_services_uuid(
    ServiceUuidCallback callback,
    std::chrono::duration<Rep, Period> timeout,
    hal::UartStatus& status) noexcept {
    // Timeout parameter reserved for future streaming implementation
    (void)timeout;

    // Constants for protocol markers
    constexpr std::string_view HEADER_START =
        "********************************************************"; // 56 asterisks mark start/end
    constexpr std::size_t SERVICE_INFO_LENGTH = 14;                 // "xxxx:xxxx:xxxx" (4:4:4 + 2 colons)

    // Send service discovery command
    transmit("AT+FINDSERVICES?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Receive streaming response using internal buffer
    constexpr std::size_t BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UartData8b buffer(response_buffer_.data(), BUFFER_SIZE);

    receive_handler_(uart_port_, buffer, status, true);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the buffer for service information
    // Convert buffer to string_view for parsing
    std::string_view data_view = util::to_string_view(buffer);

    std::size_t pos = 0;
    bool started = false;
    bool discovery_complete = false;

    // Simple parsing: Look for lines containing "xxxx:xxxx:xxxx" format
    // Skip header markers (56 asterisks), CRLF, and extract service info
    while (pos < data_view.size() && !discovery_complete) {
        // Check for header markers (start or end)
        if (pos + HEADER_START.size() <= data_view.size()) {
            std::string_view remaining = data_view.substr(pos);
            if (util::starts_with(remaining, HEADER_START)) {
                if (started) {
                    // End marker - discovery complete
                    discovery_complete = true;
                    break;
                }
                // Start marker
                started = true;
                pos += HEADER_START.size();
                continue;
            }
        }

        // Skip CRLF
        if (pos + 2 <= data_view.size()) {
            if (data_view[pos] == '\r' && data_view[pos + 1] == '\n') {
                pos += 2;
                continue;
            }
        }

        // Check if we have enough data for service info
        if (started && pos + SERVICE_INFO_LENGTH <= data_view.size()) {
            std::string_view potential_service = data_view.substr(pos, SERVICE_INFO_LENGTH);

            // Validate format: xxxx:xxxx:xxxx (4 hex : 4 hex : 4 hex)
            if (potential_service[4] == ':' && potential_service[9] == ':') {
                // Extract the three 4-character hex fields
                std::string_view start_handle = potential_service.substr(0, 4);
                std::string_view end_handle = potential_service.substr(5, 4);
                std::string_view uuid = potential_service.substr(10, 4);

                // Validate all fields are hex
                if (util::is_all_hex(start_handle) && util::is_all_hex(end_handle) && util::is_all_hex(uuid)) {
                    // Invoke callback with service info
                    callback(potential_service);
                    pos += SERVICE_INFO_LENGTH;
                    continue;
                }
            }
        }

        // No match, advance position
        ++pos;
    }
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::find_all_characteristic_uuid(
    CharacteristicUuidCallback callback,
    std::chrono::duration<Rep, Period> timeout,
    hal::UartStatus& status) noexcept {
    // Send characteristic discovery command
    transmit("AT+FINDALLCHARS?", status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Delegate to common characteristic parsing logic
    // (matches Ada's pattern: Find_All_Characteristic_UUID calls Read_Characteristic_UUID)
    // Timeout is passed to impl function
    read_characteristic_uuid_impl(callback, timeout, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::find_characteristic_uuid(
    const types::HandleType& from,
    const types::HandleType& to,
    CharacteristicUuidCallback callback,
    std::chrono::duration<Rep, Period> timeout,
    hal::UartStatus& status) noexcept {
    // Build command: AT+CHARxxxxYYYY? where xxxx=from, YYYY=to
    constexpr std::string_view CMD_PREFIX = "AT+CHAR";
    constexpr std::string_view CMD_SUFFIX = "?";

    // Use util::CommandBuilder to construct command
    util::CommandBuilder builder;
    builder.append(CMD_PREFIX);
    builder.append(from.view());
    builder.append(to.view());
    builder.append(CMD_SUFFIX);

    transmit(builder.get(), status);
    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Delegate to common characteristic parsing logic
    // (matches Ada's pattern: Find_Characteristic_UUID calls Read_Characteristic_UUID)
    read_characteristic_uuid_impl(callback, timeout, status);
}

template <typename UARTImpl, typename ReceiveHandler>
    requires hal::IS_UART_PORT_V<UARTImpl> && hal::IS_RECEIVE_HANDLER_V<ReceiveHandler, UARTImpl>
template <typename Rep, typename Period>
void HM11Driver<UARTImpl, ReceiveHandler>::read_characteristic_uuid_impl(
    CharacteristicUuidCallback callback,
    std::chrono::duration<Rep, Period> timeout,
    hal::UartStatus& status) noexcept {
    // Timeout parameter reserved for future streaming implementation
    (void)timeout;

    // Constants for protocol markers
    constexpr std::string_view HEADER_START =
        "********************************************************"; // 56 asterisks mark start/end
    constexpr std::size_t CHAR_INFO_LENGTH = 24;                    // "xxxx:ppppppppppppp:xxxx" (4:14:4 + 2 colons)

    // Receive streaming response using internal buffer
    constexpr std::size_t BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UartData8b buffer(response_buffer_.data(), BUFFER_SIZE);

    receive_handler_(uart_port_, buffer, status, true);

    if (status != hal::UartStatus::Ok) {
        return;
    }

    // Parse the buffer for characteristic information
    // Convert buffer to string_view for parsing
    std::string_view data_view = util::to_string_view(buffer);

    std::size_t pos = 0;
    bool started = false;
    bool discovery_complete = false;

    // Simple parsing: Look for lines containing "xxxx:ppppppppppppp:xxxx" format
    // Skip header markers (56 asterisks), CRLF, and extract characteristic info
    while (pos < data_view.size() && !discovery_complete) {
        // Check for header markers (start or end)
        if (pos + HEADER_START.size() <= data_view.size()) {
            std::string_view remaining = data_view.substr(pos);
            if (util::starts_with(remaining, HEADER_START)) {
                if (started) {
                    // End marker - discovery complete
                    discovery_complete = true;
                    break;
                }
                // Start marker
                started = true;
                pos += HEADER_START.size();
                continue;
            }
        }

        // Skip CRLF
        if (pos + 2 <= data_view.size()) {
            if (data_view[pos] == '\r' && data_view[pos + 1] == '\n') {
                pos += 2;
                continue;
            }
        }

        // Check if we have enough data for characteristic info
        if (started && pos + CHAR_INFO_LENGTH <= data_view.size()) {
            std::string_view potential_char = data_view.substr(pos, CHAR_INFO_LENGTH);

            // Validate format: xxxx:ppppppppppppp:xxxx (4 hex : 14 props : 4 hex)
            if (potential_char[4] == ':' && potential_char[19] == ':') {
                // Extract the three fields
                std::string_view handle = potential_char.substr(0, 4);
                std::string_view properties = potential_char.substr(5, 14);
                std::string_view uuid = potential_char.substr(20, 4);

                // Validate handle and UUID are hex
                // Properties format is "RD|WR|WN|NO|IN" or with "--" for unsupported
                if (util::is_all_hex(handle) && util::is_all_hex(uuid)) {
                    // Validate properties format (contains only valid characters)
                    bool valid_props = true;
                    for (char ch : properties) {
                        if (ch != 'R' && ch != 'D' && ch != 'W' && ch != 'N' && ch != 'O' && ch != 'I' && ch != '|' &&
                            ch != '-') {
                            valid_props = false;
                            break;
                        }
                    }

                    if (valid_props) {
                        // Invoke callback with characteristic info
                        callback(potential_char);
                        pos += CHAR_INFO_LENGTH;
                        continue;
                    }
                }
            }
        }

        // No match, advance position
        ++pos;
    }
}

} // namespace hm11

// Include template implementations for device management operations
#include "hm11/hm11_driver_device.inl"

// Include template implementations for GATT characteristic access operations
#include "hm11/hm11_driver_gatt_access.inl"

// Include template implementations for iBeacon configuration operations
#include "hm11/hm11_driver_ibeacon.inl"

// Include template implementations for security and authentication operations
#include "hm11/hm11_driver_security.inl"

// Include template implementations for UART configuration operations
#include "hm11/hm11_driver_uart.inl"

// Include template implementations for HMSensor-specific operations
#include "hm11/hm11_driver_sensor.inl"

// Include template implementations for hardware control operations
#include "hm11/hm11_driver_hardware.inl"
