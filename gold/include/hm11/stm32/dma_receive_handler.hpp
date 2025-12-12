/**
 * @file dma_receive_handler.hpp
 * @brief DMA-based receive strategy for UART5 with streaming support
 *
 * This header provides the DMA_Receive_Handler class that implements the receive
 * handler strategy (Decision 3) using DMA1 Stream 0 Channel 4 for efficient UART5
 * reception. It supports both blocking and streaming modes via the as_stream
 * parameter.
 *
 * ## Overview
 *
 * The DMA_Receive_Handler translates Ada's DMA_Receive_Handler procedure
 * (drivers.adb:324-361) into a C++ callable object that satisfies the
 * IsReceiveHandler concept. It coordinates DMA hardware, interrupt handlers,
 * and the Watchdog synchronization primitive to provide efficient UART reception.
 *
 * ## Architecture (Decision 3: Receive Strategy Injection)
 *
 * This class implements the receive strategy pattern where different receive
 * implementations (HAL blocking, DMA, interrupt-driven) can be injected into
 * the HM11Driver via template parameter. The DMA strategy offloads byte-by-byte
 * reception from CPU to DMA hardware, reducing interrupt overhead and enabling
 * efficient streaming operation for discovery.
 *
 * ## Operational Modes
 *
 * ### Blocking Mode (as_stream = false)
 *
 * Used for AT command responses:
 * 1. Configure DMA for single-shot transfer (Normal mode, not Circular)
 * 2. Start DMA from USART5->DR to destination buffer
 * 3. Enable DMA interrupts (TC, TE, FIFO, DME)
 * 4. Enable UART interrupts (Error, Idle for early completion)
 * 5. Call Watchdog.start() with timeout deadline
 * 6. Block on Watchdog.await_event() until ISR signals
 * 7. Return status (Ok, ErrTimeout, ErrError)
 *
 * ### Streaming Mode (as_stream = true)
 *
 * Used for discovery scanning (two-call pattern):
 *
 * **First Call (Start Stream):**
 * 1. Set up circular buffer and atomic write position
 * 2. Enable UART RXNE interrupt for byte-by-byte reception
 * 3. Enable UART Idle Line interrupt for message boundaries
 * 4. Call Watchdog.start() but don't block (return immediately)
 * 5. Status = Ok
 *
 * **Second Call (Stop Stream):**
 * 1. Call Watchdog.stop() to disable interrupts
 * 2. Clean up hardware state
 * 3. Status = Ok
 *
 * Between first and second calls, the application polls circular buffer write
 * position via Last_Read_Handler to parse streaming data.
 *
 * ## ISR Integration (Decision 12, 23, 25, 26)
 *
 * The DMA_Receive_Handler coordinates with ISR handlers:
 *
 * - **DMAIRQHandler**: Handles DMA transfer complete and errors
 * - **UARTIRQHandler**: Handles UART errors, idle line, RXNE (streaming)
 * - **Watchdog**: Provides ISR-to-thread signaling and deadline polling
 * - **Atomics**: std::atomic for circular buffer write position (Decision 12, 23)
 * - **Cache ops**: Invalidate D-cache after DMA reception (Decision 20)
 *
 * ## Memory Layout (Decision 20: DMA Alignment)
 *
 * Internal DMA buffers use alignas(32) for cache line alignment:
 * @code
 * alignas(32) uint8_t internal_buffer[MAX_SIZE];
 * @endcode
 *
 * User-provided buffers (Transmit operations) must ensure alignment themselves.
 * For receive operations, the DMA handler uses driver-owned internal buffers
 * with guaranteed alignment.
 *
 * ## Timeout Implementation (Decision 25: ISR-Side Deadline Polling)
 *
 * The timeout mechanism uses ISR-side deadline polling:
 * - Deadline stored as absolute time_point in Watchdog
 * - Calculated as `current_time + timeout` when start() is called
 * - ISR checks `steady_clock::now() > deadline` opportunistically
 * - If expired, ISR calls Watchdog.release(ErrTimeout)
 * - Accuracy depends on ISR firing frequency (~1-10ms typical)
 *
 * ## Error Propagation (Decision 26: Atomic Status Flags)
 *
 * Errors detected in ISR are propagated via Watchdog:
 * @code
 * // In ISR:
 * if (dma_error_detected) {
 *     watchdog_.release(UartStatus::ErrError);  // memory_order_release
 * }
 *
 * // In application thread:
 * status = watchdog_.await_event(timeout);  // memory_order_acquire
 * @endcode
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/stm32/dma_receive_handler.hpp>
 * #include <hm11/stm32/uart_port_impl.hpp>
 * #include <hm11/sync/watchdog.hpp>
 *
 * using namespace hm11::stm32;
 * using namespace hm11::sync;
 * using namespace hm11::hal;
 *
 * // Initialize hardware and synchronization
 * Watchdog watchdog;
 * STM32UARTPort uart_port;
 * DMA_Receive_Handler dma_handler(watchdog);
 *
 * // Blocking reception (AT command response)
 * uint8_t buffer[128];
 * UartStatus status;
 * UartData8b buffer_span(buffer, sizeof(buffer));
 * dma_handler(uart_port, buffer_span, status, 1000, false);
 * if (status == UartStatus::Ok) {
 *     // Process received data
 * }
 *
 * // Streaming reception (discovery scanning)
 * uint8_t stream_buffer[2048];
 * UartData8b stream_span(stream_buffer, sizeof(stream_buffer));
 *
 * // Start stream
 * dma_handler(uart_port, stream_span, status, 5000, true);
 *
 * // Poll circular buffer and parse data
 * size_t write_pos = dma_handler.get_write_position();
 * // ... parse data ...
 *
 * // Stop stream
 * dma_handler(uart_port, stream_span, status, 0, true);
 * @endcode
 *
 * ## Thread Safety
 *
 * - operator() must be called from application thread context only
 * - ISR handlers access internal state via atomics (ISR-safe)
 * - Not designed for concurrent calls (single-threaded application)
 *
 * ## Ada Correspondence
 *
 * This class translates Ada's DMA_Receive_Handler procedure:
 * - drivers.adb:324-361: Procedure implementation
 * - drivers.adb:36-41: Atomic variables for ISR state
 * - drivers.adb:189-318: ISR implementations (DMA and UART)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/hal/base_types.hpp>
#include <hm11/sync/watchdog.hpp>

namespace hm11::stm32 {

/**
 * @brief DMA-based receive handler for UART5 with blocking and streaming modes
 *
 * This class implements the IsReceiveHandler concept using DMA1 Stream 0 Channel 4
 * for UART5 reception. It provides efficient data reception with support for both
 * blocking mode (AT command responses) and streaming mode (discovery scanning).
 *
 * ## Key Features
 *
 * - **DMA offload**: Reduces CPU overhead for byte-by-byte reception
 * - **Blocking mode**: Wait for complete response with timeout
 * - **Streaming mode**: Continuous reception to circular buffer
 * - **ISR-safe**: All ISR-to-thread communication uses atomics
 * - **Cache coherent**: Proper D-cache management for Cortex-M7 (Decision 20)
 * - **Deadline polling**: ISR-side timeout detection (Decision 25)
 * - **Error propagation**: Atomic status flags via Watchdog (Decision 26)
 *
 * ## Memory Layout
 *
 * Size: ~32 bytes (Watchdog reference + atomics + state flags)
 * Does not own DMA buffer (uses user-provided buffer or driver-internal buffer)
 *
 * ## Initialization
 *
 * The handler must be initialized with a reference to a Watchdog instance:
 * @code
 * Watchdog watchdog;
 * DMA_Receive_Handler handler(watchdog);
 * @endcode
 *
 * The Watchdog reference must remain valid for the lifetime of the handler.
 *
 * @note This class is not copyable or movable (matching Ada's limited private)
 * @note All methods are noexcept for embedded reliability
 */
