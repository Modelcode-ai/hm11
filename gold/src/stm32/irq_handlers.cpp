/**
 * @file irq_handlers.cpp
 * @brief STM32 interrupt handler implementations and extern C wrappers
 *
 * This implementation file provides:
 * 1. Static method implementations for UARTIRQHandler and DMAIRQHandler classes
 * 2. Extern C wrapper functions for the STM32 vector table
 * 3. Static storage for handler state (references to synchronization primitives)
 *
 * ## Implementation Status
 *
 * **Current Implementation:**
 * Provides full ISR implementations for STM32F429 targets that:
 * - Access UART and DMA registers via CMSIS (stm32f4xx.h)
 * - Check status flags and detect errors
 * - Perform cache invalidation on Cortex-M7 (Decision 20)
 * - Implement timeout checking (Decision 25)
 * - Propagate status via Watchdog (Decision 26)
 *
 * For non-STM32 targets (test/mock environments), stub implementations
 * are provided that compile without errors but do not perform hardware operations.
 *
 * ## Design Pattern (Decision 16)
 *
 * The implementation follows the interrupt handler registration pattern where:
 * 1. C++ classes (UARTIRQHandler, DMAIRQHandler) contain ISR logic
 * 2. Static methods provide ISR entry points
 * 3. Static members store references to synchronization primitives
 * 4. Extern C wrappers bridge hardware vector table to C++ classes
 *
 * This pattern provides organization and encapsulation while maintaining
 * compatibility with the ARM Cortex-M exception mechanism.
 *
 * ## Memory Ordering (Decision 23)
 *
 * When the full implementation is added, all ISR-to-thread communication will use
 * memory_order_release/acquire semantics:
 * - ISR stores status with memory_order_release before signaling
 * - Thread loads status with memory_order_acquire after waking
 * - This provides happens-before relationship for SPSC patterns
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#include "hm11/stm32/irq_handlers.hpp"

// Include full headers for types that were forward-declared in the header
#include "hm11/hal/base_types.hpp"
#include "hm11/sync/binary_semaphore.hpp"
#include "hm11/sync/watchdog.hpp"

// Include CMSIS headers for hardware register access
// Note: These headers are provided by the STM32 platform configuration
// and are conditionally available when building for STM32 targets.
#ifdef STM32F429xx
#include "stm32f4xx.h" // For USART, DMA register definitions
#endif

// Include cache operations for Cortex-M7 DMA coherency (Decision 20)
#include "hm11/platform/cache_ops.hpp"

// Include chrono for timeout checking
#include <chrono>

namespace hm11::stm32 {

// ============================================================================
// ISR Shared State (matching Ada's package-level atomics, drivers.adb:36-41)
// ============================================================================

/**
 * @brief Atomic variables shared between application and ISR contexts
 *
 * These variables match Ada's package-level atomic variables (drivers.adb:36-41):
 * - Received_Address : System.Address with Volatile, Atomic
 * - Is_Stream : Boolean with Volatile, Atomic
 * - Maximum : Positive with Volatile, Atomic
 * - Position : Positive with Volatile, Atomic
 *
 * Note: These are static/namespace-level variables (not class members) to match
 * Ada's architecture where they're package-level state accessible to both the
 * DMA_Receive_Handler procedure and the ISR handlers.
 *
 * In the full implementation, these would be managed by the DMA_Receive_Handler
 * class and ISRs would access them through static references. For now, we include
 * them here for ISR implementation completeness.
 *
 * Memory ordering (Decision 23):
 * - Application writes with memory_order_release
 * - ISR reads with memory_order_acquire
 * - ISR writes with memory_order_release (write position)
 * - Application reads with memory_order_acquire (write position)
 */
namespace {
std::atomic<void*> received_address{nullptr};
std::atomic<bool> is_stream{false};
std::atomic<std::size_t> maximum{1};
std::atomic<std::size_t> position{1}; // Note: Ada uses 1-based indexing
} // anonymous namespace

