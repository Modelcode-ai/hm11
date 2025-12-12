/**
 * @file interrupt_handlers.hpp
 * @brief STM32 interrupt handler classes for UART and DMA
 * @details Implements Decision 16 by encapsulating interrupt handlers in C++ classes
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides C++ classes that encapsulate STM32 interrupt handlers for UART and DMA,
 * with static methods that implement the interrupt logic and extern "C" wrapper functions
 * that are referenced in the startup vector table.
 */

#ifndef HM11_PLATFORM_STM32_INTERRUPT_HANDLERS_HPP
#define HM11_PLATFORM_STM32_INTERRUPT_HANDLERS_HPP

#include <chrono>
#include <cstdint>
#include <hm11/platform/stm32/device.hpp>
#include <hm11/platform/stm32/dma.hpp>
#include <hm11/platform/stm32/uart.hpp>
#include <hm11/sync/binary_semaphore.hpp>
#include <hm11/sync/uart_status.hpp>

namespace hm11::platform::stm32 {

/**
 * @brief UART interrupt handler class
 *
 * This class encapsulates STM32 UART interrupt handling logic following Design Decision 16.
 * It provides static methods that implement the interrupt handler functionality and
 * propagate status information from ISR to thread context as per Decision 26.
 */
class UARTIRQHandler {
  public:
    /**
     * @brief Handle UART interrupt for a specific UART peripheral
     *
     * This static method handles UART interrupts, including data received,
     * transmission complete, errors, and idle line detection. It also checks
     * for timeout conditions as per Decision 25.
     *
     * @param uart_index The index of the UART peripheral generating the interrupt
     */
    static void handle_irq(device::UARTIndex uart_index);

  private:
    /**
     * @brief Process received data interrupt
     *
     * @param uart Pointer to the UART peripheral
     * @param uart_instance Pointer to the STM32UART instance
     * @return true if data was processed, false otherwise
     */
    static bool process_rx_interrupt(USART_TypeDef* uart, STM32UART* uart_instance);

    /**
     * @brief Process transmit complete interrupt
     *
     * @param uart Pointer to the UART peripheral
     * @param uart_instance Pointer to the STM32UART instance
     * @return true if transmit completed, false otherwise
     */
    static bool process_tx_interrupt(USART_TypeDef* uart, STM32UART* uart_instance);

    /**
     * @brief Process error interrupts (overrun, noise, framing, parity)
     *
     * @param uart Pointer to the UART peripheral
     * @param uart_instance Pointer to the STM32UART instance
     * @return true if error was detected, false otherwise
     */
    static bool process_error_interrupt(USART_TypeDef* uart, STM32UART* uart_instance);

    /**
     * @brief Process idle line detection interrupt
     *
     * @param uart Pointer to the UART peripheral
     * @param uart_instance Pointer to the STM32UART instance
     * @return true if idle was detected, false otherwise
     */
    static bool process_idle_interrupt(USART_TypeDef* uart, STM32UART* uart_instance);

    /**
     * @brief Check for timeout condition
     *
     * Implements Decision 25 by checking if the deadline has been exceeded
     * for ongoing UART operations.
     *
     * @param uart_instance Pointer to the STM32UART instance
     */
    static void check_timeout(STM32UART* uart_instance);
};

/**
 * @brief DMA interrupt handler class
 *
 * This class encapsulates STM32 DMA interrupt handling logic following Design Decision 16.
 * It provides static methods that implement the interrupt handler functionality and
 * propagate status information from ISR to thread context as per Decision 26.
 */
class DMAIRQHandler {
  public:
    /**
     * @brief Handle DMA interrupt for a specific DMA stream
     *
     * This static method handles DMA interrupts, including transfer complete,
     * half transfer, transfer error, and FIFO error conditions.
     *
     * @param dma_index The index of the DMA controller
     * @param stream The DMA stream
     */
    static void handle_irq(device::DMAIndex dma_index, device::DMAStream stream);

    /**
     * @brief Handle DMA receive interrupt for a specific UART peripheral
     *
     * This method is a specialized handler for UART DMA receive operations.
     *
     * @param uart_index The index of the UART peripheral
     */
    static void handle_rx_irq(device::UARTIndex uart_index);

  private:
    /**
     * @brief Process transfer complete interrupt
     *
     * @param dma_stream Pointer to the DMA stream peripheral
     * @param dma_stream_instance Pointer to the DMAStream instance
     * @return true if transfer completed, false otherwise
     */
    static bool process_transfer_complete(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance);

