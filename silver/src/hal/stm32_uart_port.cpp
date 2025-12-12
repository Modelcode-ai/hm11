// HM11 Hardware Abstraction Layer - STM32 UART Port Implementation
// Direct CMSIS register access implementation for STM32F429 UART5

#include "hm11/hal/stm32_uart_port.hpp"

#include <chrono>
#include <cstring>
#include <stdexcept>

#include "hm11/platform/dma_cache.hpp"
#include "hm11/platform/platform.hpp"

// Include STM32F429 CMSIS headers
#ifdef STM32F429xx
#include "stm32f429xx.h"
#else
// For testing/development without STM32 headers
#include "vendor/CMSIS/stm32f429xx.h"
#endif

namespace hm11::hal {

// Global interrupt manager for STM32 ISR registration
// This manages the mapping between hardware IRQs and C++ objects
class GlobalInterruptManager {
  public:
    static GlobalInterruptManager& instance() {
        static GlobalInterruptManager manager;
        return manager;
    }

    void register_uart5_handler(STM32UARTPort* handler) noexcept { uart5_handler_ = handler; }

    void register_dma1_stream0_handler(STM32UARTPort* handler) noexcept { dma1_stream0_handler_ = handler; }

    void unregister_uart5_handler() noexcept { uart5_handler_ = nullptr; }

    void unregister_dma1_stream0_handler() noexcept { dma1_stream0_handler_ = nullptr; }

    STM32UARTPort* get_uart5_handler() const noexcept { return uart5_handler_; }

    STM32UARTPort* get_dma1_stream0_handler() const noexcept { return dma1_stream0_handler_; }