// ============================================================================
// UARTIRQHandler Static Members
// ============================================================================

/**
 * @brief Static storage for Watchdog reference
 *
 * This pointer is initialized by UARTIRQHandler::initialize() during application
 * startup and accessed by the ISR to signal completion and propagate status.
 *
 * Lifetime: Initialized once during startup, accessed from ISR throughout program
 * lifetime. Must point to static storage with program lifetime.
 *
 * Thread safety: Written once during single-threaded initialization, read from
 * ISR (serialized by hardware). No additional synchronization needed.
 *
 * Initialized to nullptr to detect uninitialized access (will fault if ISR fires
 * before initialize() is called, which is a fatal programming error).
 */
sync::Watchdog* UARTIRQHandler::watchdog_ = nullptr;

// ============================================================================
// UARTIRQHandler Method Implementations
// ============================================================================

/**
 * @brief Initialize UART interrupt handler with synchronization primitives
 *
 * This function stores references to the synchronization primitives that the ISR
 * will use. It must be called during application initialization before enabling
 * UART interrupts in NVIC.
 *
 * ## Design Rationale
 *
 * Why not use constructor? This is a static-only class (cannot be instantiated)
 * because ISR entry points must be static functions (no this pointer). Using a
 * static initialize() function is the standard pattern for configuring static-only
 * utility classes.
 *
 * ## Error Handling
 *
 * This function does not validate the pointer (no assert or exception) because:
 * 1. It's called during controlled initialization (programmer error if null)
 * 2. If nullptr, ISR will fault immediately on first access (obvious failure)
 * 3. Adding checks would require exception handling or error return (unnecessary
 *    complexity for initialization that should never fail)
 *
 * @param watchdog Pointer to Watchdog instance (must remain valid forever)
 */
void UARTIRQHandler::initialize(sync::Watchdog* watchdog) {
    watchdog_ = watchdog;

    // Additional initialization may be performed here if needed
    // - Configure UART interrupt enables (may be done elsewhere)
    // - Set up any ISR-specific state
}

/**
 * @brief UART interrupt service routine implementation
 *
 * This implementation follows Ada's On_UART_IRQ pattern (drivers.adb:246-316)
 * and handles:
 * 1. Error conditions (PE, FE, NE, ORE, LBD, CTS)
 * 2. Timeout detection via ISR-side deadline polling (Decision 25)
 * 3. Idle line detection for variable-length responses
 * 4. Data reception (RXNE) for streaming mode
 *
 * ## Error Propagation (Decision 26)
 *
 * Errors detected in ISR are propagated via atomic status in Watchdog:
 * - watchdog_->release(status) stores status with memory_order_release
 * - Application reads status with memory_order_acquire in await_event()
 *
 * ## Memory Ordering (Decision 23)
 *
 * All ISR-to-thread communication uses memory_order_release/acquire:
 * - ISR writes buffer data, then releases watchdog with status
 * - Thread acquires status, then reads buffer data
 * - This provides happens-before relationship for correctness
 */