    /**
     * @brief Process half transfer interrupt
     *
     * @param dma_stream Pointer to the DMA stream peripheral
     * @param dma_stream_instance Pointer to the DMAStream instance
     * @return true if half transfer occurred, false otherwise
     */
    static bool process_half_transfer(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance);

    /**
     * @brief Process transfer error interrupt
     *
     * @param dma_stream Pointer to the DMA stream peripheral
     * @param dma_stream_instance Pointer to the DMAStream instance
     * @return true if error was detected, false otherwise
     */
    static bool process_transfer_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance);

    /**
     * @brief Process FIFO error interrupt
     *
     * @param dma_stream Pointer to the DMA stream peripheral
     * @param dma_stream_instance Pointer to the DMAStream instance
     * @return true if FIFO error was detected, false otherwise
     */
    static bool process_fifo_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance);

    /**
     * @brief Process direct mode error interrupt
     *
     * @param dma_stream Pointer to the DMA stream peripheral
     * @param dma_stream_instance Pointer to the DMAStream instance
     * @return true if direct mode error was detected, false otherwise
     */
    static bool process_direct_mode_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance);

    /**
     * @brief Check for timeout condition
     *
     * Implements Decision 25 by checking if the deadline has been exceeded
     * for ongoing DMA operations.
     *
     * @param dma_stream_instance Pointer to the DMAStream instance
     */
    static void check_timeout(DMAStream* dma_stream_instance);
};

/**
 * @brief Initialize the interrupt system
 *
 * This function initializes the NVIC and enables required system interrupts.
 * It's called during system startup.
 */
void init_interrupt_system();

} // namespace hm11::platform::stm32

// Extern "C" declarations for IRQ handler functions in vector table
extern "C" {

/**
 * @brief USART1 IRQ handler
 */
void USART1_IRQHandler(void);

/**
 * @brief USART2 IRQ handler
 */
void USART2_IRQHandler(void);

/**
 * @brief USART3 IRQ handler
 */
void USART3_IRQHandler(void);

/**
 * @brief UART4 IRQ handler
 */
void UART4_IRQHandler(void);

/**
 * @brief UART5 IRQ handler
 */
void UART5_IRQHandler(void);

/**
 * @brief USART6 IRQ handler
 */
void USART6_IRQHandler(void);

/**
 * @brief UART7 IRQ handler
 */
void UART7_IRQHandler(void);

/**
 * @brief UART8 IRQ handler
 */
void UART8_IRQHandler(void);

/**
 * @brief DMA1 Stream0 IRQ handler
 */
void DMA1_Stream0_IRQHandler(void);

/**
 * @brief DMA1 Stream1 IRQ handler
 */
void DMA1_Stream1_IRQHandler(void);

/**
 * @brief DMA1 Stream2 IRQ handler
 */
void DMA1_Stream2_IRQHandler(void);

/**
 * @brief DMA1 Stream3 IRQ handler
 */
void DMA1_Stream3_IRQHandler(void);

/**
 * @brief DMA1 Stream4 IRQ handler
 */
void DMA1_Stream4_IRQHandler(void);

/**
 * @brief DMA1 Stream5 IRQ handler
 */
void DMA1_Stream5_IRQHandler(void);

/**
 * @brief DMA1 Stream6 IRQ handler
 */
void DMA1_Stream6_IRQHandler(void);

/**
 * @brief DMA1 Stream7 IRQ handler
 */
void DMA1_Stream7_IRQHandler(void);

/**
 * @brief DMA2 Stream0 IRQ handler
 */
void DMA2_Stream0_IRQHandler(void);

/**
 * @brief DMA2 Stream1 IRQ handler
 */
void DMA2_Stream1_IRQHandler(void);

/**
 * @brief DMA2 Stream2 IRQ handler
 */
void DMA2_Stream2_IRQHandler(void);

/**
 * @brief DMA2 Stream3 IRQ handler
 */
void DMA2_Stream3_IRQHandler(void);

/**
 * @brief DMA2 Stream4 IRQ handler
 */
void DMA2_Stream4_IRQHandler(void);

/**
 * @brief DMA2 Stream5 IRQ handler
 */
void DMA2_Stream5_IRQHandler(void);

/**
 * @brief DMA2 Stream6 IRQ handler
 */
void DMA2_Stream6_IRQHandler(void);

/**
 * @brief DMA2 Stream7 IRQ handler
 */
void DMA2_Stream7_IRQHandler(void);

} // extern "C"

#endif // HM11_PLATFORM_STM32_INTERRUPT_HANDLERS_HPP