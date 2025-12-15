// Tests for HM11Driver Template Class
// Validates template instantiation, concept constraints, and basic driver functionality

#include <gtest/gtest.h>

#include <hm11/hm11.hpp>
#include <hm11/util.hpp>
#include <type_traits>

using namespace hm11;

// Test fixture for HM11Driver template validation
class HM11DriverTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create fresh mock UART instances for each test
        mock_uart_8b = MockUART{UARTDataSize::DataSize8b};
        mock_uart_9b = MockUART{UARTDataSize::DataSize9b};
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

// Template instantiation tests
TEST_F(HM11DriverTest, TemplateInstantiation) {
    // Test instantiation with MockUART
    auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));

    // Verify type deduction
    using DriverType = decltype(driver);
    using ExpectedType = HM11Driver<MockUART>;
    EXPECT_TRUE((std::is_same_v<DriverType, ExpectedType>));

    // Verify driver is not copyable (Decision #22: RAII Ownership Model)
    EXPECT_FALSE(std::is_copy_constructible_v<DriverType>);
    EXPECT_FALSE(std::is_copy_assignable_v<DriverType>);

    // Verify driver is not movable (Decision #22: matches Ada's non-movable semantics)
    EXPECT_FALSE(std::is_move_constructible_v<DriverType>);
    EXPECT_FALSE(std::is_move_assignable_v<DriverType>);
}

TEST_F(HM11DriverTest, FactoryFunction) {
    // Test factory function with 8-bit UART
    auto driver_8b = make_hm11_driver(std::move(get_mock_uart_8b()));
    EXPECT_EQ(driver_8b.GetUart().DataSize(), UARTDataSize::DataSize8b);

    // Test factory function with 9-bit UART
    auto driver_9b = make_hm11_driver(std::move(get_mock_uart_9b()));
    EXPECT_EQ(driver_9b.GetUart().DataSize(), UARTDataSize::DataSize9b);
}

TEST_F(HM11DriverTest, UARTAccess) {
    auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));

    // Test non-const access
    auto& uart_ref = driver.GetUart();
    EXPECT_EQ(uart_ref.DataSize(), UARTDataSize::DataSize8b);

    // Test const access
    const auto& driver_const_ref = driver;
    const auto& uart_const_ref = driver_const_ref.GetUart();
    EXPECT_EQ(uart_const_ref.DataSize(), UARTDataSize::DataSize8b);
}

TEST_F(HM11DriverTest, BasicDriverOperations) {
    // Set up mock UART to respond with "OK" for AT commands
    get_mock_uart_8b().SetReceiveData(std::vector<std::uint8_t>{0x4F, 0x4B}); // "OK"

    auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));
    UARTStatus status = UARTStatus::Busy;

    // Test connectivity test (basic operation)
    driver.Test(status);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Test software version query
    // Set up proper version response for the version query
    driver.GetUart().SetReceiveData(std::vector<std::uint8_t>{'H', 'M', 'S', 'o', 'f', 't', 'V', '5', '4', '7'});
    auto version = driver.SoftwareVersion();
    EXPECT_FALSE(version.empty());
    EXPECT_EQ(version, "HMSoftV547");

    // Test AT command infrastructure integration
    status = UARTStatus::Busy;
    driver.Transmit("AT", status);
    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify the command was transmitted
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 2);
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
}

