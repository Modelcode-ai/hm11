/**
 * @file irq_handlers.hpp
 * @brief STM32 interrupt handler infrastructure for UART and DMA
 *
 * This header provides C++ class-based interrupt handler organization following
 * **Decision 16** (Interrupt Handler Registration). The implementation uses C++
 * classes with static methods to encapsulate ISR logic, along with extern "C"
 * wrappers for the STM32 startup code's vector table.
 *
 * ## Architecture (Decision 16: Interrupt Handler Registration)
 *
 * The STM32 vector table requires extern "C" function pointers, but we want to
 * organize ISR logic using C++ classes for better encapsulation and state
 * management, matching Ada's protected object pattern.
 *
 * **Ada Protected Object Pattern (drivers.ads:114-133):**
 * @code{.ada}
 * protected IRQ_UART_Handler is
 *    pragma Interrupt_Priority;
 * private
 *    procedure On_UART_IRQ;
 *    pragma Attach_Handler (On_UART_IRQ, UART_IRQ);
 * end IRQ_UART_Handler;
 *
 * protected IRQ_DMA_Handler is
 *    pragma Interrupt_Priority;
 * private
 *    procedure On_DMA_IRQ;
 *    pragma Attach_Handler (On_DMA_IRQ, DMA_Tx_IRQ);
 * end IRQ_DMA_Handler;
 * @endcode
 *
 * **C++ Translation Pattern:**
 * @code
 * // C++ class with static methods (encapsulates ISR logic)
 * class UARTIRQHandler {
 * public:
 *     static void handle_uart_interrupt() noexcept;
 * private:
 *     static Watchdog* watchdog_;
 * };
 *
 * // Extern C wrapper (matches vector table signature)
 * extern "C" void USART5_IRQHandler() {
 *     UARTIRQHandler::handle_uart_interrupt();
 * }
 * @endcode
 *
 * ## Design Rationale
 *
 * ### Why Classes with Static Methods?
 *
 * 1. **Organization**: Groups ISR logic with related state (synchronization
 *    primitives, buffer references) similar to Ada's protected object
 * 2. **Encapsulation**: Private static members for state, public static methods
 *    for ISR entry points
 * 3. **Zero Overhead**: Static methods have no this pointer overhead, compile to
 *    regular functions
 * 4. **Type Safety**: C++ namespace and class scoping prevents name collisions
 * 5. **Minimal Boilerplate**: Single extern C wrapper per handler (just calls
 *    static method)
 * 6. **Clear Separation**: Class contains logic, extern C provides vector table
 *    compatibility
 *
 * ### Alternative Approaches (Rejected)
 *
 * 1. **Bare extern C functions**: Poor organization, global state, no encapsulation
 * 2. **Lambda with captures**: Cannot convert to function pointer for vector table
 * 3. **Member functions**: Require object instance, cannot use in vector table
 * 4. **Namespace functions**: Less organized than classes, no private state
 *
 * ## Interrupt Handler Lifecycle
 *
 * 1. **Initialization** (application startup):
 *    - Application calls `UARTIRQHandler::initialize(watchdog, uart, dma)`
 *    - Handler stores references to synchronization primitives and hardware
 *    - Application configures NVIC interrupt priorities and enables interrupts
 *
 * 2. **ISR Execution** (when hardware interrupt fires):
 *    - ARM Cortex-M exception mechanism calls extern C wrapper
 *    - Wrapper immediately calls static method (e.g., `handle_uart_interrupt()`)
 *    - Static method accesses stored references to perform operations
 *
 * 3. **ISR Logic** (implemented for STM32F429 targets):
 *    - Check hardware status flags
 *    - Handle errors (propagate via atomic status, Decision 26)
 *    - Check timeout deadline (ISR-side polling, Decision 25)
 *    - Process received data or signal completion
 *    - Call `watchdog->release(status)` to wake waiting thread
 *
 * ## Timeout Implementation (Decision 25: ISR-Side Deadline Polling)
 *
 * The ISR checks for timeout expiration opportunistically when it fires:
 * @code
 * void UARTIRQHandler::handle_uart_interrupt() noexcept {
 *     // Check if deadline expired (Decision 25)
 *     if (watchdog_->check_deadline()) {
 *         watchdog_->release(UartStatus::ErrTimeout);
 *         return;
 *     }
 *
 *     // Normal interrupt processing...
 * }
 * @endcode
 *
 * This matches Ada's pattern (drivers.adb:282):
 * @code{.ada}
 * if Clock > Until_Time then
 *    Watchdog.Release(Ok);  -- Actually timeout
 * end if;
 * @endcode
 *
 * ## Error Propagation (Decision 26: Atomic Status Flags)
 *
 * Errors detected in the ISR are propagated to the application thread using
 * atomic status flags in the Watchdog:
 * @code
 * // ISR detects error
 * if (uart_has_error()) {
 *     watchdog_->release(UartStatus::ErrError);  // Atomic store with release
 * }
 *
 * // Application thread receives error
 * UartStatus status = watchdog_->await_event(timeout);  // Atomic load with acquire
 * @endcode
 *
 * This provides happens-before relationship ensuring all ISR buffer writes are
 * visible when the application reads the status.
 *
 * ## ISR Safety Requirements
 *
 * All ISR code must follow these safety requirements:
 * - **noexcept**: ISR functions cannot throw exceptions (undefined behavior)
 * - **No heap allocation**: No new, malloc, or dynamic containers in ISR
 * - **No blocking operations**: No mutexes, condition variables, or sleep
 * - **ISR-safe primitives only**: Use BinarySemaphore::give_from_isr(),
 *   Watchdog::release(), std::atomic operations
 * - **Short execution time**: Complete quickly to avoid blocking other interrupts
 * - **Reentrant**: ISR may be interrupted by higher priority interrupts
 *
 * ## Interrupt Priorities
 *
 * The application should configure interrupt priorities appropriately:
 * - **DMA interrupts** (higher priority): Time-critical, minimal processing
 * - **UART interrupts** (lower priority): Can be interrupted by DMA
 * - **Application code** (lowest priority): Normal thread priority
 *
 * Example NVIC configuration:
 * @code
 * // DMA1_Stream0 (RX) - Priority 5 (0-15, lower number = higher priority)
 * NVIC_SetPriority(DMA1_Stream0_IRQn, 5);
 * NVIC_EnableIRQ(DMA1_Stream0_IRQn);
 *
 * // UART5 - Priority 6 (lower than DMA)
 * NVIC_SetPriority(UART5_IRQn, 6);
 * NVIC_EnableIRQ(UART5_IRQn);
 * @endcode
 *
 * ## Vector Table Integration
 *
 * The STM32 startup code defines weak symbol interrupt handlers that default to
 * infinite loops. Our extern C functions override these weak symbols:
 *
 * **startup_stm32f429xx.s (vector table):**
 * @code{.asm}
 * .word USART5_IRQHandler    // Vector table entry
 * .word DMA1_Stream0_IRQHandler
 *
 * .weak USART5_IRQHandler    // Weak default handler
 * .thumb_set USART5_IRQHandler,Default_Handler
 * @endcode
 *
 * **Our override (irq_handlers.cpp):**
 * @code
 * extern "C" void USART5_IRQHandler() {
 *     UARTIRQHandler::handle_uart_interrupt();
 * }
 * @endcode
 *
 * The linker replaces the weak symbol with our strong symbol, making our function
 * the ISR entry point.
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/stm32/irq_handlers.hpp>
 * #include <hm11/sync/watchdog.hpp>
 *
 * using namespace hm11::stm32;
 * using namespace hm11::sync;
 *
 * // Application initialization
 * void setup_uart_interrupts() {
 *     // Create watchdog for synchronization
 *     static Watchdog watchdog;
 *
 *     // Initialize handler with references to primitives
 *     UARTIRQHandler::initialize(&watchdog, uart_registers, dma_registers);
 *     DMAIRQHandler::initialize(&watchdog, dma_registers);
 *
 *     // Configure NVIC priorities and enable interrupts
 *     NVIC_SetPriority(DMA1_Stream0_IRQn, 5);
 *     NVIC_EnableIRQ(DMA1_Stream0_IRQn);
 *
 *     NVIC_SetPriority(UART5_IRQn, 6);
 *     NVIC_EnableIRQ(UART5_IRQn);
 * }
 *
 * // The extern C wrappers are automatically called by hardware
 * @endcode
 *
 * ## Testing Strategy (Decision 21)
 *
 * ISR handlers cannot be directly unit tested without hardware or QEMU. Testing
 * approaches:
 * 1. **Mock ISR simulation**: Use std::thread to simulate ISR context
 * 2. **Integration tests**: Run example applications on hardware
 * 3. **QEMU validation** (future): Test ISR behavior in emulator
 *
 * ## Memory Ordering (Decision 23)
 *
 * ISR-to-thread synchronization uses std::memory_order_release/acquire:
 * - ISR: `watchdog->release()` uses memory_order_release when storing status
 * - Thread: `watchdog->await_event()` uses memory_order_acquire when reading status
 * - This provides happens-before relationship for SPSC patterns
 *
 * ## Implementation Status
 *
 * **Current Implementation:**
 * - Handler class declarations with static method signatures
 * - Extern C wrapper function declarations
 * - Full ISR implementations for STM32F429 targets
 * - Complete UART interrupt logic (status flag checking, error handling)
 * - Complete DMA interrupt logic (transfer complete, error handling)
 * - Hardware register access via CMSIS
 * - Integration with Watchdog and BinarySemaphore
 * - Documentation of architectural pattern
 *
 * For non-STM32 targets (test/mock environments), stub implementations
 * are provided.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Forward declarations for synchronization primitives
// Full headers included in implementation file to avoid circular dependencies
namespace hm11::sync {
class Watchdog;
class BinarySemaphore;
} // namespace hm11::sync

namespace hm11::hal {
enum class UartStatus : uint8_t;
} // namespace hm11::hal

namespace hm11::stm32 {

/**
 * @brief UART interrupt handler for STM32 USART5
 *
 * This class encapsulates the UART5 interrupt service routine logic following
 * Decision 16 (Interrupt Handler Registration). It groups ISR logic with related
 * synchronization primitives (Watchdog) in a C++ class, matching Ada's protected
 * object pattern.
 *
 * ## Responsibilities
 *
 * The UART ISR handles:
 * 1. **Error conditions**: Parity, framing, noise, overrun errors
 * 2. **Timeout detection**: ISR-side deadline polling (Decision 25)
 * 3. **Idle line detection**: Signals completion for variable-length responses
 * 4. **Data reception**: Byte-by-byte streaming mode (for discovery)
 * 5. **Status propagation**: Atomic error reporting (Decision 26)
 *
 * ## Ada Correspondence
 *
 * This class translates Ada's `IRQ_UART_Handler` protected object (drivers.ads:114-122,
 * drivers.adb:240-318) to C++:
 *
 * **Ada:**
 * @code{.ada}
 * protected IRQ_UART_Handler is
 *    pragma Interrupt_Priority;
 * private
 *    procedure On_UART_IRQ;
 *    pragma Attach_Handler (On_UART_IRQ, UART_IRQ);
 * end IRQ_UART_Handler;
 * @endcode
 *
 * **C++:**
 * @code
 * class UARTIRQHandler {
 * public:
 *     static void handle_uart_interrupt() noexcept;
 * };
 *
 * extern "C" void USART5_IRQHandler() {
 *     UARTIRQHandler::handle_uart_interrupt();
 * }
 * @endcode
 *
 * ## Interrupt Processing Flow
 *
 * When UART5 interrupt fires:
 * 1. Hardware saves context and calls `USART5_IRQHandler()` extern C wrapper
 * 2. Wrapper calls `UARTIRQHandler::handle_uart_interrupt()`
 * 3. ISR checks hardware status flags (CMSIS register access)
 * 4. ISR checks timeout deadline: `if (watchdog_->check_deadline())`
 * 5. On error: `watchdog_->release(UartStatus::ErrError)` with atomic status
 * 6. On timeout: `watchdog_->release(UartStatus::ErrTimeout)`
 * 7. On success: `watchdog_->release(UartStatus::Ok)`
 * 8. Return to interrupted context
 *
 * ## Initialization
 *
 * Before enabling UART interrupts, the application must initialize the handler:
 * @code
 * UARTIRQHandler::initialize(watchdog_ptr);
 * @endcode
 *
 * This stores a reference to the Watchdog synchronization primitive that the ISR
 * will signal on completion.
 *
 * ## ISR Safety
 *
 * All methods are:
 * - **noexcept**: No exceptions (ISR requirement)
 * - **Non-blocking**: No mutexes or condition variables
 * - **Non-allocating**: No heap operations
 * - **ISR-safe**: Only uses ISR-safe primitives (Watchdog, std::atomic)
 *
 * @note This class cannot be instantiated (all members static)
 * @note Thread-safety: ISR execution is serialized by hardware
 * @note Reentrancy: Not reentrant if called from multiple interrupt vectors
 */
