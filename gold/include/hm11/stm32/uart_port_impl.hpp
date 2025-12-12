/**
 * @file uart_port_impl.hpp
 * @brief STM32F429 UART5 implementation using direct CMSIS register access
 *
 * This header implements the UARTPort interface concept using direct register
 * access via CMSIS device headers, without relying on the STM32 HAL library.
 * This approach follows **Decision 30** (STM32 Platform Implementation Approach)
 * for minimal overhead and maximum control.
 *
 * ## Overview
 *
 * The STM32_UART_DMA class provides a concrete implementation of the UARTPort
 * concept for STM32F429 hardware. It configures and controls UART5 peripheral
 * connected to the HM-11 Bluetooth module.
 *
 * **Hardware Configuration:**
 * - UART5 peripheral (APB1 bus)
 * - GPIO PC12 (TX) - connects to HM-11 RX (pin 4)
 * - GPIO PD2 (RX) - connects to HM-11 RX (pin 2)
 * - Baud rate: 9600 (HM-11 default)
 * - Data format: 8 data bits, no parity, 1 stop bit (8N1)
 * - DMA Stream 0 (Channel 4) for reception
 *
 * ## Architecture (Decision 1: Dependency Injection)
 *
 * This class satisfies the UARTPort concept and can be injected into HM11Driver
 * as a template parameter for compile-time polymorphism:
 *
 * @code
 * STM32_UART_DMA uart_port(receive_handler);
 * HM11Driver<STM32_UART_DMA, DMAReceiveHandler> driver(uart_port, receive_handler);
 * @endcode
 *
 * ## Direct Register Access (Decision 30)
 *
 * This implementation uses CMSIS device headers for register access:
 * - USART5: UART5 peripheral registers (USART_TypeDef)
 * - RCC: Clock enable registers for UART5, GPIOC, GPIOD
 * - GPIOC: PC12 pin configuration for TX
 * - GPIOD: PD2 pin configuration for RX
 *
 * Register access matches the Ada driver's low-level approach (drivers.adb:375-408)
 * but uses CMSIS structs instead of Ada's record overlays.
 *
 * ## Memory Safety (Decision 20)
 *
 * Transmit buffers are cleaned via cache operations before DMA transfer (Cortex-M7).
 * Receive buffers are invalidated after DMA completion (handled in DMA ISR).
 *
 * ## RAII Ownership (Decision 22)
 *
 * The class follows unique ownership semantics:
 * - Non-copyable, non-movable (deleted copy/move operations)
 * - Owns UART5 hardware resource for its lifetime
 * - Constructor initializes hardware
 * - Destructor gracefully stops ongoing operations
 *
 * ## ISR Integration (Decision 16, 25, 26)
 *
 * The class works with interrupt handlers:
 * - ISR handlers registered separately via UARTIRQHandler/DMAIRQHandler
 * - Timeout checked in ISR (Decision 25: ISR-side deadline polling)
 * - Errors propagated via atomic status in Watchdog (Decision 26)
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/stm32/uart_port_impl.hpp>
 * #include <hm11/hm11_driver.hpp>
 *
 * // Create DMA receive handler
 * DMAReceiveHandler dma_handler;
 *
 * // Create UART port (initializes hardware)
 * STM32_UART_DMA uart_port(dma_handler);
 *
 * // Create HM11 driver with UART port
 * HM11Driver<STM32_UART_DMA, DMAReceiveHandler> driver(uart_port, dma_handler);
 *
 * // Use driver...
 * UartStatus status;
 * driver.test(status);
 * @endcode
 *
 * ## Reference
 *
 * Ada implementation: examples/shared/hm11/src/drivers.adb:375-408 (Init_UART)
 * STM32F429 Reference Manual: RM0090 Section 30 (USART)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "hm11/hal/base_types.hpp"
#include "hm11/hal/uart_port.hpp"
#include "hm11/platform/cache_ops.hpp"

// CMSIS device header for STM32F429
// This provides USART_TypeDef, GPIO_TypeDef, RCC_TypeDef, etc.
#if defined(STM32F429xx)
#include "stm32f429xx.h"
#elif defined(STM32F4)
#include "stm32f4xx.h"
#else
// For testing/compilation without target, provide forward declarations
struct USART_TypeDef;
struct GPIO_TypeDef;
struct DMA_Stream_TypeDef;
#endif

namespace hm11::stm32 {

/**
 * @brief STM32F429 UART5 port implementation with DMA reception
 *
 * This class implements the UARTPort concept using direct CMSIS register access
 * to control UART5 on STM32F429. It provides blocking transmit and DMA-based
 * receive operations for communication with the HM-11 Bluetooth module.
 *
 * ## Hardware Configuration
 *
 * **UART5 (APB1 Bus):**
 * - Baud rate: 9600
 * - Data bits: 8
 * - Parity: None
 * - Stop bits: 1
 * - Flow control: None
 *
 * **GPIO Pins:**
 * - PC12: UART5_TX (AF7, push-pull, pull-up, 50MHz)
 * - PD2: UART5_RX (AF7, pull-up)
 *
 * **DMA:**
 * - DMA1 Stream 0, Channel 4 for RX
 *
 * ## Template Parameters
 *
 * @tparam ReceiveHandler Callable type satisfying IsReceiveHandler concept.
 *         Must provide operator() for handling receive operations with DMA.
 *         Injected via constructor (Decision 1: Dependency Injection).
 *
 * ## Initialization Sequence
 *
 * The constructor performs these steps (matching Ada's Init_UART, drivers.adb:378-408):
 * 1. Enable clocks: RCC->APB1ENR (UART5), RCC->AHB1ENR (GPIOC, GPIOD)
 * 2. Configure GPIO pins:
 *    - Set mode to alternate function (MODER)
 *    - Set alternate function to AF7 (UART5) (AFR)
 *    - Configure pull-up resistors (PUPDR)
 *    - Set output type to push-pull for TX (OTYPER)
 *    - Set speed to 50MHz (OSPEEDR)
 * 3. Configure UART5:
 *    - Disable UART (CR1.UE = 0)
 *    - Set baud rate (BRR register, based on APB1 clock)
 *    - Set 8N1 format (CR1: M=0 for 8-bit, CR2: STOP=00 for 1 stop bit)
 *    - Enable TX and RX (CR1.TE, CR1.RE)
 *    - Enable error interrupts (CR3)
 *    - Enable UART (CR1.UE = 1)
 *
 * ## UART Register Access (STM32F429 Reference Manual RM0090)
 *
 * Key registers used (offsets from UART5 base address):
 * - SR (0x00): Status register (TXE, RXNE, errors)
 * - DR (0x04): Data register (transmit/receive data)
 * - BRR (0x08): Baud rate register
 * - CR1 (0x0C): Control register 1 (enable, word length, parity)
 * - CR2 (0x10): Control register 2 (stop bits)
 * - CR3 (0x14): Control register 3 (DMA enable, error interrupts)
 *
 * ## Baud Rate Calculation
 *
 * BRR register value = (APB1_Clock / (16 * baud_rate))
 * For 9600 baud with APB1 = 42 MHz:
 *   BRR = 42000000 / (16 * 9600) = 273.4375
 *   BRR = 0x111 (273 decimal)
 *
 * ## Thread Safety
 *
 * This class is NOT thread-safe. Serialize access if used from multiple threads.
 * Typical usage: single-threaded or protected by HM11Driver's access control.
 *
 * ## ISR Safety
 *
 * Methods are NOT ISR-safe (they block). Call only from application context.
 * ISR handlers (UARTIRQHandler, DMAIRQHandler) access hardware directly.
 *
 * @note RAII: Constructor initializes hardware, destructor disables interrupts
 * @note Non-copyable, non-movable (Decision 22)
 */
