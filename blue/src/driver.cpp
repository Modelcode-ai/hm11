#include "hm11/driver.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <span>
#include <vector>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/ATResponseParser.hpp"

namespace hm11 {

void HM11Driver::run() {
    std::cout << "HM11 Driver Version: 1.0.0\n";
}

bool HM11Driver::send(std::string_view data, Status& status) {
    transmit(data, status);
    return status == Status::Ok;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool HM11Driver::receive(std::string& out, Status& status, std::size_t max_len) {
    // Create zero-initialized buffer for receiving data
    std::vector<uint8_t> buffer(max_len, 0);
    std::span<uint8_t> buffer_span(buffer.data(), max_len);

    // Use a small timeout for individual reads (100ms)
    // The calling code (like scan()) handles overall timeout
    uart.receive(buffer_span, status, 100);

    if (status != Status::Ok) {
        out.clear();
        return false;
    }

    // Find actual data length by scanning until null byte
    // (HM-11 responses are ASCII, so null marks end of data)
    std::size_t data_len = 0;
    for (unsigned char byte : buffer) {
        if (byte == 0) {
            break;
        }
        data_len++;
    }

    // Convert buffer to string
    out.resize(data_len);
    std::memcpy(out.data(), buffer.data(), data_len);
    return true;
}

bool HM11Driver::set_role(unsigned int role) {
    if (role > 1) {
        return false;
    }
    std::string cmd = ATCommandBuilder::build(AtCommand::SetRole, static_cast<std::uint32_t>(role));
    Status status = Status::Ok;
    transmit(cmd, status);
    return status == Status::Ok;
}

bool HM11Driver::test(Status& status) {
    transmit_and_check("AT", "OK", status);
    return status == Status::Ok;
}

std::string HM11Driver::software_version() {
    Status status = Status::Ok;
    transmit("AT+VERS?", status);
    if (status != Status::Ok) {
        return "";
    }

    // Receive response
    std::string response;
    receive(response, status);
    if (status != Status::Ok) {
        return "";
    }

    return response;
}

bool HM11Driver::get_role(unsigned int& role, Status& status) {
    transmit_and_check("AT+ROLE?", OK_GET, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Parse role value from response (OK+GET:<role>)
    if (response_buffer[OK_GET.size()] >= '0' && response_buffer[OK_GET.size()] <= '1') {
        role = response_buffer[OK_GET.size()] - '0';
        return true;
    }
    return false;
}

bool HM11Driver::get_mac_address(std::array<char, 12>& mac_address, Status& status) {
    const std::string EXPECT = "OK+ADDR:";
    transmit_and_check("AT+ADDR?", EXPECT, response_buffer, status);
    if (status != Status::Ok) {
        return false;
    }

    // Copy MAC address from response (OK+ADDR:<12 hex chars>)
    std::copy_n(response_buffer.begin() + EXPECT.size(), 12, mac_address.begin());
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
    const std::string CMD(7, 'W');
    transmit_and_check(CMD, "OK+WAKE", status);
    return status == Status::Ok;
}

} // namespace hm11
