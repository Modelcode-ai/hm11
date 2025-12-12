/**
 * @file interrupt_handlers.cpp
 * @brief Implementation of STM32 interrupt handlers for UART and DMA
 * @details Implements Decision 16, 23, 25, and 26 for STM32 interrupt handlers
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides the implementation of C++ classes that encapsulate STM32 interrupt handlers
 * for UART and DMA, and the extern "C" wrapper functions that connect to the vector table.
 */

#include <hm11/platform/cache.hpp>
#include <hm11/platform/stm32/interrupt_handlers.hpp>
#include <hm11/platform/stm32/peripherals.hpp>
#include <hm11/util/memory_order.hpp>

namespace hm11::platform::stm32 {

// UART IRQ handler implementation

void UARTIRQHandler::handle_irq(device::UARTIndex uart_index) {
    // Get the UART peripheral and instance
    USART_TypeDef* uart = device::get_uart_peripheral(uart_index);
    STM32UART* uart_instance = get_uart_instance(uart_index).get();

    if (!uart || !uart_instance) {
        return; // Nothing to do if UART not found
    }

    // Process different interrupt sources in priority order
    bool error_processed = process_error_interrupt(uart, uart_instance);
    bool idle_processed = process_idle_interrupt(uart, uart_instance);
    bool rx_processed = process_rx_interrupt(uart, uart_instance);
    bool tx_processed = process_tx_interrupt(uart, uart_instance);

    // Check for timeout even if no interrupt was processed
    // This implements Decision 25 for ISR-side deadline polling
    if (!(error_processed || idle_processed || rx_processed || tx_processed)) {
        check_timeout(uart_instance);
    }
}

bool UARTIRQHandler::process_rx_interrupt(USART_TypeDef* uart, STM32UART* uart_instance) {
    // Check if data is available to read
    if ((uart->SR & USART_SR_RXNE) != 0) {
        // Call the UART instance's handler
        uart_instance->handle_irq();
        return true;
    }
    return false;
}

bool UARTIRQHandler::process_tx_interrupt(USART_TypeDef* uart, STM32UART* uart_instance) {
    // Check if transmit buffer is empty
    if ((uart->SR & USART_SR_TXE) != 0) {
        // Call the UART instance's handler
        uart_instance->handle_irq();
        return true;
    }
    return false;
}

bool UARTIRQHandler::process_error_interrupt(USART_TypeDef* uart, STM32UART* uart_instance) {
    // Check for overrun error, noise error, framing error, or parity error
    const uint32_t error_flags = USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE;
    if ((uart->SR & error_flags) != 0) {
        // Clear the error flags by reading SR followed by DR
        volatile uint32_t sr = uart->SR;
        volatile uint32_t dr = uart->DR;
        (void)sr; // Suppress unused variable warning
        (void)dr; // Suppress unused variable warning

        // Call the UART instance's handler with error status
        uart_instance->handle_irq();
        return true;
    }
    return false;
}

bool UARTIRQHandler::process_idle_interrupt(USART_TypeDef* uart, STM32UART* uart_instance) {
    // Check for idle line detected
    if ((uart->SR & USART_SR_IDLE) != 0) {
        // Clear the idle flag by reading SR followed by DR
        volatile uint32_t sr = uart->SR;
        volatile uint32_t dr = uart->DR;
        (void)sr; // Suppress unused variable warning
        (void)dr; // Suppress unused variable warning

        // Call the UART instance's handler
        uart_instance->handle_irq();
        return true;
    }
    return false;
}

void UARTIRQHandler::check_timeout(STM32UART* uart_instance) {
    // This method implements Decision 25 by checking if the timeout deadline
    // has been exceeded for any ongoing operations. It's called opportunistically
    // from the UART IRQ handler, even when no other interrupt sources are active.

    // The watchdog checks are handled inside the UART instance's handle_irq method
    uart_instance->handle_irq();
}

// DMA IRQ handler implementation

void DMAIRQHandler::handle_irq(device::DMAIndex dma_index, device::DMAStream stream) {
    // Get the DMA stream peripheral and instance
    DMA_Stream_TypeDef* dma_stream = device::get_dma_stream_peripheral(dma_index, stream);
    DMAStream* dma_stream_instance = get_dma_stream_instance(dma_index, stream);

    if (!dma_stream || !dma_stream_instance) {
        return; // Nothing to do if DMA stream not found
    }

    // Process different interrupt sources in priority order
    bool error_processed = process_transfer_error(dma_stream, dma_stream_instance);
    bool fifo_error_processed = process_fifo_error(dma_stream, dma_stream_instance);
    bool direct_mode_error_processed = process_direct_mode_error(dma_stream, dma_stream_instance);
    bool transfer_complete_processed = process_transfer_complete(dma_stream, dma_stream_instance);
    bool half_transfer_processed = process_half_transfer(dma_stream, dma_stream_instance);

    // Check for timeout even if no interrupt was processed
    // This implements Decision 25 for ISR-side deadline polling
    if (!(error_processed || fifo_error_processed || direct_mode_error_processed || transfer_complete_processed ||
          half_transfer_processed)) {
        check_timeout(dma_stream_instance);
    }
}

void DMAIRQHandler::handle_rx_irq(device::UARTIndex uart_index) {
    // Get the UART instance
    std::shared_ptr<STM32UART> uart_instance = get_uart_instance(uart_index);

    if (!uart_instance) {
        return; // Nothing to do if UART not found
    }

    // Call the UART instance's DMA RX IRQ handler
    uart_instance->handle_dma_rx_irq();
}

bool DMAIRQHandler::process_transfer_complete(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance) {
    // Check if transfer complete flag is set
    uint32_t flag_position = 0;

    // The flag position depends on the stream number
    switch (reinterpret_cast<uintptr_t>(dma_stream) & 0x3F) {
        case 0x00:
            flag_position = DMA_LISR_TCIF0;
            break;
        case 0x18:
            flag_position = DMA_LISR_TCIF1;
            break;
        case 0x30:
            flag_position = DMA_LISR_TCIF2;
            break;
        case 0x48:
            flag_position = DMA_LISR_TCIF3;
            break;
        case 0x60:
            flag_position = DMA_HISR_TCIF4;
            break;
        case 0x78:
            flag_position = DMA_HISR_TCIF5;
            break;
        case 0x90:
            flag_position = DMA_HISR_TCIF6;
            break;
        case 0xA8:
            flag_position = DMA_HISR_TCIF7;
            break;
        default:
            return false;
    }

    // Check if the flag is set in the appropriate status register
    bool flag_set = false;
    if (flag_position <= DMA_LISR_TCIF3) {
        flag_set = (DMA1->LISR & flag_position) != 0;
    } else {
        flag_set = (DMA1->HISR & flag_position) != 0;
    }

    if (flag_set) {
        // Clear the flag
        if (flag_position <= DMA_LISR_TCIF3) {
            DMA1->LIFCR = flag_position;
        } else {
            DMA1->HIFCR = flag_position;
        }

        // Call the DMA stream instance's handler
        dma_stream_instance->handle_irq();
        return true;
    }

    return false;
}

bool DMAIRQHandler::process_half_transfer(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance) {
    // Check if half transfer flag is set
    uint32_t flag_position = 0;

    // The flag position depends on the stream number
    switch (reinterpret_cast<uintptr_t>(dma_stream) & 0x3F) {
        case 0x00:
            flag_position = DMA_LISR_HTIF0;
            break;
        case 0x18:
            flag_position = DMA_LISR_HTIF1;
            break;
        case 0x30:
            flag_position = DMA_LISR_HTIF2;
            break;
        case 0x48:
            flag_position = DMA_LISR_HTIF3;
            break;
        case 0x60:
            flag_position = DMA_HISR_HTIF4;
            break;
        case 0x78:
            flag_position = DMA_HISR_HTIF5;
            break;
        case 0x90:
            flag_position = DMA_HISR_HTIF6;
            break;
        case 0xA8:
            flag_position = DMA_HISR_HTIF7;
            break;
        default:
            return false;
    }

    // Check if the flag is set in the appropriate status register
    bool flag_set = false;
    if (flag_position <= DMA_LISR_HTIF3) {
        flag_set = (DMA1->LISR & flag_position) != 0;
    } else {
        flag_set = (DMA1->HISR & flag_position) != 0;
    }

    if (flag_set) {
        // Clear the flag
        if (flag_position <= DMA_LISR_HTIF3) {
            DMA1->LIFCR = flag_position;
        } else {
            DMA1->HIFCR = flag_position;
        }

        // Call the DMA stream instance's handler
        dma_stream_instance->handle_irq();
        return true;
    }

    return false;
}

bool DMAIRQHandler::process_transfer_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance) {
    // Check if transfer error flag is set
    uint32_t flag_position = 0;

    // The flag position depends on the stream number
    switch (reinterpret_cast<uintptr_t>(dma_stream) & 0x3F) {
        case 0x00:
            flag_position = DMA_LISR_TEIF0;
            break;
        case 0x18:
            flag_position = DMA_LISR_TEIF1;
            break;
        case 0x30:
            flag_position = DMA_LISR_TEIF2;
            break;
        case 0x48:
            flag_position = DMA_LISR_TEIF3;
            break;
        case 0x60:
            flag_position = DMA_HISR_TEIF4;
            break;
        case 0x78:
            flag_position = DMA_HISR_TEIF5;
            break;
        case 0x90:
            flag_position = DMA_HISR_TEIF6;
            break;
        case 0xA8:
            flag_position = DMA_HISR_TEIF7;
            break;
        default:
            return false;
    }

    // Check if the flag is set in the appropriate status register
    bool flag_set = false;
    if (flag_position <= DMA_LISR_TEIF3) {
        flag_set = (DMA1->LISR & flag_position) != 0;
    } else {
        flag_set = (DMA1->HISR & flag_position) != 0;
    }

    if (flag_set) {
        // Clear the flag
        if (flag_position <= DMA_LISR_TEIF3) {
            DMA1->LIFCR = flag_position;
        } else {
            DMA1->HIFCR = flag_position;
        }

        // Call the DMA stream instance's handler with error status
        dma_stream_instance->handle_irq();
        return true;
    }

    return false;
}