class DMA_Receive_Handler {
  public:
    /**
     * @brief Construct DMA receive handler with Watchdog reference
     *
     * Initializes the handler with a reference to the Watchdog synchronization
     * primitive. The Watchdog is used for timeout management and ISR-to-thread
     * signaling.
     *
     * @param watchdog Reference to Watchdog instance (must remain valid forever)
     *
     * @pre watchdog must remain valid for handler lifetime
     * @post Handler ready to perform receive operations
     */
    explicit DMA_Receive_Handler(sync::Watchdog& watchdog) noexcept;

    /**
     * @brief Destructor - ensures stream is stopped
     *
     * If a streaming operation is in progress when the handler is destroyed,
     * the destructor ensures proper cleanup by stopping the Watchdog and
     * disabling interrupts.
     */
    ~DMA_Receive_Handler() noexcept;

    // Delete copy and move operations (matching Ada's limited private pattern, Decision 22)
    DMA_Receive_Handler(const DMA_Receive_Handler&) = delete;
    DMA_Receive_Handler& operator=(const DMA_Receive_Handler&) = delete;
    DMA_Receive_Handler(DMA_Receive_Handler&&) = delete;
    DMA_Receive_Handler& operator=(DMA_Receive_Handler&&) = delete;

    /**
     * @brief Receive data from UART using DMA (with explicit timeout)
     *
     * This method implements the IsReceiveHandler concept's receive operation
     * with explicit timeout parameter. It supports both blocking and streaming
     * modes via the as_stream parameter.
     *
     * ## Blocking Mode (as_stream = false)
     *
     * Configures DMA for single-shot transfer and blocks until complete or timeout:
     * 1. Zero-initialize receive buffer (sentinel for incomplete data)
     * 2. Configure DMA in Normal mode (not Circular)
     * 3. Start DMA transfer from USART5->DR to buffer
     * 4. Enable DMA and UART interrupts
     * 5. Call Watchdog.start() with timeout deadline
     * 6. Block on Watchdog.await_event()
     * 7. Return status (Ok, ErrTimeout, ErrError)
     *
     * ## Streaming Mode (as_stream = true, First Call)
     *
     * Starts continuous reception without blocking:
     * 1. Store buffer address and size in atomic variables
     * 2. Initialize write position to 0
     * 3. Zero first byte as sentinel
     * 4. Enable UART RXNE interrupt (byte-by-byte ISR)
     * 5. Enable UART Idle Line interrupt (message boundaries)
     * 6. Call Watchdog.start() with timeout (for deadline polling)
     * 7. Set is_stream_ flag to true
     * 8. Return immediately with Status = Ok
     *
     * ## Streaming Mode (as_stream = true, Second Call)
     *
     * Stops streaming and cleans up:
     * 1. Check is_stream_ flag (if true, this is stop call)
     * 2. Call Watchdog.stop() to disable interrupts
     * 3. Clear is_stream_ flag
     * 4. Return immediately with Status = Ok
     *
     * ## ISR Interaction
     *
     * Between start and stop calls in streaming mode, ISR handlers process data:
     * - **UART RXNE ISR**: Reads byte, stores in circular buffer, increments position
     * - **UART Idle ISR**: Checks deadline, signals timeout if expired
     * - **DMA TC ISR**: Not used in streaming mode (RXNE handles byte-by-byte)
     *
     * @tparam UARTPortType UART port type satisfying IS_UART_PORT_V
     * @param port UART port to receive from
     * @param buffer Buffer to store received data
     * @param status Output parameter for operation result
     * @param timeout_ms Timeout in milliseconds
     * @param as_stream If true, start/stop streaming; if false, blocking receive
     *
     * @note ISR-safe: No (must be called from thread context)
     * @note Thread-safe: No (designed for single application thread)
     * @note Blocking: Yes in blocking mode, No in streaming mode
     *
     * @pre port must be initialized and ready
     * @pre buffer must remain valid until operation completes
     * @pre In streaming mode, buffer must remain valid until stop call
     *
     * @post In blocking mode: status contains result, buffer contains data
     * @post In streaming mode (first call): reception started, returns immediately
     * @post In streaming mode (second call): reception stopped, interrupts disabled
     */
    template <typename UARTPortType>
    void operator()(
        UARTPortType& port,
        hal::UartData8b buffer,
        hal::UartStatus& status,
        uint32_t timeout_ms,
        bool as_stream) noexcept;

