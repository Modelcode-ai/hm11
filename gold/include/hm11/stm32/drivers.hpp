/**
 * @file drivers.hpp
 * @brief Hardware initialization and driver instance integration for STM32F429
 *
 * This header provides the initialization functions and global driver instance
 * that integrate all STM32 platform components to create a fully functional
 * HM11 driver for the STM32F429 platform.
 *
 * ## Overview
 *
 * This file is the integration point for:
 * - System clock and peripheral clock configuration
 * - GPIO pin configuration for UART5
 * - UART5 peripheral initialization (9600 baud, 8N1)
 * - DMA1 Stream 0 configuration for UART5 RX
 * - Interrupt handler registration (NVIC configuration)
 * - Global driver instance with STM32_UART_DMA and DMA_Receive_Handler
 *
 * ## Architecture
 *
 * This file implements the final integration bringing together:
 * - UARTPort interface and STM32_UART_DMA implementation
 * - IRQ handler classes (UARTIRQHandler, DMAIRQHandler)
 * - STM32_UART_DMA concrete UART port implementation
 * - DMA_Receive_Handler for DMA-based reception
 *
 * ## Ada Correspondence
 *
 * This translates Ada's driver package (drivers.ads:44-85, drivers.adb:375-438):
 * - `Init_UART` procedure → init_uart() function
 * - `Initialize_DMA` procedure → initialize_dma() function
 * - Global `Driver` object → factory function or global instance
 *
 * ## Initialization Sequence
 *
 * Applications must call initialization functions in this order:
 * 1. `init_uart()` - Initialize GPIO pins and UART5 peripheral
 * 2. delay (1 second) - Allow hardware to stabilize
 * 3. `initialize_dma()` - Configure DMA1 Stream 0 for UART5 RX
 * 4. delay (1 second) - Allow DMA to initialize
 * 5. Use driver instance for HM-11 communication
 *
 * ## Hardware Configuration
 *
 * **UART5:**
 * - TX Pin: PC12 (AF7, push-pull, pull-up, 50MHz)
 * - RX Pin: PD2 (AF7, pull-up)
 * - Baud rate: 9600
 * - Data format: 8N1 (8 data bits, no parity, 1 stop bit)
 * - Flow control: None
 *
 * **DMA1 Stream 0:**
 * - Channel: Channel 4 (UART5_RX)
 * - Direction: Peripheral-to-memory
 * - Data size: Byte (8-bit)
 * - Mode: Normal (for blocking) / Circular (for streaming)
 * - Priority: Very High
 * - FIFO: Disabled (direct mode)
 *
 * **Interrupts:**
 * - UART5: Priority 6, enabled for RX, errors, idle line
 * - DMA1 Stream 0: Priority 5, enabled for TC, TE, FIFO, DME
 *
 * ## Memory Layout
 *
 * The driver instance is created with proper alignment for DMA operations:
 * - Internal receive buffers: alignas(32) for cache line alignment
 * - Watchdog state: atomic variables for ISR-safe access
 * - Circular buffer: atomic write position for lock-free SPSC
 *
 * ## Usage Example
 *
 * @code
 * #include <hm11/stm32/drivers.hpp>
 * #include <chrono>
 * #include <thread>
 *
 * using namespace hm11::stm32;
 *
 * int main() {
 *     // Initialize hardware
 *     init_uart();
 *     std::this_thread::sleep_for(std::chrono::seconds(1));
 *
 *     initialize_dma();
 *     std::this_thread::sleep_for(std::chrono::seconds(1));
 *
 *     // Get driver instance
 *     auto& driver = get_hm11_driver();
 *
 *     // Use driver
 *     hal::UartStatus status;
 *     driver.test(status);
 *     if (status == hal::UartStatus::Ok) {
 *         // Module is responding
 *     }
 *
 *     // Driver is automatically cleaned up on scope exit
 *     return 0;
 * }
 * @endcode
 *
 * ## Interrupt Registration (Decision 16)
 *
 * The initialization functions register interrupt handlers with NVIC:
 * - Enable UART5 interrupt (NVIC_EnableIRQ)
 * - Enable DMA1 Stream 0 interrupt (NVIC_EnableIRQ)
 * - Set interrupt priorities (DMA higher than UART)
 * - Initialize handler static members with Watchdog reference
 *
 * ## RAII Ownership (Decision 22)
 *
 * The driver instance follows unique ownership semantics:
 * - Non-copyable, non-movable
 * - Lifetime tied to application scope
 * - Automatic cleanup on scope exit
 * - For bare-metal applications, global or static storage is acceptable
 *
 * ## Implementation Notes
 *
 * - All register access uses CMSIS device headers (Decision 30)
 * - No STM32 HAL library dependency
 * - Clock configuration assumes default reset state (HSI, no PLL)
 * - APB1 clock frequency: 16 MHz (HSI divided by 1)
 * - For different clock configurations, adjust BRR calculation
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 *
 * This file is part of the HM11 Bluetooth Low Energy driver, migrated from
 * the Ada implementation to modern C++20.
 */

