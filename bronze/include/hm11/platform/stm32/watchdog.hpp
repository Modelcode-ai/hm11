/**
 * @file watchdog.hpp
 * @brief STM32-specific Watchdog implementation for timeout handling
 * @details Implements Decision 19, 25, and 26 for watchdog and timeout handling
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides platform-specific extensions to the Watchdog class.
 */

#ifndef HM11_PLATFORM_STM32_WATCHDOG_HPP
#define HM11_PLATFORM_STM32_WATCHDOG_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <hm11/platform/alignment.hpp>
#include <hm11/platform/cache.hpp>
#include <hm11/platform/stm32/device.hpp>
#include <hm11/sync/watchdog.hpp>

namespace hm11::platform::stm32 {

/**
 * @brief STM32-specific Watchdog implementation
 *
 * This class extends the core Watchdog functionality with STM32-specific
 * features like checking the UART state and handling DMA-specific timeouts.
 * It follows Decision 19 for protected objects translation, Decision 25 for
 * timeout implementation, and Decision 26 for ISR error propagation.
 */
class Stm32Watchdog : public sync::Watchdog {
  public:
    /**
     * @brief Construct a new STM32 Watchdog
     */
    Stm32Watchdog() = default;

    /**
     * @brief Destroy the STM32 Watchdog
     */
    ~Stm32Watchdog() = default;

    // Delete copy and move operations to ensure proper resource management
    Stm32Watchdog(const Stm32Watchdog&) = delete;
    Stm32Watchdog& operator=(const Stm32Watchdog&) = delete;
    Stm32Watchdog(Stm32Watchdog&&) = delete;
    Stm32Watchdog& operator=(Stm32Watchdog&&) = delete;

    /**
     * @brief Start a watchdog-monitored DMA operation
     *
     * This method stores the timeout deadline and prepares the watchdog for
     * monitoring a DMA operation. It extends the base Watchdog::start method
     * with STM32-specific initializations.
     *
     * @param buffer Pointer to the receiving buffer
     * @param length Length of the receiving buffer
     * @param timeout_ms Timeout duration in milliseconds
     * @param uart UART peripheral to monitor
     * @param stream_mode True if operating in stream mode
     * @return true if successfully started, false if already started
     */
    bool start_dma(
        void* buffer,
        std::size_t length,
        std::chrono::milliseconds timeout_ms,
        USART_TypeDef* uart,
        bool stream_mode = false);

    /**
     * @brief Stop the watchdog-monitored DMA operation
     *
     * This method stops the watchdog monitoring and resets its state.
     * It extends the base Watchdog::stop method with STM32-specific cleanup.
     */
    void stop_dma();

    /**
     * @brief Check for UART errors and update the watchdog status
     *
     * This method checks the UART status register for error flags and updates
     * the watchdog status if errors are detected.
     *
     * @return true if errors were detected, false otherwise
     */
    bool check_uart_errors();

    /**
     * @brief Get the UART peripheral
     *
     * @return USART_TypeDef* Pointer to the UART peripheral
     */
    USART_TypeDef* get_uart() const { return uart_; }

    /**
     * @brief Check if the watchdog is in stream mode
     *
     * @return true if in stream mode, false otherwise
     */
    bool is_stream_mode() const { return stream_mode_; }

  private:
    // STM32-specific state
    USART_TypeDef* uart_{nullptr}; ///< UART peripheral being monitored
    bool stream_mode_{false};      ///< Flag indicating stream mode
};

} // namespace hm11::platform::stm32

#endif // HM11_PLATFORM_STM32_WATCHDOG_HPP