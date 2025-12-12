/**
 * @file registers.hpp
 * @brief STM32 register access utilities and common bit definitions
 *
 * This file contains utilities for accessing and manipulating STM32 hardware registers
 * and defines common bit patterns used across different peripherals.
 * It provides type-safe and efficient register access functions that help prevent
 * common errors when working with hardware registers.
 *
 * @note This implementation follows Design Decision 30 by using direct register
 * access via CMSIS headers without the STM32 HAL library dependency.
 */

#ifndef HM11_PLATFORM_STM32_REGISTERS_HPP
#define HM11_PLATFORM_STM32_REGISTERS_HPP

#include <cstdint>

namespace hm11 {
namespace platform {
namespace stm32 {
namespace reg {

/**
 * @brief Set bits in a register while preserving other bits
 *
 * This function sets the specified bits in a register without affecting other bits.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to set
 */
inline void set_bits(volatile uint32_t* reg, uint32_t bits) {
    *reg |= bits;
}

/**
 * @brief Clear bits in a register while preserving other bits
 *
 * This function clears the specified bits in a register without affecting other bits.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to clear
 */
inline void clear_bits(volatile uint32_t* reg, uint32_t bits) {
    *reg &= ~bits;
}

/**
 * @brief Toggle bits in a register
 *
 * This function toggles the specified bits in a register without affecting other bits.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to toggle
 */
inline void toggle_bits(volatile uint32_t* reg, uint32_t bits) {
    *reg ^= bits;
}

/**
 * @brief Read bits from a register
 *
 * This function reads the specified bits from a register.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to read
 * @return uint32_t Value of the specified bits
 */
inline uint32_t read_bits(const volatile uint32_t* reg, uint32_t bits) {
    return *reg & bits;
}

/**
 * @brief Check if specified bits are set in a register
 *
 * This function checks if all specified bits are set in a register.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to check
 * @return true if all bits are set
 * @return false if any bit is clear
 */
inline bool is_set(const volatile uint32_t* reg, uint32_t bits) {
    return (*reg & bits) == bits;
}

/**
 * @brief Check if specified bits are clear in a register
 *
 * This function checks if all specified bits are clear in a register.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to check
 * @return true if all bits are clear
 * @return false if any bit is set
 */
inline bool is_clear(const volatile uint32_t* reg, uint32_t bits) {
    return (*reg & bits) == 0;
}

/**
 * @brief Modify specific bits in a register
 *
 * This function clears the bits specified by the mask and then sets the value.
 *
 * @param reg Pointer to the register
 * @param mask Bitmask of bits to modify
 * @param value New value to set (must be pre-shifted to correct position)
 */
inline void modify_bits(volatile uint32_t* reg, uint32_t mask, uint32_t value) {
    *reg = (*reg & ~mask) | (value & mask);
}

/**
 * @brief Clear and set specific bits in a register based on position and size
 *
 * This function clears a field of bits at the specified position and sets a new value.
 *
 * @param reg Pointer to the register
 * @param pos Starting bit position of the field
 * @param width Width of the field in bits
 * @param value New value to set (will be shifted to the correct position)
 */
inline void modify_field(volatile uint32_t* reg, uint32_t pos, uint32_t width, uint32_t value) {
    const uint32_t mask = ((1U << width) - 1) << pos;
    *reg = (*reg & ~mask) | ((value << pos) & mask);
}

/**
 * @brief Read a field of bits from a register
 *
 * This function reads a field of bits at the specified position and width.
 *
 * @param reg Pointer to the register
 * @param pos Starting bit position of the field
 * @param width Width of the field in bits
 * @return uint32_t Value of the field (shifted down to start at bit 0)
 */
inline uint32_t read_field(const volatile uint32_t* reg, uint32_t pos, uint32_t width) {
    const uint32_t mask = ((1U << width) - 1) << pos;
    return (*reg & mask) >> pos;
}

/**
 * @brief Wait until specified bits in a register match the expected value
 *
 * This function blocks until the specified bits in a register match the expected value.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to check
 * @param expected_value Expected value of the bits
 */
inline void wait_until_match(const volatile uint32_t* reg, uint32_t bits, uint32_t expected_value) {
    while ((*reg & bits) != expected_value) {
        // Wait
    }
}

/**
 * @brief Wait until specified bits in a register are set
 *
 * This function blocks until all specified bits in a register are set.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to check
 */
inline void wait_until_set(const volatile uint32_t* reg, uint32_t bits) {
    wait_until_match(reg, bits, bits);
}

/**
 * @brief Wait until specified bits in a register are clear
 *
 * This function blocks until all specified bits in a register are clear.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to check
 */
inline void wait_until_clear(const volatile uint32_t* reg, uint32_t bits) {
    wait_until_match(reg, bits, 0);
}

/**
 * @brief Atomic bit set operation
 *
 * This function atomically sets bits in a register.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to set
 *
 * @note Only works for registers that have atomic bit set/clear registers
 */
template <typename T> inline void atomic_set_bits(volatile T* reg, uint32_t bits) {
    // For STM32F4xx, registers that support atomic bit operations have a set
    // register at offset 0x18 from the base register
    auto set_reg = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uintptr_t>(reg) + 0x18);
    *set_reg = bits;
}

/**
 * @brief Atomic bit clear operation
 *
 * This function atomically clears bits in a register.
 *
 * @param reg Pointer to the register
 * @param bits Bitmask of bits to clear
 *
 * @note Only works for registers that have atomic bit set/clear registers
 */
template <typename T> inline void atomic_clear_bits(volatile T* reg, uint32_t bits) {
    // For STM32F4xx, registers that support atomic bit operations have a reset
    // register at offset 0x1C from the base register
    auto reset_reg = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uintptr_t>(reg) + 0x1C);
    *reset_reg = bits;
}

// RCC (Reset and Clock Control) register bit definitions
namespace rcc {
// AHB1 peripheral clock enable register (RCC_AHB1ENR)
constexpr uint32_t AHB1ENR_GPIOAEN = (1U << 0);  ///< GPIOA clock enable
constexpr uint32_t AHB1ENR_GPIOBEN = (1U << 1);  ///< GPIOB clock enable
constexpr uint32_t AHB1ENR_GPIOCEN = (1U << 2);  ///< GPIOC clock enable
constexpr uint32_t AHB1ENR_GPIODEN = (1U << 3);  ///< GPIOD clock enable
constexpr uint32_t AHB1ENR_GPIOEEN = (1U << 4);  ///< GPIOE clock enable
constexpr uint32_t AHB1ENR_GPIOFEN = (1U << 5);  ///< GPIOF clock enable
constexpr uint32_t AHB1ENR_GPIOGEN = (1U << 6);  ///< GPIOG clock enable
constexpr uint32_t AHB1ENR_GPIOHEN = (1U << 7);  ///< GPIOH clock enable
constexpr uint32_t AHB1ENR_GPIOIEN = (1U << 8);  ///< GPIOI clock enable
constexpr uint32_t AHB1ENR_GPIOJEN = (1U << 9);  ///< GPIOJ clock enable
constexpr uint32_t AHB1ENR_GPIOKEN = (1U << 10); ///< GPIOK clock enable
constexpr uint32_t AHB1ENR_DMA1EN = (1U << 21);  ///< DMA1 clock enable
constexpr uint32_t AHB1ENR_DMA2EN = (1U << 22);  ///< DMA2 clock enable

// APB1 peripheral clock enable register (RCC_APB1ENR)
constexpr uint32_t APB1ENR_USART2EN = (1U << 17); ///< USART2 clock enable
constexpr uint32_t APB1ENR_USART3EN = (1U << 18); ///< USART3 clock enable
constexpr uint32_t APB1ENR_UART4EN = (1U << 19);  ///< UART4 clock enable
constexpr uint32_t APB1ENR_UART5EN = (1U << 20);  ///< UART5 clock enable
constexpr uint32_t APB1ENR_UART7EN = (1U << 30);  ///< UART7 clock enable
constexpr uint32_t APB1ENR_UART8EN = (1U << 31);  ///< UART8 clock enable

// APB2 peripheral clock enable register (RCC_APB2ENR)
constexpr uint32_t APB2ENR_USART1EN = (1U << 4); ///< USART1 clock enable
constexpr uint32_t APB2ENR_USART6EN = (1U << 5); ///< USART6 clock enable
} // namespace rcc

// USART register bit definitions
namespace usart {
// Control register 1 (USART_CR1)
constexpr uint32_t CR1_UE = (1U << 13);    ///< USART enable
constexpr uint32_t CR1_M = (1U << 12);     ///< Word length (0: 8 data bits, 1: 9 data bits)
constexpr uint32_t CR1_WAKE = (1U << 11);  ///< Wakeup method
constexpr uint32_t CR1_PCE = (1U << 10);   ///< Parity control enable
constexpr uint32_t CR1_PS = (1U << 9);     ///< Parity selection (0: even, 1: odd)
constexpr uint32_t CR1_PEIE = (1U << 8);   ///< PE interrupt enable
constexpr uint32_t CR1_TXEIE = (1U << 7);  ///< TXE interrupt enable
constexpr uint32_t CR1_TCIE = (1U << 6);   ///< TC interrupt enable
constexpr uint32_t CR1_RXNEIE = (1U << 5); ///< RXNE interrupt enable
constexpr uint32_t CR1_IDLEIE = (1U << 4); ///< IDLE interrupt enable
constexpr uint32_t CR1_TE = (1U << 3);     ///< Transmitter enable
constexpr uint32_t CR1_RE = (1U << 2);     ///< Receiver enable
constexpr uint32_t CR1_RWU = (1U << 1);    ///< Receiver wakeup
constexpr uint32_t CR1_SBK = (1U << 0);    ///< Send break

// Control register 2 (USART_CR2)
constexpr uint32_t CR2_STOP_MASK = (3U << 12); ///< STOP bits mask
constexpr uint32_t CR2_STOP_1 = (0U << 12);    ///< 1 stop bit
constexpr uint32_t CR2_STOP_0_5 = (1U << 12);  ///< 0.5 stop bits
constexpr uint32_t CR2_STOP_2 = (2U << 12);    ///< 2 stop bits
constexpr uint32_t CR2_STOP_1_5 = (3U << 12);  ///< 1.5 stop bits

// Control register 3 (USART_CR3)
constexpr uint32_t CR3_DMAT = (1U << 7);    ///< DMA enable transmitter
constexpr uint32_t CR3_DMAR = (1U << 6);    ///< DMA enable receiver
constexpr uint32_t CR3_RTSE = (1U << 8);    ///< RTS enable
constexpr uint32_t CR3_CTSE = (1U << 9);    ///< CTS enable
constexpr uint32_t CR3_CTSIE = (1U << 10);  ///< CTS interrupt enable
constexpr uint32_t CR3_ONEBIT = (1U << 11); ///< One sample bit method enable

// Status register (USART_SR)
constexpr uint32_t SR_PE = (1U << 0);   ///< Parity error
constexpr uint32_t SR_FE = (1U << 1);   ///< Framing error
constexpr uint32_t SR_NE = (1U << 2);   ///< Noise error
constexpr uint32_t SR_ORE = (1U << 3);  ///< Overrun error
constexpr uint32_t SR_IDLE = (1U << 4); ///< IDLE line detected
constexpr uint32_t SR_RXNE = (1U << 5); ///< Read data register not empty
constexpr uint32_t SR_TC = (1U << 6);   ///< Transmission complete
constexpr uint32_t SR_TXE = (1U << 7);  ///< Transmit data register empty
constexpr uint32_t SR_LBD = (1U << 8);  ///< LIN break detection
constexpr uint32_t SR_CTS = (1U << 9);  ///< CTS flag
} // namespace usart

// GPIO register bit definitions
namespace gpio {
// GPIO mode register (GPIOx_MODER)
constexpr uint32_t MODER_INPUT = 0x0U;     ///< Input mode
constexpr uint32_t MODER_OUTPUT = 0x1U;    ///< Output mode
constexpr uint32_t MODER_ALTERNATE = 0x2U; ///< Alternate function mode
constexpr uint32_t MODER_ANALOG = 0x3U;    ///< Analog mode

// GPIO output type register (GPIOx_OTYPER)
constexpr uint32_t OTYPER_PUSH_PULL = 0x0U;  ///< Push-pull output
constexpr uint32_t OTYPER_OPEN_DRAIN = 0x1U; ///< Open-drain output

// GPIO output speed register (GPIOx_OSPEEDR)
constexpr uint32_t OSPEEDR_LOW = 0x0U;       ///< Low speed
constexpr uint32_t OSPEEDR_MEDIUM = 0x1U;    ///< Medium speed
constexpr uint32_t OSPEEDR_HIGH = 0x2U;      ///< High speed
constexpr uint32_t OSPEEDR_VERY_HIGH = 0x3U; ///< Very high speed

// GPIO pull-up/pull-down register (GPIOx_PUPDR)
constexpr uint32_t PUPDR_NONE = 0x0U;      ///< No pull-up/pull-down
constexpr uint32_t PUPDR_PULL_UP = 0x1U;   ///< Pull-up
constexpr uint32_t PUPDR_PULL_DOWN = 0x2U; ///< Pull-down
} // namespace gpio

// DMA register bit definitions
namespace dma {
// DMA stream configuration register (DMA_SxCR)
constexpr uint32_t SxCR_EN = (1U << 0);               ///< Stream enable
constexpr uint32_t SxCR_DMEIE = (1U << 1);            ///< Direct mode error interrupt enable
constexpr uint32_t SxCR_TEIE = (1U << 2);             ///< Transfer error interrupt enable
constexpr uint32_t SxCR_HTIE = (1U << 3);             ///< Half transfer interrupt enable
constexpr uint32_t SxCR_TCIE = (1U << 4);             ///< Transfer complete interrupt enable
constexpr uint32_t SxCR_PFCTRL = (1U << 5);           ///< Peripheral flow controller
constexpr uint32_t SxCR_DIR_MASK = (3U << 6);         ///< Data transfer direction mask
constexpr uint32_t SxCR_DIR_P2M = (0U << 6);          ///< Peripheral to memory
constexpr uint32_t SxCR_DIR_M2P = (1U << 6);          ///< Memory to peripheral
constexpr uint32_t SxCR_DIR_M2M = (2U << 6);          ///< Memory to memory
constexpr uint32_t SxCR_CIRC = (1U << 8);             ///< Circular mode
constexpr uint32_t SxCR_PINC = (1U << 9);             ///< Peripheral increment mode
constexpr uint32_t SxCR_MINC = (1U << 10);            ///< Memory increment mode
constexpr uint32_t SxCR_PSIZE_MASK = (3U << 11);      ///< Peripheral data size mask
constexpr uint32_t SxCR_PSIZE_BYTE = (0U << 11);      ///< Peripheral data size: Byte
constexpr uint32_t SxCR_PSIZE_HALF_WORD = (1U << 11); ///< Peripheral data size: Half-word
constexpr uint32_t SxCR_PSIZE_WORD = (2U << 11);      ///< Peripheral data size: Word
constexpr uint32_t SxCR_MSIZE_MASK = (3U << 13);      ///< Memory data size mask
constexpr uint32_t SxCR_MSIZE_BYTE = (0U << 13);      ///< Memory data size: Byte
constexpr uint32_t SxCR_MSIZE_HALF_WORD = (1U << 13); ///< Memory data size: Half-word
constexpr uint32_t SxCR_MSIZE_WORD = (2U << 13);      ///< Memory data size: Word
constexpr uint32_t SxCR_PINCOS = (1U << 15);          ///< Peripheral increment offset size
constexpr uint32_t SxCR_PL_MASK = (3U << 16);         ///< Priority level mask
constexpr uint32_t SxCR_PL_LOW = (0U << 16);          ///< Priority level: Low
constexpr uint32_t SxCR_PL_MEDIUM = (1U << 16);       ///< Priority level: Medium
constexpr uint32_t SxCR_PL_HIGH = (2U << 16);         ///< Priority level: High
constexpr uint32_t SxCR_PL_VERY_HIGH = (3U << 16);    ///< Priority level: Very high
constexpr uint32_t SxCR_DBM = (1U << 18);             ///< Double buffer mode
constexpr uint32_t SxCR_CT = (1U << 19);              ///< Current target (only in double buffer mode)
constexpr uint32_t SxCR_CHSEL_MASK = (7U << 25);      ///< Channel selection mask
constexpr uint32_t SxCR_CHSEL_SHIFT = 25;             ///< Channel selection shift

// DMA stream FIFO control register (DMA_SxFCR)
constexpr uint32_t SxFCR_FTH_MASK = (3U << 0); ///< FIFO threshold selection mask
constexpr uint32_t SxFCR_FTH_1_4 = (0U << 0);  ///< FIFO threshold 1/4 full
constexpr uint32_t SxFCR_FTH_2_4 = (1U << 0);  ///< FIFO threshold 2/4 full
constexpr uint32_t SxFCR_FTH_3_4 = (2U << 0);  ///< FIFO threshold 3/4 full
constexpr uint32_t SxFCR_FTH_4_4 = (3U << 0);  ///< FIFO threshold 4/4 full
constexpr uint32_t SxFCR_DMDIS = (1U << 2);    ///< Direct mode disable
constexpr uint32_t SxFCR_FEIE = (1U << 7);     ///< FIFO error interrupt enable

// DMA interrupt status register (DMA_LISR, DMA_HISR)
constexpr uint32_t LISR_FEIF0 = (1U << 0);  ///< Stream 0 FIFO error interrupt flag
constexpr uint32_t LISR_DMEIF0 = (1U << 2); ///< Stream 0 direct mode error interrupt flag
constexpr uint32_t LISR_TEIF0 = (1U << 3);  ///< Stream 0 transfer error interrupt flag
constexpr uint32_t LISR_HTIF0 = (1U << 4);  ///< Stream 0 half transfer interrupt flag
constexpr uint32_t LISR_TCIF0 = (1U << 5);  ///< Stream 0 transfer complete interrupt flag

// DMA interrupt flag clear register (DMA_LIFCR, DMA_HIFCR)
constexpr uint32_t LIFCR_CFEIF0 = (1U << 0);  ///< Stream 0 clear FIFO error interrupt flag
constexpr uint32_t LIFCR_CDMEIF0 = (1U << 2); ///< Stream 0 clear direct mode error interrupt flag
constexpr uint32_t LIFCR_CTEIF0 = (1U << 3);  ///< Stream 0 clear transfer error interrupt flag
constexpr uint32_t LIFCR_CHTIF0 = (1U << 4);  ///< Stream 0 clear half transfer interrupt flag
constexpr uint32_t LIFCR_CTCIF0 = (1U << 5);  ///< Stream 0 clear transfer complete interrupt flag
} // namespace dma

// NVIC register bit definitions
namespace nvic {
// NVIC set-enable registers (NVIC_ISER)
constexpr uint32_t ISER_SETENA = 0xFFFFFFFF; ///< Interrupt set-enable bits

// NVIC clear-enable registers (NVIC_ICER)
constexpr uint32_t ICER_CLRENA = 0xFFFFFFFF; ///< Interrupt clear-enable bits

// NVIC set-pending registers (NVIC_ISPR)
constexpr uint32_t ISPR_SETPEND = 0xFFFFFFFF; ///< Interrupt set-pending bits

// NVIC clear-pending registers (NVIC_ICPR)
constexpr uint32_t ICPR_CLRPEND = 0xFFFFFFFF; ///< Interrupt clear-pending bits

// NVIC priority registers (NVIC_IPR)
constexpr uint32_t IPR_PRI_MASK = 0xFF; ///< Priority mask
} // namespace nvic

} // namespace reg
} // namespace stm32
} // namespace platform
} // namespace hm11

#endif // HM11_PLATFORM_STM32_REGISTERS_HPP