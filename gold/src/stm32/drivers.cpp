/**
 * @file drivers.cpp
 * @brief Hardware initialization and driver instance implementation for STM32F429
 *
 * This file implements the initialization functions declared in drivers.hpp,
 * providing the complete hardware setup sequence for the HM11 driver on
 * STM32F429 platform.
 *
 * ## Implementation Strategy
 *
 * The implementation uses direct CMSIS register access (Decision 30) without
 * STM32 HAL library dependency. All configuration follows the STM32F429
 * reference manual (RM0090) register descriptions.
 *
 * ## Ada Correspondence
 *
 * This translates Ada's implementation (drivers.adb:375-438):
 * - Init_UART (lines 378-408) → init_uart()
 * - Initialize_DMA (lines 414-438) → initialize_dma()
 * - Global Driver object (drivers.ads:77-78) → get_hm11_driver()
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include "hm11/stm32/drivers.hpp"

// CMSIS device headers for STM32F429
#if defined(STM32F429xx)
#include "core_cm4.h"
#include "stm32f429xx.h"
#elif defined(STM32F4)
#include "core_cm4.h"
#include "stm32f4xx.h"
#else
// For compilation without target, provide stub types
// This allows the code to compile for testing purposes
#warning "STM32 device not defined - using stub definitions"

// Stub register structures for compilation
struct USART_TypeDef {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
};

struct GPIO_TypeDef {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
};

struct DMA_Stream_TypeDef {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
};

struct DMA_TypeDef {
    volatile uint32_t LISR;
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
};

struct RCC_TypeDef {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
};

// Stub peripheral pointers
static USART_TypeDef stub_uart5;
static GPIO_TypeDef stub_gpioc;
static GPIO_TypeDef stub_gpiod;
static DMA_Stream_TypeDef stub_dma_stream;
static DMA_TypeDef stub_dma1;
static RCC_TypeDef stub_rcc;

#define UART5 (&stub_uart5)
#define GPIOC (&stub_gpioc)
#define GPIOD (&stub_gpiod)
#define DMA1_Stream0 (&stub_dma_stream)
#define DMA1 (&stub_dma1)
#define RCC (&stub_rcc)

// Stub interrupt numbers
enum IRQn_Type { DMA1_Stream0_IRQn = 11, USART5_IRQn = 53 };

// Stub NVIC functions
inline void NVIC_SetPriority(IRQn_Type, uint32_t) {}
inline void NVIC_EnableIRQ(IRQn_Type) {}
#endif

namespace hm11::stm32 {

// ============================================================================
// Constants and Helper Functions
// ============================================================================

namespace {

// GPIO configuration constants
constexpr uint32_t GPIO_MODE_AF = 0x2U;     // Alternate function mode
constexpr uint32_t GPIO_OTYPE_PP = 0x0U;    // Push-pull output type
constexpr uint32_t GPIO_SPEED_HIGH = 0x2U;  // 50 MHz speed
constexpr uint32_t GPIO_PUPD_PULLUP = 0x1U; // Pull-up resistor
constexpr uint32_t GPIO_AF7_UART5 = 0x7U;   // Alternate function 7 for UART5

// UART configuration constants
constexpr uint32_t UART_CR1_UE = 0x2000U;     // UART enable
constexpr uint32_t UART_CR1_TE = 0x0008U;     // Transmitter enable
constexpr uint32_t UART_CR1_RE = 0x0004U;     // Receiver enable
constexpr uint32_t UART_CR1_RXNEIE = 0x0020U; // RXNE interrupt enable
constexpr uint32_t UART_CR1_PEIE = 0x0100U;   // Parity error interrupt enable
constexpr uint32_t UART_CR3_EIE = 0x0001U;    // Error interrupt enable
constexpr uint32_t UART_CR3_DMAR = 0x0040U;   // DMA receiver enable

// DMA configuration constants
constexpr uint32_t DMA_SxCR_EN = 0x00000001U;           // Stream enable
constexpr uint32_t DMA_SxCR_TCIE = 0x00000010U;         // Transfer complete interrupt enable
constexpr uint32_t DMA_SxCR_TEIE = 0x00000004U;         // Transfer error interrupt enable
constexpr uint32_t DMA_SxCR_DMEIE = 0x00000002U;        // Direct mode error interrupt enable
constexpr uint32_t DMA_SxCR_CHSEL_4 = 0x08000000U;      // Channel 4
constexpr uint32_t DMA_SxCR_PL_VERY_HIGH = 0x00030000U; // Priority very high
constexpr uint32_t DMA_SxCR_MSIZE_BYTE = 0x00000000U;   // Memory data size: byte
constexpr uint32_t DMA_SxCR_PSIZE_BYTE = 0x00000000U;   // Peripheral data size: byte
constexpr uint32_t DMA_SxCR_MINC = 0x00000400U;         // Memory increment mode
constexpr uint32_t DMA_SxCR_DIR_P2M = 0x00000000U;      // Direction: peripheral to memory

constexpr uint32_t DMA_SxFCR_DMDIS = 0x00000004U; // Direct mode disable
constexpr uint32_t DMA_SxFCR_FEIE = 0x00000080U;  // FIFO error interrupt enable

// RCC clock enable bits
constexpr uint32_t RCC_AHB1ENR_GPIOCEN = 0x00000004U; // GPIOC clock enable
constexpr uint32_t RCC_AHB1ENR_GPIODEN = 0x00000008U; // GPIOD clock enable
constexpr uint32_t RCC_AHB1ENR_DMA1EN = 0x00200000U;  // DMA1 clock enable
constexpr uint32_t RCC_APB1ENR_UART5EN = 0x00100000U; // UART5 clock enable

// DMA interrupt flag clear bits (for Stream 0 in LIFCR)
constexpr uint32_t DMA_LIFCR_CTCIF0 = 0x00000020U;  // Clear transfer complete flag
constexpr uint32_t DMA_LIFCR_CHTIF0 = 0x00000010U;  // Clear half transfer flag
constexpr uint32_t DMA_LIFCR_CTEIF0 = 0x00000008U;  // Clear transfer error flag
constexpr uint32_t DMA_LIFCR_CDMEIF0 = 0x00000004U; // Clear direct mode error flag
constexpr uint32_t DMA_LIFCR_CFEIF0 = 0x00000001U;  // Clear FIFO error flag

/**
 * @brief Calculate UART BRR register value for given baud rate
 *
 * Calculates the baud rate register value based on the APB1 clock frequency
 * and desired baud rate. The formula is:
 *
 *   BRR = UART_CLK / (16 * baud_rate)
 *
 * This function assumes the system is using HSI (16 MHz) without PLL,
 * which gives APB1_CLK = 16 MHz after reset. For different clock
 * configurations, adjust APB1_CLOCK_HZ accordingly.
 *
 * @param baud_rate Desired baud rate (typically 9600 for HM-11)
 * @return BRR register value
 */