  private:
    STM32UARTPort* uart5_handler_ = nullptr;
    STM32UARTPort* dma1_stream0_handler_ = nullptr;
};

// Forward declarations for ISR handlers
extern "C" void UART5_IRQHandler();
extern "C" void DMA1_Stream0_IRQHandler();

STM32UARTPort::STM32UARTPort(const PinConfig& pins)
    : uart_(UART5), dma_(DMA1_Stream0), dma_controller_(DMA1),
      tx_gpio_(pins.tx_port == 2 ? GPIOC : GPIOD) // Simplified for this implementation
      ,
      rx_gpio_(pins.rx_port == 3 ? GPIOD : GPIOC), pins_(pins), dma_active_(false), rx_count_(0) {
    // Register this instance with global interrupt manager
    auto& manager = GlobalInterruptManager::instance();
    manager.register_uart5_handler(this);
    manager.register_dma1_stream0_handler(this);

    try {
        // Initialize hardware in correct sequence (matching Ada implementation)
        init_gpio();       // Configure GPIO pins first
        init_uart();       // Then UART configuration
        init_dma();        // Then DMA setup
        init_interrupts(); // Finally enable interrupts
    } catch (...) {
        // Cleanup on failure
        cleanup_hardware();
        throw;
    }
}

STM32UARTPort::~STM32UARTPort() noexcept {
    cleanup_hardware();

    // Unregister from global interrupt manager
    auto& manager = GlobalInterruptManager::instance();
    manager.unregister_uart5_handler();
    manager.unregister_dma1_stream0_handler();
}

void STM32UARTPort::init_gpio() noexcept {
    // Enable GPIO clocks (equivalent to Ada's Enable_Clock(TX & RX))
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN;

    // Small delay for clock stabilization
    for (volatile int i = 0; i < 1000; ++i) {
        __NOP();
    }

    // Configure TX pin (PC12) for alternate function
    // Equivalent to Ada's Configure_IO with Mode_AF, AF_UART5_8, Push_Pull, Speed_50MHz
    {
        GPIO_TypeDef* gpio = GPIOC;
        const uint32_t pin = 12;
        const uint32_t pos = pin * 2;

        // Set mode to alternate function (0b10)
        gpio->MODER &= ~(3U << pos);
        gpio->MODER |= (2U << pos);

        // Set output type to push-pull (0)
        gpio->OTYPER &= ~(1U << pin);

        // Set speed to 50MHz (0b10)
        gpio->OSPEEDR &= ~(3U << pos);
        gpio->OSPEEDR |= (2U << pos);

        // Set pull-up resistor (Ada: Pull_Up)
        gpio->PUPDR &= ~(3U << pos);
        gpio->PUPDR |= (1U << pos);

        // Set alternate function to AF8 (UART5)
        if (pin < 8) {
            gpio->AFR[0] &= ~(0xFU << (pin * 4));
            gpio->AFR[0] |= (8U << (pin * 4));
        } else {
            gpio->AFR[1] &= ~(0xFU << ((pin - 8) * 4));
            gpio->AFR[1] |= (8U << ((pin - 8) * 4));
        }
    }

    // Configure RX pin (PD2) for alternate function
    {
        GPIO_TypeDef* gpio = GPIOD;
        const uint32_t pin = 2;
        const uint32_t pos = pin * 2;

        // Set mode to alternate function (0b10)
        gpio->MODER &= ~(3U << pos);
        gpio->MODER |= (2U << pos);

        // Set output type to push-pull (0)
        gpio->OTYPER &= ~(1U << pin);

        // Set speed to 50MHz (0b10)
        gpio->OSPEEDR &= ~(3U << pos);
        gpio->OSPEEDR |= (2U << pos);

        // Set pull-up resistor
        gpio->PUPDR &= ~(3U << pos);
        gpio->PUPDR |= (1U << pos);

        // Set alternate function to AF8 (UART5)
        gpio->AFR[0] &= ~(0xFU << (pin * 4));
        gpio->AFR[0] |= (8U << (pin * 4));
    }
}

void STM32UARTPort::init_uart() noexcept {
    // Enable UART5 clock (equivalent to Ada's Enable_Clock(UART))
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN;

    // Small delay for clock stabilization
    for (volatile int i = 0; i < 1000; ++i) {
        __NOP();
    }

    // Disable UART first (equivalent to Ada's Disable(UART))
    uart_->CR1 &= ~USART_CR1_UE;

    // Configure UART parameters (equivalent to Ada's configuration sequence)

    // Set baud rate to 9600 (Ada: Set_Baud_Rate(UART, 9600))
    uart_->BRR = calculate_baud_rate(9600);

    // Set mode to TX+RX (Ada: Set_Mode(UART, Tx_Rx_Mode))
    uart_->CR1 |= USART_CR1_TE | USART_CR1_RE;

    // Set word length to 8 bits (Ada: Set_Word_Length(UART, Word_Length_8))
    uart_->CR1 &= ~USART_CR1_M; // 0 = 8 bits

    // Set stop bits to 1 (Ada: Set_Stop_Bits(UART, Stopbits_1))
    uart_->CR2 &= ~(3U << 12); // 0b00 = 1 stop bit

    // Set no parity (Ada: Set_Parity(UART, No_Parity))
    uart_->CR1 &= ~USART_CR1_PCE;

    // Set no flow control (Ada: Set_Flow_Control(UART, No_Flow_Control))
    uart_->CR3 &= ~(3U << 8); // Clear CTSE and RTSE

    // Enable DMA for receiver (Ada: Enable_DMA_Receive_Requests)
    uart_->CR3 |= USART_CR3_DMAR;

    // Clear all status flags
    uart_->SR = 0;

    // Enable UART (Ada: Enable(UART))
    uart_->CR1 |= USART_CR1_UE;
}

void STM32UARTPort::init_dma() noexcept {
    // Enable DMA1 clock (equivalent to Ada's Enable_Clock(Controller))
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    // Small delay for clock stabilization
    for (volatile int i = 0; i < 1000; ++i) {
        __NOP();
    }

    // Reset DMA stream (equivalent to Ada's Reset(Controller, Rx_Stream))
    dma_->CR &= ~DMA_SxCR_EN; // Disable stream first
    while (dma_->CR & DMA_SxCR_EN) {
        __NOP();
    } // Wait for disable

    // Clear all flags for Stream0
    dma_controller_->LIFCR = 0x3F; // Clear all flags for Stream0

    // Configure DMA stream (equivalent to Ada's DMA_Stream_Configuration)
    dma_->CR = 0; // Start with clean configuration

    // Set channel 4 (UART5_RX channel)
    dma_->CR |= (4U << DMA_SxCR_CHSEL_Pos);

    // Set direction: peripheral to memory
    dma_->CR &= ~DMA_SxCR_DIR; // 0b00 = peripheral-to-memory

    // Set peripheral address (UART5 data register)
    dma_->PAR = reinterpret_cast<uint32_t>(&uart_->DR);

    // Set memory address (our DMA buffer)
    dma_->M0AR = reinterpret_cast<uint32_t>(dma_rx_buffer_);

    // Set transfer size (will be updated per transfer)
    dma_->NDTR = 0;

    // Configure control register
    dma_->CR |= DMA_SxCR_MINC;           // Memory increment mode
    dma_->CR &= ~DMA_SxCR_PINC;          // No peripheral increment
    dma_->CR &= ~DMA_SxCR_CIRC;          // Normal mode (no circular)
    dma_->CR |= (3U << DMA_SxCR_PL_Pos); // Priority very high
    dma_->CR &= ~DMA_SxCR_MSIZE;         // Memory data size: byte
    dma_->CR &= ~DMA_SxCR_PSIZE;         // Peripheral data size: byte

    // Disable FIFO mode (equivalent to Ada's FIFO_Enabled := False)
    dma_->FCR &= ~(1U << 2); // Clear DMDIS bit

    // Enable transfer complete interrupt
    dma_->CR |= DMA_SxCR_TCIE;

    // Enable error interrupts
    dma_->CR |= DMA_SxCR_TEIE | DMA_SxCR_DMEIE;
}

void STM32UARTPort::init_interrupts() noexcept {
    // Set interrupt priorities (equivalent to Ada's pragma Interrupt_Priority)
    NVIC_SetPriority(UART5_IRQn, 5);        // Medium priority for UART errors
    NVIC_SetPriority(DMA1_Stream0_IRQn, 4); // Higher priority for DMA completion

    // Enable interrupts in NVIC
    NVIC_EnableIRQ(UART5_IRQn);
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    // Enable UART error interrupts (equivalent to Ada's Enable_Interrupts)
    uart_->CR1 |= USART_CR1_PEIE;   // Parity error
    uart_->CR1 |= USART_CR1_IDLEIE; // Idle line detection
    uart_->CR3 |= USART_CR3_EIE;    // Error interrupts (frame, noise, overrun)
}

void STM32UARTPort::cleanup_hardware() noexcept {
    // Disable interrupts first
    NVIC_DisableIRQ(UART5_IRQn);
    NVIC_DisableIRQ(DMA1_Stream0_IRQn);

    // Stop any ongoing DMA transfers
    if (dma_active_) {
        dma_->CR &= ~DMA_SxCR_EN;
        // Wait up to 100ms for DMA to stop
        auto start = std::chrono::steady_clock::now();
        while ((dma_->CR & DMA_SxCR_EN) && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100)) {
            __NOP();
        }
        dma_active_ = false;
    }

