/**
 * @file hm11_driver_mock_explicit_instantiation.cpp
 * @brief Explicit instantiations of HM11Driver template for testing with mocks
 */

#include "hm11/driver.hpp"
#include "mocks/hm11_driver_mock.hpp"
#include "mocks/uart_mock.hpp"

// Include the implementation file
#include "../../src/hm11_driver.cpp"

// Explicit template instantiation for the tests
namespace hm11 {

// Instantiate all methods of HM11Driver with EnhancedUARTMock
template class HM11Driver<std::shared_ptr<hm11::tests::mocks::EnhancedUARTMock>>;

} // namespace hm11