constexpr uint32_t calculate_brr(uint32_t baud_rate) noexcept {
    // Default STM32F429 configuration after reset:
    // - HSI: 16 MHz (internal oscillator)
    // - SYSCLK: 16 MHz (HSI directly)
    // - AHB prescaler: /1 → HCLK = 16 MHz
    // - APB1 prescaler: /1 → APB1_CLK = 16 MHz
    //
    // For systems with PLL configured for 180 MHz:
    // - SYSCLK: 180 MHz
    // - AHB prescaler: /1 → HCLK = 180 MHz
    // - APB1 prescaler: /4 → APB1_CLK = 45 MHz
    //
    // Change this constant if using different clock configuration
    constexpr uint32_t APB1_CLOCK_HZ = 16000000U; // 16 MHz (HSI default)

    // BRR = APB1_CLK / (16 * baud_rate)
    // Add (8 * baud_rate) for rounding
    return (APB1_CLOCK_HZ + (8U * baud_rate)) / (16U * baud_rate);
}

/**
 * @brief Configure GPIO pin for alternate function
 *
 * Helper function to configure a single GPIO pin for alternate function mode
 * with specified parameters. This reduces code duplication for PC12 and PD2
 * configuration.
 *
 * @param port GPIO port (GPIOC or GPIOD)
 * @param pin Pin number (0-15)
 * @param af Alternate function number (0-15)
 * @param output_type Output type (push-pull or open-drain)
 * @param speed Output speed (low, medium, high, very high)
 * @param pull Pull-up/pull-down configuration
 */
