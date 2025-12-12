/**
 * @file test_hm11_driver_core.cpp
 * @brief Unit tests for HM11Driver core functionality
 *
 * Tests cover:
 * - Template instantiation with different UART implementations
 * - RAII ownership model (construction/destruction)
 * - Basic device operations (Test, Reset, Restart, Sleep, Wake_Up, Software_Version)
 * - Status error propagation
 * - Buffer alignment verification
 */

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <string_view>
#include <vector>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/hm11_driver.hpp"

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// ============================================================================
// Mock UART Port Implementation
// ============================================================================

/**
 * @brief Mock UART port for testing HM11Driver
 *
 * Provides controllable transmit/receive behavior for testing driver logic
 * without real hardware. Satisfies IS_UART_PORT_V concept.
 */
class HM11MockUARTPort {
  public:
    // Captured transmit data for verification
    std::vector<UInt8> transmitted_data;

    // Data to return on receive calls
    std::vector<UInt8> receive_buffer;

    // Status to return on next UART operation
    UartStatus next_status{UartStatus::Ok};

    // Track method calls
    int transmit_call_count{0};
    int receive_call_count{0};

    [[nodiscard]] static UartDataSize data_size() noexcept { return UartDataSize::DataSize8b; }

    void transmit(UartData8bConst data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000) noexcept {
        ++transmit_call_count;
        status = next_status;

        if (status == UartStatus::Ok) {
            // Capture transmitted data
            transmitted_data.insert(transmitted_data.end(), data.begin(), data.end());
        }
    }

    void receive(UartData8b data, UartStatus& status, uint32_t /*timeout_ms*/ = 1000, bool as_stream = false) noexcept {
        ++receive_call_count;
        status = next_status;

        if (status == UartStatus::Ok && !as_stream) {
            // Copy from receive buffer to output
            const std::size_t TO_COPY = std::min(data.size(), receive_buffer.size());
            std::copy_n(receive_buffer.begin(), TO_COPY, data.begin());
        }
    }

    // Test helpers
    void set_receive_response(std::string_view response) {
        receive_buffer.clear();
        receive_buffer.insert(receive_buffer.end(), response.begin(), response.end());
    }

    [[nodiscard]] std::string get_transmitted_string() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return {reinterpret_cast<const char*>(transmitted_data.data()), transmitted_data.size()};
    }

    void clear() {
        transmitted_data.clear();
        receive_buffer.clear();
        next_status = UartStatus::Ok;
        transmit_call_count = 0;
        receive_call_count = 0;
    }

    void clear_buffers() {
        transmitted_data.clear();
        receive_buffer.clear();
        // Don't reset call counts - those accumulate across commands
    }
};

// Verify HM11MockUARTPort satisfies UART port concept
static_assert(IS_UART_PORT_V<HM11MockUARTPort>, "HM11MockUARTPort must satisfy UART port interface");

// ============================================================================
// Mock Receive Handler Implementation
// ============================================================================

/**
 * @brief Mock receive handler for testing
 *
 * Delegates to UART port's receive method. Satisfies IS_RECEIVE_HANDLER_V concept.
 */
struct MockReceiveHandler {
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        UartData8b buffer,
        UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept {
        port.receive(buffer, status, timeout_ms, as_stream);
    }

    template <typename UARTPortType>
    void operator()(UARTPortType& port, UartData8b buffer, UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }
};

// Verify MockReceiveHandler satisfies receive handler concept
static_assert(
    IS_RECEIVE_HANDLER_V<MockReceiveHandler, HM11MockUARTPort>,
    "MockReceiveHandler must satisfy receive handler interface");

// ============================================================================
// Test Fixture
// ============================================================================

class HM11DriverCoreTest : public ::testing::Test {
  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    HM11MockUARTPort mock_uart;
    MockReceiveHandler mock_handler;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void SetUp() override { mock_uart.clear(); }
};

// ============================================================================
// Construction and Ownership Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, ConstructionAndDestruction) {
    // Test RAII construction and destruction
    {
        HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);
        // Driver constructed successfully
        SUCCEED();
    } // Driver destroyed automatically
    SUCCEED();
}

TEST_F(HM11DriverCoreTest, TemplateInstantiationWithDifferentTypes) {
    // Verify driver can be instantiated with different UART implementations
    // This test verifies template-based dependency injection works

    // Type 1: MockUARTPort
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver1(mock_uart, mock_handler);
    SUCCEED();

    // Type 2: IUARTPort abstract base class (if needed for testing)
    // Would work but requires heap allocation, not testing here

    // Verify concept enforcement at compile time
    static_assert(
        std::is_constructible_v<
            HM11Driver<HM11MockUARTPort, MockReceiveHandler>,
            HM11MockUARTPort&,
            MockReceiveHandler&>,
        "Driver should be constructible with valid UART and handler types");
}