template <typename ReceiveHandler> class STM32_UART_DMA {
  public:
    /**
     * @brief Construct and initialize UART5 hardware
     *
     * Initializes UART5 peripheral, GPIO pins, and stores reference to the
     * receive handler for delegation of receive operations.
     *
     * ## Initialization Steps
     *
     * 1. Enable peripheral clocks (RCC)
     * 2. Configure GPIO alternate functions
     * 3. Configure UART5 parameters (9600 8N1)
     * 4. Enable UART5 peripheral
     * 5. Store receive handler reference
     *
     * ## Hardware Dependencies
     *
     * Requires:
     * - STM32F429 CMSIS device header (defines UART5, GPIOC, GPIOD, RCC)
     * - APB1 clock running (default 42 MHz after reset)
     * - GPIO ports C and D available
     *
     * @param receive_handler Reference to receive handler callable object.
     *        Must remain valid for the lifetime of this UART port instance.
     *
     * @pre APB1 clock is configured and running
     * @pre GPIOC and GPIOD ports are not in use by other peripherals
     * @post UART5 is configured and ready for transmit/receive operations
     * @post GPIO pins PC12 and PD2 are configured for UART5 function
     *
     * @throws None (noexcept would be ideal but omitted for compatibility)
     *
     * @note This constructor performs hardware initialization with blocking
     *       register writes. Not suitable for ISR context.
     */
    explicit STM32_UART_DMA(ReceiveHandler& receive_handler);

    /**
     * @brief Destructor - gracefully stop UART operations
     *
     * Disables UART5 interrupts and peripheral to prevent spurious interrupts
     * during cleanup. Follows Decision 22 (RAII Ownership).
     *
     * ## Cleanup Steps
     *
     * 1. Disable UART interrupts (CR3)
     * 2. Disable UART peripheral (CR1.UE = 0)
     * 3. GPIO pins remain configured (safe for future initialization)
     *
     * @note Does not disable clocks (they may be used by other peripherals)
     * @note Does not reconfigure GPIO pins to input mode
     */
    ~STM32_UART_DMA() noexcept;

    // Delete copy and move operations (Decision 22: RAII Ownership Model)
    STM32_UART_DMA(const STM32_UART_DMA&) = delete;
    STM32_UART_DMA& operator=(const STM32_UART_DMA&) = delete;
    STM32_UART_DMA(STM32_UART_DMA&&) = delete;
    STM32_UART_DMA& operator=(STM32_UART_DMA&&) = delete;

    /**
     * @brief Query the configured UART data size
     *
     * Returns the data bit width configuration for this UART port. For HM-11,
     * this is always 8-bit mode as required by the hardware specification.
     *
     * This method satisfies the UARTPort concept requirement.
     *
     * @return UartDataSize::DataSize8b (always 8-bit for HM-11)
     *
     * @note Constexpr for compile-time optimization
     * @note noexcept for exception safety
     */
    [[nodiscard]] constexpr hal::UartDataSize data_size() const noexcept { return hal::UartDataSize::DataSize8b; }

    /**
     * @brief Transmit data buffer via UART5
     *
     * Transmits the provided data buffer using blocking polling of the TXE flag.
     * This method writes bytes one-by-one to the UART5 DR register and waits
     * for each byte to be transmitted before writing the next.
     *
     * ## Transmit Algorithm
     *
     * For each byte in the buffer:
     * 1. Wait for TXE flag (transmit data register empty)
     * 2. Write byte to DR register
     * 3. Check for timeout
     * 4. Check for errors (framing, parity, overrun)
     *
     * After all bytes sent:
     * 5. Wait for TC flag (transmission complete)
     * 6. Return status
     *
     * ## Timeout Implementation
     *
     * Uses std::chrono::steady_clock to calculate absolute deadline:
     * - deadline = now + timeout_ms
     * - On each TXE check: if (now > deadline) return ErrTimeout
     *
     * This matches Ada's Clock + Timeout pattern (drivers.adb:282).
     *
     * ## Cache Coherency (Decision 20)
     *
     * For Cortex-M7 devices, the data cache is cleaned before transmission
     * to ensure UART reads the latest data from memory. This is a no-op on
     * Cortex-M4 (STM32F429).
     *
     * ## Status Codes
     *
     * - Ok: All data transmitted successfully
     * - ErrTimeout: Timeout expired before transmission completed
     * - ErrError: UART hardware error detected (framing, parity, overrun)
     *
     * @param data Read-only buffer containing data to transmit
     * @param status Output parameter for operation result
     * @param timeout_ms Maximum time to wait for transmission (milliseconds)
     *
     * @pre data_size() == UartDataSize::DataSize8b (interface contract)
     * @pre data buffer remains valid for duration of call
     * @post status contains operation result (Ok, ErrTimeout, or ErrError)
     * @post On success: all bytes transmitted
     * @post On error: partial transmission possible
     *
     * @note Blocking operation - waits until completion or timeout
     * @note Not ISR-safe (uses blocking delays)
     * @note Thread-unsafe (caller must serialize access)
     */
    void transmit(hal::UartData8bConst data, hal::UartStatus& status, uint32_t timeout_ms = 1000) noexcept;

    /**
     * @brief Receive data from UART5 via DMA handler
     *
     * Delegates to the injected receive handler for DMA-based reception.
     * The receive handler manages DMA configuration, buffer management,
     * and blocking/streaming mode selection.
     *
     * ## Delegation Pattern
     *
     * This method is a thin wrapper that calls the receive handler:
     * @code
     * receive_handler_(*this, data, status, timeout_ms, as_stream);
     * @endcode
     *
     * The handler can access UART5 registers via this object if needed for
     * configuration or status checking.
     *
     * ## Receive Modes
     *
     * **Blocking Mode** (as_stream = false):
     * - Wait until buffer filled or timeout
     * - Used for AT command responses
     *
     * **Streaming Mode** (as_stream = true):
     * - Start continuous reception without blocking
     * - Used for discovery scanning
     * - Data arrives in circular buffer
     *
     * ## Timeout Semantics (Decision 25, 28)
     *
     * Timeout uses ISR-side deadline polling where the ISR checks if
     * Clock > Until_Time opportunistically. For streaming operations,
     * timeout is soft (operation may exceed timeout by current response
     * parse time).
     *
     * ## Status Codes
     *
     * - Ok: Data received successfully
     * - ErrTimeout: Timeout expired before reception completed
     * - ErrError: UART or DMA hardware error
     *
     * @param data Mutable buffer to store received data
     * @param status Output parameter for operation result
     * @param timeout_ms Maximum time to wait for data (milliseconds)
     * @param as_stream Enable streaming mode (non-blocking continuous reception)
     *
     * @pre data_size() == UartDataSize::DataSize8b (interface contract)
     * @pre data buffer remains valid for duration of call
     * @pre receive_handler_ reference is valid
     * @post status contains operation result
     * @post In blocking mode: data buffer filled or timeout/error occurred
     * @post In streaming mode: continuous reception started
     *
     * @note Blocking in blocking mode, non-blocking in streaming mode
     * @note Not ISR-safe (handler may block or configure DMA)
     * @note Thread-unsafe (caller must serialize access)
     */
    void
    receive(hal::UartData8b data, hal::UartStatus& status, uint32_t timeout_ms = 1000, bool as_stream = false) noexcept;

  private:
    /**
     * @brief Configure GPIO pins for UART5 function
     *
     * Configures PC12 (TX) and PD2 (RX) as alternate function pins for UART5.
     * This matches Ada's Configure_IO call (drivers.adb:386-392).
     *
     * ## Pin Configuration
     *
     * **PC12 (UART5_TX):**
     * - Mode: Alternate Function (MODER = 10b)
     * - Alternate Function: AF7 (UART5) (AFRH = 0111b)
     * - Output Type: Push-Pull (OTYPER = 0)
     * - Pull: Pull-Up (PUPDR = 01b)
     * - Speed: 50 MHz (OSPEEDR = 10b)
     *
     * **PD2 (UART5_RX):**
     * - Mode: Alternate Function (MODER = 10b)
     * - Alternate Function: AF7 (UART5) (AFRL = 0111b)
     * - Pull: Pull-Up (PUPDR = 01b)
     *
     * ## Register Access (Reference Manual RM0090 Section 8.4)
     *
     * GPIO registers used (offset from GPIOC/GPIOD base):
     * - MODER (0x00): Pin mode register (input, output, AF, analog)
     * - OTYPER (0x04): Output type register (push-pull, open-drain)
     * - OSPEEDR (0x08): Output speed register (2, 25, 50, 100 MHz)
     * - PUPDR (0x0C): Pull-up/pull-down register
     * - AFR[0]/AFR[1] (0x20/0x24): Alternate function registers (low/high)
     *
     * @note Called from constructor only
     * @note Requires GPIOC and GPIOD clocks enabled (RCC->AHB1ENR)
     *
     * @post PC12 configured as UART5_TX
     * @post PD2 configured as UART5_RX
     */
    void configure_gpio() noexcept;

    /**
     * @brief Configure UART5 peripheral parameters
     *
     * Configures UART5 for 9600 baud, 8N1 format, and enables the peripheral.
     * This matches Ada's UART configuration (drivers.adb:394-407).
     *
     * ## Configuration Steps
     *
     * 1. Disable UART (CR1.UE = 0) for configuration
     * 2. Set baud rate (BRR = APB1_CLK / (16 * 9600))
     * 3. Set word length to 8 bits (CR1.M = 0)
     * 4. Set parity to none (CR1.PCE = 0)
     * 5. Set stop bits to 1 (CR2.STOP = 00b)
     * 6. Enable transmitter (CR1.TE = 1)
     * 7. Enable receiver (CR1.RE = 1)
     * 8. Enable error interrupts (CR3.EIE = 1)
     * 9. Enable parity error interrupt (CR1.PEIE = 1)
     * 10. Enable RXNE interrupt (CR1.RXNEIE = 1)
     * 11. Enable UART (CR1.UE = 1)
     *
     * ## Baud Rate Calculation
     *
     * BRR = UART_CLK / (16 * baud_rate)
     *
     * For STM32F429 with default clocks:
     * - APB1_CLK = 42 MHz (HCLK / 4, HCLK = 168 MHz)
     * - baud_rate = 9600
     * - BRR = 42000000 / (16 * 9600) = 273.4375 ≈ 273 (0x111)
     *
     * BRR register format (RM0090 Section 30.6.3):
     * - Bits 15:4: Mantissa (DIV_Mantissa)
     * - Bits 3:0: Fraction (DIV_Fraction)
     *
     * ## UART Registers (RM0090 Section 30.6)
     *
     * - CR1 (0x0C): Control register 1 (enable, interrupts, word length)
     * - CR2 (0x10): Control register 2 (stop bits, clock config)
     * - CR3 (0x14): Control register 3 (DMA, flow control, error interrupts)
     * - BRR (0x08): Baud rate register (mantissa and fraction)
     *
     * @note Called from constructor only
     * @note Requires UART5 clock enabled (RCC->APB1ENR)
     * @note Assumes APB1 clock frequency is 42 MHz (default after reset)
     *
     * @post UART5 configured for 9600 baud, 8N1 format
     * @post UART5 transmitter and receiver enabled
     * @post UART5 peripheral enabled and ready for operation
     */
    void configure_uart() noexcept;

    /**
     * @brief Calculate BRR register value for baud rate
     *
     * Calculates the USART BRR (Baud Rate Register) value based on the
     * APB1 clock frequency and desired baud rate.
     *
     * ## Formula
     *
     * BRR = UART_CLK / (16 * baud_rate)
     *
     * where UART_CLK is the APB1 clock frequency.
     *
     * ## Example
     *
     * For APB1_CLK = 42 MHz and baud_rate = 9600:
     * BRR = 42000000 / (16 * 9600) = 273.4375
     *
     * The fractional part is handled by the BRR register's 4 fraction bits.
     *
     * @param apb1_clock_hz APB1 clock frequency in Hz (typically 42 MHz)
     * @param baud_rate Desired baud rate (typically 9600 for HM-11)
     * @return BRR register value (mantissa and fraction combined)
     *
     * @note This is a helper function for configure_uart()
     * @note Result is rounded to nearest integer
     */
    static constexpr uint32_t calculate_brr(uint32_t apb1_clock_hz, uint32_t baud_rate) noexcept {
        // BRR = UART_CLK / (16 * baud_rate)
        // Add (16 * baud_rate / 2) for rounding
        return (apb1_clock_hz + (8U * baud_rate)) / (16U * baud_rate);
    }

    /**
     * @brief Check UART status register for errors
     *
     * Checks the UART5 status register for error flags (framing error,
     * parity error, noise error, overrun error).
     *
     * ## Error Flags (SR Register, RM0090 Section 30.6.1)
     *
     * - PE (bit 0): Parity error
     * - FE (bit 1): Framing error
     * - NF (bit 2): Noise detected flag
     * - ORE (bit 3): Overrun error
     *
     * @return true if any error flag is set, false otherwise
     *
     * @note Used during transmit/receive operations to detect errors
     * @note Caller should clear error flags after detecting them
     */
    bool has_uart_error() const noexcept;

    /**
     * @brief Clear UART error flags
     *
     * Clears all error flags in the UART5 status register by reading SR
     * followed by reading DR (standard STM32 error clearing sequence).
     *
     * ## Error Clearing Sequence (RM0090 Section 30.6.1)
     *
     * Most UART flags are cleared by:
     * 1. Read SR register
     * 2. Read DR register
     *
     * This sequence is required for: PE, FE, NF, ORE flags.
     *
     * @note Called after detecting errors to prepare for next operation
     */
    void clear_uart_errors() noexcept;

    /// Reference to receive handler for delegation (Decision 1: Dependency Injection)
    ReceiveHandler& receive_handler_;

    // Note: We don't store pointers to USART5, GPIOC, GPIOD, etc. because
    // they are available globally via CMSIS headers. This reduces object size
    // and matches Ada's direct register access pattern.
};

// ============================================================================
// Static Assertions (Concept Verification)
// ============================================================================

/**
 * @brief Verify STM32_UART_DMA satisfies UARTPort concept
 *
 * This static assertion ensures that STM32_UART_DMA implements all required
 * methods specified by the UARTPort concept at compile time.
 *
 * If this assertion fails, the class is missing required methods or has
 * incorrect method signatures.
 *
 * @note Requires ReceiveHandler to be a complete type for instantiation
 * @note This verification happens at template instantiation time
 */
// Deferred to instantiation since STM32_UART_DMA is a template
// User code should include:
// static_assert(hm11::hal::IS_UART_PORT_V<STM32_UART_DMA<YourHandler>>,
//               "STM32_UART_DMA must satisfy UARTPort concept");

} // namespace hm11::stm32

// Include implementation (if separating declaration and implementation)
// For now, we'll include implementation inline below for simplicity
#include "uart_port_impl.inl"