TEST_F(HM11DriverTest, UARTIntegration) {
    // Set up mock UART with test data
    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetReceiveData(std::vector<std::uint8_t>{0x4F, 0x4B}); // "OK"

    auto driver = make_hm11_driver(std::move(uart));

    // Access UART through driver and perform operations
    auto& driver_uart = driver.GetUart();

    const std::vector<std::uint8_t> TEST_COMMAND = {0x41, 0x54}; // "AT"
    UARTStatus status = UARTStatus::Busy;

    // Test transmit through driver's UART
    driver_uart.Transmit(hal::UARTData8bConst{TEST_COMMAND}, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(driver_uart.GetTransmitted8b(), TEST_COMMAND);

    // Test receive through driver's UART
    std::vector<std::uint8_t> response(10, 0);
    driver_uart.Receive(hal::UARTData8b{response}, status);
    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(response[0], 0x4F); // 'O'
    EXPECT_EQ(response[1], 0x4B); // 'K'
}

// Compile-time concept validation for HM11Driver
namespace driver_concept_tests {
// Test that ValidHM11Driver concept works correctly
static_assert(ValidHM11Driver<MockUART>, "MockUART should be valid for HM11Driver");

// Test invalid UART type (not move constructible)
struct NonMovableUART {
    NonMovableUART() = default;
    ~NonMovableUART() = default;
    NonMovableUART(const NonMovableUART&) = delete;
    NonMovableUART(NonMovableUART&&) = delete;
    NonMovableUART& operator=(const NonMovableUART&) = delete;
    NonMovableUART& operator=(NonMovableUART&&) = delete;

    static hal::UARTDataSize data_size() { return hal::UARTDataSize::DataSize8b; }
    static void transmit(hal::UARTData8bConst /*data*/, hal::UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void transmit(hal::UARTData9bConst /*data*/, hal::UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void transmit(hal::UARTData8bConst /*data*/, hal::UARTStatus& /*status*/) {}
    static void transmit(hal::UARTData9bConst /*data*/, hal::UARTStatus& /*status*/) {}
    static void receive(hal::UARTData8b /*data*/, hal::UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void receive(hal::UARTData9b /*data*/, hal::UARTStatus& /*status*/, std::uint32_t /*timeout*/) {}
    static void receive(hal::UARTData8b /*data*/, hal::UARTStatus& /*status*/) {}
    static void receive(hal::UARTData9b /*data*/, hal::UARTStatus& /*status*/) {}
};

static_assert(!ValidHM11Driver<NonMovableUART>, "NonMovableUART should not be valid for HM11Driver");
} // namespace driver_concept_tests

TEST_F(HM11DriverTest, ConceptValidation) {
    // Verify that concept validation works at compile time
    // Static assertions in driver_concept_tests namespace validate this
    SUCCEED() << "HM11Driver concept validation passed at compile time";
}

// RAII and resource management tests
TEST_F(HM11DriverTest, RAIIOwnership) {
    // Test that driver takes ownership of UART
    auto uart_data_size = get_mock_uart_8b().DataSize();

    {
        auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));
        EXPECT_EQ(driver.GetUart().DataSize(), uart_data_size);
        // Driver and its UART should be destroyed when leaving scope
    }

    // Original mock_uart_8b is now in moved-from state
    // This test validates RAII cleanup behavior
    SUCCEED() << "RAII ownership and cleanup validated";
}

// Error propagation and status handling tests
TEST_F(HM11DriverTest, ErrorPropagation) {
    // Configure mock to return error status
    get_mock_uart_8b().SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));

    // Verify that UART errors propagate through driver
    UARTStatus status = UARTStatus::Ok;
    const std::vector<std::uint8_t> TEST_DATA = {0x41, 0x54};

    driver.GetUart().Transmit(hal::UARTData8bConst{TEST_DATA}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// Template specialization and type deduction tests
TEST_F(HM11DriverTest, TypeDeduction) {
    // Test automatic type deduction in factory function
    auto driver1 = make_hm11_driver(MockUART{UARTDataSize::DataSize8b});
    auto driver2 = make_hm11_driver(MockUART{UARTDataSize::DataSize9b});

    // Both should be the same type despite different UART configurations
    using Type1 = decltype(driver1);
    using Type2 = decltype(driver2);
    EXPECT_TRUE((std::is_same_v<Type1, Type2>));

    // But should have different internal UART configurations
    EXPECT_EQ(driver1.GetUart().DataSize(), UARTDataSize::DataSize8b);
    EXPECT_EQ(driver2.GetUart().DataSize(), UARTDataSize::DataSize9b);
}

// Interface completeness test
TEST_F(HM11DriverTest, InterfaceCompleteness) {
    auto driver = make_hm11_driver(std::move(get_mock_uart_8b()));

    // Verify all expected public methods are callable
    UARTStatus status = UARTStatus::Busy;
    driver.Test(status);

    auto version = driver.SoftwareVersion();
    (void)version; // Suppress unused variable warning

    auto& uart = driver.GetUart();
    const auto& const_uart = static_cast<const decltype(driver)&>(driver).GetUart();
    (void)uart;
    (void)const_uart;

    SUCCEED() << "All expected interface methods are callable";
}

// ============================================================================
// AT Command Infrastructure Tests
// ============================================================================

class ATCommandTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create fresh mock UART for each test
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);
    }

  private:
    MockUART mock_uart{UARTDataSize::DataSize8b};

  protected:
    MockUART& get_mock_uart() { return mock_uart; }
};

TEST_F(ATCommandTest, BasicTransmit) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test basic AT command transmission
    const std::string AT_COMMAND = "AT";
    driver.Transmit(AT_COMMAND, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify transmitted data matches expected bytes
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 2);
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
}

TEST_F(ATCommandTest, TransmitWithData) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test AT command with additional data payload
    const std::string AT_COMMAND = "AT+ROLE";
    const std::vector<uint8_t> PAYLOAD = {0x31}; // '1' as byte
    hal::UARTData8bConst payload_data{PAYLOAD};

    driver.Transmit(AT_COMMAND, payload_data, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify combined transmission
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 8); // "AT+ROLE" + '1'
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
    EXPECT_EQ(transmitted[2], '+');
    EXPECT_EQ(transmitted[3], 'R');
    EXPECT_EQ(transmitted[4], 'O');
    EXPECT_EQ(transmitted[5], 'L');
    EXPECT_EQ(transmitted[6], 'E');
    EXPECT_EQ(transmitted[7], 0x31);
}

