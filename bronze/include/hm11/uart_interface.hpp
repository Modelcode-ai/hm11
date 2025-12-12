#ifndef HM11_UART_INTERFACE_HPP
#define HM11_UART_INTERFACE_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace hm11 {

/**
 * @brief Status codes for UART operations
 *
 * This enum class represents the possible status results from UART operations,
 * matching the Ada UART_Status type.
 */
enum class UartStatus : std::uint8_t {
    Ok,        ///< Operation completed successfully
    ErrError,  ///< General error
    ErrTimeout ///< Operation timed out
};

/**
 * @brief Concept defining the required interface for UART port implementations
 *
 * This concept specifies the minimum requirements for any UART implementation
 * to be compatible with the HM11Driver. It enforces that any UART implementation
 * must provide transmit and receive methods with the correct signatures.
 */
template <typename T>
concept UartPort = requires(T& uart, std::span<std::uint8_t> data, UartStatus& status) {
    { uart.transmit(data, status) } -> std::same_as<void>;
    { uart.receive(data, status) } -> std::same_as<void>;
};

/**
 * @brief Concept defining the interface for a receive handler
 *
 * This concept specifies the requirements for a receive handler function
 * that can be used with the HM11Driver. It enforces that the handler must
 * provide the correct signature to match the Ada Receive_Handler type.
 */
template <typename T>
concept ReceiveHandler = requires(
    T& handler,
    typename std::remove_reference_t<T>::PortType& port,
    void* received,
    std::size_t length,
    UartStatus& status,
    std::uint32_t timeout,
    bool as_stream) {
    { handler(port, received, length, status, timeout, as_stream) } -> std::same_as<void>;
};

/**
 * @brief Concept defining the interface for a last read position handler
 *
 * This concept specifies the requirements for a last read position handler
 * function that can be used with the HM11Driver. It enforces that the handler
 * must provide the correct signature to match the Ada Last_Read_Position_Handler type.
 */
template <typename T>
concept LastReadPositionHandler = requires(T& handler, bool& closed, std::size_t& zero) {
    { handler(closed, zero) } -> std::same_as<void>;
};

/**
 * @brief Default receive handler implementation
 *
 * This function provides a default implementation for the receive handler,
 * matching the Ada Default_Receive_Handler. It simply forwards the call to
 * the UART port's receive method.
 *
 * @param port The UART port to receive from
 * @param received Pointer to the buffer where received data will be stored
 * @param length The size of the buffer
 * @param status Output parameter that will be set to the operation status
 * @param timeout Optional timeout in milliseconds (default: 1000)
 * @param asStream Optional flag indicating whether to read in stream mode (default: false)
 */
template <typename Port>
void default_receive_handler(
    Port& port,
    void* received,
    std::size_t length,
    UartStatus& status,
    std::uint32_t timeout = 1000,
    bool as_stream = false) {
    // Note: timeout parameter is unused in this implementation
    (void)timeout;

    // as_stream is not supported in the default implementation
    assert(!as_stream && "Stream mode not supported in default receive handler");

    // Create a span over the provided buffer
    auto data = std::span<std::uint8_t>(static_cast<std::uint8_t*>(received), length);

    // Initialize the buffer to zeros
    std::ranges::fill(data, 0);

    // Forward the call to the port's receive method
    port.receive(data, status);
}

} // namespace hm11

#endif // HM11_UART_INTERFACE_HPP