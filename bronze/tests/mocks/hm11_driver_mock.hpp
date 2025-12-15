/**
 * @file hm11_driver_mock.hpp
 * @brief Mock implementation of the HM11Driver for testing
 *
 * This file provides a mock implementation of the HM11Driver for testing purposes.
 * It extends the existing UART mock with additional functionality specific to
 * the HM11Driver class.
 *
 * @copyright Copyright (C) 2025
 */

#ifndef HM11_TESTS_MOCKS_HM11_DRIVER_MOCK_HPP
#define HM11_TESTS_MOCKS_HM11_DRIVER_MOCK_HPP

#include <deque>
#include <gmock/gmock.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "hm11/driver.hpp"
#include "hm11/hm11.hpp"
#include "hm11/hal/uart.hpp"
#include "mocks/uart_mock.hpp"

namespace hm11::tests::mocks {

/**
 * @brief Mock implementation of receive handler
 */
class MockReceiveHandler {
  public:
    MOCK_METHOD(void, call,
                (hal::AnyUartPort, void*, std::size_t, hal::UartStatus&,
                 std::chrono::milliseconds, bool),
                ());

    /**
     * @brief Wrapper to match the function signature expected by the HM11Driver
     */
    static void receive_wrapper(hal::AnyUartPort port, void* received,
                               std::size_t length, hal::UartStatus& status,
                               std::chrono::milliseconds timeout_ms, bool as_stream) {
        if (instance != nullptr) {
            instance->call(port, received, length, status, timeout_ms, as_stream);
        } else {
            // Default implementation if no mock instance is set
            std::memset(received, 0, length);
            status = hal::UartStatus::Ok;
        }
    }
    
    // Alias for backward compatibility with existing tests
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void ReceiveWrapper(hal::AnyUartPort port, void* received,
                              std::size_t length, hal::UartStatus& status,
                              std::chrono::milliseconds timeout_ms, bool as_stream) {
        receive_wrapper(port, received, length, status, timeout_ms, as_stream);
    }

     /**
      * @brief Set the global instance of this mock
      */
    static void set_instance(MockReceiveHandler* inst) {
        MockReceiveHandler::instance = inst;
    }

    // Alias for backward compatibility with existing tests
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void SetInstance(MockReceiveHandler* inst) {
        set_instance(inst);
    }

  private:
    static MockReceiveHandler* instance;
};

/**
 * @brief Mock implementation of last read position handler
 */
class MockLastReadPositionHandler {
  public:
    MOCK_METHOD(void, call, (bool&, std::size_t&), ());

    /**
     * @brief Wrapper to match the function signature expected by the HM11Driver
     */
    static void last_read_position_wrapper(bool& closed, std::size_t& zero) {
        if (instance != nullptr) {
            instance->call(closed, zero);
        } else {
            // Default implementation if no mock instance is set
            closed = false;
            zero = 1;
        }
    }
    
    // Alias for backward compatibility with existing tests
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void LastReadPositionWrapper(bool& closed, std::size_t& zero) {
        last_read_position_wrapper(closed, zero);
    }

     /**
      * @brief Set the global instance of this mock
      */
    static void set_instance(MockLastReadPositionHandler* inst) {
        MockLastReadPositionHandler::instance = inst;
    }

    // Alias for backward compatibility with existing tests
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void SetInstance(MockLastReadPositionHandler* inst) {
        set_instance(inst);
    }

  private:
    static MockLastReadPositionHandler* instance;
};

/**
 * @brief Enhanced mock for HM11Driver tests
 *
 * This class extends the UARTMock with additional functionality to simplify
 * testing of the HM11Driver class.
 */
class EnhancedUARTMock : public UARTMock {
  public:
    /**
     * @brief Constructor
     * @param data_size The data size to report (defaults to 8-bit)
     */
    explicit EnhancedUARTMock(hal::UartDataSize data_size = hal::UartDataSize::DataSize8b)
        : UARTMock(data_size), 
          current_role(Role::Peripheral),
          current_name("HMSoft"),
          current_mode(WorkMode::TransmissionMode),
          command_responses{},
          command_handlers_{} {
        // Set up default behavior including command response mapping
        setup_enhanced_behavior();
    }

    /**
     * @brief Add a response for a specific AT command
     * @param command The AT command to respond to
     * @param response The response to return
     * @param status The status to return
     */
    void add_command_response(std::string command, std::string response,
                              hal::UartStatus status = hal::UartStatus::Ok) {
        command_responses[std::move(command)] = {std::move(response), status};
    }

