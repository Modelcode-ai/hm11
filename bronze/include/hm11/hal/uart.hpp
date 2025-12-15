/**
 * @file uart.hpp
 * @brief UART Hardware Abstraction Layer Interface for HM11 driver
 *
 * This file defines the UART Hardware Abstraction Layer (HAL) interface used by the
 * HM11 driver. It provides a hardware-independent abstraction for UART communication,
 * supporting both dynamic and static polymorphism approaches.
 *
 * The interface includes:
 * - Status enumerations for UART operations
 * - Data size configuration options
 * - Template-based data types for UART communication
 * - Abstract interface class for UART port operations
 *
 * This implementation follows the Design Decision 1 (UART Interface Translation) and
 * Design Decision 3 (Error Handling Strategy) from the modernization specifications.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_HAL_UART_HPP
#define HM11_HAL_UART_HPP

#include <chrono>
#include <concepts>
#include <memory>
#include <span>
#include <vector>

#include "types.hpp"

namespace hm11::hal {

/**
 * @brief Status codes for UART operations
 *
 * These status codes indicate the result of UART transmit and receive operations.
 * This directly maps to the Ada HAL.UART.UART_Status enumeration.
 */
enum class UartStatus : std::uint8_t {
    Ok = 0,           ///< Operation completed successfully
    ErrError = 1,     ///< Operation failed due to a hardware error
    ErrTimeout = 2,   ///< Operation timed out
    Busy = 3,         ///< UART port is busy with another operation
    Disconnected = 4, ///< Device is disconnected

    // Aliases for backwards compatibility
    Error = ErrError,    ///< Alias for ErrError
    Timeout = ErrTimeout ///< Alias for ErrTimeout
};

/**
 * @brief Data size options for UART communication
 *
 * Specifies the bit width of data for UART communication.
 * This directly maps to the Ada HAL.UART.UART_Data_Size enumeration.
 */
enum class UartDataSize : std::uint8_t {
    DataSize8b, ///< 8-bit data size
    DataSize9b  ///< 9-bit data size
};

/**
 * @brief Data type for 8-bit UART data
 *
 * This class provides a view over a range of 8-bit UART data.
 * It can be constructed from any contiguous range of UInt8 values.
 */
class UartData8b {
  public:
    /**
     * @brief Construct a new UartData8b object from a span
     *
     * @param span Span of UInt8 values
     */
    explicit UartData8b(UInt8Span span) noexcept : data_span(span) {}

    /**
     * @brief Construct a new UartData8b object from a pointer and length
     *
     * @param data Pointer to UInt8 data
     * @param length Length of the data
     */
    explicit UartData8b(UInt8* data, std::size_t length) noexcept : data_span(data, length) {}

    /**
     * @brief Construct a new UartData8b object from a container
     *
     * @tparam Container Container type supporting data() and size()
     * @param container Container of UInt8 values
     */
    template <typename Container>
        requires std::ranges::contiguous_range<Container> && std::same_as<std::ranges::range_value_t<Container>, UInt8>
    explicit UartData8b(Container& container) noexcept : data_span(std::span(container)) {}

    /**
     * @brief Get the size of the data
     *
     * @return std::size_t Number of bytes in the data
     */
    std::size_t size() const noexcept { return data_span.size(); }

    /**
     * @brief Get a pointer to the start of the data
     *
     * @return UInt8* Pointer to the data
     */
    UInt8* data() noexcept { return data_span.data(); }

    /**
     * @brief Get a const pointer to the start of the data
     *
     * @return const UInt8* Const pointer to the data
     */
    const UInt8* data() const noexcept { return data_span.data(); }

    /**
     * @brief Access an element of the data
     *
     * @param index Index of the element to access
     * @return UInt8& Reference to the element
     */
    UInt8& operator[](std::size_t index) noexcept { return data_span[index]; }

    /**
     * @brief Access an element of the data (const version)
     *
     * @param index Index of the element to access
     * @return const UInt8& Const reference to the element
     */
    const UInt8& operator[](std::size_t index) const noexcept { return data_span[index]; }

    /**
     * @brief Beginning iterator
     *
     * @return auto Iterator to the beginning of the data
     */
    auto begin() noexcept { return data_span.begin(); }

    /**
     * @brief Ending iterator
     *
     * @return auto Iterator to the end of the data
     */
    auto end() noexcept { return data_span.end(); }

    /**
     * @brief Beginning const iterator
     *
     * @return auto Const iterator to the beginning of the data
     */
    auto begin() const noexcept { return data_span.begin(); }

    /**
     * @brief Ending const iterator
     *
     * @return auto Const iterator to the end of the data
     */
    auto end() const noexcept { return data_span.end(); }

  private:
    UInt8Span data_span; ///< Span of UInt8 values
};

/**
 * @brief Data type for 9-bit UART data
 *
 * This class provides a view over a range of 9-bit UART data.
 * Since standard C++ doesn't have a 9-bit type, we use UInt16 (16-bit) with the
 * assumption that only the lower 9 bits are used.
 */
class UartData9b {
  public:
    /**
     * @brief Construct a new UART_Data_9b object from a span
     *
     * @param span Span of UInt16 values (only lower 9 bits used)
     */
    explicit UartData9b(UInt16Span span) noexcept : data_span(span) {}

    /**
     * @brief Construct a new UartData9b object from a pointer and length
     *
     * @param data Pointer to UInt16 data
     * @param length Length of the data
     */
    explicit UartData9b(UInt16* data, std::size_t length) noexcept : data_span(data, length) {}

    /**
     * @brief Construct a new UART_Data_9b object from a container
     *
     * @tparam Container Container type supporting data() and size()
     * @param container Container of UInt16 values
     */
    template <typename Container>
        requires std::ranges::contiguous_range<Container> && std::same_as<std::ranges::range_value_t<Container>, UInt16>
    explicit UartData9b(Container& container) noexcept : data_span(std::span(container)) {}

