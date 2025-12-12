#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace hm11 {

enum class Status : uint8_t {
    Ok,
    Error,
    Timeout,
    InvalidCommand,
    InvalidResponse,
    InvalidParameter,
    InvalidLength,
};

/**
 * @brief Abstract UART port interface.
 *
 * Provides virtual methods for transmitting and receiving data. Concrete
 * implementations can use blocking UART, DMA, or mock behavior for testing.
 */
class UARTPort {
  public:
    using ReceiveHandler = std::function<void(std::span<uint8_t> received, Status& status, unsigned int timeout)>;

    UARTPort() = default;
    virtual ~UARTPort() = default;

    /**
     * @brief Transmit data over UART.
     */
    virtual void transmit(std::string_view data, Status& status) = 0;

    /**
     * @brief Receive data from UART.
     */
    virtual void receive(std::span<uint8_t> buffer, Status& status, unsigned int timeout = 1000) = 0;

    /**
     * @brief Install a custom receive handler (optional for concrete types).
     */
    virtual void set_receive_handler(ReceiveHandler h) = 0;
};

} // namespace hm11