bool DMAIRQHandler::process_fifo_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance) {
    // Check if FIFO error flag is set
    uint32_t flag_position = 0;

    // The flag position depends on the stream number
    switch (reinterpret_cast<uintptr_t>(dma_stream) & 0x3F) {
        case 0x00:
            flag_position = DMA_LISR_FEIF0;
            break;
        case 0x18:
            flag_position = DMA_LISR_FEIF1;
            break;
        case 0x30:
            flag_position = DMA_LISR_FEIF2;
            break;
        case 0x48:
            flag_position = DMA_LISR_FEIF3;
            break;
        case 0x60:
            flag_position = DMA_HISR_FEIF4;
            break;
        case 0x78:
            flag_position = DMA_HISR_FEIF5;
            break;
        case 0x90:
            flag_position = DMA_HISR_FEIF6;
            break;
        case 0xA8:
            flag_position = DMA_HISR_FEIF7;
            break;
        default:
            return false;
    }

    // Check if the flag is set in the appropriate status register
    bool flag_set = false;
    if (flag_position <= DMA_LISR_FEIF3) {
        flag_set = (DMA1->LISR & flag_position) != 0;
    } else {
        flag_set = (DMA1->HISR & flag_position) != 0;
    }

    if (flag_set) {
        // Clear the flag
        if (flag_position <= DMA_LISR_FEIF3) {
            DMA1->LIFCR = flag_position;
        } else {
            DMA1->HIFCR = flag_position;
        }

        // Call the DMA stream instance's handler with error status
        dma_stream_instance->handle_irq();
        return true;
    }

    return false;
}

