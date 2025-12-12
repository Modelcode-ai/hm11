/**
 * @file device.hpp
 * @brief STM32 device-specific definitions and constants
 *
 * This file contains device-specific definitions for STM32F4xx microcontrollers,
 * including peripheral base addresses, interrupt IDs, and pin mappings.
 * It provides direct access to hardware registers following Design Decision 30.
 *
 * @note This implementation uses direct register access via CMSIS headers
 * without the STM32 HAL library dependency, matching the Ada implementation
 * in the original codebase.
 */

#ifndef HM11_PLATFORM_STM32_DEVICE_HPP
#define HM11_PLATFORM_STM32_DEVICE_HPP

#include <cstdint>

// Include CMSIS Core headers
#include "core_cm4.h"
// Include STM32F4xx specific CMSIS headers
#include "stm32f4xx.h"

namespace hm11 {
namespace platform {
namespace stm32 {

/**
 * @brief STM32F429 specific peripheral and pin definitions.
 *
 * This namespace contains constants and definitions specific to the STM32F429
 * microcontroller, including peripheral registers, GPIO pins, DMA streams, etc.
 */
namespace device {

/**
 * @brief Enumeration of available USART/UART peripherals on STM32F429
 */
enum class UARTIndex {
    USART1, ///< USART1 peripheral
    USART2, ///< USART2 peripheral
    USART3, ///< USART3 peripheral
    UART4,  ///< UART4 peripheral
    UART5,  ///< UART5 peripheral
    USART6, ///< USART6 peripheral
    UART7,  ///< UART7 peripheral
    UART8   ///< UART8 peripheral
};

/**
 * @brief Enumeration of available DMA controllers on STM32F429
 */
enum class DMAIndex {
    DMA1, ///< DMA1 controller
    DMA2  ///< DMA2 controller
};

/**
 * @brief Enumeration of DMA stream selectors
 */
enum class DMAStream {
    Stream0, ///< Stream 0
    Stream1, ///< Stream 1
    Stream2, ///< Stream 2
    Stream3, ///< Stream 3
    Stream4, ///< Stream 4
    Stream5, ///< Stream 5
    Stream6, ///< Stream 6
    Stream7  ///< Stream 7
};

/**
 * @brief Enumeration of DMA channel selectors
 */
enum class DMAChannel {
    Channel0, ///< Channel 0
    Channel1, ///< Channel 1
    Channel2, ///< Channel 2
    Channel3, ///< Channel 3
    Channel4, ///< Channel 4
    Channel5, ///< Channel 5
    Channel6, ///< Channel 6
    Channel7  ///< Channel 7
};

/**
 * @brief Enumeration of GPIO ports
 */
enum class GPIOPort {
    PortA, ///< GPIO Port A
    PortB, ///< GPIO Port B
    PortC, ///< GPIO Port C
    PortD, ///< GPIO Port D
    PortE, ///< GPIO Port E
    PortF, ///< GPIO Port F
    PortG, ///< GPIO Port G
    PortH, ///< GPIO Port H
    PortI, ///< GPIO Port I
    PortJ, ///< GPIO Port J
    PortK  ///< GPIO Port K
};

/**
 * @brief Structure representing a GPIO pin
 *
 * This structure combines port and pin information to uniquely identify a GPIO pin.
 */
struct GPIOPin {
    GPIOPort port; ///< The GPIO port
    uint8_t pin;   ///< The pin number (0-15)