#pragma once

#include <cstdint>

#include "hm11/hal/base_types.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/stm32/dma_receive_handler.hpp"
#include "hm11/stm32/irq_handlers.hpp"
#include "hm11/stm32/uart_port_impl.hpp"
#include "hm11/sync/watchdog.hpp"

namespace hm11::stm32 {

/**
 * @brief Initialize UART5 GPIO pins and peripheral
 *
 * Configures UART5 for communication with the HM-11 Bluetooth module.
 * This function performs the complete UART initialization sequence matching
 * Ada's Init_UART procedure (drivers.adb:378-408).
 *
 * ## Initialization Steps
 *
 * 1. **Enable peripheral clocks** (RCC):
 *    - GPIOC clock (AHB1ENR.GPIOCEN)
 *    - GPIOD clock (AHB1ENR.GPIODEN)
 *    - UART5 clock (APB1ENR.UART5EN)
 *
 * 2. **Configure GPIO pins**:
 *    - PC12 (UART5_TX): Alternate function AF7, push-pull, pull-up, 50MHz
 *    - PD2 (UART5_RX): Alternate function AF7, pull-up
 *
 * 3. **Configure UART5 peripheral**:
 *    - Disable UART (CR1.UE = 0)
 *    - Set baud rate: 9600 (BRR = APB1_CLK / (16 * 9600))
 *    - Set data format: 8N1 (CR1.M = 0, CR2.STOP = 00)
 *    - Enable TX and RX (CR1.TE, CR1.RE)
 *    - Enable error interrupts (CR3.EIE)
 *    - Enable parity error interrupt (CR1.PEIE)
 *    - Enable RXNE interrupt (CR1.RXNEIE)
 *    - Enable UART (CR1.UE = 1)
 *
 * ## Hardware Requirements
 *
 * - STM32F429 with UART5 peripheral
 * - GPIOC and GPIOD available
 * - APB1 clock running (default 16 MHz from HSI)
 *
 * ## Baud Rate Calculation
 *
 * For HSI 16 MHz (default after reset):
 * - APB1_CLK = HSI / APB1_DIV = 16 MHz / 1 = 16 MHz
 * - BRR = 16000000 / (16 * 9600) = 104.166... ≈ 104 (0x68)
 *
 * For PLL 180 MHz configuration (typical):
 * - APB1_CLK = 45 MHz (HCLK / 4)
 * - BRR = 45000000 / (16 * 9600) = 293.0 ≈ 293 (0x125)
 *
 * @note This function must be called before initialize_dma()
 * @note Not idempotent - calling multiple times may cause configuration issues
 * @note Blocking operation - performs register writes with delays
 * @note Not ISR-safe - must be called from application context
 *
 * @post UART5 peripheral is configured and enabled
 * @post GPIO pins PC12 and PD2 are configured for UART5
 * @post UART interrupts are enabled (error, parity, RXNE)
 */
void init_uart() noexcept;

/**
 * @brief Initialize DMA1 Stream 0 for UART5 reception
 *
 * Configures DMA1 Stream 0 Channel 4 for UART5 RX data transfers.
 * This function performs the complete DMA initialization sequence matching
 * Ada's Initialize_DMA procedure (drivers.adb:414-438).
 *
 * ## Initialization Steps
 *
 * 1. **Enable DMA1 clock** (RCC):
 *    - DMA1 clock (AHB1ENR.DMA1EN)
 *
 * 2. **Reset DMA1 Stream 0**:
 *    - Disable stream (CR.EN = 0)
 *    - Wait until stream is disabled (CR.EN reads 0)
 *    - Clear all interrupt flags (LIFCR)
 *
 * 3. **Configure DMA1 Stream 0**:
 *    - Channel: 4 (UART5_RX)
 *    - Direction: Peripheral-to-memory (CR.DIR = 00)
 *    - Peripheral address: USART5->DR
 *    - Memory address: (set per transfer, not configured here)
 *    - Data size: Byte (CR.PSIZE = 00, CR.MSIZE = 00)
 *    - Mode: Normal (CR.CIRC = 0, changed to Circular for streaming)
 *    - Priority: Very High (CR.PL = 11)
 *    - Increment: Memory yes, peripheral no
 *    - FIFO: Disabled (FCR.DMDIS = 0, direct mode)
 *
 * 4. **Enable DMA requests on UART5**:
 *    - Enable UART5 DMA receive requests (CR3.DMAR = 1)
 *    - Pause DMA reception initially (wait for operation start)
 *
 * 5. **Configure NVIC**:
 *    - Enable DMA1 Stream 0 interrupt (NVIC_EnableIRQ)
 *    - Enable UART5 interrupt (NVIC_EnableIRQ)
 *    - Set interrupt priorities (DMA priority 5, UART priority 6)
 *    - Initialize IRQ handler static members with Watchdog reference
 *
 * ## Hardware Requirements
 *
 * - init_uart() must be called first
 * - DMA1 peripheral available
 * - UART5 peripheral configured and enabled
 *
 * ## DMA Stream Configuration
 *
 * DMA1 Stream 0 is dedicated to UART5 RX in the STM32F429 DMA mapping:
 * - Stream: 0
 * - Channel: 4
 * - Request: UART5_RX
 *
 * ## Interrupt Configuration
 *
 * This function also configures the NVIC for UART and DMA interrupts:
 * - **DMA1_Stream0_IRQn**: Priority 5 (higher priority, time-critical)
 * - **UART5_IRQn**: Priority 6 (lower priority, can be interrupted by DMA)
 *
 * Lower numbers indicate higher priority in ARM Cortex-M NVIC (0-15 range).
 *
 * @note This function must be called after init_uart()
 * @note Not idempotent - calling multiple times may cause configuration issues
 * @note Blocking operation - performs register writes with delays
 * @note Not ISR-safe - must be called from application context
 *
 * @pre init_uart() has been called
 * @post DMA1 Stream 0 is configured for UART5 RX transfers
 * @post DMA and UART interrupts are enabled in NVIC
 * @post IRQ handlers are initialized with Watchdog reference
 */
void initialize_dma() noexcept;

/**
 * @brief Get reference to the global HM11 driver instance
 *
 * Returns a reference to the global HM11Driver instance that is initialized
 * after calling init_uart() and initialize_dma(). The driver uses the
 * STM32_UART_DMA port implementation and DMA_Receive_Handler strategy.
 *
 * ## Driver Configuration
 *
 * The driver is instantiated with:
 * - **UARTImpl**: STM32_UART_DMA<DMA_Receive_Handler>
 * - **ReceiveHandler**: DMA_Receive_Handler
 * - Internal Watchdog for synchronization
 * - DMA_Receive_Handler configured with Watchdog reference
 *
 * ## Initialization Requirements
 *
 * Before calling this function:
 * 1. Call init_uart() to initialize UART5 hardware
 * 2. Wait at least 1 second for hardware stabilization
 * 3. Call initialize_dma() to configure DMA and interrupts
 * 4. Wait at least 1 second for DMA initialization
 *
 * ## Lifetime
 *
 * The driver instance has static storage duration (lives for program lifetime).
 * It will be automatically destroyed on program termination with graceful cleanup.
 *
 * ## Thread Safety
 *
 * The driver instance is NOT thread-safe. Applications must serialize access
 * if using from multiple threads.
 *
 * ## Example Usage
 *
 * @code
 * // Initialize hardware
 * init_uart();
 * std::this_thread::sleep_for(std::chrono::seconds(1));
 * initialize_dma();
 * std::this_thread::sleep_for(std::chrono::seconds(1));
 *
 * // Get driver and use it
 * auto& driver = get_hm11_driver();
 * hal::UartStatus status;
 * driver.test(status);
 * @endcode
 *
 * @return Reference to the global HM11Driver instance
 *
 * @pre init_uart() and initialize_dma() have been called
 * @post Returns valid reference to driver instance
 *
 * @note The returned reference remains valid for program lifetime
 * @note Not ISR-safe - driver methods must be called from application context
 */
HM11Driver<STM32_UART_DMA<DMA_Receive_Handler>, DMA_Receive_Handler>& get_hm11_driver() noexcept;

/**
 * @brief Type alias for the STM32F429 HM11 driver instance type
 *
 * Convenience alias for the fully-qualified driver type with STM32
 * platform implementations. Use this to declare driver variables or
 * references in application code.
 *
 * ## Example Usage
 *
 * @code
 * // Function that takes driver reference
 * void configure_bluetooth(STM32_HM11_Driver& driver) {
 *     hal::UartStatus status;
 *     driver.set_role(types::Role::Central, status);
 * }
 *
 * // Use with global driver instance
 * configure_bluetooth(get_hm11_driver());
 * @endcode
 */
using STM32_HM11_Driver = HM11Driver<STM32_UART_DMA<DMA_Receive_Handler>, DMA_Receive_Handler>;

/**
 * @brief Get reference to UART port for direct data transfer
 *
 * Returns a reference to the STM32_UART_DMA port implementation used by
 * the HM11 driver. This allows applications to perform direct UART data
 * transfers outside of AT command processing, matching Ada's pattern where
 * the example calls DMA_Receive_Handler directly for echo operations.
 *
 * ## Use Cases
 *
 * - Echo server: Receive and transmit user data bypassing AT command layer
 * - Raw data transfer: Send/receive binary data after BLE connection
 * - Custom protocols: Implement application-specific UART protocols
 *
 * ## Example Usage
 *
 * @code
 * // Get UART port and receive handler
 * auto& uart_port = get_uart_port();
 * auto& receive_handler = get_receive_handler();
 *
 * // Receive data directly (bypass driver)
 * uint8_t buffer[128];
 * hal::UartStatus status;
 * hal::UartData8b data{buffer, sizeof(buffer)};
 * receive_handler(uart_port, data, status, 5000, false);
 *
 * // Transmit data directly
 * uart_port.transmit(data, status);
 * @endcode
 *
 * ## Ada Correspondence
 *
 * Matches Ada's pattern where UART and DMA_Receive_Handler are exposed
 * in Drivers package (drivers.ads:59, 63) for direct use in examples.
 *
 * @return Reference to STM32_UART_DMA port instance
 *
 * @pre init_uart() and initialize_dma() have been called
 * @post Returns valid reference to UART port
 *
 * @note The returned reference remains valid for program lifetime
 * @note Not ISR-safe - must be called from application context
 */
STM32_UART_DMA<DMA_Receive_Handler>& get_uart_port() noexcept;

/**
 * @brief Get reference to DMA receive handler for direct reception
 *
 * Returns a reference to the DMA_Receive_Handler instance used by the
 * HM11 driver. This allows applications to perform direct UART reception
 * with DMA, matching Ada's pattern where DMA_Receive_Handler is exposed
 * as a procedure in the Drivers package.
 *
 * ## Use Cases
 *
 * - Connection monitoring: Poll for "OK+CONN" notification messages
 * - Echo server: Receive user data for immediate echo back
 * - Disconnection detection: Wait for "OK+LOST" messages
 *
 * ## Example Usage
 *
 * @code
 * // Wait for connection notification
 * auto& uart_port = get_uart_port();
 * auto& receive_handler = get_receive_handler();
 *
 * std::array<uint8_t, 7> buffer{};  // Size of "OK+CONN"
 * hal::UartStatus status;
 * hal::UartData8b data{buffer.data(), buffer.size()};
 *
 * while (true) {
 *     receive_handler(uart_port, data, status, 120000, false);
 *     if (status == hal::UartStatus::Ok) {
 *         std::string_view msg{reinterpret_cast<const char*>(buffer.data()), buffer.size()};
 *         if (msg == "OK+CONN") {
 *             // Connected!
 *             break;
 *         }
 *     }
 * }
 * @endcode
 *
 * ## Ada Correspondence
 *
 * Matches Ada's DMA_Receive_Handler procedure (drivers.ads:63-69) exposed
 * for direct use in hm11_example.adb lines 179-180, 200-202.
 *
 * @return Reference to DMA_Receive_Handler instance
 *
 * @pre init_uart() and initialize_dma() have been called
 * @post Returns valid reference to receive handler
 *
 * @note The returned reference remains valid for program lifetime
 * @note Not ISR-safe - operator() must be called from application context
 */
DMA_Receive_Handler& get_receive_handler() noexcept;

} // namespace hm11::stm32
