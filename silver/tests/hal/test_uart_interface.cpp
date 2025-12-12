// Tests for UART Interface Concept and Mock Implementation
// Validates C++20 concept constraints and mock UART behavior

#include <gtest/gtest.h>

#include <cstdint>
#include <hm11/hal.hpp>
#include <ranges>
#include <vector>

using namespace hm11::hal;

// Test fixture for UART interface concept validation
class UARTInterfaceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Reset mock UART to default state
        mock_uart_8b = MockUART{UARTDataSize::DataSize8b};
        mock_uart_9b = MockUART{UARTDataSize::DataSize9b};
        mock_uart_8b.ClearBuffers();
        mock_uart_9b.ClearBuffers();
        mock_uart_8b.SetForcedStatus(UARTStatus::Ok);
        mock_uart_9b.SetForcedStatus(UARTStatus::Ok);
    }

  private:
    MockUART mock_uart_8b{UARTDataSize::DataSize8b};
    MockUART mock_uart_9b{UARTDataSize::DataSize9b};

  protected:
    MockUART& get_mock_uart_8b() { return mock_uart_8b; }
    MockUART& get_mock_uart_9b() { return mock_uart_9b; }
};

// Concept validation tests
TEST_F(UARTInterfaceTest, ConceptValidation) {
    // Verify that MockUART satisfies UARTInterface concept
    EXPECT_TRUE((UARTInterface<MockUART>));

    // Verify static assertions in MockUART compile
    // This test passes by virtue of compilation success
    SUCCEED() << "MockUART satisfies UARTInterface concept requirements";
}

TEST_F(UARTInterfaceTest, DataSizeQuery) {
    auto& uart_8b = get_mock_uart_8b();
    auto& uart_9b = get_mock_uart_9b();

    // Test 8-bit data size configuration
    EXPECT_EQ(uart_8b.DataSize(), UARTDataSize::DataSize8b);

    // Test 9-bit data size configuration
    EXPECT_EQ(uart_9b.DataSize(), UARTDataSize::DataSize9b);
}

