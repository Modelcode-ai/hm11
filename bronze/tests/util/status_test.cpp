/**
 * @file status_test.cpp
 * @brief Tests for the status utility functions
 *
 * This file contains unit tests for the status utility functions defined in status.hpp.
 */

#include <gtest/gtest.h>

#include "hm11/hal/uart.hpp"
#include "hm11/util/status.hpp"

namespace hm11::util::test {

// Test the is_ok function for Status type
TEST(StatusTest, IsOkWithStatus) {
    EXPECT_TRUE(is_ok(Status::Ok));
    EXPECT_FALSE(is_ok(Status::Err_Error));
    EXPECT_FALSE(is_ok(Status::Err_Timeout));
    EXPECT_FALSE(is_ok(Status::Err_Busy));
    EXPECT_FALSE(is_ok(Status::Err_Invalid_Param));
    EXPECT_FALSE(is_ok(Status::Err_Invalid_State));
    EXPECT_FALSE(is_ok(Status::Err_Not_Connected));
    EXPECT_FALSE(is_ok(Status::Err_Not_Supported));
    EXPECT_FALSE(is_ok(Status::Err_Parse_Error));
    EXPECT_FALSE(is_ok(Status::Err_Buffer_Full));
}

// Test the is_ok function for UartStatus type
TEST(StatusTest, IsOkWithUartStatus) {
    EXPECT_TRUE(is_ok(hal::UartStatus::Ok));
    EXPECT_FALSE(is_ok(hal::UartStatus::ErrError));
    EXPECT_FALSE(is_ok(hal::UartStatus::ErrTimeout));
    EXPECT_FALSE(is_ok(hal::UartStatus::Busy));
}

// Test the is_error function for Status type
TEST(StatusTest, IsErrorWithStatus) {
    EXPECT_FALSE(is_error(Status::Ok));
    EXPECT_TRUE(is_error(Status::Err_Error));
    EXPECT_TRUE(is_error(Status::Err_Timeout));
    EXPECT_TRUE(is_error(Status::Err_Busy));
    EXPECT_TRUE(is_error(Status::Err_Invalid_Param));
    EXPECT_TRUE(is_error(Status::Err_Invalid_State));
    EXPECT_TRUE(is_error(Status::Err_Not_Connected));
    EXPECT_TRUE(is_error(Status::Err_Not_Supported));
    EXPECT_TRUE(is_error(Status::Err_Parse_Error));
    EXPECT_TRUE(is_error(Status::Err_Buffer_Full));
}

// Test the is_error function for UartStatus type
TEST(StatusTest, IsErrorWithUartStatus) {
    EXPECT_FALSE(is_error(hal::UartStatus::Ok));
    EXPECT_TRUE(is_error(hal::UartStatus::ErrError));
    EXPECT_TRUE(is_error(hal::UartStatus::ErrTimeout));
    EXPECT_TRUE(is_error(hal::UartStatus::Busy));
}

// Test the is_timeout function for Status type
TEST(StatusTest, IsTimeoutWithStatus) {
    EXPECT_FALSE(is_timeout(Status::Ok));
    EXPECT_FALSE(is_timeout(Status::Err_Error));
    EXPECT_TRUE(is_timeout(Status::Err_Timeout));
    EXPECT_FALSE(is_timeout(Status::Err_Busy));
    EXPECT_FALSE(is_timeout(Status::Err_Invalid_Param));
    EXPECT_FALSE(is_timeout(Status::Err_Invalid_State));
    EXPECT_FALSE(is_timeout(Status::Err_Not_Connected));
    EXPECT_FALSE(is_timeout(Status::Err_Not_Supported));
    EXPECT_FALSE(is_timeout(Status::Err_Parse_Error));
    EXPECT_FALSE(is_timeout(Status::Err_Buffer_Full));
}

// Test the is_timeout function for UartStatus type
TEST(StatusTest, IsTimeoutWithUartStatus) {
    EXPECT_FALSE(is_timeout(hal::UartStatus::Ok));
    EXPECT_FALSE(is_timeout(hal::UartStatus::ErrError));
    EXPECT_TRUE(is_timeout(hal::UartStatus::ErrTimeout));
    EXPECT_FALSE(is_timeout(hal::UartStatus::Busy));
}

// Test the is_busy function for Status type
TEST(StatusTest, IsBusyWithStatus) {
    EXPECT_FALSE(is_busy(Status::Ok));
    EXPECT_FALSE(is_busy(Status::Err_Error));
    EXPECT_FALSE(is_busy(Status::Err_Timeout));
    EXPECT_TRUE(is_busy(Status::Err_Busy));
    EXPECT_FALSE(is_busy(Status::Err_Invalid_Param));
    EXPECT_FALSE(is_busy(Status::Err_Invalid_State));
    EXPECT_FALSE(is_busy(Status::Err_Not_Connected));
    EXPECT_FALSE(is_busy(Status::Err_Not_Supported));
    EXPECT_FALSE(is_busy(Status::Err_Parse_Error));
    EXPECT_FALSE(is_busy(Status::Err_Buffer_Full));
}

// Test the is_busy function for UartStatus type
TEST(StatusTest, IsBusyWithUartStatus) {
    EXPECT_FALSE(is_busy(hal::UartStatus::Ok));
    EXPECT_FALSE(is_busy(hal::UartStatus::ErrError));
    EXPECT_FALSE(is_busy(hal::UartStatus::ErrTimeout));
    EXPECT_TRUE(is_busy(hal::UartStatus::Busy));
}

// Test the to_status function for converting UartStatus to Status
TEST(StatusTest, ToStatus) {
    EXPECT_EQ(to_status(hal::UartStatus::Ok), Status::Ok);
    EXPECT_EQ(to_status(hal::UartStatus::ErrError), Status::Err_Error);
    EXPECT_EQ(to_status(hal::UartStatus::ErrTimeout), Status::Err_Timeout);
    EXPECT_EQ(to_status(hal::UartStatus::Busy), Status::Err_Busy);
}

// Test the to_uart_status function for converting Status to UartStatus
TEST(StatusTest, ToUartStatus) {
    EXPECT_EQ(to_uart_status(Status::Ok), hal::UartStatus::Ok);
    EXPECT_EQ(to_uart_status(Status::Err_Error), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Timeout), hal::UartStatus::ErrTimeout);
    EXPECT_EQ(to_uart_status(Status::Err_Busy), hal::UartStatus::Busy);

