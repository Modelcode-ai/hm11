/**
 * @file test_uart_port.cpp
 * @brief Unit tests for UARTPort concept and receive handler abstraction
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include "hm11/hal/uart_port.hpp"

using namespace hm11::hal;

// ============================================================================
// Test Fixtures: Simple UART Implementation
// ============================================================================

/**
 * @brief Simple UART port implementation for testing concept satisfaction
 */
class SimpleUARTPort {
  public:
    SimpleUARTPort() = default;
    ~SimpleUARTPort() noexcept = default;

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        // Copy transmitted data for verification
        tx_buffer.assign(data.begin(), data.end());
        ++tx_count;
        status = UartStatus::Ok;
    }

    void receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool as_stream = false) noexcept {
        if (as_stream) {
            // Streaming mode not supported in simple implementation
            status = UartStatus::ErrError;
            return;
        }

        // Copy from receive buffer (up to available data)
        const auto COUNT = std::min(data.size(), rx_buffer.size());
        if (COUNT > 0) {
            std::copy_n(rx_buffer.begin(), static_cast<std::ptrdiff_t>(COUNT), data.begin());
            status = UartStatus::Ok;
        } else {
            status = UartStatus::ErrTimeout;
        }
        ++rx_count;
    }

    // Test helper members
    std::vector<UInt8> tx_buffer;
    std::vector<UInt8> rx_buffer;
    size_t tx_count = 0;
    size_t rx_count = 0;

    // Delete copy/move per Decision 22 (RAII Ownership Model)
    SimpleUARTPort(const SimpleUARTPort&) = delete;
    SimpleUARTPort& operator=(const SimpleUARTPort&) = delete;
    SimpleUARTPort(SimpleUARTPort&&) = delete;
    SimpleUARTPort& operator=(SimpleUARTPort&&) = delete;
};

// Verify type trait satisfaction
static_assert(IS_UART_PORT_V<SimpleUARTPort>, "SimpleUARTPort should satisfy UART port interface");

// ============================================================================
// Test Fixtures: Mock UART using Abstract Base Class
// ============================================================================

/**
 * @brief Mock UART port using abstract base class for testing
 */
class MockUARTPort : public IUARTPort {
  public:
    MockUARTPort() = default;
    ~MockUARTPort() noexcept override = default;

    [[nodiscard]] UartDataSize data_size() const noexcept override { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept override {
        // Copy transmitted data for verification
        tx_buffer.assign(data.begin(), data.end());
        ++tx_count;
        status = mock_tx_status;
    }

    void receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool as_stream = false) noexcept
        override {
        if (as_stream && !supports_streaming) {
            status = UartStatus::ErrError;
            return;
        }

        // Copy from receive buffer
        const auto COUNT = std::min(data.size(), rx_buffer.size());
        if (COUNT > 0) {
            std::copy_n(rx_buffer.begin(), static_cast<std::ptrdiff_t>(COUNT), data.begin());
            status = mock_rx_status;
        } else {
            status = UartStatus::ErrTimeout;
        }
        ++rx_count;
    }

    // Test configuration
    std::vector<UInt8> tx_buffer;
    std::vector<UInt8> rx_buffer;
    size_t tx_count = 0;
    size_t rx_count = 0;
    UartStatus mock_tx_status = UartStatus::Ok;
    UartStatus mock_rx_status = UartStatus::Ok;
    bool supports_streaming = false;
};

// Verify type trait satisfaction
static_assert(IS_UART_PORT_V<MockUARTPort>, "MockUARTPort should satisfy UART port interface");

// ============================================================================
// Test Fixtures: Custom Receive Handlers
// ============================================================================

/**
 * @brief Simple receive handler struct (callable object pattern)
 */
struct SimpleReceiveHandler {
    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload without timeout (uses default 1000ms)
    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, 1000, as_stream);
    }
};

// Verify type trait satisfaction
static_assert(
    IS_RECEIVE_HANDLER_V<SimpleReceiveHandler, SimpleUARTPort>,
    "SimpleReceiveHandler should satisfy receive handler interface");

/**
 * @brief Stateful receive handler (callable object pattern)
 */
