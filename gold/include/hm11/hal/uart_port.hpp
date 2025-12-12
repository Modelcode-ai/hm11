/**
 * @file uart_port.hpp
 * @brief UART port interface abstraction using C++17 SFINAE type traits
 *
 * This header defines type traits (IsUartPort, IsReceiveHandler) that specify
 * the interface contract for UART implementations. It enables compile-time polymorphism
 * and zero-overhead abstraction by using C++17 SFINAE type traits instead of virtual
 * functions, matching Ada's discriminant-based compile-time binding pattern.
 *
 * ## Architecture Decision Resolution
 *
 * This implementation resolves two key architecture questions from the
 * migration specification:
 *
 * 1. **UART Interface Abstraction (Decision 3)**: Uses C++17 SFINAE type traits as the
 *    primary interface mechanism with optional abstract base class for testing.
 *    Type traits provide compile-time validation with zero runtime overhead, while
 *    the abstract base class enables mock testing with runtime polymorphism.
 *
 * 2. **Receive Strategy Injection (Decision 3)**: Implements template parameter
 *    approach where receive handler is a callable object validated by IsReceiveHandler
 *    type trait, injected as a template parameter to HM11Driver. This enables different
 *    receive implementations (blocking HAL-based, DMA-based, interrupt-driven) without
 *    runtime overhead.
 *
 * ## Design Rationale
 *
 * ### Why SFINAE Type Traits Over Abstract Classes?
 *
 * C++17 SFINAE type traits provide several advantages for embedded systems:
 * - **Zero runtime overhead**: No vtable, no virtual dispatch
 * - **Compile-time validation**: Interface violations caught at instantiation
 * - **Optimizable**: Compiler can inline and optimize across interface boundary
 * - **Small binary size**: No RTTI, no virtual table overhead
 * - **Wide toolchain support**: Works with C++17 (GCC 7+, Clang 5+, all embedded compilers)
 * - **Perfect Ada match**: Template parameters = Ada discriminants (compile-time binding)
 *
 * The optional abstract base class (IUARTPort) is provided for testing purposes
 * where runtime polymorphism is acceptable (mock objects, test fixtures).
 *
 * ### Receive Strategy Pattern
 *
 * The receive strategy uses a callable object validated by IsReceiveHandler that can be:
 * - Callable structs with operator() (zero overhead, can hold state)
 * - Function pointers (zero overhead, stateless)
 * - Lambdas (with or without captures)
 *
 * This provides flexibility for different receive implementations while
 * maintaining type safety and enabling the compiler to optimize the calls.
 *
 * ## Usage Patterns
 *
 * ### Pattern 1: Type Trait Validation (Production Code)
 *
 * ```cpp
 * class STM32F4UARTPort {
 * public:
 *     [[nodiscard]] UartDataSize data_size() const noexcept;
 *
 *     void transmit(UartData8bConst data, UartStatus& status,
 *                   uint32_t timeout_ms = 1000) noexcept;
 *
 *     void receive(UartData8b data, UartStatus& status,
 *                  uint32_t timeout_ms = 1000, bool as_stream = false) noexcept;
 * };
 *
 * // Validate interface satisfaction at compile time
 * static_assert(IS_UART_PORT_V<STM32F4UARTPort>,
 *               "STM32F4UARTPort must satisfy UART port interface");
 *
 * // Instantiate driver with specific UART implementation
 * DefaultReceiveHandler my_handler;
 * HM11Driver<STM32F4UARTPort, DefaultReceiveHandler> driver(uart_port, my_handler);
 * ```
 *
 * ### Pattern 2: Abstract Base Class (Testing)
 *
 * ```cpp
 * class MockUARTPort : public IUARTPort {
 * public:
 *     UartDataSize data_size() const noexcept override { return UartDataSize::DataSize8b; }
 *
 *     void transmit(UartData8bConst data, UartStatus& status,
 *                   uint32_t timeout_ms) noexcept override {
 *         // Mock implementation
 *     }
 *
 *     void receive(UartData8b data, UartStatus& status,
 *                  uint32_t timeout_ms, bool as_stream) noexcept override {
 *         // Mock implementation
 *     }
 * };
 *
 * MockUARTPort mock;
 * DefaultReceiveHandler mock_handler;
 * HM11Driver<MockUARTPort, DefaultReceiveHandler> driver(mock, mock_handler);
 * ```
 *
 * ## Blocking vs Streaming Modes
 *
 * The `as_stream` parameter controls the receive behavior:
 *
 * - **Blocking Mode** (`as_stream = false`):
 *   - Blocks until buffer is filled or timeout occurs
 *   - Returns when requested amount of data is received
 *   - Used for command-response patterns (AT commands)
 *   - Example: Send "AT+ADDR?" and wait for complete response
 *
 * - **Streaming Mode** (`as_stream = true`):
 *   - Starts continuous reception without blocking caller
 *   - Data arrives in circular buffer accessible via polling
 *   - Used for discovery scanning and async notifications
 *   - Caller polls read position to process incoming data
 *   - Example: Scan operation collecting multiple device advertisements
 *
 * ## Timeout Semantics
 *
 * Timeout values are specified in milliseconds as uint32_t:
 * - Default: 1000ms (1 second) for most operations
 * - Long operations (Scan, GATT queries): Caller-specified, typically 5000-10000ms
 * - Timeout implementation uses ISR-side deadline polling (Decision 25)
 * - Timeout indicates "no new data arriving" rather than absolute deadline
 * - For streaming operations, timeout may be soft (Decision 28)
 *
 * ## ISR Safety
 *
 * UART operations interface with ISR context for DMA and interrupt-driven modes:
 * - Transmit/Receive methods are callable from application context
 * - Implementation may use ISR handlers internally (e.g., DMA completion)
 * - Synchronization uses ISR-safe primitives (BinarySemaphore, std::atomic)
 * - Memory ordering follows Decision 23 (acquire/release for ISR-to-thread)
 *
 * ## Namespace Organization
 *
 * This header resides in hm11::hal namespace, matching the directory structure
 * (include/hm11/hal/) per Decision 24 (Namespace Organization).
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include "hm11/hal/base_types.hpp"

namespace hm11::hal {

// ============================================================================
// Receive Handler Type Trait (Receive Strategy Abstraction)
// ============================================================================

/**
 * @brief Type trait defining the interface for receive handler strategies
 *
 * The receive handler implements the strategy pattern for UART reception,
 * allowing different implementations (blocking HAL-based, DMA-based,
 * interrupt-driven) to be injected into the HM11Driver via template parameter.
 *
 * This corresponds to Ada's Receive_Handler access type (procedure pointer)
 * from hm11.ads lines 41-52, with support for default timeout parameter
 * (Timeout : Natural := 1000).
 *
 * ## Method Signatures (Both Required)
 *
 * The receive handler must be callable with TWO signatures to match Ada's
 * default parameter behavior:
 *
 * ### Signature 1: With explicit timeout (for long operations)
 * ```cpp
 * void operator()(
 *     UARTPortType& port,      // UART port to receive from
 *     UartData8b buffer,       // Buffer to store received data
 *     UartStatus& status,      // Out parameter for operation status
 *     uint32_t timeout_ms,     // Explicit timeout in milliseconds
 *     bool as_stream           // Streaming vs blocking mode
 * ) noexcept;
 * ```
 *
 * ### Signature 2: Without timeout (uses handler's default)
 * ```cpp
 * void operator()(
 *     UARTPortType& port,      // UART port to receive from
 *     UartData8b buffer,       // Buffer to store received data
 *     UartStatus& status,      // Out parameter for operation status
 *     bool as_stream           // Streaming vs blocking mode
 * ) noexcept;
 * ```
 *
 * ## Parameters
 *
 * - `port`: Reference to UART port implementation satisfying IS_UART_PORT_V
 * - `buffer`: Mutable span where received data will be stored
 * - `status`: Output parameter for operation result (Ok, ErrTimeout, etc.)
 * - `timeout_ms`: Maximum time to wait for data, in milliseconds (optional)
 * - `as_stream`: If true, start continuous streaming without blocking;
 *                if false, block until buffer filled or timeout
 *
 * ## Implementation Examples
 *
 * ### Example 1: Handler with Default Timeout (Recommended)
 * ```cpp
 * struct DefaultReceiveHandler {
 *     static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;
 *
 *     // With explicit timeout
 *     template<typename UARTPortType>
 *     void operator()(UARTPortType& port, UartData8b buffer,
 *                     UartStatus& status, uint32_t timeout_ms,
 *                     bool as_stream) noexcept {
 *         if (as_stream) {
 *             status = UartStatus::ErrError;
 *             return;
 *         }
 *         port.receive(buffer, status, timeout_ms, false);
 *     }
 *
 *     // Without timeout - uses default
 *     template<typename UARTPortType>
 *     void operator()(UARTPortType& port, UartData8b buffer,
 *                     UartStatus& status, bool as_stream) noexcept {
 *         (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
 *     }
 * };
 * ```
 *
 * ### Example 2: Handler with C++ Default Parameters
 * ```cpp
 * struct SimpleReceiveHandler {
 *     template<typename UARTPortType>
 *     void operator()(UARTPortType& port, UartData8b buffer,
 *                     UartStatus& status, uint32_t timeout_ms = 1000,
 *                     bool as_stream = false) noexcept {
 *         port.receive(buffer, status, timeout_ms, as_stream);
 *     }
 * };
 * ```
 *
 * @tparam Handler Type to check for receive handler interface compliance
 * @tparam UARTPortType UART port type the handler will work with
 *
 * @note The noexcept requirement ensures ISR-safe implementations
 * @note Handler should not throw exceptions (embedded environment)
 * @note Both call signatures (with/without timeout) must be supported
 *
 * @see default_receive_handler for HAL-based reference implementation
 */