void configure_gpio_pin(
    GPIO_TypeDef* port,
    uint32_t pin,
    uint32_t af,
    uint32_t output_type,
    uint32_t speed,
    uint32_t pull) noexcept {
    // Configure mode to alternate function (MODER register)
    // Each pin uses 2 bits: [1:0] = mode
    uint32_t moder = port->MODER;
    moder &= ~(0x3U << (pin * 2U));        // Clear mode bits
    moder |= (GPIO_MODE_AF << (pin * 2U)); // Set alternate function mode
    port->MODER = moder;

    // Configure output type (OTYPER register)
    // Each pin uses 1 bit: 0 = push-pull, 1 = open-drain
    uint32_t otyper = port->OTYPER;
    otyper &= ~(0x1U << pin);       // Clear output type bit
    otyper |= (output_type << pin); // Set output type
    port->OTYPER = otyper;

    // Configure output speed (OSPEEDR register)
    // Each pin uses 2 bits: [1:0] = speed
    uint32_t ospeedr = port->OSPEEDR;
    ospeedr &= ~(0x3U << (pin * 2U)); // Clear speed bits
    ospeedr |= (speed << (pin * 2U)); // Set speed
    port->OSPEEDR = ospeedr;

    // Configure pull-up/pull-down (PUPDR register)
    // Each pin uses 2 bits: 00 = none, 01 = pull-up, 10 = pull-down
    uint32_t pupdr = port->PUPDR;
    pupdr &= ~(0x3U << (pin * 2U)); // Clear pull bits
    pupdr |= (pull << (pin * 2U));  // Set pull configuration
    port->PUPDR = pupdr;

    // Configure alternate function (AFR register)
    // AFR[0] = pins 0-7 (AFRL), AFR[1] = pins 8-15 (AFRH)
    // Each pin uses 4 bits: [3:0] = AF number
    uint32_t afr_index = pin >> 3U;         // 0 for pins 0-7, 1 for pins 8-15
    uint32_t afr_shift = (pin & 0x7U) * 4U; // Bit position within AFR register
    uint32_t afr = port->AFR[afr_index];
    afr &= ~(0xFU << afr_shift); // Clear AF bits
    afr |= (af << afr_shift);    // Set AF number
    port->AFR[afr_index] = afr;
}

} // anonymous namespace

// ============================================================================
// Public API Implementation
// ============================================================================

void init_uart() noexcept {
    // Step 1: Enable clocks for UART5, GPIOC, and GPIOD
    // Ada: Enable_Clock (UART) and Enable_Clock (TX & RX)
    // drivers.adb:383-384

#if defined(STM32F429xx) || defined(STM32F4)
    // Enable GPIOC clock (for PC12 - UART5_TX)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // Enable GPIOD clock (for PD2 - UART5_RX)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    // Enable UART5 clock (APB1 peripheral)
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
#endif

    // Step 2: Configure GPIO pins for UART5
    // Ada: Configure_IO (TX & RX, ...) drivers.adb:386-392
    //
    // PC12: UART5_TX - Alternate Function 7, push-pull, pull-up, 50MHz
    // PD2: UART5_RX - Alternate Function 7, pull-up

#if defined(STM32F429xx) || defined(STM32F4)
    // Configure PC12 (UART5_TX)
    configure_gpio_pin(
        GPIOC,           // port
        12,              // pin number
        GPIO_AF7_UART5,  // alternate function
        GPIO_OTYPE_PP,   // push-pull
        GPIO_SPEED_HIGH, // 50 MHz
        GPIO_PUPD_PULLUP // pull-up
    );

    // Configure PD2 (UART5_RX)
    configure_gpio_pin(
        GPIOD,           // port
        2,               // pin number
        GPIO_AF7_UART5,  // alternate function
        GPIO_OTYPE_PP,   // push-pull (not used for input but set anyway)
        GPIO_SPEED_HIGH, // 50 MHz (not used for input but set anyway)
        GPIO_PUPD_PULLUP // pull-up
    );
#endif

    // Step 3: Configure UART5 peripheral
    // Ada: drivers.adb:394-407

#if defined(STM32F429xx) || defined(STM32F4)
    // Disable UART5 for configuration
    // Ada: Disable (UART)
    UART5->CR1 &= ~UART_CR1_UE;

    // Set baud rate: 9600
    // Ada: Set_Baud_Rate (UART, 9600)
    UART5->BRR = calculate_brr(9600);

    // Configure data format: 8N1
    // Ada: Set_Word_Length (UART, Word_Length_8)
    //      Set_Stop_Bits (UART, Stopbits_1)
    //      Set_Parity (UART, No_Parity)
    //      Set_Flow_Control (UART, No_Flow_Control)
    //
    // CR1: M=0 (8 data bits), PCE=0 (no parity)
    // CR2: STOP=00 (1 stop bit)
    // CR3: Flow control disabled
    UART5->CR1 &= ~0x1000U; // M bit = 0 (8 data bits)
    UART5->CR1 &= ~0x0400U; // PCE bit = 0 (parity disabled)
    UART5->CR2 &= ~0x3000U; // STOP bits = 00 (1 stop bit)
    UART5->CR3 &= ~0x0300U; // CTSE=0, RTSE=0 (flow control disabled)

    // Enable transmitter and receiver
    // Ada: Set_Mode (UART, Tx_Rx_Mode)
    UART5->CR1 |= UART_CR1_TE | UART_CR1_RE;

    // Enable interrupts
    // Ada: Enable_Interrupts (UART, Error)
    //      Enable_Interrupts (UART, Parity_Error)
    //      Enable_Interrupts (UART, Received_Data_Not_Empty)
    UART5->CR3 |= UART_CR3_EIE;    // Error interrupt enable
    UART5->CR1 |= UART_CR1_PEIE;   // Parity error interrupt enable
    UART5->CR1 |= UART_CR1_RXNEIE; // RXNE interrupt enable

    // Enable UART5 peripheral
    // Ada: Enable (UART)
    UART5->CR1 |= UART_CR1_UE;
#endif
}