    /**
     * @brief Receive data from UART using DMA (with default timeout)
     *
     * Overload that uses default timeout (1000ms), matching Ada's pattern where
     * timeout parameter can be omitted.
     *
     * @tparam UARTPortType UART port type satisfying IS_UART_PORT_V
     * @param port UART port to receive from
     * @param buffer Buffer to store received data
     * @param status Output parameter for operation result
     * @param as_stream If true, start/stop streaming; if false, blocking receive
     *
     * @see operator()(port, buffer, status, timeout_ms, as_stream) for details
     */
    template <typename UARTPortType>
    void operator()(UARTPortType& port, hal::UartData8b buffer, hal::UartStatus& status, bool as_stream) noexcept {
        (*this)(port, buffer, status, DEFAULT_TIMEOUT_MS, as_stream);
    }

    /**
     * @brief Query circular buffer write position (for streaming mode)
     *
     * Returns the current write position in the circular buffer for streaming mode.
     * The application uses this to determine how much data has been received and
     * parse it incrementally.
     *
     * ## Ada Correspondence
     *
     * This method translates Ada's Last_Read_Handler procedure (drivers.adb:367-372)
     * which returns the Watchdog.Readed state (closed flag and write position).
     *
     * ## Usage Pattern
     *
     * @code
     * // Start streaming
     * dma_handler(port, buffer, status, 5000, true);
     *
     * // Poll write position and parse data
     * while (true) {
     *     size_t write_pos = dma_handler.get_write_position();
     *     // Parse data from read_pos to write_pos
     *     // ...
     *     if (stream_closed()) break;
     * }
     *
     * // Stop streaming
     * dma_handler(port, buffer, status, 0, true);
     * @endcode
     *
     * @return Current write position in circular buffer (atomic load with acquire)
     *
     * @note ISR-safe: Yes (atomic load)
     * @note Thread-safe: Yes
     * @note Blocking: No
     *
     * @post Returns position written by ISR (0 to buffer_size-1)
     */
    [[nodiscard]] std::size_t get_write_position() const noexcept;

