# HM11 Driver Test Infrastructure

This document describes the test infrastructure for the HM11 driver project, including how to create, run, and extend tests.

## Overview

The HM11 driver test infrastructure is built using Google Test, a C++ testing framework. It provides a framework for writing and running unit tests, as well as integration tests for the HM11 driver components.

The infrastructure includes:

- A CMake-based build system for tests
- Mock objects for hardware interfaces
- Utilities for common test operations
- Custom test assertions
- Test coverage reporting

## Project Structure

The test directory is structured to mirror the source code organization:

```
tests/
├── CMakeLists.txt         # Main CMake file for tests
├── README.md              # This documentation
├── example_test.cpp       # Example test demonstrating the infrastructure
├── driver/                # Tests for the HM11Driver class
│   ├── driver_test.cpp               # Main driver test file with fixtures
│   ├── driver_constructor_test.cpp   # Constructor tests
│   ├── driver_basic_ops_test.cpp     # Basic operation tests
│   ├── driver_getset_test.cpp        # Getter/setter tests
│   ├── at_command_test.cpp           # AT command tests
│   ├── response_parser_test.cpp      # Response parser tests
│   └── driver_mock_test.cpp          # Error handling tests
├── hal/                   # Tests for the HAL module
├── mocks/                 # Mock implementations of interfaces
│   ├── uart_mock.hpp      # Mock UART implementation
│   ├── uart_mock_test.cpp # Tests for the UART mock
│   ├── hm11_driver_mock.hpp  # HM11Driver mock extensions
│   └── hm11_driver_mock.cpp  # Static member implementations
├── platform/              # Tests for the platform module
├── sync/                  # Tests for the synchronization module
├── integration/           # Integration tests
│   └── driver_integration_test.cpp  # Integration workflows
├── test_main.cpp          # Main test entry point
└── utils/                 # Test utilities
    └── test_utils.hpp     # Common test utilities
```

Each module has its own subdirectory for tests, mirroring the source code organization.

## Building and Running Tests

Tests are built and run using CMake. The following commands are available:

```bash
# Configure with CMake
cmake -B build -S . -DBUILD_TESTS=ON

# Build all tests
cmake --build build --target all

# Run all tests
cmake --build build --target run_tests

# Run a specific test
cd build
./tests/example_test
```

You can also use CMake presets for a more convenient workflow:

```bash
# Configure with Clang debug preset (recommended for testing)
cmake --preset clang-debug

# Build all tests
cmake --build --preset clang-debug --parallel

# Run all tests
ctest --preset clang-debug
```

For advanced users, you can also run tests with specific options:

```bash
# Run tests with verbose output
./tests/example_test --gtest_output=verbose

# Run a specific test
./tests/example_test --gtest_filter=ExampleTest.UARTMockExample

# Run tests with break on failure
./tests/example_test --gtest_break_on_failure
```

## Code Coverage

Code coverage is automatically enabled when the `ENABLE_COVERAGE` option is enabled in CMake:

```bash
# Configure with code coverage
cmake -B build -S . -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON

# Build and run tests
cmake --build build --target all
cmake --build build --target run_tests

# Generate coverage report (GCC)
cd build
gcov ../src/*.cpp -o CMakeFiles/hm11_driver.dir/src
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# Open the coverage report
open coverage_report/index.html
```

For Clang, use the following commands:

```bash
# Generate coverage report (Clang)
cd build
llvm-profdata merge -sparse default.profraw -o default.profdata
llvm-cov show ./tests/example_test -instr-profile=default.profdata -format=html -output-dir=coverage_report ../src

# Open the coverage report
open coverage_report/index.html
```

## Creating Tests

### Basic Test Structure

All tests should follow this basic structure:

```cpp
#include <gtest/gtest.h>
#include "hm11/module/header.hpp"  // Header under test

namespace hm11::tests {

class MyTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize test environment
    }

    void TearDown() override {
        // Clean up test environment
    }
};

TEST_F(MyTest, TestName) {
    // Test code
    EXPECT_TRUE(true);
}

} // namespace hm11::tests
```

### Using the UART Mock

The UART mock (`UARTMock`) can be used to simulate UART hardware for testing. Here's an example:

```cpp
#include <gtest/gtest.h>
#include "hm11/hal/uart.hpp"
#include "mocks/uart_mock.hpp"

namespace hm11::tests {

TEST(UARTTest, BasicTest) {
    // Create a UART mock
    auto uart_mock = std::make_shared<mocks::UARTMock>();
    uart_mock->setup_default_behavior();

    // Queue data to be received
    uart_mock->queue_receive_data("OK");

    // Create buffers for transmitting and receiving
    std::vector<hal::UInt8> tx_buffer = {'A', 'T'};
    std::vector<hal::UInt8> rx_buffer(2);
    
    hal::UartData8b tx_data(tx_buffer);
    hal::UartData8b rx_data(rx_buffer);

    // Transmit the data
    hal::UartStatus tx_status;
    uart_mock->transmit(tx_data, tx_status, std::chrono::milliseconds(100));

    // Verify the transmit status and data
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, tx_status);

    // Receive the response
    hal::UartStatus rx_status;
    uart_mock->receive(rx_data, rx_status, std::chrono::milliseconds(100));

    // Verify the receive status and data
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, rx_status);
    EXPECT_EQ(std::string(rx_buffer.begin(), rx_buffer.end()), "OK");
}

} // namespace hm11::tests
```

