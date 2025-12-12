#pragma once
#define HM11_STM32_UART_PORT_HPP_INCLUDED

// HM11 Hardware Abstraction Layer - STM32 UART Port Implementation
// Direct CMSIS register access implementation for STM32F429 UART5
// Matches Ada_Drivers_Library register-level approach (Decision 30)

#include <chrono>
#include <cstdint>

#include "hm11/hal/uart_interface.hpp"
#include "hm11/hal/uart_types.hpp"
#include "hm11/sync/binary_semaphore.hpp"

// Forward declare CMSIS types to avoid including STM32 headers in public API
struct USART_TypeDef;
struct DMA_Stream_TypeDef;
struct DMA_TypeDef;
struct GPIO_TypeDef;

namespace hm11::hal {

/// STM32F429-specific UART Port implementation using UART5
/// Provides concrete implementation of UART interface using direct CMSIS register access
/// Equivalent to Ada's STM32.USARTs.USART configuration with DMA1 Stream0 Channel4
///
/// Hardware Configuration:
/// - UART5 peripheral (APB1 bus, 84MHz max)
/// - TX: PC12 (GPIO_AF8, connects to HM-11 RX pin 4)
/// - RX: PD2 (GPIO_AF8, connects to HM-11 TX pin 2)
/// - DMA1 Stream0 Channel4 for receive operations
/// - 9600 baud, 8N1, no flow control (HM-11 default)
///
/// Usage Pattern:
/// 1. Construct with hardware initialization
/// 2. Use transmit/receive methods (satisfies UARTInterface concept)
/// 3. Automatic cleanup on destruction
///
/// Safety Features:
/// - RAII hardware resource management
/// - DMA buffer alignment (32-byte, Decision 20)
/// - ISR-safe synchronization with BinarySemaphore
/// - Timeout-based error recovery
/// - Non-copyable, non-movable (Decision 22)
class STM32UARTPort {
  public:
    /// Hardware pins configuration
    /// Matches Ada's TX/RX GPIO_Point aliases (PC12/PD2)
    struct PinConfig {
        std::uint8_t tx_port; ///< GPIO port number for TX (2 = GPIOC for PC12)
        std::uint8_t tx_pin;  ///< GPIO pin number for TX (12 for PC12)
        std::uint8_t rx_port; ///< GPIO port number for RX (3 = GPIOD for PD2)
        std::uint8_t rx_pin;  ///< GPIO pin number for RX (2 for PD2)
    };

    /// Default pin configuration for STM32F429 Discovery + HM-11
    /// TX: PC12 (UART5_TX) -> HM-11 RX (pin 4)
    /// RX: PD2 (UART5_RX) <- HM-11 TX (pin 2)
    static constexpr PinConfig DEFAULT_PINS = {
        .tx_port = 2, // GPIOC
        .tx_pin = 12, // PC12
        .rx_port = 3, // GPIOD
        .rx_pin = 2   // PD2
    };

    /// Constructor - initializes UART5 and DMA1 Stream0 for HM-11 communication
    /// Equivalent to Ada's Init_UART + Initialize_DMA procedures
    ///
    /// Initialization sequence:
    /// 1. Enable peripheral clocks (UART5, GPIOC, GPIOD, DMA1)
    /// 2. Configure GPIO pins for alternate function (AF8)
    /// 3. Configure UART5 (9600 baud, 8N1, enable TX/RX)
    /// 4. Configure DMA1 Stream0 Channel4 (peripheral-to-memory)
    /// 5. Enable interrupts (UART5_IRQn, DMA1_Stream0_IRQn)
    /// 6. Register ISR handlers with global interrupt manager
    ///
    /// @param pins GPIO pin configuration (defaults to HM-11 standard pins)
    /// @throws std::runtime_error if hardware initialization fails
    explicit STM32UARTPort(const PinConfig& pins = DEFAULT_PINS);

    /// Destructor - graceful hardware cleanup with timeout
    /// Equivalent to Ada's finalization with timeout handling
    ///
    /// Cleanup sequence:
    /// 1. Stop ongoing DMA transfers (wait up to 100ms)
    /// 2. Disable UART5 and DMA interrupts
    /// 3. Disable peripheral clocks
    /// 4. Unregister ISR handlers
    /// 5. Reset GPIO pins to input mode
    ~STM32UARTPort() noexcept;

    /// Non-copyable and non-movable (RAII ownership model, Decision 22)
    STM32UARTPort(const STM32UARTPort&) = delete;
    STM32UARTPort& operator=(const STM32UARTPort&) = delete;
    STM32UARTPort(STM32UARTPort&&) = delete;
    STM32UARTPort& operator=(STM32UARTPort&&) = delete;

    // UARTInterface concept implementation
    // Provides the interface methods required by HM11Driver template

    /// Get data size configuration (always 8-bit for HM-11)
    /// Equivalent to Ada's HAL.UART.Data_Size function
    [[nodiscard]] UARTDataSize data_size() const noexcept { return UARTDataSize::DataSize8b; }

