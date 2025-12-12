/**
 * @file    stm32f429_startup.cpp
 * @brief   Minimal STM32F429 startup code in C++
 *
 * Provides essential startup functionality for STM32F429xx microcontrollers.
 * This replaces traditional startup.s assembly files with C++ equivalents.
 */

#ifdef STM32F429xx

#include <cstdint>
#include <cstring>

#include "stm32f429xx.h"

// External symbols defined by linker script
extern "C" {
extern std::uint32_t _sidata; // Start of initialized data in flash
extern std::uint32_t _sdata;  // Start of initialized data in RAM
extern std::uint32_t _edata;  // End of initialized data in RAM
extern std::uint32_t _sbss;   // Start of BSS section
extern std::uint32_t _ebss;   // End of BSS section
extern std::uint32_t _estack; // Top of stack

// Main application entry point
int main();
}

namespace {

/// Copy initialized data from flash to RAM
void copy_data_section() noexcept {
    auto* src = reinterpret_cast<std::uint32_t*>(&_sidata);
    auto* dst = reinterpret_cast<std::uint32_t*>(&_sdata);
    auto* end = reinterpret_cast<std::uint32_t*>(&_edata);

    while (dst < end) {
        *dst++ = *src++;
    }
}

/// Zero-initialize BSS section
void zero_bss_section() noexcept {
    auto* start = reinterpret_cast<std::uint32_t*>(&_sbss);
    auto* end = reinterpret_cast<std::uint32_t*>(&_ebss);

    while (start < end) {
        *start++ = 0;
    }
}

/// Basic system clock initialization to 168MHz
/// Uses HSE (8MHz external crystal) with PLL
void init_system_clock() noexcept {
    // Enable HSE (High Speed External oscillator)
    RCC->CR |= RCC_CR_HSEON;

    // Wait for HSE to stabilize
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        __NOP();
    }

    // Configure Flash latency for 168MHz
    // 5 wait states for VDD=2.7-3.6V at 168MHz
    FLASH->ACR = FLASH_ACR_LATENCY_5WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    // Configure PLL: HSE (8MHz) -> 168MHz
    // PLL_VCO = (HSE_VALUE / PLL_M) * PLL_N = (8MHz / 8) * 336 = 336MHz
    // SYSCLK = PLL_VCO / PLL_P = 336MHz / 2 = 168MHz
    // USB OTG FS = PLL_VCO / PLL_Q = 336MHz / 7 = 48MHz
    constexpr std::uint32_t PLL_M = 8;
    constexpr std::uint32_t PLL_N = 336;
    constexpr std::uint32_t PLL_P = 0; // 0 = /2
    constexpr std::uint32_t PLL_Q = 7;

    RCC->PLLCFGR = (PLL_M) | (PLL_N << 6) | (PLL_P << 16) | (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;

    // Wait for PLL to lock
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
        __NOP();
    }

    // Configure AHB, APB1, APB2 prescalers
    // AHB = SYSCLK (168MHz)
    // APB1 = AHB/4 = 42MHz (max 45MHz)
    // APB2 = AHB/2 = 84MHz (max 90MHz)
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  // AHB = SYSCLK
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2; // APB2 = AHB/2
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4; // APB1 = AHB/4

    // Select PLL as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    // Wait for PLL to be used as system clock source
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
        __NOP();
    }

    // Disable HSI to save power
    RCC->CR &= ~RCC_CR_HSION;
}

/// Initialize FPU (Floating Point Unit)
void init_fpu() noexcept {
    // Enable CP10 and CP11 coprocessors (FPU)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));

    // Ensure FPU settings take effect
    __DSB();
    __ISB();
}

} // anonymous namespace

// Exception and interrupt handlers

/// Reset handler - system entry point
extern "C" void Reset_Handler() noexcept {
    // Initialize FPU first (required for C++ floating point operations)
    init_fpu();

    // Copy initialized data from flash to RAM
    copy_data_section();

    // Zero-initialize BSS section
    zero_bss_section();

    // Initialize system clock
    init_system_clock();

    // Call global constructors (C++ initialization)
    // Note: In a full implementation, this would call __libc_init_array
    // For minimal implementation, we assume no global objects with constructors

    // Jump to main application
    main();

    // Should never reach here
    while (true) {
        __NOP();
    }
}

/// Default handler for unhandled exceptions/interrupts
extern "C" void Default_Handler() noexcept {
    // Infinite loop - allows debugger to catch unhandled interrupts
    while (true) {
        __NOP();
    }
}

// Exception handler aliases (weak linkage allows override)
extern "C" {
void NMI_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void SVC_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler() noexcept __attribute__((weak, alias("Default_Handler")));

// External interrupt handlers (weak linkage allows override)
void WWDG_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void UART5_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void TIM6_DAC_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void TIM7_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler() noexcept __attribute__((weak, alias("Default_Handler")));
}

// Vector table - must be in .isr_vector section (placed at 0x08000000 by linker)
using vector_func_t = void (*)();

__attribute__((section(".isr_vector"))) const vector_func_t vector_table[] = {
    // Cortex-M4 system exceptions
    reinterpret_cast<vector_func_t>(&_estack), // Initial stack pointer
    Reset_Handler,                             // Reset handler
    NMI_Handler,                               // NMI handler
    HardFault_Handler,                         // Hard fault handler
    MemManage_Handler,                         // Memory management fault
    BusFault_Handler,                          // Bus fault handler
    UsageFault_Handler,                        // Usage fault handler
    nullptr,                                   // Reserved
    nullptr,                                   // Reserved
    nullptr,                                   // Reserved
    nullptr,                                   // Reserved
    SVC_Handler,                               // SVC handler
    DebugMon_Handler,                          // Debug monitor handler
    nullptr,                                   // Reserved
    PendSV_Handler,                            // PendSV handler
    SysTick_Handler,                           // SysTick handler

    // STM32F429 external interrupts
    WWDG_IRQHandler,         // Window WatchDog
    PVD_IRQHandler,          // PVD through EXTI Line detection
    TAMP_STAMP_IRQHandler,   // Tamper and TimeStamp
    RTC_WKUP_IRQHandler,     // RTC Wakeup
    FLASH_IRQHandler,        // FLASH
    RCC_IRQHandler,          // RCC
    EXTI0_IRQHandler,        // EXTI Line0
    EXTI1_IRQHandler,        // EXTI Line1
    EXTI2_IRQHandler,        // EXTI Line2
    EXTI3_IRQHandler,        // EXTI Line3
    EXTI4_IRQHandler,        // EXTI Line4
    DMA1_Stream0_IRQHandler, // DMA1 Stream 0 (used for UART5 RX)
    DMA1_Stream1_IRQHandler, // DMA1 Stream 1
    DMA1_Stream2_IRQHandler, // DMA1 Stream 2
    DMA1_Stream3_IRQHandler, // DMA1 Stream 3
    DMA1_Stream4_IRQHandler, // DMA1 Stream 4
    DMA1_Stream5_IRQHandler, // DMA1 Stream 5
    DMA1_Stream6_IRQHandler, // DMA1 Stream 6
    ADC_IRQHandler,          // ADC1, ADC2 and ADC3

    // Skip to UART interrupts (index would be correct in full vector table)
    // For minimal implementation, we include key interrupts needed for HM11
    [52] = UART4_IRQHandler, // UART4 global interrupt
    [53] = UART5_IRQHandler, // UART5 global interrupt (HM11)
};

#endif // STM32F429xx