    /**
     * @brief Construct a new GPIO Pin object
     *
     * @param p The GPIO port
     * @param n The pin number (0-15)
     */
    constexpr GPIOPin(GPIOPort p, uint8_t n) : port(p), pin(n) {}
};

// USART/UART peripheral pointers - direct register access
inline USART_TypeDef* const USART1_Peripheral = USART1;
inline USART_TypeDef* const USART2_Peripheral = USART2;
inline USART_TypeDef* const USART3_Peripheral = USART3;
inline USART_TypeDef* const UART4_Peripheral = UART4;
inline USART_TypeDef* const UART5_Peripheral = UART5;
inline USART_TypeDef* const USART6_Peripheral = USART6;
inline USART_TypeDef* const UART7_Peripheral = UART7;
inline USART_TypeDef* const UART8_Peripheral = UART8;

// GPIO peripheral pointers
inline GPIO_TypeDef* const GPIOA_Peripheral = GPIOA;
inline GPIO_TypeDef* const GPIOB_Peripheral = GPIOB;
inline GPIO_TypeDef* const GPIOC_Peripheral = GPIOC;
inline GPIO_TypeDef* const GPIOD_Peripheral = GPIOD;
inline GPIO_TypeDef* const GPIOE_Peripheral = GPIOE;
inline GPIO_TypeDef* const GPIOF_Peripheral = GPIOF;
inline GPIO_TypeDef* const GPIOG_Peripheral = GPIOG;
inline GPIO_TypeDef* const GPIOH_Peripheral = GPIOH;
inline GPIO_TypeDef* const GPIOI_Peripheral = GPIOI;
inline GPIO_TypeDef* const GPIOJ_Peripheral = GPIOJ;
inline GPIO_TypeDef* const GPIOK_Peripheral = GPIOK;

// DMA controller pointers
inline DMA_TypeDef* const DMA1_Peripheral = DMA1;
inline DMA_TypeDef* const DMA2_Peripheral = DMA2;

// DMA stream pointers
inline DMA_Stream_TypeDef* const DMA1_Stream0_Peripheral = DMA1_Stream0;
inline DMA_Stream_TypeDef* const DMA1_Stream1_Peripheral = DMA1_Stream1;
inline DMA_Stream_TypeDef* const DMA1_Stream2_Peripheral = DMA1_Stream2;
inline DMA_Stream_TypeDef* const DMA1_Stream3_Peripheral = DMA1_Stream3;
inline DMA_Stream_TypeDef* const DMA1_Stream4_Peripheral = DMA1_Stream4;
inline DMA_Stream_TypeDef* const DMA1_Stream5_Peripheral = DMA1_Stream5;
inline DMA_Stream_TypeDef* const DMA1_Stream6_Peripheral = DMA1_Stream6;
inline DMA_Stream_TypeDef* const DMA1_Stream7_Peripheral = DMA1_Stream7;

inline DMA_Stream_TypeDef* const DMA2_Stream0_Peripheral = DMA2_Stream0;
inline DMA_Stream_TypeDef* const DMA2_Stream1_Peripheral = DMA2_Stream1;
inline DMA_Stream_TypeDef* const DMA2_Stream2_Peripheral = DMA2_Stream2;
inline DMA_Stream_TypeDef* const DMA2_Stream3_Peripheral = DMA2_Stream3;
inline DMA_Stream_TypeDef* const DMA2_Stream4_Peripheral = DMA2_Stream4;
inline DMA_Stream_TypeDef* const DMA2_Stream5_Peripheral = DMA2_Stream5;
inline DMA_Stream_TypeDef* const DMA2_Stream6_Peripheral = DMA2_Stream6;
inline DMA_Stream_TypeDef* const DMA2_Stream7_Peripheral = DMA2_Stream7;

// Interrupt IDs (matching STM32F429 NVIC vector table positions)
enum class IRQn_Type {
    // USART IRQs
    USART1_IRQn = 37,
    USART2_IRQn = 38,
    USART3_IRQn = 39,
    UART4_IRQn = 52,
    UART5_IRQn = 53,
    USART6_IRQn = 71,
    UART7_IRQn = 82,
    UART8_IRQn = 83,

    // DMA1 IRQs
    DMA1_Stream0_IRQn = 11,
    DMA1_Stream1_IRQn = 12,
    DMA1_Stream2_IRQn = 13,
    DMA1_Stream3_IRQn = 14,
    DMA1_Stream4_IRQn = 15,
    DMA1_Stream5_IRQn = 16,
    DMA1_Stream6_IRQn = 17,
    DMA1_Stream7_IRQn = 47,