class UARTIRQHandler {
  public:
    /**
     * @brief Initialize the UART interrupt handler
     *
     * Stores references to synchronization primitives that the ISR will use.
     * Must be called before enabling UART interrupts in NVIC.
     *
     * ## Lifecycle
     *
     * Call this function during application initialization, after creating the
     * Watchdog but before enabling UART interrupts:
     * @code
     * static Watchdog watchdog;
     * UARTIRQHandler::initialize(&watchdog);
     *
     * // Now safe to enable UART interrupt
     * NVIC_EnableIRQ(UART5_IRQn);
     * @endcode
     *
     * ## Thread Safety
     *
     * This function is NOT thread-safe and must be called before any interrupts
     * are enabled. Typically called from main() during single-threaded startup.
     *
     * @param watchdog Pointer to Watchdog instance for ISR-to-thread signaling
     *
     * @pre watchdog must remain valid for the lifetime of the program
     * @pre Must be called before enabling UART interrupts
     * @post ISR can safely access watchdog via stored reference
     *
     * @note Not noexcept because it's called during initialization where exceptions
     *       may be acceptable (startup errors should be fatal)
     */
    static void initialize(sync::Watchdog* watchdog);

    /**
     * @brief UART interrupt service routine entry point
     *
     * This static method is called by the extern C wrapper `USART5_IRQHandler()`
     * when a UART5 interrupt fires. It implements the complete ISR logic following
     * Ada's `On_UART_IRQ` procedure (drivers.adb:246-316).
     *
     * ## ISR Logic
     *
     * 1. **Check error conditions** (using CMSIS register access):
     *    - Parity error (SR.PE)
     *    - Framing error (SR.FE)
     *    - Noise error (SR.NF)
     *    - Overrun error (SR.ORE)
     *    - Line break detection (SR.LBD)
     *    - If any error: `watchdog_->release(UartStatus::ErrError)`
     *
     * 2. **Check timeout deadline** (Decision 25):
     *    - `if (watchdog_->check_deadline())`
     *    - If expired: `watchdog_->release(UartStatus::ErrTimeout)`
     *
     * 3. **Handle idle line detection**:
     *    - If SR.IDLE set and timeout not expired: just clear flag
     *    - If SR.IDLE set and timeout expired: signal completion
     *
     * 4. **Handle data reception** (streaming mode):
     *    - If SR.RXNE set: read DR register
     *    - Store in circular buffer
     *    - Advance write position
     *    - If buffer full or complete: `watchdog_->release(UartStatus::Ok)`
     *
     * 5. **Clear interrupt flags**:
     *    - Clear all serviced flags to prevent spurious re-entry
     *
     * ## Implementation
     *
     * On STM32F429 targets, provides full ISR implementation with UART register
     * access and Watchdog integration. On non-STM32 targets, provides stub
     * implementation for test/mock environments.
     *
     * @note ISR-safe: Yes (noexcept, non-blocking, non-allocating)
     * @note Called from: extern C USART5_IRQHandler() wrapper
     * @note Context: Interrupt context (cannot call blocking functions)
     *
     * @pre initialize() must have been called
     * @post Hardware interrupt flags are cleared
     * @post If operation complete, watchdog_->release() called with status
     */
    static void handle_uart_interrupt() noexcept;

