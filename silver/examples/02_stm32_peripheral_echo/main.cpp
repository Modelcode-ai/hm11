/**
 * @file    main.cpp
 * @brief   HM11 STM32F429 Peripheral Echo Example
 * 
 * This example demonstrates the complete STM32 hardware driver implementation
 * for the HM11 BLE module using direct CMSIS register access.
 * 
 * Equivalent to Ada's hm11_example.adb peripheral echo functionality.
 * 
 * Hardware Setup:
 * - STM32F429 Discovery board
 * - HM11 BLE module connected to UART5 (PC12/PD2)
 * - TX: PC12 (UART5_TX) -> HM11 RX (pin 4)  
 * - RX: PD2 (UART5_RX) <- HM11 TX (pin 2)
 * - Power: 3.3V, GND
 * 
 * Functionality:
 * 1. Initialize STM32 UART port with DMA
 * 2. Initialize HM11 driver with peripheral role
 * 3. Set advertising name to "STM32_HM11"
 * 4. Enable advertising and wait for central connection
 * 5. Echo any received data back to central device
 * 6. Handle connection events with LED feedback
 * 
 * Build Command:
 * cmake --preset stm32f429-debug
 * cmake --build --preset stm32f429-debug
 */

#include <cstring>
#include <array>

// HM11 Driver with STM32 HAL
#include "hm11/hm11.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/hal/stm32_uart_port.hpp"

// Platform initialization
#include "hm11/platform/platform.hpp"

// For demonstration: Simple LED control (optional)
#ifdef STM32F429xx
#include "stm32f429xx.h"
#endif

using namespace hm11;
using namespace hm11::hal;
using namespace hm11::types;

// Application configuration matching Ada example
constexpr auto DEVICE_NAME = FixedString<12>("STM32_HM11");
constexpr auto ECHO_BUFFER_SIZE = 128u;

// Simple LED control for status indication (GPIO PG13/PG14 on Discovery)
class StatusLED {
  public:
    StatusLED() {
#ifdef STM32F429xx
        // Enable GPIOG clock
        RCC->AHB1ENR |= (1 << 6);  // GPIOGEN
        
        // Configure PG13 and PG14 as output (Green and Red LEDs)
        GPIOG->MODER |= (1 << 26) | (1 << 28);  // Output mode
        GPIOG->OTYPER &= ~((1 << 13) | (1 << 14));  // Push-pull
        GPIOG->OSPEEDR |= (1 << 26) | (1 << 28);  // Fast speed
#endif
    }
    
    void set_green(bool on) {
#ifdef STM32F429xx
        if (on) {
            GPIOG->BSRR = (1 << 13);  // Set PG13
        } else {
            GPIOG->BSRR = (1 << 29);  // Reset PG13
        }
#endif
    }
    
    void set_red(bool on) {
#ifdef STM32F429xx
        if (on) {
            GPIOG->BSRR = (1 << 14);  // Set PG14
        } else {
            GPIOG->BSRR = (1 << 30);  // Reset PG14
        }
#endif
    }
    
    void toggle_green() {
#ifdef STM32F429xx
        GPIOG->ODR ^= (1 << 13);
#endif
    }
    
    private:
#ifdef STM32F429xx
    // GPIO register access through CMSIS
    GPIO_TypeDef* const gpio_ = GPIOG;
#endif
};

// Application state machine
enum class AppState {
    Initializing,
    Configuring,
    Advertising,
    Connected,
    Error
};

// Simple delay function (for demonstration - normally use proper timer)
void delay_ms(uint32_t ms) {
    // Approximate delay using busy loop (not recommended for production)
    for (volatile uint32_t i = 0; i < ms * 1000; ++i) {
        __NOP();
    }
}

// Main application class matching Ada example structure
class PeripheralEchoApp {
  public:
    PeripheralEchoApp() : uart_port_(), driver_(uart_port_), state_(AppState::Initializing) {
        // Initialize platform features
        hm11::platform::Initialize::PlatformFeatures();
        
        // Initialize status LEDs
        led_.set_red(true);  // Red LED on during initialization
        led_.set_green(false);
    }
    
    void run() {
        UARTStatus status;
        
        // Step 1: Initialize and configure HM11 module
        if (!initialize_hm11()) {
            error_state("HM11 initialization failed");
            return;
        }
        
        // Step 2: Configure as peripheral
        if (!configure_peripheral()) {
            error_state("Peripheral configuration failed");
            return;
        }
        
        // Step 3: Start advertising
        if (!start_advertising()) {
            error_state("Failed to start advertising");
            return;
        }
        
        // Step 4: Main application loop - echo server
        echo_server_loop();
    }
    
  private:
    STM32UARTPort uart_port_;
    HM11Driver<STM32UARTPort> driver_;
    StatusLED led_;
    AppState state_;
    std::array<std::uint8_t, ECHO_BUFFER_SIZE> buffer_;
    
