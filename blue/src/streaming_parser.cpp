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
    m_buffer.reserve(512);  // Reserve some space to avoid reallocations
}

bool StreamingParser::parse(const std::uint8_t* data, std::size_t size) {
    // Append new data to m_buffer
    for (std::size_t i = 0; i < size; ++i) {
        m_buffer.push_back(static_cast<char>(data[i]));
    }

    while (!m_buffer.empty() && m_state != State::Complete) {
        switch (m_state) {
            case State::Selection: {
                // Need at least 8 chars for shortest prefix
                if (m_buffer.size() < 8) {
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
                if (m_buffer.size() < MAC_LENGTH) {
                    return false;
                }

                // Read MAC address
                std::copy_n(m_buffer.begin(), MAC_LENGTH, m_current_device.mac.begin());
                consume(MAC_LENGTH);
                m_state = State::Selection;
                break;
            }

            case State::Name: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data
                    m_current_device.name += m_buffer;
                    m_buffer.clear();
                    return false;
                }

                // Found terminator, extract name
                m_current_device.name += m_buffer.substr(0, pos);
                consume(pos + CRLF.size());
                m_state = State::Selection;
                break;
            }

            case State::RSSI: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data
                    m_current_device.rssi += m_buffer;
                    m_buffer.clear();
                    return false;
                }

                // Found terminator, extract RSSI
                m_current_device.rssi += m_buffer.substr(0, pos);
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
    m_buffer.clear();
}

void StreamingParser::flush() {
    invoke_callback();
}

bool StreamingParser::starts_with(std::string_view prefix) const {
    if (m_buffer.size() < prefix.size()) {
        return false;
    }
    return m_buffer.starts_with(prefix);
}

void StreamingParser::consume(std::size_t count) {
    if (count >= m_buffer.size()) {
        m_buffer.clear();
    } else {
        m_buffer.erase(0, count);
    }
}

std::size_t StreamingParser::find(std::string_view needle) const {
    return m_buffer.find(needle);
}

void StreamingParser::invoke_callback() {
    if (m_current_device.has_mac() && m_callback != nullptr) {
        m_callback(m_current_device, m_user_data);
        m_current_device.clear();
    }
}

} // namespace hm11
