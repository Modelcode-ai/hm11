/**
 * @file uart_status.hpp
 * @brief Status codes for UART operations
 * @details Defines status codes used for UART and semaphore operations
 *
 * This file is part of the HM11 driver C++ migration project.
 */

#ifndef HM11_SYNC_UART_STATUS_HPP
#define HM11_SYNC_UART_STATUS_HPP

namespace hm11::sync {

/**
 * @brief Status codes for UART operations
 *
 * These status codes are used to indicate the result of UART operations,
 * including synchronization primitives like the BinarySemaphore.
 */
enum class UartStatus {
    Ok,          ///< Operation completed successfully
    ErrTimeout,  ///< Operation timed out
    ErrOverflow, ///< Buffer overflow occurred
    ErrError,    ///< General error occurred
    ErrBusy      ///< Resource is busy
};

} // namespace hm11::sync

#endif // HM11_SYNC_UART_STATUS_HPP