struct CountingReceiveHandler {
    size_t call_count = 0;

    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        ++call_count;
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    // Overload without timeout (uses default 1000ms)
    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, 1000, as_stream);
    }
};

// Verify type trait satisfaction
static_assert(
    IS_RECEIVE_HANDLER_V<CountingReceiveHandler, SimpleUARTPort>,
    "CountingReceiveHandler should satisfy receive handler interface");

// ============================================================================
// UART Port Interface Tests
// ============================================================================

TEST(UARTPortInterface, SimpleUARTPortSatisfiesInterface) {
    // Verify at compile time
    EXPECT_TRUE((IS_UART_PORT_V<SimpleUARTPort>));
}

TEST(UARTPortInterface, MockUARTPortSatisfiesInterface) {
    // Verify at compile time
    EXPECT_TRUE((IS_UART_PORT_V<MockUARTPort>));
}

TEST(UARTPortInterface, IUARTPortSatisfiesInterface) {
    // Verify abstract base class satisfies interface
    EXPECT_TRUE((IS_UART_PORT_V<IUARTPort>));
}

TEST(UARTPortInterface, DataSizeReturns8Bit) {
    SimpleUARTPort uart;
    EXPECT_EQ(uart.data_size(), UartDataSize::DataSize8b);
}

TEST(UARTPortInterface, TransmitCopiesDataToBuffer) {
    SimpleUARTPort uart;
    std::array<UInt8, 5> tx_data = {'H', 'e', 'l', 'l', 'o'};
    UartStatus status = UartStatus::Busy;

    uart.transmit(tx_data, status, 1000);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart.tx_buffer.size(), 5U);
    EXPECT_EQ(uart.tx_buffer[0], 'H');
    EXPECT_EQ(uart.tx_buffer[4], 'o');
    EXPECT_EQ(uart.tx_count, 1U);
}

TEST(UARTPortInterface, ReceiveBlockingModeCopiesToBuffer) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'O', 'K'};

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    uart.receive(rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'O');
    EXPECT_EQ(rx_data[1], 'K');
    EXPECT_EQ(uart.rx_count, 1U);
}

TEST(UARTPortInterface, ReceiveTimeoutWhenNoData) {
    SimpleUARTPort uart;
    // rx_buffer is empty

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    uart.receive(rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

TEST(UARTPortInterface, ReceiveStreamingModeNotSupported) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'O', 'K'};

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    uart.receive(rx_data, status, 1000, true); // as_stream=true

    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Receive Handler Interface Tests
// ============================================================================

TEST(ReceiveHandlerInterface, SimpleHandlerSatisfiesInterface) {
    // Verify at compile time
    EXPECT_TRUE((IS_RECEIVE_HANDLER_V<SimpleReceiveHandler, SimpleUARTPort>));
}

TEST(ReceiveHandlerInterface, CallableObjectSatisfiesInterface) {
    // Verify at compile time
    EXPECT_TRUE((IS_RECEIVE_HANDLER_V<CountingReceiveHandler, SimpleUARTPort>));
}

TEST(ReceiveHandlerInterface, DefaultReceiveHandlerSatisfiesInterface) {
    // Verify at compile time
    EXPECT_TRUE((IS_RECEIVE_HANDLER_V<DefaultReceiveHandler, SimpleUARTPort>));
}

TEST(ReceiveHandlerInterface, DefaultHandlerBlockingMode) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'A', 'T'};

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    DefaultReceiveHandler handler;
    handler(uart, rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'A');
    EXPECT_EQ(rx_data[1], 'T');
}

TEST(ReceiveHandlerInterface, DefaultHandlerRejectsStreaming) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'A', 'T'};

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    DefaultReceiveHandler handler;
    handler(uart, rx_data, status, 1000, true); // as_stream=true

    EXPECT_EQ(status, UartStatus::ErrError);
}

TEST(ReceiveHandlerInterface, DefaultHandlerUsesDefaultTimeout) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'O', 'K'};

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    DefaultReceiveHandler handler;
    // Call without timeout - should use default 1000ms
    handler(uart, rx_data, status, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'O');
    EXPECT_EQ(rx_data[1], 'K');
}