    // Test that other error statuses map to ErrError
    EXPECT_EQ(to_uart_status(Status::Err_Invalid_Param), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Invalid_State), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Not_Connected), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Not_Supported), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Parse_Error), hal::UartStatus::ErrError);
    EXPECT_EQ(to_uart_status(Status::Err_Buffer_Full), hal::UartStatus::ErrError);
}

// Test the status_to_string function for Status type
TEST(StatusTest, StatusToString) {
    EXPECT_EQ(status_to_string(Status::Ok), "Ok");
    EXPECT_EQ(status_to_string(Status::Err_Error), "Error");
    EXPECT_EQ(status_to_string(Status::Err_Timeout), "Timeout");
    EXPECT_EQ(status_to_string(Status::Err_Busy), "Busy");
    EXPECT_EQ(status_to_string(Status::Err_Invalid_Param), "Invalid Parameter");
    EXPECT_EQ(status_to_string(Status::Err_Invalid_State), "Invalid State");
    EXPECT_EQ(status_to_string(Status::Err_Not_Connected), "Not Connected");
    EXPECT_EQ(status_to_string(Status::Err_Not_Supported), "Not Supported");
    EXPECT_EQ(status_to_string(Status::Err_Parse_Error), "Parse Error");
    EXPECT_EQ(status_to_string(Status::Err_Buffer_Full), "Buffer Full");
}

// Test the status_to_string function for UartStatus type
TEST(StatusTest, UartStatusToString) {
    EXPECT_EQ(status_to_string(hal::UartStatus::Ok), "Ok");
    EXPECT_EQ(status_to_string(hal::UartStatus::ErrError), "Error");
    EXPECT_EQ(status_to_string(hal::UartStatus::ErrTimeout), "Timeout");
    EXPECT_EQ(status_to_string(hal::UartStatus::Busy), "Busy");
}

} // namespace hm11::util::test