bool DMAIRQHandler::process_direct_mode_error(DMA_Stream_TypeDef* dma_stream, DMAStream* dma_stream_instance) {
    // Check if direct mode error flag is set
    uint32_t flag_position = 0;

    // The flag position depends on the stream number
    switch (reinterpret_cast<uintptr_t>(dma_stream) & 0x3F) {
        case 0x00:
            flag_position = DMA_LISR_DMEIF0;
            break;
        case 0x18:
            flag_position = DMA_LISR_DMEIF1;
            break;
        case 0x30:
            flag_position = DMA_LISR_DMEIF2;
            break;
        case 0x48:
            flag_position = DMA_LISR_DMEIF3;
            break;
        case 0x60:
            flag_position = DMA_HISR_DMEIF4;
            break;
        case 0x78:
            flag_position = DMA_HISR_DMEIF5;
            break;
        case 0x90:
            flag_position = DMA_HISR_DMEIF6;
            break;
        case 0xA8:
            flag_position = DMA_HISR_DMEIF7;
            break;
        default:
            return false;
    }

    // Check if the flag is set in the appropriate status register
    bool flag_set = false;
    if (flag_position <= DMA_LISR_DMEIF3) {
        flag_set = (DMA1->LISR & flag_position) != 0;
    } else {
        flag_set = (DMA1->HISR & flag_position) != 0;
    }

    if (flag_set) {
        // Clear the flag
        if (flag_position <= DMA_LISR_DMEIF3) {
            DMA1->LIFCR = flag_position;
        } else {
            DMA1->HIFCR = flag_position;
        }

        // Call the DMA stream instance's handler with error status
        dma_stream_instance->handle_irq();
        return true;
    }

    return false;
}

