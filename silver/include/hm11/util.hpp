#pragma once

// HM11 Driver Utility Library
// Umbrella header for all utility functions

#include "util/parsing.hpp"
#include "util/string_builder.hpp"

/// @brief Utilities for HM11 driver implementation
///
/// This namespace contains:
/// - String construction utilities (string_builder.hpp)
/// - Zero-copy parsing utilities (parsing.hpp)
///
/// These utilities enable efficient AT command construction and response parsing
/// without heap allocation, matching Ada's stack-based approach while providing
/// modern C++ convenience and safety.
namespace hm11::util {
// All functionality is included via the headers above
}