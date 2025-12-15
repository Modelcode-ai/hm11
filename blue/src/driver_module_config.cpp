#include <algorithm>
#include <array>
#include <string>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/util/numeric_formatter.hpp"
#include "hm11/util/string_parser.hpp"

namespace hm11 {

using util::NumericFormatter;

// Helper to build expected response strings
static std::string_view build_expected(const char* prefix, std::string_view value, std::array<char, 64>& buffer) {
    std::size_t pos = 0;
    std::size_t prefix_len = std::strlen(prefix);
    std::memcpy(buffer.data(), prefix, prefix_len);
    pos += prefix_len;
    std::memcpy(buffer.data() + pos, value.data(), value.size());
    pos += value.size();
    return {buffer.data(), pos};
}

// ========== Module Config Methods ==========

bool HM11Driver::set_work_type(WorkType type, Status& status) {
    auto type_str = NumericFormatter::format(static_cast<int>(type));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetWorkType, {type_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, type_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_work_type(WorkType& type, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetWorkType, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    type = static_cast<WorkType>(value);
    return true;
}

bool HM11Driver::set_work_mode(WorkMode mode, Status& status) {
    auto mode_str = NumericFormatter::format(static_cast<int>(mode));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetWorkMode, {mode_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, mode_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_work_mode(WorkMode& mode, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetWorkMode, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    mode = static_cast<WorkMode>(value);
    return true;
}

bool HM11Driver::set_notify_mode(NotifyMode mode, Status& status) {
    auto mode_str = NumericFormatter::format(static_cast<int>(mode));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetNotifyMode, {mode_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, mode_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_notify_mode(NotifyMode& mode, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetNotifyMode, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    mode = static_cast<NotifyMode>(value);
    return true;
}

bool HM11Driver::set_module_name(std::string_view name, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetModuleName, {name});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, name, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_name(std::string_view& name, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetModuleName, {});
    constexpr const char* EXPECTED = OK_GET;
    transmit_and_check(cmd_buf.finalize(), EXPECTED, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET: followed by the module name (up to 12 characters)
    // Find the end of the name by looking for \r or \n or end of buffer
    std::size_t start = std::strlen(EXPECTED);
    std::size_t end = start;
    while (end < response_buffer.size() &&
           response_buffer.at(end) != '\r' &&
           response_buffer.at(end) != '\n' &&
           response_buffer.at(end) != '\0') {
        ++end;
    }
    // Return string_view pointing into response_buffer (no heap allocation)
    name = std::string_view(reinterpret_cast<const char*>(response_buffer.data() + start), end - start);
    return true;
}

bool HM11Driver::set_notify_information(bool notify, Status& status) {
    constexpr const char* NOTIFY_STR_1 = "1";
    constexpr const char* NOTIFY_STR_0 = "0";
    auto notify_str = notify ? NOTIFY_STR_1 : NOTIFY_STR_0;
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetNotifyInformation, {notify_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, notify_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_notify_information(bool& notify, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetNotifyInformation, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    notify = (response_buffer[std::strlen(OK_GET)] == '1');
    return true;
}

bool HM11Driver::set_module_rx_gain(RXGain gain, Status& status) {
    auto gain_str = NumericFormatter::format(static_cast<int>(gain));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetModuleRXGain, {gain_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, gain_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_rx_gain(RXGain& gain, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetModuleRXGain, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    gain = static_cast<RXGain>(value);
    return true;
}

// ========== PIO Control Methods ==========

bool HM11Driver::set_pio_output_status(util::PioNumber pio, PIOOutput output, Status& status) {
    // Store formatted output value first (before it gets overwritten by pio formatting)
    std::array<char, 4> output_buf{};
    auto output_str_temp = NumericFormatter::format(static_cast<int>(output));
    std::memcpy(output_buf.data(), output_str_temp.data(), output_str_temp.size());
    std::string_view output_str(output_buf.data(), output_str_temp.size());

    std::array<char, 64> expected_buf{};
    std::string_view expected;

    if (static_cast<int>(pio) == 1) {
        // Special case for PIO1
        auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPIOOutputStatus, {"1", output_str});
        expected = build_expected(OK_SET, output_str, expected_buf);
        transmit_and_check(cmd_buf.finalize(), expected, status);
    } else {
        // PIO 2 and 3: AT+PIO[2|3][0|1]
        auto pio_str = NumericFormatter::format(static_cast<int>(pio));
        auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPIOOutputStatus, {pio_str, output_str});
        // Build expected: "OK+PIO" + pio_str + ":" + output_str
        std::size_t pos = 0;
        constexpr const char* PREFIX = "OK+PIO";
        std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
        pos += std::strlen(PREFIX);
        std::memcpy(expected_buf.data() + pos, pio_str.data(), pio_str.size());
        pos += pio_str.size();
        expected_buf[pos++] = ':';
        std::memcpy(expected_buf.data() + pos, output_str.data(), output_str.size());
        pos += output_str.size();
        expected = std::string_view(expected_buf.data(), pos);
        transmit_and_check(cmd_buf.finalize(), expected, status);
    }

    return status == Status::Ok;
}

bool HM11Driver::get_pio_output_status(util::PioNumber pio, PIOOutput& output, Status& status) {
    std::array<char, 64> expected_buf{};
    std::string_view expected;

    if (static_cast<int>(pio) == 1) {
        // Special case for PIO1: AT+PIO1?
        auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPIOOutputStatus, {"1?"});
        expected = OK_GET;
        transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    } else {
        // PIO 2 and 3: AT+PIO[2|3]?
        auto pio_str = NumericFormatter::format(static_cast<int>(pio));
        auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPIOOutputStatus, {pio_str, "?"});
        // Build expected: "OK+PIO" + pio_str + ":"
        std::size_t pos = 0;
        constexpr const char* PREFIX = "OK+PIO";
        std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
        pos += std::strlen(PREFIX);
        std::memcpy(expected_buf.data() + pos, pio_str.data(), pio_str.size());
        pos += pio_str.size();
        expected_buf[pos++] = ':';
        expected = std::string_view(expected_buf.data(), pos);
        transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    }

    if (status != Status::Ok) {
        return false;
    }

    int value = response_buffer.at(expected.size()) - '0';
    output = static_cast<PIOOutput>(value);
    return true;
}

bool HM11Driver::get_pios_output_status(util::PIONumbers& pios, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPIOsOutputStatus, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::array<char, 3> pios_arr{};
    std::memcpy(pios_arr.data(), response_buffer.data() + std::strlen(OK_GET), 3);
    pios = util::PIONumbers(std::string_view(pios_arr.data(), 3));
    return true;
}

bool HM11Driver::set_pios_output_status(const util::PIONumbers& pios, Status& status) {
    auto pios_str = pios.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPIOsOutputStatus, {pios_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, pios_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_pio_collection_rate(util::PioCollectionRate rate, Status& status) {
    auto rate_str = NumericFormatter::format(static_cast<int>(rate));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPIOCollectionRate, {rate_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, rate_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_pio_collection_rate(util::PioCollectionRate& rate, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPIOCollectionRate, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Parse 2 digit characters from response
    std::size_t offset = std::strlen(OK_GET);
    std::string_view rate_view(reinterpret_cast<const char*>(response_buffer.data() + offset), 2);
    unsigned int value = 0;
    if (!util::parse_decimal(rate_view, value)) {
        status = Status::InvalidResponse;
        return false;
    }
    rate = util::PioCollectionRate(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_power_pin_output(const util::PIONumbers& pios, Status& status) {
    auto pios_str = pios.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPowerPinOutput, {pios_str});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_power_pin_output(util::PIONumbers& pios, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPowerPinOutput, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::array<char, 3> pios_arr{};
    std::memcpy(pios_arr.data(), response_buffer.data() + std::strlen(OK_GET), 3);
    pios = util::PIONumbers(std::string_view(pios_arr.data(), 3));
    return true;
}

bool HM11Driver::set_connect_pin_output(const util::PIONumbers& pios, Status& status) {
    auto pios_str = pios.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetConnectPinOutput, {pios_str});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connect_pin_output(util::PIONumbers& pios, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetConnectPinOutput, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::array<char, 3> pios_arr{};
    std::memcpy(pios_arr.data(), response_buffer.data() + std::strlen(OK_GET), 3);
    pios = util::PIONumbers(std::string_view(pios_arr.data(), 3));
    return true;
}

// ========== Security Methods ==========

bool HM11Driver::set_bond_mode(BondMode mode, Status& status) {
    auto mode_str = NumericFormatter::format(static_cast<int>(mode));
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetBondMode, {mode_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, mode_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_bond_mode(BondMode& mode, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetBondMode, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    int value = response_buffer[std::strlen(OK_GET)] - '0';
    mode = static_cast<BondMode>(value);
    return true;
}

bool HM11Driver::set_pin_code(const util::PINType& pin, Status& status) {
    auto pin_str = pin.view();
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetPINCode, {pin_str});
    std::array<char, 64> expected_buf{};
    auto expected = build_expected(OK_SET, pin_str, expected_buf);
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_pin_code(util::PINType& pin, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetPINCode, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 6 digit characters from response
    std::array<char, 6> pin_arr{};
    std::memcpy(pin_arr.data(), response_buffer.data() + std::strlen(OK_GET), 6);
    pin = util::PINType(std::string_view(pin_arr.data(), 6));
    return true;
}

} // namespace hm11
