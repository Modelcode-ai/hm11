/**
 * @file hm11_driver_mock.cpp
 * @brief Implementation of static members for HM11Driver mock classes
 *
 * @copyright Copyright (C) 2025
 */

#include "mocks/hm11_driver_mock.hpp"

namespace hm11::tests::mocks {

// Initialize static members
MockReceiveHandler* MockReceiveHandler::instance = nullptr;
MockLastReadPositionHandler* MockLastReadPositionHandler::instance = nullptr;

} // namespace hm11::tests::mocks