#pragma once

// HM11 Bluetooth Low Energy Driver
// Implements Design Decision #1 (Dependency Injection Mechanism) using C++20 concepts
// Template-based zero-overhead abstraction with compile-time interface validation

#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "hal/uart_interface.hpp"
#include "types.hpp"
#include "util.hpp"

namespace hm11 {

/// Variable-capacity string with bounded stack allocation
/// Equivalent to Ada's Variable_String(Length) discriminant record
///
/// This matches the Ada pattern:
/// ```ada
/// type Variable_String (Length : Positive) is record
///    Value : String (1 .. Length);
///    Last  : Natural := 0;
/// end record;
/// ```
///
/// @tparam Capacity Maximum number of characters that can be stored
template <std::size_t Capacity> class VariableString {
  public:
    static constexpr std::size_t MAX_CAPACITY = Capacity;

    /// Default constructor - creates empty string
    constexpr VariableString() noexcept : buffer{}, length_(0) {}

    /// Constructor from string_view with capacity checking
    explicit VariableString(std::string_view str) : buffer{}, length_(0) { assign(str); }

    /// Get current length
    constexpr std::size_t size() const noexcept { return length_; }

    /// Get current length (Ada's Last field)
    constexpr std::size_t length() const noexcept { return length_; }

    /// Check if empty
    constexpr bool empty() const noexcept { return length_ == 0; }

    /// Get remaining capacity
    constexpr std::size_t remaining_capacity() const noexcept { return Capacity - length_; }

    /// Clear the string
    constexpr void clear() noexcept { length_ = 0; }

    /// Get string view of current content
    constexpr std::string_view view() const noexcept { return std::string_view(buffer.data(), length_); }

    /// Get const char* to underlying data
    constexpr const char* data() const noexcept { return buffer.data(); }

    /// Assign from string_view
    void assign(std::string_view str) {
        if (str.size() > Capacity) {
            throw std::length_error("VariableString: assignment would exceed capacity");
        }

        // Copy characters
        for (std::size_t i = 0; i < str.size(); ++i) {
            buffer[i] = str[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        length_ = str.size();
    }

    /// Append character
    void append(char c) {
        if (length_ >= Capacity) {
            throw std::length_error("VariableString: append would exceed capacity");
        }
        buffer[length_++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// Append string_view
    void append(std::string_view str) {
        if (length_ + str.size() > Capacity) {
            throw std::length_error("VariableString: append would exceed capacity");
        }

        for (char c : str) {
            buffer[length_++] = c; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    }

  private:
    std::array<char, Capacity> buffer;
    std::size_t length_;
};

/// Discovery information for temporary storage during parsing
/// Equivalent to Ada's Discovered_Info_Type record
///
/// Maps to Ada:
/// ```ada
/// type Discovered_Info_Type is record
///    Id   : Character;
///    MAC  : MAC_Address;
///    Name : Variable_String (Max_Name_Length);
///    RSSI : Variable_String (Max_RSII_Length);
/// end record;
/// ```
struct DiscoveredInfo {
    char id = ' ';
    MacAddress mac;
    VariableString<MAX_NAME_LENGTH> name;
    VariableString<MAX_RSSI_LENGTH> rssi;

    /// Reset to null state (Ada's Null_Discovered_Info pattern)
    void reset() noexcept {
        id = ' ';
        mac = MacAddress{};
        name.clear();
        rssi.clear();
    }
};

/// Scanning stage enumeration for state machine
/// Equivalent to Ada's Scan_Stage_Kind
enum class ScanStage : uint8_t {
    Selection = 0, ///< Looking for prefixes
    MAC = 1,       ///< Parsing 12-byte MAC address
    Name = 2,      ///< Parsing device name until CR+LF
    RSSI = 3       ///< Parsing RSSI until CR+LF
};

/// GATT service discovery parsing stage enumeration for state machine
/// Implements streaming response parsing with explicit state transitions
enum class GattServiceStage : uint8_t {
    LookingForHeader = 0, ///< Looking for service discovery start header
    ParsingService = 1,   ///< Parsing service UUID data
    LookingForEnd = 2     ///< Looking for service discovery end marker
};

/// GATT characteristic discovery parsing stage enumeration for state machine
/// Implements streaming response parsing with explicit state transitions
enum class GattCharacteristicStage : uint8_t {
    LookingForHeader = 0,      ///< Looking for characteristic discovery start header
    ParsingCharacteristic = 1, ///< Parsing characteristic UUID data
    LookingForEnd = 2          ///< Looking for characteristic discovery end marker
};

/// Driver operational state for resource management
/// Tracks lifecycle state for graceful cleanup
enum class OperationalState : uint8_t {
    Initializing = 0, ///< Constructor phase - hardware setup in progress
    Ready = 1,        ///< Normal operation - ready for commands
    ShuttingDown = 2, ///< Destructor phase - graceful cleanup in progress
    Failed = 3        ///< Error state - hardware initialization failed
};

/// Forward declaration of receive strategy interface
/// (To be implemented in future tasks - Decision #3)
template <typename UARTImpl> class ReceiveStrategy;

/// HM11 Bluetooth Low Energy Driver Template Class
///
/// This is a template-based driver that provides zero-overhead abstraction
/// through compile-time polymorphism. The UART implementation is injected
/// as a template parameter, enabling different hardware backends while
/// maintaining type safety through C++20 concepts.
///
/// Equivalent to Ada's:
/// ```ada
/// type HM11_Driver
///   (Port            : HAL.UART.Any_UART_Port;
///    Receive         : Receive_Handler;
///    Readed_Position : Last_Read_Position_Handler)
/// is limited private;
/// ```
///
/// @tparam UARTImpl UART implementation type (must satisfy UARTInterface concept)
template <hal::UARTInterface UARTImpl> class HM11Driver {
  public:
    /// Type alias for the UART implementation
    using UARTType = UARTImpl;
    using StatusType = hal::UARTStatus;

    /// Constructor with UART dependency injection
    /// Follows RAII principles with exclusive ownership (Decision #22)
    /// @param uart UART implementation instance (moved into driver)
    explicit HM11Driver(UARTImpl&& uart_impl) noexcept
        : uart_(std::move(uart_impl)), response_buffer_{}, discovered_info_{},
          operational_state_(OperationalState::Initializing), scan_stage_(ScanStage::Selection), buffer_position_(0),
          gatt_service_stage_(GattServiceStage::LookingForHeader), gatt_discovery_started_(false),
          gatt_characteristic_stage_(GattCharacteristicStage::LookingForHeader),
          gatt_characteristic_discovery_started_(false), discovered_devices_{}, discovered_device_count_(0) {
        static_assert(std::is_move_constructible_v<UARTImpl>, "UART implementation must be move constructible");
        static_assert(!std::is_copy_constructible_v<HM11Driver>, "Driver should have unique ownership (no copying)");

        // Initialize hardware resources
        initialize_hardware();
        operational_state_ = OperationalState::Ready;
    }

    // Disable copy operations following Decision #22 (RAII Ownership Model)
    // Matches Ada's limited private pattern for exclusive resource ownership
    HM11Driver(const HM11Driver&) = delete;
    HM11Driver& operator=(const HM11Driver&) = delete;

    // Disable move operations following Decision #22
    // Maintains Ada's non-movable semantics for hardware resource lifetime
    HM11Driver(HM11Driver&&) = delete;
    HM11Driver& operator=(HM11Driver&&) = delete;

    /// Destructor performs graceful cleanup with timeout (Decision #22)
    /// Attempts graceful stop of ongoing operations, waits up to 100ms for completion,
    /// force-stops if timeout exceeded, then unregisters IRQ handlers
    ~HM11Driver() noexcept { perform_graceful_cleanup(); }

    /// Test connectivity to HM11 module
    /// Equivalent to Ada's Test procedure
    /// Sends "AT" command and expects "OK" response
    /// @param status Output parameter for operation result
    void Test(StatusType& status) noexcept { TransmitAndCheck("AT", "OK", status); }

    /// Set the role of the HM11 module
    /// Equivalent to Ada's Set_Role procedure
    /// @param role Role to set (Peripheral or Central)
    /// @param status Output parameter for operation result
    void SetRole(Role role, StatusType& status) noexcept {
        // Construct command: "AT+ROLE" + role_value
        const auto ROLE_STR = util::to_string(role);

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+ROLE").append(ROLE_STR);

            // Construct expected response: "OK+Set:" + role_value
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(ROLE_STR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get the current role of the HM11 module
    /// Equivalent to Ada's Get_Role procedure
    /// @param role Output parameter for the current role
    /// @param status Output parameter for operation result
    void GetRole(Role& role, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1; // +1 for role character

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+ROLE?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto PARSED_ROLE = util::parse_role(PARSED_RESPONSE->front());
        if (!PARSED_ROLE.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        role = *PARSED_ROLE;
    }

    /// Get software version string from HM11 module
    /// Equivalent to Ada's Software_Version function
    /// Returns version string like "HMSoftV547" or empty string on error
    /// @return Version string (empty if error occurs)
    [[nodiscard]] std::string SoftwareVersion() noexcept {
        StatusType status = StatusType::Ok;

        // Send version query command
        Transmit("AT+VERS?", status);
        if (status != StatusType::Ok) {
            return {};
        }

        // Receive response (variable length, use full buffer)
        hal::UARTData8b response_data{response_buffer_.data(), response_buffer_.size()};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return {};
        }

        // Find null terminator or end of printable characters
        std::size_t version_length = 0;
        for (std::size_t i = 0; i < response_buffer_.size(); ++i) {
            if (response_buffer_[i] == 0 || response_buffer_[i] < 32 || response_buffer_[i] > 126) {
                version_length = i;
                break;
            }
        }

        if (version_length == 0) {
            return {};
        }

        // Convert to string
        return {reinterpret_cast<const char*>(response_buffer_.data()), version_length};
    }

    /// Access to underlying UART for advanced operations
    /// Provides controlled access while maintaining encapsulation
    /// @return Reference to UART implementation
    [[nodiscard]] UARTImpl& GetUart() noexcept { return uart_; }

    /// Const access to underlying UART for inspection
    /// @return Const reference to UART implementation
    [[nodiscard]] const UARTImpl& GetUart() const noexcept { return uart_; }

    // ============================================================================
    // Security and Bonding Operations
    // ============================================================================

    /// Set PIN code for authentication
    /// Equivalent to Ada's Set_PIN_Code procedure
    /// V7xx default: 000000
    /// @param pin 6-digit PIN code
    /// @param status Output parameter for operation result
    void SetPinCode(const PinType& pin, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+PASS").append(pin.view());

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(pin.view());

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get current PIN code
    /// Equivalent to Ada's Get_PIN_Code procedure
    /// @param pin Output parameter for 6-digit PIN code
    /// @param status Output parameter for operation result
    void GetPinCode(PinType& pin, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t PIN_LENGTH = 6;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + PIN_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+PASS?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->size() != PIN_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy PIN code
        try {
            pin = PinType(*PARSED_RESPONSE);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set bond mode for authentication
    /// Equivalent to Ada's Set_Bond_Mode procedure
    /// Default: AuthAndBonded
    /// Note: Android system AuthNotNeedPin is same as AuthWithPin
    /// @param mode Bond mode (NotNeedPin, AuthNotNeedPin, AuthWithPin, AuthAndBonded)
    /// @param status Output parameter for operation result
    void SetBondMode(BondMode mode, StatusType& status) noexcept {
        try {
            const auto MODE_CHAR = static_cast<char>('0' + static_cast<uint8_t>(mode));

            util::ATCommandBuilder cmd;
            cmd.append("AT+TYPE").append(MODE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(MODE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get current bond mode
    /// Equivalent to Ada's Get_Bond_Mode procedure
    /// Note: Uses AT+SHOW? command (not AT+TYPE?)
    /// @param mode Output parameter for bond mode
    /// @param status Output parameter for operation result
    void GetBondMode(BondMode& mode, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1; // +1 for mode character

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command (Ada uses AT+SHOW? not AT+TYPE?)
        Transmit("AT+SHOW?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse mode character ('0'-'3')
        const char MODE_CHAR = PARSED_RESPONSE->front();
        if (MODE_CHAR < '0' || MODE_CHAR > '3') {
            status = StatusType::ErrError;
            return;
        }

        mode = static_cast<BondMode>(MODE_CHAR - '0');
    }

    /// Remove all bond information from module
    /// Equivalent to Ada's Remove_Bond_Information procedure
    /// Clears all stored bonding data
    /// @param status Output parameter for operation result
    void RemoveBondInformation(StatusType& status) noexcept {
        TransmitAndCheck("AT+ERASE", "OK+ERASE", status);
    }

    // ============================================================================
    // Notification Settings
    // ============================================================================

    /// Enable or disable connection notification information
    /// Equivalent to Ada's Set_Notify_Information procedure
    /// When enabled, module sends "OK+CONN" or "OK+LOST" messages on connection state changes
    /// @param enable True to enable notifications, false to disable
    /// @param status Output parameter for operation result
    void SetNotifyInformation(bool enable, StatusType& status) noexcept {
        try {
            const char NOTIFY_CHAR = enable ? '1' : '0';

            util::ATCommandBuilder cmd;
            cmd.append("AT+NOTI").append(NOTIFY_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(NOTIFY_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get current connection notification information setting
    /// Equivalent to Ada's Get_Notify_Information procedure
    /// @param enabled Output parameter for notification status (true=enabled, false=disabled)
    /// @param status Output parameter for operation result
    void GetNotifyInformation(bool& enabled, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1; // +1 for '0' or '1'

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+NOTI?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse enabled character ('0' or '1')
        const char NOTIFY_CHAR = PARSED_RESPONSE->front();
        enabled = (NOTIFY_CHAR == '1');
    }

    /// Set notification mode (with or without MAC address)
    /// Equivalent to Ada's Set_Notify_Mode procedure
    /// Determines whether connection notifications include the MAC address
    /// @param mode Notification mode (WithoutAddress or WithAddress)
    /// @param status Output parameter for operation result
    void SetNotifyMode(NotifyMode mode, StatusType& status) noexcept {
        try {
            const auto MODE_CHAR = static_cast<char>('0' + static_cast<uint8_t>(mode));

            util::ATCommandBuilder cmd;
            cmd.append("AT+NOTP").append(MODE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(MODE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get current notification mode
    /// Equivalent to Ada's Get_Notify_Mode procedure
    /// @param mode Output parameter for notification mode
    /// @param status Output parameter for operation result
    void GetNotifyMode(NotifyMode& mode, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1; // +1 for mode character

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+NOTP?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse mode character ('0' or '1')
        const char MODE_CHAR = PARSED_RESPONSE->front();
        if (MODE_CHAR < '0' || MODE_CHAR > '1') {
            status = StatusType::ErrError;
            return;
        }

        mode = static_cast<NotifyMode>(MODE_CHAR - '0');
    }

    // ============================================================================
    // Module Configuration
    // ============================================================================

    /// Set module name
    /// Equivalent to Ada's Set_Module_Name procedure
    /// Maximum length: 12 characters
    /// V7xx default: "HMSoft"
    /// @param name Module name (max 12 characters)
    /// @param status Output parameter for operation result
    void SetModuleName(std::string_view name, StatusType& status) noexcept {
        if (name.size() > 12) {
            status = StatusType::ErrError;
            return;
        }

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+NAME").append(name);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(name);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get module name
    /// Equivalent to Ada's Get_Module_Name procedure
    /// @param name Output parameter for module name
    /// @param status Output parameter for operation result
    void GetModuleName(std::string& name, StatusType& status) noexcept {
        constexpr std::string_view OK_NAME_PREFIX = "OK+NAME:";
        constexpr std::size_t MODULE_NAME_MAX_LENGTH = 12;
        const auto EXPECTED_LENGTH = OK_NAME_PREFIX.size() + MODULE_NAME_MAX_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+NAME?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response - find null terminator or end of valid characters
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_NAME_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract name (after "OK+NAME:"), stop at null terminator or non-printable
        std::string_view name_part = RESPONSE_STR.substr(OK_NAME_PREFIX.size());
        std::size_t actual_length = 0;
        for (char c : name_part) {
            if (c == '\0' || c < 32 || c > 126) {
                break;
            }
            actual_length++;
        }

        name = std::string(name_part.substr(0, actual_length));
    }

    /// Set module RX gain
    /// Equivalent to Ada's Set_Module_RX_Gain procedure
    /// @param gain RX gain setting (No or Open)
    /// @param status Output parameter for operation result
    void SetModuleRxGain(RxGain gain, StatusType& status) noexcept {
        try {
            const auto GAIN_CHAR = static_cast<char>('0' + static_cast<uint8_t>(gain));

            util::ATCommandBuilder cmd;
            cmd.append("AT+GAIN").append(GAIN_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(GAIN_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get module RX gain
    /// Equivalent to Ada's Get_Module_RX_Gain procedure
    /// @param gain Output parameter for RX gain setting
    /// @param status Output parameter for operation result
    void GetModuleRxGain(RxGain& gain, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+GAIN?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse gain character ('0' or '1')
        const char GAIN_CHAR = PARSED_RESPONSE->front();
        if (GAIN_CHAR < '0' || GAIN_CHAR > '1') {
            status = StatusType::ErrError;
            return;
        }

        gain = static_cast<RxGain>(GAIN_CHAR - '0');
    }

    /// Set work type (immediate start vs AT command mode)
    /// Equivalent to Ada's Set_Work_Type procedure
    /// Default: RespondAtCommand
    /// WARNING: Setting to StartImmediately disables AT commands until next power cycle
    /// @param work_type Work type (StartImmediately or RespondAtCommand)
    /// @param status Output parameter for operation result
    void SetWorkType(WorkType work_type, StatusType& status) noexcept {
        try {
            const auto TYPE_CHAR = static_cast<char>('0' + static_cast<uint8_t>(work_type));

            util::ATCommandBuilder cmd;
            cmd.append("AT+IMME").append(TYPE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(TYPE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get work type
    /// Equivalent to Ada's Get_Work_Type procedure
    /// @param work_type Output parameter for work type
    /// @param status Output parameter for operation result
    void GetWorkType(WorkType& work_type, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+IMME?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse type character ('0' or '1')
        const char TYPE_CHAR = PARSED_RESPONSE->front();
        if (TYPE_CHAR < '0' || TYPE_CHAR > '1') {
            status = StatusType::ErrError;
            return;
        }

        work_type = static_cast<WorkType>(TYPE_CHAR - '0');
    }

    /// Set work mode
    /// Equivalent to Ada's Set_Work_Mode procedure
    /// Determines module operational mode
    /// @param mode Work mode (TransmissionMode, PioCollectionMode, or RemoteControlMode)
    /// @param status Output parameter for operation result
    void SetWorkMode(WorkMode mode, StatusType& status) noexcept {
        try {
            const auto MODE_CHAR = static_cast<char>('0' + static_cast<uint8_t>(mode));

            util::ATCommandBuilder cmd;
            cmd.append("AT+MODE").append(MODE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(MODE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get work mode
    /// Equivalent to Ada's Get_Work_Mode procedure
    /// @param mode Output parameter for work mode
    /// @param status Output parameter for operation result
    void GetWorkMode(WorkMode& mode, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+MODE?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse mode character ('0', '1', or '2')
        const char MODE_CHAR = PARSED_RESPONSE->front();
        if (MODE_CHAR < '0' || MODE_CHAR > '2') {
            status = StatusType::ErrError;
            return;
        }

        mode = static_cast<WorkMode>(MODE_CHAR - '0');
    }

    // ============================================================================
    // Discovery Operations
    // ============================================================================

    /// Scan for discoverable devices in range
    /// Equivalent to Ada's Scan procedure with streaming response parsing
    /// Required: Set_Role (Central) and Set_Work_Type (Respond_AT_Command)
    ///
    /// This method implements a state machine for parsing streaming discovery responses:
    /// 1. Selection stage - looks for OK+DISCS, OK+DISC:, OK+NAME:, OK+RSSI: prefixes
    /// 2. MAC stage - extracts 12-byte hex MAC address
    /// 3. Name stage - accumulates device name until CR+LF terminator
    /// 4. RSSI stage - accumulates RSSI value until CR+LF terminator
    ///
    /// Callback is invoked when complete device information is available.
    /// Uses soft-stop timeout semantics - completes partial parse on timeout.
    /// Clears discovered device list on module (same behavior as Connect by index).
    ///
    /// @param callback Function pointer for device discovery notifications
    /// @param user_data Optional user data pointer passed to callback (can be nullptr)
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void scan(DiscoveredCallback callback, void* user_data, uint32_t timeout_ms, StatusType& status) noexcept;

    /// Scan for iBeacon devices in range
    /// Equivalent to Ada's Scan_iBeacon procedure
    /// Required: Set_Role (Central) and Set_Work_Type (Respond_AT_Command)
    /// @param callback Function pointer for iBeacon discovery notifications
    /// @param user_data Optional user data pointer passed to callback (can be nullptr)
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void
    scan_ibeacon(DiscoveredIBeaconCallback callback, void* user_data, uint32_t timeout_ms, StatusType& status) noexcept;

    /// Set discovery time duration
    /// Equivalent to Ada's Set_Discovery_Time procedure
    /// Duration for device discovery scanning (1-9 seconds)
    /// @param time Discovery time in seconds (1-9)
    /// @param status Output parameter for operation result
    void SetDiscoveryTime(DiscoveryTime time, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+SCAN").append_int(static_cast<uint8_t>(time.value()));

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append_int(static_cast<uint8_t>(time.value()));

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get discovery time duration
    /// Equivalent to Ada's Get_Discovery_Time procedure
    /// @param time Output parameter for discovery time
    /// @param status Output parameter for operation result
    void GetDiscoveryTime(DiscoveryTime& time, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+SCAN?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse time character ('1'-'9')
        const char TIME_CHAR = PARSED_RESPONSE->front();
        if (TIME_CHAR < '1' || TIME_CHAR > '9') {
            status = StatusType::ErrError;
            return;
        }

        // Create DiscoveryTime from character
        try {
            time = DiscoveryTime(static_cast<uint8_t>(TIME_CHAR - '0'));
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set show device information when discovery
    /// Equivalent to Ada's Set_Show_Device_Information_When_Discovery procedure
    /// Default: DontShow
    /// If ShowName is set, Scan will add the device name information into scan result package.
    /// If ShowRssi is set, Scan will add device RSSI information into scan result package.
    /// If ShowRssiAndName is set, Scan will add device name and RSSI information into scan result package.
    /// @param show Device information display mode
    /// @param status Output parameter for operation result
    void SetShowDeviceInformationWhenDiscovery(ShowDeviceInformation show, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+SHOW").append_int(static_cast<uint8_t>(show));

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append_int(static_cast<uint8_t>(show));

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get show device information when discovery
    /// Equivalent to Ada's Get_Show_Device_Information_When_Discovery procedure
    /// @param show Output parameter for device information display mode
    /// @param status Output parameter for operation result
    void GetShowDeviceInformationWhenDiscovery(ShowDeviceInformation& show, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+SHOW?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse character ('0'-'3')
        const char SHOW_CHAR = PARSED_RESPONSE->front();
        if (SHOW_CHAR < '0' || SHOW_CHAR > '3') {
            status = StatusType::ErrError;
            return;
        }

        // Create ShowDeviceInformation from character
        show = static_cast<ShowDeviceInformation>(SHOW_CHAR - '0');
    }

    // ============================================================================
    // iBeacon Configuration Operations
    // ============================================================================

    /// Set iBeacon enable/disable switch
    /// Equivalent to Ada's Set_iBeacon_Switch procedure
    /// This command is added since V517 version.
    /// Default: False
    /// @param enabled True to enable iBeacon mode, false to disable
    /// @param status Output parameter for operation result
    void set_ibeacon_switch(bool enabled, StatusType& status) noexcept;

    /// Get iBeacon enable/disable switch status
    /// Equivalent to Ada's Get_iBeacon_Switch procedure
    /// @param enabled Output parameter for iBeacon switch status
    /// @param status Output parameter for operation result
    void get_ibeacon_switch(bool& enabled, StatusType& status) noexcept;

    /// Set iBeacon UUID (32 hex characters)
    /// Equivalent to Ada's Set_iBeacon_UUID procedure
    /// This command is added since V520 version.
    /// Default iBeacon UUID is: 74278BDA-B644-4520-8F0C-720EAF059935
    /// Uses multi-part setting approach matching Ada's implementation
    /// @param uuid iBeacon UUID as 32 hex characters
    /// @param status Output parameter for operation result
    void set_ibeacon_uuid(const IBeaconUuid& uuid, StatusType& status) noexcept;

    /// Get iBeacon UUID
    /// Equivalent to Ada's Get_iBeacon_UUID procedure
    /// Uses multi-part getting approach matching Ada's implementation
    /// @param uuid Output parameter for iBeacon UUID
    /// @param status Output parameter for operation result
    void get_ibeacon_uuid(IBeaconUuid& uuid, StatusType& status) noexcept;

    /// Set iBeacon Major version (4 hex characters)
    /// Equivalent to Ada's Set_iBeacon_Marjor_Version procedure
    /// This command is added since V517 version.
    /// Default: 0xFFE0
    /// @param major_version Major version as 4 hex characters
    /// @param status Output parameter for operation result
    void set_ibeacon_major_version(const VersionType& major_version, StatusType& status) noexcept;

    /// Get iBeacon Major version
    /// Equivalent to Ada's Get_iBeacon_Marjor_Version procedure
    /// @param major_version Output parameter for major version
    /// @param status Output parameter for operation result
    void get_ibeacon_major_version(VersionType& major_version, StatusType& status) noexcept;

    /// Set iBeacon Minor version (4 hex characters)
    /// Equivalent to Ada's Set_iBeacon_Minor_Version procedure
    /// This command is added since V517 version.
    /// Default: 0xFFE1
    /// @param minor_version Minor version as 4 hex characters
    /// @param status Output parameter for operation result
    void set_ibeacon_minor_version(const VersionType& minor_version, StatusType& status) noexcept;

    /// Get iBeacon Minor version
    /// Equivalent to Ada's Get_iBeacon_Minor_Version procedure
    /// @param minor_version Output parameter for minor version
    /// @param status Output parameter for operation result
    void get_ibeacon_minor_version(VersionType& minor_version, StatusType& status) noexcept;

    /// Set iBeacon measured power (2 hex characters)
    /// Equivalent to Ada's Set_iBeacon_Measured_Power procedure
    /// This command is added since V519 version.
    /// Default: 0xC5
    /// @param measured_power Measured power as 2 hex characters
    /// @param status Output parameter for operation result
    void set_ibeacon_measured_power(const MeasuredPower& measured_power, StatusType& status) noexcept;

    /// Get iBeacon measured power
    /// Equivalent to Ada's Get_iBeacon_Measured_Power procedure
    /// @param measured_power Output parameter for measured power
    /// @param status Output parameter for operation result
    void get_ibeacon_measured_power(MeasuredPower& measured_power, StatusType& status) noexcept;

    /// Set iBeacon deploy mode
    /// Equivalent to Ada's Set_iBeacon_Deploy_Mode procedure
    /// After received OK, module will reset after 500ms.
    /// This command will let module into non-connectable status until next power on.
    /// @param deploy_mode Deploy mode (BroadcastScanning or OnlyBroadcast)
    /// @param status Output parameter for operation result
    void set_ibeacon_deploy_mode(IBeaconDeployMode deploy_mode, StatusType& status) noexcept;

    // ============================================================================
    // GATT Service Discovery Operations
    // ============================================================================

    /// Find all services UUID on connected device
    /// Equivalent to Ada's Find_All_Services_UUID procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used to
    /// find all services UUID on the slave device.
    ///
    /// Service format is [P1]:[P2]:[P3] where:
    ///   P1: 4 Bytes, Services start handle.
    ///   P2: 4 Bytes, Services end handle
    ///   P3: 4 Bytes, Services UUID
    ///
    /// This method implements streaming response parsing using explicit state machine.
    /// Callback is invoked for each discovered service.
    /// Uses soft-stop timeout semantics - completes partial parse on timeout.
    ///
    /// @param callback Function pointer for service UUID notifications
    /// @param user_data Optional user data pointer passed to callback (can be nullptr)
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void find_all_services_uuid(
        ServiceUuidCallback callback,
        void* user_data,
        uint32_t timeout_ms,
        StatusType& status) noexcept;

    // ============================================================================
    // GATT Characteristic Discovery Operations
    // ============================================================================

    /// Find all characteristics UUID on connected device
    /// Equivalent to Ada's Find_All_Characteristic_UUID procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used to
    /// find all characteristics UUID on the slave device.
    ///
    /// Characteristic format is [P1]:[P2]:[P3] where:
    ///   P1: 4 bytes, Characteristic handle.
    ///   P2: 14 bytes, "RD|WR|WN|NO|IN" properties string
    ///   P3: 4 bytes, Characteristic UUID
    ///
    /// Characteristic properties length 14 bytes, full string is
    /// "RD|WR|WN|NO|IN"; WR - Write; WN - Write_Without_Response;
    /// IN - Indicate; NO - Notify; RD - Read;
    /// Usually, a characteristic maybe only has one or two properties.
    /// Unsupported property part replaced by "--"
    ///
    /// This method implements streaming response parsing using explicit state machine.
    /// Callback is invoked for each discovered characteristic.
    /// Uses soft-stop timeout semantics - completes partial parse on timeout.
    ///
    /// @param callback Function pointer for characteristic UUID notifications
    /// @param user_data Optional user data pointer passed to callback (can be nullptr)
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void find_all_characteristics_uuid(
        CharacteristicUuidCallback callback,
        void* user_data,
        uint32_t timeout_ms,
        StatusType& status) noexcept;

    /// Find characteristics UUID within specific handle range on connected device
    /// Equivalent to Ada's Find_Characteristic_UUID procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used to
    /// find characteristics UUID on the slave device within the specified handle range.
    /// From and To value could get from Find_All_Services_UUID Command.
    ///
    /// @param from Starting handle for characteristic search
    /// @param to Ending handle for characteristic search
    /// @param callback Function pointer for characteristic UUID notifications
    /// @param user_data Optional user data pointer passed to callback (can be nullptr)
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void find_characteristics_uuid(
        const HandleType& from,
        const HandleType& to,
        CharacteristicUuidCallback callback,
        void* user_data,
        uint32_t timeout_ms,
        StatusType& status) noexcept;

    // ============================================================================
    // GATT Characteristic Operations
    // ============================================================================

    /// Enable notification on a characteristic
    /// Equivalent to Ada's Enable_Characteristic_Notify procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used
    /// to enable notify on a characteristic who owned notify property.
    ///
    /// @param handle Handle of the characteristic to enable notifications for
    /// @param status Output parameter for operation result
    /// @param response Output parameter for notification response
    void enable_characteristic_notify(const HandleType& handle, StatusType& status, NotifyResponse& response) noexcept;

    /// Disable notification on a characteristic
    /// Equivalent to Ada's Disable_Characteristic_Notify procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used
    /// to disable notify on a characteristic who owned notify property.
    ///
    /// @param handle Handle of the characteristic to disable notifications for
    /// @param status Output parameter for operation result
    /// @param response Output parameter for notification response
    void disable_characteristic_notify(const HandleType& handle, StatusType& status, NotifyResponse& response) noexcept;

    /// Read characteristic value
    /// Equivalent to Ada's Read_Characteristic_Notify procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used to
    /// read characteristic value who owned read property.
    ///
    /// @param handle Handle of the characteristic to read
    /// @param status Output parameter for operation result
    /// @param response Output parameter for notification response
    void read_characteristic_notify(const HandleType& handle, StatusType& status, NotifyResponse& response) noexcept;

    /// Set method and characteristic handle for data operations
    /// Equivalent to Ada's Set_Method_And_Characteristic_Handle procedure
    /// Required state: after connect; Required role: Central.
    /// Note: after execute this command, now you can start to send and receive
    /// data without any AT commands.
    /// Note: This command is different between Send_Data_To_Characteristic,
    /// this command only need be executed once more.
    ///
    /// @param handle Characteristic handle to use for data operations
    /// @param method Send data method (Write, WriteWithoutResponse, Indicate, Notify)
    /// @param status Output parameter for operation result
    void
    set_method_and_characteristic_handle(const HandleType& handle, SendDataMethod method, StatusType& status) noexcept;

    /// Send data to characteristic (deprecated in favor of set_method_and_characteristic_handle)
    /// Equivalent to Ada's Send_Data_To_Characteristic procedure
    /// Required state: after connect; Required role: Central.
    /// Only central role device can use this command. This command is used to
    /// send data to a characteristic who owned Write or Write-Without-Response property.
    /// Note: Since V701, We added Set_Method_And_Characteristic_Handle command,
    /// you can forget this command.
    ///
    /// @param handle Characteristic handle to send data to
    /// @param method Send data characteristic method (Write or WriteWithoutResponse)
    /// @param data Data payload to send
    /// @param status Output parameter for operation result
    void send_data_to_characteristic(
        const HandleType& handle,
        SendDataCharacteristic method,
        hal::UARTData8bConst data,
        StatusType& status) noexcept;

    /// Set service UUID
    /// Equivalent to Ada's Set_Service_UUID procedure
    /// Response format: "OK+Set:" + "0x" + uuid
    /// @param uuid Service UUID as 4 hex characters
    /// @param status Output parameter for operation result
    void SetServiceUuid(const UUID& uuid, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+UUID0x").append(uuid.view());

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append("0x").append(uuid.view());

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get service UUID
    /// Equivalent to Ada's Get_Service_UUID procedure
    /// Response format: "OK+Get:" + "0x" + uuid (total 6 bytes after OK+Get:)
    /// @param uuid Output parameter for service UUID
    /// @param status Output parameter for operation result
    void GetServiceUuid(UUID& uuid, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 6; // "0x" + 4 hex chars

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+UUID?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract UUID (skip "OK+Get:" and "0x", get 4 hex chars)
        // Ada uses offset +3 from OK+Get: which skips "0x" prefix
        const std::size_t UUID_OFFSET = OK_GET_PREFIX.size() + 2; // +2 for "0x"
        if (UUID_OFFSET + 4 > RESPONSE_STR.size()) {
            status = StatusType::ErrError;
            return;
        }

        std::string_view uuid_part = RESPONSE_STR.substr(UUID_OFFSET, 4);

        // Copy UUID value
        try {
            uuid = UUID(uuid_part);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set characteristic UUID count to use
    /// Equivalent to Ada's Set_Use_Characteristic_UUID_Count procedure
    /// Special case: Query value sends "?" instead of position number
    /// @param count Characteristic UUID count (Query, One, OneAndNext, OneAndPrev)
    /// @param status Output parameter for operation result
    void SetUseCharacteristicUuidCount(CharacteristicUuidCount count, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            util::ATCommandBuilder expected;

            if (count == CharacteristicUuidCount::Query) {
                // Special case: Query sends "?"
                cmd.append("AT+FFE2").append("?");
                expected.append("OK+Set:").append("?");
            } else {
                // Regular case: send position as character
                const auto COUNT_CHAR = static_cast<char>('0' + static_cast<uint8_t>(count));
                cmd.append("AT+FFE2").append(COUNT_CHAR);
                expected.append("OK+Set:").append(COUNT_CHAR);
            }

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set characteristic UUID
    /// Equivalent to Ada's Set_Characteristic procedure
    /// Response format: "OK+Set:" + "0x" + handle (total 6 bytes after OK+Set:)
    /// @param characteristic Characteristic UUID as 4 hex characters
    /// @param status Output parameter for operation result
    void SetCharacteristic(const CharacteristicType& characteristic, StatusType& status) noexcept {
        constexpr std::string_view OK_SET_PREFIX = "OK+Set:";
        const auto EXPECTED_LENGTH = OK_SET_PREFIX.size() + 6; // "0x" + 4 hex chars

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+CHAR0x").append(characteristic.view());

            // Send command
            Transmit(cmd.view(), status);
            if (status != StatusType::Ok) {
                return;
            }

            // Receive response
            hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
            uart_.Receive(response_data, status);
            if (status != StatusType::Ok) {
                return;
            }

            // Verify response starts with OK+Set:
            const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};
            if (!RESPONSE_STR.starts_with(OK_SET_PREFIX)) {
                status = StatusType::ErrError;
            }
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get characteristic UUID
    /// Equivalent to Ada's Get_Characteristic procedure
    /// Response format: "OK+Get:" + "0x" + handle (total 6 bytes after OK+Get:)
    /// @param characteristic Output parameter for characteristic UUID
    /// @param status Output parameter for operation result
    void GetCharacteristic(CharacteristicType& characteristic, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 6; // "0x" + 4 hex chars

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+CHAR?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract characteristic (skip "OK+Get:" and "0x", get 4 hex chars)
        // Ada uses offset +3 from OK+Get: which skips "0x" prefix
        const std::size_t CHAR_OFFSET = OK_GET_PREFIX.size() + 2; // +2 for "0x"
        if (CHAR_OFFSET + 4 > RESPONSE_STR.size()) {
            status = StatusType::ErrError;
            return;
        }

        std::string_view char_part = RESPONSE_STR.substr(CHAR_OFFSET, 4);

        // Copy characteristic value
        try {
            characteristic = CharacteristicType(char_part);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    // ============================================================================
    // Connection Management
    // ============================================================================

    /// Connect to last connected device
    /// Equivalent to Ada's Connect_Last_Device procedure
    /// Required: Set_Work_Type (Respond_AT_Command) and Set_Role (Central)
    /// and Set_Save_Connected_MAC (True).
    /// If remote device is not connectable, Connect_Fail result will be
    /// received after about 10 seconds.
    /// @param result Output parameter for connection result
    /// @param status Output parameter for operation result
    void connect_last_device(ConnectResult& result, StatusType& status) noexcept {
        result = ConnectResult::OtherError; // Initialize to error state

        // Expected response: "OK+CONN" + single result character
        constexpr std::string_view expected_prefix = "OK+CONN";
        const auto expected_length = expected_prefix.size() + 1; // +1 for result character

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send "AT+CONNL" command and expect "OK+CONN" + result character
        TransmitAndCheck(
            "AT+CONNL",
            expected_prefix,
            hal::UARTData8b{response_buffer_.data(), expected_length},
            status);

        if (status != StatusType::Ok) {
            return;
        }

        // Parse result character
        const char result_char = static_cast<char>(response_buffer_[expected_prefix.size()]);
        const auto parsed_result = util::parse_connect_result(result_char);
        if (parsed_result.has_value()) {
            result = *parsed_result;
        } else {
            result = ConnectResult::OtherError;
            status = StatusType::ErrError;
        }
    }

    /// Connect to device by MAC address and type
    /// Equivalent to Ada's Connect procedure with MAC_Type and Address parameters
    /// Required: Set_Work_Type (Respond_AT_Command) and Set_Role (Central)
    /// @param mac_type Type of MAC address (Normal, Static, etc.)
    /// @param address MAC address to connect to
    /// @param result Output parameter for connection result
    /// @param status Output parameter for operation result
    void
    connect(MacAddressType mac_type, const MacAddress& address, ConnectResult& result, StatusType& status) noexcept {
        result = ConnectResult::OtherError; // Initialize to error state

        try {
            const auto mac_type_str = util::to_string(mac_type);

            // Build command: "AT+CO" + type + address
            util::ATCommandBuilder cmd;
            cmd.append("AT+CO").append(mac_type_str).append(std::string_view(address));

            // Build expected response: "OK+CO" + type + type
            util::ATCommandBuilder expected;
            expected.append("OK+CO").append(mac_type_str).append(mac_type_str);

            const auto expected_length = expected.view().size() + 1; // +1 for result character

            if (expected_length > response_buffer_.size()) {
                status = StatusType::ErrError;
                return;
            }

            // Send command and receive response
            TransmitAndCheck(
                cmd.view(),
                expected.view(),
                hal::UARTData8b{response_buffer_.data(), expected_length},
                status);

            if (status != StatusType::Ok) {
                return;
            }

            // Parse result character
            const char result_char = static_cast<char>(response_buffer_[expected.view().size()]);
            const auto parsed_result = util::parse_connect_result(result_char);
            if (parsed_result.has_value()) {
                result = *parsed_result;
            } else {
                result = ConnectResult::OtherError;
                status = StatusType::ErrError;
            }
        } catch (...) {
            result = ConnectResult::OtherError;
            status = StatusType::ErrError;
        }
    }

    /// Connect to discovered device by index
    /// Equivalent to Ada's Connect procedure with Index parameter
    /// This command is used after executing Scan
    /// This command will clear all discovered devices list.
    /// The first discovered device array index is 0, second device array
    /// index is 1. Scan command could return more than 6 devices, but only
    /// first 6 devices could use array index, other devices must use AT+CO or
    /// AT+LN command.
    /// @param index Index of discovered device (0-5)
    /// @param result Output parameter for connection result
    /// @param status Output parameter for operation result
    void connect(DiscoveredIndex index, ConnectResult& result, StatusType& status) noexcept {
        result = ConnectResult::OtherError; // Initialize to error state

        try {
            // Build command: "AT+CONN" + index
            util::ATCommandBuilder cmd;
            cmd.append("AT+CONN").append_int(static_cast<uint8_t>(index.value()));

            // Expected response: "OK+CONN" + result character
            constexpr std::string_view expected_prefix = "OK+CONN";
            const auto expected_length = expected_prefix.size() + 1; // +1 for result character

            if (expected_length > response_buffer_.size()) {
                status = StatusType::ErrError;
                return;
            }

            // Send command and receive response
            TransmitAndCheck(
                cmd.view(),
                expected_prefix,
                hal::UARTData8b{response_buffer_.data(), expected_length},
                status);

            if (status != StatusType::Ok) {
                return;
            }

            // Parse result character
            const char result_char = static_cast<char>(response_buffer_[expected_prefix.size()]);
            const auto parsed_result = util::parse_connect_result(result_char);
            if (parsed_result.has_value()) {
                result = *parsed_result;
            } else {
                result = ConnectResult::OtherError;
                status = StatusType::ErrError;
            }
        } catch (...) {
            result = ConnectResult::OtherError;
            status = StatusType::ErrError;
        }
    }

    /// Disconnect from currently connected device
    /// Equivalent to Ada's Disconnect procedure
    /// Sends "AT" command and expects "OK+LOST" response
    /// @param status Output parameter for operation result
    void disconnect(StatusType& status) noexcept { TransmitAndCheck("AT", WHEN_DISCONNECTED_MESSAGE, status); }

    /// Get last connected device MAC address
    /// Equivalent to Ada's Get_Last_Connected_Device_Address procedure
    /// @param mac Output parameter for MAC address (12 hex characters)
    /// @param status Output parameter for operation result
    void GetLastConnectedDeviceAddress(MacAddress& mac, StatusType& status) noexcept {
        constexpr std::string_view OK_RADD_PREFIX = "OK+RADD:";
        constexpr std::size_t MAC_LENGTH = 12;
        const auto EXPECTED_LENGTH = OK_RADD_PREFIX.size() + MAC_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+RADD?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_RADD_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract MAC address
        std::string_view mac_part = RESPONSE_STR.substr(OK_RADD_PREFIX.size());
        if (mac_part.size() != MAC_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy MAC address
        try {
            mac = MacAddress(mac_part);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set whether to save connected device MAC address
    /// Equivalent to Ada's Set_Save_Connected_MAC procedure
    /// NOTE: Ada uses inverted logic - "not Save" is sent to module
    /// When enabled (true), module saves last connected MAC for use with Connect_Last_Device
    /// @param enable True to save connected MAC, false to not save
    /// @param status Output parameter for operation result
    void SetSaveConnectedMac(bool enable, StatusType& status) noexcept {
        try {
            // Ada inverts the logic: Image(not Save)
            // So enable=true sends '0', enable=false sends '1'
            const char SAVE_CHAR = enable ? '0' : '1';

            util::ATCommandBuilder cmd;
            cmd.append("AT+SAVE").append(SAVE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(SAVE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get whether module saves connected device MAC address
    /// Equivalent to Ada's Get_Save_Connected_MAC procedure
    /// NOTE: Ada uses inverted logic - response '0' means enabled
    /// @param enabled Output parameter for save MAC status
    /// @param status Output parameter for operation result
    void GetSaveConnectedMac(bool& enabled, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+SAVE?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Ada logic: Save := S = '0' (inverted logic)
        const char SAVE_CHAR = PARSED_RESPONSE->front();
        enabled = (SAVE_CHAR == '0');
    }

    /// Set timeout for connecting to remote device
    /// Equivalent to Ada's Set_Connect_Remote_Device_Timeout procedure
    /// @param timeout Connection timeout as 6-digit string (in milliseconds)
    /// @param status Output parameter for operation result
    void SetConnectRemoteDeviceTimeout(const ConnectTimeout& timeout, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+TCON").append(timeout.view());

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(timeout.view());

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get timeout for connecting to remote device
    /// Equivalent to Ada's Get_Connect_Remote_Device_Timeout procedure
    /// @param timeout Output parameter for connection timeout (6-digit string)
    /// @param status Output parameter for operation result
    void GetConnectRemoteDeviceTimeout(ConnectTimeout& timeout, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t TIMEOUT_LENGTH = 6;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + TIMEOUT_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+TCON?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->size() != TIMEOUT_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy timeout value
        try {
            timeout = ConnectTimeout(*PARSED_RESPONSE);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    // ============================================================================
    // HMSensor-Specific Features
    // ============================================================================

    /// Set sensor type for HMSensor module
    /// Equivalent to Ada's Set_Sensor_Type procedure
    /// This command is only use for HMSensor
    /// Sensor type on module PIO11 (PIO3 on HM-11)
    /// Default: None
    /// @param sensor_type Sensor type to configure (None, DHT11, DS18B20)
    /// @param status Output parameter for operation result
    void set_sensor_type(SensorType sensor_type, StatusType& status) noexcept {
        // Construct command: "AT+SENS" + sensor_type_value
        const auto sensor_str = util::to_string(sensor_type);

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+SENS").append(sensor_str);

            // Construct expected response: "OK+Set:" + sensor_type_value
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(sensor_str);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get current sensor type from HMSensor module
    /// Equivalent to Ada's Get_Sensor_Type procedure
    /// @param sensor_type Output parameter for current sensor type
    /// @param status Output parameter for operation result
    void get_sensor_type(SensorType& sensor_type, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for sensor type character

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+SENS?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_sensor_type = util::parse_sensor_type(parsed_response->front());
        if (!parsed_sensor_type.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        sensor_type = *parsed_sensor_type;
    }

    /// Set sensor work interval for HMSensor module
    /// Equivalent to Ada's Set_Sensor_Work_Interval procedure
    /// Default: 0, Unit: minute
    /// Note: This command is only used for HMSensor
    /// @param interval Work interval as 2-digit string (00-99 minutes)
    /// @param status Output parameter for operation result
    void set_sensor_work_interval(const WorkInterval& interval, StatusType& status) noexcept {
        // Ada logic: if first character is '0', send only the last character
        // Otherwise send the full interval string
        const std::string_view interval_str(interval);

        try {
            if (interval_str.size() >= 2 && interval_str[0] == '0') {
                // Send only the last character for intervals like "01" -> "1"
                const char last_char = interval_str[1];
                util::ATCommandBuilder cmd;
                cmd.append("AT+RAT").append(last_char);

                util::ATCommandBuilder expected;
                expected.append("OK+Set:").append(last_char);

                TransmitAndCheck(cmd.view(), expected.view(), status);
            } else {
                // Send full interval string for intervals like "10", "25", etc.
                util::ATCommandBuilder cmd;
                cmd.append("AT+RAT").append(interval_str);

                util::ATCommandBuilder expected;
                expected.append("OK+Set:").append(interval_str);

                TransmitAndCheck(cmd.view(), expected.view(), status);
            }
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get sensor work interval from HMSensor module
    /// Equivalent to Ada's Get_Sensor_Work_Interval procedure
    /// @param interval Output parameter for work interval (2-digit string)
    /// @param status Output parameter for operation result
    void get_sensor_work_interval(WorkInterval& interval, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t MAX_RESPONSE_LENGTH = 16; // Allow for variable length

        if (MAX_RESPONSE_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+RAT??", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Initialize buffer with zeros to ensure proper null termination detection
        std::fill(response_buffer_.begin(), response_buffer_.begin() + MAX_RESPONSE_LENGTH, 0);

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), MAX_RESPONSE_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Find actual response length (until null terminator or buffer end)
        std::size_t actual_length = 0;
        for (std::size_t i = 0; i < MAX_RESPONSE_LENGTH; ++i) {
            if (response_buffer_[i] == 0) {
                actual_length = i;
                break;
            }
            if (response_buffer_[i] < 32 || response_buffer_[i] > 126) {
                // Non-printable character - likely uninitialized
                actual_length = i;
                break;
            }
        }
        if (actual_length == 0) {
            actual_length = MAX_RESPONSE_LENGTH;
        }

        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), actual_length};

        if (!util::starts_with(response_str, OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        const auto data_part = response_str.substr(OK_GET_PREFIX.size());

        // Handle empty or error responses
        if (data_part.empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Extract numeric characters only
        std::string numeric_chars;
        for (char c : data_part) {
            if (c >= '0' && c <= '9') {
                numeric_chars += c;
            } else if (c == 0 || c < 32 || c > 126) {
                // Null terminator or non-printable character - stop parsing
                break;
            }
            // Skip other non-numeric characters but don't treat as error
        }

        // Validate we have 1-2 digits
        if (numeric_chars.empty() || numeric_chars.size() > 2) {
            status = StatusType::ErrError;
            return;
        }

        // Pad single digit with leading zero
        std::string interval_str;
        if (numeric_chars.size() == 1) {
            interval_str = "0" + numeric_chars;
        } else {
            interval_str = numeric_chars;
        }

        // Validate the interval string contains only valid characters
        for (char c : interval_str) {
            if (c < '0' || c > '9') {
                status = StatusType::ErrError;
                return;
            }
        }

        // Create the WorkInterval safely
        try {
            interval = WorkInterval(interval_str);
        } catch (...) {
            status = StatusType::ErrError;
            return;
        }
    }

    /// Get sensor temperature and humidity readings
    /// Equivalent to Ada's Get_Sensor_Temperature_And_Humidity procedure
    /// Note: This command is only use for HMSensor version and has a sensor
    /// This value is added into scan response data package.
    /// Data format is 0x02, 0x16, 0x00, 0xB0, [reserved], [temperature],
    /// [humidity], [battery].
    /// Android:
    ///   Included in OnLeScan function result array, you can see it directly.
    /// iOS:
    ///   Included in LeScan function result NSDictionary struct, service id
    ///   is 0xB000.
    /// @param temperature Output parameter for temperature (0-120 range)
    /// @param humidity Output parameter for humidity (0-100 range)
    /// @param status Output parameter for operation result
    void get_sensor_temperature_and_humidity(
        TemperatureType& temperature,
        HumidityType& humidity,
        StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t MAX_EXPECTED_LENGTH = OK_GET_PREFIX.size() + 20; // Allow for decimal format

        if (MAX_EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+TEHU?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response with extended length to accommodate decimals
        hal::UARTData8b response_data{response_buffer_.data(), MAX_EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Find actual response length (MockUART copies min of request and available data)
        std::size_t actual_length = MAX_EXPECTED_LENGTH;
        for (std::size_t i = OK_GET_PREFIX.size(); i < MAX_EXPECTED_LENGTH; ++i) {
            if (response_buffer_[i] == 0) {
                actual_length = i;
                break;
            }
        }

        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), actual_length};

        if (!util::starts_with(response_str, OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract temperature and humidity parts
        const auto data_part = response_str.substr(OK_GET_PREFIX.size());

        if (data_part.empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse temperature and humidity from data_part
        std::string temp_str;
        std::string hum_str;

        if (data_part.size() == 6) {
            // Standard format: "TTTHH" - validate all characters are digits
            temp_str = data_part.substr(0, 3);
            hum_str = data_part.substr(3, 3);

            // Validate temperature digits
            for (char c : temp_str) {
                if (c < '0' || c > '9') {
                    status = StatusType::ErrError;
                    return;
                }
            }

            // Validate humidity digits
            for (char c : hum_str) {
                if (c < '0' || c > '9') {
                    status = StatusType::ErrError;
                    return;
                }
            }
        } else {
            // Try decimal format: "TTT.XHHH.Y" (positions 0-2 for temp, 5-7 for humidity)
            if (data_part.size() >= 8) {
                // Extract temperature (positions 0-2)
                bool valid = true;
                for (std::size_t i = 0; i < 3 && i < data_part.size(); i++) {
                    char c = data_part[i];
                    if (c >= '0' && c <= '9') {
                        temp_str += c;
                    } else {
                        valid = false;
                        break;
                    }
                }

                // Extract humidity (positions 5-7)
                if (valid && data_part.size() > 7) {
                    for (std::size_t i = 5; i < 8 && i < data_part.size(); i++) {
                        char c = data_part[i];
                        if (c >= '0' && c <= '9') {
                            hum_str += c;
                        } else {
                            valid = false;
                            break;
                        }
                    }
                }

                if (!valid || temp_str.size() != 3 || hum_str.size() != 3) {
                    status = StatusType::ErrError;
                    return;
                }
            } else {
                status = StatusType::ErrError;
                return;
            }
        }

        // Parse temperature
        const auto temp_result = util::parse_uint<uint8_t>(temp_str, 10);
        if (!temp_result.has_value() || temp_result.value > 120) {
            status = StatusType::ErrError;
            return;
        }

        // Parse humidity
        const auto hum_result = util::parse_uint<uint8_t>(hum_str, 10);
        if (!hum_result.has_value() || hum_result.value > 100) {
            status = StatusType::ErrError;
            return;
        }

        // Create typed values with range validation
        try {
            temperature = TemperatureType(temp_result.value);
            humidity = HumidityType(hum_result.value);
        } catch (...) {
            status = StatusType::ErrError;
            return;
        }
    }

    /// Get module internal temperature (HMSensor only)
    /// Equivalent to Ada's Get_Module_Temperature procedure
    /// Returns 7-character temperature string (e.g., "+025.50")
    /// Note: HMSensor version only
    /// @param temperature Output parameter for internal temperature (7 chars)
    /// @param status Output parameter for operation result
    void GetModuleTemperature(InternalTemperatureType& temperature, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t TEMP_LENGTH = 7;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + TEMP_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+TEMP?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->size() != TEMP_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy temperature value (7 characters, e.g., "+025.50")
        try {
            temperature = InternalTemperatureType(*PARSED_RESPONSE);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get the number of devices discovered during the last scan
    /// @return Number of discovered devices (0-6)
    [[nodiscard]] std::size_t get_discovered_device_count() const noexcept { return discovered_device_count_; }

    /// Get information about a discovered device by index
    /// @param index Index of the device to query (must be < get_discovered_device_count())
    /// @return Const reference to discovered device info, or std::nullopt if index invalid
    [[nodiscard]] std::optional<std::reference_wrapper<const DiscoveredInfo>> get_discovered_device(
        std::size_t index) const noexcept {
        if (index >= discovered_device_count_) {
            return std::nullopt;
        }
        return std::cref(discovered_devices_[index]);
    }

    // ============================================================================
    // Power Management Configuration
    // ============================================================================

    /// Set module power level
    /// Equivalent to Ada's Set_Module_Power procedure
    /// Default: Dbm_0
    /// @param power Module power level (DbmMinus23, DbmMinus6, Dbm0, Dbm6)
    /// @param status Output parameter for operation result
    void set_module_power(ModulePower power, StatusType& status) noexcept {
        try {
            const auto power_str = util::to_string(power);
            util::ATCommandBuilder cmd;
            cmd.append("AT+POWE").append(power_str);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(power_str);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get module power level
    /// Equivalent to Ada's Get_Module_Power procedure
    /// @param power Output parameter for module power level
    /// @param status Output parameter for operation result
    void get_module_power(ModulePower& power, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1;

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+POWE?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_power = util::parse_module_power(parsed_response->front());
        if (!parsed_power.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        power = *parsed_power;
    }

    /// Set output power level
    /// Equivalent to Ada's Set_Output_Power procedure
    /// Added in V527, Default: Max_Power
    /// @param power Output power level (NormalPower, MaxPower)
    /// @param status Output parameter for operation result
    void set_output_power(OutputPower power, StatusType& status) noexcept {
        try {
            const auto power_str = util::to_string(power);
            util::ATCommandBuilder cmd;
            cmd.append("AT+POWR").append(power_str);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(power_str);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get output power level
    /// Equivalent to Ada's Get_Output_Power procedure
    /// @param power Output parameter for output power level
    /// @param status Output parameter for operation result
    void get_output_power(OutputPower& power, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1;

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+POWR?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_power = util::parse_output_power(parsed_response->front());
        if (!parsed_power.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        power = *parsed_power;
    }

    /// Set module auto-sleep mode
    /// Equivalent to Ada's Set_Module_Auto_Sleep procedure
    /// Only Peripheral role is used. Default: False
    /// @param sleep True to enable auto-sleep, false to disable
    /// @param status Output parameter for operation result
    void set_module_auto_sleep(bool sleep, StatusType& status) noexcept {
        try {
            const auto sleep_str = util::to_string(sleep);
            util::ATCommandBuilder cmd;
            cmd.append("AT+PWRM").append(sleep_str);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(sleep_str);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get module auto-sleep mode
    /// Equivalent to Ada's Get_Module_Auto_Sleep procedure
    /// Only Peripheral role is used.
    /// @param sleep Output parameter for auto-sleep status
    /// @param status Output parameter for operation result
    void get_module_auto_sleep(bool& sleep, StatusType& status) noexcept;

    /// Enter sleep mode
    /// Equivalent to Ada's Sleep procedure
    /// Only support Peripheral role.
    /// @param status Output parameter for operation result
    void sleep(StatusType& status) noexcept { TransmitAndCheck("AT+SLEEP", "OK+SLEEP", status); }

    /// Wake up from sleep mode
    /// Equivalent to Ada's Wake_Up procedure
    /// @param status Output parameter for operation result
    void wake_up(StatusType& status) noexcept { TransmitAndCheck("AT", "OK", status); }

    // ============================================================================
    // Advertising Configuration
    // ============================================================================

    /// Set advertising interval
    /// Equivalent to Ada's Set_Advertising_Interval procedure
    /// Added since V517 version. V522 version added max value F.
    /// The maximum 1285ms recommendations from the IOS system.
    /// @param interval Advertising interval value
    /// @param status Output parameter for operation result
    void set_advertising_interval(AdvertisingInterval interval, StatusType& status) noexcept {
        try {
            const auto interval_char = util::to_char(interval);
            util::ATCommandBuilder cmd;
            cmd.append("AT+ADVI").append(interval_char);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(interval_char);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get advertising interval
    /// Equivalent to Ada's Get_Advertising_Interval procedure
    /// @param interval Output parameter for advertising interval
    /// @param status Output parameter for operation result
    void get_advertising_interval(AdvertisingInterval& interval, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1;

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+ADVI?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_interval = util::parse_advertising_interval(parsed_response->front());
        if (!parsed_interval.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        interval = *parsed_interval;
    }

    /// Set advertising type
    /// Equivalent to Ada's Set_Advertising_Type procedure
    /// Added since V519
    /// @param type Advertising type value
    /// @param status Output parameter for operation result
    void set_advertising_type(AdvertisingType type, StatusType& status) noexcept {
        try {
            const auto type_str = util::to_string(type);
            util::ATCommandBuilder cmd;
            cmd.append("AT+ADTY").append(type_str);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(type_str);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get advertising type
    /// Equivalent to Ada's Get_Advertising_Type procedure
    /// @param type Output parameter for advertising type
    /// @param status Output parameter for operation result
    void get_advertising_type(AdvertisingType& type, StatusType& status) noexcept;

    /// Set advertising FLAG
    /// Equivalent to Ada's Set_Advertising_FLAG procedure
    /// This command added in V530.
    /// This command is used to set flag information byte in advertising package.
    /// @param flag Advertising flag as 2 hex characters (0x00~0xFF)
    /// @param status Output parameter for operation result
    void set_advertising_flag(const AdvertisingFlag& flag, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+FLAG").append(std::string_view(flag));
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(std::string_view(flag));
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set temperature information byte in advertising package
    /// Equivalent to Ada's Set_Temperature_Information_Byte_In_Advertising procedure
    /// Response format: "OK+Set:" + "0x" + temperature
    /// @param temperature Temperature value as 2 hex characters
    /// @param status Output parameter for operation result
    void SetTemperatureInformationByteInAdvertising(const AdvertisingTemperatureType& temperature, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+TEMP0x").append(temperature.view());

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append("0x").append(temperature.view());

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set humidity information byte in advertising package
    /// Equivalent to Ada's Set_Humi_Information_Byte_In_Advertising procedure
    /// @param humidity Humidity value as 2 hex characters
    /// @param status Output parameter for operation result
    void SetHumiInformationByteInAdvertising(const HumiInformation& humidity, StatusType& status) noexcept {
        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+HUMI").append(humidity.view());

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(humidity.view());

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set reliable advertising mode
    /// Equivalent to Ada's Set_Reliable_Advertising_Mode procedure
    /// This command is added since V530, Default: Normal_Advertising
    /// This command is used to make sure module always send advertising
    /// package when module is in long time standby mode.
    /// @param mode Reliable advertising mode
    /// @param status Output parameter for operation result
    void set_reliable_advertising_mode(AdvertisingMode mode, StatusType& status) noexcept;

    /// Get reliable advertising mode
    /// Equivalent to Ada's Get_Reliable_Advertising_Mode procedure
    /// @param mode Output parameter for reliable advertising mode
    /// @param status Output parameter for operation result
    void get_reliable_advertising_mode(AdvertisingMode& mode, StatusType& status) noexcept;

    /// Set module advertisement data
    /// Equivalent to Ada's Set_Module_Advertisement_Data procedure
    /// Added since V607/V702
    /// @param data Advertisement data as 12 hex characters
    /// @param status Output parameter for operation result
    void set_module_advertisement_data(const AdvertisementData& data, StatusType& status) noexcept;

    // ============================================================================
    // UART Configuration
    // ============================================================================

    /// Set UART baud rate
    /// Equivalent to Ada's Set_UART_Baud_Rate procedure
    /// Added since V518. Default: 4 (9600), setting 8 (57600)
    /// is recommended for high throughput applications.
    /// Note: When setting 8 (57600) or 9 (115200), please make sure the power supply is enough.
    /// @param rate UART baud rate
    /// @param status Output parameter for operation result
    void set_uart_baud_rate(UartBaudRate rate, StatusType& status) noexcept {
        try {
            const auto rate_str = util::to_string(rate);
            util::ATCommandBuilder cmd;
            cmd.append("AT+BAUD").append(rate_str);
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(rate_str);
            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get UART baud rate
    /// Equivalent to Ada's Get_UART_Baud_Rate procedure
    /// @param rate Output parameter for UART baud rate
    /// @param status Output parameter for operation result
    void get_uart_baud_rate(UartBaudRate& rate, StatusType& status) noexcept;

    /// Set parity bit configuration
    /// Equivalent to Ada's Set_Parity_Bit procedure
    /// Default: None
    /// @param parity Parity bit setting (None, Odd, Even)
    /// @param status Output parameter for operation result
    void set_parity_bit(ParityBit parity, StatusType& status) noexcept;

    /// Get parity bit configuration
    /// Equivalent to Ada's Get_Parity_Bit procedure
    /// @param parity Output parameter for parity bit setting
    /// @param status Output parameter for operation result
    void get_parity_bit(ParityBit& parity, StatusType& status) noexcept;

    /// Set stop bit configuration
    /// Equivalent to Ada's Set_Stop_Bit procedure
    /// Default: One_Stop_Bit
    /// @param value Stop bit setting (OneStopBit, TwoStopBit)
    /// @param status Output parameter for operation result
    void set_stop_bit(StopBit value, StatusType& status) noexcept;

    /// Get stop bit configuration
    /// Equivalent to Ada's Get_Stop_Bit procedure
    /// @param result Output parameter for stop bit setting
    /// @param status Output parameter for operation result
    void get_stop_bit(StopBit& result, StatusType& status) noexcept;

    /// Set UART flow control switch
    /// Equivalent to Ada's Set_UART_Flow_Control_Switch procedure
    /// Default: False
    /// @param switch_enabled True to enable flow control, false to disable
    /// @param status Output parameter for operation result
    void set_uart_flow_control_switch(bool switch_enabled, StatusType& status) noexcept;

    /// Get UART flow control switch
    /// Equivalent to Ada's Get_UART_Flow_Control_Switch procedure
    /// @param switch_enabled Output parameter for flow control switch status
    /// @param status Output parameter for operation result
    void get_uart_flow_control_switch(bool& switch_enabled, StatusType& status) noexcept;

    /// Set UART sleep type (HMSensor only)
    /// Equivalent to Ada's Set_UART_Sleep_Type procedure
    /// This command is only for HMSensor version
    /// @param sleep_type UART sleep type (CanWakeUpThroughUart or ShutdownUart)
    /// @param status Output parameter for operation result
    void SetUartSleepType(UartSleepType sleep_type, StatusType& status) noexcept {
        try {
            const auto TYPE_CHAR = static_cast<char>('0' + static_cast<uint8_t>(sleep_type));

            util::ATCommandBuilder cmd;
            cmd.append("AT+UART").append(TYPE_CHAR);

            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append(TYPE_CHAR);

            TransmitAndCheck(cmd.view(), expected.view(), status);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get UART sleep type (HMSensor only)
    /// Equivalent to Ada's Get_UART_Sleep_Type procedure
    /// @param sleep_type Output parameter for UART sleep type
    /// @param status Output parameter for operation result
    void GetUartSleepType(UartSleepType& sleep_type, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + 1;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+UART?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        const auto PARSED_RESPONSE = util::extract_get_response(RESPONSE_STR);
        if (!PARSED_RESPONSE.has_value() || PARSED_RESPONSE->empty()) {
            status = StatusType::ErrError;
            return;
        }

        // Parse type character ('0' or '1')
        const char TYPE_CHAR = PARSED_RESPONSE->front();
        if (TYPE_CHAR < '0' || TYPE_CHAR > '1') {
            status = StatusType::ErrError;
            return;
        }

        sleep_type = static_cast<UartSleepType>(TYPE_CHAR - '0');
    }

    // ============================================================================
    // Connection Parameter Management
    // ============================================================================

    /// Set minimum link layer connection interval
    /// Equivalent to Ada's Set_Minimum_Link_Layer_Connection_Interval procedure
    /// This command is added since V538, Default: 20ms
    /// @param value Minimum connection interval
    /// @param status Output parameter for operation result
    void set_minimum_link_layer_connection_interval(LinkLayerConnectionInterval value, StatusType& status) noexcept;

    /// Get minimum link layer connection interval
    /// Equivalent to Ada's Get_Minimum_Link_Layer_Connection_Interval procedure
    /// @param result Output parameter for minimum connection interval
    /// @param status Output parameter for operation result
    void get_minimum_link_layer_connection_interval(LinkLayerConnectionInterval& result, StatusType& status) noexcept;

    /// Set maximum link layer connection interval
    /// Equivalent to Ada's Set_Maximum_Link_Layer_Connection_Interval procedure
    /// This command is added since V538, Default: 40ms
    /// @param value Maximum connection interval
    /// @param status Output parameter for operation result
    void set_maximum_link_layer_connection_interval(LinkLayerConnectionInterval value, StatusType& status) noexcept;

    /// Get maximum link layer connection interval
    /// Equivalent to Ada's Get_Maximum_Link_Layer_Connection_Interval procedure
    /// @param result Output parameter for maximum connection interval
    /// @param status Output parameter for operation result
    void get_maximum_link_layer_connection_interval(LinkLayerConnectionInterval& result, StatusType& status) noexcept;

    /// Set link layer connection slave latency
    /// Equivalent to Ada's Set_Link_Layer_Connection_Slave_Latency procedure
    /// This command is added since V538, Default: 0
    /// @param value Connection slave latency (0-4)
    /// @param status Output parameter for operation result
    void set_link_layer_connection_slave_latency(LayerConnectionLatency value, StatusType& status) noexcept;

    /// Get link layer connection slave latency
    /// Equivalent to Ada's Get_Link_Layer_Connection_Slave_Latency procedure
    /// @param result Output parameter for connection slave latency
    /// @param status Output parameter for operation result
    void get_link_layer_connection_slave_latency(LayerConnectionLatency& result, StatusType& status) noexcept;

    /// Set connection supervision timeout
    /// Equivalent to Ada's Set_Connection_Supervision_Timeout procedure
    /// This command is added since V538, Default: 6000ms
    /// @param value Connection supervision timeout
    /// @param status Output parameter for operation result
    void set_connection_supervision_timeout(ConnectionSupervisionTimeout value, StatusType& status) noexcept;

    /// Get connection supervision timeout
    /// Equivalent to Ada's Get_Connection_Supervision_Timeout procedure
    /// @param result Output parameter for connection supervision timeout
    /// @param status Output parameter for operation result
    void get_connection_supervision_timeout(ConnectionSupervisionTimeout& result, StatusType& status) noexcept;

    /// Set update connection setting
    /// Equivalent to Ada's Set_Update_Connection procedure
    /// This command is added since V538, Only Peripheral role is used, Default: True
    /// This command is only use when module is in slave role.
    /// @param value True to enable connection updates, false to disable
    /// @param status Output parameter for operation result
    void set_update_connection(bool value, StatusType& status) noexcept;

    /// Get update connection setting
    /// Equivalent to Ada's Get_Update_Connection procedure
    /// Only Peripheral role is used.
    /// @param result Output parameter for update connection setting
    /// @param status Output parameter for operation result
    void get_update_connection(bool& result, StatusType& status) noexcept;

    // ============================================================================
    // White List Management
    // ============================================================================

    /// Set white list switch
    /// Equivalent to Ada's Set_White_List_Switch procedure
    /// This command added in V523.
    /// White List allows three MAC address link to module. Please use
    /// Set_White_List_MAC_Addresses command set white list MAC address.
    /// @param value True to enable white list, false to disable
    /// @param status Output parameter for operation result
    void set_white_list_switch(bool value, StatusType& status) noexcept;

    /// Get white list switch
    /// Equivalent to Ada's Get_White_List_Switch procedure
    /// @param result Output parameter for white list switch status
    /// @param status Output parameter for operation result
    void get_white_list_switch(bool& result, StatusType& status) noexcept;

    /// Set white list MAC addresses
    /// Equivalent to Ada's Set_White_List_MAC_Addresses procedure
    /// This command added in V523.
    /// White List allows three MAC address link to module.
    /// @param index White list index (1-3)
    /// @param value MAC address to set
    /// @param status Output parameter for operation result
    void set_white_list_mac_addresses(MacWhiteListIndex index, const MacAddress& value, StatusType& status) noexcept;

    /// Get white list MAC address
    /// Equivalent to Ada's Get_White_List_MAC_Address procedure
    /// @param index White list index (1-3)
    /// @param result Output parameter for MAC address
    /// @param status Output parameter for operation result
    void get_white_list_mac_address(MacWhiteListIndex index, MacAddress& result, StatusType& status) noexcept;

    // ============================================================================
    // PIO Control
    // ============================================================================

    /// Set PIO output status
    /// Equivalent to Ada's Set_PIO_Output_Status procedure
    /// Required Work_Mode: Transmission_Mode or Remote_Control_Mode.
    /// Note: HM-11 only have PIO0, PIO1, PIO2, PIO3.
    /// PIO0 has system KEY function, PIO1 - System LED
    /// @param pio PIO pin number (1-3)
    /// @param output PIO output level (Low, High)
    /// @param status Output parameter for operation result
    void set_pio_output_status(PioNumber pio, PioOutput output, StatusType& status) noexcept;

    /// Get PIO output status
    /// Equivalent to Ada's Get_PIO_Output_Status procedure
    /// @param pio PIO pin number (1-3)
    /// @param output Output parameter for PIO output level
    /// @param status Output parameter for operation result
    void get_pio_output_status(PioNumber pio, PioOutput& output, StatusType& status) noexcept;

    /// Set multiple PIOs output status
    /// Equivalent to Ada's Set_PIOs_Output_Status procedure
    /// This command added since V551
    /// This command is used to control multiple PIO pins output HIGH or LOW.
    /// PIOs is Hex format, max value 3FF changed to binary format is
    /// 001111111111, Total length is 12 bit, left side to right bit mapped
    /// to module PIO0 to PIOB, Note: HM-11 only have PIO0, PIO1, PIO2, PIO3
    /// 1: is output HIGH; 0: is output LOW.
    /// @param pios PIO numbers configuration as 3 hex characters
    /// @param status Output parameter for operation result
    void set_pios_output_status(const PioNumbers& pios, StatusType& status) noexcept;

    /// Get multiple PIOs output status
    /// Equivalent to Ada's Get_PIOs_Output_Status procedure
    /// @param pios Output parameter for PIO numbers configuration
    /// @param status Output parameter for operation result
    void get_pios_output_status(PioNumbers& pios, StatusType& status) noexcept;

    /// Set PIO collection rate
    /// Equivalent to Ada's Set_PIO_Collection_Rate procedure
    /// This command is added since V515 version, Default: 10 seconds
    /// Required Work Mode = PIO_Collection_Mode,
    /// when PIO state is change, module will send OK+Col:[xx]
    /// to UART or remote side. This command is set send interval.
    /// @param value Collection rate in seconds (0-99)
    /// @param status Output parameter for operation result
    void set_pio_collection_rate(PioCollectionRate value, StatusType& status) noexcept;

    /// Get PIO collection rate
    /// Equivalent to Ada's Get_PIO_Collection_Rate procedure
    /// @param result Output parameter for collection rate
    /// @param status Output parameter for operation result
    void get_pio_collection_rate(PioCollectionRate& result, StatusType& status) noexcept;

    /// Set PIO pins output state after power supplied
    /// Equivalent to Ada's Set_Power_Pin_Output procedure
    /// Set pins state (high/low) after power supplied
    /// NOTE: PIO0 and PIO1 are used by system, so must be 0
    /// Only PIO2~PIOB pins can be configured (HM-11: only PIO2~PIO3)
    /// @param pios PIO numbers configuration as 3 hex characters
    /// @param status Output parameter for operation result
    void SetPowerPinOutput(const PioNumbers& pios, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t PIOS_LENGTH = 3;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + PIOS_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+BEFC").append(pios.view());

            // Send command
            Transmit(cmd.view(), status);
            if (status != StatusType::Ok) {
                return;
            }

            // Receive response (Ada pattern uses OK+Get not OK+Set)
            hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
            uart_.Receive(response_data, status);
            if (status != StatusType::Ok) {
                return;
            }

            // Verify response starts with OK+Get:
            const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};
            if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
                status = StatusType::ErrError;
            }
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get PIO pins output state after power supplied
    /// Equivalent to Ada's Get_Power_Pin_Output procedure
    /// @param pios Output parameter for PIO numbers configuration
    /// @param status Output parameter for operation result
    void GetPowerPinOutput(PioNumbers& pios, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t PIOS_LENGTH = 3;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + PIOS_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+BEFC?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract PIO numbers (after "OK+Get:")
        std::string_view pios_part = RESPONSE_STR.substr(OK_GET_PREFIX.size());
        if (pios_part.size() != PIOS_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy PIO numbers
        try {
            pios = PioNumbers(pios_part);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Set PIO pins output state after connection established
    /// Equivalent to Ada's Set_Connect_Pin_Output procedure
    /// Set pins state (high/low) after connection established
    /// NOTE: PIO0 and PIO1 are used by system, so must be 0
    /// Only PIO2~PIOB pins can be configured (HM-11: only PIO2~PIO3)
    /// @param pios PIO numbers configuration as 3 hex characters
    /// @param status Output parameter for operation result
    void SetConnectPinOutput(const PioNumbers& pios, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t PIOS_LENGTH = 3;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + PIOS_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        try {
            util::ATCommandBuilder cmd;
            cmd.append("AT+AFTC").append(pios.view());

            // Send command
            Transmit(cmd.view(), status);
            if (status != StatusType::Ok) {
                return;
            }

            // Receive response (Ada pattern uses OK+Get not OK+Set)
            hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
            uart_.Receive(response_data, status);
            if (status != StatusType::Ok) {
                return;
            }

            // Verify response starts with OK+Get:
            const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};
            if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
                status = StatusType::ErrError;
            }
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    /// Get PIO pins output state after connection established
    /// Equivalent to Ada's Get_Connect_Pin_Output procedure
    /// @param pios Output parameter for PIO numbers configuration
    /// @param status Output parameter for operation result
    void GetConnectPinOutput(PioNumbers& pios, StatusType& status) noexcept {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        constexpr std::size_t PIOS_LENGTH = 3;
        const auto EXPECTED_LENGTH = OK_GET_PREFIX.size() + PIOS_LENGTH;

        if (EXPECTED_LENGTH > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        Transmit("AT+AFTC?", status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), EXPECTED_LENGTH};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view RESPONSE_STR{reinterpret_cast<const char*>(response_buffer_.data()), EXPECTED_LENGTH};

        if (!RESPONSE_STR.starts_with(OK_GET_PREFIX)) {
            status = StatusType::ErrError;
            return;
        }

        // Extract PIO numbers (after "OK+Get:")
        std::string_view pios_part = RESPONSE_STR.substr(OK_GET_PREFIX.size());
        if (pios_part.size() != PIOS_LENGTH) {
            status = StatusType::ErrError;
            return;
        }

        // Copy PIO numbers
        try {
            pios = PioNumbers(pios_part);
        } catch (...) {
            status = StatusType::ErrError;
        }
    }

    // ============================================================================
    // Additional Configuration Methods
    // ============================================================================

    /// Get MAC address
    /// Equivalent to Ada's Get_MAC_Address procedure
    /// @param result Output parameter for MAC address
    /// @param status Output parameter for operation result
    void get_mac_address(MacAddress& result, StatusType& status) noexcept;

    /// Set battery monitor switch
    /// Equivalent to Ada's Set_Battery_Monitor_Switch procedure
    /// This command added in V520
    /// When is set, module will add battery information into scan response data package.
    /// Default: False.
    /// @param value True to enable battery monitoring, false to disable
    /// @param status Output parameter for operation result
    void set_battery_monitor_switch(bool value, StatusType& status) noexcept;

    /// Get battery monitor switch
    /// Equivalent to Ada's Get_Battery_Monitor_Switch procedure
    /// @param result Output parameter for battery monitor switch status
    /// @param status Output parameter for operation result
    void get_battery_monitor_switch(bool& result, StatusType& status) noexcept;

    /// Set battery information
    /// Equivalent to Ada's Set_Battery_Information procedure
    /// Should not be connected! Required: Set_Battery_Monitor_Switch (False)
    /// This is used to set battery information byte in advertising data when
    /// you closed module power monitor.
    /// @param value Battery percentage (0-100)
    /// @param status Output parameter for operation result
    void set_battery_information(Percent value, StatusType& status) noexcept;

    /// Query battery information
    /// Equivalent to Ada's Query_Battery_Information procedure
    /// @param result Output parameter for battery percentage
    /// @param status Output parameter for operation result
    void query_battery_information(Percent& result, StatusType& status) noexcept;

    /// Reset module to factory defaults
    /// Equivalent to Ada's Reset procedure
    /// Restore all setup value to factory setup
    /// @param status Output parameter for operation result
    void reset(StatusType& status) noexcept { TransmitAndCheck("AT+RENEW", "OK+RENEW", status); }

    /// Restart module
    /// Equivalent to Ada's Restart procedure
    /// @param status Output parameter for operation result
    void restart(StatusType& status) noexcept { TransmitAndCheck("AT+RESET", "OK+RESET", status); }

    /// Start working mode
    /// Equivalent to Ada's Start_Working procedure
    /// This command is only used when Work_Type = Respond_AT_Command.
    /// @param status Output parameter for operation result
    void start_working(StatusType& status) noexcept;

    /// Clear last connected device address
    /// Equivalent to Ada's Clear_Last_Connected_Address procedure
    /// Notice: Only Central role is used.
    /// @param status Output parameter for operation result
    void clear_last_connected_address(StatusType& status) noexcept;

    // ============================================================================
    // AT Command Infrastructure
    // ============================================================================

    /// Transmit AT command string to HM11 module
    /// Equivalent to Ada's Transmit procedure
    /// @param command AT command string to transmit
    /// @param status Output parameter for operation result
    void Transmit(std::string_view command, StatusType& status) noexcept { TransmitImpl(command, status); }

    /// Transmit AT command with additional data payload
    /// Equivalent to Ada's Transmit procedure with Data parameter
    /// @param command AT command string to transmit
    /// @param data Additional data payload to append
    /// @param status Output parameter for operation result
    void Transmit(std::string_view command, hal::UARTData8bConst data, StatusType& status) noexcept {
        TransmitImpl(command, data, status);
    }

    /// Check received response against expected pattern
    /// Equivalent to Ada's Check_Responce procedure
    /// @param received_data Received response data
    /// @param expected_response Expected response pattern
    /// @param status Input/output parameter for operation result (only modified if Ok on input)
    void check_response(
        hal::UARTData8bConst received_data,
        std::string_view expected_response,
        StatusType& status) noexcept {
        if (status != StatusType::Ok) {
            return; // Don't check if already in error state
        }

        const auto expected_size = expected_response.size();
        if (received_data.size() < expected_size) {
            status = StatusType::ErrError;
            return;
        }

        // Convert string to bytes for comparison
        for (std::size_t i = 0; i < expected_size; ++i) {
            if (received_data[i] != static_cast<uint8_t>(expected_response[i])) {
                status = StatusType::ErrError;
                return;
            }
        }
    }

    /// Transmit command and check response using internal buffer
    /// Equivalent to Ada's Transmit_And_Check procedure
    /// @param command AT command string to transmit
    /// @param expected_response Expected response pattern
    /// @param status Output parameter for operation result
    void TransmitAndCheck(std::string_view command, std::string_view expected_response, StatusType& status) noexcept {
        // Use internal response buffer
        const auto expected_length = expected_response.size();
        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        TransmitAndCheck(command, expected_response, hal::UARTData8b{response_buffer_.data(), expected_length}, status);
    }

    /// Transmit command and check response using provided buffer
    /// Equivalent to Ada's Transmit_And_Check procedure with explicit buffer
    /// @param command AT command string to transmit
    /// @param expected_response Expected response pattern
    /// @param response_buffer Buffer to store received response
    /// @param status Output parameter for operation result
    void TransmitAndCheck(
        std::string_view command,
        std::string_view expected_response,
        hal::UARTData8b response_buffer,
        StatusType& status) noexcept {
        // First transmit the command
        Transmit(command, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Then receive response
        uart_.Receive(response_buffer, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Finally check the response
        check_response(hal::UARTData8bConst{response_buffer.data(), response_buffer.size()}, expected_response, status);
    }

  private:
    /// UART implementation instance
    /// Exclusive ownership through move semantics (Decision #22)
    UARTImpl uart_;

    /// Internal response buffer for AT command processing
    /// DMA-aligned for efficient hardware access (Decision #20)
    /// Equivalent to Ada's Response : UART_Data_8b (1 .. Max_Message_Length * 5);
    alignas(32) std::array<uint8_t, MAX_MESSAGE_LENGTH * 5> response_buffer_;

    /// Temporary discovery information storage during parsing
    /// Equivalent to Ada's Discovered_Info : Discovered_Info_Type := Null_Discovered_Info;
    DiscoveredInfo discovered_info_;

    /// Current operational state for resource management
    OperationalState operational_state_;

    /// Current discovery parsing stage for state machine
    /// Equivalent to Ada's Kind : Scan_Stage_Kind := Selection;
    ScanStage scan_stage_;

    /// Current position in circular buffer for parsing
    /// Equivalent to Ada's Pos : Positive := 1;
    std::size_t buffer_position_;

    /// Current GATT service discovery parsing stage
    /// Used in Find_All_Services_UUID streaming response parsing
    GattServiceStage gatt_service_stage_;

    /// Flag tracking if GATT service discovery has started
    /// Equivalent to Ada's Started : Boolean := False;
    bool gatt_discovery_started_;

    /// Current GATT characteristic discovery parsing stage
    /// Used in Find_All_Characteristics_UUID streaming response parsing
    GattCharacteristicStage gatt_characteristic_stage_;

    /// Flag tracking if GATT characteristic discovery has started
    /// Equivalent to Ada's Started : Boolean := False;
    bool gatt_characteristic_discovery_started_;

    /// Storage for discovered devices during scan operation
    /// Supports indexing from 0-5 as per HM11 specification
    /// This enables Connect(index) to reference devices found during discovery
    /// Cleared at the start of each scan operation
    static constexpr std::size_t MAX_DISCOVERED_DEVICES = 6;
    std::array<DiscoveredInfo, MAX_DISCOVERED_DEVICES> discovered_devices_;
    std::size_t discovered_device_count_;

    /// Initialize hardware resources during construction
    /// Performs any hardware-specific setup required for operation
    void initialize_hardware() noexcept {
        // TODO: Initialize any hardware-specific resources
        // This could include setting up DMA channels, interrupt handlers, etc.
        // For now, hardware initialization is handled by the UART implementation itself
    }

    /// Perform graceful cleanup during destruction
    /// Implements Decision #22: graceful stop with 100ms timeout
    void perform_graceful_cleanup() noexcept {
        // Mark as shutting down to prevent new operations
        operational_state_ = OperationalState::ShuttingDown;

        // TODO: For future milestones, implement:
        // 1. Attempt graceful stop of ongoing operations
        // 2. Wait up to 100ms for completion
        // 3. Force-stop if timeout exceeded
        // 4. Unregister IRQ handlers

        // For now, cleanup is handled by UART implementation's destructor
    }

    // ============================================================================
    // AT Command Implementation Helpers
    // ============================================================================

    /// Internal implementation for transmitting string commands
    /// Converts string to UART data format and transmits
    /// @param command Command string to transmit
    /// @param status Output parameter for operation result
    void TransmitImpl(std::string_view command, StatusType& status) noexcept {
        // Convert string to UART data using stack-allocated buffer
        // This matches Ada's approach: Local : constant String := Command;
        if (command.size() > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Copy string bytes to buffer for transmission
        std::array<uint8_t, MAX_MESSAGE_LENGTH * 5> command_buffer{};
        for (std::size_t i = 0; i < command.size(); ++i) {
            command_buffer[i] = static_cast<uint8_t>(command[i]);
        }

        // Transmit using UART
        hal::UARTData8bConst uart_data{command_buffer.data(), command.size()};
        uart_.Transmit(uart_data, status);
    }

    /// Internal implementation for transmitting string commands with data
    /// Converts string to UART data format and appends additional data
    /// @param command Command string to transmit
    /// @param data Additional data to append
    /// @param status Output parameter for operation result
    void TransmitImpl(std::string_view command, hal::UARTData8bConst data, StatusType& status) noexcept {
        // Calculate total size needed
        const auto total_size = command.size() + data.size();
        if (total_size > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Build combined command + data in buffer
        std::array<uint8_t, MAX_MESSAGE_LENGTH * 5> command_buffer{};

        // Copy command string
        for (std::size_t i = 0; i < command.size(); ++i) {
            command_buffer[i] = static_cast<uint8_t>(command[i]);
        }

        // Append data payload
        for (std::size_t i = 0; i < data.size(); ++i) {
            command_buffer[command.size() + i] = data[i];
        }

        // Transmit combined data using UART
        hal::UARTData8bConst uart_data{command_buffer.data(), total_size};
        uart_.Transmit(uart_data, status);
    }

    // ============================================================================
    // GATT Service Discovery Implementation Helpers
    // ============================================================================

    /// Parse service discovery streaming response using explicit state machine
    /// Equivalent to Ada's Main loop with streaming parsing logic
    /// @param callback Function pointer for service UUID notifications
    /// @param user_data Optional user data pointer passed to callback
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void parse_service_discovery_response(
        ServiceUuidCallback callback,
        void* user_data,
        uint32_t timeout_ms,
        StatusType& status) noexcept;

    // ============================================================================
    // GATT Characteristic Discovery Implementation Helpers
    // ============================================================================

    /// Parse characteristic discovery streaming response using explicit state machine
    /// Equivalent to Ada's Main loop with streaming parsing logic for characteristics
    /// @param callback Function pointer for characteristic UUID notifications
    /// @param user_data Optional user data pointer passed to callback
    /// @param timeout_ms Maximum time to wait for responses in milliseconds
    /// @param status Output parameter for operation result
    void parse_characteristic_discovery_response(
        CharacteristicUuidCallback callback,
        void* user_data,
        uint32_t timeout_ms,
        StatusType& status) noexcept;

    // ============================================================================
    // Characteristic Notification Implementation Helpers
    // ============================================================================

    /// Parse notification response from characteristic operations
    /// Equivalent to Ada's Read_Characteristic_Notify_Responce procedure
    /// Parses "OK+SEND-OK", "OK+DATA-ER", or other responses
    /// @param status Input/output parameter for operation result
    /// @param response Output parameter for notification response
    void parse_characteristic_notify_response(StatusType& status, NotifyResponse& response) noexcept;
};

// ============================================================================
// Template Instantiation Patterns and Validation
// ============================================================================

/// Enhanced concept validation for HM11Driver template instantiation
/// Provides comprehensive compile-time checking with clear error messages
template <typename UARTImpl>
concept ValidHM11Driver = hal::UARTInterface<UARTImpl> && std::is_move_constructible_v<UARTImpl> &&
                          !std::is_reference_v<UARTImpl> && !std::is_pointer_v<UARTImpl>;

/// Compile-time memory layout validation
/// Ensures the driver fits within reasonable memory constraints for embedded systems
template <typename UARTImpl> consteval bool validate_driver_memory_layout() {
    constexpr std::size_t DRIVER_SIZE = sizeof(HM11Driver<UARTImpl>);
    constexpr std::size_t MAX_ACCEPTABLE_SIZE = 4096; // 4KB maximum for embedded use

    static_assert(
        DRIVER_SIZE <= MAX_ACCEPTABLE_SIZE,
        "HM11Driver instance exceeds maximum acceptable size for embedded systems");

    // Validate alignment requirements are met
    constexpr std::size_t DRIVER_ALIGNMENT = alignof(HM11Driver<UARTImpl>);
    static_assert(DRIVER_ALIGNMENT >= 4, "Driver must be at least 4-byte aligned");

    return true;
}

/// Factory function for creating HM11Driver instances with enhanced validation
/// Provides type deduction, instantiation control, and memory layout verification
/// @param uart UART implementation instance
/// @return HM11Driver instance with injected UART
template <hal::UARTInterface UARTImpl>
[[nodiscard]] auto make_hm11_driver(UARTImpl&& uart) noexcept -> HM11Driver<std::remove_cvref_t<UARTImpl>> {
    using CleanUARTType = std::remove_cvref_t<UARTImpl>;

    // Comprehensive compile-time validation
    static_assert(ValidHM11Driver<CleanUARTType>, "Invalid UART type for HM11Driver instantiation");
    static_assert(validate_driver_memory_layout<CleanUARTType>(), "Driver memory layout validation failed");

    return HM11Driver<CleanUARTType>(std::forward<UARTImpl>(uart));
}

// ============================================================================
// Common Type Aliases for Template Instantiation Control
// ============================================================================

// Note: Common type aliases and explicit template instantiations
// are defined in the implementation file to avoid circular dependencies

// ============================================================================
// Validation Tests
// ============================================================================

namespace detail {
/// Compile-time validation for template functionality
/// Tests run at compile-time and provide clear error messages

/// Test VariableString functionality at compile time
static_assert(VariableString<32>::MAX_CAPACITY == 32);
static_assert(VariableString<MAX_NAME_LENGTH>::MAX_CAPACITY == MAX_NAME_LENGTH);

/// Test DiscoveredInfo structure layout
static_assert(std::is_standard_layout_v<DiscoveredInfo>);

} // namespace detail


// ============================================================================
// Template Member Function Implementations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_module_auto_sleep(bool& sleep, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+PWRM?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(*parsed_response);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    sleep = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_advertising_type(AdvertisingType& type, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1;

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    Transmit("AT+ADTY?", status);
    if (status != StatusType::Ok) {
        return;
    }

    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};
    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_type = util::parse_advertising_type(parsed_response->front());
    if (!parsed_type.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    type = *parsed_type;
}

// Continue with a more concise approach for remaining methods to save space

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_reliable_advertising_mode(AdvertisingMode mode, StatusType& status) noexcept {
    try {
        const auto mode_str = util::to_string(mode);
        util::ATCommandBuilder cmd;
        cmd.append("AT+RELI").append(mode_str);
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(mode_str);
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_reliable_advertising_mode(AdvertisingMode& mode, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for mode character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+RELI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_mode = util::parse_advertising_mode(parsed_response->front());
    if (!parsed_mode.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    mode = *parsed_mode;
    status = StatusType::ErrError; // Placeholder
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_module_advertisement_data(const AdvertisementData& data, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+MARJ").append(std::string_view(data));
        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(std::string_view(data));
        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// UART Configuration

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_uart_baud_rate(UartBaudRate& rate, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for rate character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BAUD?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_rate = util::parse_uart_baud_rate(parsed_response->front());
    if (!parsed_rate.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    rate = *parsed_rate;
    status = StatusType::ErrError; // Placeholder
}

// UART Configuration Implementation

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_parity_bit(ParityBit parity, StatusType& status) noexcept {
    try {
        const auto parity_str = util::to_string(parity);

        util::ATCommandBuilder cmd;
        cmd.append("AT+PARI").append(parity_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(parity_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_parity_bit(ParityBit& parity, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for parity character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+PARI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_parity = util::parse_parity_bit(parsed_response->front());
    if (!parsed_parity.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    parity = *parsed_parity;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_mac_address(MacAddress& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 12; // +12 for MAC address

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+ADDR?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->size() != 12) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = MacAddress(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl> void HM11Driver<UARTImpl>::start_working(StatusType& status) noexcept {
    TransmitAndCheck("AT+START", "OK+START", status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::clear_last_connected_address(StatusType& status) noexcept {
    TransmitAndCheck("AT+CLEAR", "OK+CLEAR", status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_stop_bit(StopBit value, StatusType& status) noexcept {
    try {
        const auto stop_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+STOP").append(stop_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(stop_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_stop_bit(StopBit& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for stop bit character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+STOP?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_stop_bit = util::parse_stop_bit(parsed_response->front());
    if (!parsed_stop_bit.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_stop_bit;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_uart_flow_control_switch(bool switch_enabled, StatusType& status) noexcept {
    try {
        const auto switch_str = util::to_string(switch_enabled);

        util::ATCommandBuilder cmd;
        cmd.append("AT+FLOW").append(switch_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(switch_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_uart_flow_control_switch(bool& switch_enabled, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+FLOW?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    switch_enabled = parsed_bool.value;
}

// Connection Parameter Management Implementation

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval value,
    StatusType& status) noexcept {
    try {
        const auto interval_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COMI").append(interval_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(interval_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_minimum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for interval character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COMI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_interval = util::parse_link_layer_connection_interval(parsed_response->front());
    if (!parsed_interval.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_interval;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval value,
    StatusType& status) noexcept {
    try {
        const auto interval_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COMA").append(interval_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(interval_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_maximum_link_layer_connection_interval(
    LinkLayerConnectionInterval& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for interval character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COMA?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_interval = util::parse_link_layer_connection_interval(parsed_response->front());
    if (!parsed_interval.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_interval;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_link_layer_connection_slave_latency(
    LayerConnectionLatency value,
    StatusType& status) noexcept {
    try {
        // Convert uint8_t to string using simple arithmetic
        util::ATCommandBuilder cmd;
        cmd.append("AT+COLA").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_link_layer_connection_slave_latency(
    LayerConnectionLatency& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for latency character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COLA?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse single digit (0-4)
    const char latency_char = parsed_response->front();
    if (latency_char >= '0' && latency_char <= '4') {
        try {
            result = LayerConnectionLatency(static_cast<uint8_t>(latency_char - '0'));
        } catch (...) {
            status = StatusType::ErrError;
        }
    } else {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_connection_supervision_timeout(
    ConnectionSupervisionTimeout value,
    StatusType& status) noexcept {
    try {
        const auto timeout_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COSU").append(timeout_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(timeout_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_connection_supervision_timeout(
    ConnectionSupervisionTimeout& result,
    StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for timeout character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COSU?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_timeout = util::parse_connection_supervision_timeout(parsed_response->front());
    if (!parsed_timeout.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = *parsed_timeout;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_update_connection(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+COUP").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_update_connection(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+COUP?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

// White List Management Implementation

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_white_list_switch(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+WHLI").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_white_list_switch(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+WHLI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_white_list_mac_addresses(
    MacWhiteListIndex index,
    const MacAddress& value,
    StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+AD").append_int(index.value()).append(std::string_view(value));

        util::ATCommandBuilder expected;
        expected.append("OK+Set:");

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_white_list_mac_address(
    MacWhiteListIndex index,
    MacAddress& result,
    StatusType& status) noexcept {
    try {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 12; // +12 for MAC address

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        util::ATCommandBuilder cmd;
        cmd.append("AT+AD").append_int(index.value()).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->size() != 12) {
            status = StatusType::ErrError;
            return;
        }

        result = MacAddress(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// PIO Control Implementation

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pio_output_status(PioNumber pio, PioOutput output, StatusType& status) noexcept {
    try {
        const auto output_str = util::to_string(output);

        util::ATCommandBuilder cmd;
        cmd.append("AT+PIO").append_int(pio.value()).append(output_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(pio.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pio_output_status(PioNumber pio, PioOutput& output, StatusType& status) noexcept {
    try {
        constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
        const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for output character

        if (expected_length > response_buffer_.size()) {
            status = StatusType::ErrError;
            return;
        }

        // Send query command
        util::ATCommandBuilder cmd;
        cmd.append("AT+PIO").append_int(pio.value()).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        // Receive response
        hal::UARTData8b response_data{response_buffer_.data(), expected_length};
        uart_.Receive(response_data, status);
        if (status != StatusType::Ok) {
            return;
        }

        // Parse response
        const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

        const auto parsed_response = util::extract_get_response(response_str);
        if (!parsed_response.has_value() || parsed_response->empty()) {
            status = StatusType::ErrError;
            return;
        }

        const auto parsed_output = util::parse_pio_output(parsed_response->front());
        if (!parsed_output.has_value()) {
            status = StatusType::ErrError;
            return;
        }

        output = *parsed_output;
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pios_output_status(const PioNumbers& pios, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+PIOS").append(std::string_view(pios));

        util::ATCommandBuilder expected;
        expected.append("OK+Set:");

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pios_output_status(PioNumbers& pios, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 3; // +3 for PIO numbers

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+PIOS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->size() != 3) {
        status = StatusType::ErrError;
        return;
    }

    try {
        pios = PioNumbers(parsed_response.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_pio_collection_rate(PioCollectionRate value, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+CYCL").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_pio_collection_rate(PioCollectionRate& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 2; // +2 for max 2-digit rate

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+CYCL?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse rate as integer
    const auto rate_result = util::parse_uint<uint8_t>(parsed_response.value, 10);
    if (!rate_result.has_value() || rate_result.value > 99) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = PioCollectionRate(rate_result.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// Additional Configuration Implementation

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_battery_monitor_switch(bool value, StatusType& status) noexcept {
    try {
        const auto value_str = util::to_string(value);

        util::ATCommandBuilder cmd;
        cmd.append("AT+BATC").append(value_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append(value_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_battery_monitor_switch(bool& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for boolean character

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BATC?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    const auto parsed_bool = util::parse_bool(parsed_response.value);
    if (!parsed_bool.has_value()) {
        status = StatusType::ErrError;
        return;
    }

    result = parsed_bool.value;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_battery_information(Percent value, StatusType& status) noexcept {
    try {
        util::ATCommandBuilder cmd;
        cmd.append("AT+BATT").append_int(value.value());

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_int(value.value());

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::query_battery_information(Percent& result, StatusType& status) noexcept {
    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 3; // +3 for max 3-digit percentage

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+BATT?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    const auto parsed_response = util::extract_get_response(response_str);
    if (!parsed_response.has_value() || parsed_response->empty()) {
        status = StatusType::ErrError;
        return;
    }

    // Parse percentage as integer
    const auto percent_result = util::parse_uint<uint8_t>(parsed_response.value, 10);
    if (!percent_result.has_value() || percent_result.value > 100) {
        status = StatusType::ErrError;
        return;
    }

    try {
        result = Percent(percent_result.value);
    } catch (...) {
        status = StatusType::ErrError;
    }
}
// Discovery implementation for all template instantiations
// This approach avoids linker errors by providing concrete implementations

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::scan(
    DiscoveredCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Reset discovery state and clear discovered devices storage
    discovered_info_.reset();
    scan_stage_ = ScanStage::Selection;
    buffer_position_ = 0;
    discovered_device_count_ = 0;

    // Clear discovered devices array for new scan
    for (auto& device : discovered_devices_) {
        device.reset();
    }

    // Helper function to invoke callback with accumulated data and store device with index
    auto invoke_callback = [&]() {
        if (discovered_info_.id != ' ') {
            // Store device in discovered devices array (up to 6 devices)
            if (discovered_device_count_ < MAX_DISCOVERED_DEVICES) {
                discovered_devices_[discovered_device_count_] = discovered_info_;
                // Assign sequential index as device ID for Connect(index) operations
                discovered_devices_[discovered_device_count_].id = '0' + static_cast<char>(discovered_device_count_);
                discovered_device_count_++;
            }

            // Invoke callback with current device information
            callback(
                discovered_info_.id,
                discovered_info_.mac,
                discovered_info_.name.view(),
                discovered_info_.rssi.view(),
                user_data);

            // Reset for next device (matches Ada's Null_Discovered_Info assignment)
            discovered_info_.reset();
        }
    };

    // Send discovery command
    Transmit("AT+DISC?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // TODO: For now use simple receive - in Task 4 this will be replaced with streaming
    // For prototype, receive into internal buffer and parse synchronously
    constexpr std::size_t DISCOVERY_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), DISCOVERY_BUFFER_SIZE};

    // Simple timeout implementation - in Task 4 this will use Watchdog with ISR polling
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse discovery responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        // Check for discovery end marker
        if (util::starts_with(
                std::string_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining),
                util::discovery_prefixes::OK_DISCE)) {
            break;
        }

        // State machine processing
        switch (scan_stage_) {
            case ScanStage::Selection: {
                // Look for command prefixes
                if (remaining < util::discovery_prefixes::MIN_PREFIX) {
                    // Not enough data for any prefix
                    break;
                }

                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                if (util::starts_with(current_view, util::discovery_prefixes::OK_DISCS)) {
                    pos += util::discovery_prefixes::OK_DISCS.size();
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_DISC)) {
                    pos += util::discovery_prefixes::OK_DISC.size();
                    scan_stage_ = ScanStage::MAC;
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_NAME)) {
                    pos += util::discovery_prefixes::OK_NAME.size();
                    scan_stage_ = ScanStage::Name;
                    continue;
                }

                if (util::starts_with(current_view, util::discovery_prefixes::OK_RSSI)) {
                    pos += util::discovery_prefixes::OK_RSSI.size();
                    scan_stage_ = ScanStage::RSSI;
                    continue;
                }

                // No prefix matched, advance by one character
                pos++;
                break;
            }

            case ScanStage::MAC: {
                // Call callback for any previous complete device
                invoke_callback();

                constexpr std::size_t MAC_LENGTH = 12;
                if (remaining < MAC_LENGTH) {
                    // Not enough data for MAC address
                    return;
                }

                // Extract MAC address (12 hex characters)
                const std::string_view mac_str(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    MAC_LENGTH);

                const auto parsed_mac = util::parse_mac_address(mac_str);
                if (parsed_mac.has_value()) {
                    discovered_info_.mac = *parsed_mac;
                    discovered_info_.id = '0'; // Set default ID
                }

                pos += MAC_LENGTH;
                scan_stage_ = ScanStage::Selection;
                break;
            }

            case ScanStage::Name: {
                // Find CR+LF terminator
                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                const std::size_t crlf_pos = current_view.find(util::discovery_prefixes::CRLF);
                if (crlf_pos == std::string_view::npos) {
                    // No terminator found, append all remaining data
                    try {
                        discovered_info_.name.append(current_view);
                        pos = buffer_end; // Consumed all data
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                } else {
                    // Found terminator, append data before it
                    try {
                        discovered_info_.name.append(current_view.substr(0, crlf_pos));
                        pos += crlf_pos + util::discovery_prefixes::CRLF.size();
                        scan_stage_ = ScanStage::Selection;
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                }
                break;
            }

            case ScanStage::RSSI: {
                // Find CR+LF terminator
                const std::string_view current_view(
                    reinterpret_cast<const char*>(response_buffer_.data() + pos),
                    remaining);

                const std::size_t crlf_pos = current_view.find(util::discovery_prefixes::CRLF);
                if (crlf_pos == std::string_view::npos) {
                    // No terminator found, append all remaining data
                    try {
                        discovered_info_.rssi.append(current_view);
                        pos = buffer_end; // Consumed all data
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                } else {
                    // Found terminator, append data before it
                    try {
                        discovered_info_.rssi.append(current_view.substr(0, crlf_pos));
                        pos += crlf_pos + util::discovery_prefixes::CRLF.size();
                        scan_stage_ = ScanStage::Selection;
                    } catch (...) {
                        status = StatusType::ErrError;
                        return;
                    }
                }
                break;
            }
        }
    }

    // Call callback for any final accumulated data (matching Ada's final Call)
    invoke_callback();

    status = StatusType::Ok;
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::scan_ibeacon(
    DiscoveredIBeaconCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send iBeacon discovery command (matching Ada's "AT+DISI?")
    Transmit("AT+DISI?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t IBEACON_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), IBEACON_BUFFER_SIZE};

    // Receive iBeacon discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse iBeacon responses using simplified approach based on Ada implementation
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure for iBeacon
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        // Need at least minimum prefix length
        if (remaining < util::ibeacon_discovery_prefixes::OK_DISC.size()) {
            break;
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // Check for iBeacon discovery end marker
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISCE)) {
            break;
        }

        // Check for iBeacon discovery start (OK+DISCS)
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISCS)) {
            pos += util::ibeacon_discovery_prefixes::OK_DISCS.size();
            continue;
        }

        // Check for iBeacon device data (OK+DISC + 66 characters)
        if (util::starts_with(current_view, util::ibeacon_discovery_prefixes::OK_DISC)) {
            const std::size_t data_start = pos + util::ibeacon_discovery_prefixes::OK_DISC.size();
            const std::size_t required_length = util::ibeacon_discovery_prefixes::OK_DISC.size() +
                                                util::ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH;

            if (remaining >= required_length) {
                // Extract the 66-character iBeacon data
                const std::string_view ibeacon_data(
                    reinterpret_cast<const char*>(response_buffer_.data() + data_start),
                    util::ibeacon_discovery_prefixes::IBEACON_DATA_LENGTH);

                // Invoke callback with the device string (matches Ada interface exactly)
                try {
                    callback(ibeacon_data, user_data);
                } catch (...) {
                    // Continue processing even if callback throws - log error but don't halt parsing
                    // In production code, this should log the error appropriately
                    // For now, we silently continue to prevent termination of discovery
                    (void)0; // Explicit no-op to satisfy linter
                }

                pos += required_length;
                continue;
            }
            // Not enough data for complete iBeacon record
            break;
        }

        // No prefix matched, advance by one character
        pos++;
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Service Discovery Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_all_services_uuid(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send service discovery command
    Transmit("AT+FINDALLSERVICES?", status);
    if (status != StatusType::Ok) {
        return;
    }

    parse_service_discovery_response(callback, user_data, timeout_ms, status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_service_discovery_response(
    ServiceUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    // Reset service discovery state
    gatt_service_stage_ = GattServiceStage::LookingForHeader;
    gatt_discovery_started_ = false;

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t SERVICE_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), SERVICE_BUFFER_SIZE};

    // Receive service discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse service responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        if (remaining < util::gatt_service_discovery::SERVICE_UUID_LENGTH) {
            break; // Not enough data for service UUID
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // State machine processing
        switch (gatt_service_stage_) {
            case GattServiceStage::LookingForHeader: {
                // Look for header marker
                if (remaining >= util::gatt_service_discovery::SERVICE_HEADER_LENGTH) {
                    if (util::starts_with(current_view, util::gatt_service_discovery::SERVICE_HEADER)) {
                        if (gatt_discovery_started_) {
                            // End message, exit
                            return;
                        }
                        // Start message
                        gatt_discovery_started_ = true;
                        pos += util::gatt_service_discovery::SERVICE_HEADER_LENGTH;
                        gatt_service_stage_ = GattServiceStage::ParsingService;
                        continue;
                    }
                }
                pos++;
                break;
            }

            case GattServiceStage::ParsingService: {
                // Look for CRLF to skip line breaks
                if (util::starts_with(current_view, util::discovery_prefixes::CRLF)) {
                    pos += util::discovery_prefixes::CRLF.size();
                    continue;
                }

                // Check for end header
                if (remaining >= util::gatt_service_discovery::SERVICE_HEADER_LENGTH &&
                    util::starts_with(current_view, util::gatt_service_discovery::SERVICE_HEADER)) {
                    // End of services
                    return;
                }

                // Parse service UUID line (14 characters: 4:4:4)
                if (remaining >= util::gatt_service_discovery::SERVICE_UUID_LENGTH) {
                    const std::string_view service_data =
                        current_view.substr(0, util::gatt_service_discovery::SERVICE_UUID_LENGTH);

                    // Validate service format and invoke callback
                    std::string_view start_handle;
                    std::string_view end_handle;
                    std::string_view service_uuid;
                    if (util::parse_service_uuid_components(service_data, start_handle, end_handle, service_uuid)) {
                        try {
                            callback(service_data, user_data);
                        } catch (...) {
                            // Continue processing even if callback throws - log error but don't halt parsing
                            // In production code, this should log the error appropriately
                            // For now, we silently continue to prevent termination of discovery
                            (void)0; // Explicit no-op to satisfy linter
                        }
                    }

                    pos += util::gatt_service_discovery::SERVICE_UUID_LENGTH;
                    continue;
                }

                pos++;
                break;
            }

            case GattServiceStage::LookingForEnd: {
                // This state is handled by the header detection above
                pos++;
                break;
            }
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Discovery Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_all_characteristics_uuid(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    // Send characteristic discovery command
    Transmit("AT+FINDALLCHARS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::find_characteristics_uuid(
    const HandleType& from,
    const HandleType& to,
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    if (callback == nullptr) {
        status = StatusType::ErrError;
        return;
    }

    try {
        // Build command: "AT+CHAR" + from + to + "?"
        util::ATCommandBuilder cmd;
        cmd.append("AT+CHAR").append(std::string_view(from)).append(std::string_view(to)).append("?");

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            return;
        }

        parse_characteristic_discovery_response(callback, user_data, timeout_ms, status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_discovery_response(
    CharacteristicUuidCallback callback,
    void* user_data,
    [[maybe_unused]] uint32_t timeout_ms,
    StatusType& status) noexcept {
    // Reset characteristic discovery state
    gatt_characteristic_stage_ = GattCharacteristicStage::LookingForHeader;
    gatt_characteristic_discovery_started_ = false;

    // For prototype, use simple synchronous receive - in production this would use streaming with circular buffer
    constexpr std::size_t CHARACTERISTIC_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    hal::UARTData8b response_data{response_buffer_.data(), CHARACTERISTIC_BUFFER_SIZE};

    // Receive characteristic discovery responses
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse characteristic responses using state machine
    std::size_t pos = 0;
    const std::size_t buffer_end = response_data.size();

    // Main parsing loop matching Ada's Main loop structure
    while (pos < buffer_end) {
        const std::size_t remaining = buffer_end - pos;

        if (remaining < util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH) {
            break; // Not enough data for characteristic UUID
        }

        const std::string_view current_view(reinterpret_cast<const char*>(response_buffer_.data() + pos), remaining);

        // State machine processing
        switch (gatt_characteristic_stage_) {
            case GattCharacteristicStage::LookingForHeader: {
                // Look for header marker
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH) {
                    if (util::starts_with(current_view, util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER)) {
                        if (gatt_characteristic_discovery_started_) {
                            // End message, exit
                            return;
                        }
                        // Start message
                        gatt_characteristic_discovery_started_ = true;
                        pos += util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH;
                        gatt_characteristic_stage_ = GattCharacteristicStage::ParsingCharacteristic;
                        continue;
                    }
                }
                pos++;
                break;
            }

            case GattCharacteristicStage::ParsingCharacteristic: {
                // Look for CRLF to skip line breaks
                if (util::starts_with(current_view, util::discovery_prefixes::CRLF)) {
                    pos += util::discovery_prefixes::CRLF.size();
                    continue;
                }

                // Check for end header
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER_LENGTH &&
                    util::starts_with(current_view, util::gatt_characteristic_discovery::CHARACTERISTIC_HEADER)) {
                    // End of characteristics
                    return;
                }

                // Parse characteristic UUID line (24 characters: 4:14:4)
                if (remaining >= util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH) {
                    const std::string_view characteristic_data =
                        current_view.substr(0, util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH);

                    // Validate characteristic format and invoke callback
                    std::string_view handle;
                    util::CharacteristicProperty properties = {};
                    std::string_view characteristic_uuid;

                    if (util::parse_characteristic_uuid_components(
                            characteristic_data,
                            handle,
                            properties,
                            characteristic_uuid)) {
                        try {
                            callback(characteristic_data, user_data);
                        } catch (...) {
                            // Continue processing even if callback throws - log error but don't halt parsing
                            // In production code, this should log the error appropriately
                            // For now, we silently continue to prevent termination of discovery
                            (void)0; // Explicit no-op to satisfy linter
                        }
                    }

                    pos += util::gatt_characteristic_discovery::CHARACTERISTIC_UUID_LENGTH;
                    continue;
                }

                pos++;
                break;
            }

            case GattCharacteristicStage::LookingForEnd: {
                // This state is handled by the header detection above
                pos++;
                break;
            }
        }
    }

    status = StatusType::Ok;
}

// ============================================================================
// GATT Characteristic Operations
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::enable_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+NOTIFY_ON" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+NOTIFY_ON").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::disable_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+NOTIFYOFF" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+NOTIFYOFF").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::read_characteristic_notify(
    const HandleType& handle,
    StatusType& status,
    NotifyResponse& response) noexcept {
    try {
        // Build command: "AT+READDATA" + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+READDATA").append(std::string_view(handle));

        Transmit(cmd.view(), status);
        if (status != StatusType::Ok) {
            response = NotifyResponse::SendEr;
            return;
        }

        parse_characteristic_notify_response(status, response);
    } catch (...) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_method_and_characteristic_handle(
    const HandleType& handle,
    SendDataMethod method,
    StatusType& status) noexcept {
    try {
        // Build command: "AT+SET_WAY" + method_string + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+SET_WAY").append(util::to_string(method)).append(std::string_view(handle));

        TransmitAndCheck(cmd.view(), "OK+SEND-OK", status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::send_data_to_characteristic(
    const HandleType& handle,
    SendDataCharacteristic method,
    hal::UARTData8bConst data,
    StatusType& status) noexcept {
    try {
        // Build command: "AT+SEND_DATA" + method_string + handle
        util::ATCommandBuilder cmd;
        cmd.append("AT+SEND_DATA").append(util::to_string(method)).append(std::string_view(handle));

        Transmit(cmd.view(), data, status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::parse_characteristic_notify_response(StatusType& status, NotifyResponse& response) noexcept {
    if (status != StatusType::Ok) {
        response = NotifyResponse::SendEr;
        return;
    }

    // Expected responses: "OK+SEND-OK", "OK+DATA-ER", or other (mapped to Send_Er)
    constexpr std::string_view SEND_OK = "OK+SEND-OK";
    constexpr std::string_view DATA_ER = "OK+DATA-ER";
    const auto expected_length = std::max(SEND_OK.size(), DATA_ER.size());

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        response = NotifyResponse::SendEr;
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        response = NotifyResponse::SendEr;
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (response_str == SEND_OK) {
        response = NotifyResponse::SendOk;
    } else if (response_str == DATA_ER) {
        response = NotifyResponse::DataEr;
    } else {
        response = NotifyResponse::SendEr;
    }
}
// ============================================================================
// iBeacon Configuration Operations Implementation
// ============================================================================

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_switch(bool enabled, StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_Switch procedure
    // Command: "AT+IBEA" + "1"/"0"
    // Expected response: "OK+Set:" + "1"/"0"

    try {
        const char switch_value = enabled ? '1' : '0';

        util::ATCommandBuilder cmd;
        cmd.append("AT+IBEA").append_char(switch_value);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_char(switch_value);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_ibeacon_switch(bool& enabled, StatusType& status) noexcept {
    // Equivalent to Ada's Get_iBeacon_Switch procedure
    // Command: "AT+IBEA?"
    // Expected response: "OK+Get:" + "1"/"0"

    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 1; // +1 for switch value

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+IBEA?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (!response_str.starts_with(OK_GET_PREFIX)) {
        status = StatusType::ErrError;
        return;
    }

    const char switch_value = response_str[OK_GET_PREFIX.size()];
    if (switch_value == '1') {
        enabled = true;
    } else if (switch_value == '0') {
        enabled = false;
    } else {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_uuid(const IBeaconUuid& uuid, StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_UUID procedure with multi-part setting
    // The Ada implementation sends 4 commands, each setting 8 characters:
    // AT+IBE00x + 8chars, AT+IBE10x + 8chars, AT+IBE20x + 8chars, AT+IBE30x + 8chars

    try {
        const std::string_view uuid_str = uuid.view();

        // Validate UUID length at runtime (compile-time validation handled by FixedString)
        if (uuid_str.size() != 32) {
            status = StatusType::ErrError;
            return;
        }

        // Set each 8-character part (0-3)
        for (int part = 0; part < 4; ++part) {
            const std::size_t start_pos = static_cast<std::size_t>(part) * 8;
            const std::string_view uuid_part = uuid_str.substr(start_pos, 8);

            // Build command: "AT+IBE" + part + "0x" + uuid_part
            util::ATCommandBuilder cmd;
            cmd.append("AT+IBE").append_char(static_cast<char>('0' + part)).append("0x").append(uuid_part);

            // Build expected response: "OK+Set:" + part + "0x" + uuid_part
            util::ATCommandBuilder expected;
            expected.append("OK+Set:").append_char(static_cast<char>('0' + part)).append("0x").append(uuid_part);

            TransmitAndCheck(cmd.view(), expected.view(), status);
            if (status != StatusType::Ok) {
                return; // Exit early if any part fails
            }
        }
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_ibeacon_uuid(IBeaconUuid& uuid, StatusType& status) noexcept {
    // Equivalent to Ada's Get_iBeacon_UUID procedure with multi-part getting
    // Query each part: AT+IBE0?, AT+IBE1?, AT+IBE2?, AT+IBE3?
    // Response format: "OK+Get:" + "0x" + 8chars

    try {
        std::array<char, 32> uuid_buffer{};

        // Get each 8-character part (0-3)
        for (int part = 0; part < 4; ++part) {
            // Build query command: "AT+IBE" + part + "?"
            util::ATCommandBuilder cmd;
            cmd.append("AT+IBE").append_char(static_cast<char>('0' + part)).append("?");

            constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
            const auto expected_length = OK_GET_PREFIX.size() + 2 + 8; // +2 for "0x", +8 for hex chars

            if (expected_length > response_buffer_.size()) {
                status = StatusType::ErrError;
                return;
            }

            // Send query command
            Transmit(cmd.view(), status);
            if (status != StatusType::Ok) {
                return;
            }

            // Receive response
            hal::UARTData8b response_data{response_buffer_.data(), expected_length};
            uart_.Receive(response_data, status);
            if (status != StatusType::Ok) {
                return;
            }

            // Parse response
            const std::string_view response_str{
                reinterpret_cast<const char*>(response_buffer_.data()),
                expected_length};

            if (!response_str.starts_with(OK_GET_PREFIX) || response_str.substr(OK_GET_PREFIX.size(), 2) != "0x") {
                status = StatusType::ErrError;
                return;
            }

            // Extract 8-character UUID part
            const std::string_view uuid_part = response_str.substr(OK_GET_PREFIX.size() + 2, 8);
            const std::size_t start_pos = static_cast<std::size_t>(part) * 8;

            // Copy to buffer
            for (std::size_t i = 0; i < 8; ++i) {
                uuid_buffer[start_pos + i] = uuid_part[i];
            }
        }

        // Create UUID from buffer
        uuid = IBeaconUuid(std::string_view(uuid_buffer.data(), 32));

    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_major_version(const VersionType& major_version, StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_Marjor_Version procedure
    // Command: "AT+MARJ0x" + version
    // Expected response: "OK+Set:" + "0x" + version

    try {
        const std::string_view version_str = major_version.view();

        util::ATCommandBuilder cmd;
        cmd.append("AT+MARJ0x").append(version_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:0x").append(version_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_ibeacon_major_version(VersionType& major_version, StatusType& status) noexcept {
    // Equivalent to Ada's Get_iBeacon_Marjor_Version procedure
    // Command: "AT+MARJ?"
    // Expected response: "OK+Get:" + "0x" + version

    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 2 + 4; // +2 for "0x", +4 for version

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+MARJ?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (!response_str.starts_with(OK_GET_PREFIX) || response_str.substr(OK_GET_PREFIX.size(), 2) != "0x") {
        status = StatusType::ErrError;
        return;
    }

    // Extract version
    const std::string_view version_part = response_str.substr(OK_GET_PREFIX.size() + 2, 4);

    try {
        major_version = VersionType(version_part);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_minor_version(const VersionType& minor_version, StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_Minor_Version procedure
    // Command: "AT+MINO0x" + version
    // Expected response: "OK+Set:" + "0x" + version

    try {
        const std::string_view version_str = minor_version.view();

        util::ATCommandBuilder cmd;
        cmd.append("AT+MINO0x").append(version_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:0x").append(version_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_ibeacon_minor_version(VersionType& minor_version, StatusType& status) noexcept {
    // Equivalent to Ada's Get_iBeacon_Minor_Version procedure
    // Command: "AT+MINO?"
    // Expected response: "OK+Get:" + "0x" + version

    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 2 + 4; // +2 for "0x", +4 for version

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+MINO?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (!response_str.starts_with(OK_GET_PREFIX) || response_str.substr(OK_GET_PREFIX.size(), 2) != "0x") {
        status = StatusType::ErrError;
        return;
    }

    // Extract version
    const std::string_view version_part = response_str.substr(OK_GET_PREFIX.size() + 2, 4);

    try {
        minor_version = VersionType(version_part);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_measured_power(
    const MeasuredPower& measured_power,
    StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_Measured_Power procedure
    // Command: "AT+MEAS0x" + power
    // Expected response: "OK+Set:" + "0x" + power

    try {
        const std::string_view power_str = measured_power.view();

        util::ATCommandBuilder cmd;
        cmd.append("AT+MEAS0x").append(power_str);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:0x").append(power_str);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::get_ibeacon_measured_power(MeasuredPower& measured_power, StatusType& status) noexcept {
    // Equivalent to Ada's Get_iBeacon_Measured_Power procedure
    // Command: "AT+MEAS?"
    // Expected response: "OK+Get:" + "0x" + power

    constexpr std::string_view OK_GET_PREFIX = "OK+Get:";
    const auto expected_length = OK_GET_PREFIX.size() + 2 + 2; // +2 for "0x", +2 for power

    if (expected_length > response_buffer_.size()) {
        status = StatusType::ErrError;
        return;
    }

    // Send query command
    Transmit("AT+MEAS?", status);
    if (status != StatusType::Ok) {
        return;
    }

    // Receive response
    hal::UARTData8b response_data{response_buffer_.data(), expected_length};
    uart_.Receive(response_data, status);
    if (status != StatusType::Ok) {
        return;
    }

    // Parse response
    const std::string_view response_str{reinterpret_cast<const char*>(response_buffer_.data()), expected_length};

    if (!response_str.starts_with(OK_GET_PREFIX) || response_str.substr(OK_GET_PREFIX.size(), 2) != "0x") {
        status = StatusType::ErrError;
        return;
    }

    // Extract power
    const std::string_view power_part = response_str.substr(OK_GET_PREFIX.size() + 2, 2);

    try {
        measured_power = MeasuredPower(power_part);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

template <hal::UARTInterface UARTImpl>
void HM11Driver<UARTImpl>::set_ibeacon_deploy_mode(IBeaconDeployMode deploy_mode, StatusType& status) noexcept {
    // Equivalent to Ada's Set_iBeacon_Deploy_Mode procedure
    // Command: "AT+DELO" + mode_value
    // Expected response: "OK+Set:" + mode_value
    // After received OK, module will reset after 500ms.
    // This command will let module into non-connectable status until next power on.

    try {
        const char mode_value = (deploy_mode == IBeaconDeployMode::BroadcastScanning) ? '0' : '1';

        util::ATCommandBuilder cmd;
        cmd.append("AT+DELO").append_char(mode_value);

        util::ATCommandBuilder expected;
        expected.append("OK+Set:").append_char(mode_value);

        TransmitAndCheck(cmd.view(), expected.view(), status);
    } catch (...) {
        status = StatusType::ErrError;
    }
}

// ============================================================================
} // namespace hm11
