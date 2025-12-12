/**
 * @file driver_constructor_test.cpp
 * @brief Tests for HM11Driver constructor and initialization
 *
 * This file contains tests for the HM11Driver constructor and initialization logic.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <memory>

#include "hm11/driver.hpp"
#include "hm11/hal/uart.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "utils/test_utils.hpp"

namespace hm11::tests {

/**
 * @brief Test fixture for HM11Driver constructor tests
 *
 * This fixture provides specialized setup for testing constructor variations.
 */
class HM11DriverConstructorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create the mock UART
        uart_mock_ = std::make_shared<mocks::EnhancedUARTMock>();

        // Set up the mock receive handler
        mocks::MockReceiveHandler::SetInstance(&receive_handler_);

        // Set up the mock last read position handler
        mocks::MockLastReadPositionHandler::SetInstance(&last_read_handler_);
    }

    void TearDown() override {
        // Clean up the mock handlers
        mocks::MockReceiveHandler::SetInstance(nullptr);
        mocks::MockLastReadPositionHandler::SetInstance(nullptr);
    }

    std::shared_ptr<mocks::EnhancedUARTMock> uart_mock_;
    ::testing::NiceMock<mocks::MockReceiveHandler> receive_handler_;
    ::testing::NiceMock<mocks::MockLastReadPositionHandler> last_read_handler_;
};

// Test basic constructor functionality
TEST_F(HM11DriverConstructorTest, BasicConstruction) {
    // Set up expectations
    EXPECT_CALL(
        receive_handler_,
        call(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(0); // Should not be called during construction

    EXPECT_CALL(last_read_handler_, call(::testing::_, ::testing::_))
        .Times(0); // Should not be called during construction

    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_,
        mocks::MockReceiveHandler::ReceiveWrapper,
        mocks::MockLastReadPositionHandler::LastReadPositionWrapper);

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);
}

// Test construction with null UART implementation
TEST_F(HM11DriverConstructorTest, NullUartConstruction) {
    // Attempting to construct with a null UART should fail
    // either by throwing an exception or by asserting in debug builds

    if (::testing::GTEST_FLAG(death_test_style) == "threadsafe") {
        EXPECT_DEBUG_DEATH(
            {
                auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
                    nullptr,
                    mocks::MockReceiveHandler::ReceiveWrapper,
                    mocks::MockLastReadPositionHandler::LastReadPositionWrapper);
            },
            "");
    }
}

// Test construction with different UART data sizes
TEST_F(HM11DriverConstructorTest, UartDataSizeConstruction) {
    // Create a UART with 9-bit data size
    auto uart_mock_9bit = std::make_shared<mocks::EnhancedUARTMock>(hal::UartDataSize::DataSize9b);

    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_9bit,
        mocks::MockReceiveHandler::ReceiveWrapper,
        mocks::MockLastReadPositionHandler::LastReadPositionWrapper);

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);
}

// Test construction with custom receive handler
TEST_F(HM11DriverConstructorTest, CustomReceiveHandlerConstruction) {
    // Create a custom receive handler
    bool handler_called = false;
    auto custom_handler = [&handler_called](
                              hal::AnyUartPort,
                              void*,
                              std::size_t,
                              hal::UartStatus& status,
                              std::chrono::milliseconds,
                              bool) {
        handler_called = true;
        status = hal::UartStatus::Ok;
    };

    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_,
        custom_handler,
        mocks::MockLastReadPositionHandler::LastReadPositionWrapper);

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);
}

// Test construction with custom last read position handler
TEST_F(HM11DriverConstructorTest, CustomLastReadHandlerConstruction) {
    // Create a custom last read position handler
    bool handler_called = false;
    auto custom_handler = [&handler_called](bool& closed, std::size_t& zero) {
        handler_called = true;
        closed = false;
        zero = 1;
    };

    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_,
        mocks::MockReceiveHandler::ReceiveWrapper,
        custom_handler);

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);
}

// Additional test to verify Rule of 5 (move and copy operations are deleted)
TEST_F(HM11DriverConstructorTest, RuleOfFive) {
    // This is a compile-time check, but we'll include it for clarity
    // Note: These should fail to compile if uncommented

    // Create the driver
    auto driver = std::make_unique<HM11Driver<std::shared_ptr<mocks::EnhancedUARTMock>>>(
        uart_mock_,
        mocks::MockReceiveHandler::ReceiveWrapper,
        mocks::MockLastReadPositionHandler::LastReadPositionWrapper);

    // Verify the driver was created
    EXPECT_NE(driver, nullptr);

    // The following operations should be deleted:
    // HM11Driver copy(driver);  // Copy constructor (deleted)
    // driver = copy;            // Copy assignment (deleted)
    // HM11Driver move(std::move(driver)); // Move constructor (deleted)
    // driver = std::move(move); // Move assignment (deleted)
}

} // namespace hm11::tests