template <typename Handler, typename UARTPortType, typename = void> struct IsReceiveHandler : std::false_type {};

template <typename Handler, typename UARTPortType>
struct IsReceiveHandler<
    Handler,
    UARTPortType,
    std::void_t<
        // Check callable WITH explicit timeout
        decltype(std::declval<Handler&>()(
            std::declval<UARTPortType&>(),
            std::declval<UartData8b>(),
            std::declval<UartStatus&>(),
            std::declval<uint32_t>(),
            std::declval<bool>())),
        // Check callable WITHOUT timeout (uses default)
        decltype(std::declval<Handler&>()(
            std::declval<UARTPortType&>(),
            std::declval<UartData8b>(),
            std::declval<UartStatus&>(),
            std::declval<bool>()))>> : std::true_type {};

/**
 * @brief Helper variable template for IsReceiveHandler type trait
 *
 * Use this for cleaner syntax in static_assert and SFINAE constraints.
 *
 * Example:
 * ```cpp
 * static_assert(IS_RECEIVE_HANDLER_V<MyHandler, MyUARTPort>,
 *               "MyHandler must satisfy receive handler interface");
 * ```
 */
template <typename Handler, typename UARTPortType>
inline constexpr bool IS_RECEIVE_HANDLER_V = IsReceiveHandler<Handler, UARTPortType>::value;

