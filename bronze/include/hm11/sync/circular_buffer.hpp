#ifndef HM11_SYNC_CIRCULAR_BUFFER_HPP
#define HM11_SYNC_CIRCULAR_BUFFER_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace hm11::sync {

/// Lock-free UART stream buffer for ISR-to-thread communication.
///
/// This stream buffer directly implements Ada's atomic Position pattern (drivers.adb:40, 294-303)
/// for streaming UART data from ISR context to application thread. The ISR (producer) writes
/// data and atomically updates the write position. The application (consumer) can either:
/// 1. Use data() + size() for Ada-style direct buffer access, or
/// 2. Use read() for incremental byte-by-byte parsing (C++ convenience)
///
/// ## Memory Ordering (Decision 23)
/// - Producer (ISR) uses memory_order_release when updating write_position
/// - Consumer (thread) uses memory_order_acquire when reading write_position
/// - This establishes happens-before relationship: all buffer writes before position update
///   are visible when consumer observes the new position
///
/// ## DMA Alignment (Decision 20)
/// - Buffer uses alignas(32) for cache line alignment on Cortex-M7 (STM32F7/H7)
/// - Prevents buffer from spanning cache lines, enabling efficient cache invalidation
/// - Harmless on Cortex-M4 (STM32F4 without data cache) but maintains consistency
///
/// ## Buffer Full Policy (Matches Ada Exactly)
/// When position reaches Capacity, it wraps to 0 and continues writing, overwriting old data.
/// This matches Ada's behavior (drivers.adb:298-301) where Position wraps from Maximum to 1.
/// The application is responsible for reading fast enough to avoid data loss.
///
/// ## Ada Source Translation
/// @code{.ada}
/// Position : Positive := 1 with Volatile, Atomic;
/// Memory_Block : array (1 .. Maximum) of UInt8;
///
/// -- ISR writes (drivers.adb:294-303):
/// Memory_Block(Position) := received_byte;
/// if Position < Maximum then
///    Position := Position + 1;
/// else
///    Position := 1;
/// end if;
/// Memory_Block(Position) := 0;  -- Clear next position
/// @endcode
///
/// ## Thread Safety
/// Safe for exactly ONE producer (ISR) and ONE consumer (application thread).
/// Producer only modifies write_position, consumer never modifies it (only reads).
/// Not safe for multiple producers or multiple consumers.
///
/// @tparam Capacity Fixed buffer size (must be power of 2 for efficient modulo via masking)
template <size_t Capacity> class CircularBuffer {
  public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2 for efficient wraparound");
    static_assert(Capacity > 0, "Capacity must be greater than zero");

    /// Constructs an empty circular buffer
    constexpr CircularBuffer() noexcept = default;

    // Delete copy and move operations (Decision 22: RAII unique ownership)
    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    CircularBuffer(CircularBuffer&&) = delete;
    CircularBuffer& operator=(CircularBuffer&&) = delete;
    ~CircularBuffer() = default;

    /// Writes a single byte to the buffer (producer/ISR operation).
    ///
    /// Directly translates Ada's ISR receive pattern (drivers.adb:294-303).
    /// Always succeeds by wrapping position and overwriting old data if buffer full.
    /// Safe to call from ISR context (no locks, no allocations, single atomic store).
    ///
    /// Memory ordering: Uses memory_order_release when updating write_position to ensure
    /// all buffer writes are visible to consumer before position update.
    ///
    /// @param data Byte to write
    /// @note Always succeeds (void return, like Ada procedure)
    /// @note May overwrite old data if consumer doesn't read fast enough
    /// @note Producer NEVER touches read_position (only consumer does)
    constexpr void write(uint8_t data) noexcept {
        // Read current write position
        const size_t CURRENT_POS = write_position.load(std::memory_order_relaxed);

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Write data to current position (matches Ada: Memory_Block(Position) := byte)
        buffer[CURRENT_POS] = data;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

        // Calculate next position with wraparound (matches Ada: Position + 1 or wrap to 1)
        const size_t NEXT_POS = (CURRENT_POS + 1) & (Capacity - 1);

        // Update write position with release semantics (Decision 23)
        // Ensures all buffer writes are visible before position update
        write_position.store(NEXT_POS, std::memory_order_release);

        // Clear next position (matches Ada: Memory_Block(Position) := 0)
        // This helps detect end of valid data in some parsing scenarios
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer[NEXT_POS] = 0;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /// Reads a single byte from the buffer (consumer/thread operation).
    ///
    /// Convenience wrapper that provides incremental byte-by-byte reading.
    /// Internally tracks read_position to support incremental parsing.
    ///
    /// Note: This is a C++ convenience method not present in Ada. Ada applications
    /// access buffer via data() + size(). This method is provided for ease of use.
    ///
    /// @param[out] data Output parameter for read byte
    /// @return true if read succeeded, false if no new data available
    [[nodiscard]] constexpr bool read(uint8_t& data) noexcept {
        // Read write position with acquire semantics
        const size_t CURRENT_WRITE = write_position.load(std::memory_order_acquire);

        if (read_position == CURRENT_WRITE) {
            // No new data
            return false;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        data = buffer[read_position];
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

        // Advance read position (only consumer modifies this, no synchronization needed)
        read_position = (read_position + 1) & (Capacity - 1);

        return true;
    }

    /// Peeks at the next byte without consuming it (consumer/thread operation).
    ///
    /// Convenience method for lookahead parsing. Not present in Ada.
    ///
    /// @param[out] data Output parameter for peeked byte
    /// @return true if peek succeeded, false if no data available
    [[nodiscard]] constexpr bool peek(uint8_t& data) const noexcept {
        const size_t CURRENT_WRITE = write_position.load(std::memory_order_acquire);

        if (read_position == CURRENT_WRITE) {
            return false;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        data = buffer[read_position];
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        return true;
    }

    /// Returns number of unread bytes (consumer/thread operation).
    ///
    /// This is for compatibility with read() API. Returns the number of bytes
    /// between read_position and write_position.
    ///
    /// @return Number of unread bytes (handles wraparound correctly)
    [[nodiscard]] constexpr size_t available() const noexcept {
        const size_t CURRENT_WRITE = write_position.load(std::memory_order_acquire);

        if (CURRENT_WRITE >= read_position) {
            return CURRENT_WRITE - read_position;
        }
        return (Capacity - read_position) + CURRENT_WRITE;
    }

    /// Returns whether there are no unread bytes (consumer/thread operation).
    [[nodiscard]] constexpr bool empty() const noexcept { return available() == 0; }

    /// Returns whether buffer is full (consumer/thread operation).
    [[nodiscard]] constexpr bool full() const noexcept {
        const size_t CURRENT_WRITE = write_position.load(std::memory_order_acquire);
        const size_t NEXT_WRITE = (CURRENT_WRITE + 1) & (Capacity - 1);
        return NEXT_WRITE == read_position;
    }

    /// Clears unread data by resetting read position to write position (consumer/thread operation).
    ///
    /// This is for compatibility with read() API. It marks all current data as consumed.
    constexpr void clear() noexcept {
        const size_t CURRENT_WRITE = write_position.load(std::memory_order_acquire);
        read_position = CURRENT_WRITE;
    }

    /// Returns current write position / total bytes written (consumer/thread operation).
    ///
    /// Matches Ada's Position variable access. Application uses this to determine how many
    /// bytes are in the buffer for Ada-style direct access via data() pointer.
    ///
    /// Note: If producer has written more than Capacity bytes, position will have wrapped.
    /// The value returned is the current position (0 to Capacity-1), not total bytes written.
    ///
    /// @return Current write position (0 to Capacity-1)
    [[nodiscard]] constexpr size_t size() const noexcept { return write_position.load(std::memory_order_acquire); }

    /// Returns pointer to buffer data for direct access (consumer/thread operation).
    ///
    /// Matches Ada's pattern where application accesses Memory_Block array directly:
    /// @code{.ada}
    /// for I in 1 .. Position loop
    ///    process(Memory_Block(I));
    /// end loop;
    /// @endcode
    ///
    /// C++ equivalent:
    /// @code
    /// size_t len = buffer.size();
    /// const uint8_t* bytes = buffer.data();
    /// for (size_t i = 0; i < len; ++i) {
    ///     process(bytes[i]);
    /// }
    /// @endcode
    ///
    /// @return Const pointer to buffer data
    /// @note Application must use size() to determine valid data length
    /// @note Pointer is valid for the lifetime of this buffer
    [[nodiscard]] constexpr const uint8_t* data() const noexcept { return buffer.data(); }

    /// Returns buffer capacity.
    [[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }

    /// Resets buffer to empty state (consumer/thread operation).
    ///
    /// Resets both write and read positions to 0. Use this between messages or
    /// when you want to completely clear the buffer.
    constexpr void reset() noexcept {
        write_position.store(0, std::memory_order_release);
        read_position = 0;
    }

    /// Returns current write position (for debugging/testing only).
    ///
    /// Matches Ada's Readed procedure (drivers.adb:179-185) that returns Position.
    /// This is an alias for size() provided for clarity in test code.
    [[nodiscard]] constexpr size_t get_write_position() const noexcept {
        return write_position.load(std::memory_order_acquire);
    }

    /// Returns current read position (for debugging/testing only).
    ///
    /// This is for the convenience read() API. Not applicable when using Ada-style
    /// data() + size() access.
    [[nodiscard]] constexpr size_t get_read_position() const noexcept { return read_position; }

  private:
    // Fixed-size buffer with 32-byte alignment for DMA and cache coherency (Decision 20)
    // On Cortex-M7 (STM32F7/H7), this prevents buffer from spanning cache lines
    // On Cortex-M4 (STM32F4), this is harmless but maintains consistency
    // Placed first to minimize padding overhead (aligns entire struct to 32 bytes)
    alignas(32) std::array<uint8_t, Capacity> buffer{};

    // Write position updated by producer (ISR) with atomic operations
    // Memory ordering: producer stores with release, consumer loads with acquire (Decision 23)
    // Corresponds directly to Ada's Position variable
    std::atomic<size_t> write_position{0};

    // Read position for convenience read() API (not in Ada, C++ addition for incremental parsing)
    // Updated only by consumer (thread), no synchronization needed
    // Not atomic because only consumer modifies it (producer NEVER touches this)
    size_t read_position{0};
};

} // namespace hm11::sync

#endif // HM11_SYNC_CIRCULAR_BUFFER_HPP