    // Disable UART
    uart_->CR1 &= ~USART_CR1_UE;

    // Clear all interrupt enables
    uart_->CR1 &= ~(USART_CR1_PEIE | USART_CR1_IDLEIE | USART_CR1_RXNEIE);
    uart_->CR3 &= ~USART_CR3_EIE;

    // Clear all status flags
    uart_->SR = 0;

    // Reset GPIO pins to input mode (safe default)
    if (tx_gpio_ == GPIOC) {
        tx_gpio_->MODER &= ~(3U << (pins_.tx_pin * 2)); // Input mode
    }
    if (rx_gpio_ == GPIOD) {
        rx_gpio_->MODER &= ~(3U << (pins_.rx_pin * 2)); // Input mode
    }
}

uint32_t STM32UARTPort::calculate_baud_rate(uint32_t baud) const noexcept {
    // APB1 clock is typically 42MHz on STM32F429 (HCLK/2)
    // BRR = fCK / (16 * baud)
    constexpr uint32_t APB1_CLOCK = 42000000;
    return APB1_CLOCK / (16 * baud);
}

void STM32UARTPort::transmit(UARTData8bConst data, UARTStatus& status, uint32_t timeout_ms) noexcept {
    status = UARTStatus::Ok;

    if (data.empty()) {
        return;
    }

    const auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(timeout_ms);

    for (const auto byte : data) {
        // Wait for transmit data register empty (equivalent to polling TXE)
        while (!(uart_->SR & USART_SR_TXE)) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                status = UARTStatus::ErrTimeout;
                return;
            }
            __NOP();
        }

        // Write byte to data register
        uart_->DR = byte;
    }

    // Wait for transmission complete (equivalent to waiting for TC)
    while (!(uart_->SR & USART_SR_TC)) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            status = UARTStatus::ErrTimeout;
            return;
        }
        __NOP();
    }
}