TEST(ReceiveHandlerInterface, CountingHandlerTracksCallCount) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'O', 'K'};

    CountingReceiveHandler handler;
    EXPECT_EQ(handler.call_count, 0U);

    std::array<UInt8, 2> rx_data{};
    UartStatus status = UartStatus::Busy;

    handler(uart, rx_data, status, 1000, false);
    EXPECT_EQ(handler.call_count, 1U);
    EXPECT_EQ(status, UartStatus::Ok);

    handler(uart, rx_data, status, 1000, false);
    EXPECT_EQ(handler.call_count, 2U);
}

// ============================================================================
// Abstract Base Class Tests
// ============================================================================

TEST(IUARTPortAbstractClass, MockImplementationWorks) {
    MockUARTPort mock;
    mock.rx_buffer = {'O', 'K', '+', 'V', 'E', 'R', 'S', 'I', 'O', 'N'};

    EXPECT_EQ(mock.data_size(), UartDataSize::DataSize8b);

    // Test transmit
    std::array<UInt8, 2> tx_data = {'A', 'T'};
    UartStatus status = UartStatus::Busy;
    mock.transmit(tx_data, status, 1000);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock.tx_buffer.size(), 2U);
    EXPECT_EQ(mock.tx_count, 1U);

    // Test receive
    std::array<UInt8, 10> rx_data{};
    mock.receive(rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'O');
    EXPECT_EQ(rx_data[1], 'K');
    EXPECT_EQ(mock.rx_count, 1U);
}

TEST(IUARTPortAbstractClass, PolymorphicUsage) {
    // Test polymorphic usage through base class pointer
    MockUARTPort mock;
    mock.rx_buffer = {'X', 'Y', 'Z'};

    IUARTPort* uart_ptr = &mock;

    std::array<UInt8, 3> rx_data{};
    UartStatus status = UartStatus::Busy;

    uart_ptr->receive(rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx_data[0], 'X');
    EXPECT_EQ(rx_data[1], 'Y');
    EXPECT_EQ(rx_data[2], 'Z');
}

TEST(IUARTPortAbstractClass, MockStatusConfiguration) {
    MockUARTPort mock;
    mock.rx_buffer = {'D', 'A', 'T', 'A'};
    mock.mock_rx_status = UartStatus::ErrError;

    std::array<UInt8, 4> rx_data{};
    UartStatus status = UartStatus::Busy;

    mock.receive(rx_data, status, 1000, false);

    // Should return configured mock status
    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(UARTPortIntegration, MultipleOperationsSequence) {
    SimpleUARTPort uart;

    // First transmission
    std::array<UInt8, 2> tx1 = {'A', 'T'};
    UartStatus status = UartStatus::Busy;
    uart.transmit(tx1, status, 1000);
    EXPECT_EQ(status, UartStatus::Ok);

    // First reception
    uart.rx_buffer = {'O', 'K'};
    std::array<UInt8, 2> rx1{};
    uart.receive(rx1, status, 1000, false);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(rx1[0], 'O');
    EXPECT_EQ(rx1[1], 'K');

    // Second transmission
    std::array<UInt8, 8> tx2 = {'A', 'T', '+', 'V', 'E', 'R', 'S', '?'};
    uart.transmit(tx2, status, 1000);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(uart.tx_buffer.size(), 8U);

    // Second reception
    uart.rx_buffer = {'O', 'K', '+', 'V', '7', '0', '0'};
    std::array<UInt8, 7> rx2{};
    uart.receive(rx2, status, 1000, false);
    EXPECT_EQ(status, UartStatus::Ok);

    // Verify operation counts
    EXPECT_EQ(uart.tx_count, 2U);
    EXPECT_EQ(uart.rx_count, 2U);
}

TEST(UARTPortIntegration, HandlerWithUARTPort) {
    SimpleUARTPort uart;
    uart.rx_buffer = {'T', 'E', 'S', 'T'};

    CountingReceiveHandler handler;

    std::array<UInt8, 4> rx_data{};
    UartStatus status = UartStatus::Busy;

    // Use handler to receive data
    handler(uart, rx_data, status, 1000, false);

    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(handler.call_count, 1U);
    EXPECT_EQ(rx_data[0], 'T');
    EXPECT_EQ(rx_data[3], 'T');
}