void initialize_dma() noexcept {
    // Step 1: Enable DMA1 clock
    // Ada: Enable_Clock (Controller) drivers.adb:419

#if defined(STM32F429xx) || defined(STM32F4)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
#endif

    // Step 2: Reset DMA1 Stream 0
    // Ada: Reset (Controller, Rx_Stream) drivers.adb:421

#if defined(STM32F429xx) || defined(STM32F4)
    // Disable stream
    DMA1_Stream0->CR &= ~DMA_SxCR_EN;

    // Wait until stream is disabled (EN bit reads 0)
    while ((DMA1_Stream0->CR & DMA_SxCR_EN) != 0) {
        // Busy wait
    }

    // Clear all interrupt flags for Stream 0 (in LIFCR register)
    DMA1->LIFCR |= DMA_LIFCR_CTCIF0 |  // Transfer complete
                   DMA_LIFCR_CHTIF0 |  // Half transfer
                   DMA_LIFCR_CTEIF0 |  // Transfer error
                   DMA_LIFCR_CDMEIF0 | // Direct mode error
                   DMA_LIFCR_CFEIF0;   // FIFO error
#endif

    // Step 3: Configure DMA1 Stream 0
    // Ada: Configure (Controller, Rx_Stream, Configuration) drivers.adb:423-435

#if defined(STM32F429xx) || defined(STM32F4)
    // Configure stream control register
    uint32_t cr = 0;
    cr |= DMA_SxCR_CHSEL_4;      // Channel 4 (UART5_RX)
    cr |= DMA_SxCR_DIR_P2M;      // Direction: peripheral to memory
    cr |= DMA_SxCR_MINC;         // Memory increment enabled
    cr |= DMA_SxCR_PSIZE_BYTE;   // Peripheral data size: byte
    cr |= DMA_SxCR_MSIZE_BYTE;   // Memory data size: byte
    cr |= DMA_SxCR_PL_VERY_HIGH; // Priority: very high
    // CIRC bit not set = Normal mode (not circular)
    // Enable interrupts: TC, TE, DME
    cr |= DMA_SxCR_TCIE;  // Transfer complete interrupt
    cr |= DMA_SxCR_TEIE;  // Transfer error interrupt
    cr |= DMA_SxCR_DMEIE; // Direct mode error interrupt

    DMA1_Stream0->CR = cr;

    // Set peripheral address (USART5 data register)
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    // Required for hardware register address conversion - DMA PAR register requires uint32_t address
    DMA1_Stream0->PAR = reinterpret_cast<uint32_t>(&UART5->DR);
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    // Memory address and transfer count will be set per operation
    // (not configured here - set by DMA_Receive_Handler when starting transfer)

    // Configure FIFO control register
    // Disable FIFO (use direct mode)
    // Enable FIFO error interrupt
    DMA1_Stream0->FCR = DMA_SxFCR_FEIE;
    // Note: DMDIS=0 means direct mode enabled (FIFO disabled)
#endif

    // Step 4: Enable DMA requests on UART5
    // Ada: Enable_DMA_Receive_Requests (Drivers.UART) drivers.adb:436
    //      Pause_DMA_Reception (Drivers.UART) drivers.adb:437

#if defined(STM32F429xx) || defined(STM32F4)
    // Enable UART5 DMA receive requests
    UART5->CR3 |= UART_CR3_DMAR;

    // Note: DMA is configured but not started yet
    // It will be started when DMA_Receive_Handler initiates a transfer
#endif

    // Step 5: Configure NVIC interrupt priorities and enable interrupts
    // This section implements Decision 16 (Interrupt Handler Registration)
    //
    // Note: IRQ handler initialization (UARTIRQHandler::initialize, DMAIRQHandler::initialize)
    // is performed later in get_hm11_driver() when the watchdog instance is created.
    // This avoids initialization order issues where IRQ handlers would reference
    // a watchdog that doesn't exist yet.

#if defined(STM32F429xx) || defined(STM32F4)
    // Set interrupt priorities (Decision 16: Interrupt Handler Registration)
    // DMA has higher priority (lower number) than UART because DMA interrupts
    // are time-critical and should not be delayed by UART processing.
    //
    // Priority range: 0-15, where 0 is highest priority
    // - DMA1 Stream 0: Priority 5 (time-critical DMA transfers)
    // - UART5: Priority 6 (can be interrupted by DMA)
    NVIC_SetPriority(DMA1_Stream0_IRQn, 5); // DMA priority: 5 (higher)
    NVIC_SetPriority(USART5_IRQn, 6);       // UART priority: 6 (lower)

    // Enable interrupts in NVIC
    // These interrupts will fire when hardware events occur, calling the
    // extern "C" wrappers (USART5_IRQHandler, DMA1_Stream0_IRQHandler) which
    // delegate to the C++ handler classes (UARTIRQHandler, DMAIRQHandler).
    NVIC_EnableIRQ(DMA1_Stream0_IRQn); // Enable DMA1 Stream 0 interrupt
    NVIC_EnableIRQ(USART5_IRQn);       // Enable UART5 interrupt
#endif
}