void UARTIRQHandler::handle_uart_interrupt() noexcept {
#ifdef STM32F429xx
    // Access UART5 registers via CMSIS
    USART_TypeDef* uart = UART5;

    // Helper lambda to check error flag and release watchdog if set
    // Matches Ada's Check procedure (drivers.adb:251-266)
    auto check_error = [&](uint32_t status_flag, uint32_t interrupt_flag) {
        if ((uart->SR & status_flag) && (uart->CR1 & interrupt_flag)) {
            // Disable interrupt
            uart->CR1 &= ~interrupt_flag;
            // Clear status flag (note: some flags clear on read, some need write)
            // For simplicity, we attempt to clear by reading SR then DR
            volatile uint32_t sr = uart->SR;
            volatile uint32_t dr = uart->DR;
            (void)sr;
            (void)dr;
            // Signal error to application thread
            watchdog_->release(hal::UartStatus::ErrError);
        }
    };

    // Check error conditions first (highest priority)
    // Matches Ada's sequence (drivers.adb:272-277)
    check_error(USART_SR_PE, USART_CR1_PEIE); // Parity error
    check_error(USART_SR_FE, USART_CR3_EIE);  // Framing error
    check_error(USART_SR_NE, USART_CR3_EIE);  // Noise error
    check_error(USART_SR_ORE, USART_CR3_EIE); // Overrun error
    // Note: Line break and CTS may not have dedicated interrupt enables in all configurations

    // Handle idle line detection (drivers.adb:279-289)
    if ((uart->SR & USART_SR_IDLE) && (uart->CR1 & USART_CR1_IDLEIE)) {
        // Check timeout deadline (Decision 25: ISR-side deadline polling)
        if (watchdog_->check_deadline()) {
            // Timeout expired - disable interrupt and signal timeout
            uart->CR1 &= ~USART_CR1_IDLEIE;
            // Clear idle flag by reading SR then DR
            volatile uint32_t sr = uart->SR;
            volatile uint32_t dr = uart->DR;
            (void)sr;
            (void)dr;
            watchdog_->release(hal::UartStatus::Ok); // Ada releases Ok on timeout in idle handler
        } else {
            // Not timed out yet - just clear the idle flag and continue
            volatile uint32_t sr = uart->SR;
            volatile uint32_t dr = uart->DR;
            (void)sr;
            (void)dr;
        }
    }

    // Handle data reception (RXNE) - streaming or blocking mode (drivers.adb:291-312)
    if ((uart->SR & USART_SR_RXNE) && (uart->CR1 & USART_CR1_RXNEIE)) {
        // Read received byte from data register
        uint8_t received_byte = static_cast<uint8_t>(uart->DR & 0xFF);

        // Get buffer info from atomics (Decision 23: memory_order_acquire)
        void* buffer_ptr = received_address.load(std::memory_order_acquire);
        std::size_t max_size = maximum.load(std::memory_order_acquire);
        std::size_t pos = position.load(std::memory_order_acquire);
        bool streaming = is_stream.load(std::memory_order_acquire);

        if (buffer_ptr != nullptr && max_size > 0) {
            // Store byte in buffer (0-based indexing, but Ada uses 1-based, so pos-1)
            // Actually, looking at Ada code, Position is used as 1-based array index
            // and we use it directly: Memory_Block(Position) := ...
            // In C++, we need 0-based, so we use pos-1
            auto* buffer = static_cast<uint8_t*>(buffer_ptr);

            if (pos > 0 && pos <= max_size) {
                buffer[pos - 1] = received_byte; // Convert 1-based to 0-based
            }

            if (streaming) {
                // Streaming mode: advance position with wrap-around (drivers.adb:298-303)
                if (pos < max_size) {
                    pos = pos + 1;
                } else {
                    pos = 1; // Wrap to beginning (1-based)
                }
                // Store zero byte at new position as sentinel (drivers.adb:303)
                if (pos > 0 && pos <= max_size) {
                    buffer[pos - 1] = 0;
                }
                // Update position atomically (Decision 23: memory_order_release)
                position.store(pos, std::memory_order_release);

            } else {
                // Blocking mode: check if buffer full (drivers.adb:306-310)
                if (pos == max_size) {
                    // Buffer full - signal completion
                    watchdog_->release(hal::UartStatus::Ok);
                } else {
                    // Advance position
                    position.store(pos + 1, std::memory_order_release);
                }
            }
        }
    }
#else
    // Non-STM32 target: stub implementation
    // When compiling for test/mock environments without hardware
    (void)watchdog_; // Suppress unused warning
#endif
}

// ============================================================================
// DMAIRQHandler Static Members
// ============================================================================

