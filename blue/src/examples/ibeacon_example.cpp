#include <array>
#include <iostream>
#include <string>

#include "hm11/driver.hpp"
#include "hm11/stm32/uart_port.hpp"
#include "hm11/tests/utils/mock_uart_port.hpp"

using namespace hm11;

int main() {
    // Set up mock UART port and driver
    MockUARTPort mock_uart;
    HM11Driver driver(mock_uart);

    // Helper lambda to print result
    auto print_res = [&](const char* name, bool ok) {
        std::cout << name << ": " << (ok ? "OK" : "FAIL") << std::endl;
    };

    // 1. set_ibeacon_major
    mock_uart.set_next_receive(OK_SET);               // response for OK
    std::array<char, 4> major = {'1', '2', '3', '4'}; // example hex
    print_res("set_ibeacon_major", driver.set_ibeacon_major(major));

    // 2. set_ibeacon_minor
    mock_uart.set_next_receive(OK_SET);
    std::array<char, 4> minor = {'5', '6', '7', '8'};
    print_res("set_ibeacon_minor", driver.set_ibeacon_minor(minor));

    // 3. set_ibeacon_measured_power
    mock_uart.set_next_receive(OK_SET);
    std::array<char, 2> power = {'9', 'A'}; // example
    print_res("set_ibeacon_measured_power", driver.set_ibeacon_measured_power(power));

    // 4. set_ibeacon_deploy_mode (BroadcastScanning = 0 -> expected OK+DELO1)
    mock_uart.set_next_receive("OK+DELO1");
    print_res(
        "set_ibeacon_deploy_mode",
        driver.set_ibeacon_deploy_mode(HM11Driver::IBeaconDeployMode::BroadcastScanning));

    // 5. set_ibeacon_switch (enable)
    mock_uart.set_next_receive("OK+IBEA1");
    print_res("set_ibeacon_switch (on)", driver.set_ibeacon_switch(true));

    // 6. get_ibeacon_switch
    mock_uart.set_next_receive("OK+IBEA=1");
    bool enabled = false;
    bool got = driver.get_ibeacon_switch(enabled);
    std::cout << "get_ibeacon_switch: " << (got ? "OK" : "FAIL") << ", value=" << enabled << std::endl;

    // 7. get_ibeacon_uuid (4 chunks, each returning same dummy data)
    // Mock response format: "OK+GET:=<8hex>"
    mock_uart.set_next_receive("OK+GET:=12345678");
    std::array<char, 32> uuid{};
    bool uuid_ok = driver.get_ibeacon_uuid(uuid);
    std::cout << "get_ibeacon_uuid: " << (uuid_ok ? "OK" : "FAIL") << std::endl;
    if (uuid_ok) {
        std::cout << "UUID: ";
        for (char c : uuid)
            std::cout << c;
        std::cout << std::endl;
    }

    return 0;
}
