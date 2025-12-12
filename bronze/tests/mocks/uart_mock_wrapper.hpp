#ifndef HM11_TESTS_MOCKS_UART_MOCK_WRAPPER_HPP
#define HM11_TESTS_MOCKS_UART_MOCK_WRAPPER_HPP

#include <memory>
#include "hm11/hal/uart.hpp"
#include "uart_mock.hpp"
#include "hm11_driver_mock.hpp"

namespace hm11::tests::mocks {

/**
 * @brief A wrapper class for shared_ptr<EnhancedUARTMock> that satisfies UART_Implementation
 * 
 * This class forwards all operations to the underlying shared_ptr, ensuring that
 * it properly satisfies the UART_Implementation concept requirements.
 */
class EnhancedUARTMockWrapper {
public:
    /**
     * @brief Constructor
     * @param mock Pointer to the mock object
     */
    explicit EnhancedUARTMockWrapper(std::shared_ptr<EnhancedUARTMock> mock)
        : mock_(std::move(mock)) {
    }
    
    /**
     * @brief Get the data size
     * @return The data size
     */
    hal::UartDataSize data_size() const {
        return mock_->data_size();
    }
    
    /**
     * @brief Transmit 8-bit data
     * @param data The data to transmit
     * @param status The status of the operation
     * @param timeout The timeout in milliseconds
     */
    void transmit(
        const hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        mock_->transmit(data, status, timeout);
    }
    
    /**
     * @brief Transmit 9-bit data
     * @param data The data to transmit
     * @param status The status of the operation
     * @param timeout The timeout in milliseconds
     */
    void transmit(
        const hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        mock_->transmit(data, status, timeout);
    }
    
    /**
     * @brief Receive 8-bit data
     * @param data The data to receive
     * @param status The status of the operation
     * @param timeout The timeout in milliseconds
     */
    void receive(
        hal::UartData8b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        mock_->receive(data, status, timeout);
    }
    
    /**
     * @brief Receive 9-bit data
     * @param data The data to receive
     * @param status The status of the operation
     * @param timeout The timeout in milliseconds
     */
    void receive(
        hal::UartData9b& data,
        hal::UartStatus& status,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        mock_->receive(data, status, timeout);
    }
    
    /**
     * @brief Get the underlying mock object
     * @return The mock object
     */
    std::shared_ptr<EnhancedUARTMock> mock() const {
        return mock_;
    }
    
private:
    std::shared_ptr<EnhancedUARTMock> mock_;
};

} // namespace hm11::tests::mocks

#endif // HM11_TESTS_MOCKS_UART_MOCK_WRAPPER_HPP