#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <utility>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/streaming_parser.hpp"

namespace hm11 {

// ========== Security Methods ==========

bool HM11Driver::remove_bond_information(Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::RemoveBondInformation, {});
    transmit_and_check(cmd, "OK+ERASE", status);
    return status == Status::Ok;
}

// ========== iBeacon Query Methods ==========

bool HM11Driver::get_ibeacon_major(util::VersionType& major, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetiBeaconMajor, {});
    std::string expected = OK_GET + "0x";
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 4 hex characters
    std::size_t start = expected.size();
    std::string major_str(4, '\0');
    std::memcpy(major_str.data(), response_buffer.data() + start, 4);
    major = util::VersionType(major_str);
    return true;
}

bool HM11Driver::get_ibeacon_minor(util::VersionType& minor, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetiBeaconMinor, {});
    std::string expected = OK_GET + "0x";
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 4 hex characters
    std::size_t start = expected.size();
    std::string minor_str(4, '\0');
    std::memcpy(minor_str.data(), response_buffer.data() + start, 4);
    minor = util::VersionType(minor_str);
    return true;
}

bool HM11Driver::get_ibeacon_measured_power(util::MeasuredPower& power, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetiBeaconMeasuredPower, {});
    std::string expected = OK_GET + "0x";
    transmit_and_check(cmd, expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 2 hex characters
    std::size_t start = expected.size();
    std::string power_str(2, '\0');
    std::memcpy(power_str.data(), response_buffer.data() + start, 2);
    power = util::MeasuredPower(power_str);
    return true;
}

// ========== Power Management Methods ==========