// ============================================================================
// UART Port Type Trait (Interface Abstraction)
// ============================================================================

/**
 * @brief Type trait defining the UART port interface contract
 *
 * This type trait specifies the required interface that platform-specific UART
 * implementations must satisfy to be used with the HM11Driver. It provides
 * compile-time validation of interface compliance with zero runtime overhead.
 *
 * Corresponds to Ada's HAL.UART.UART_Port limited interface (hal-uart.ads).
 *
 * ## Required Methods
 *
 * ### 1. data_size() - Query data bit width
 * ```cpp
 * [[nodiscard]] UartDataSize data_size() const noexcept;
 * ```
 * Returns the configured data size (8-bit or 9-bit). For HM-11 driver,
 * this must always return `UartDataSize::DataSize8b` as HM-11 hardware
 * only supports 8-bit mode.
 *
 * ### 2. transmit() - Send data over UART
 * ```cpp
 * void transmit(UartData8bConst data, UartStatus& status,
 *               uint32_t timeout_ms = 1000) noexcept;
 * ```
 * Transmits data buffer over UART. Blocks until all data sent or timeout.
 * - `data`: Read-only buffer to transmit
 * - `status`: Output parameter for operation result
 * - `timeout_ms`: Maximum time to wait for transmission completion
 *
 * ### 3. receive() - Receive data from UART
 * ```cpp
 * void receive(UartData8b data, UartStatus& status,
 *              uint32_t timeout_ms = 1000,
 *              bool as_stream = false) noexcept;
 * ```
 * Receives data from UART. Behavior depends on `as_stream` parameter:
 * - Blocking mode: Wait until buffer filled or timeout
 * - Streaming mode: Start continuous reception, return immediately
 * - `data`: Mutable buffer to store received data
 * - `status`: Output parameter for operation result
 * - `timeout_ms`: Maximum time to wait in blocking mode
 * - `as_stream`: Enable streaming (non-blocking) mode
 *
 * ## Interface Contract (Preconditions)
 *
 * Ada's HAL.UART interface includes Pre'Class contracts that enforce:
 * - `transmit()` requires `data_size() == DataSize8b` for 8-bit buffers
 * - `receive()` requires `data_size() == DataSize8b` for 8-bit buffers
 *
 * In C++, these are documented preconditions that implementations must satisfy.
 * Debug builds may include runtime assertions to verify preconditions.
 *
 * ## Implementation Notes
 *
 * ### Memory Safety
 * - Buffer parameters use std::span for automatic bounds checking
 * - Implementations must not access memory beyond span boundaries
 * - For DMA operations, ensure proper cache coherency (Decision 20)
 *
 * ### ISR Safety
 * - Methods are callable from application context, not ISR context
 * - Implementations may use ISR handlers internally (DMA completion, etc.)
 * - Internal synchronization must use ISR-safe primitives
 *
 * ### Exception Safety
 * - All methods are noexcept (no exceptions in embedded environment)
 * - Errors reported via status out parameter
 *
 * ### Thread Safety
 * - Interface does not guarantee thread safety
 * - Caller must serialize access if used from multiple threads
 * - Typical usage: single-threaded or protected by higher-level mutex
 *
 * ## Validation
 *
 * Validate your UART implementation satisfies the interface:
 * ```cpp
 * class MyUARTPort {
 *     // ... implementation ...
 * };
 *
 * static_assert(IS_UART_PORT_V<MyUARTPort>, "MyUARTPort must satisfy UART port interface");
 * ```
 *
 * @tparam T Type to validate for UART port interface compliance
 *
 * @see IUARTPort for abstract base class alternative (testing)
 * @see IsReceiveHandler for receive strategy type trait
 *
 * @note 9-bit variants (transmit_9b, receive_9b) are omitted since HM-11
 *       hardware only supports 8-bit mode. Add if needed for other hardware.
 */