void STM32UARTPort::receive(UARTData8b data, UARTStatus& status, uint32_t timeout_ms) noexcept {
    status = UARTStatus::Ok;

    if (data.empty()) {
        return;
    }

    // Ensure data fits in our DMA buffer
    const size_t receive_size = std::min(data.size(), sizeof(dma_rx_buffer_));

    // Reset semaphore state
    dma_complete_.Reset();

    // Configure DMA for this transfer
    dma_->CR &= ~DMA_SxCR_EN; // Disable stream
    while (dma_->CR & DMA_SxCR_EN) {
        __NOP();
    } // Wait for disable

    dma_->NDTR = receive_size;                               // Set transfer count
    dma_->M0AR = reinterpret_cast<uint32_t>(dma_rx_buffer_); // Ensure correct buffer

    // Clear any pending flags
    dma_controller_->LIFCR = 0x3F; // Clear all Stream0 flags

    // Invalidate cache for DMA buffer (Decision 20)
    if constexpr (platform::Platform::HasDataCache()) {
        platform::DMACache::InvalidateRange(dma_rx_buffer_, receive_size);
    }

    // Start DMA transfer
    dma_active_ = true;
    rx_count_ = 0;
    dma_->CR |= DMA_SxCR_EN; // Enable stream

    // Wait for completion with timeout
    const auto timeout_result = dma_complete_.TakeWithTimeout(std::chrono::milliseconds(timeout_ms));

    if (timeout_result == UARTStatus::Ok) {
        // DMA completed successfully - copy data to user buffer
        const size_t bytes_received = std::min(rx_count_, data.size());
        std::memcpy(data.data(), dma_rx_buffer_, bytes_received);

        // Zero remaining bytes if user buffer is larger
        if (data.size() > bytes_received) {
            std::memset(data.data() + bytes_received, 0, data.size() - bytes_received);
        }
    } else {
        // Timeout or error occurred
        status = timeout_result;

        // Stop DMA transfer
        dma_->CR &= ~DMA_SxCR_EN;
        dma_active_ = false;
    }
}

void STM32UARTPort::transmit(UARTData9bConst data, UARTStatus& status, uint32_t timeout_ms) noexcept {
    // 9-bit mode not implemented for HM-11 (requires different UART configuration)
    // Set error status to indicate unsupported operation
    status = UARTStatus::ErrError;
}

void STM32UARTPort::receive(UARTData9b data, UARTStatus& status, uint32_t timeout_ms) noexcept {
    // 9-bit mode not implemented for HM-11
    status = UARTStatus::ErrError;
}

void STM32UARTPort::handle_uart_interrupt() noexcept {
    // Handle UART error conditions (equivalent to Ada's IRQ_UART_Handler.On_UART_IRQ)
    const uint32_t status_reg = uart_->SR;

    // Check for error conditions
    if (status_reg & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) {
        // Clear error flags
        uart_->SR = 0;

        // Signal error through semaphore
        uart_error_.GiveFromISRWithStatus(UARTStatus::ErrError);
    }

    // Handle idle line detection (used for end-of-frame detection)
    if (status_reg & USART_SR_IDLE) {
        // Clear idle flag by reading SR then DR
        volatile uint32_t dummy = uart_->DR;
        (void)dummy;

        // If DMA is active, this might indicate end of receive
        if (dma_active_) {
            // Calculate received bytes
            rx_count_ = sizeof(dma_rx_buffer_) - dma_->NDTR;

            // Stop DMA and signal completion
            dma_->CR &= ~DMA_SxCR_EN;
            dma_active_ = false;
            dma_complete_.GiveFromISR();
        }
    }
}

void STM32UARTPort::handle_dma_interrupt() noexcept {
    // Handle DMA completion and errors (equivalent to Ada's IRQ_DMA_Handler.On_DMA_IRQ)
    const uint32_t isr = dma_controller_->LISR;

    // Check transfer complete for Stream0
    if (isr & DMA_LISR_TCIF0) {
        // Clear transfer complete flag
        dma_controller_->LIFCR = DMA_LISR_TCIF0;

        // Calculate bytes received
        rx_count_ = sizeof(dma_rx_buffer_) - dma_->NDTR;

        // Mark DMA as inactive and signal completion
        dma_active_ = false;
        dma_complete_.GiveFromISR();
    }

    // Check for DMA errors
    if (isr & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0)) {
        // Clear error flags
        dma_controller_->LIFCR = DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0;

        // Mark DMA as inactive and signal error
        dma_active_ = false;
        dma_complete_.GiveFromISRWithStatus(UARTStatus::ErrError);
    }
}

} // namespace hm11::hal

// Global ISR handlers - these are referenced in the vector table
// They call the registered C++ object methods through the global manager

extern "C" void UART5_IRQHandler() {
    auto& manager = hm11::hal::GlobalInterruptManager::instance();
    auto* handler = manager.get_uart5_handler();
    if (handler) {
        handler->handle_uart_interrupt();
    }
}

extern "C" void DMA1_Stream0_IRQHandler() {
    auto& manager = hm11::hal::GlobalInterruptManager::instance();
    auto* handler = manager.get_dma1_stream0_handler();
    if (handler) {
        handler->handle_dma_interrupt();
    }
}