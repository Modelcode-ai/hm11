// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file streaming_parser.hpp
 * @brief Streaming response parser for HM11 scan operations.
 *
 * Implements a state machine to parse incremental responses from scan operations
 * that return multiple devices over a stream. Handles MAC addresses, device names,
 * and RSSI values that may arrive in fragments across multiple UART reads.
 */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace hm11 {

/**
 * @brief Information about a discovered Bluetooth device.
 */
struct DiscoveredDevice {
    std::array<char, 12> mac{};   // MAC address (12 hex chars, e.g., "001122334455")
    std::array<char, 64> name{};  // Device name (fixed buffer)
    std::size_t name_len{0};      // Actual name length
    std::array<char, 8> rssi{};   // RSSI value as string (fixed buffer)
    std::size_t rssi_len{0};      // Actual RSSI length

    DiscoveredDevice() = default;

    void clear() {
        mac.fill('\0');
        name.fill('\0');
        name_len = 0;
        rssi.fill('\0');
        rssi_len = 0;
    }

    bool has_mac() const {
        return mac[0] != '\0';
    }

    std::string_view name_view() const noexcept {
        return std::string_view(name.data(), name_len);
    }

    std::string_view rssi_view() const noexcept {
        return std::string_view(rssi.data(), rssi_len);
    }
};

/**
 * @brief Callback type for discovered devices (C-style function pointer).
 *
 * @param device The discovered device information.
 * @param user_data Optional user data pointer passed through from parser construction.
 */
using DiscoveredCallback = void (*)(const DiscoveredDevice& device, void* user_data);

/**
 * @brief State machine for parsing streaming scan responses.
 *
 * The HM11 scan operation returns a stream of responses:
 * - OK+DISCS (scan start)
 * - OK+DISC:<MAC> (device MAC address, 12 hex chars)
 * - OK+NAME:<name>\r\n (device name, variable length)
 * - OK+RSSI:<rssi>\r\n (RSSI value, variable length)
 * - OK+DISCE (scan end)
 *
 * This parser handles incremental data arrival and calls the callback
 * for each complete device discovered.
 */
class StreamingParser {
public:
    /**
     * @brief Parser states.
     */
    enum class State : std::uint8_t {
        Selection,  // Looking for next prefix (OK+DISCS, OK+DISC:, OK+NAME:, OK+RSSI:, OK+DISCE)
        MAC,        // Reading MAC address (12 chars)
        Name,       // Reading device name (until \r\n)
        RSSI,       // Reading RSSI value (until \r\n)
        Complete    // Scan complete (OK+DISCE received)
    };

    /**
     * @brief Construct a new streaming parser.
     *
     * @param callback Function to call when a device is discovered.
     * @param user_data Optional user data pointer to pass to callback.
     */
    explicit StreamingParser(DiscoveredCallback callback, void* user_data = nullptr);

    /**
     * @brief Parse a chunk of data from the stream.
     *
     * @param data Pointer to data buffer.
     * @param size Number of bytes in the buffer.
     * @return true if scan is complete (OK+DISCE received), false otherwise.
     */
    bool parse(const std::uint8_t* data, std::size_t size);

    /**
     * @brief Get the current parser state.
     */
    State get_state() const { return m_state; }

    /**
     * @brief Check if scan is complete.
     */
    bool is_complete() const { return m_state == State::Complete; }

    /**
     * @brief Reset the parser to initial state.
     */
    void reset();

    /**
     * @brief Flush any pending device data by calling the callback.
     *
     * Should be called at the end of parsing to ensure the last device
     * is reported if it doesn't have all fields.
     */
    void flush();

private:
    static constexpr std::size_t BUFFER_SIZE = 1024;    // Conservative buffer size
    static constexpr std::size_t MAX_NAME_SIZE = 64;    // Maximum device name length
    static constexpr std::size_t MAX_RSSI_SIZE = 8;     // Maximum RSSI string length

    /**
     * @brief Check if buffer starts with a prefix at current position.
     */
    bool starts_with(std::string_view prefix) const;

    /**
     * @brief Consume bytes from the buffer.
     */
    void consume(std::size_t count);

    /**
     * @brief Find a substring in the buffer.
     * @return Position of substring, or std::string::npos if not found.
     */
    std::size_t find(std::string_view needle) const;

    /**
     * @brief Append data to current field (name or RSSI).
     */
    void append_to_current_field(std::string_view data);

    /**
     * @brief Call the callback with current device and reset it.
     */
    void invoke_callback();

    /**
     * @brief Get current buffer view.
     */
    std::string_view buffer_view() const noexcept {
        return std::string_view(m_buffer.data(), m_buffer_size);
    }

    DiscoveredCallback m_callback;
    void* m_user_data;
    DiscoveredDevice m_current_device;
    State m_state{State::Selection};
    std::array<char, BUFFER_SIZE> m_buffer{};  // Fixed-size buffer for partial data
    std::size_t m_buffer_size{0};              // Current data size in buffer
};

} // namespace hm11