template <typename T, typename = void> struct IsUartPort : std::false_type {};

template <typename T>
struct IsUartPort<
    T,
    std::void_t<
        // Check data_size() method
        decltype(std::declval<const T&>().data_size()),
        // Check transmit() method
        decltype(std::declval<T&>()
                     .transmit(std::declval<UartData8bConst>(), std::declval<UartStatus&>(), std::declval<uint32_t>())),
        // Check receive() method
        decltype(std::declval<T&>().receive(
            std::declval<UartData8b>(),
            std::declval<UartStatus&>(),
            std::declval<uint32_t>(),
            std::declval<bool>()))>> : std::true_type {};

/**
 * @brief Helper variable template for IsUartPort type trait
 *
 * Use this for cleaner syntax in static_assert and SFINAE constraints.
 *
 * Example:
 * ```cpp
 * static_assert(IS_UART_PORT_V<MyUARTPort>,
 *               "MyUARTPort must satisfy UART port interface");
 * ```
 */
template <typename T> inline constexpr bool IS_UART_PORT_V = IsUartPort<T>::value;

// ============================================================================
// Abstract Base Class (Optional, for Testing)
// ============================================================================

/**
 * @brief Abstract base class for UART port (testing/mocking alternative)
 *
 * This abstract base class provides runtime polymorphism for UART port
 * implementations, enabling mock objects and test fixtures. It defines
 * the same interface as the UARTPort concept but uses virtual functions.
 *
 * ## When to Use
 *
 * Use IUARTPort when:
 * - Writing unit tests with mock UART implementations
 * - Using test frameworks that require runtime polymorphism (GoogleTest, etc.)
 * - Prototyping without specific hardware implementation
 *
 * Use type trait validation (IS_UART_PORT_V) when:
 * - Writing production code (zero overhead, better optimization)
 * - Performance is critical (no virtual dispatch overhead)
 * - Binary size matters (no vtable overhead)
 *
 * ## Trade-offs
 *
 * Abstract base class provides:
 * - ✓ Runtime polymorphism (swap implementations at runtime)
 * - ✓ Familiar OOP pattern (easier for some developers)
 * - ✓ Mock testing support (override virtual methods)
 * - ✗ Runtime overhead (virtual dispatch, ~1-3 cycles per call)
 * - ✗ Binary size increase (vtable, RTTI if enabled)
 * - ✗ Optimization barrier (compiler cannot inline across interface)
 *
 * Type trait-based approach provides:
 * - ✓ Zero runtime overhead (compile-time polymorphism)
 * - ✓ Full inlining and optimization (compiler sees implementation)
 * - ✓ Smaller binary size (no vtable)
 * - ✓ Better cache performance (no vtable indirection)
 * - ✗ Cannot swap implementations at runtime (template parameter)
 * - ✗ Longer compile times (template instantiation)
 *
 * ## Usage Example
 *
 * ```cpp
 * // Mock UART for testing
 * class MockUARTPort : public IUARTPort {
 * public:
 *     UartDataSize data_size() const noexcept override {
 *         return UartDataSize::DataSize8b;
 *     }
 *
 *     void transmit(UartData8bConst data, UartStatus& status,
 *                   uint32_t timeout_ms) noexcept override {
 *         transmitted_data.insert(transmitted_data.end(),
 *                                 data.begin(), data.end());
 *         status = UartStatus::Ok;
 *     }
 *
 *     void receive(UartData8b data, UartStatus& status,
 *                  uint32_t timeout_ms, bool as_stream) noexcept override {
 *         // Copy from mock receive buffer
 *         size_t to_copy = std::min(data.size(), receive_buffer.size());
 *         std::copy_n(receive_buffer.begin(), to_copy, data.begin());
 *         status = (to_copy > 0) ? UartStatus::Ok : UartStatus::ErrTimeout;
 *     }
 *
 *     // Test helper methods
 *     std::vector<UInt8> transmitted_data;
 *     std::vector<UInt8> receive_buffer;
 * };
 *
 * // Use in tests
 * TEST(HM11Driver, TransmitCommand) {
 *     MockUARTPort mock_uart;
 *     mock_uart.receive_buffer = {'O', 'K'};
 *
 *     // HM11Driver<IUARTPort> driver(mock_uart, ...);
 *     // driver.test(status);
 *
 *     EXPECT_EQ(mock_uart.transmitted_data.size(), 2);  // "AT"
 * }
 * ```
 *
 * @note Virtual destructor included to enable polymorphic deletion
 * @note All methods match UARTPort concept requirements
 * @note Implementations automatically satisfy UARTPort concept
 */
