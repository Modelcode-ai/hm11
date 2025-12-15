#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/sync/watchdog.hpp"
#include "utils/mock_uart_port.hpp"

// Forward declarations from drivers.cpp
namespace hm11 {
void init_uart();
void initialize_dma();
void dma_receive_handler(
    UARTPort& uart,
    void* received,
    std::size_t length,
    Status& status,
    unsigned int timeout,
    bool as_stream);
void irq_uart_handler(
    Watchdog& wd,
    bool is_stream,
    std::uint8_t* received_address,
    std::size_t& position,
    std::size_t maximum,
    bool& started,
    std::chrono::steady_clock::time_point until_time,
    Status& status,
    bool idle_line_detected,
    bool idle_line_interrupt_enabled,
    bool read_data_not_empty,
    bool received_data_not_empty,
    bool parity_error,
    bool framing_error,
    bool noise_error,
    bool overrun_error,
    bool line_break_detection,
    bool clear_to_send,
    bool error_flag);
void irq_dma_handler(Watchdog& wd, bool transfer_error, bool fifo_error, bool direct_mode_error, bool transfer_complete);
} // namespace hm11

using namespace hm11;

class DriversTest : public ::testing::Test {};

// Test init_uart (stub, should not crash)
TEST_F(DriversTest, InitUART) {
    // Just verify it doesn't crash
    EXPECT_NO_THROW(init_uart());
}

// Test initialize_dma (stub, should not crash)
TEST_F(DriversTest, InitializeDMA) {
    // Just verify it doesn't crash
    EXPECT_NO_THROW(initialize_dma());
}

// Test dma_receive_handler in stream mode
TEST_F(DriversTest, DMAReceiveHandlerStreamMode) {
    std::array<std::uint8_t, 10> buffer{};
    Status status = Status::Error;
    MockUARTPort uart{}; // Create a mock UARTPort object

    dma_receive_handler(uart, buffer.data(), buffer.size(), status, 1000, true);

    // In stream mode, should return immediately with Ok
    EXPECT_EQ(status, Status::Ok);
}

// Test dma_receive_handler in non-stream mode with timeout
TEST_F(DriversTest, DMAReceiveHandlerNonStreamTimeout) {
    std::array<std::uint8_t, 10> buffer{};
    Status status = Status::Ok;
    MockUARTPort uart{}; // Create a mock UARTPort object

    auto start_time = std::chrono::steady_clock::now();
    dma_receive_handler(uart, buffer.data(), buffer.size(), status, 50, false);
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should timeout and set error status
    EXPECT_EQ(status, Status::Error);
    EXPECT_GE(elapsed.count(), 45); // Allow some tolerance
}

// Test irq_uart_handler with parity error
TEST_F(DriversTest, IRQUARTHandlerParityError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    // Call with parity error
    irq_uart_handler(
        wd,
        false,
        received_address,
        position,
        maximum,
        started,
        until_time,
        status,
        false, // idle_line_detected
        false, // idle_line_interrupt_enabled
        false, // read_data_not_empty
        false, // received_data_not_empty
        true,  // parity_error
        false, // framing_error
        false, // noise_error
        false, // overrun_error
        false, // line_break_detection
        false, // clear_to_send
        false  // error_flag
    );

    // Watchdog should have been released with error
    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with framing error
TEST_F(DriversTest, IRQUARTHandlerFramingError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd,
        false,
        received_address,
        position,
        maximum,
        started,
        until_time,
        status,
        false, // idle_line_detected
        false, // idle_line_interrupt_enabled
        false, // read_data_not_empty
        false, // received_data_not_empty
        false, // parity_error
        true,  // framing_error
        false, // noise_error
        false, // overrun_error
        false, // line_break_detection
        false, // clear_to_send
        false  // error_flag
    );

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with noise error
TEST_F(DriversTest, IRQUARTHandlerNoiseError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, false, false, false,
        false, true, false, false, false, false);

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with overrun error
TEST_F(DriversTest, IRQUARTHandlerOverrunError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, false, false, false,
        false, false, true, false, false, false);

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with line break detection
TEST_F(DriversTest, IRQUARTHandlerLineBreakDetection) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, false, false, false,
        false, false, false, true, false, false);

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with clear to send
TEST_F(DriversTest, IRQUARTHandlerClearToSend) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, false, false, false,
        false, false, false, false, true, false);

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with generic error flag
TEST_F(DriversTest, IRQUARTHandlerErrorFlag) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, false, false, false,
        false, false, false, false, false, true);

    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Error);

    wd.stop();
}

// Test irq_uart_handler with idle line detected after deadline
TEST_F(DriversTest, IRQUARTHandlerIdleLineAfterDeadline) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(100); // Already passed
    Status status = Status::Ok;

    irq_uart_handler(
        wd,
        false,
        received_address,
        position,
        maximum,
        started,
        until_time,
        status,
        true,  // idle_line_detected
        true,  // idle_line_interrupt_enabled
        false, // read_data_not_empty
        false, // received_data_not_empty
        false, // parity_error
        false, // framing_error
        false, // noise_error
        false, // overrun_error
        false, // line_break_detection
        false, // clear_to_send
        false  // error_flag
    );

    Status received_status = Status::Error;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Ok);

    wd.stop();
}