  private:
    /**
     * @brief Pointer to Watchdog synchronization primitive
     *
     * The ISR uses the Watchdog to:
     * - Check deadline for timeout detection (Decision 25)
     * - Signal completion with status (Decision 26)
     *
     * Initialized by initialize() during application startup.
     * Must remain valid for program lifetime (typically static storage).
     *
     * Thread safety: Only written during initialization (single-threaded),
     * only read from ISR (serialized by hardware).
     */
    static sync::Watchdog* watchdog_;

    // Delete all constructors (static-only class)
    UARTIRQHandler() = delete;
    ~UARTIRQHandler() = delete;
    UARTIRQHandler(const UARTIRQHandler&) = delete;
    UARTIRQHandler& operator=(const UARTIRQHandler&) = delete;
    UARTIRQHandler(UARTIRQHandler&&) = delete;
    UARTIRQHandler& operator=(UARTIRQHandler&&) = delete;
};

/**
 * @brief DMA interrupt handler for STM32 DMA1 Stream 0 (UART5 RX)
 *
 * This class encapsulates the DMA Stream 0 interrupt service routine logic
 * following Decision 16 (Interrupt Handler Registration). It handles DMA
 * transfer completion, errors, and FIFO issues for UART5 reception.
 *
 * ## Responsibilities
 *
 * The DMA ISR handles:
 * 1. **Transfer complete**: Successful DMA reception finished
 * 2. **Transfer errors**: DMA configuration or bus errors
 * 3. **FIFO errors**: FIFO overrun/underrun conditions
 * 4. **Direct mode errors**: Direct mode transfer issues
 * 5. **Status propagation**: Atomic error reporting (Decision 26)
 *
 * ## Ada Correspondence
 *
 * This class translates Ada's `IRQ_DMA_Handler` protected object (drivers.ads:124-133,
 * drivers.adb:193-234) to C++:
 *
 * **Ada:**
 * @code{.ada}
 * protected IRQ_DMA_Handler is
 *    pragma Interrupt_Priority;
 * private
 *    procedure On_DMA_IRQ;
 *    pragma Attach_Handler (On_DMA_IRQ, DMA_Tx_IRQ);
 * end IRQ_DMA_Handler;
 * @endcode
 *
 * **C++:**
 * @code
 * class DMAIRQHandler {
 * public:
 *     static void handle_dma_interrupt() noexcept;
 * };
 *
 * extern "C" void DMA1_Stream0_IRQHandler() {
 *     DMAIRQHandler::handle_dma_interrupt();
 * }
 * @endcode
 *
 * ## Interrupt Processing Flow
 *
 * When DMA1 Stream 0 interrupt fires:
 * 1. Hardware saves context and calls `DMA1_Stream0_IRQHandler()` extern C wrapper
 * 2. Wrapper calls `DMAIRQHandler::handle_dma_interrupt()`
 * 3. ISR checks DMA status flags (CMSIS register access)
 * 4. On transfer error: `watchdog_->release(UartStatus::ErrError)`
 * 5. On FIFO error: `watchdog_->release(UartStatus::ErrError)`
 * 6. On transfer complete: `watchdog_->release(UartStatus::Ok)`
 * 7. Invalidate D-cache if Cortex-M7 (cache coherency, Decision 20)
 * 8. Return to interrupted context
 *
 * ## Cache Coherency (Decision 20)
 *
 * On Cortex-M7 devices (STM32F7/H7), DMA bypasses the data cache. After DMA
 * reception completes, the ISR must invalidate the cache to ensure the CPU
 * reads fresh data:
 * @code
 * // After DMA RX completion
 * hm11::platform::invalidate_dcache_range(rx_buffer, bytes_received);
 * watchdog_->release(UartStatus::Ok);
 * @endcode
 *
 * On Cortex-M4 (STM32F429), this compiles to a no-op (zero overhead).
 *
 * ## Initialization
 *
 * Before enabling DMA interrupts, the application must initialize the handler:
 * @code
 * DMAIRQHandler::initialize(watchdog_ptr);
 * @endcode
 *
 * ## ISR Safety
 *
 * All methods are:
 * - **noexcept**: No exceptions (ISR requirement)
 * - **Non-blocking**: No mutexes or condition variables
 * - **Non-allocating**: No heap operations
 * - **ISR-safe**: Only uses ISR-safe primitives (Watchdog, std::atomic)
 *
 * @note This class cannot be instantiated (all members static)
 * @note Thread-safety: ISR execution is serialized by hardware
 * @note Priority: DMA ISR should have higher priority than UART ISR
 */