    /**
     * @brief Add a response for a specific AT command with a custom handler
     * @param command The AT command to respond to
     * @param handler The handler to call when the command is received
     */
    void add_command_handler(
        std::string command,
        std::function<void(const std::string&, std::string&, hal::UartStatus&)> handler) {
        command_handlers_[std::move(command)] = std::move(handler);
    }

    /**
     * @brief Set up enhanced behavior for HM11 testing
     */
    void setup_enhanced_behavior() {
        setup_default_behavior();

        // Default AT command responses
        add_command_response("AT", "OK");
        add_command_response("AT+VERSION", "HMSoft V545");
        add_command_response("AT+RESET", "OK");
        add_command_response("AT+RESTART", "OK");
        add_command_response("AT+ROLE?", "OK+Get:0");  // Default is Peripheral
        add_command_response("AT+ADDR?", "OK+ADDR:112233445566");
        add_command_response("AT+RENEW", "OK");
        add_command_response("AT+SLEEP", "OK+SLEEP");
        add_command_response("AT+MODE?", "OK+Get:0");  // Default is Transmission mode

        // Note: ROLE, NAME, and MODE command handlers removed to allow tests to override responses
        // Tests can use add_command_response() to set up expected responses

        // Add other command handlers as needed

        // Set up transmit behavior
        using ::testing::_;
        using ::testing::Invoke;
        
        // Explicitly specify which overload we're using to avoid ambiguity
        using Data8b = const hal::UartData8b&;

        ON_CALL(*this, transmit(::testing::An<Data8b>(), _, _))
            .WillByDefault(Invoke([this](const hal::UartData8b& data,
                                         hal::UartStatus& status,
                                         std::chrono::milliseconds) {
                // Clear any previous responses from the receive queue
                clear_receive_queue();

                // Store the transmitted data (now accessible as protected member)
                m_transmitted_data_8b.insert(m_transmitted_data_8b.end(), data.begin(), data.end());

                // Check if a non-Ok default status has been set (for error injection)
                if (m_default_status != hal::UartStatus::Ok) {
                    status = m_default_status;
                    return;
                }

                // Convert to string for easier handling
                std::string command(data.begin(), data.end());

                // Find matching command handler
                for (const auto& [cmd_prefix, handler] : command_handlers_) {
                    if (command.find(cmd_prefix) == 0) {
                        std::string response;
                        handler(command, response, status);
                        queue_receive_data(response);
                        return;
                    }
                }

                // Find matching command response
                for (const auto& [cmd_prefix, resp] : command_responses) {
                    if (command.find(cmd_prefix) == 0) {
                        queue_receive_data(resp.response);
                        status = resp.status;
                        return;
                    }
                }

                // Default response
                queue_receive_data("OK");
                status = hal::UartStatus::Ok;
            }));
    }

    /**
     * @brief Get the current role
     * @return The current role
     */
    Role get_current_role() const {
        return current_role;
    }

    /**
     * @brief Set the current role
     * @param role The role to set
     */
    void set_current_role(Role role) {
        current_role = role;
    }

    /**
     * @brief Get the current name
     * @return The current name
     */
    const std::string& get_current_name() const {
        return current_name;
    }

    /**
     * @brief Set the current name
     * @param name The name to set
     */
    void set_current_name(std::string name) {
        current_name = std::move(name);
    }

    /**
     * @brief Get the current work mode
     * @return The current work mode
     */
    WorkMode get_current_mode() const {
        return current_mode;
    }

    /**
     * @brief Set the current work mode
     * @param mode The mode to set
     */
    void set_current_mode(WorkMode mode) {
        current_mode = mode;
    }

  private:
    Role current_role = Role::Peripheral;
    std::string current_name = "HMSoft";
    WorkMode current_mode = WorkMode::TransmissionMode;
    
    struct CommandResponse {
        std::string response{};
        hal::UartStatus status{hal::UartStatus::Ok};
    };
    
    std::unordered_map<std::string, CommandResponse> command_responses;
    std::unordered_map<std::string, 
                      std::function<void(const std::string&, std::string&, hal::UartStatus&)>> 
                      command_handlers_;
};

/**
 * @brief Create a shared pointer to an EnhancedUARTMock
 * @param data_size The data size to report (defaults to 8-bit)
 * @return A shared pointer to an EnhancedUARTMock
 */
inline hal::AnyUartPort create_enhanced_uart_mock(
    hal::UartDataSize data_size = hal::UartDataSize::DataSize8b) {
    auto mock = std::make_shared<EnhancedUARTMock>(data_size);
    return mock;
}

} // namespace hm11::tests::mocks

#endif // HM11_TESTS_MOCKS_HM11_DRIVER_MOCK_HPP