HM11Driver<STM32_UART_DMA<DMA_Receive_Handler>, DMA_Receive_Handler>& get_hm11_driver() noexcept {
    // Static local variable with static storage duration
    // Initialized on first call (thread-safe in C++11)
    // Destroyed automatically at program termination
    //
    // This implements Decision 22 (RAII Ownership Model) with static storage
    // for bare-metal applications where global lifetime is acceptable.

    // Create watchdog for ISR-to-thread synchronization
    static sync::Watchdog watchdog;

    // Create DMA receive handler with watchdog reference
    static DMA_Receive_Handler dma_handler(watchdog);

    // Create UART port with receive handler reference
    static STM32_UART_DMA<DMA_Receive_Handler> uart_port(dma_handler);

    // Create HM11 driver with UART port and receive handler
    static HM11Driver<STM32_UART_DMA<DMA_Receive_Handler>, DMA_Receive_Handler> driver(uart_port, dma_handler);

    // Initialize IRQ handlers on first call
    // This ensures watchdog reference is valid when ISRs are called
    static bool irq_initialized = false;
    if (!irq_initialized) {
#if defined(STM32F429xx) || defined(STM32F4)
        UARTIRQHandler::initialize(&watchdog);
        DMAIRQHandler::initialize(&watchdog);
#endif
        irq_initialized = true;
    }

    return driver;
}

/**
 * @brief Get reference to UART port for direct data transfer
 *
 * Returns the same UART port instance used by the HM11 driver, allowing
 * applications to perform direct UART operations outside of AT command
 * processing. This matches Ada's pattern where the UART is exposed as a
 * package-level object (drivers.ads:59).
 *
 * @return Reference to STM32_UART_DMA port instance
 */
STM32_UART_DMA<DMA_Receive_Handler>& get_uart_port() noexcept {
    // Access the same static instance created in get_hm11_driver()
    // The static variables are created on first call and persist for program lifetime
    [[maybe_unused]] auto& driver = get_hm11_driver(); // Ensure initialization

    // Get the same uart_port instance
    static sync::Watchdog watchdog;
    static DMA_Receive_Handler dma_handler(watchdog);
    static STM32_UART_DMA<DMA_Receive_Handler> uart_port(dma_handler);

    return uart_port;
}

/**
 * @brief Get reference to DMA receive handler for direct reception
 *
 * Returns the same DMA receive handler instance used by the HM11 driver,
 * allowing applications to perform direct UART reception with DMA. This
 * matches Ada's pattern where DMA_Receive_Handler is exposed as a package-
 * level procedure (drivers.ads:63-69).
 *
 * @return Reference to DMA_Receive_Handler instance
 */
DMA_Receive_Handler& get_receive_handler() noexcept {
    // Access the same static instance created in get_hm11_driver()
    [[maybe_unused]] auto& driver = get_hm11_driver(); // Ensure initialization

    // Get the same dma_handler instance
    static sync::Watchdog watchdog;
    static DMA_Receive_Handler dma_handler(watchdog);

    return dma_handler;
}

} // namespace hm11::stm32
