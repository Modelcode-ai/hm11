/**
 * @file    main.cpp
 * @brief   Basic HM11 Driver Initialization and Testing Example
 * 
 * This example demonstrates the essential HM11 driver functionality following
 * the pattern established in Ada's hm11_example.adb. It provides a complete
 * demonstration of:
 * 
 * 1. Driver initialization with mock UART for development
 * 2. Basic device communication testing (equivalent to Test function)
 * 3. Role configuration and verification
 * 4. Essential operations sequence
 * 5. Proper error handling and status reporting
 * 
 * This example serves as both integration test and user documentation,
 * showing how to instantiate the driver template, execute basic operations,
 * and handle responses correctly.
 * 
 * Build Instructions:
 *   cmake --preset clang-debug
 *   cmake --build --preset clang-debug --target 01_basic_init
 *   ./build/clang-debug/examples/01_basic_init/01_basic_init
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

// HM11 Driver and Types
#include "hm11/hm11.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types.hpp"
#include "hm11/hal.hpp"

using namespace hm11;
using namespace hm11::hal;

/// @brief Application state matching Ada example structure
enum class AppPhase : std::uint8_t {
    Initialization,
    BasicTesting,
    Configuration,
    Connection,
    Verification,
    Completed,
    Failed
};

/// @brief Status tracking and error reporting
class StatusReporter {
public:
    StatusReporter() : current_phase_(AppPhase::Initialization), error_count_(0) {}
    
    void set_phase(AppPhase phase) {
        current_phase_ = phase;
        std::cout << "\n=== " << phase_name(phase) << " ===\n";
    }
    
    void check_status(const std::string& operation, UARTStatus status) {
        if (status == UARTStatus::Ok) {
            std::cout << "✓ " << operation << " - OK\n";
        } else {
            std::cout << "✗ " << operation << " - FAILED (status: " 
                      << static_cast<int>(status) << ")\n";
            ++error_count_;
            
            if (error_count_ >= 3) {
                current_phase_ = AppPhase::Failed;
                std::cout << "\n*** Too many errors, aborting ***\n";
                throw std::runtime_error("Operation failed after multiple errors");
            }
        }
    }
    
    bool has_errors() const { return error_count_ > 0; }
    AppPhase current_phase() const { return current_phase_; }
    
private:
    AppPhase current_phase_;
    int error_count_;
    
    static const char* phase_name(AppPhase phase) {
        switch (phase) {
            case AppPhase::Initialization: return "Driver Initialization";
            case AppPhase::BasicTesting: return "Basic Device Communication";
            case AppPhase::Configuration: return "Device Configuration";
            case AppPhase::Connection: return "Connection Management";
            case AppPhase::Verification: return "Configuration Verification";
            case AppPhase::Completed: return "Example Completed Successfully";
            case AppPhase::Failed: return "Example Failed";
        }
        return "Unknown";
    }
};

/// @brief Demonstrate basic HM11 operations equivalent to Ada example
class BasicInitExample {
public:
    BasicInitExample() : driver_(make_hm11_driver(std::move(uart_))) {
        std::cout << "HM11 Driver Basic Initialization Example\n";
        std::cout << "=========================================\n";
        std::cout << "This example demonstrates the core driver functionality\n";
        std::cout << "equivalent to Ada's hm11_example.adb initialization sequence.\n";
    }
    
    void run() {
        try {
            // Phase 1: Basic device communication (equivalent to "Test" in Ada)
            test_basic_communication();
            
            // Phase 2: Configuration sequence (matching Ada example order)  
            configure_device_settings();
            
            // Phase 3: Connection management demonstration
            demonstrate_connection_management();
            
            // Phase 4: Verification (ensure settings were applied)
            verify_configuration();
            
            // Summary
            reporter_.set_phase(AppPhase::Completed);
            print_summary();
            
        } catch (const std::exception& e) {
            reporter_.set_phase(AppPhase::Failed);
            std::cout << "\nExample failed with exception: " << e.what() << "\n";
            throw;
        }
    }

private:
    MockUART uart_;
    HM11Driver<MockUART> driver_;
    StatusReporter reporter_;
    
    /// @brief Test basic communication - equivalent to Ada's "Test" procedure
    void test_basic_communication() {
        reporter_.set_phase(AppPhase::BasicTesting);

        UARTStatus status = UARTStatus::Ok;

        // Basic AT command test (equivalent to Test procedure)
        std::cout << "Testing basic device communication...\n";
        driver_.Test(status);
        reporter_.check_status("Basic AT Communication Test", status);
        
        // Allow some processing time (Ada example has delays)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    /// @brief Configure device settings following Ada example pattern
    void configure_device_settings() {
        reporter_.set_phase(AppPhase::Configuration);

        UARTStatus status = UARTStatus::Ok;

        // Set role to Peripheral (equivalent to Set_Role(Drivers.Driver, Peripheral, Status))
        std::cout << "Setting device role to Peripheral...\n";
        driver_.SetRole(Role::Peripheral, status);
        reporter_.check_status("Set Role to Peripheral", status);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Note: Additional configuration operations would go here
        // Following the Ada example sequence:
        // - Set_Advertising_Type
        // - Set_Bond_Mode  
        // - Set_Notify_Information
        // - Set_Module_Name
        // - Set_PIN_Code
        // - Set_Work_Type
        // These will be implemented in future tasks as the AT command infrastructure expands
        
        std::cout << "Basic configuration completed.\n";
    }
    
    /// @brief Demonstrate connection management functionality
    void demonstrate_connection_management() {
        reporter_.set_phase(AppPhase::Connection);
        
        std::cout << "Demonstrating connection management capabilities...\n";
        
        // Note: In a real application, these would be used with Central role
        // and after proper configuration. Here we demonstrate the API usage.

        UARTStatus status = UARTStatus::Ok;
        ConnectResult result = ConnectResult::Connecting;
        
        // Example 1: Connect to last device
        std::cout << "1. Attempting to connect to last connected device...\n";
        driver_.connect_last_device(result, status);
        report_connection_result("Connect to Last Device", result, status);
        
        // Example 2: Connect by MAC address  
        std::cout << "\n2. Attempting to connect by MAC address...\n";
        try {
            MacAddress target_mac{"001122AABBCC"};
            driver_.connect(MacAddressType::NormalAddress, target_mac, result, status);
            report_connection_result("Connect by MAC Address", result, status);
        } catch (const std::exception& e) {
            std::cout << "✗ MAC Address Connection - Failed (exception: " << e.what() << ")\n";
        }
        
        // Example 3: Connect by discovered device index
        std::cout << "\n3. Attempting to connect by device index...\n";
        try {
            DiscoveredIndex device_index(0); // First discovered device
            driver_.connect(device_index, result, status);
            report_connection_result("Connect by Device Index", result, status);
        } catch (const std::exception& e) {
            std::cout << "✗ Index Connection - Failed (exception: " << e.what() << ")\n";
        }
        
        std::cout << "\nConnection management demonstration completed.\n";
        std::cout << "Note: In real usage, device must be in Central role and properly configured.\n";
    }
    
    /// @brief Report connection operation result
    static void report_connection_result(const std::string& operation, ConnectResult result, UARTStatus status) {
        std::cout << "  Operation: " << operation << "\n";
        std::cout << "  UART Status: " << (status == UARTStatus::Ok ? "OK" : "ERROR") << "\n";
        std::cout << "  Connection Result: " << connection_result_to_string(result) << "\n";
        
        if (status == UARTStatus::Ok) {
            switch (result) {
                case ConnectResult::Connected:
                    std::cout << "  ✓ Successfully connected to device\n";
                    break;
                case ConnectResult::Connecting:
                    std::cout << "  ⏳ Connection in progress...\n";
                    break;
                case ConnectResult::ConnectError:
                    std::cout << "  ✗ Connection error occurred\n";
                    break;
                case ConnectResult::ConnectFail:
                    std::cout << "  ✗ Connection failed (device not connectable)\n";
                    break;
                case ConnectResult::NoAddress:
                    std::cout << "  ✗ No address available for connection\n";
                    break;
                case ConnectResult::OtherError:
                    std::cout << "  ✗ Other connection error occurred\n";
                    break;
            }
        } else {
            std::cout << "  ✗ UART communication error\n";
        }
    }
    
    /// @brief Convert connection result to human-readable string
    static const char* connection_result_to_string(ConnectResult result) {
        switch (result) {
            case ConnectResult::Connected: return "Connected";
            case ConnectResult::Connecting: return "Connecting";
            case ConnectResult::ConnectError: return "Connect Error";
            case ConnectResult::ConnectFail: return "Connect Fail";
            case ConnectResult::NoAddress: return "No Address";
            case ConnectResult::OtherError: return "Other Error";
        }
        return "Unknown";
    }
    
    /// @brief Verify configuration was applied correctly
    void verify_configuration() {
        reporter_.set_phase(AppPhase::Verification);

        UARTStatus status = UARTStatus::Ok;
        Role current_role = Role::Peripheral;
        
        // Verify role setting (equivalent to Get_Role verification)
        std::cout << "Verifying device role configuration...\n";
        driver_.GetRole(current_role, status);
        reporter_.check_status("Get Role Verification", status);
        
        if (status == UARTStatus::Ok) {
            std::cout << "Current device role: " 
                      << (current_role == Role::Peripheral ? "Peripheral" : "Central") << "\n";
                      
            if (current_role == Role::Peripheral) {
                std::cout << "✓ Role verification successful - device is configured as Peripheral\n";
            } else {
                std::cout << "⚠ Role mismatch - expected Peripheral, got Central\n";
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    /// @brief Print final summary equivalent to Ada example completion
    void print_summary() {
        std::cout << "\n";
        std::cout << "================================================\n";
        std::cout << "Example Execution Summary\n";
        std::cout << "================================================\n";
        
        std::cout << "Operations Performed:\n";
        std::cout << "  ✓ Driver instantiation with MockUART\n";
        std::cout << "  ✓ Basic AT command communication test\n";
        std::cout << "  ✓ Role configuration (Peripheral mode)\n";
        std::cout << "  ✓ Connection management demonstration\n";
        std::cout << "  ✓ Configuration verification\n";
        
        if (reporter_.has_errors()) {
            std::cout << "\n⚠ Some operations encountered errors during execution.\n";
            std::cout << "This is normal in mock mode - real hardware may behave differently.\n";
        } else {
            std::cout << "\n✓ All operations completed successfully!\n";
        }
        
        std::cout << "\nNext Steps:\n";
        std::cout << "  • Build with real hardware UART for actual device testing\n";
        std::cout << "  • See examples/02_stm32_peripheral_echo for full hardware example\n";  
        std::cout << "  • Explore type system usage in examples/basic\n";
        
        std::cout << "\nExample demonstrates:\n";
        std::cout << "  • Proper driver template instantiation\n";
        std::cout << "  • RAII resource management (automatic cleanup)\n";
        std::cout << "  • Type-safe operation parameters\n";
        std::cout << "  • Comprehensive error handling\n";
        std::cout << "  • Ada-equivalent operational sequence\n";
    }
};

/// @brief Demonstrate type system integration with driver operations
void demonstrate_type_system_integration() {
    std::cout << "\n=== Type System Integration Demo ===\n";
    
    // Show how type system integrates with driver operations
    std::cout << "HM11 Type System Integration:\n";
    
    // Role enumeration
    Role peripheral_role = Role::Peripheral;
    Role central_role = Role::Central;
    std::cout << "  • Role types: " 
              << (peripheral_role == Role::Peripheral ? "Peripheral" : "Central")
              << ", " << (central_role == Role::Central ? "Central" : "Peripheral") << "\n";
    
    // MAC address handling
    try {
        MacAddress device_mac{"001122AABBCC"};
        std::cout << "  • MAC address: " << std::string_view(device_mac) << "\n";
    } catch (const std::exception& e) {
        std::cout << "  • MAC validation: " << e.what() << "\n";
    }
    
    // Range-constrained values
    Percent battery_level{85};
    TemperatureType sensor_temp{25};
    std::cout << "  • Percent: " << static_cast<int>(battery_level.value()) << "%\n";
    std::cout << "  • Temperature: " << static_cast<int>(sensor_temp.value()) << "°C\n";
    
    std::cout << "✓ Type system provides compile-time safety for driver parameters\n";
}

/// @brief Main entry point for basic initialization example
int main() {
    try {
        // Show type system integration first
        demonstrate_type_system_integration();
        
        // Run main example
        BasicInitExample example;
        example.run();
        
        std::cout << "\n=== Example completed successfully ===\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nExample failed with error: " << e.what() << "\n";
        std::cerr << "This may be expected behavior when using mock UART implementation.\n";
        std::cerr << "For real hardware testing, use the STM32 peripheral echo example.\n";
        return 1;
    } catch (...) {
        std::cerr << "\nExample failed with unknown error.\n";
        return 1;
    }
}