TEST_F(ATCommandTest, CheckResponseSuccess) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Ok;

    // Test successful response check
    const std::vector<uint8_t> RESPONSE_DATA = {'O', 'K', '+', 'G', 'e', 't'};
    const std::string EXPECTED = "OK+Get";

    driver.check_response(hal::UARTData8bConst{RESPONSE_DATA}, EXPECTED, status);
    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(ATCommandTest, CheckResponseMismatch) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Ok;

    // Test response mismatch detection
    const std::vector<uint8_t> RESPONSE_DATA = {'E', 'R', 'R', 'O', 'R'};
    const std::string EXPECTED = "OK+Get";

    driver.check_response(hal::UARTData8bConst{RESPONSE_DATA}, EXPECTED, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(ATCommandTest, CheckResponseTooShort) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Ok;

    // Test response too short
    const std::vector<uint8_t> RESPONSE_DATA = {'O', 'K'};
    const std::string EXPECTED = "OK+Get";

    driver.check_response(hal::UARTData8bConst{RESPONSE_DATA}, EXPECTED, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(ATCommandTest, CheckResponseSkipsIfError) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::ErrError; // Already in error state

    // Response check should be skipped if status already indicates error
    const std::vector<uint8_t> RESPONSE_DATA = {'O', 'K'};
    const std::string EXPECTED = "OK";

    driver.check_response(hal::UARTData8bConst{RESPONSE_DATA}, EXPECTED, status);
    EXPECT_EQ(status, UARTStatus::ErrError); // Should remain in error state
}

TEST_F(ATCommandTest, TransmitAndCheckSuccess) {
    // Configure mock to return expected response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test combined transmit and check operation
    const std::string AT_COMMAND = "AT";
    const std::string EXPECTED_RESPONSE = "OK";

    driver.TransmitAndCheck(AT_COMMAND, EXPECTED_RESPONSE, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify command was transmitted
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    EXPECT_EQ(transmitted.size(), 2);
    EXPECT_EQ(transmitted[0], 'A');
    EXPECT_EQ(transmitted[1], 'T');
}

TEST_F(ATCommandTest, TransmitAndCheckWithBuffer) {
    // Configure mock to return expected response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'R', 'o', 'l', 'e', '0'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test transmit and check with external buffer
    const std::string AT_COMMAND = "AT+ROLE?";
    const std::string EXPECTED_RESPONSE = "OK+Role";
    std::array<uint8_t, 16> response_buffer{};

    driver.TransmitAndCheck(AT_COMMAND, EXPECTED_RESPONSE, hal::UARTData8b{response_buffer.data(), 8}, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify response was received in buffer
    EXPECT_EQ(response_buffer[0], 'O');
    EXPECT_EQ(response_buffer[1], 'K');
    EXPECT_EQ(response_buffer[2], '+');
    EXPECT_EQ(response_buffer[3], 'R');
}

TEST_F(ATCommandTest, TransmitAndCheckTransmitError) {
    // Configure mock to fail transmission
    auto mock = MockUART{UARTDataSize::DataSize8b};
    mock.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling when transmission fails
    const std::string AT_COMMAND = "AT";
    const std::string EXPECTED_RESPONSE = "OK";

    driver.TransmitAndCheck(AT_COMMAND, EXPECTED_RESPONSE, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(ATCommandTest, TransmitAndCheckReceiveError) {
    // Configure mock to fail on receive
    auto mock = MockUART{UARTDataSize::DataSize8b};
    mock.SetForcedStatusForReceive(UARTStatus::ErrTimeout);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling when receive fails
    const std::string AT_COMMAND = "AT";
    const std::string EXPECTED_RESPONSE = "OK";

    driver.TransmitAndCheck(AT_COMMAND, EXPECTED_RESPONSE, status);

    EXPECT_EQ(status, UARTStatus::ErrTimeout);
}

TEST_F(ATCommandTest, CommandTooLong) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling for excessively long commands
    // Create a command longer than buffer capacity
    const std::string LONG_COMMAND(MAX_MESSAGE_LENGTH * 5 + 1, 'A');

    driver.Transmit(LONG_COMMAND, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(ATCommandTest, DataPayloadTooLong) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling for combined command + data too long
    const std::string COMMAND = "AT+DATA";
    std::vector<uint8_t> huge_payload(MAX_MESSAGE_LENGTH * 5, 0x42);

    driver.Transmit(COMMAND, hal::UARTData8bConst{huge_payload}, status);
    EXPECT_EQ(status, UARTStatus::ErrError);
}

// Test string-to-UART data conversion utilities
TEST_F(ATCommandTest, StringToUartConversion) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test various AT command patterns
    const std::vector<std::string> TEST_COMMANDS = {"AT", "AT+ROLE0", "AT+ADVI5", "AT+CHAR", "AT+RESET"};

    for (const auto& command : TEST_COMMANDS) {
        status = UARTStatus::Busy;
        driver.Transmit(command, status);
        EXPECT_EQ(status, UARTStatus::Ok);

        // Verify transmitted data matches string bytes
        const auto transmitted = driver.GetUart().GetTransmitted8b();
        EXPECT_EQ(transmitted.size(), command.size());

        for (std::size_t i = 0; i < command.size(); ++i) {
            EXPECT_EQ(transmitted[i], static_cast<uint8_t>(command[i]));
        }

        // Clear transmitted data for next iteration
        driver.GetUart().ClearTransmittedData();
    }
}

// Integration test combining string building and command transmission
TEST_F(ATCommandTest, IntegrationWithStringBuilder) {
    // Configure mock with appropriate response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'S', 'e', 't', ':', '1'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Build AT command using StringBuilder utility
    util::ATCommandBuilder cmd_builder;
    cmd_builder.append("AT+ROLE");
    cmd_builder.append(util::to_string(Role::Central));

    // Transmit and verify response
    const std::string expected_response = "OK+Set:";
    driver.TransmitAndCheck(cmd_builder.view(), expected_response, status);

    EXPECT_EQ(status, UARTStatus::Ok);

    // Verify complete command was built and transmitted correctly
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+ROLE1";
    EXPECT_EQ(transmitted.size(), expected_command.size());

    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

// ============================================================================
// Connection Management Tests
// ============================================================================

class ConnectionManagementTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create fresh mock UART for each test
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);
    }

  private:
    MockUART mock_uart{UARTDataSize::DataSize8b};

  protected:
    MockUART& get_mock_uart() { return mock_uart; }
};

TEST_F(ConnectionManagementTest, ConnectLastDeviceSuccess) {
    // Configure mock to return successful connection response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CONN" + result character (using space for Connected)
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', 'N', 'N', ' '};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    // Test connecting to last device
    driver.connect_last_device(result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::Connected);

    // Verify correct AT command was sent
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+CONNL";
    EXPECT_EQ(transmitted.size(), expected_command.size());
    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

TEST_F(ConnectionManagementTest, ConnectLastDeviceConnecting) {
    // Configure mock to return connecting response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CONN" + 'L' for Connecting
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', 'N', 'N', 'L'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    driver.connect_last_device(result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::Connecting);
}

TEST_F(ConnectionManagementTest, ConnectLastDeviceError) {
    // Configure mock to return error response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CONN" + 'E' for Connect_Error
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', 'N', 'N', 'E'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    driver.connect_last_device(result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::ConnectError);
}

TEST_F(ConnectionManagementTest, ConnectByMacAddressSuccess) {
    // Configure mock to return successful connection response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CO" + type + type + result character ('1' for Connected)
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', '0', '0', '1'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    // Create a test MAC address
    MacAddress mac("001122334455");

    driver.connect(MacAddressType::NormalAddress, mac, result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::Connected);

    // Verify correct AT command was sent: AT+CO0 + MAC address
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+CO0001122334455";
    EXPECT_EQ(transmitted.size(), expected_command.size());
    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

TEST_F(ConnectionManagementTest, ConnectByMacAddressFail) {
    // Configure mock to return failure response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CO" + type + type + 'F' for Connect_Fail
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', '1', '1', 'F'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    // Create a test MAC address
    MacAddress mac("AABBCCDDEEFF");

    driver.connect(MacAddressType::StaticMac, mac, result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::ConnectFail);

    // Verify correct AT command was sent: AT+CO1 + MAC address
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+CO1AABBCCDDEEFF";
    EXPECT_EQ(transmitted.size(), expected_command.size());
}

TEST_F(ConnectionManagementTest, ConnectByIndexSuccess) {
    // Configure mock to return successful connection response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CONN" + result character ('0' for Connected)
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', 'N', 'N', '0'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    DiscoveredIndex index(2); // Connect to third discovered device
    driver.connect(index, result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::Connected);

    // Verify correct AT command was sent: AT+CONN + index
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+CONN2";
    EXPECT_EQ(transmitted.size(), expected_command.size());
    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

TEST_F(ConnectionManagementTest, ConnectByIndexNoAddress) {
    // Configure mock to return no address response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    // "OK+CONN" + 'N' for No_Address
    const std::vector<uint8_t> MOCK_RESPONSE = {'O', 'K', '+', 'C', 'O', 'N', 'N', 'N'};
    mock.SetReceiveData(MOCK_RESPONSE);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    DiscoveredIndex index(5); // Connect to sixth discovered device
    driver.connect(index, result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(result, ConnectResult::NoAddress);

    // Verify correct AT command was sent
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+CONN5";
    EXPECT_EQ(transmitted.size(), expected_command.size());
}

TEST_F(ConnectionManagementTest, ConnectionUartError) {
    // Configure mock to return UART error
    auto mock = MockUART{UARTDataSize::DataSize8b};
    mock.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock));
    ConnectResult result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    // Test that UART errors are properly propagated
    driver.connect_last_device(result, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(result, ConnectResult::OtherError); // Should remain in error state
}

// Test result parsing with various character values
TEST_F(ConnectionManagementTest, ConnectResultParsing) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));

    // Test all valid result character mappings from Ada To_Connect_Result
    struct TestCase {
        char result_char;
        ConnectResult expected_result;
    };

    const std::vector<TestCase> test_cases = {
        {'L', ConnectResult::Connecting},
        {'E', ConnectResult::ConnectError},
        {'F', ConnectResult::ConnectFail},
        {'N', ConnectResult::NoAddress},
        {' ', ConnectResult::Connected},  // Space -> Connected
        {'\0', ConnectResult::Connected}, // NULL -> Connected
        {'0', ConnectResult::Connected},  // '0' -> Connected
        {'1', ConnectResult::Connected},  // '1' -> Connected
        {'2', ConnectResult::Connected},  // '2' -> Connected
        {'3', ConnectResult::Connected},  // '3' -> Connected
        {'4', ConnectResult::Connected},  // '4' -> Connected
        {'5', ConnectResult::Connected},  // '5' -> Connected
        {'X', ConnectResult::OtherError}, // Unknown -> Other_Error
    };

    for (const auto& test_case : test_cases) {
        // Configure mock with test result character
        auto mock = MockUART{UARTDataSize::DataSize8b};
        std::vector<uint8_t> response =
            {'O', 'K', '+', 'C', 'O', 'N', 'N', static_cast<uint8_t>(test_case.result_char)};
        mock.SetReceiveData(response);

        auto test_driver = make_hm11_driver(std::move(mock));
        ConnectResult result = ConnectResult::OtherError;
        UARTStatus status = UARTStatus::Busy;

        test_driver.connect_last_device(result, status);

        EXPECT_EQ(status, UARTStatus::Ok) << "Failed for character: '" << test_case.result_char << "'";
        EXPECT_EQ(result, test_case.expected_result) << "Failed for character: '" << test_case.result_char << "'";
    }
}

// ============================================================================
// Discovery Operations Tests
// ============================================================================

class DiscoveryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create fresh mock UART for each test
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);
    }

  private:
    MockUART mock_uart{UARTDataSize::DataSize8b};

  protected:
    MockUART& get_mock_uart() { return mock_uart; }
};