    /// Transmit data with timeout
    /// Equivalent to Ada's HAL.UART.Transmit procedure
    ///
    /// Implementation uses polling mode for simplicity and deterministic timing:
    /// 1. Check UART ready (TXE flag)
    /// 2. Write each byte to data register
    /// 3. Wait for transmission complete (TC flag)
    /// 4. Handle timeout using steady_clock
    ///
    /// @param data Span of bytes to transmit (caller-owned buffer)
    /// @param status Output status (Ok, ErrTimeout, ErrError)
    /// @param timeout_ms Maximum time to wait in milliseconds
    void transmit(UARTData8bConst data, UARTStatus& status, std::uint32_t timeout_ms) noexcept;

    /// Transmit data with default timeout (convenience overload)
    void transmit(UARTData8bConst data, UARTStatus& status) noexcept { transmit(data, status, DEFAULT_TIMEOUT_MS); }

    /// Receive data with timeout and DMA
    /// Equivalent to Ada's DMA_Receive_Handler procedure
    ///
    /// Implementation uses DMA1 Stream0 Channel4 with interrupt completion:
    /// 1. Configure DMA transfer (peripheral-to-memory, UART5->DR to buffer)
    /// 2. Start DMA with transfer complete interrupt enabled
    /// 3. Block on BinarySemaphore until DMA completion or timeout
    /// 4. Handle errors via ISR status propagation (Decision 26)
    /// 5. Cache invalidation on Cortex-M7 (Decision 20)
    ///
    /// @param data Span for received bytes (driver-owned internal buffer)
    /// @param status Output status (Ok, ErrTimeout, ErrError)
    /// @param timeout_ms Maximum time to wait in milliseconds
    void receive(UARTData8b data, UARTStatus& status, std::uint32_t timeout_ms) noexcept;

    /// Receive data with default timeout (convenience overload)
    void receive(UARTData8b data, UARTStatus& status) noexcept { receive(data, status, DEFAULT_TIMEOUT_MS); }

    // 9-bit mode operations (not used by HM-11, provided for interface completeness)
    void transmit(UARTData9bConst data, UARTStatus& status, std::uint32_t timeout_ms) noexcept;
    void transmit(UARTData9bConst data, UARTStatus& status) noexcept { transmit(data, status, DEFAULT_TIMEOUT_MS); }
    void receive(UARTData9b data, UARTStatus& status, std::uint32_t timeout_ms) noexcept;
    void receive(UARTData9b data, UARTStatus& status) noexcept { receive(data, status, DEFAULT_TIMEOUT_MS); }

  private:
    // Hardware register pointers (initialized to CMSIS peripheral bases)
    USART_TypeDef* uart_;         ///< UART5 peripheral registers
    DMA_Stream_TypeDef* dma_;     ///< DMA1 Stream0 registers
    DMA_TypeDef* dma_controller_; ///< DMA1 controller registers
    GPIO_TypeDef* tx_gpio_;       ///< TX GPIO port (GPIOC)
    GPIO_TypeDef* rx_gpio_;       ///< RX GPIO port (GPIOD)

    // Pin configuration
    PinConfig pins_;

    // Synchronization primitives for DMA operations
    sync::BinarySemaphore dma_complete_; ///< DMA transfer completion signal
    sync::BinarySemaphore uart_error_;   ///< UART error signal

    // DMA-aligned receive buffer (Decision 20)
    // 32-byte alignment for cache coherency on Cortex-M7
    // Size chosen to match typical HM-11 AT command response length
    alignas(32) std::uint8_t dma_rx_buffer_[256];

    // Internal state
    volatile bool dma_active_;      ///< DMA transfer in progress flag
    volatile std::size_t rx_count_; ///< Number of bytes received

    /// Initialize GPIO pins for UART alternate function
    /// Equivalent to Ada's Configure_IO procedure
    void init_gpio() noexcept;

    /// Initialize UART peripheral
    /// Equivalent to Ada's UART configuration sequence
    void init_uart() noexcept;

    /// Initialize DMA for receive operations
    /// Equivalent to Ada's Initialize_DMA procedure
    void init_dma() noexcept;

    /// Configure interrupt priorities and enable IRQs
    /// Matches Ada's interrupt priority settings
    void init_interrupts() noexcept;

    /// Cleanup function for destructor
    void cleanup_hardware() noexcept;

    /// Calculate baud rate register value for given baud rate
    /// Uses APB1 clock frequency (42MHz on STM32F429)
    [[nodiscard]] std::uint32_t calculate_baud_rate(std::uint32_t baud) const noexcept;

    // Interrupt service routines (called by global ISR handlers)
    // These are registered with the global interrupt manager during construction

    /// UART5 interrupt handler
    /// Handles error conditions and idle line detection
    /// Equivalent to Ada's IRQ_UART_Handler.On_UART_IRQ
    void handle_uart_interrupt() noexcept;

    /// DMA1 Stream0 interrupt handler
    /// Handles transfer complete and error conditions
    /// Equivalent to Ada's IRQ_DMA_Handler.On_DMA_IRQ
    void handle_dma_interrupt() noexcept;

    // Friend declaration for global ISR access
    friend class GlobalInterruptManager;
};

// Concept verification: ensure STM32UARTPort satisfies UARTInterface
static_assert(UARTInterface<STM32UARTPort>, "STM32UARTPort must satisfy UARTInterface concept");

} // namespace hm11::hal