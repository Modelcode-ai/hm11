#pragma once

#include "hm11/driver.hpp"

namespace hm11 {

/**
 * @brief Run a simplified HM11 example.
 *
 * This function mirrors the Ada HM11_Example program but uses a console
 * interface for portability. It demonstrates driver initialization, basic
 * configuration calls, and an echo loop using standard input/output.
 *
 * @param driver Initialized HM11Driver instance.
 */
void run_hm11_example(HM11Driver& driver);

} // namespace hm11
