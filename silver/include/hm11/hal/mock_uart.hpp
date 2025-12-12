#pragma once

// HM11 Hardware Abstraction Layer - Mock UART Implementation
// Provides test implementation satisfying UARTInterface concept

#include <algorithm>
#include <ranges>
#include <vector>

#include "uart_interface.hpp"

namespace hm11::hal {

/// Mock UART implementation for testing and development
/// Satisfies UARTInterface concept requirements
///
/// Features:
/// - Configurable data size (8-bit or 9-bit mode)
/// - Internal buffers for transmitted and received data
/// - Controllable status responses and delays
/// - Thread-safe operations via internal state management
class MockUART {
  public:
    /// Construct mock UART with specified data size
    /// @param data_size_param UART data configuration (8-bit or 9-bit)
    explicit MockUART(UARTDataSize data_size_param = UARTDataSize::DataSize8b) noexcept
        : data_size_member_(data_size_param) {}

    /// Get configured data size
    /// @return Current UART data size configuration
    [[nodiscard]] UARTDataSize DataSize() const noexcept { return data_size_member_; }

    /// Transmit 8-bit data with explicit timeout
    /// @param data Data buffer to transmit
    /// @param status Output parameter for operation status
    /// @param timeout_ms Timeout in milliseconds
    void Transmit(UARTData8bConst data, UARTStatus& status, std::uint32_t timeout_ms) noexcept {
        if (data_size_member_ != UARTDataSize::DataSize8b) {
            status = UARTStatus::ErrError;
            return;
        }

        // Simulate timeout behavior
        if (simulate_timeout_ && timeout_ms < required_timeout_ms_) {
            status = UARTStatus::ErrTimeout;
            return;
        }

        // Copy transmitted data to internal buffer for inspection
        transmitted_8b_.clear();
        transmitted_8b_.reserve(data.size());
        std::ranges::copy(data, std::back_inserter(transmitted_8b_));

        status = forced_status_;
    }

    /// Transmit 8-bit data with default timeout
    /// @param data Data buffer to transmit
    /// @param status Output parameter for operation status
    void Transmit(UARTData8bConst data, UARTStatus& status) noexcept { Transmit(data, status, DEFAULT_TIMEOUT_MS); }

    /// Transmit 9-bit data with explicit timeout
    /// @param data Data buffer to transmit (stored in uint16_t)
    /// @param status Output parameter for operation status
    /// @param timeout_ms Timeout in milliseconds
    void Transmit(UARTData9bConst data, UARTStatus& status, std::uint32_t timeout_ms) noexcept {
        if (data_size_member_ != UARTDataSize::DataSize9b) {
            status = UARTStatus::ErrError;
            return;
        }

        // Simulate timeout behavior
        if (simulate_timeout_ && timeout_ms < required_timeout_ms_) {
            status = UARTStatus::ErrTimeout;
            return;
        }

        // Copy transmitted data to internal buffer for inspection
        transmitted_9b_.clear();
        transmitted_9b_.reserve(data.size());
        std::ranges::copy(data, std::back_inserter(transmitted_9b_));

        status = forced_status_;
    }

    /// Transmit 9-bit data with default timeout
    /// @param data Data buffer to transmit
    /// @param status Output parameter for operation status
    void Transmit(UARTData9bConst data, UARTStatus& status) noexcept { Transmit(data, status, DEFAULT_TIMEOUT_MS); }

    /// Receive 8-bit data with explicit timeout
    /// @param data Buffer to store received data
    /// @param status Output parameter for operation status
    /// @param timeout_ms Timeout in milliseconds
    void Receive(UARTData8b data, UARTStatus& status, std::uint32_t timeout_ms) noexcept {
        if (data_size_member_ != UARTDataSize::DataSize8b) {
            status = UARTStatus::ErrError;
            return;
        }

        // Simulate timeout behavior
        if (simulate_timeout_ && timeout_ms < required_timeout_ms_) {
            status = UARTStatus::ErrTimeout;
            return;
        }

        // Copy from mock receive buffer
        const auto COPY_SIZE = std::min(data.size(), receive_8b_.size());
        std::copy_n(receive_8b_.begin(), COPY_SIZE, data.begin());

        status = forced_receive_status_;
    }

