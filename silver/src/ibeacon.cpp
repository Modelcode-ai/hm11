// HM11 iBeacon Configuration Implementation
// Implements Design Decision #14 (iBeacon Configuration) using individual setter methods
// Follows Ada's separate procedure approach with strong typedef validation

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/util/parsing.hpp"
#include "hm11/util/string_builder.hpp"

#ifdef HM11_STM32_PLATFORM
#include "hm11/hal/stm32_uart_port.hpp"
#endif

namespace hm11 {

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
// Template Instantiation for Common Types
// ============================================================================

// MockUART template instantiations for iBeacon operations
template void HM11Driver<hal::MockUART>::set_ibeacon_switch(bool, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::get_ibeacon_switch(bool&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::set_ibeacon_uuid(const IBeaconUuid&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::get_ibeacon_uuid(IBeaconUuid&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::set_ibeacon_major_version(const VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::get_ibeacon_major_version(VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::set_ibeacon_minor_version(const VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::get_ibeacon_minor_version(VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::set_ibeacon_measured_power(const MeasuredPower&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::get_ibeacon_measured_power(MeasuredPower&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::MockUART>::set_ibeacon_deploy_mode(IBeaconDeployMode, hal::UARTStatus&) noexcept;

#ifdef HM11_STM32_PLATFORM
// STM32 template instantiations for iBeacon operations
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_switch(bool, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::get_ibeacon_switch(bool&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_uuid(const IBeaconUuid&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::get_ibeacon_uuid(IBeaconUuid&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_major_version(const VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::get_ibeacon_major_version(VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_minor_version(const VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::get_ibeacon_minor_version(VersionType&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_measured_power(
    const MeasuredPower&,
    hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::get_ibeacon_measured_power(MeasuredPower&, hal::UARTStatus&) noexcept;
template void HM11Driver<hal::STM32UARTPort>::set_ibeacon_deploy_mode(IBeaconDeployMode, hal::UARTStatus&) noexcept;
#endif

} // namespace hm11