/**
 * @brief Static storage for Watchdog reference
 *
 * This pointer is initialized by DMAIRQHandler::initialize() during application
 * startup and accessed by the ISR to signal completion and propagate status.
 *
 * See UARTIRQHandler::watchdog_ documentation for detailed explanation of
 * lifetime, thread safety, and initialization rationale.
 */
sync::Watchdog* DMAIRQHandler::watchdog_ = nullptr;

// ============================================================================
// DMAIRQHandler Method Implementations
// ============================================================================

/**
 * @brief Initialize DMA interrupt handler with synchronization primitives
 *
 * This function stores references to the synchronization primitives that the ISR
 * will use. It must be called during application initialization before enabling
 * DMA interrupts in NVIC.
 *
 * See UARTIRQHandler::initialize() documentation for detailed explanation of
 * design rationale and error handling strategy.
 *
 * @param watchdog Pointer to Watchdog instance (must remain valid forever)
 */
void DMAIRQHandler::initialize(sync::Watchdog* watchdog) {
    watchdog_ = watchdog;

    // Additional initialization may be performed here if needed
    // - Configure DMA interrupt enables (may be done elsewhere)
    // - Set up any ISR-specific state
}

/**
 * @brief DMA interrupt service routine implementation
 *
 * This implementation follows Ada's On_DMA_IRQ pattern (drivers.adb:199-232)
 * and handles:
 * 1. Transfer error detection (TEIF)
 * 2. FIFO error detection (FEIF)
 * 3. Direct mode error detection (DMEIF)
 * 4. Transfer complete notification (TCIF)
 * 5. D-cache invalidation for Cortex-M7 (Decision 20)
 *
 * ## Cache Coherency (Decision 20)
 *
 * The DMA ISR is the critical location where cache invalidation must occur on
 * Cortex-M7 devices. After DMA writes data directly to memory (bypassing cache),
 * the cache must be invalidated before the application thread reads the data.
 *
 * The invalidation happens in the ISR (not in the application thread) because:
 * 1. Timing: Must happen immediately after DMA completion, before thread wakes
 * 2. Memory ordering: Pairs with atomic release in watchdog_->release()
 * 3. Correctness: Ensures thread observes DMA-written data when it wakes
 *
 * ## Error Propagation (Decision 26)
 *
 * Errors detected in ISR are propagated via atomic status in Watchdog:
 * - watchdog_->release(status) stores status with memory_order_release
 * - Application reads status with memory_order_acquire in await_event()
 */