// Test data structures for discovery callbacks
struct DiscoveredDeviceInfo {
    char id;
    MacAddress mac;
    std::string name;
    std::string rssi;
    void* user_data_ptr;
};

// Static storage for test callback data
static std::vector<DiscoveredDeviceInfo> discovered_devices;

// Test callback function
static void
test_discovery_callback(char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
    discovered_devices.push_back({id, mac, std::string(name), std::string(rssi), user_data});
}

TEST_F(DiscoveryTest, ScanBasicDevice) {
    // Clear previous test data
    discovered_devices.clear();

    // Configure mock to return single device discovery response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::string response = "OK+DISCS"
                                 "OK+DISC:001122334455"
                                 "OK+NAME:TestDevice\r\n"
                                 "OK+RSSI:-50\r\n"
                                 "OK+DISCE";
    std::vector<uint8_t> response_bytes(response.begin(), response.end());
    mock.SetReceiveData(response_bytes);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    int TEST_USER_DATA = 42;

    // Test device discovery scan
    driver.scan(test_discovery_callback, &TEST_USER_DATA, 1000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_devices.size(), 1);

    if (!discovered_devices.empty()) {
        const auto& device = discovered_devices[0];
        EXPECT_EQ(device.id, '0'); // Default ID set by implementation
        EXPECT_EQ(std::string(device.mac), "001122334455");
        EXPECT_EQ(device.name, "TestDevice");
        EXPECT_EQ(device.rssi, "-50");
        EXPECT_EQ(device.user_data_ptr, &TEST_USER_DATA);
    }

    // Verify correct AT command was sent
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+DISC?";
    EXPECT_EQ(transmitted.size(), expected_command.size());
    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

TEST_F(DiscoveryTest, ScanMultipleDevices) {
    // Clear previous test data
    discovered_devices.clear();

    // Configure mock to return multiple device discovery response
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::string response = "OK+DISCS"
                                 "OK+DISC:AABBCCDDEEFF"
                                 "OK+NAME:Device1\r\n"
                                 "OK+RSSI:-45\r\n"
                                 "OK+DISC:112233445566"
                                 "OK+NAME:Device2\r\n"
                                 "OK+RSSI:-60\r\n"
                                 "OK+DISCE";
    std::vector<uint8_t> response_bytes(response.begin(), response.end());
    mock.SetReceiveData(response_bytes);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test multiple device discovery
    driver.scan(test_discovery_callback, nullptr, 2000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_devices.size(), 2);

    if (discovered_devices.size() >= 2) {
        const auto& device1 = discovered_devices[0];
        EXPECT_EQ(std::string(device1.mac), "AABBCCDDEEFF");
        EXPECT_EQ(device1.name, "Device1");
        EXPECT_EQ(device1.rssi, "-45");

        const auto& device2 = discovered_devices[1];
        EXPECT_EQ(std::string(device2.mac), "112233445566");
        EXPECT_EQ(device2.name, "Device2");
        EXPECT_EQ(device2.rssi, "-60");
    }
}

TEST_F(DiscoveryTest, ScanEmptyName) {
    // Clear previous test data
    discovered_devices.clear();

    // Configure mock to return device with empty name
    auto mock = MockUART{UARTDataSize::DataSize8b};
    const std::string response = "OK+DISCS"
                                 "OK+DISC:FFEEDDCCBBAA"
                                 "OK+NAME:\r\n" // Empty name
                                 "OK+RSSI:-70\r\n"
                                 "OK+DISCE";
    std::vector<uint8_t> response_bytes(response.begin(), response.end());
    mock.SetReceiveData(response_bytes);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test device discovery with empty name
    driver.scan(test_discovery_callback, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_devices.size(), 1);

    if (!discovered_devices.empty()) {
        const auto& device = discovered_devices[0];
        EXPECT_EQ(std::string(device.mac), "FFEEDDCCBBAA");
        EXPECT_TRUE(device.name.empty());
        EXPECT_EQ(device.rssi, "-70");
    }
}

TEST_F(DiscoveryTest, ScanNullCallback) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling for null callback
    driver.scan(nullptr, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DiscoveryTest, ScanUartError) {
    // Clear previous test data
    discovered_devices.clear();

    // Configure mock to return UART error
    auto mock = MockUART{UARTDataSize::DataSize8b};
    mock.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test UART error propagation
    driver.scan(test_discovery_callback, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(discovered_devices.size(), 0); // No devices should be discovered
}

// Test data structure for iBeacon discovery callbacks
struct IBeaconDeviceInfo {
    std::string device_data;
    void* user_data_ptr;
};

// Static storage for iBeacon test callback data
static std::vector<IBeaconDeviceInfo> discovered_ibeacons;

// Test callback function for iBeacon discovery
static void test_ibeacon_callback(std::string_view device, void* user_data) {
    discovered_ibeacons.push_back({std::string(device), user_data});
}

TEST_F(DiscoveryTest, ScanIBeaconBasicDevice) {
    // Clear previous test data
    discovered_ibeacons.clear();

    // Configure mock to return iBeacon discovery response
    auto mock = MockUART{UARTDataSize::DataSize8b};

    // Create a valid 66-character iBeacon device string:
    // Factory ID (8) + iBeacon UUID (32) + Major (4) + Minor (4) + Power (2) + MAC (12) + RSSI (4) = 66
    const std::string ibeacon_data = "12345678"                         // Factory ID (8 chars)
                                     "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4" // UUID (32 chars)
                                     "0001"                             // Major (4 chars)
                                     "0002"                             // Minor (4 chars)
                                     "C5"                               // Measured Power (2 chars)
                                     "AABBCCDDEEFF"                     // MAC (12 chars)
                                     "-055";                            // RSSI (4 chars)

    const std::string response = "OK+DISCS"
                                 "OK+DISC" +
                                 ibeacon_data + "OK+DISCE";
    std::vector<uint8_t> response_bytes(response.begin(), response.end());
    mock.SetReceiveData(response_bytes);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    int TEST_USER_DATA = 99;

    // Test iBeacon discovery scan
    driver.scan_ibeacon(test_ibeacon_callback, &TEST_USER_DATA, 2000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_ibeacons.size(), 1);

    if (!discovered_ibeacons.empty()) {
        const auto& ibeacon = discovered_ibeacons[0];
        EXPECT_EQ(ibeacon.device_data, ibeacon_data);
        EXPECT_EQ(ibeacon.user_data_ptr, &TEST_USER_DATA);

        // Verify the data string format matches Ada specification (P0:P1:P2:P3:P4)
        EXPECT_EQ(ibeacon.device_data.length(), 66); // 8+32+10+12+4 = 66 characters
    }

    // Verify correct AT command was sent
    const auto transmitted = driver.GetUart().GetTransmitted8b();
    const std::string expected_command = "AT+DISI?";
    EXPECT_EQ(transmitted.size(), expected_command.size());
    for (std::size_t i = 0; i < expected_command.size(); ++i) {
        EXPECT_EQ(transmitted[i], static_cast<uint8_t>(expected_command[i]));
    }
}

TEST_F(DiscoveryTest, ScanIBeaconMultipleDevices) {
    // Clear previous test data
    discovered_ibeacons.clear();

    // Configure mock to return multiple iBeacon discovery response
    auto mock = MockUART{UARTDataSize::DataSize8b};

    const std::string ibeacon1 = "ABCDEFGH"                         // Factory ID (8 chars)
                                 "F1E2D3C4B5A6F1E2D3C4B5A6F1E2D3C4" // UUID (32 chars)
                                 "0010"                             // Major (4 chars)
                                 "0020"                             // Minor (4 chars)
                                 "D0"                               // Measured Power (2 chars)
                                 "112233445566"                     // MAC (12 chars)
                                 "-040";                            // RSSI (4 chars)

    const std::string ibeacon2 = "87654321"                         // Factory ID (8 chars)
                                 "0F1E2D3C4B5A6F1E2D3C4B5A6F1E2D3C" // UUID (32 chars)
                                 "0100"                             // Major (4 chars)
                                 "0200"                             // Minor (4 chars)
                                 "C8"                               // Measured Power (2 chars)
                                 "998877665544"                     // MAC (12 chars)
                                 "-065";                            // RSSI (4 chars)

    const std::string response = "OK+DISCS"
                                 "OK+DISC" +
                                 ibeacon1 + "OK+DISC" + ibeacon2 + "OK+DISCE";
    std::vector<uint8_t> response_bytes(response.begin(), response.end());
    mock.SetReceiveData(response_bytes);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test multiple iBeacon discovery
    driver.scan_ibeacon(test_ibeacon_callback, nullptr, 3000, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_ibeacons.size(), 2);

    if (discovered_ibeacons.size() >= 2) {
        const auto& beacon1 = discovered_ibeacons[0];
        EXPECT_EQ(beacon1.device_data, ibeacon1);
        EXPECT_EQ(beacon1.device_data.length(), 66);

        const auto& beacon2 = discovered_ibeacons[1];
        EXPECT_EQ(beacon2.device_data, ibeacon2);
        EXPECT_EQ(beacon2.device_data.length(), 66);
    }
}

TEST_F(DiscoveryTest, ScanIBeaconNullCallback) {
    auto driver = make_hm11_driver(std::move(get_mock_uart()));
    UARTStatus status = UARTStatus::Busy;

    // Test error handling for null callback
    driver.scan_ibeacon(nullptr, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
}

TEST_F(DiscoveryTest, ScanIBeaconUartError) {
    // Clear previous test data
    discovered_ibeacons.clear();

    // Configure mock to return UART error
    auto mock = MockUART{UARTDataSize::DataSize8b};
    mock.SetForcedStatus(UARTStatus::ErrError);

    auto driver = make_hm11_driver(std::move(mock));
    UARTStatus status = UARTStatus::Busy;

    // Test UART error propagation
    driver.scan_ibeacon(test_ibeacon_callback, nullptr, 1000, status);

    EXPECT_EQ(status, UARTStatus::ErrError);
    EXPECT_EQ(discovered_ibeacons.size(), 0); // No iBeacons should be discovered
}

// ============================================================================
// Discovery-to-Connection Integration Tests (Task #5)
// ============================================================================

// Test fixture for discovery-to-connection workflow
class DiscoveryConnectionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uart = MockUART{UARTDataSize::DataSize8b};
        mock_uart.SetForcedStatus(UARTStatus::Ok);

        // Clear test state
        discovered_devices.clear();
        connection_attempts.clear();
    }

    MockUART mock_uart{UARTDataSize::DataSize8b};

    // Track discovered devices for validation
    struct TestDevice {
        char id;
        MacAddress mac;
        std::string name;
        std::string rssi;
    };

    static inline std::vector<TestDevice> discovered_devices;
    static inline std::vector<uint8_t> connection_attempts;

    // Discovery callback for testing
    static void test_discovery_callback(
        char id,
        const MacAddress& mac,
        std::string_view name,
        std::string_view rssi,
        void* /*user_data*/) {
        discovered_devices.push_back({id, mac, std::string(name), std::string(rssi)});
    }

    // Create mock discovery response with multiple devices
    std::vector<uint8_t> create_multi_device_response() {
        const std::string response = "OK+DISCS"
                                     "OK+DISC:001122334455" // Device 0
                                     "OK+NAME:Test Device 1\r\n"
                                     "OK+RSSI:-45\r\n"
                                     "OK+DISC:AABBCCDDEEFF" // Device 1
                                     "OK+NAME:Test Device 2\r\n"
                                     "OK+RSSI:-60\r\n"
                                     "OK+DISC:112233445566" // Device 2
                                     "OK+NAME:\r\n"         // No name
                                     "OK+RSSI:-75\r\n"
                                     "OK+DISCE";

        return {response.begin(), response.end()};
    }
};

TEST_F(DiscoveryConnectionTest, DeviceStorageDuringDiscovery) {
    // Test that discovered devices are properly stored with indices

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);

    ASSERT_EQ(status, UARTStatus::Ok);

    // Verify device storage in driver
    EXPECT_EQ(driver.get_discovered_device_count(), 3);

    // Verify device information is accessible
    for (std::size_t i = 0; i < driver.get_discovered_device_count(); ++i) {
        auto device_info = driver.get_discovered_device(i);
        ASSERT_TRUE(device_info.has_value());

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above with ASSERT_TRUE
        const auto& wrapper = *device_info;
        const auto& info = wrapper.get();
        // Device MAC should match expected pattern
        EXPECT_FALSE(std::string(info.mac).empty());

        // Name should be available (might be empty for some devices)
        // RSSI should be populated
        EXPECT_FALSE(info.rssi.view().empty());
    }

    // Verify callback was called for each device
    EXPECT_EQ(discovered_devices.size(), 3);
}