class IUARTPort {
  public:
    /**
     * @brief Virtual destructor for polymorphic deletion
     *
     * Ensures derived classes are properly destroyed when deleted through
     * base class pointer.
     */
    virtual ~IUARTPort() noexcept = default;

    /**
     * @brief Query the configured data size (8-bit or 9-bit mode)
     *
     * @return UartDataSize Current data size configuration
     *
     * @note For HM-11 driver, must return DataSize8b
     */
    [[nodiscard]] virtual UartDataSize data_size() const noexcept = 0;

    /**
     * @brief Transmit data buffer over UART
     *
     * Blocks until all data transmitted or timeout occurs.
     *
     * @param data Read-only buffer containing data to transmit
     * @param status Output parameter for operation result
     * @param timeout_ms Maximum time to wait for transmission (milliseconds)
     *
     * @pre data_size() == UartDataSize::DataSize8b
     * @post status contains operation result (Ok, ErrTimeout, etc.)
     */
    virtual void transmit(UartData8bConst data, UartStatus& status, uint32_t timeout_ms = 1000) noexcept = 0;

    /**
     * @brief Receive data from UART
     *
     * Behavior depends on as_stream parameter:
     * - Blocking mode (as_stream=false): Wait until buffer filled or timeout
     * - Streaming mode (as_stream=true): Start continuous reception, return immediately
     *
     * @param data Mutable buffer to store received data
     * @param status Output parameter for operation result
     * @param timeout_ms Maximum time to wait in blocking mode (milliseconds)
     * @param as_stream Enable streaming (non-blocking) mode
     *
     * @pre data_size() == UartDataSize::DataSize8b
     * @post status contains operation result (Ok, ErrTimeout, etc.)
     * @post In blocking mode: data buffer filled up to timeout or error
     * @post In streaming mode: continuous reception started, data arrives asynchronously
     */
    virtual void
    receive(UartData8b data, UartStatus& status, uint32_t timeout_ms = 1000, bool as_stream = false) noexcept = 0;