void DMAIRQHandler::handle_dma_interrupt() noexcept {
#ifdef STM32F429xx
    // Access DMA1 registers via CMSIS
    DMA_Stream_TypeDef* dma_stream = DMA1_Stream0;
    DMA_TypeDef* dma = DMA1;

    // Helper lambda to check and handle DMA errors
    // Matches Ada's Check procedure (drivers.adb:202-218)
    auto check_error = [&](uint32_t status_flag, uint32_t clear_flag, uint32_t interrupt_enable) -> bool {
        if (dma->LISR & status_flag) {
            // Check if interrupt is enabled (only handle if we enabled it)
            if (dma_stream->CR & interrupt_enable) {
                // Disable interrupt
                dma_stream->CR &= ~interrupt_enable;
                // Clear status flag
                dma->LIFCR = clear_flag;
                // Signal error to application thread (Decision 26)
                watchdog_->release(hal::UartStatus::ErrError);
                return true; // Error handled
            }
        }
        return false; // No error or not our interrupt
    };

    // Check error conditions first (highest priority)
    // Matches Ada's sequence (drivers.adb:221-226)
    if (check_error(DMA_LISR_TEIF0, DMA_LIFCR_CTEIF0, DMA_SxCR_TEIE)) {
        return; // Transfer error - exit early
    }

    if (check_error(DMA_LISR_FEIF0, DMA_LIFCR_CFEIF0, DMA_SxFCR_FEIE)) {
        return; // FIFO error - exit early
    }

    if (check_error(DMA_LISR_DMEIF0, DMA_LIFCR_CDMEIF0, DMA_SxCR_DMEIE)) {
        return; // Direct mode error - exit early
    }

    // Check transfer complete flag (success case) (drivers.adb:227)
    if (dma->LISR & DMA_LISR_TCIF0) {
        // Check if transfer complete interrupt is enabled
        if (dma_stream->CR & DMA_SxCR_TCIE) {
            // Disable interrupt
            dma_stream->CR &= ~DMA_SxCR_TCIE;
            // Clear status flag
            dma->LIFCR = DMA_LIFCR_CTCIF0;

            // Invalidate D-cache for received buffer (Decision 20: Cache Coherency)
            // On Cortex-M7, DMA writes bypass cache. Must invalidate to see DMA data.
            // On Cortex-M4 (STM32F429), this compiles to no-op (zero overhead).
            void* rx_buffer = received_address.load(std::memory_order_acquire);
            std::size_t buffer_size = maximum.load(std::memory_order_acquire);

            if (rx_buffer != nullptr && buffer_size > 0) {
                // Invalidate cache range to ensure CPU sees DMA-written data
                // This function is conditionally compiled:
                // - Cortex-M7: Calls SCB_InvalidateDCache_by_Addr
                // - Cortex-M4: Compiles to empty inline function (zero overhead)
                platform::invalidate_dcache_range(rx_buffer, buffer_size);
            }

            // Signal success (Decision 26: Atomic Status Propagation)
            // The memory_order_release in watchdog_->release() ensures all DMA
            // writes and cache invalidation are visible when the application thread
            // reads the status with memory_order_acquire.
            watchdog_->release(hal::UartStatus::Ok);
            return;
        }
    }
#else
    // Non-STM32 target: stub implementation
    // When compiling for test/mock environments without hardware
    (void)watchdog_; // Suppress unused warning
#endif
}

} // namespace hm11::stm32

// ============================================================================
// Extern C Wrapper Functions (Vector Table Entry Points)
// ============================================================================

/**
 * @brief UART5 interrupt vector table wrapper
 *
 * This extern "C" function overrides the weak symbol defined in the STM32 startup
 * code (startup_stm32f429xx.s or .c). When a UART5 interrupt fires, the ARM
 * Cortex-M exception mechanism calls this function.
 *
 * ## Implementation
 *
 * The implementation is intentionally minimal - just a single function call to
 * the C++ static method. This provides:
 * - Zero overhead: One function call instruction
 * - Clear intent: Obvious mapping from vector name to handler class
 * - Maintainability: All logic lives in C++ class, wrapper is trivial
 *
 * ## Why Not Inline?
 *
 * This function cannot be inline because it must have C linkage and be visible to
 * the linker as a strong symbol to override the weak symbol in startup code.
 *
 * ## Memory Ordering
 *
 * No explicit memory barriers are needed here. The C++ handler implementation
 * uses std::atomic with appropriate memory ordering (release/acquire) for
 * ISR-to-thread synchronization.
 */
extern "C" void USART5_IRQHandler() {
    hm11::stm32::UARTIRQHandler::handle_uart_interrupt();
}

/**
 * @brief DMA1 Stream 0 interrupt vector table wrapper
 *
 * This extern "C" function overrides the weak symbol defined in the STM32 startup
 * code (startup_stm32f429xx.s or .c). When a DMA1 Stream 0 interrupt fires, the
 * ARM Cortex-M exception mechanism calls this function.
 *
 * See USART5_IRQHandler() documentation for detailed explanation of wrapper
 * pattern, implementation rationale, and memory ordering.
 */
extern "C" void DMA1_Stream0_IRQHandler() {
    hm11::stm32::DMAIRQHandler::handle_dma_interrupt();
}
