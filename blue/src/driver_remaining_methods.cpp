#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <utility>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/driver.hpp"
#include "hm11/streaming_parser.hpp"
#include "hm11/util/variable_string.hpp"

namespace hm11 {

// ========== Security Methods ==========

bool HM11Driver::remove_bond_information(Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::RemoveBondInformation, {});
    transmit_and_check(cmd_buf.finalize(), "OK+ERASE", status);
    return status == Status::Ok;
}

// ========== iBeacon Query Methods ==========

bool HM11Driver::get_ibeacon_major(util::VersionType& major, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetiBeaconMajor, {});
    std::array<char, 64> expected_buf{};
    constexpr const char* PREFIX = OK_GET;
    constexpr const char* SUFFIX = "0x";
    std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
    std::memcpy(expected_buf.data() + std::strlen(PREFIX), SUFFIX, std::strlen(SUFFIX));
    auto expected = std::string_view(expected_buf.data(), std::strlen(PREFIX) + std::strlen(SUFFIX));
    transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 4 hex characters
    std::array<char, 4> major_arr{};
    std::memcpy(major_arr.data(), response_buffer.data() + expected.size(), 4);
    major = util::VersionType(std::string_view(major_arr.data(), 4));
    return true;
}

bool HM11Driver::get_ibeacon_minor(util::VersionType& minor, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetiBeaconMinor, {});
    std::array<char, 64> expected_buf{};
    constexpr const char* PREFIX = OK_GET;
    constexpr const char* SUFFIX = "0x";
    std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
    std::memcpy(expected_buf.data() + std::strlen(PREFIX), SUFFIX, std::strlen(SUFFIX));
    auto expected = std::string_view(expected_buf.data(), std::strlen(PREFIX) + std::strlen(SUFFIX));
    transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 4 hex characters
    std::array<char, 4> minor_arr{};
    std::memcpy(minor_arr.data(), response_buffer.data() + expected.size(), 4);
    minor = util::VersionType(std::string_view(minor_arr.data(), 4));
    return true;
}

bool HM11Driver::get_ibeacon_measured_power(util::MeasuredPower& power, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetiBeaconMeasuredPower, {});
    std::array<char, 64> expected_buf{};
    constexpr const char* PREFIX = OK_GET;
    constexpr const char* SUFFIX = "0x";
    std::memcpy(expected_buf.data(), PREFIX, std::strlen(PREFIX));
    std::memcpy(expected_buf.data() + std::strlen(PREFIX), SUFFIX, std::strlen(SUFFIX));
    auto expected = std::string_view(expected_buf.data(), std::strlen(PREFIX) + std::strlen(SUFFIX));
    transmit_and_check(cmd_buf.finalize(), expected, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET:0x followed by 2 hex characters
    std::array<char, 2> power_arr{};
    std::memcpy(power_arr.data(), response_buffer.data() + expected.size(), 2);
    power = util::MeasuredPower(std::string_view(power_arr.data(), 2));
    return true;
}

// ========== Power Management Methods ==========

bool HM11Driver::set_module_auto_sleep(bool auto_sleep, Status& status) {
    // IMPORTANT: The Ada source uses inverted boolean logic
    // Ada: S := Image (not Sleep)
    // So if auto_sleep is true, we send "0", if false we send "1"
    constexpr const char* AUTO_SLEEP_STR_0 = "0";
    constexpr const char* AUTO_SLEEP_STR_1 = "1";
    auto auto_sleep_str = auto_sleep ? AUTO_SLEEP_STR_0 : AUTO_SLEEP_STR_1;
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetModuleAutoSleep, {auto_sleep_str});
    std::array<char, 64> expected_buf{};
    std::memcpy(expected_buf.data(), OK_SET, std::strlen(OK_SET));
    std::memcpy(expected_buf.data() + std::strlen(OK_SET), auto_sleep_str, std::strlen(auto_sleep_str));
    auto expected = std::string_view(expected_buf.data(), std::strlen(OK_SET) + std::strlen(auto_sleep_str));
    transmit_and_check(cmd_buf.finalize(), expected, status);
    return status == Status::Ok;
}

bool HM11Driver::get_module_auto_sleep(bool& auto_sleep, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetModuleAutoSleep, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // IMPORTANT: The Ada source uses inverted boolean logic
    // Ada: Sleep := S = '0'
    // So if response is '0', auto_sleep is true, if '1', auto_sleep is false
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto_sleep = (response_buffer[std::strlen(OK_GET)] == '0');
    return true;
}

// ========== HMSensor Methods ==========