### Using the Enhanced UART Mock

The `EnhancedUARTMock` class extends the basic UART mock with additional functionality for HM11 driver testing:

```cpp
#include <gtest/gtest.h>
#include "hm11/hal/uart.hpp"
#include "mocks/hm11_driver_mock.hpp"

namespace hm11::tests {

TEST(HM11DriverMockTest, BasicTest) {
    // Create an enhanced UART mock
    auto uart_mock = std::make_shared<mocks::EnhancedUARTMock>();

    // Add a command-response mapping
    uart_mock->add_command_response("AT", "OK");
    uart_mock->add_command_response("AT+ROLE?", "OK+Get:0");

    // Add a custom command handler
    uart_mock->add_command_handler("AT+ROLE", [](const std::string& command,
                                               std::string& response,
                                               hal::UartStatus& status) {
        // Custom handling logic
        response = "OK+Set:1";
        status = hal::UartStatus::Ok;
    });

    // Test transmission
    hal::UartStatus tx_status;
    std::vector<hal::UInt8> tx_buffer = {'A', 'T'};
    hal::UartData8b tx_data(tx_buffer);
    uart_mock->transmit(tx_data, tx_status, std::chrono::milliseconds(100));

    // Verify the transmit status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, tx_status);

    // Verify the response was queued
    hal::UartStatus rx_status;
    std::vector<hal::UInt8> rx_buffer(2);
    hal::UartData8b rx_data(rx_buffer);
    uart_mock->receive(rx_data, rx_status, std::chrono::milliseconds(100));
    EXPECT_EQ(std::string(rx_buffer.begin(), rx_buffer.end()), "OK");
}

} // namespace hm11::tests
```

### Using Test Utilities

The test utilities (`utils/test_utils.hpp`) provide common functions for testing:

```cpp
#include <gtest/gtest.h>
#include "utils/test_utils.hpp"

namespace hm11::tests {

TEST(UtilsTest, BasicTest) {
    // Generate random data
    auto random_data = utils::random_uint8_vector(10);
    auto random_str = utils::random_string(10);
    auto random_hex = utils::random_hex_string(10);

    // Convert between string and UInt8 vector
    auto str = utils::uint8_vector_to_string(random_data);
    auto vec = utils::string_to_uint8_vector(str);
    
    EXPECT_EQ(vec, random_data);

    // Execute with timeout
    bool result = utils::execute_with_timeout(
        []() { return true; },
        std::chrono::milliseconds(100)
    );
    
    EXPECT_TRUE(result);
}

} // namespace hm11::tests
```

### Custom Assertions

The test infrastructure provides custom assertions for common operations:

```cpp
#include <gtest/gtest.h>
#include "hm11/hal/uart.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

TEST(AssertionsTest, BasicTest) {
    hal::UartStatus status = hal::UartStatus::Ok;
    
    // Use custom assertion for UART status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    ASSERT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}

} // namespace hm11::tests
```

## HM11Driver Test Suite

The HM11Driver test suite includes several components:

### Test Fixtures

The main test fixture for HM11Driver tests is defined in `tests/driver/driver_test.cpp`:

```cpp
class HM11DriverTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create and configure the mock UART
        uart_mock_ = std::make_shared<mocks::EnhancedUARTMock>();
        
        // Set up the mock receive handler
        mocks::MockReceiveHandler::SetInstance(&receive_handler_);
        
        // Set up the mock last read position handler
        mocks::MockLastReadPositionHandler::SetInstance(&last_read_handler_);
        
        // Create the driver instance
        driver_ = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
            uart_mock_,
            mocks::MockReceiveHandler::ReceiveWrapper,
            mocks::MockLastReadPositionHandler::LastReadPositionWrapper
        );
    }

    std::shared_ptr<mocks::EnhancedUARTMock> uart_mock_;
    std::unique_ptr<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>> driver_;
    ::testing::NiceMock<mocks::MockReceiveHandler> receive_handler_;
    ::testing::NiceMock<mocks::MockLastReadPositionHandler> last_read_handler_;
};
```

### Constructor Tests

Tests for the HM11Driver constructor and initialization are in `tests/driver/driver_constructor_test.cpp`:

```cpp
TEST_F(HM11DriverConstructorTest, BasicConstruction) {
    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_,
        mocks::MockReceiveHandler::ReceiveWrapper,
        mocks::MockLastReadPositionHandler::LastReadPositionWrapper
    );

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);
}
```

### Basic Operation Tests