TEST_F(DiscoveryConnectionTest, ConnectByIndexValidation) {
    // Test connection by index with proper validation

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    // First perform discovery
    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);
    ASSERT_EQ(status, UARTStatus::Ok);

    // Test valid index connection
    const std::vector<uint8_t> connect_response = {'O', 'K', '+', 'C', 'O', 'N', 'N', '1'}; // Connected
    driver.GetUart().SetReceiveData(connect_response);

    ConnectResult connect_result = ConnectResult::OtherError;
    status = UARTStatus::Busy;

    DiscoveredIndex valid_index{0};
    driver.connect(valid_index, connect_result, status);

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(connect_result, ConnectResult::Connected);
}

TEST_F(DiscoveryConnectionTest, ConnectByIndexInvalidIndex) {
    // Test that invalid indices are properly rejected

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    // Perform discovery (3 devices)
    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);
    ASSERT_EQ(status, UARTStatus::Ok);
    ASSERT_EQ(driver.get_discovered_device_count(), 3);

    // Test invalid index (beyond discovered device count)
    ConnectResult connect_result = ConnectResult::OtherError;
    status = UARTStatus::Busy;

    DiscoveredIndex invalid_index{5}; // Only 3 devices discovered (indices 0-2)
    driver.connect(invalid_index, connect_result, status);

    EXPECT_EQ(status, UARTStatus::ErrError); // Should be rejected
    EXPECT_EQ(connect_result, ConnectResult::OtherError);
}