bool HM11Driver::get_module_temperature(util::InternalTemperatureType& temperature, Status& status) {
    auto cmd_buf = ATCommandBuilder::build(AtCommand::GetModuleTemperature, {});
    transmit_and_check(cmd_buf.finalize(), OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }
    // Response is OK+GET: followed by 7 digit characters
    std::array<char, 7> temp_arr{};
    std::memcpy(temp_arr.data(), response_buffer.data() + std::strlen(OK_GET), 7);
    temperature = util::InternalTemperatureType(std::string_view(temp_arr.data(), 7));
    return true;
}

// ========== Discovery Streaming Methods ==========

bool HM11Driver::scan(
    DiscoveredCallback callback,
    void* user_data,
    Status& status,
    std::uint32_t timeout_ms) {

    // Adapter to wrap user callback for StreamingParser
    struct CallbackAdapter {
        DiscoveredCallback fn;
        void* user_data;

        static void invoke(const DiscoveredDevice& device, void* adapter_ptr) {
            auto* adapter = static_cast<CallbackAdapter*>(adapter_ptr);
            if (adapter != nullptr && adapter->fn != nullptr) {
                // Call user callback with string_view (no heap allocation)
                adapter->fn(device.mac, device.name_view(), device.rssi_view(), adapter->user_data);
            }
        }
    };

    CallbackAdapter adapter{callback, user_data};

    // Create streaming parser with C-style callback
    StreamingParser parser(&CallbackAdapter::invoke, &adapter);

    // Send scan command
    auto cmd_buf = ATCommandBuilder::build(AtCommand::Scan, {});
    transmit(cmd_buf.finalize(), status);
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
        std::string_view received;
        if (receive(received, status, 512)) {
            // Parse the received data directly (no std::vector allocation)
            scan_complete = parser.parse(reinterpret_cast<const uint8_t*>(received.data()), received.size());

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
    DiscoveredIBeaconCallback callback,
    void* user_data,
    Status& status,
    std::uint32_t timeout_ms) {

    // iBeacon data format after "OK+DISC":
    // FactoryID(8) + UUID(32) + MajorMinorPower(10) + MAC(12) + RSSI(4) = 66 bytes
    constexpr std::size_t IBEACON_DATA_LENGTH = 66;
    constexpr std::string_view OK_DISCS = "OK+DISCS";
    constexpr std::string_view OK_DISC = "OK+DISC";  // Note: No colon for iBeacon
    constexpr std::string_view OK_DISCE = "OK+DISCE";

    // Send scan iBeacon command
    auto cmd_buf = ATCommandBuilder::build(AtCommand::ScaniBeacon, {});
    transmit(cmd_buf.finalize(), status);
    if (status != Status::Ok) {
        return false;
    }

    // Read streaming response with timeout
    auto start_time = std::chrono::steady_clock::now();
    util::VariableString<1024> buffer;  // Fixed-size buffer for accumulation
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
        std::string_view received;
        if (receive(received, status, 512)) {
            buffer.append(received);
            std::string_view buffer_view = buffer.view();

            // Remove OK+DISCS header if present (must do this before looking for OK+DISC)
            std::size_t discs_pos = buffer_view.find(OK_DISCS);
            if (discs_pos != std::string_view::npos) {
                // Shift buffer contents to remove header
                buffer.clear();
                buffer.append(buffer_view.substr(0, discs_pos));
                buffer.append(buffer_view.substr(discs_pos + OK_DISCS.size()));
                buffer_view = buffer.view();
            }

            // Process complete messages in buffer
            while (true) {
                // Look for device data
                std::size_t disc_pos = buffer_view.find(OK_DISC);

                // Check if this is actually OK+DISCE (end marker)
                if (disc_pos != std::string_view::npos) {
                    // Verify it's not the end marker by checking if "OK+DISCE" starts here
                    if (buffer_view.substr(disc_pos, OK_DISCE.size()) == OK_DISCE) {
                        // This is the end marker, not a device
                        scan_complete = true;
                        break;
                    }
                }

                if (disc_pos == std::string_view::npos) {
                    break;  // No more complete devices in buffer
                }

                // Check if we have enough data for complete iBeacon record
                std::size_t data_start = disc_pos + OK_DISC.size();
                if (buffer_view.size() < data_start + IBEACON_DATA_LENGTH) {
                    break;  // Incomplete device data, wait for more
                }

                // Extract iBeacon data as string_view (no heap allocation)
                std::string_view device_data = buffer_view.substr(data_start, IBEACON_DATA_LENGTH);
                callback(device_data, user_data);

                // Remove processed data from buffer by rebuilding it
                std::string_view remaining = buffer_view.substr(data_start + IBEACON_DATA_LENGTH);
                buffer.clear();
                buffer.append(remaining);
                buffer_view = buffer.view();
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
