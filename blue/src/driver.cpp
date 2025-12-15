#include "hm11/driver.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <span>
#include <vector>

#include "hm11/ATCommandBuffer.hpp"
#include "hm11/ATCommandBuilder.hpp"
#include "hm11/ATResponseParser.hpp"
#include "hm11/util/numeric_formatter.hpp"

namespace hm11 {

using util::NumericFormatter;

void HM11Driver::run() {
    std::cout << "HM11 Driver Version: 1.0.0\n";
}

bool HM11Driver::send(std::string_view data, Status& status) {
    transmit(data, status);
    return status == Status::Ok;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool HM11Driver::receive(std::string_view& out, Status& status, std::size_t max_len) {
    // Use member receive_buffer_ (1024 bytes)
    constexpr std::size_t MAX_BUFFER_SIZE = 1024;

    // Clamp max_len to buffer size
    std::size_t actual_len = (max_len < MAX_BUFFER_SIZE) ? max_len : MAX_BUFFER_SIZE;
    std::span<uint8_t> buffer_span(receive_buffer_.data(), actual_len);

    // Use a small timeout for individual reads (100ms)
    // The calling code (like scan()) handles overall timeout
    uart.receive(buffer_span, status, 100);

    if (status != Status::Ok) {
        out = std::string_view{};
        return false;
    }

    // Find actual data length by scanning until null byte
    // (HM-11 responses are ASCII, so null marks end of data)
    std::size_t data_len = 0;
    for (std::size_t i = 0; i < actual_len; ++i) {
        if (receive_buffer_[i] == 0) {
            break;
        }
        data_len++;
    }

    // Return string_view into receive_buffer_ (zero heap allocation)
    out = std::string_view(reinterpret_cast<const char*>(receive_buffer_.data()), data_len);
    return true;
}

bool HM11Driver::set_role(unsigned int role) {
    if (role > 1) {
        return false;
    }
    auto cmd_buf = ATCommandBuilder::build(AtCommand::SetRole, {NumericFormatter::format(static_cast<std::uint32_t>(role))});
    Status status = Status::Ok;
    transmit(cmd_buf.finalize(), status);
    return status == Status::Ok;
}

bool HM11Driver::test(Status& status) {
    transmit_and_check("AT", "OK", status);
    return status == Status::Ok;
}

bool HM11Driver::software_version(std::string_view& version, Status& status) {
    transmit("AT+VERS?", status);
    if (status != Status::Ok) {
        return false;
    }

    // Receive response into response_buffer
    receive(response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Find actual length by scanning until null byte
    std::size_t len = 0;
    while (len < response_buffer.size() && response_buffer[len] != 0) {
        ++len;
    }

    // Return string_view pointing into response_buffer (no heap allocation)
    version = std::string_view(reinterpret_cast<const char*>(response_buffer.data()), len);
    return true;
}

bool HM11Driver::get_role(unsigned int& role, Status& status) {
    transmit_and_check("AT+ROLE?", OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse role value from response (OK+GET:<role>)
    std::size_t ok_get_len = std::strlen(OK_GET);
    if (response_buffer[ok_get_len] >= '0' && response_buffer[ok_get_len] <= '1') {
        role = response_buffer[ok_get_len] - '0';
        return true;
    }
    return false;
}

bool HM11Driver::get_mac_address(std::array<char, 12>& mac_address, Status& status) {
    constexpr const char* EXPECT = "OK+ADDR:";
    transmit_and_check("AT+ADDR?", EXPECT, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Copy MAC address from response (OK+ADDR:<12 hex chars>)
    std::copy_n(response_buffer.begin() + std::strlen(EXPECT), 12, mac_address.begin());
    return true;
}

bool HM11Driver::reset(Status& status) {
    transmit_and_check("AT+RENEW", "OK+RENEW", status);
    return status == Status::Ok;
}

bool HM11Driver::restart(Status& status) {
    transmit_and_check("AT+RESET", "OK+RESET", status);
    return status == Status::Ok;
}

bool HM11Driver::disconnect(Status& status) {
    transmit_and_check("AT", "OK+LOST", status);
    return status == Status::Ok;
}

bool HM11Driver::start_working(Status& status) {
    transmit_and_check("AT+START", "OK+START", status);
    return status == Status::Ok;
}

bool HM11Driver::sleep(Status& status) {
    transmit_and_check("AT+SLEEP", "OK+SLEEP", status);
    return status == Status::Ok;
}

bool HM11Driver::wake_up(Status& status) {
    // Wake up command is 7 'W' characters
    constexpr std::string_view CMD = "WWWWWWW";
    transmit_and_check(CMD, "OK+WAKE", status);
    return status == Status::Ok;
}

} // namespace hm11