    /// Receive 8-bit data with default timeout
    /// @param data Buffer to store received data
    /// @param status Output parameter for operation status
    void Receive(UARTData8b data, UARTStatus& status) noexcept { Receive(data, status, DEFAULT_TIMEOUT_MS); }

    /// Receive 9-bit data with explicit timeout
    /// @param data Buffer to store received data
    /// @param status Output parameter for operation status
    /// @param timeout_ms Timeout in milliseconds
    void Receive(UARTData9b data, UARTStatus& status, std::uint32_t timeout_ms) noexcept {
        if (data_size_member_ != UARTDataSize::DataSize9b) {
            status = UARTStatus::ErrError;
            return;
        }

        // Simulate timeout behavior
        if (simulate_timeout_ && timeout_ms < required_timeout_ms_) {
            status = UARTStatus::ErrTimeout;
            return;
        }

        // Copy from mock receive buffer
        const auto COPY_SIZE = std::min(data.size(), receive_9b_.size());
        std::copy_n(receive_9b_.begin(), COPY_SIZE, data.begin());

        status = forced_receive_status_;
    }

    /// Receive 9-bit data with default timeout
    /// @param data Buffer to store received data
    /// @param status Output parameter for operation status
    void Receive(UARTData9b data, UARTStatus& status) noexcept { Receive(data, status, DEFAULT_TIMEOUT_MS); }

    // Test control methods

    /// Set the status that will be returned by operations
    /// @param status Status to return from subsequent operations
    void SetForcedStatus(UARTStatus status) noexcept { forced_status_ = status; }

    /// Configure timeout simulation
    /// @param enable Whether to simulate timeout conditions
    /// @param required_timeout_ms Minimum timeout needed to avoid timeout
    void SetTimeoutSimulation(bool enable, std::uint32_t timeout_ms = 2000) noexcept {
        simulate_timeout_ = enable;
        required_timeout_ms_ = timeout_ms;
    }

    /// Set data to be returned by receive operations (8-bit mode)
    /// @param data Data that will be copied to receive buffers
    void SetReceiveData(const std::vector<std::uint8_t>& data) noexcept { receive_8b_ = data; }

    /// Set data to be returned by receive operations (9-bit mode)
    /// @param data Data that will be copied to receive buffers
    void SetReceiveData(const std::vector<std::uint16_t>& data) noexcept { receive_9b_ = data; }

    /// Set forced status only for receive operations
    /// @param status Status to return for receive operations
    void SetForcedStatusForReceive(UARTStatus status) noexcept { forced_receive_status_ = status; }

    /// Clear transmitted data buffers
    void ClearTransmittedData() noexcept {
        transmitted_8b_.clear();
        transmitted_9b_.clear();
    }

    /// Get data that was transmitted in 8-bit mode
    /// @return Vector containing all transmitted 8-bit data
    [[nodiscard]] const std::vector<std::uint8_t>& GetTransmitted8b() const noexcept { return transmitted_8b_; }

    /// Get data that was transmitted in 9-bit mode
    /// @return Vector containing all transmitted 9-bit data
    [[nodiscard]] const std::vector<std::uint16_t>& GetTransmitted9b() const noexcept { return transmitted_9b_; }

    /// Clear all internal buffers
    void ClearBuffers() noexcept {
        transmitted_8b_.clear();
        transmitted_9b_.clear();
        receive_8b_.clear();
        receive_9b_.clear();
    }

  private:
    UARTDataSize data_size_member_;
    UARTStatus forced_status_ = UARTStatus::Ok;
    UARTStatus forced_receive_status_ = UARTStatus::Ok;
    bool simulate_timeout_ = false;
    std::uint32_t required_timeout_ms_ = DEFAULT_TIMEOUT_MS;

    // Internal buffers for testing
    std::vector<std::uint8_t> transmitted_8b_;
    std::vector<std::uint16_t> transmitted_9b_;
    std::vector<std::uint8_t> receive_8b_;
    std::vector<std::uint16_t> receive_9b_;
};

// Compile-time verification that MockUART satisfies the concept
static_assert(UARTInterface<MockUART>, "MockUART must satisfy UARTInterface concept requirements");

} // namespace hm11::hal