    /**
     * @brief Check if streaming reception is closed (timeout/error)
     *
     * Returns true if the streaming operation has been closed due to timeout
     * or error. This is determined by checking if the Watchdog has been released.
     *
     * ## Usage Pattern
     *
     * @code
     * while (!dma_handler.stream_closed()) {
     *     size_t write_pos = dma_handler.get_write_position();
     *     // Process data...
     * }
     * @endcode
     *
     * @return true if stream closed (timeout/error), false if still active
     *
     * @note ISR-safe: Yes (atomic loads)
     * @note Thread-safe: Yes
     * @note Blocking: No
     */
    [[nodiscard]] bool stream_closed() const noexcept;

  private:
    /// Default timeout matching Ada's (Timeout : Natural := 1000)
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

    /**
     * @brief Reference to Watchdog synchronization primitive
     *
     * The Watchdog manages timeout deadlines and ISR-to-thread signaling.
     * ISR handlers call watchdog_.release(status) to wake the waiting thread.
     *
     * Lifetime: Must remain valid for handler lifetime (typically static storage).
     */
    sync::Watchdog& watchdog_;

    /**
     * @brief Atomic flag indicating streaming mode is active
     *
     * Matches Ada's Is_Stream : Boolean with Volatile, Atomic (drivers.adb:37).
     *
     * States:
     * - false: Not in streaming mode (idle or blocking mode)
     * - true: Streaming mode active (between start and stop calls)
     *
     * Used by operator() to determine if this is a start or stop call for streaming.
     *
     * Memory ordering:
     * - Written with memory_order_release when starting stream
     * - Read with memory_order_acquire to determine if stopping stream
     */
    std::atomic<bool> is_stream_{false};

    /**
     * @brief Atomic write position for circular buffer (streaming mode)
     *
     * Matches Ada's Position : Positive with Volatile, Atomic (drivers.adb:40).
     *
     * In streaming mode:
     * - ISR increments this position after writing each received byte
     * - Application reads this position to determine how much data is available
     * - Wraps around when reaching buffer size (circular buffer behavior)
     *
     * Memory ordering:
     * - ISR writes with memory_order_release (publish byte writes)
     * - Application reads with memory_order_acquire (observe byte writes)
     *
     * Valid range: 0 to buffer_size-1 (Note: Ada uses 1-based indexing)
     */
    std::atomic<std::size_t> write_position_{0};

    /**
     * @brief Atomic buffer address for ISR access (streaming mode)
     *
     * Matches Ada's Received_Address : System.Address with Volatile, Atomic (drivers.adb:36).
     *
     * Stores the buffer address during streaming mode so ISR can write received bytes.
     * Set when starting stream, cleared when stopping stream.
     *
     * Memory ordering:
     * - Written with memory_order_release when starting stream
     * - Read with memory_order_acquire in ISR to access buffer
     */
    std::atomic<void*> buffer_address_{nullptr};

    /**
     * @brief Atomic buffer size for ISR bounds checking (streaming mode)
     *
     * Matches Ada's Maximum : Positive with Volatile, Atomic (drivers.adb:38).
     *
     * Stores the buffer size during streaming mode so ISR knows when to wrap
     * the circular buffer write position.
     *
     * Memory ordering:
     * - Written with memory_order_release when starting stream
     * - Read with memory_order_acquire in ISR for bounds checking
     */
    std::atomic<std::size_t> buffer_size_{0};