    // Delete copy and move operations (follows Decision 22: RAII Ownership Model)
    IUARTPort(const IUARTPort&) = delete;
    IUARTPort& operator=(const IUARTPort&) = delete;
    IUARTPort(IUARTPort&&) = delete;
    IUARTPort& operator=(IUARTPort&&) = delete;

  protected:
    /**
     * @brief Protected default constructor
     *
     * Prevents direct instantiation of abstract base class.
     * Derived classes can use this constructor.
     */
    IUARTPort() noexcept = default;
};

// Verify that IUARTPort satisfies the UART port interface
static_assert(IS_UART_PORT_V<IUARTPort>, "IUARTPort abstract base class must satisfy UART port interface");

// ============================================================================
// Default Receive Handler (HAL-based Reference Implementation)
// ============================================================================

/**
 * @brief Default receive handler callable object using HAL blocking receive
 *
 * This struct provides a simple reference implementation of the receive handler
 * interface that delegates directly to the UART port's receive method. It supports
 * only blocking mode and returns an error for streaming mode.
 *
 * Corresponds to Ada's Default_Receive_Handler procedure (hm11.ads lines 60-70)
 * with default timeout parameter (Timeout : Natural := 1000).
 *
 * ## Behavior
 *
 * - **Blocking mode** (as_stream=false): Calls port.receive() and blocks until
 *   buffer filled or timeout
 * - **Streaming mode** (as_stream=true): Returns ErrError status (not supported)
 * - **Default timeout**: 1000ms (1 second) when timeout parameter omitted
 *
 * ## Usage
 *
 * ```cpp
 * STM32F4UARTPort uart_port;
 * DefaultReceiveHandler handler;
 * HM11Driver<STM32F4UARTPort, DefaultReceiveHandler> driver(uart_port, handler);
 *
 * // Regular commands use default timeout (1000ms)
 * handler(uart_port, buffer, status, false);
 *
 * // Long operations use explicit timeout
 * handler(uart_port, buffer, status, 5000, true);
 * ```
 *
 * ## Limitations
 *
 * This implementation does not support:
 * - Streaming mode reception (returns error)
 * - DMA-based transfers
 * - Interrupt-driven asynchronous reception
 *
 * These features require platform-specific implementations with hardware support.
 *
 * @see IS_RECEIVE_HANDLER_V for interface requirements
 */
struct DefaultReceiveHandler {
    /// Default timeout matching Ada's (Timeout : Natural := 1000)
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    /**
     * @brief Receive with explicit timeout (for long operations)
     *
     * @tparam UARTPortType UART port type satisfying IS_UART_PORT_V
     * @param port UART port to receive from
     * @param buffer Buffer to store received data
     * @param status Output parameter for operation result
     * @param timeout_ms Timeout in milliseconds
     * @param as_stream If true, returns error (streaming not supported)
     */
    template <typename UARTPortType>
    constexpr void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        if (as_stream) {
            status = UartStatus::ErrError;
            return;
        }
        port.receive(buffer, status, timeout_ms, false);
    }

    /**
     * @brief Receive with default timeout (for regular commands)
     *
     * Matches Ada's ability to omit timeout parameter, uses DEFAULT_TIMEOUT_MS.
     *
     * @tparam UARTPortType UART port type satisfying IS_UART_PORT_V
     * @param port UART port to receive from
     * @param buffer Buffer to store received data
     * @param status Output parameter for operation result
     * @param as_stream If true, returns error (streaming not supported)
     */
    template <typename UARTPortType>
    constexpr void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// Verification deferred to instantiation time since it's a template

// ============================================================================
// Example Usage
// ============================================================================

