#include "hm11/streaming_parser.hpp"

#include <algorithm>
#include <cstring>

namespace hm11 {

// Protocol constants
static constexpr std::string_view OK_DISCS = "OK+DISCS";
static constexpr std::string_view OK_DISC = "OK+DISC:";
static constexpr std::string_view OK_NAME = "OK+NAME:";
static constexpr std::string_view OK_RSSI = "OK+RSSI:";
static constexpr std::string_view OK_DISCE = "OK+DISCE";
static constexpr std::string_view CRLF = "\r\n";
static constexpr std::size_t MAC_LENGTH = 12;

StreamingParser::StreamingParser(DiscoveredCallback callback, void* user_data)
    : m_callback(callback), m_user_data(user_data) {
    // Fixed-size buffer, no allocation needed
}

bool StreamingParser::parse(const std::uint8_t* data, std::size_t size) {
    // Append new data to m_buffer (with overflow protection)
    std::size_t bytes_to_copy = std::min(size, BUFFER_SIZE - m_buffer_size);
    if (bytes_to_copy < size) {
        // Buffer overflow - saturate
        // In production, this could log a warning
    }
    std::memcpy(m_buffer.data() + m_buffer_size, data, bytes_to_copy);
    m_buffer_size += bytes_to_copy;

    while (m_buffer_size > 0 && m_state != State::Complete) {
        switch (m_state) {
            case State::Selection: {
                // Need at least 8 chars for shortest prefix
                if (m_buffer_size < 8) {
                    return false;
                }

                // Check for scan end
                if (starts_with(OK_DISCE)) {
                    invoke_callback();  // Flush last device if any
                    m_state = State::Complete;
                    consume(OK_DISCE.size());
                    return true;
                }

                // Check for scan start
                if (starts_with(OK_DISCS)) {
                    consume(OK_DISCS.size());
                    continue;
                }

                // Check for MAC address
                if (starts_with(OK_DISC)) {
                    // Before reading new MAC, call m_callback with previous device
                    invoke_callback();
                    consume(OK_DISC.size());
                    m_state = State::MAC;
                    continue;
                }

                // Check for device name
                if (starts_with(OK_NAME)) {
                    consume(OK_NAME.size());
                    m_state = State::Name;
                    continue;
                }

                // Check for RSSI
                if (starts_with(OK_RSSI)) {
                    consume(OK_RSSI.size());
                    m_state = State::RSSI;
                    continue;
                }

                // Unknown data, consume one byte and retry
                consume(1);
                break;
            }

            case State::MAC: {
                // Need 12 characters for MAC address
                if (m_buffer_size < MAC_LENGTH) {
                    return false;
                }

                // Read MAC address
                std::memcpy(m_current_device.mac.data(), m_buffer.data(), MAC_LENGTH);
                consume(MAC_LENGTH);
                m_state = State::Selection;
                break;
            }

            case State::Name: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data with saturation
                    std::size_t space_left = MAX_NAME_SIZE - m_current_device.name_len;
                    std::size_t name_bytes = std::min(m_buffer_size, space_left);
                    if (name_bytes < m_buffer_size) {
                        // Name overflow - saturate
                        // In production, this could log a warning
                    }
                    std::memcpy(m_current_device.name.data() + m_current_device.name_len,
                               m_buffer.data(), name_bytes);
                    m_current_device.name_len += name_bytes;
                    consume(m_buffer_size);  // Consume all data
                    return false;
                }

                // Found terminator, extract name with saturation
                std::size_t space_left = MAX_NAME_SIZE - m_current_device.name_len;
                std::size_t name_bytes = std::min(pos, space_left);
                if (name_bytes < pos) {
                    // Name overflow - saturate
                    // In production, this could log a warning
                }
                std::memcpy(m_current_device.name.data() + m_current_device.name_len,
                           m_buffer.data(), name_bytes);
                m_current_device.name_len += name_bytes;
                consume(pos + CRLF.size());
                m_state = State::Selection;
                break;
            }

            case State::RSSI: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data with saturation
                    std::size_t space_left = MAX_RSSI_SIZE - m_current_device.rssi_len;
                    std::size_t rssi_bytes = std::min(m_buffer_size, space_left);
                    if (rssi_bytes < m_buffer_size) {
                        // RSSI overflow - saturate
                        // In production, this could log a warning
                    }
                    std::memcpy(m_current_device.rssi.data() + m_current_device.rssi_len,
                               m_buffer.data(), rssi_bytes);
                    m_current_device.rssi_len += rssi_bytes;
                    consume(m_buffer_size);  // Consume all data
                    return false;
                }

                // Found terminator, extract RSSI with saturation
                std::size_t space_left = MAX_RSSI_SIZE - m_current_device.rssi_len;
                std::size_t rssi_bytes = std::min(pos, space_left);
                if (rssi_bytes < pos) {
                    // RSSI overflow - saturate
                    // In production, this could log a warning
                }
                std::memcpy(m_current_device.rssi.data() + m_current_device.rssi_len,
                           m_buffer.data(), rssi_bytes);
                m_current_device.rssi_len += rssi_bytes;
                consume(pos + CRLF.size());
                m_state = State::Selection;
                break;
            }

            case State::Complete:
                // Should not reach here
                return true;
        }
    }

    return m_state == State::Complete;
}

void StreamingParser::reset() {
    m_state = State::Selection;
    m_current_device.clear();
    m_buffer_size = 0;
}

void StreamingParser::flush() {
    invoke_callback();
}

bool StreamingParser::starts_with(std::string_view prefix) const {
    if (m_buffer_size < prefix.size()) {
        return false;
    }
    return buffer_view().starts_with(prefix);
}

void StreamingParser::consume(std::size_t count) {
    if (count >= m_buffer_size) {
        m_buffer_size = 0;
    } else {
        // Use memmove for compaction (handles overlapping regions)
        std::memmove(m_buffer.data(), m_buffer.data() + count, m_buffer_size - count);
        m_buffer_size -= count;
    }
}

std::size_t StreamingParser::find(std::string_view needle) const {
    return buffer_view().find(needle);
}

void StreamingParser::invoke_callback() {
    if (m_current_device.has_mac() && m_callback != nullptr) {
        m_callback(m_current_device, m_user_data);
        m_current_device.clear();
    }
}

} // namespace hm11