    bool initialize_hm11() {
        state_ = AppState::Initializing;
        
        // Allow HM11 module to boot (Ada example has 2 second delay)
        delay_ms(2000);
        
        // Test basic communication
        UARTStatus status;
        driver_.Send_AT_Command(AT_Command::AT, status);
        
        if (status != UARTStatus::Ok) {
            return false;
        }
        
        // Get firmware version (optional verification)
        FixedString<16> version;
        driver_.Get_Firmware_Version(version, status);
        
        return status == UARTStatus::Ok;
    }
    
    bool configure_peripheral() {
        state_ = AppState::Configuring;
        led_.set_red(false);
        led_.set_green(true);  // Green LED during configuration
        
        UARTStatus status;
        
        // Set role to peripheral (equivalent to Ada example)
        driver_.Set_Role(Role::Peripheral, status);
        if (status != UARTStatus::Ok) return false;
        
        // Set device name
        driver_.Set_Name(DEVICE_NAME, status);
        if (status != UARTStatus::Ok) return false;
        
        // Set advertising interval (100ms, Ada default)
        driver_.Set_Advertising_Interval(AdvertisingInterval::Ms100, status);
        if (status != UARTStatus::Ok) return false;
        
        // Reset module to apply settings
        driver_.Reset(status);
        if (status != UARTStatus::Ok) return false;
        
        delay_ms(1000);  // Wait for reset to complete
        
        return true;
    }
    
    bool start_advertising() {
        state_ = AppState::Advertising;
        led_.set_green(false);  // Turn off green, blink to show advertising
        
        UARTStatus status;
        
        // Start advertising (HM11 starts advertising automatically in peripheral mode)
        // Just verify we can communicate
        driver_.Send_AT_Command(AT_Command::AT, status);
        
        if (status == UARTStatus::Ok) {
            // Blink green LED to indicate advertising
            for (int i = 0; i < 10; ++i) {
                led_.toggle_green();
                delay_ms(200);
            }
            return true;
        }
        
        return false;
    }
    
    void echo_server_loop() {
        state_ = AppState::Connected;
        led_.set_green(true);   // Solid green when connected
        led_.set_red(false);
        
        UARTStatus status;
        
        while (true) {
            // Check for connection status periodically
            ConnectionState conn_state;
            driver_.Get_Connection_State(conn_state, status);
            
            if (status != UARTStatus::Ok) {
                error_state("Failed to get connection state");
                return;
            }
            
            if (conn_state == ConnectionState::Connected) {
                // We're connected - handle data echo
                handle_data_echo();
                led_.set_green(true);
            } else {
                // Not connected - blink green to show advertising
                led_.toggle_green();
                delay_ms(500);
            }
        }
    }
    
    void handle_data_echo() {
        UARTStatus status;
        
        // Try to receive data (non-blocking with short timeout)
        std::span<std::uint8_t> rx_buffer(buffer_.data(), ECHO_BUFFER_SIZE);
        
        // Use short timeout to allow connection status checking
        auto rx_span = rx_buffer.subspan(0, 32);  // Receive up to 32 bytes at a time
        driver_.Receive_Data(rx_span, status, 100);  // 100ms timeout
        
        if (status == UARTStatus::Ok) {
            // Data received - echo it back
            // Find actual data length (look for null terminator or use all data)
            size_t data_length = rx_span.size();
            for (size_t i = 0; i < rx_span.size(); ++i) {
                if (rx_span[i] == 0) {
                    data_length = i;
                    break;
                }
            }
            
            if (data_length > 0) {
                // Echo the data back
                auto tx_span = std::span<const std::uint8_t>(rx_buffer.data(), data_length);
                driver_.Send_Data(tx_span, status);
                
                // Brief LED blink to indicate data activity
                led_.set_red(true);
                delay_ms(50);
                led_.set_red(false);
            }
        } else if (status == UARTStatus::ErrTimeout) {
            // Timeout is expected when no data available
            // Continue normal operation
        } else {
            // Actual error occurred
            error_state("Data receive error");
        }
    }
    
    void error_state(const char* error_msg) {
        state_ = AppState::Error;
        
        // Flash red LED to indicate error
        while (true) {
            led_.set_red(true);
            led_.set_green(false);
            delay_ms(100);
            
            led_.set_red(false);
            delay_ms(100);
        }
    }
};

// Application entry point
int main() {
    try {
        PeripheralEchoApp app;
        app.run();
    } catch (...) {
        // Error handling - flash both LEDs
        StatusLED error_led;
        while (true) {
            error_led.set_red(true);
            error_led.set_green(true);
            delay_ms(200);
            
            error_led.set_red(false);
            error_led.set_green(false);
            delay_ms(200);
        }
    }
    
    return 0;
}