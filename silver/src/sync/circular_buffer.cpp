// HM11 Synchronization Primitives - Circular Buffer Explicit Template Instantiations
// Provides explicit instantiations for common circular buffer types to reduce compile times
// and ensure consistent code generation across translation units

#include "hm11/sync/circular_buffer.hpp"

namespace hm11::sync {

// Explicit template instantiations for common buffer types
// This reduces compilation time and ensures consistent optimization across translation units

/// UART data buffer - 512 bytes, power of 2 for efficient wrapping
/// Used for general UART receive operations and AT command responses
template class CircularBuffer<std::uint8_t, 512>;

/// Discovery data buffer - 1024 bytes for streaming discovery responses
/// Handles variable-length device names, RSSI values, and MAC addresses during scanning
template class CircularBuffer<std::uint8_t, 1024>;

/// Command response buffer - 256 bytes for typical AT command responses
/// Sufficient for most HM11 command responses (OK, ERROR, configuration values)
template class CircularBuffer<std::uint8_t, 256>;

/// 16-bit data buffer for sensor readings or configuration values
/// Used when data elements are larger than single bytes
template class CircularBuffer<std::uint16_t, 256>;

/// 32-bit data buffer for high-level data structures or timestamps
/// Used for complex data types that require larger storage per element
template class CircularBuffer<std::uint32_t, 128>;

} // namespace hm11::sync