// Test irq_uart_handler with idle line detected before deadline
TEST_F(DriversTest, IRQUARTHandlerIdleLineBeforeDeadline) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000); // In future
    Status status = Status::Ok;

    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, true, true, false, false, false,
        false, false, false, false, false, false);

    // Should not release watchdog, so await_event should timeout
    Status received_status = Status::Ok;
    bool result = wd.await_event(received_status);

    // Since we didn't wait long enough and watchdog wasn't released, should timeout
    EXPECT_FALSE(result);

    wd.stop();
}

// Test irq_uart_handler with data received in non-stream mode
TEST_F(DriversTest, IRQUARTHandlerDataReceivedNonStream) {
    Watchdog wd;
    std::array<std::uint8_t, 3> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    // Receive first byte
    irq_uart_handler(
        wd,
        false,
        received_address,
        position,
        maximum,
        started,
        until_time,
        status,
        false, // idle_line_detected
        false, // idle_line_interrupt_enabled
        true,  // read_data_not_empty
        true,  // received_data_not_empty
        false, // parity_error
        false, // framing_error
        false, // noise_error
        false, // overrun_error
        false, // line_break_detection
        false, // clear_to_send
        false  // error_flag
    );

    EXPECT_EQ(position, 1);

    // Receive second byte
    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);

    EXPECT_EQ(position, 2);

    // Receive third byte - should trigger release
    irq_uart_handler(
        wd, false, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);

    Status received_status = Status::Error;
    bool result = wd.await_event(received_status);
    EXPECT_TRUE(result);
    EXPECT_EQ(received_status, Status::Ok);

    wd.stop();
}

// Test irq_uart_handler with data received in stream mode
TEST_F(DriversTest, IRQUARTHandlerDataReceivedStreamMode) {
    Watchdog wd;
    std::array<std::uint8_t, 3> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), true);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = buffer.size();
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    // Receive bytes in stream mode
    for (int i = 0; i < 5; ++i) {
        irq_uart_handler(
            wd, true, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
            false, false, false, false, false, false);
    }

    // In stream mode, position should wrap
    EXPECT_LT(position, maximum);

    wd.stop();
}

// Test irq_uart_handler with data received at buffer boundary in stream mode
TEST_F(DriversTest, IRQUARTHandlerStreamModeWrapAround) {
    Watchdog wd;
    std::array<std::uint8_t, 3> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), true);

    std::uint8_t* received_address = buffer.data();
    std::size_t position = 0;
    std::size_t maximum = 3;
    bool started = true;
    auto until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    Status status = Status::Ok;

    // Receive until position == maximum
    irq_uart_handler(
        wd, true, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);
    EXPECT_EQ(position, 1);

    irq_uart_handler(
        wd, true, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);
    EXPECT_EQ(position, 2);

    irq_uart_handler(
        wd, true, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);
    EXPECT_EQ(position, 3);

    // Next should wrap to 0
    irq_uart_handler(
        wd, true, received_address, position, maximum, started, until_time, status, false, false, true, true, false,
        false, false, false, false, false, false);
    EXPECT_EQ(position, 0);

    wd.stop();
}

// Test irq_dma_handler with transfer complete
TEST_F(DriversTest, IRQDMAHandlerTransferComplete) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    irq_dma_handler(wd, false, false, false, true);

    Status status = Status::Error;
    bool result = wd.await_event(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);

    wd.stop();
}

// Test irq_dma_handler with transfer error
TEST_F(DriversTest, IRQDMAHandlerTransferError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    irq_dma_handler(wd, true, false, false, false);

    Status status = Status::Ok;
    bool result = wd.await_event(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Error);

    wd.stop();
}

// Test irq_dma_handler with FIFO error
TEST_F(DriversTest, IRQDMAHandlerFIFOError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    irq_dma_handler(wd, false, true, false, false);

    Status status = Status::Ok;
    bool result = wd.await_event(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Error);

    wd.stop();
}

// Test irq_dma_handler with direct mode error
TEST_F(DriversTest, IRQDMAHandlerDirectModeError) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    irq_dma_handler(wd, false, false, true, false);

    Status status = Status::Ok;
    bool result = wd.await_event(status);
    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Error);

    wd.stop();
}

// Test irq_dma_handler with no errors and no transfer complete (should not signal)
TEST_F(DriversTest, IRQDMAHandlerNoEvent) {
    Watchdog wd;
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(100), false);

    irq_dma_handler(wd, false, false, false, false);

    // Watchdog should not be released, so await_event should timeout
    Status status = Status::Ok;
    bool result = wd.await_event(status);

    EXPECT_FALSE(result);

    wd.stop();
}