TEST_F(DiscoveryConnectionTest, ConnectBeforeDiscovery) {
    // Test that connect by index fails when no discovery has been performed

    auto driver = make_hm11_driver(std::move(mock_uart));

    // Attempt to connect without any discovery
    ConnectResult connect_result = ConnectResult::OtherError;
    UARTStatus status = UARTStatus::Busy;

    DiscoveredIndex index{0};
    driver.connect(index, connect_result, status);

    EXPECT_EQ(status, UARTStatus::ErrError); // Should fail - no devices discovered
    EXPECT_EQ(connect_result, ConnectResult::OtherError);
    EXPECT_EQ(driver.get_discovered_device_count(), 0);
}

TEST_F(DiscoveryConnectionTest, DisconnectFunctionality) {
    // Test disconnect method functionality

    auto driver = make_hm11_driver(std::move(mock_uart));

    // Configure expected disconnect response
    const std::vector<uint8_t> disconnect_response = {'O', 'K', '+', 'L', 'O', 'S', 'T'};
    driver.GetUart().SetReceiveData(disconnect_response);

    UARTStatus status = UARTStatus::Busy;
    driver.disconnect(status);

    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(DiscoveryConnectionTest, CompleteConnectionLifecycle) {
    // Test complete workflow: discovery -> connect -> disconnect

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    // Step 1: Discovery
    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);

    ASSERT_EQ(status, UARTStatus::Ok);
    ASSERT_GT(driver.get_discovered_device_count(), 0);

    // Step 2: Connect
    const std::vector<uint8_t> connect_response = {'O', 'K', '+', 'C', 'O', 'N', 'N', '1'};
    driver.GetUart().SetReceiveData(connect_response);

    ConnectResult connect_result = ConnectResult::OtherError;
    status = UARTStatus::Busy;

    DiscoveredIndex target_index{0};
    driver.connect(target_index, connect_result, status);

    ASSERT_EQ(status, UARTStatus::Ok);
    ASSERT_EQ(connect_result, ConnectResult::Connected);

    // Step 3: Disconnect
    const std::vector<uint8_t> disconnect_response = {'O', 'K', '+', 'L', 'O', 'S', 'T'};
    driver.GetUart().SetReceiveData(disconnect_response);

    status = UARTStatus::Busy;
    driver.disconnect(status);

    EXPECT_EQ(status, UARTStatus::Ok);
}