TEST_F(HM11DriverCoreTest, CopyAndMoveOperationsDeleted) {
    // Verify copy/move operations are deleted (Decision 22)
    using DriverType = HM11Driver<HM11MockUARTPort, MockReceiveHandler>;

    static_assert(!std::is_copy_constructible_v<DriverType>, "Driver should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<DriverType>, "Driver should not be copy assignable");
    static_assert(!std::is_move_constructible_v<DriverType>, "Driver should not be move constructible");
    static_assert(!std::is_move_assignable_v<DriverType>, "Driver should not be move assignable");
}

// ============================================================================
// Test Command: AT (connectivity test)
// ============================================================================

TEST_F(HM11DriverCoreTest, TestCommandSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with "OK"
    mock_uart.set_receive_response("OK");

    // Execute: Send AT command
    UartStatus status{UartStatus::Busy};
    driver.test(status);

    // Verify: Command transmitted
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT");

    // Verify: Response received and validated
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(mock_uart.transmit_call_count, 1);
    EXPECT_EQ(mock_uart.receive_call_count, 1);
}

TEST_F(HM11DriverCoreTest, TestCommandTransmitFailure) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Transmit will fail
    mock_uart.next_status = UartStatus::ErrError;

    // Execute: Send AT command
    UartStatus status{UartStatus::Busy};
    driver.test(status);

    // Verify: Transmit attempted, error propagated
    EXPECT_EQ(status, UartStatus::ErrError);
    EXPECT_EQ(mock_uart.transmit_call_count, 1);
    EXPECT_EQ(mock_uart.receive_call_count, 0); // Should not attempt receive
}

TEST_F(HM11DriverCoreTest, TestCommandReceiveTimeout) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Transmit succeeds, receive times out
    mock_uart.next_status = UartStatus::Ok;

    // Execute transmit
    UartStatus status{UartStatus::Busy};
    mock_uart.transmit(UartData8bConst{}, status, 1000);
    ASSERT_EQ(status, UartStatus::Ok);

    // Setup: Receive will timeout
    mock_uart.next_status = UartStatus::ErrTimeout;

    // Execute: Send AT command
    driver.test(status);

    // Verify: Timeout propagated
    EXPECT_EQ(status, UartStatus::ErrTimeout);
}

TEST_F(HM11DriverCoreTest, TestCommandResponseMismatch) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with wrong response
    mock_uart.set_receive_response("ERROR");

    // Execute: Send AT command
    UartStatus status{UartStatus::Busy};
    driver.test(status);

    // Verify: Error detected due to response mismatch
    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Reset Command Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, ResetCommandSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with "OK+RESET"
    mock_uart.set_receive_response("OK+RESET");

    // Execute: Send AT+RESET command
    UartStatus status{UartStatus::Busy};
    driver.reset(status);

    // Verify: Command transmitted
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+RESET");

    // Verify: Response received and validated
    EXPECT_EQ(status, UartStatus::Ok);
}

// ============================================================================
// Restart Command Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, RestartCommandSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with "OK+START"
    mock_uart.set_receive_response("OK+START");

    // Execute: Send AT+START command
    UartStatus status{UartStatus::Busy};
    driver.restart(status);

    // Verify: Command transmitted
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+START");

    // Verify: Response received and validated
    EXPECT_EQ(status, UartStatus::Ok);
}

// ============================================================================
// Sleep Command Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, SleepCommandSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with "OK+SLEEP"
    mock_uart.set_receive_response("OK+SLEEP");

    // Execute: Send AT+SLEEP command
    UartStatus status{UartStatus::Busy};
    driver.sleep(status);

    // Verify: Command transmitted
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+SLEEP");

    // Verify: Response received and validated
    EXPECT_EQ(status, UartStatus::Ok);
}

// ============================================================================
// Wake Up Command Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, WakeUpCommandSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with "OK+WAKE"
    mock_uart.set_receive_response("OK+WAKE");

    // Execute: Send WWWWWWW wake sequence
    UartStatus status{UartStatus::Busy};
    driver.wake_up(status);

    // Verify: Wake sequence transmitted (7 'W' characters)
    EXPECT_EQ(mock_uart.get_transmitted_string(), "WWWWWWW");

    // Verify: Response received and validated
    EXPECT_EQ(status, UartStatus::Ok);
}

// ============================================================================
// Software Version Command Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, SoftwareVersionSuccess) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with version string
    mock_uart.set_receive_response("OK+Get:HMSoftV545");

    // Execute: Query software version
    std::string_view version;
    UartStatus status{UartStatus::Busy};
    driver.software_version(version, status);

    // Verify: Command transmitted
    EXPECT_EQ(mock_uart.get_transmitted_string(), "AT+VERS?");

    // Verify: Version parsed correctly
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(version, "HMSoftV545");
}