    // DMA2 IRQs
    DMA2_Stream0_IRQn = 56,
    DMA2_Stream1_IRQn = 57,
    DMA2_Stream2_IRQn = 58,
    DMA2_Stream3_IRQn = 59,
    DMA2_Stream4_IRQn = 60,
    DMA2_Stream5_IRQn = 68,
    DMA2_Stream6_IRQn = 69,
    DMA2_Stream7_IRQn = 70,
};

// Pre-defined GPIO pins used by the HM11 driver on STM32F429 Discovery board
// (These match the pins used in the Ada implementation)
constexpr GPIOPin PC12 = GPIOPin(GPIOPort::PortC, 12); // UART5_TX (Connect to HM-11 RX)
constexpr GPIOPin PD2 = GPIOPin(GPIOPort::PortD, 2);   // UART5_RX (Connect to HM-11 TX)

// DMA configuration for UART5 matching the Ada implementation
constexpr DMAIndex UART5_DMA = DMAIndex::DMA1;
constexpr DMAStream UART5_RX_Stream = DMAStream::Stream0;
constexpr DMAChannel UART5_RX_Channel = DMAChannel::Channel4;

/**
 * @brief Get the CMSIS NVIC IRQ number for a given IRQ type
 *
 * @param irq The IRQ type from the IRQn_Type enum
 * @return int16_t The corresponding CMSIS NVIC IRQ number
 */
constexpr int16_t to_cmsis_irqn(IRQn_Type irq) {
    return static_cast<int16_t>(irq);
}

/**
 * @brief Get the UART peripheral pointer for a given UART index
 *
 * @param uart The UART index
 * @return USART_TypeDef* Pointer to the UART peripheral registers
 */
inline USART_TypeDef* get_uart_peripheral(UARTIndex uart) {
    switch (uart) {
        case UARTIndex::USART1:
            return USART1_Peripheral;
        case UARTIndex::USART2:
            return USART2_Peripheral;
        case UARTIndex::USART3:
            return USART3_Peripheral;
        case UARTIndex::UART4:
            return UART4_Peripheral;
        case UARTIndex::UART5:
            return UART5_Peripheral;
        case UARTIndex::USART6:
            return USART6_Peripheral;
        case UARTIndex::UART7:
            return UART7_Peripheral;
        case UARTIndex::UART8:
            return UART8_Peripheral;
        default:
            return nullptr;
    }
}

/**
 * @brief Get the GPIO peripheral pointer for a given GPIO port
 *
 * @param port The GPIO port
 * @return GPIO_TypeDef* Pointer to the GPIO peripheral registers
 */
inline GPIO_TypeDef* get_gpio_peripheral(GPIOPort port) {
    switch (port) {
        case GPIOPort::PortA:
            return GPIOA_Peripheral;
        case GPIOPort::PortB:
            return GPIOB_Peripheral;
        case GPIOPort::PortC:
            return GPIOC_Peripheral;
        case GPIOPort::PortD:
            return GPIOD_Peripheral;
        case GPIOPort::PortE:
            return GPIOE_Peripheral;
        case GPIOPort::PortF:
            return GPIOF_Peripheral;
        case GPIOPort::PortG:
            return GPIOG_Peripheral;
        case GPIOPort::PortH:
            return GPIOH_Peripheral;
        case GPIOPort::PortI:
            return GPIOI_Peripheral;
        case GPIOPort::PortJ:
            return GPIOJ_Peripheral;
        case GPIOPort::PortK:
            return GPIOK_Peripheral;
        default:
            return nullptr;
    }
}

/**
 * @brief Get the DMA controller peripheral pointer
 *
 * @param dma The DMA controller index
 * @return DMA_TypeDef* Pointer to the DMA controller peripheral registers
 */
inline DMA_TypeDef* get_dma_peripheral(DMAIndex dma) {
    switch (dma) {
        case DMAIndex::DMA1:
            return DMA1_Peripheral;
        case DMAIndex::DMA2:
            return DMA2_Peripheral;
        default:
            return nullptr;
    }
}

/**
 * @brief Get the DMA stream peripheral pointer
 *
 * @param dma The DMA controller index
 * @param stream The DMA stream
 * @return DMA_Stream_TypeDef* Pointer to the DMA stream peripheral registers
 */
inline DMA_Stream_TypeDef* get_dma_stream_peripheral(DMAIndex dma, DMAStream stream) {
    if (dma == DMAIndex::DMA1) {
        switch (stream) {
            case DMAStream::Stream0:
                return DMA1_Stream0_Peripheral;
            case DMAStream::Stream1:
                return DMA1_Stream1_Peripheral;
            case DMAStream::Stream2:
                return DMA1_Stream2_Peripheral;
            case DMAStream::Stream3:
                return DMA1_Stream3_Peripheral;
            case DMAStream::Stream4:
                return DMA1_Stream4_Peripheral;
            case DMAStream::Stream5:
                return DMA1_Stream5_Peripheral;
            case DMAStream::Stream6:
                return DMA1_Stream6_Peripheral;
            case DMAStream::Stream7:
                return DMA1_Stream7_Peripheral;
            default:
                return nullptr;
        }
    } else if (dma == DMAIndex::DMA2) {
        switch (stream) {
            case DMAStream::Stream0:
                return DMA2_Stream0_Peripheral;
            case DMAStream::Stream1:
                return DMA2_Stream1_Peripheral;
            case DMAStream::Stream2:
                return DMA2_Stream2_Peripheral;
            case DMAStream::Stream3:
                return DMA2_Stream3_Peripheral;
            case DMAStream::Stream4:
                return DMA2_Stream4_Peripheral;
            case DMAStream::Stream5:
                return DMA2_Stream5_Peripheral;
            case DMAStream::Stream6:
                return DMA2_Stream6_Peripheral;
            case DMAStream::Stream7:
                return DMA2_Stream7_Peripheral;
            default:
                return nullptr;
        }
    }
    return nullptr;
}

/**
 * @brief Get the IRQ number for a given UART peripheral
 *
 * @param uart The UART index
 * @return IRQn_Type The IRQ number
 */
inline IRQn_Type get_uart_irq(UARTIndex uart) {
    switch (uart) {
        case UARTIndex::USART1:
            return IRQn_Type::USART1_IRQn;
        case UARTIndex::USART2:
            return IRQn_Type::USART2_IRQn;
        case UARTIndex::USART3:
            return IRQn_Type::USART3_IRQn;
        case UARTIndex::UART4:
            return IRQn_Type::UART4_IRQn;
        case UARTIndex::UART5:
            return IRQn_Type::UART5_IRQn;
        case UARTIndex::USART6:
            return IRQn_Type::USART6_IRQn;
        case UARTIndex::UART7:
            return IRQn_Type::UART7_IRQn;
        case UARTIndex::UART8:
            return IRQn_Type::UART8_IRQn;
        default:
            return static_cast<IRQn_Type>(-1);
    }
}

/**
 * @brief Get the IRQ number for a given DMA stream
 *
 * @param dma The DMA controller index
 * @param stream The DMA stream
 * @return IRQn_Type The IRQ number
 */
inline IRQn_Type get_dma_stream_irq(DMAIndex dma, DMAStream stream) {
    if (dma == DMAIndex::DMA1) {
        switch (stream) {
            case DMAStream::Stream0:
                return IRQn_Type::DMA1_Stream0_IRQn;
            case DMAStream::Stream1:
                return IRQn_Type::DMA1_Stream1_IRQn;
            case DMAStream::Stream2:
                return IRQn_Type::DMA1_Stream2_IRQn;
            case DMAStream::Stream3:
                return IRQn_Type::DMA1_Stream3_IRQn;
            case DMAStream::Stream4:
                return IRQn_Type::DMA1_Stream4_IRQn;
            case DMAStream::Stream5:
                return IRQn_Type::DMA1_Stream5_IRQn;
            case DMAStream::Stream6:
                return IRQn_Type::DMA1_Stream6_IRQn;
            case DMAStream::Stream7:
                return IRQn_Type::DMA1_Stream7_IRQn;
            default:
                return static_cast<IRQn_Type>(-1);
        }
    } else if (dma == DMAIndex::DMA2) {
        switch (stream) {
            case DMAStream::Stream0:
                return IRQn_Type::DMA2_Stream0_IRQn;
            case DMAStream::Stream1:
                return IRQn_Type::DMA2_Stream1_IRQn;
            case DMAStream::Stream2:
                return IRQn_Type::DMA2_Stream2_IRQn;
            case DMAStream::Stream3:
                return IRQn_Type::DMA2_Stream3_IRQn;
            case DMAStream::Stream4:
                return IRQn_Type::DMA2_Stream4_IRQn;
            case DMAStream::Stream5:
                return IRQn_Type::DMA2_Stream5_IRQn;
            case DMAStream::Stream6:
                return IRQn_Type::DMA2_Stream6_IRQn;
            case DMAStream::Stream7:
                return IRQn_Type::DMA2_Stream7_IRQn;
            default:
                return static_cast<IRQn_Type>(-1);
        }
    }
    return static_cast<IRQn_Type>(-1);
}

// Clock control functions for peripherals

/**
 * @brief Enable the clock for a UART peripheral
 *
 * @param uart The UART index
 */
void enable_uart_clock(UARTIndex uart);

/**
 * @brief Enable the clock for a GPIO port
 *
 * @param port The GPIO port
 */
void enable_gpio_clock(GPIOPort port);

/**
 * @brief Enable the clock for a DMA controller
 *
 * @param dma The DMA controller index
 */
void enable_dma_clock(DMAIndex dma);

} // namespace device
} // namespace stm32
} // namespace platform
} // namespace hm11

#endif // HM11_PLATFORM_STM32_DEVICE_HPP