class DMAIRQHandler {
  public:
    /**
     * @brief Initialize the DMA interrupt handler
     *
     * Stores references to synchronization primitives that the ISR will use.
     * Must be called before enabling DMA interrupts in NVIC.
     *
     * ## Lifecycle
     *
     * Call this function during application initialization, after creating the
     * Watchdog but before enabling DMA interrupts:
     * @code
     * static Watchdog watchdog;
     * DMAIRQHandler::initialize(&watchdog);
     *
     * // Now safe to enable DMA interrupt
     * NVIC_EnableIRQ(DMA1_Stream0_IRQn);
     * @endcode
     *
     * ## Thread Safety
     *
     * This function is NOT thread-safe and must be called before any interrupts
     * are enabled. Typically called from main() during single-threaded startup.
     *
     * @param watchdog Pointer to Watchdog instance for ISR-to-thread signaling
     *
     * @pre watchdog must remain valid for the lifetime of the program
     * @pre Must be called before enabling DMA interrupts
     * @post ISR can safely access watchdog via stored reference
     *
     * @note Not noexcept because it's called during initialization where exceptions
     *       may be acceptable (startup errors should be fatal)
     */
    static void initialize(sync::Watchdog* watchdog);

    /**
     * @brief DMA interrupt service routine entry point
     *
     * This static method is called by the extern C wrapper `DMA1_Stream0_IRQHandler()`
     * when a DMA1 Stream 0 interrupt fires. It implements the complete ISR logic
     * following Ada's `On_DMA_IRQ` procedure (drivers.adb:199-232).
     *
     * ## ISR Logic
     *
     * 1. **Check transfer error** (DMA ISR.TEIF0):
     *    - If set: disable interrupt, clear flag
     *    - `watchdog_->release(UartStatus::ErrError)`
     *
     * 2. **Check FIFO error** (DMA ISR.FEIF0):
     *    - If set: disable interrupt, clear flag
     *    - `watchdog_->release(UartStatus::ErrError)`
     *
     * 3. **Check direct mode error** (DMA ISR.DMEIF0):
     *    - If set: disable interrupt, clear flag
     *    - `watchdog_->release(UartStatus::ErrError)`
     *
     * 4. **Check transfer complete** (DMA ISR.TCIF0):
     *    - If set: disable interrupt, clear flag
     *    - Invalidate D-cache for received buffer (Cortex-M7 only)
     *    - `watchdog_->release(UartStatus::Ok)`
     *
     * 5. **Clear all serviced flags**:
     *    - Write to DMA IFCR register to clear flags
     *
     * ## Implementation
     *
     * On STM32F429 targets, provides full ISR implementation with DMA register
     * access and Watchdog integration. On non-STM32 targets, provides stub
     * implementation for test/mock environments.
     *
     * @note ISR-safe: Yes (noexcept, non-blocking, non-allocating)
     * @note Called from: extern C DMA1_Stream0_IRQHandler() wrapper
     * @note Context: Interrupt context (cannot call blocking functions)
     *
     * @pre initialize() must have been called
     * @post DMA interrupt flags are cleared
     * @post Cache invalidated if transfer complete (Cortex-M7 only)
     * @post If operation complete, watchdog_->release() called with status
     */
    static void handle_dma_interrupt() noexcept;

