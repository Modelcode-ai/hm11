#include <algorithm>
#include <string>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"

namespace hm11 {

// ========== Module Config Methods ==========

bool HM11Driver::set_work_type(WorkType type, Status& status) {
    std::string type_str = std::to_string(static_cast<int>(type));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetWorkType, {type_str});
    std::string expected = OK_SET + type_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_work_type(WorkType& type, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetWorkType, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    type = static_cast<WorkType>(value);
    return true;
}

bool HM11Driver::set_work_mode(WorkMode mode, Status& status) {
    std::string mode_str = std::to_string(static_cast<int>(mode));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetWorkMode, {mode_str});
    std::string expected = OK_SET + mode_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_work_mode(WorkMode& mode, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetWorkMode, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    mode = static_cast<WorkMode>(value);
    return true;
}

bool HM11Driver::set_notify_mode(NotifyMode mode, Status& status) {
    std::string mode_str = std::to_string(static_cast<int>(mode));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetNotifyMode, {mode_str});
    std::string expected = OK_SET + mode_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_notify_mode(NotifyMode& mode, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetNotifyMode, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    mode = static_cast<NotifyMode>(value);
    return true;
}

bool HM11Driver::set_module_name(std::string_view name, Status& status) {
    std::string name_str(name);
    std::string cmd = ATCommandBuilder::build(AtCommand::SetModuleName, {name_str});
    std::string expected = OK_SET + name_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_name(std::string& name, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetModuleName, {});
    std::string expected = OK_GET;
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET: followed by the module name (up to 12 characters)
    // Find the end of the name by looking for \r or \n or end of buffer
    std::size_t start = expected.size();
    std::size_t end = start;
    while (end < response_buffer.size() &&
           response_buffer.at(end) != '\r' &&
           response_buffer.at(end) != '\n' &&
           response_buffer.at(end) != '\0') {
        ++end;
    }
    std::string name_str(end - start, '\0');
    std::memcpy(name_str.data(), response_buffer.data() + start, end - start);
    name = name_str;
    return true;
}

bool HM11Driver::set_notify_information(bool notify, Status& status) {
    std::string notify_str = notify ? "1" : "0";
    std::string cmd = ATCommandBuilder::build(AtCommand::SetNotifyInformation, {notify_str});
    std::string expected = OK_SET + notify_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_notify_information(bool& notify, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetNotifyInformation, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    notify = (response_buffer[OK_GET.size()] == '1');
    return true;
}

bool HM11Driver::set_module_rx_gain(RXGain gain, Status& status) {
    std::string gain_str = std::to_string(static_cast<int>(gain));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetModuleRXGain, {gain_str});
    std::string expected = OK_SET + gain_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_rx_gain(RXGain& gain, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetModuleRXGain, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    gain = static_cast<RXGain>(value);
    return true;
}

// ========== PIO Control Methods ==========

bool HM11Driver::set_pio_output_status(util::PioNumber pio, PIOOutput output, Status& status) {
    std::string output_str = std::to_string(static_cast<int>(output));
    std::string cmd;
    std::string expected;

    if (static_cast<int>(pio) == 1) {
        // Special case for PIO1
        cmd = ATCommandBuilder::build(AtCommand::SetPIOOutputStatus, {"1", output_str});
        expected = OK_SET + output_str;
    } else {
        // PIO 2 and 3: AT+PIO[2|3][0|1]
        std::string pio_str = std::to_string(static_cast<int>(pio));
        cmd = ATCommandBuilder::build(AtCommand::SetPIOOutputStatus, {pio_str, output_str});
        expected = "OK+PIO" + pio_str + ":" + output_str;
    }

    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_pio_output_status(util::PioNumber pio, PIOOutput& output, Status& status) {
    std::string cmd;
    std::string expected;

    if (static_cast<int>(pio) == 1) {
        // Special case for PIO1: AT+PIO1?
        cmd = ATCommandBuilder::build(AtCommand::GetPIOOutputStatus, {"1?"});
        expected = OK_GET;
    } else {
        // PIO 2 and 3: AT+PIO[2|3]?
        std::string pio_str = std::to_string(static_cast<int>(pio));
        cmd = ATCommandBuilder::build(AtCommand::GetPIOOutputStatus, {pio_str, "?"});
        expected = "OK+PIO" + pio_str + ":";
    }

    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    int value = response_buffer.at(expected.size()) - '0';
    output = static_cast<PIOOutput>(value);
    return true;
}

bool HM11Driver::get_pios_output_status(util::PIONumbers& pios, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetPIOsOutputStatus, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::string pios_str(3, '\0');
    std::memcpy(pios_str.data(), response_buffer.data() + OK_GET.size(), 3);
    pios = util::PIONumbers(pios_str);
    return true;
}

bool HM11Driver::set_pios_output_status(const util::PIONumbers& pios, Status& status) {
    std::string pios_str(pios.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetPIOsOutputStatus, {pios_str});
    std::string expected = OK_SET + pios_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::set_pio_collection_rate(util::PioCollectionRate rate, Status& status) {
    std::string rate_str = std::to_string(static_cast<int>(rate));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetPIOCollectionRate, {rate_str});
    std::string expected = OK_SET + rate_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_pio_collection_rate(util::PioCollectionRate& rate, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetPIOCollectionRate, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Parse 2 digit characters from response
    std::string rate_str(2, '\0');
    std::memcpy(rate_str.data(), response_buffer.data() + OK_GET.size(), 2);
    int value = std::stoi(rate_str);
    rate = util::PioCollectionRate(static_cast<std::uint8_t>(value));
    return true;
}

bool HM11Driver::set_power_pin_output(const util::PIONumbers& pios, Status& status) {
    std::string pios_str(pios.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetPowerPinOutput, {pios_str});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_power_pin_output(util::PIONumbers& pios, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetPowerPinOutput, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::string pios_str(3, '\0');
    std::memcpy(pios_str.data(), response_buffer.data() + OK_GET.size(), 3);
    pios = util::PIONumbers(pios_str);
    return true;
}

bool HM11Driver::set_connect_pin_output(const util::PIONumbers& pios, Status& status) {
    std::string pios_str(pios.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetConnectPinOutput, {pios_str});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    return status == Status::Ok;
}

bool HM11Driver::get_connect_pin_output(util::PIONumbers& pios, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetConnectPinOutput, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 3 hex characters from response
    std::string pios_str(3, '\0');
    std::memcpy(pios_str.data(), response_buffer.data() + OK_GET.size(), 3);
    pios = util::PIONumbers(pios_str);
    return true;
}

// ========== Security Methods ==========

bool HM11Driver::set_bond_mode(BondMode mode, Status& status) {
    std::string mode_str = std::to_string(static_cast<int>(mode));
    std::string cmd = ATCommandBuilder::build(AtCommand::SetBondMode, {mode_str});
    std::string expected = OK_SET + mode_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_bond_mode(BondMode& mode, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetBondMode, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    int value = response_buffer[OK_GET.size()] - '0';
    mode = static_cast<BondMode>(value);
    return true;
}

bool HM11Driver::set_pin_code(const util::PINType& pin, Status& status) {
    std::string pin_str(pin.view());
    std::string cmd = ATCommandBuilder::build(AtCommand::SetPINCode, {pin_str});
    std::string expected = OK_SET + pin_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_pin_code(util::PINType& pin, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetPINCode, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Copy 6 digit characters from response
    std::string pin_str(6, '\0');
    std::memcpy(pin_str.data(), response_buffer.data() + OK_GET.size(), 6);
    pin = util::PINType(pin_str);
    return true;
}

} // namespace hm11