    /**
     * @brief Get the size of the data
     *
     * @return std::size_t Number of 9-bit values in the data
     */
    std::size_t size() const noexcept { return data_span.size(); }

    /**
     * @brief Get a pointer to the start of the data
     *
     * @return UInt16* Pointer to the data
     */
    UInt16* data() noexcept { return data_span.data(); }

    /**
     * @brief Get a const pointer to the start of the data
     *
     * @return const UInt16* Const pointer to the data
     */
    const UInt16* data() const noexcept { return data_span.data(); }

    /**
     * @brief Access an element of the data
     *
     * @param index Index of the element to access
     * @return UInt16& Reference to the element
     */
    UInt16& operator[](std::size_t index) noexcept { return data_span[index]; }

    /**
     * @brief Access an element of the data (const version)
     *
     * @param index Index of the element to access
     * @return const UInt16& Const reference to the element
     */
    const UInt16& operator[](std::size_t index) const noexcept { return data_span[index]; }

    /**
     * @brief Beginning iterator
     *
     * @return auto Iterator to the beginning of the data
     */
    auto begin() noexcept { return data_span.begin(); }

    /**
     * @brief Ending iterator
     *
     * @return auto Iterator to the end of the data
     */
    auto end() noexcept { return data_span.end(); }

    /**
     * @brief Beginning const iterator
     *
     * @return auto Const iterator to the beginning of the data
     */
    auto begin() const noexcept { return data_span.begin(); }

    /**
     * @brief Ending const iterator
     *
     * @return auto Const iterator to the end of the data
     */
    auto end() const noexcept { return data_span.end(); }

  private:
    UInt16Span data_span; ///< Span of UInt16 values
};

/**
 * @brief Abstract interface for UART port operations
 *
 * This class defines the abstract interface for UART port operations,
 * including transmit and receive with both 8-bit and 9-bit data sizes.
 * It provides an interface for dynamic polymorphism.
 */
class UARTPort {
  public:
    /**
     * @brief Virtual destructor for interface class
     */
    virtual ~UARTPort() = default;

    /**
     * @brief Get the data size of the UART port
     *
     * @return UART_Data_Size The data size (8-bit or 9-bit)
     */
    virtual UartDataSize data_size() const = 0;

    /**
     * @brief Transmit 8-bit data over the UART port
     *
     * @param data The data to transmit
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds (default: 1000ms)
     */
    virtual void transmit(
        const UartData8b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) = 0;

    /**
     * @brief Transmit 9-bit data over the UART port
     *
     * @param data The data to transmit
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds (default: 1000ms)
     */
    virtual void transmit(
        const UartData9b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) = 0;

    /**
     * @brief Receive 8-bit data from the UART port
     *
     * @param data Output parameter for received data
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds (default: 1000ms)
     */
    virtual void receive(
        UartData8b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) = 0;

    /**
     * @brief Receive 9-bit data from the UART port
     *
     * @param data Output parameter for received data
     * @param status Output parameter for operation status
     * @param timeout_ms Timeout in milliseconds (default: 1000ms)
     */
    virtual void receive(
        UartData9b& data,
        UartStatus& status,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(1000)) = 0;
};

/**
 * @brief Type alias for a shared pointer to a UART port
 *
 * This type corresponds to the Ada HAL.UART.Any_UART_Port type,
 * which is an access to a class-wide UART_Port.
 */
using AnyUartPort = std::shared_ptr<UARTPort>;

/**
 * @brief Helper for specialized checks for UART_Implementation with std::shared_ptr
 */
template <typename T> struct IsSharedPtrToUart : std::false_type {};

template <typename T> struct IsSharedPtrToUart<std::shared_ptr<T>> : std::is_base_of<UARTPort, T> {};

/**
 * @brief Concept defining the requirements for a UART implementation
 *
 * This concept defines the requirements that a class must satisfy to be
 * usable as a UART implementation with the template-based interface.
 * It enforces the same interface as the abstract UARTPort class.
 */
template <typename T>
concept UART_Implementation =
    // Direct class implementation
    requires(
        T t,
        const UartData8b& data_8b,
        const UartData9b& data_9b,
        UartData8b& out_data_8b,
        UartData9b& out_data_9b,
        UartStatus& status,
        std::chrono::milliseconds timeout) {
        { t.data_size() } -> std::convertible_to<UartDataSize>;
        { t.transmit(data_8b, status, timeout) } -> std::same_as<void>;
        { t.transmit(data_9b, status, timeout) } -> std::same_as<void>;
        { t.receive(out_data_8b, status, timeout) } -> std::same_as<void>;
        { t.receive(out_data_9b, status, timeout) } -> std::same_as<void>;
    } ||
    // Pointer implementation
    requires(
        T t,
        const UartData8b& data_8b,
        const UartData9b& data_9b,
        UartData8b& out_data_8b,
        UartData9b& out_data_9b,
        UartStatus& status,
        std::chrono::milliseconds timeout) {
        { t->data_size() } -> std::convertible_to<UartDataSize>;
        { t->transmit(data_8b, status, timeout) } -> std::same_as<void>;
        { t->transmit(data_9b, status, timeout) } -> std::same_as<void>;
        { t->receive(out_data_8b, status, timeout) } -> std::same_as<void>;
        { t->receive(out_data_9b, status, timeout) } -> std::same_as<void>;
    } ||
    // std::shared_ptr to any UARTPort-derived class
    IsSharedPtrToUart<T>::value;

} // namespace hm11::hal

#endif // HM11_HAL_UART_HPP