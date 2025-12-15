// SPDX-FileCopyrightText: 2025
// SPDX-License-Identifier: MIT

/**
 * @file ATCommandBuffer.hpp
 * @brief Fixed-size command buffer for AT commands without heap allocation.
 *
 * Provides a fixed-buffer command builder suitable for embedded systems
 * with constrained memory and real-time requirements.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace hm11 {

/**
 * @brief Fixed-size buffer for building AT command strings without heap allocation.
 *
 * This class provides a stack-allocated buffer for building AT command strings.
 * All operations are noexcept and use zero dynamic memory allocation, making it
 * suitable for embedded systems and real-time applications.
 *
 * @tparam BufferSize Maximum command size in bytes (default: 128).
 *                    Conservative sizing provides 3x margin over typical commands.
 *
 * @par Thread Safety
 * Not thread-safe. Each thread should use its own instance.
 *
 * @par Performance
 * All operations are O(n) where n is the string length being appended.
 * No dynamic memory allocation. Stack-only operations.
 *
 * @par Overflow Handling
 * When the buffer is full, append operations saturate (write what fits) and
 * set an overflow flag. Check overflow() after building commands.
 *
 * @par Example Usage
 * @code
 * // Method 1: Fluent interface
 * ATCommandBuffer<128> cmd;
 * cmd.reset(AtCommand::SetRole);
 * cmd.append("1").finalize();
 * uart.transmit(cmd.view());
 *
 * // Method 2: Build in one line
 * auto cmd = ATCommandBuffer<128>()
 *     .reset(AtCommand::SetBaudRate)
 *     .append(NumericFormatter::format(9600))
 *     .finalize();
 * uart.transmit(cmd.view());
 * @endcode
 */
template <std::size_t BufferSize = 128>
class ATCommandBuffer {
  public:
    /**
     * @brief Default constructor creates an empty buffer.
     */
    constexpr ATCommandBuffer() noexcept = default;

    /**
     * @brief Reset buffer and initialize with a command base string.
     *
     * Clears any previous content and writes the command base string.
     *
     * @param cmd_base The command base string to initialize with (e.g., "AT+ROLE").
     * @return Reference to this buffer for chaining.
     */
    ATCommandBuffer& reset(std::string_view cmd_base) noexcept {
        m_size = 0;
        m_overflow = false;
        append_str(cmd_base);
        return *this;
    }

    /**
     * @brief Append a single character to the buffer.
     *
     * @param c The character to append.
     * @return Reference to this buffer for chaining.
     */
    ATCommandBuffer& append(char c) noexcept {
        if (m_size < BufferSize) {
            m_data[m_size++] = c;
        } else {
            m_overflow = true;
        }
        return *this;
    }

    /**
     * @brief Append a string_view to the buffer.
     *
     * If the buffer doesn't have enough space, writes what fits and sets
     * the overflow flag.
     *
     * @param sv The string to append.
     * @return Reference to this buffer for chaining.
     */
    ATCommandBuffer& append(std::string_view sv) noexcept {
        append_str(sv);
        return *this;
    }

    /**
     * @brief Finalize the command by appending CR LF terminator.
     *
     * Appends "\\r\\n" to the buffer and returns a view of the complete command.
     *
     * @return std::string_view of the complete command including terminator.
     */
    std::string_view finalize() noexcept {
        append_str("\r\n");
        return view();
    }

    /**
     * @brief Get a string_view of the current buffer contents.
     *
     * @return std::string_view pointing to the buffer data.
     */
    constexpr std::string_view view() const noexcept {
        return std::string_view(m_data.data(), m_size);
    }

    /**
     * @brief Get a null-terminated C-string of the buffer contents.
     *
     * Adds a null terminator after the current content (safe due to BufferSize+1 allocation).
     *
     * @return Pointer to null-terminated string.
     */
    const char* c_str() const noexcept {
        m_data[m_size] = '\0';
        return m_data.data();
    }

    /**
     * @brief Get the current size of the buffer contents.
     *
     * @return Number of bytes currently in the buffer.
     */
    constexpr std::size_t size() const noexcept { return m_size; }

    /**
     * @brief Check if an overflow has occurred.
     *
     * An overflow occurs when append operations try to write beyond BufferSize.
     * The buffer saturates (writes what fits) and sets this flag.
     *
     * @return true if overflow occurred, false otherwise.
     */
    constexpr bool overflow() const noexcept { return m_overflow; }

    /**
     * @brief Check if the buffer is empty.
     *
     * @return true if size is 0, false otherwise.
     */
    constexpr bool empty() const noexcept { return m_size == 0; }

    /**
     * @brief Get the maximum capacity of the buffer.
     *
     * @return BufferSize template parameter.
     */
    static constexpr std::size_t capacity() noexcept { return BufferSize; }

  private:
    /**
     * @brief Internal append with saturation behavior.
     *
     * Appends as much of sv as fits in the buffer. If buffer is full,
     * sets overflow flag.
     */
    void append_str(std::string_view sv) noexcept {
        if (sv.empty()) {
            return;
        }

        const std::size_t AVAILABLE = BufferSize - m_size;
        const std::size_t TO_COPY = (sv.size() <= AVAILABLE) ? sv.size() : AVAILABLE;

        if (TO_COPY > 0) {
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
            std::memcpy(m_data.data() + m_size, sv.data(), TO_COPY);
            m_size += TO_COPY;
        }

        if (TO_COPY < sv.size()) {
            m_overflow = true;
        }
    }

    // +1 for null terminator in c_str()
    mutable std::array<char, BufferSize + 1> m_data{};
    std::size_t m_size{0};
    bool m_overflow{false};
};

} // namespace hm11