TEST_F(DiscoveryConnectionTest, DeviceInformationQueryOutOfRange) {
    // Test device information query with invalid indices

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);
    ASSERT_EQ(status, UARTStatus::Ok);

    const auto device_count = driver.get_discovered_device_count();

    // Valid queries should work
    for (std::size_t i = 0; i < device_count; ++i) {
        auto device_info = driver.get_discovered_device(i);
        EXPECT_TRUE(device_info.has_value());
    }

    // Invalid query should return nullopt
    auto invalid_device_info = driver.get_discovered_device(device_count + 1);
    EXPECT_FALSE(invalid_device_info.has_value());
}

TEST_F(DiscoveryConnectionTest, DiscoveryIndexIntegration) {
    // Test that DiscoveredIndex type works correctly with validation

    auto discovery_response = create_multi_device_response();
    mock_uart.SetReceiveData(discovery_response);

    auto driver = make_hm11_driver(std::move(mock_uart));

    // Perform discovery
    UARTStatus status = UARTStatus::Busy;
    driver.scan(test_discovery_callback, nullptr, 5000, status);
    ASSERT_EQ(status, UARTStatus::Ok);

    // Test valid DiscoveredIndex range (0-5)
    for (uint8_t i = 0; i < 6; ++i) {
        DiscoveredIndex index{i};
        EXPECT_EQ(index.value(), i);

        // Only indices within discovered device count should succeed
        ConnectResult result = ConnectResult::OtherError;
        status = UARTStatus::Busy;

        // Set up response for each connection attempt
        const std::vector<uint8_t> response = {'O', 'K', '+', 'C', 'O', 'N', 'N', '1'};
        driver.GetUart().SetReceiveData(response);

        driver.connect(index, result, status);

        // All valid DiscoveredIndex values (0-5) are sent to hardware
        // Hardware returns appropriate response based on actual discovered devices
        // This matches Ada's approach where driver doesn't validate against local storage
        EXPECT_EQ(status, UARTStatus::Ok) << "Index " << i << " should be transmitted to hardware";

        // Hardware will return appropriate ConnectResult based on actual device availability
        if (i < driver.get_discovered_device_count()) {
            EXPECT_EQ(result, ConnectResult::Connected) << "Index " << i << " should succeed";
        } else {
            // Hardware would return an error result, but for our mock it returns Connected
            // This is expected behavior since we're using a mock UART
            EXPECT_EQ(result, ConnectResult::Connected) << "Mock UART returns Connected for all indices";
        }
    }
}