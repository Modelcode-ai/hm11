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
    : callback_(callback), user_data_(user_data), current_device_{}, state_(State::Selection), buffer_{} {
    buffer_.reserve(512);  // Reserve some space to avoid reallocations
}

bool StreamingParser::parse(const std::uint8_t* data, std::size_t size) {
    // Append new data to buffer
    buffer_.append(reinterpret_cast<const char*>(data), size);

    while (!buffer_.empty() && state_ != State::Complete) {
        switch (state_) {
            case State::Selection: {
                // Need at least 8 chars for shortest prefix
                if (buffer_.size() < 8) {
                    return false;
                }

                // Check for scan end
                if (starts_with(OK_DISCE)) {
                    invoke_callback();  // Flush last device if any
                    state_ = State::Complete;
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
                    // Before reading new MAC, call callback with previous device
                    invoke_callback();
                    consume(OK_DISC.size());
                    state_ = State::MAC;
                    continue;
                }

                // Check for device name
                if (starts_with(OK_NAME)) {
                    consume(OK_NAME.size());
                    state_ = State::Name;
                    continue;
                }

                // Check for RSSI
                if (starts_with(OK_RSSI)) {
                    consume(OK_RSSI.size());
                    state_ = State::RSSI;
                    continue;
                }

                // Unknown data, consume one byte and retry
                consume(1);
                break;
            }

            case State::MAC: {
                // Need 12 characters for MAC address
                if (buffer_.size() < MAC_LENGTH) {
                    return false;
                }

                // Read MAC address
                std::copy_n(buffer_.begin(), MAC_LENGTH, current_device_.mac.begin());
                consume(MAC_LENGTH);
                state_ = State::Selection;
                break;
            }

            case State::Name: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data
                    current_device_.name += buffer_;
                    buffer_.clear();
                    return false;
                }

                // Found terminator, extract name
                current_device_.name += buffer_.substr(0, pos);
                consume(pos + CRLF.size());
                state_ = State::Selection;
                break;
            }

            case State::RSSI: {
                // Look for \r\n terminator
                std::size_t pos = find(CRLF);
                if (pos == std::string::npos) {
                    // No terminator yet, accumulate all data
                    current_device_.rssi += buffer_;
                    buffer_.clear();
                    return false;
                }

                // Found terminator, extract RSSI
                current_device_.rssi += buffer_.substr(0, pos);
                consume(pos + CRLF.size());
                state_ = State::Selection;
                break;
            }

            case State::Complete:
                // Should not reach here
                return true;
        }
    }

    return state_ == State::Complete;
}

void StreamingParser::reset() {
    state_ = State::Selection;
    current_device_.clear();
    buffer_.clear();
}

void StreamingParser::flush() {
    invoke_callback();
}

bool StreamingParser::starts_with(std::string_view prefix) const {
    if (buffer_.size() < prefix.size()) {
        return false;
    }
    return buffer_.compare(0, prefix.size(), prefix) == 0;
}

void StreamingParser::consume(std::size_t count) {
    if (count >= buffer_.size()) {
        buffer_.clear();
    } else {
        buffer_.erase(0, count);
    }
}

std::size_t StreamingParser::find(std::string_view needle) const {
    return buffer_.find(needle);
}

void StreamingParser::invoke_callback() {
    if (current_device_.has_mac() && callback_) {
        callback_(current_device_, user_data_);
        current_device_.clear();
    }
}

} // namespace hm11