TEST_F(UARTInterfaceTest, Transmit8BitData) {
    const std::vector<std::uint8_t> TEST_DATA = {0x41, 0x54, 0x0D, 0x0A}; // "AT\r\n"
    UARTData8bConst data_span{TEST_DATA};
    UARTStatus status = UARTStatus::Busy;
    auto& uart = get_mock_uart_8b();

    // Test transmit with explicit timeout
    uart.Transmit(data_span, status, 500);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(uart.GetTransmitted8b(), TEST_DATA);

    // Clear and test transmit with default timeout
    uart.ClearBuffers();
    uart.Transmit(data_span, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(uart.GetTransmitted8b(), TEST_DATA);
}

TEST_F(UARTInterfaceTest, Transmit9BitData) {
    const std::vector<std::uint16_t> TEST_DATA = {0x141, 0x154, 0x10D, 0x10A}; // Extended 9-bit data
    UARTData9bConst data_span{TEST_DATA};
    UARTStatus status = UARTStatus::Busy;
    auto& uart = get_mock_uart_9b();

    // Test transmit with explicit timeout
    uart.Transmit(data_span, status, 500);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(uart.GetTransmitted9b(), TEST_DATA);

    // Clear and test transmit with default timeout
    uart.ClearBuffers();
    uart.Transmit(data_span, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(uart.GetTransmitted9b(), TEST_DATA);
}

TEST_F(UARTInterfaceTest, Receive8BitData) {
    const std::vector<std::uint8_t> EXPECTED_DATA = {0x4F, 0x4B, 0x0D, 0x0A}; // "OK\r\n"
    auto& uart = get_mock_uart_8b();
    uart.SetReceiveData(EXPECTED_DATA);

    std::vector<std::uint8_t> receive_buffer(10, 0);
    UARTData8b data_span{receive_buffer};
    UARTStatus status = UARTStatus::Busy;

    // Test receive with explicit timeout
    uart.Receive(data_span, status, 500);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify received data matches expected
    const auto EXPECTED_SIZE = static_cast<long>(EXPECTED_DATA.size());
    std::vector<std::uint8_t> received_data(receive_buffer.begin(), receive_buffer.begin() + EXPECTED_SIZE);
    EXPECT_EQ(received_data, EXPECTED_DATA);

    // Test receive with default timeout
    std::ranges::fill(receive_buffer, 0);
    uart.Receive(data_span, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(UARTInterfaceTest, Receive9BitData) {
    const std::vector<std::uint16_t> EXPECTED_DATA = {0x14F, 0x14B, 0x10D, 0x10A}; // Extended 9-bit data
    auto& uart = get_mock_uart_9b();
    uart.SetReceiveData(EXPECTED_DATA);

    std::vector<std::uint16_t> receive_buffer(10, 0);
    UARTData9b data_span{receive_buffer};
    UARTStatus status = UARTStatus::Busy;

    // Test receive with explicit timeout
    uart.Receive(data_span, status, 500);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify received data matches expected
    const auto EXPECTED_SIZE = static_cast<long>(EXPECTED_DATA.size());
    std::vector<std::uint16_t> received_data(receive_buffer.begin(), receive_buffer.begin() + EXPECTED_SIZE);
    EXPECT_EQ(received_data, EXPECTED_DATA);

    // Test receive with default timeout
    std::ranges::fill(receive_buffer, 0);
    uart.Receive(data_span, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(UARTInterfaceTest, StatusErrorHandling) {
    const std::vector<std::uint8_t> TEST_DATA = {0x41, 0x54};
    UARTData8bConst data_span{TEST_DATA};
    UARTStatus status = UARTStatus::Ok;
    auto& uart = get_mock_uart_8b();

    // Test error status propagation
    uart.SetForcedStatus(UARTStatus::ErrError);
    uart.Transmit(data_span, status);
    EXPECT_EQ(status, UARTStatus::ErrError);

    // Test timeout status
    uart.SetForcedStatus(UARTStatus::ErrTimeout);
    uart.Transmit(data_span, status);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);

    // Test busy status
    uart.SetForcedStatus(UARTStatus::Busy);
    uart.Transmit(data_span, status);
    EXPECT_EQ(status, UARTStatus::Busy);
}

TEST_F(UARTInterfaceTest, TimeoutSimulation) {
    const std::vector<std::uint8_t> TEST_DATA = {0x41, 0x54};
    UARTData8bConst data_span{TEST_DATA};
    UARTStatus status = UARTStatus::Ok;
    auto& uart = get_mock_uart_8b();

    // Configure timeout simulation requiring 2000ms
    uart.SetTimeoutSimulation(true, 2000);

    // Short timeout should result in timeout error
    uart.Transmit(data_span, status, 1000);
    EXPECT_EQ(status, UARTStatus::ErrTimeout);

    // Long enough timeout should succeed
    uart.Transmit(data_span, status, 3000);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Disable timeout simulation
    uart.SetTimeoutSimulation(false);
    uart.Transmit(data_span, status, 100);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(UARTInterfaceTest, DataSizeValidation) {
    const std::vector<std::uint8_t> TEST_DATA_8B = {0x41, 0x54};
    const std::vector<std::uint16_t> TEST_DATA_9B = {0x141, 0x154};
    UARTStatus status = UARTStatus::Ok;
    auto& uart_8b = get_mock_uart_8b();
    auto& uart_9b = get_mock_uart_9b();

    // 8-bit UART should reject 9-bit operations
    uart_8b.Transmit(UARTData9bConst{TEST_DATA_9B}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);

    std::vector<std::uint16_t> receive_buffer_9b(10);
    uart_8b.Receive(UARTData9b{receive_buffer_9b}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);

    // 9-bit UART should reject 8-bit operations
    uart_9b.Transmit(UARTData8bConst{TEST_DATA_8B}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);

    std::vector<std::uint8_t> receive_buffer_8b(10);
    uart_9b.Receive(UARTData8b{receive_buffer_8b}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(UARTInterfaceTest, BufferManagement) {
    const std::vector<std::uint8_t> DATA1 = {0x41, 0x54};
    const std::vector<std::uint8_t> DATA2 = {0x4F, 0x4B};
    UARTStatus status = UARTStatus::Busy;
    auto& uart = get_mock_uart_8b();

    // Transmit first data set
    uart.Transmit(UARTData8bConst{DATA1}, status);
    EXPECT_EQ(uart.GetTransmitted8b(), DATA1);

    // Transmit second data set (should replace first)
    uart.Transmit(UARTData8bConst{DATA2}, status);
    EXPECT_EQ(uart.GetTransmitted8b(), DATA2);

    // Clear buffers
    uart.ClearBuffers();
    EXPECT_TRUE(uart.GetTransmitted8b().empty());
    EXPECT_TRUE(uart.GetTransmitted9b().empty());
}

// Compile-time concept validation tests
namespace concept_tests {
// These tests validate concepts at compile time

// A minimal UART implementation for concept testing
struct MinimalUART {
    static UARTDataSize DataSize() { return UARTDataSize::DataSize8b; }
    static void Transmit(UARTData8bConst /*data*/, UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void Transmit(UARTData9bConst /*data*/, UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void Transmit(UARTData8bConst /*data*/, UARTStatus& /*status*/) {}
    static void Transmit(UARTData9bConst /*data*/, UARTStatus& /*status*/) {}
    static void Receive(UARTData8b /*data*/, UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void Receive(UARTData9b /*data*/, UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void Receive(UARTData8b /*data*/, UARTStatus& /*status*/) {}
    static void Receive(UARTData9b /*data*/, UARTStatus& /*status*/) {}
};

// Concept validation at compile time
static_assert(UARTInterface<MinimalUART>, "MinimalUART should satisfy concept");
static_assert(UARTInterface<MockUART>, "MockUART should satisfy concept");

// Negative test - incomplete interface
struct IncompleteUART {
    static UARTDataSize data_size() { return UARTDataSize::DataSize8b; }
    // Missing required methods
};

static_assert(!UARTInterface<IncompleteUART>, "IncompleteUART should not satisfy concept");
} // namespace concept_tests

TEST_F(UARTInterfaceTest, ConceptCompileTimeValidation) {
    // This test validates that our concept checks work at compile time
    // The static_assert statements above ensure proper concept validation
    SUCCEED() << "Concept validation passed at compile time";
}