bool HM11Driver::set_module_auto_sleep(bool auto_sleep, Status& status) {
    // IMPORTANT: The Ada source uses inverted boolean logic
    // Ada: S := Image (not Sleep)
    // So if auto_sleep is true, we send "0", if false we send "1"
    std::string auto_sleep_str = auto_sleep ? "0" : "1";
    std::string cmd = ATCommandBuilder::build(AtCommand::SetModuleAutoSleep, {auto_sleep_str});
    std::string expected = OK_SET + auto_sleep_str;
    transmit_and_check(cmd, expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_auto_sleep(bool& auto_sleep, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetModuleAutoSleep, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // IMPORTANT: The Ada source uses inverted boolean logic
    // Ada: Sleep := S = '0'
    // So if response is '0', auto_sleep is true, if '1', auto_sleep is false
    auto_sleep = (response_buffer[OK_GET.size()] == '0');
    return true;
}

// ========== HMSensor Methods ==========

bool HM11Driver::get_module_temperature(util::InternalTemperatureType& temperature, Status& status) {
    std::string cmd = ATCommandBuilder::build(AtCommand::GetModuleTemperature, {});
    transmit_and_check(cmd, OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET: followed by 7 digit characters
    std::string temp_str(7, '\0');
    std::memcpy(temp_str.data(), response_buffer.data() + OK_GET.size(), 7);
    temperature = util::InternalTemperatureType(temp_str);
    return true;
}

// ========== Discovery Streaming Methods ==========

bool HM11Driver::scan(
    std::function<void(const std::array<char, 12>& mac, const std::string& name, const std::string& rssi)> callback,
    Status& status,
    std::uint32_t timeout_ms) {

    // Adapter to convert std::function to C-style callback for StreamingParser
    struct CallbackAdapter {
        std::function<void(const std::array<char, 12>&, const std::string&, const std::string&)>* fn;

        static void invoke(const DiscoveredDevice& device, void* user_data) {
            auto* adapter = static_cast<CallbackAdapter*>(user_data);
            if (adapter != nullptr && adapter->fn != nullptr) {
                (*adapter->fn)(device.mac, device.name, device.rssi);
            }
        }
    };

    CallbackAdapter adapter{&callback};

    // Create streaming parser with C-style callback
    StreamingParser parser(&CallbackAdapter::invoke, &adapter);

    // Send scan command
    std::string cmd = ATCommandBuilder::build(AtCommand::Scan, {});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Read streaming response with timeout
    auto start_time = std::chrono::steady_clock::now();
    bool scan_complete = false;

    while (!scan_complete) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (std::cmp_greater(elapsed, timeout_ms)) {
            status = Status::Timeout;
            parser.flush();  // Flush any pending device
            return false;
        }

        // Try to receive data (non-blocking with small timeout)
        std::string received;
        if (receive(received, status, 512)) {
            // Parse the received data
            std::vector<std::uint8_t> received_bytes(received.begin(), received.end());
            scan_complete = parser.parse(received_bytes.data(), received_bytes.size());

            if (parser.is_complete()) {
                status = Status::Ok;
                return true;
            }
        } else if (status == Status::Timeout) {
            // Small timeout between reads, continue
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            status = Status::Ok;  // Reset for next read
        } else {
            // Real error
            parser.flush();  // Flush any pending device
            return false;
        }
    }

    parser.flush();  // Flush any pending device
    status = Status::Ok;
    return true;
}

bool HM11Driver::scan_ibeacon(
    const std::function<void(const std::string& device_data)>& callback,
    Status& status,
    std::uint32_t timeout_ms) {

    // iBeacon data format after "OK+DISC":
    // FactoryID(8) + UUID(32) + MajorMinorPower(10) + MAC(12) + RSSI(4) = 66 bytes
    constexpr std::size_t IBEACON_DATA_LENGTH = 66;
    constexpr const char* OK_DISCS = "OK+DISCS";
    constexpr const char* OK_DISC = "OK+DISC";  // Note: No colon for iBeacon
    constexpr const char* OK_DISCE = "OK+DISCE";

    // Send scan iBeacon command
    std::string cmd = ATCommandBuilder::build(AtCommand::ScaniBeacon, {});
    transmit(cmd, status);
    if (status != Status::Ok) {
        return false;
    }

    // Read streaming response with timeout
    auto start_time = std::chrono::steady_clock::now();
    std::string buffer;
    bool scan_complete = false;

    while (!scan_complete) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (std::cmp_greater(elapsed, timeout_ms)) {
            status = Status::Timeout;
            return false;
        }

        // Try to receive data
        std::string received;
        if (receive(received, status, 512)) {
            buffer += received;

            // Remove OK+DISCS header if present (must do this before looking for OK+DISC)
            std::size_t discs_pos = buffer.find(OK_DISCS);
            if (discs_pos != std::string::npos) {
                buffer.erase(discs_pos, std::strlen(OK_DISCS));
            }

            // Process complete messages in buffer
            while (true) {
                // Look for device data
                std::size_t disc_pos = buffer.find(OK_DISC);

                // Check if this is actually OK+DISCE (end marker)
                if (disc_pos != std::string::npos) {
                    // Verify it's not the end marker by checking if "OK+DISCE" starts here
                    if (buffer.substr(disc_pos, std::strlen(OK_DISCE)) == OK_DISCE) {
                        // This is the end marker, not a device
                        scan_complete = true;
                        break;
                    }
                }

                if (disc_pos == std::string::npos) {
                    break;  // No more complete devices in buffer
                }

                // Check if we have enough data for complete iBeacon record
                std::size_t data_start = disc_pos + std::strlen(OK_DISC);
                if (buffer.size() < data_start + IBEACON_DATA_LENGTH) {
                    break;  // Incomplete device data, wait for more
                }

                // Extract iBeacon data
                std::string device_data = buffer.substr(data_start, IBEACON_DATA_LENGTH);
                callback(device_data);

                // Remove processed data from buffer
                buffer.erase(0, data_start + IBEACON_DATA_LENGTH);
            }

            // If scan is complete, exit immediately
            if (scan_complete) {
                status = Status::Ok;
                return true;
            }

        } else if (status == Status::Timeout) {
            // Small timeout between reads, continue
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            status = Status::Ok;  // Reset for next read
        } else {
            // Real error
            return false;
        }
    }

    status = Status::Ok;
    return true;
}

} // namespace hm11