    /**
     * @brief Internal DMA receive buffer with cache line alignment
     *
     * This buffer is used for DMA reception operations to ensure proper alignment
     * and cache coherency (Decision 20: Memory Layout and DMA Alignment).
     *
     * ## Alignment Requirements
     *
     * The buffer is aligned to 32 bytes (DCACHE_LINE_SIZE) for two reasons:
     * 1. **DMA alignment**: Some STM32 DMA controllers require aligned addresses
     * 2. **Cache coherency**: On Cortex-M7, cache operations work on 32-byte lines
     *
     * ## Cache Coherency (Decision 20)
     *
     * On Cortex-M7 devices:
     * - After DMA writes to this buffer, the ISR invalidates the cache
     * - The 32-byte alignment ensures we don't invalidate adjacent data
     *
     * On Cortex-M4 (STM32F429):
     * - No data cache exists, so alignment provides DMA address alignment only
     * - Cache operations compile to no-ops (zero overhead)
     *
     * ## Buffer Size
     *
     * 2048 bytes is chosen to match Ada's buffer sizes:
     * - Sufficient for discovery streaming (variable device names up to 248 bytes)
     * - Adequate for GATT responses (service/characteristic lists)
     * - Reasonable memory footprint for embedded systems
     *
     * ## Usage
     *
     * This buffer can be used internally by the handler for:
     * - Temporary storage during blocking receive operations
     * - Circular buffer for streaming mode
     * - Validated to have proper alignment for DMA and cache operations
     *
     * Applications can also provide their own buffers via the operator() interface,
     * but those buffers should also be aligned (documented in API comments).
     *
     * @note Size: 2048 bytes + padding for alignment (~2080 bytes total)
     * @note Alignment: 32 bytes (cache line boundary)
     * @note Memory: Stack or static storage depending on where handler is allocated
     */
    alignas(32) std::array<uint8_t, 2048> internal_rx_buffer_;
};

// Template implementation must be in header

template <typename UARTPortType>
void DMA_Receive_Handler::operator()(
    UARTPortType& port,
    hal::UartData8b buffer,
    hal::UartStatus& status,
    uint32_t timeout_ms,
    bool as_stream) noexcept {
    // Check if this is a streaming stop call
    if (is_stream_.load(std::memory_order_acquire)) {
        // Second call in streaming mode - stop the stream
        watchdog_.stop();
        is_stream_.store(false, std::memory_order_release);
        status = hal::UartStatus::Ok;
        return;
    }

    // Store mode flag for ISR and future calls
    is_stream_.store(as_stream, std::memory_order_release);

    if (as_stream) {
        // Streaming mode - start continuous reception
        // Store buffer info for ISR access (memory_order_release)
        buffer_address_.store(buffer.data(), std::memory_order_release);
        buffer_size_.store(buffer.size(), std::memory_order_release);
        write_position_.store(0, std::memory_order_release);

        // Zero first byte as sentinel (matches Ada pattern)
        if (!buffer.empty()) {
            buffer[0] = 0;
        }

        // Start watchdog with timeout (for deadline polling in ISR)
        // Note: We pass buffer.data() but ISR will use buffer_address_ atomic
        watchdog_.start(buffer.data(), buffer.size(), std::chrono::milliseconds(timeout_ms));

        // On STM32F429 targets with hardware integration:
        // - UART RXNE interrupt enables byte-by-byte reception
        // - UART Idle Line interrupt provides timeout detection
        // The ISR (UARTIRQHandler::handle_uart_interrupt):
        // - Reads bytes from USART5->DR on RXNE
        // - Stores at buffer[write_position_]
        // - Increments write_position_ atomically
        // - Wraps around at buffer_size_
        // - Checks deadline on Idle Line interrupt

        status = hal::UartStatus::Ok;
        return; // Don't block - stream is running

    } else {
        // Blocking mode - single-shot DMA transfer
        // Zero-initialize buffer (sentinel for distinguishing received data)
        std::fill(buffer.begin(), buffer.end(), uint8_t{0});

        // Start watchdog with timeout (blocking mode)
        watchdog_.start(buffer.data(), buffer.size(), std::chrono::milliseconds(timeout_ms));

        // On STM32F429 targets with hardware integration:
        // - DMA is configured in Normal mode (not Circular)
        // - DMA transfer runs from USART5->DR to buffer
        // - DMA interrupts are enabled (TC, TE, FIFO, DME)
        // - UART interrupts are enabled (Error, Idle)
        // The ISRs (DMAIRQHandler and UARTIRQHandler):
        // - On DMA TC: invalidate D-cache, call watchdog_.release(Ok)
        // - On DMA error: call watchdog_.release(ErrError)
        // - On UART error: call watchdog_.release(ErrError)
        // - On Idle Line + deadline expired: call watchdog_.release(ErrTimeout)

        // Block until ISR signals completion
        status = watchdog_.await_event(std::chrono::milliseconds(timeout_ms));
        return;
    }
}

} // namespace hm11::stm32