Tests for basic operations like test, reset, restart are in `tests/driver/driver_basic_ops_test.cpp`:

```cpp
TEST_F(HM11DriverBasicOpsTest, TestOperation) {
    // Configure the mock
    uart_mock_->add_command_response("AT", "OK");
    
    // Call the test method
    hal::UartStatus status;
    driver_->test(status);
    
    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}
```

### Getter/Setter Tests

Tests for getter and setter operations are in `tests/driver/driver_getset_test.cpp`:

```cpp
TEST_F(HM11DriverGetSetTest, RoleGetSet) {
    // Configure the mock
    uart_mock_->add_command_response("AT+ROLE?", "OK+Get:0");
    uart_mock_->add_command_response("AT+ROLE1", "OK+Set:1");
    
    // Get the current role
    hal::UartStatus status;
    Role role;
    driver_->get_role(role, status);
    
    // Verify the role
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    EXPECT_EQ(role, Role::Peripheral);
    
    // Set the role to Central
    driver_->set_role(Role::Central, status);
    
    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}
```

### AT Command and Response Parser Tests

Tests for AT command construction and response parsing are in `tests/driver/at_command_test.cpp` and `tests/driver/response_parser_test.cpp`:

```cpp
TEST_F(ATCommandTest, CommandWithParams) {
    const std::string command = util::build_command("AT+ROLE", "1");
    EXPECT_EQ(command, "AT+ROLE1");
}

TEST_F(ResponseParserTest, ParseInteger) {
    const std::string response = "OK+Get:42";
    int value = 0;
    bool success = util::parse_response(response, "OK+Get:", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, 42);
}
```

### Error Handling Tests

Tests for error handling are in `tests/driver/driver_mock_test.cpp`:

```cpp
TEST_F(HM11DriverMockTest, TransmitError) {
    // Configure the mock to return an error status
    uart_mock_->set_default_status(hal::UartStatus::Error);
    
    // Call the test method
    hal::UartStatus status;
    driver_->test(status);
    
    // Verify the status
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Error, status);
}
```

### Integration Tests

Integration tests that test complete workflows are in `tests/integration/driver_integration_test.cpp`:

```cpp
TEST_F(HM11DriverIntegrationTest, DeviceInitializationWorkflow) {
    // Configure the mock
    uart_mock_->add_command_response("AT", "OK");
    uart_mock_->add_command_response("AT+VERSION", "HMSoft V1.0");
    uart_mock_->add_command_response("AT+ROLE0", "OK+Set:0");
    
    // Execute the initialization workflow
    hal::UartStatus status;
    
    // Step 1: Test communication
    driver_->test(status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    
    // Step 2: Get software version
    util::VersionType version;
    driver_->software_version(version, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
    
    // Step 3: Set role to Peripheral
    driver_->set_role(Role::Peripheral, status);
    EXPECT_UART_STATUS_EQ(hal::UartStatus::Ok, status);
}
```

## Adding New Tests to CMake

To add a new test file to the build system, add it to the appropriate section in the relevant `CMakeLists.txt`:

```cmake
# Add a new test to the driver tests
add_hm11_test(new_driver_test
    driver/new_driver_test.cpp
)
```

For tests that involve multiple files, you can specify them as additional arguments:

```cmake
add_hm11_test(complex_test
    complex_test.cpp
    complex_test_helper.cpp
)
```

Don't forget to update the `run_tests` target in `tests/CMakeLists.txt` to include your new test:

```cmake
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS minimal_test example_test new_driver_test
    COMMENT "Running all tests"
    USES_TERMINAL
)
```

## Best Practices

1. **Test Independence**: Each test should be independent of other tests. Avoid dependencies between tests.

2. **Test Naming**: Use descriptive names for tests, following the pattern `TestSuite.TestName`.

3. **Mocks**: Use mocks for hardware interfaces to avoid dependencies on real hardware.

4. **Coverage**: Aim for high code coverage, especially for critical components.

5. **Error Cases**: Test error cases as well as success cases.

6. **Timeouts**: Use timeouts to prevent tests from hanging.

7. **Documentation**: Document complex tests to explain their purpose and behavior.

8. **CI/CD**: Run tests as part of the continuous integration pipeline.

## Troubleshooting

### Tests Not Running

- Ensure that Google Test is installed and correctly linked.
- Check that the test executable is built correctly.
- Verify that the test is registered with CTest.

### Tests Failing

- Check the test output for specific failure messages.
- Use `--gtest_output=verbose` for more detailed output.
- Try running the specific failing test with `--gtest_filter`.

### Coverage Not Working

- Ensure that coverage is enabled with `-DENABLE_COVERAGE=ON`.
- Verify that the correct coverage tools are installed (gcov/lcov for GCC, llvm-cov for Clang).
- Check that the source files are being compiled with coverage flags.

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [CMake Documentation](https://cmake.org/documentation/)
- [Code Coverage with gcov/lcov](https://gcovr.com/en/stable/guide.html)
- [Code Coverage with llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html)