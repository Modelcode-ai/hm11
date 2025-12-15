/**
 * @file advanced_settings.cpp
 * @brief Implementation of advanced settings operations for HM11Driver
 *
 * This file provides the implementation of advanced configuration operations
 * including advertising, work modes, PIO control, iBeacon, and bonding settings.
 */

#include "../include/hm11/hm11_driver.hpp"
#include "../include/hm11/enum_parsers.hpp"

namespace hm11 {

// ============================================================================
// Advertising Configuration
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_advertising_interval(
    AdvertisingInterval interval, UartStatus& status) {
    const char interval_char = to_char(interval);
    const std::string cmd = std::string("AT+ADVI") + interval_char;
    const std::string expected = std::string(OK_SET) + interval_char;
    transmit_and_check(cmd, expected, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_advertising_interval(
    AdvertisingInterval& interval, UartStatus& status) {
    transmit_and_check(
        "AT+ADVI?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        interval = from_char<AdvertisingInterval>(value_char);
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_advertising_type(
    AdvertisingType type, UartStatus& status) {
    const char type_char = to_char(type);
    const std::string cmd = std::string("AT+ADTY") + type_char;
    const std::string expected = std::string(OK_SET) + type_char;
    transmit_and_check(cmd, expected, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_advertising_type(
    AdvertisingType& type, UartStatus& status) {
    transmit_and_check(
        "AT+ADTY?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        type = from_char<AdvertisingType>(value_char);
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_reliable_advertising_mode(
    AdvertisingMode mode, UartStatus& status) {
    const char mode_char = to_char(mode);
    const std::string cmd = std::string("AT+RELI") + mode_char;
    const std::string expected = std::string(OK_SET) + mode_char;
    transmit_and_check(cmd, expected, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_reliable_advertising_mode(
    AdvertisingMode& mode, UartStatus& status) {
    transmit_and_check(
        "AT+RELI?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        mode = from_char<AdvertisingMode>(value_char);
    }
}

// ============================================================================
// White List Management
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_white_list_switch(
    bool enable, UartStatus& status) {
    const std::string value_str = enable ? "1" : "0";
    transmit_and_check("AT+ALLO" + value_str, std::string(OK_SET) + value_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_white_list_switch(
    bool& enabled, UartStatus& status) {
    transmit_and_check(
        "AT+ALLO?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        enabled = (value_char == '1');
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_white_list_mac_addresses(
    util::MacWhiteListIndex index, const util::MacAddress& mac, UartStatus& status) {
    const std::string index_str = std::to_string(static_cast<int>(index.get_value()));
    const std::string mac_str = mac.to_string();
    const std::string cmd = "AT+AD" + index_str + mac_str;
    const std::string expected = std::string(OK_SET) + mac_str;
    transmit_and_check(cmd, expected, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_white_list_mac_address(
    util::MacWhiteListIndex index, util::MacAddress& mac, UartStatus& status) {
    const std::string index_str = std::to_string(static_cast<int>(index.get_value()));
    const std::string cmd = "AT+AD" + index_str + "?";
    constexpr std::size_t MAC_LENGTH = 12;

    transmit_and_check(
        cmd,
        OK_GET,
        m_response.data(),
        OK_GET.size() + MAC_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract MAC address from response
        for (std::size_t i = 0; i < MAC_LENGTH; ++i) {
            mac[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

// ============================================================================
// Security and Bonding
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_pin_code(
    const util::PinType& pin, UartStatus& status) {
    const std::string pin_str = pin.to_string();
    transmit_and_check("AT+PASS" + pin_str, std::string(OK_SET) + pin_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_pin_code(
    util::PinType& pin, UartStatus& status) {
    constexpr std::size_t PIN_LENGTH = 6;

    transmit_and_check(
        "AT+PASS?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + PIN_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract PIN from response
        for (std::size_t i = 0; i < PIN_LENGTH; ++i) {
            pin[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_bond_mode(
    BondMode mode, UartStatus& status) {
    const std::string mode_str = std::to_string(static_cast<int>(mode));
    transmit_and_check("AT+TYPE" + mode_str, std::string(OK_SET) + mode_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_bond_mode(
    BondMode& mode, UartStatus& status) {
    transmit_and_check(
        "AT+TYPE?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '0' && value_char <= '3') {
            mode = static_cast<BondMode>(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::remove_bond_information(
    UartStatus& status) {
    transmit_and_check("AT+ERASE", "OK+ERASE", status);
}

// ============================================================================
// Work Mode and Type Configuration
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_work_mode(
    WorkMode mode, UartStatus& status) {
    const std::string mode_str = std::to_string(static_cast<int>(mode));
    transmit_and_check("AT+MODE" + mode_str, std::string(OK_SET) + mode_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_work_mode(
    WorkMode& mode, UartStatus& status) {
    transmit_and_check(
        "AT+MODE?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '0' && value_char <= '2') {
            mode = static_cast<WorkMode>(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_work_type(
    WorkType type, UartStatus& status) {
    const std::string type_str = std::to_string(static_cast<int>(type));
    transmit_and_check("AT+IMME" + type_str, std::string(OK_SET) + type_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_work_type(
    WorkType& type, UartStatus& status) {
    transmit_and_check(
        "AT+IMME?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '0' && value_char <= '1') {
            type = static_cast<WorkType>(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

// ============================================================================
// Notification Configuration
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_notify_information(
    bool enable, UartStatus& status) {
    const std::string value_str = enable ? "1" : "0";
    transmit_and_check("AT+NOTI" + value_str, std::string(OK_SET) + value_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_notify_information(
    bool& enabled, UartStatus& status) {
    transmit_and_check(
        "AT+NOTI?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        enabled = (value_char == '1');
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_notify_mode(
    NotifyMode mode, UartStatus& status) {
    const std::string mode_str = std::to_string(static_cast<int>(mode));
    transmit_and_check("AT+NOTP" + mode_str, std::string(OK_SET) + mode_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_notify_mode(
    NotifyMode& mode, UartStatus& status) {
    transmit_and_check(
        "AT+NOTP?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        if (value_char >= '0' && value_char <= '1') {
            mode = static_cast<NotifyMode>(value_char - '0');
        } else {
            status = UartStatus::ErrError;
        }
    }
}

// ============================================================================
// PIO/GPIO Control
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_pio_output_status(
    util::PioNumber pio, bool high, UartStatus& status) {
    const std::string pio_str = std::to_string(pio.get_value());
    const std::string value_str = high ? "1" : "0";
    const std::string cmd = "AT+PIO" + pio_str + value_str;
    transmit_and_check(cmd, "OK+Set:" + pio_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_pio_output_status(
    util::PioNumber pio, bool& high, UartStatus& status) {
    const std::string pio_str = std::to_string(pio.get_value());
    const std::string cmd = "AT+PIO" + pio_str + "?";

    transmit_and_check(
        cmd,
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        high = (value_char == '1');
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_pios_output_status(
    const util::PioNumbers& pios, UartStatus& status) {
    const std::string pios_str = pios.to_string();
    transmit_and_check("AT+PIO" + pios_str, std::string(OK_SET) + pios_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_pios_output_status(
    util::PioNumbers& pios, UartStatus& status) {
    constexpr std::size_t PIO_LENGTH = 3;

    transmit_and_check(
        "AT+PIO?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + PIO_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        // Extract PIO values from response
        for (std::size_t i = 0; i < PIO_LENGTH; ++i) {
            pios[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_pio_collection_rate(
    util::PioCollectionRate rate, UartStatus& status) {
    char rate_str[3];
    std::snprintf(rate_str, sizeof(rate_str), "%02d", rate.get_value());
    const std::string cmd = std::string("AT+RATE") + rate_str;
    const std::string expected = std::string(OK_SET) + rate_str;
    transmit_and_check(cmd, expected, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_pio_collection_rate(
    util::PioCollectionRate& rate, UartStatus& status) {
    transmit_and_check(
        "AT+RATE?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 2,
        status
    );

    if (status == UartStatus::Ok) {
        // Parse 2-digit decimal number
        const char tens = static_cast<char>(m_response[OK_GET.size()]);
        const char ones = static_cast<char>(m_response[OK_GET.size() + 1]);

        if (tens >= '0' && tens <= '9' && ones >= '0' && ones <= '9') {
            const int value = (tens - '0') * 10 + (ones - '0');
            if (value <= 99) {
                rate = util::PioCollectionRate(value);
            } else {
                status = UartStatus::ErrError;
            }
        } else {
            status = UartStatus::ErrError;
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_power_pin_output(
    const util::PioNumbers& pios, UartStatus& status) {
    const std::string pios_str = pios.to_string();
    transmit_and_check("AT+PWRM" + pios_str, std::string(OK_SET) + pios_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_power_pin_output(
    util::PioNumbers& pios, UartStatus& status) {
    constexpr std::size_t PIO_LENGTH = 3;

    transmit_and_check(
        "AT+PWRM?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + PIO_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < PIO_LENGTH; ++i) {
            pios[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_connect_pin_output(
    const util::PioNumbers& pios, UartStatus& status) {
    const std::string pios_str = pios.to_string();
    transmit_and_check("AT+POWE" + pios_str, std::string(OK_SET) + pios_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_connect_pin_output(
    util::PioNumbers& pios, UartStatus& status) {
    constexpr std::size_t PIO_LENGTH = 3;

    transmit_and_check(
        "AT+POWE?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + PIO_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < PIO_LENGTH; ++i) {
            pios[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

// ============================================================================
// iBeacon Configuration
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_ibeacon_switch(
    bool enable, UartStatus& status) {
    const std::string value_str = enable ? "1" : "0";
    transmit_and_check("AT+IBEA" + value_str, std::string(OK_SET) + value_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_ibeacon_switch(
    bool& enabled, UartStatus& status) {
    transmit_and_check(
        "AT+IBEA?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + 1,
        status
    );

    if (status == UartStatus::Ok) {
        const char value_char = static_cast<char>(m_response[OK_GET.size()]);
        enabled = (value_char == '1');
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_ibeacon_uuid(
    const util::IBeaconUuid& uuid, UartStatus& status) {
    const std::string uuid_str = uuid.to_string();
    transmit_and_check("AT+IBE0" + uuid_str, std::string(OK_SET) + uuid_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_ibeacon_uuid(
    util::IBeaconUuid& uuid, UartStatus& status) {
    constexpr std::size_t UUID_LENGTH = 32;

    transmit_and_check(
        "AT+IBE0?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + UUID_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < UUID_LENGTH; ++i) {
            uuid[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_ibeacon_major_version(
    const util::VersionType& version, UartStatus& status) {
    const std::string version_str = version.to_string();
    transmit_and_check("AT+MARJ0x" + version_str, std::string(OK_SET) + "0x" + version_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_ibeacon_major_version(
    util::VersionType& version, UartStatus& status) {
    constexpr std::size_t VERSION_LENGTH = 4;

    transmit_and_check(
        "AT+MARJ?",
        "OK+Get:0x",
        m_response.data(),
        10 + VERSION_LENGTH,  // "OK+Get:0x" + version
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < VERSION_LENGTH; ++i) {
            version[i] = static_cast<char>(m_response[10 + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_ibeacon_minor_version(
    const util::VersionType& version, UartStatus& status) {
    const std::string version_str = version.to_string();
    transmit_and_check("AT+MINO0x" + version_str, std::string(OK_SET) + "0x" + version_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_ibeacon_minor_version(
    util::VersionType& version, UartStatus& status) {
    constexpr std::size_t VERSION_LENGTH = 4;

    transmit_and_check(
        "AT+MINO?",
        "OK+Get:0x",
        m_response.data(),
        10 + VERSION_LENGTH,  // "OK+Get:0x" + version
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < VERSION_LENGTH; ++i) {
            version[i] = static_cast<char>(m_response[10 + i]);
        }
    }
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::set_ibeacon_measured_power(
    const util::MeasuredPower& power, UartStatus& status) {
    const std::string power_str = power.to_string();
    transmit_and_check("AT+MEAS" + power_str, std::string(OK_SET) + power_str, status);
}

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_ibeacon_measured_power(
    util::MeasuredPower& power, UartStatus& status) {
    constexpr std::size_t POWER_LENGTH = 2;

    transmit_and_check(
        "AT+MEAS?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + POWER_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < POWER_LENGTH; ++i) {
            power[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

// ============================================================================
// Temperature Monitoring
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::get_module_temperature(
    util::InternalTemperatureType& temperature, UartStatus& status) {
    constexpr std::size_t TEMP_LENGTH = 7;

    transmit_and_check(
        "AT+TEMP?",
        OK_GET,
        m_response.data(),
        OK_GET.size() + TEMP_LENGTH,
        status
    );

    if (status == UartStatus::Ok) {
        for (std::size_t i = 0; i < TEMP_LENGTH; ++i) {
            temperature[i] = static_cast<char>(m_response[OK_GET.size() + i]);
        }
    }
}

// ============================================================================
// GATT Services Discovery
// ============================================================================

template <UartPort Port, ReceiveHandler ReceiveHandlerImpl, LastReadPositionHandler ReadPositionHandlerImpl>
void HM11Driver<Port, ReceiveHandlerImpl, ReadPositionHandlerImpl>::find_all_services_uuid(
    UartStatus& status) {
    // This command initiates service discovery, results are streamed
    transmit("AT+FINDA?", status);
}

// Explicit template instantiations for PlaceholderDriver
template void PlaceholderDriver::set_advertising_interval(AdvertisingInterval, UartStatus&);
template void PlaceholderDriver::get_advertising_interval(AdvertisingInterval&, UartStatus&);
template void PlaceholderDriver::set_advertising_type(AdvertisingType, UartStatus&);
template void PlaceholderDriver::get_advertising_type(AdvertisingType&, UartStatus&);
template void PlaceholderDriver::set_reliable_advertising_mode(AdvertisingMode, UartStatus&);
template void PlaceholderDriver::get_reliable_advertising_mode(AdvertisingMode&, UartStatus&);

template void PlaceholderDriver::set_white_list_switch(bool, UartStatus&);
template void PlaceholderDriver::get_white_list_switch(bool&, UartStatus&);
template void PlaceholderDriver::set_white_list_mac_addresses(util::MacWhiteListIndex, const util::MacAddress&, UartStatus&);
template void PlaceholderDriver::get_white_list_mac_address(util::MacWhiteListIndex, util::MacAddress&, UartStatus&);

template void PlaceholderDriver::set_pin_code(const util::PinType&, UartStatus&);
template void PlaceholderDriver::get_pin_code(util::PinType&, UartStatus&);
template void PlaceholderDriver::set_bond_mode(BondMode, UartStatus&);
template void PlaceholderDriver::get_bond_mode(BondMode&, UartStatus&);
template void PlaceholderDriver::remove_bond_information(UartStatus&);

template void PlaceholderDriver::set_work_mode(WorkMode, UartStatus&);
template void PlaceholderDriver::get_work_mode(WorkMode&, UartStatus&);
template void PlaceholderDriver::set_work_type(WorkType, UartStatus&);
template void PlaceholderDriver::get_work_type(WorkType&, UartStatus&);

template void PlaceholderDriver::set_notify_information(bool, UartStatus&);
template void PlaceholderDriver::get_notify_information(bool&, UartStatus&);
template void PlaceholderDriver::set_notify_mode(NotifyMode, UartStatus&);
template void PlaceholderDriver::get_notify_mode(NotifyMode&, UartStatus&);

template void PlaceholderDriver::set_pio_output_status(util::PioNumber, bool, UartStatus&);
template void PlaceholderDriver::get_pio_output_status(util::PioNumber, bool&, UartStatus&);
template void PlaceholderDriver::set_pios_output_status(const util::PioNumbers&, UartStatus&);
template void PlaceholderDriver::get_pios_output_status(util::PioNumbers&, UartStatus&);
template void PlaceholderDriver::set_pio_collection_rate(util::PioCollectionRate, UartStatus&);
template void PlaceholderDriver::get_pio_collection_rate(util::PioCollectionRate&, UartStatus&);
template void PlaceholderDriver::set_power_pin_output(const util::PioNumbers&, UartStatus&);
template void PlaceholderDriver::get_power_pin_output(util::PioNumbers&, UartStatus&);
template void PlaceholderDriver::set_connect_pin_output(const util::PioNumbers&, UartStatus&);
template void PlaceholderDriver::get_connect_pin_output(util::PioNumbers&, UartStatus&);

template void PlaceholderDriver::set_ibeacon_switch(bool, UartStatus&);
template void PlaceholderDriver::get_ibeacon_switch(bool&, UartStatus&);
template void PlaceholderDriver::set_ibeacon_uuid(const util::IBeaconUuid&, UartStatus&);
template void PlaceholderDriver::get_ibeacon_uuid(util::IBeaconUuid&, UartStatus&);
template void PlaceholderDriver::set_ibeacon_major_version(const util::VersionType&, UartStatus&);
template void PlaceholderDriver::get_ibeacon_major_version(util::VersionType&, UartStatus&);
template void PlaceholderDriver::set_ibeacon_minor_version(const util::VersionType&, UartStatus&);
template void PlaceholderDriver::get_ibeacon_minor_version(util::VersionType&, UartStatus&);
template void PlaceholderDriver::set_ibeacon_measured_power(const util::MeasuredPower&, UartStatus&);
template void PlaceholderDriver::get_ibeacon_measured_power(util::MeasuredPower&, UartStatus&);

template void PlaceholderDriver::get_module_temperature(util::InternalTemperatureType&, UartStatus&);
template void PlaceholderDriver::find_all_services_uuid(UartStatus&);

} // namespace hm11