  private:
    /**
     * @brief Pointer to Watchdog synchronization primitive
     *
     * The ISR uses the Watchdog to signal completion with status (Decision 26).
     *
     * Initialized by initialize() during application startup.
     * Must remain valid for program lifetime (typically static storage).
     *
     * Thread safety: Only written during initialization (single-threaded),
     * only read from ISR (serialized by hardware).
     */
    static sync::Watchdog* watchdog_;

    // Delete all constructors (static-only class)
    DMAIRQHandler() = delete;
    ~DMAIRQHandler() = delete;
    DMAIRQHandler(const DMAIRQHandler&) = delete;
    DMAIRQHandler& operator=(const DMAIRQHandler&) = delete;
    DMAIRQHandler(DMAIRQHandler&&) = delete;
    DMAIRQHandler& operator=(DMAIRQHandler&&) = delete;
};

} // namespace hm11::stm32

/**
 * @brief Extern C interrupt vector table function declarations
 *
 * These extern "C" functions override the weak symbols defined in the STM32
 * startup code's vector table. They provide the bridge between the hardware
 * interrupt mechanism (which requires C linkage) and the C++ handler classes.
 *
 * ## Vector Table Override Pattern
 *
 * **Startup code (startup_stm32f429xx.s):**
 * @code{.asm}
 * .weak USART5_IRQHandler
 * .thumb_set USART5_IRQHandler,Default_Handler
 * @endcode
 *
 * **Our strong symbol override (irq_handlers.cpp):**
 * @code
 * extern "C" void USART5_IRQHandler() {
 *     hm11::stm32::UARTIRQHandler::handle_uart_interrupt();
 * }
 * @endcode
 *
 * The linker replaces the weak default handler with our strong symbol, making
 * our function the ISR entry point when the interrupt fires.
 *
 * ## Why Extern C?
 *
 * The ARM Cortex-M vector table is an array of function pointers with C linkage.
 * C++ name mangling would prevent the linker from matching our function name with
 * the vector table entry. Extern "C" suppresses name mangling, making the function
 * name match exactly.
 *
 * ## Minimal Wrapper Pattern
 *
 * Each wrapper function does only one thing: call the corresponding static method.
 * This provides:
 * - **Zero overhead**: Single function call instruction
 * - **Clear intent**: Obvious mapping from vector name to handler class
 * - **Maintainability**: Logic lives in C++ class, wrapper is trivial
 *
 * ## Implementation Note
 *
 * These are function declarations only. The implementations are in
 * irq_handlers.cpp to ensure they have C linkage when compiled.
 */
extern "C" {

/**
 * @brief UART5 interrupt vector table entry point
 *
 * This function is called by the ARM Cortex-M exception mechanism when a UART5
 * interrupt is triggered (receive, transmit, error, idle, etc.).
 *
 * Corresponds to vector table offset 0x16C (IRQ 53) on STM32F429.
 *
 * @note This function must have C linkage to override the weak symbol in startup code
 * @note Implementation immediately delegates to UARTIRQHandler::handle_uart_interrupt()
 */
void USART5_IRQHandler();

/**
 * @brief DMA1 Stream 0 interrupt vector table entry point
 *
 * This function is called by the ARM Cortex-M exception mechanism when a DMA1
 * Stream 0 interrupt is triggered (transfer complete, transfer error, FIFO error,
 * direct mode error).
 *
 * Corresponds to vector table offset 0x68 (IRQ 11) on STM32F429.
 *
 * @note This function must have C linkage to override the weak symbol in startup code
 * @note Implementation immediately delegates to DMAIRQHandler::handle_dma_interrupt()
 */
void DMA1_Stream0_IRQHandler();

} // extern "C"
