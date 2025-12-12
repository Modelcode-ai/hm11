#pragma once

// HM11 Platform Abstraction Layer - Timing Utilities
// Platform-specific timing abstractions for timeout and deadline calculations
// Provides consistent timing interface across ARM and x86 platforms

#include <chrono>
#include <cstdint>

namespace hm11::platform {

/// High-resolution timing abstraction for timeout and deadline calculations
/// Uses steady clock to prevent issues with system time adjustments
/// Optimized for embedded systems with minimal overhead
namespace Timing {

/// Timestamp type for deadline calculations
/// Uses steady clock to be immune to system time changes
using Timestamp = std::chrono::steady_clock::time_point;

/// Duration type for timeout values
/// Standardizes on milliseconds for ISR timeout calculations
using Duration = std::chrono::milliseconds;

/// Get current timestamp
/// Uses monotonic clock that doesn't go backwards
/// Safe for deadline and timeout calculations
///
/// @return Current timestamp
inline Timestamp Now() noexcept {
    return std::chrono::steady_clock::now();
}

/// Calculate absolute deadline from current time and timeout
/// Used for ISR-side deadline polling (Decision 25)
/// Deadline = current_time + timeout_duration
///
/// @param timeout Timeout duration in milliseconds
/// @return Absolute deadline timestamp
inline Timestamp DeadlineAfter(Duration timeout) noexcept {
    return Now() + timeout;
}

/// Check if deadline has been exceeded
/// Used in ISR context to check for timeout expiration
/// Returns true if current time >= deadline
///
/// @param deadline Absolute deadline timestamp
/// @return true if deadline exceeded, false otherwise
inline bool IsDeadlineExceeded(Timestamp deadline) noexcept {
    return Now() >= deadline;
}

/// Calculate remaining time until deadline
/// Returns zero duration if deadline already exceeded
/// Used for platform wait operations with timeouts
///
/// @param deadline Absolute deadline timestamp
/// @return Remaining duration (0 if deadline exceeded)
inline Duration TimeUntilDeadline(Timestamp deadline) noexcept {
    const auto now = Now();
    if (now >= deadline) {
        return Duration::zero();
    }
    return std::chrono::duration_cast<Duration>(deadline - now);
}

/// Convert timeout to microseconds for platform-specific operations
/// Some platform operations need microsecond precision
///
/// @param timeout Duration in milliseconds
/// @return Duration in microseconds
inline std::chrono::microseconds ToMicroseconds(Duration timeout) noexcept {
    return std::chrono::duration_cast<std::chrono::microseconds>(timeout);
}

/// Convert timeout to nanoseconds for platform-specific operations
/// Used for high-precision platform timing operations
///
/// @param timeout Duration in milliseconds
/// @return Duration in nanoseconds
inline std::chrono::nanoseconds ToNanoseconds(Duration timeout) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(timeout);
}

/// Platform-specific timing constants
namespace Constants {
/// Typical interrupt latency on ARM Cortex-M (microseconds)
constexpr auto INTERRUPT_LATENCY = std::chrono::microseconds{10};

/// Typical context switch time on RTOS (microseconds)
constexpr auto CONTEXT_SWITCH_TIME = std::chrono::microseconds{50};

/// Minimum meaningful timeout (avoid spurious timeouts)
constexpr auto MINIMUM_TIMEOUT = std::chrono::milliseconds{1};

/// Default timeout for blocking operations
constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds{1000};

/// Maximum timeout to prevent overflow in calculations
constexpr auto MAXIMUM_TIMEOUT = std::chrono::milliseconds{30000};
} // namespace Constants

} // namespace Timing

} // namespace hm11::platform