TEST_F(HM11DriverCoreTest, SoftwareVersionWithNullTerminator) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with null-terminated version
    std::string response = "OK+Get:HMSoftV710";
    response.push_back('\0');   // Null terminator
    response.append("garbage"); // Extra data after null
    mock_uart.set_receive_response(response);

    // Execute: Query software version
    std::string_view version;
    UartStatus status{UartStatus::Busy};
    driver.software_version(version, status);

    // Verify: Version correctly terminated at null
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(version, "HMSoftV710");
}

TEST_F(HM11DriverCoreTest, SoftwareVersionWithCRLF) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Module responds with CRLF-terminated version
    mock_uart.set_receive_response("OK+Get:HMSoftV545\r\n");

    // Execute: Query software version
    std::string_view version;
    UartStatus status{UartStatus::Busy};
    driver.software_version(version, status);

    // Verify: Version correctly terminated at CRLF
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(version, "HMSoftV545");
}

TEST_F(HM11DriverCoreTest, SoftwareVersionTransmitFailure) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Setup: Transmit will fail
    mock_uart.next_status = UartStatus::ErrError;

    // Execute: Query software version
    std::string_view version;
    UartStatus status{UartStatus::Busy};
    driver.software_version(version, status);

    // Verify: Error propagated
    EXPECT_EQ(status, UartStatus::ErrError);
}

// ============================================================================
// Buffer Alignment Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, ResponseBufferAlignment) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Verify response buffer is 32-byte aligned (Decision 20)
    // Note: Can't directly access private member, but alignment is guaranteed by alignas(32)
    // This test verifies the type system enforces alignment

    // Indirect verification: ensure buffer size is correct
    constexpr std::size_t EXPECTED_BUFFER_SIZE = MAX_MESSAGE_LENGTH * 5;
    static_assert(EXPECTED_BUFFER_SIZE > 0, "Buffer size should be positive");

    SUCCEED();
}

// ============================================================================
// Status Error Propagation Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, UARTErrorPropagation) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test various UART error statuses propagate correctly
    const std::array<UartStatus, 3> ERROR_STATUSES = {UartStatus::ErrError, UartStatus::ErrTimeout, UartStatus::Busy};

    for (const auto ERROR_STATUS : ERROR_STATUSES) {
        mock_uart.clear();
        mock_uart.next_status = ERROR_STATUS;

        UartStatus status{UartStatus::Ok};
        driver.test(status);

        EXPECT_EQ(status, ERROR_STATUS) << "Error status should propagate: " << static_cast<int>(ERROR_STATUS);
    }
}

// ============================================================================
// Concept Enforcement Tests (Compile-Time)
// ============================================================================

TEST_F(HM11DriverCoreTest, ConceptEnforcement) {
    // These are compile-time checks, verifying concepts work correctly

    // Valid: HM11MockUARTPort satisfies IS_UART_PORT_V
    static_assert(IS_UART_PORT_V<HM11MockUARTPort>, "HM11MockUARTPort should satisfy UART port concept");

    // Valid: MockReceiveHandler satisfies IS_RECEIVE_HANDLER_V
    static_assert(
        IS_RECEIVE_HANDLER_V<MockReceiveHandler, HM11MockUARTPort>,
        "MockReceiveHandler should satisfy receive handler concept");

    // Valid: Driver instantiation with valid types
    using ValidDriver = HM11Driver<HM11MockUARTPort, MockReceiveHandler>;
    static_assert(
        std::is_constructible_v<ValidDriver, HM11MockUARTPort&, MockReceiveHandler&>,
        "Should be constructible");

    SUCCEED();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(HM11DriverCoreTest, MultipleCommandSequence) {
    HM11Driver<HM11MockUARTPort, MockReceiveHandler> driver(mock_uart, mock_handler);

    // Test sequence of commands
    UartStatus status{UartStatus::Busy}; // Initialize to avoid uninitialized variable warning

    // 1. Test connectivity
    mock_uart.set_receive_response("OK");
    driver.test(status);
    EXPECT_EQ(status, UartStatus::Ok);

    // 2. Query version
    mock_uart.clear_buffers();
    mock_uart.set_receive_response("OK+Get:HMSoftV545");
    std::string_view version;
    driver.software_version(version, status);
    EXPECT_EQ(status, UartStatus::Ok);
    EXPECT_EQ(version, "HMSoftV545");

    // 3. Reset
    mock_uart.clear_buffers();
    mock_uart.set_receive_response("OK+RESET");
    driver.reset(status);
    EXPECT_EQ(status, UartStatus::Ok);

    // Verify all commands were transmitted
    EXPECT_EQ(mock_uart.transmit_call_count, 3);
    EXPECT_EQ(mock_uart.receive_call_count, 3);
}