void DMAIRQHandler::check_timeout(DMAStream* dma_stream_instance) {
    // This method implements Decision 25 by checking if the timeout deadline
    // has been exceeded for any ongoing operations. It's called opportunistically
    // from the DMA IRQ handler, even when no other interrupt sources are active.

    // The timeout checks are handled inside the DMA instance's handle_irq method
    dma_stream_instance->handle_irq();
}

// Initialize interrupt system
void init_interrupt_system() {
    // Configure NVIC priority grouping
    // Use 4 bits for preempt priority and 0 bits for subpriority
    NVIC_SetPriorityGrouping(0);

    // Set default priorities for UART and DMA interrupts
    // These can be overridden by individual UART and DMA instances

    // UART interrupts have higher priority (lower value) than DMA
    for (int i = 0; i < 8; ++i) {
        // Set UART interrupt priorities
        device::UARTIndex uart = static_cast<device::UARTIndex>(i);
        NVIC_SetPriority(static_cast<IRQn_Type>(device::to_cmsis_irqn(device::get_uart_irq(uart))), 5);
    }

    // DMA interrupts have slightly lower priority
    for (int j = 0; j < 8; ++j) {
        // Set DMA1 interrupt priorities
        NVIC_SetPriority(
            static_cast<IRQn_Type>(device::to_cmsis_irqn(
                device::get_dma_stream_irq(device::DMAIndex::DMA1, static_cast<device::DMAStream>(j)))),
            6);

        // Set DMA2 interrupt priorities
        NVIC_SetPriority(
            static_cast<IRQn_Type>(device::to_cmsis_irqn(
                device::get_dma_stream_irq(device::DMAIndex::DMA2, static_cast<device::DMAStream>(j)))),
            6);
    }
}

} // namespace hm11::platform::stm32

// Extern "C" IRQ handler implementations

extern "C" {

// UART IRQ handlers

void USART1_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::USART1);
}

void USART2_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::USART2);
}

void USART3_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::USART3);
}

void UART4_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::UART4);
}

void UART5_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::UART5);
}

void USART6_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::USART6);
}

void UART7_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::UART7);
}

void UART8_IRQHandler(void) {
    hm11::platform::stm32::UARTIRQHandler::handle_irq(hm11::platform::stm32::device::UARTIndex::UART8);
}

// DMA IRQ handlers

void DMA1_Stream0_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream0);

    // For UART5 RX (as per the predefined configuration)
    if (hm11::platform::stm32::device::UART5_DMA == hm11::platform::stm32::device::DMAIndex::DMA1 &&
        hm11::platform::stm32::device::UART5_RX_Stream == hm11::platform::stm32::device::DMAStream::Stream0) {
        hm11::platform::stm32::DMAIRQHandler::handle_rx_irq(hm11::platform::stm32::device::UARTIndex::UART5);
    }
}

void DMA1_Stream1_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream1);
}

void DMA1_Stream2_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream2);
}

void DMA1_Stream3_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream3);
}

void DMA1_Stream4_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream4);
}

void DMA1_Stream5_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream5);
}

void DMA1_Stream6_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream6);
}

void DMA1_Stream7_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA1,
        hm11::platform::stm32::device::DMAStream::Stream7);
}

void DMA2_Stream0_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream0);
}

void DMA2_Stream1_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream1);
}

void DMA2_Stream2_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream2);
}

void DMA2_Stream3_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream3);
}

void DMA2_Stream4_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream4);
}

void DMA2_Stream5_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream5);
}

void DMA2_Stream6_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream6);
}

void DMA2_Stream7_IRQHandler(void) {
    hm11::platform::stm32::DMAIRQHandler::handle_irq(
        hm11::platform::stm32::device::DMAIndex::DMA2,
        hm11::platform::stm32::device::DMAStream::Stream7);
}

} // extern "C"