/**
 * @example uart_port_concept_example.cpp
 * Example of implementing and using UARTPort concept
 *
 * @code
 * #include "hm11/hal/uart_port.hpp"
 * #include "hm11/hal/base_types.hpp"
 * #include <array>
 * #include <cstring>
 *
 * using namespace hm11::hal;
 *
 * // Example 1: Simple UART implementation satisfying UARTPort concept
 * class SimpleUARTPort {
 * public:
 *     [[nodiscard]] UartDataSize data_size() const noexcept {
 *         return UartDataSize::DataSize8b;
 *     }
 *
 *     void transmit(UartData8bConst data, UartStatus& status,
 *                   uint32_t timeout_ms = 1000) noexcept {
 *         // Simulate transmission by copying to internal buffer
 *         tx_buffer_.assign(data.begin(), data.end());
 *         status = UartStatus::Ok;
 *     }
 *
 *     void receive(UartData8b data, UartStatus& status,
 *                  uint32_t timeout_ms = 1000,
 *                  bool as_stream = false) noexcept {
 *         if (as_stream) {
 *             status = UartStatus::ErrError;  // Not supported
 *             return;
 *         }
 *
 *         // Simulate reception by copying from internal buffer
 *         size_t to_copy = std::min(data.size(), rx_buffer_.size());
 *         std::copy_n(rx_buffer_.begin(), to_copy, data.begin());
 *         status = (to_copy > 0) ? UartStatus::Ok : UartStatus::ErrTimeout;
 *     }
 *
 *     // Test helpers
 *     std::vector<UInt8> tx_buffer_;
 *     std::vector<UInt8> rx_buffer_;
 * };
 *
 * // Verify type trait satisfaction
 * static_assert(IS_UART_PORT_V<SimpleUARTPort>);
 *
 * // Example 2: Custom receive handler with state
 * struct CustomReceiveHandler {
 *     int receive_count = 0;
 *
 *     template<typename UARTPortType>
 *     void operator()(UARTPortType& port, UartData8b buffer,
 *                     UartStatus& status, uint32_t timeout_ms,
 *                     bool as_stream) noexcept {
 *         ++receive_count;
 *         port.receive(buffer, status, timeout_ms, as_stream);
 *     }
 *
 *     // Overload without timeout (uses default)
 *     template<typename UARTPortType>
 *     void operator()(UARTPortType& port, UartData8b buffer,
 *                     UartStatus& status, bool as_stream) noexcept {
 *         (*this)(port, buffer, status, 1000, as_stream);
 *     }
 * };
 *
 * // Verify type trait satisfaction
 * static_assert(IS_RECEIVE_HANDLER_V<CustomReceiveHandler, SimpleUARTPort>);
 *
 * // Example 3: Using with HM11Driver (conceptual - driver not yet implemented)
 * void usage_example() {
 *     SimpleUARTPort uart_port;
 *     CustomReceiveHandler receive_handler;
 *
 *     // Prepare mock data
 *     uart_port.rx_buffer_ = {'O', 'K'};
 *
 *     // Test transmit
 *     UartStatus status;
 *     std::array<UInt8, 2> tx_data = {'A', 'T'};
 *     uart_port.transmit(tx_data, status, 1000);
 *
 *     // Test receive
 *     std::array<UInt8, 2> rx_data;
 *     uart_port.receive(rx_data, status, 1000, false);
 *
 *     // Future usage with HM11Driver:
 *     // HM11Driver<SimpleUARTPort, CustomReceiveHandler> driver(uart_port, receive_handler);
 * }
 *
 * // Example 4: Mock UART using abstract base class
 * class MockUARTPort : public IUARTPort {
 * public:
 *     UartDataSize data_size() const noexcept override {
 *         return UartDataSize::DataSize8b;
 *     }
 *
 *     void transmit(UartData8bConst data, UartStatus& status,
 *                   uint32_t timeout_ms) noexcept override {
 *         // Mock implementation
 *         status = UartStatus::Ok;
 *     }
 *
 *     void receive(UartData8b data, UartStatus& status,
 *                  uint32_t timeout_ms, bool as_stream) noexcept override {
 *         // Mock implementation
 *         status = UartStatus::Ok;
 *     }
 * };
 *
 * // Verify mock satisfies type trait
 * static_assert(IS_UART_PORT_V<MockUARTPort>);
 *
 * @endcode
 */

} // namespace hm11::hal
