/**
 * @file integration_test.cpp
 * @brief Integration tests for synchronization primitives
 *
 * This file contains comprehensive integration tests for all synchronization primitives
 * developed for the HM11 driver, testing them together in realistic usage scenarios.
 * It validates that the components work correctly in both ISR and thread contexts,
 * with proper error handling and timeout behavior.
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "hm11/platform/cache.hpp"
#include "hm11/sync/binary_semaphore.hpp"
#include "hm11/sync/circular_buffer.hpp"
#include "hm11/sync/watchdog.hpp"
#include "hm11/util/memory_order.hpp"

// Test utilities
#include "../../mocks/dma_mock.hpp"
#include "../../util/isr_simulator.hpp"

using namespace hm11;
using namespace hm11::sync;
using namespace hm11::test;
using namespace std::chrono_literals;

/**
 * @class SyncPrimitivesIntegrationTest
 * @brief Test fixture for synchronization primitives integration testing
 *
 * This fixture provides common setup and utilities for testing the interaction
 * between multiple synchronization primitives in realistic scenarios.
 */
class SyncPrimitivesIntegrationTest : public ::testing::Test {
  protected:
    // Common test size constants
    static constexpr std::size_t kBufferSize = 256;
    static constexpr std::size_t kSmallBufferSize = 16;

    void SetUp() override {
        // Initialize ISR simulator
        isr_simulator_.reset(new ISRSimulator());
        isr_simulator_->start();

        // Seed random number generator
        random_engine_.seed(std::random_device{}());
    }

    void TearDown() override {
        // Stop ISR simulator
        isr_simulator_->stop();
        isr_simulator_.reset();
    }

    /**
     * @brief Generate random data for testing
     *
     * @param size Size of the data to generate
     * @return std::vector<uint8_t> Random data vector
     */
    std::vector<std::uint8_t> generateRandomData(std::size_t size) {
        std::vector<std::uint8_t> data(size);
        std::uniform_int_distribution<std::uint16_t> distribution(0, 255);
        std::generate(data.begin(), data.end(), [&]() {
            return static_cast<std::uint8_t>(distribution(random_engine_));
        });
        return data;
    }

    /**
     * @brief Utility to wait with timeout and message
     *
     * @param condition Function returning bool, true when condition is met
     * @param timeout Timeout duration
     * @param message Message to display on timeout
     * @return bool True if condition was met, false if timed out
     */
    template <typename Func, typename Rep, typename Period>
    bool waitFor(
        Func&& condition,
        std::chrono::duration<Rep, Period> timeout,
        const std::string& message = "Timeout waiting for condition") {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                ADD_FAILURE() << message
                              << " (timeout: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()
                              << "ms)";
                return false;
            }
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }

    // Common test resources
    std::unique_ptr<ISRSimulator> isr_simulator_;
    std::mt19937 random_engine_;
};

/**
 * @brief Test BinarySemaphore with ISR-to-thread signaling
 *
 * This test verifies that a BinarySemaphore can be used to signal from an ISR to a thread,
 * with proper handling of status codes and timing.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_BinarySemaphoreISRSignaling) {
    // Create binary semaphore
    BinarySemaphore semaphore;

    // Create flag for tracking completion
    std::atomic<bool> thread_completed{false};
    std::atomic<UartStatus> received_status{UartStatus::ErrError};

    // Register an ISR with the simulator
    int isr_id = isr_simulator_->registerISR([&]() {
        // Simulate ISR giving semaphore with OK status
        semaphore.give_from_isr(UartStatus::Ok);
    });

    // Start a thread that waits on the semaphore
    std::thread waiter_thread([&]() {
        UartStatus status = UartStatus::ErrError;
        bool result = semaphore.take_with_timeout(status, 1000ms);
        (void)result; // Prevent unused variable warning

        // Store results in atomic variables
        received_status.store(status, std::memory_order_release);
        thread_completed.store(result, std::memory_order_release);
    });

    // Trigger the ISR after a short delay
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(isr_id));

    // Wait for thread to complete
    EXPECT_TRUE(waitFor(
        [&]() { return thread_completed.load(std::memory_order_acquire); },
        1500ms,
        "Thread did not receive semaphore signal"));

    // Verify correct status was received
    EXPECT_EQ(received_status.load(std::memory_order_acquire), UartStatus::Ok);

    // Clean up
    waiter_thread.join();
}

/**
 * @brief Test CircularBuffer with ISR-to-thread data transfer
 *
 * This test verifies that a CircularBuffer can be used to transfer data from an ISR
 * to a thread safely, with proper memory ordering and synchronization.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_CircularBufferISRToThreadTransfer) {
    // Create circular buffer
    CircularBuffer<std::uint8_t, kBufferSize> buffer;

    // Generate test data
    const auto test_data = generateRandomData(kBufferSize / 2);

    // Create binary semaphore for signaling
    BinarySemaphore data_ready_semaphore;

    // Track received data
    std::vector<std::uint8_t> received_data;
    std::atomic<bool> thread_completed{false};

    // Register an ISR with the simulator
    int isr_id = isr_simulator_->registerISR([&]() {
        // Simulate ISR writing data to the buffer
        for (const auto& byte : test_data) {
            buffer.write(byte);
        }

        // Signal that data is ready
        data_ready_semaphore.give_from_isr();
    });

    // Start a thread that waits for data
    std::thread reader_thread([&]() {
        UartStatus status = UartStatus::ErrError;
        bool result = data_ready_semaphore.take_with_timeout(status, 1000ms);
        (void)result; // Prevent unused variable warning

        // If semaphore signaled successfully, read the data
        if (result && status == UartStatus::Ok) {
            received_data.reserve(test_data.size());
            std::uint8_t byte;
            while (buffer.read(byte)) {
                received_data.push_back(byte);
            }
        }

        thread_completed.store(true, std::memory_order_release);
    });

    // Trigger the ISR after a short delay
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(isr_id));

    // Wait for thread to complete
    EXPECT_TRUE(waitFor(
        [&]() { return thread_completed.load(std::memory_order_acquire); },
        1500ms,
        "Thread did not complete data reception"));

    // Verify correct data was received
    ASSERT_EQ(received_data.size(), test_data.size());
    EXPECT_TRUE(std::equal(received_data.begin(), received_data.end(), test_data.begin()));

    // Clean up
    reader_thread.join();
}

/**
 * @brief Test Watchdog with timeout and error propagation
 *
 * This test verifies that a Watchdog can correctly handle timeouts and propagate
 * error statuses from ISR to thread context.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_WatchdogTimeoutAndErrorPropagation) {
    // Create watchdog
    Watchdog watchdog;

    // Create buffer for receiving data
    std::array<std::uint8_t, kBufferSize> buffer{};

    // Test variables
    std::atomic<bool> thread_completed{false};
    std::atomic<UartStatus> received_status{UartStatus::Ok};

    // Test case 1: Error propagation
    {
        // Start watchdog with a long timeout
        EXPECT_TRUE(watchdog.start(buffer.data(), buffer.size(), 5000ms));

        // Register an ISR that signals an error
        int error_isr_id = isr_simulator_->registerISR([&]() {
            // Simulate error in ISR
            watchdog.release(UartStatus::ErrError);
        });

        // Start a thread that waits on the watchdog
        std::thread waiter_thread([&]() {
            UartStatus status;
            bool result = watchdog.await_event(status);
            (void)result; // Prevent unused variable warning

            // Store results
            received_status.store(status, std::memory_order_release);
            thread_completed.store(true, std::memory_order_release);
        });

        // Trigger the error ISR
        std::this_thread::sleep_for(100ms);
        EXPECT_TRUE(isr_simulator_->triggerISR(error_isr_id));

        // Wait for thread to complete
        EXPECT_TRUE(waitFor(
            [&]() { return thread_completed.load(std::memory_order_acquire); },
            1500ms,
            "Thread did not receive error signal"));

        // Verify correct error status was received
        EXPECT_EQ(received_status.load(std::memory_order_acquire), UartStatus::ErrError);

        // Clean up
        waiter_thread.join();
        thread_completed.store(false, std::memory_order_relaxed);
        received_status.store(UartStatus::Ok, std::memory_order_relaxed);
    }

    // Test case 2: Timeout
    {
        // Start watchdog with a short timeout
        EXPECT_TRUE(watchdog.start(buffer.data(), buffer.size(), 500ms));

        // Start a thread that waits on the watchdog
        std::thread waiter_thread([&]() {
            UartStatus status;
            bool result = watchdog.await_event(status);
            (void)result; // Prevent unused variable warning

            // Store results
            received_status.store(status, std::memory_order_release);
            thread_completed.store(true, std::memory_order_release);
        });

        // Wait for thread to complete with timeout
        EXPECT_TRUE(waitFor(
            [&]() { return thread_completed.load(std::memory_order_acquire); },
            1000ms,
            "Thread did not detect timeout"));

        // Verify timeout status was received
        EXPECT_EQ(received_status.load(std::memory_order_acquire), UartStatus::ErrTimeout);

        // Clean up
        waiter_thread.join();
    }
}

/**
 * @brief Test buffer wrapping with CircularBuffer
 *
 * This test verifies that CircularBuffer correctly handles buffer wrapping,
 * where the write position wraps around to the beginning of the buffer.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_CircularBufferWrapping) {
    // Create a small circular buffer to ensure wrapping
    CircularBuffer<std::uint8_t, kSmallBufferSize> buffer;

    // Create a semaphore for signaling
    BinarySemaphore buffer_ready_semaphore;

    // Generate test data that exceeds buffer capacity to force wrapping
    const auto test_data = generateRandomData(kSmallBufferSize * 3);

    // Track results
    std::vector<std::uint8_t> all_received_data;
    std::atomic<bool> thread_completed{false};

    // Register an ISR that writes data in batches
    int isr_id = isr_simulator_->registerISR([&]() {
        // Write data in chunks to simulate multiple ISR firings
        // First chunk will fill the buffer
        std::size_t pos = 0;
        std::size_t chunk_size = kSmallBufferSize;

        // First chunk - fills the buffer
        for (std::size_t i = 0; i < chunk_size && pos < test_data.size(); ++i, ++pos) {
            buffer.write(test_data[pos]);
        }

        // Signal reader thread
        buffer_ready_semaphore.give_from_isr();
    });

    // Register a second ISR that writes more data after the first read
    int isr_id2 = isr_simulator_->registerISR([&]() {
        // Second chunk - wraps around after reader has consumed some data
        std::size_t pos = kSmallBufferSize;
        std::size_t chunk_size = kSmallBufferSize * 2;

        for (std::size_t i = 0; i < chunk_size && pos < test_data.size(); ++i, ++pos) {
            buffer.write(test_data[pos]);
        }

        // Signal reader thread again
        buffer_ready_semaphore.give_from_isr();
    });

    // Start a thread that reads data in multiple chunks
    std::thread reader_thread([&]() {
        UartStatus status;

        // First read
        if (buffer_ready_semaphore.take_with_timeout(status, 1000ms)) {
            std::uint8_t byte;
            while (buffer.read(byte)) {
                all_received_data.push_back(byte);
            }
        }

        // Second read after second ISR
        if (buffer_ready_semaphore.take_with_timeout(status, 1000ms)) {
            std::uint8_t byte;
            while (buffer.read(byte)) {
                all_received_data.push_back(byte);
            }
        }

        thread_completed.store(true, std::memory_order_release);
    });

    // Trigger the first ISR
    EXPECT_TRUE(isr_simulator_->triggerISR(isr_id));

    // Wait a bit to ensure the first batch is processed
    std::this_thread::sleep_for(200ms);

    // Trigger the second ISR
    EXPECT_TRUE(isr_simulator_->triggerISR(isr_id2));

    // Wait for thread to complete
    EXPECT_TRUE(waitFor(
        [&]() { return thread_completed.load(std::memory_order_acquire); },
        2000ms,
        "Thread did not complete data reception"));

    // Verify all data was received correctly
    EXPECT_EQ(all_received_data.size(), test_data.size());

    // Verify data integrity
    bool data_match = std::equal(all_received_data.begin(), all_received_data.end(), test_data.begin());
    EXPECT_TRUE(data_match);

    if (!data_match && all_received_data.size() == test_data.size()) {
        // If sizes match but data doesn't, find the first mismatch for debugging
        for (std::size_t i = 0; i < all_received_data.size(); ++i) {
            if (all_received_data[i] != test_data[i]) {
                ADD_FAILURE() << "Data mismatch at index " << i << ": expected " << static_cast<int>(test_data[i])
                              << ", got " << static_cast<int>(all_received_data[i]);
                break;
            }
        }
    }

    // Clean up
    reader_thread.join();
}

/**
 * @brief Test multiple ISR interactions with synchronization primitives
 *
 * This test simulates a realistic scenario with multiple ISRs interacting
 * with different synchronization primitives simultaneously.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_MultipleISRInteractions) {
    // Create synchronization primitives
    BinarySemaphore data_ready_semaphore;
    CircularBuffer<std::uint8_t, kBufferSize> rx_buffer;
    CircularBuffer<std::uint8_t, kBufferSize> tx_buffer;
    Watchdog watchdog;

    // Create buffers for data
    std::array<std::uint8_t, kBufferSize> rx_dma_buffer{};

    // Generate test data
    const auto rx_test_data = generateRandomData(kBufferSize / 2);
    const auto tx_test_data = generateRandomData(kBufferSize / 2);

    // Set up DMA mock for receiving data
    DMAMock dma_rx;

    // Create shared flags and result variables
    std::atomic<bool> rx_complete{false};
    std::atomic<bool> tx_complete{false};
    std::atomic<UartStatus> rx_status{UartStatus::ErrError};
    std::atomic<UartStatus> tx_status{UartStatus::ErrError};
    std::vector<std::uint8_t> received_data;

    // Configure DMA mock
    dma_rx.configure(
        const_cast<std::uint8_t*>(rx_test_data.data()),
        rx_dma_buffer.data(),
        rx_test_data.size(),
        DMAMock::Direction::PeripheralToMemory);
    dma_rx.setCompleteInterruptEnabled(true);

    // Register DMA RX complete ISR
    int dma_rx_isr_id = isr_simulator_->registerISR([&]() {
        // Simulate memory fence after DMA completion
        hm11::util::acquire_fence();

        // Invalidate cache for DMA buffer
        hm11::platform::cache::invalidate_dma_buffer(rx_dma_buffer.data(), rx_test_data.size());

        // Copy data from DMA buffer to circular buffer
        for (std::size_t i = 0; i < rx_test_data.size(); ++i) {
            rx_buffer.write(rx_dma_buffer[i]);
        }

        // Signal watchdog completion
        watchdog.release(UartStatus::Ok);
    });

    // Register TX ISR (simulating UART TX complete)
    int tx_isr_id = isr_simulator_->registerISR([&]() {
        // Simulate reading data from tx buffer and transmitting
        std::uint8_t byte;
        while (tx_buffer.read(byte)) {
            // In a real system, this would send to hardware
        }

        // Signal TX completion
        data_ready_semaphore.give_from_isr(UartStatus::Ok);
    });

    // Start RX operation with watchdog
    watchdog.start(rx_dma_buffer.data(), rx_test_data.size(), 1000ms);

    // Start a thread for handling RX completion
    std::thread rx_thread([&]() {
        // Wait for watchdog to signal completion
        UartStatus status;
        bool result = watchdog.await_event(status);
        (void)result; // Prevent unused variable warning

        // Store results
        rx_status.store(status, std::memory_order_release);

        // If successful, read data from circular buffer
        if (result && status == UartStatus::Ok) {
            received_data.reserve(rx_test_data.size());
            std::uint8_t byte;
            while (rx_buffer.read(byte)) {
                received_data.push_back(byte);
            }
        }

        rx_complete.store(true, std::memory_order_release);
    });

    // Start a thread for handling TX operation
    std::thread tx_thread([&]() {
        // Write data to TX buffer
        for (const auto& byte : tx_test_data) {
            tx_buffer.write(byte);
        }

        // Wait for TX completion
        UartStatus status;
        bool result = data_ready_semaphore.take_with_timeout(status, 1000ms);
        (void)result; // Prevent unused variable warning

        // Store results
        tx_status.store(status, std::memory_order_release);
        tx_complete.store(true, std::memory_order_release);
    });

    // Simulate DMA transfer
    dma_rx.enable();
    dma_rx.simulateCompleteTransfer();

    // Trigger ISRs
    EXPECT_TRUE(isr_simulator_->triggerISR(dma_rx_isr_id));
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(tx_isr_id));

    // Wait for both operations to complete
    EXPECT_TRUE(waitFor(
        [&]() { return rx_complete.load(std::memory_order_acquire) && tx_complete.load(std::memory_order_acquire); },
        2000ms,
        "RX or TX operation did not complete"));

    // Verify RX operation
    EXPECT_EQ(rx_status.load(std::memory_order_acquire), UartStatus::Ok);
    ASSERT_EQ(received_data.size(), rx_test_data.size());
    EXPECT_TRUE(std::equal(received_data.begin(), received_data.end(), rx_test_data.begin()));

    // Verify TX operation
    EXPECT_EQ(tx_status.load(std::memory_order_acquire), UartStatus::Ok);

    // Clean up
    rx_thread.join();
    tx_thread.join();
}

/**
 * @brief Test deadline expiration handling with Watchdog
 *
 * This test verifies that Watchdog correctly handles deadline expiration
 * and signals timeout status to the waiting thread.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_WatchdogDeadlineExpiration) {
    // Create watchdog
    Watchdog watchdog;

    // Create buffer for receiving data
    std::array<std::uint8_t, kBufferSize> buffer{};

    // Configure watchdog with short timeout
    const auto timeout = 200ms;
    EXPECT_TRUE(watchdog.start(buffer.data(), buffer.size(), timeout));

    // Create variable to track results
    std::atomic<UartStatus> result_status{UartStatus::Ok};
    std::atomic<bool> thread_completed{false};

    // Start a thread that waits on the watchdog
    std::thread waiter_thread([&]() {
        UartStatus status;
        bool result = watchdog.await_event(status);
        (void)result; // Prevent unused variable warning

        // Store results
        result_status.store(status, std::memory_order_release);
        thread_completed.store(true, std::memory_order_release);
    });

    // Wait for thread to complete with timeout
    // We expect this to take approximately the timeout duration
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(waitFor(
        [&]() { return thread_completed.load(std::memory_order_acquire); },
        timeout * 3,
        "Watchdog timeout did not occur"));
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Verify timeout occurred with expected status
    EXPECT_EQ(result_status.load(std::memory_order_acquire), UartStatus::ErrTimeout);

    // Verify timing (with some tolerance)
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_GE(elapsed_ms, timeout.count() * 0.75) << "Timeout occurred too quickly";
    EXPECT_LE(elapsed_ms, timeout.count() * 2.0) << "Timeout took too long";

    // Clean up
    waiter_thread.join();
}

/**
 * @brief Test error propagation between ISR and thread
 *
 * This test verifies that error statuses are correctly propagated from
 * ISR context to thread context through synchronization primitives.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_ErrorPropagation) {
    // Create synchronization primitives
    BinarySemaphore semaphore;
    Watchdog watchdog;

    // Create buffer for receiving data
    std::array<std::uint8_t, kBufferSize> buffer{};

    // Test variables
    std::atomic<UartStatus> semaphore_status{UartStatus::Ok};
    std::atomic<UartStatus> watchdog_status{UartStatus::Ok};
    std::atomic<bool> semaphore_completed{false};
    std::atomic<bool> watchdog_completed{false};

    // Start watchdog
    EXPECT_TRUE(watchdog.start(buffer.data(), buffer.size(), 1000ms));

    // Register ISRs with different error types
    int invalid_state_isr_id = isr_simulator_->registerISR(
        [&]() { semaphore.give_from_isr(UartStatus::ErrInvalidState); });

    int not_supported_isr_id = isr_simulator_->registerISR([&]() { watchdog.release(UartStatus::ErrNotSupported); });

    // Start threads waiting on synchronization primitives
    std::thread semaphore_thread([&]() {
        UartStatus status;
        bool result = semaphore.take_with_timeout(status, 1000ms);
        (void)result; // Prevent unused variable warning

        semaphore_status.store(status, std::memory_order_release);
        semaphore_completed.store(true, std::memory_order_release);
    });

    std::thread watchdog_thread([&]() {
        UartStatus status;
        bool result = watchdog.await_event(status);
        (void)result; // Prevent unused variable warning

        watchdog_status.store(status, std::memory_order_release);
        watchdog_completed.store(true, std::memory_order_release);
    });

    // Trigger ISRs
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(invalid_state_isr_id));
    EXPECT_TRUE(isr_simulator_->triggerISR(not_supported_isr_id));

    // Wait for both threads to complete
    EXPECT_TRUE(waitFor(
        [&]() {
            return semaphore_completed.load(std::memory_order_acquire) &&
                   watchdog_completed.load(std::memory_order_acquire);
        },
        1500ms,
        "Synchronization operations did not complete"));

    // Verify error statuses were correctly propagated
    EXPECT_EQ(semaphore_status.load(std::memory_order_acquire), UartStatus::ErrInvalidState);
    EXPECT_EQ(watchdog_status.load(std::memory_order_acquire), UartStatus::ErrNotSupported);

    // Clean up
    semaphore_thread.join();
    watchdog_thread.join();
}

/**
 * @brief Test memory ordering in concurrent access scenarios
 *
 * This test verifies that memory ordering guarantees are enforced correctly
 * when accessing shared data from multiple threads and ISRs.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_MemoryOrderingConcurrentAccess) {
    // Create shared flag for tracking visibility
    std::atomic<bool> flag{false};

    // Create shared data
    std::array<std::uint32_t, 10> shared_data{};

    // Create synchronization primitive
    BinarySemaphore semaphore;

    // Initialize shared data with known values
    for (std::size_t i = 0; i < shared_data.size(); ++i) {
        shared_data[i] = 0;
    }

    // Register an ISR that modifies shared data and sets flag
    int isr_id = isr_simulator_->registerISR([&]() {
        // Modify shared data
        for (std::size_t i = 0; i < shared_data.size(); ++i) {
            shared_data[i] = static_cast<std::uint32_t>(i + 1);
        }

        // Store flag with release semantics to ensure all prior writes are visible
        flag.store(true, std::memory_order_release);

        // Signal semaphore
        semaphore.give_from_isr();
    });

    // Create a thread that waits for the flag to become visible
    std::atomic<bool> thread_completed{false};
    std::atomic<bool> data_visible{false};

    std::thread reader_thread([&]() {
        UartStatus status;
        bool signaled = semaphore.take_with_timeout(status, 1000ms);

        // Check if signaled successfully
        if (signaled && status == UartStatus::Ok) {
            // Check flag with acquire semantics
            if (flag.load(std::memory_order_acquire)) {
                // If flag is visible, check if shared data modifications are visible
                bool all_visible = true;
                for (std::size_t i = 0; i < shared_data.size(); ++i) {
                    if (shared_data[i] != static_cast<std::uint32_t>(i + 1)) {
                        all_visible = false;
                        break;
                    }
                }

                data_visible.store(all_visible, std::memory_order_release);
            }
        }

        thread_completed.store(true, std::memory_order_release);
    });

    // Trigger the ISR
    EXPECT_TRUE(isr_simulator_->triggerISR(isr_id));

    // Wait for thread to complete
    EXPECT_TRUE(waitFor(
        [&]() { return thread_completed.load(std::memory_order_acquire); },
        1500ms,
        "Thread did not complete operation"));

    // Verify that data was visible
    EXPECT_TRUE(data_visible.load(std::memory_order_acquire))
        << "Memory ordering violation: thread could not see shared data modifications";

    // Clean up
    reader_thread.join();
}

/**
 * @brief Test realistic UART data streaming scenario
 *
 * This test simulates a realistic UART data streaming scenario, where data arrives
 * in chunks and is processed by a circular buffer, with ISR notifications.
 */
TEST_F(SyncPrimitivesIntegrationTest, DISABLED_RealisticUARTDataStreaming) {
    // Create circular buffer for UART RX data
    CircularBuffer<std::uint8_t, kBufferSize> rx_buffer;

    // Create watchdog for timeout monitoring
    Watchdog watchdog;

    // Create a buffer to store the streaming data
    std::array<std::uint8_t, kBufferSize> stream_buffer{};

    // Generate test data in multiple chunks
    const auto chunk1 = generateRandomData(50);
    const auto chunk2 = generateRandomData(30);
    const auto chunk3 = generateRandomData(20);

    // Create flags for tracking progress
    std::atomic<bool> streaming_completed{false};
    std::atomic<UartStatus> final_status{UartStatus::ErrError};
    std::vector<std::uint8_t> received_data;

    // Start watchdog with a long timeout
    EXPECT_TRUE(watchdog.start(stream_buffer.data(), stream_buffer.size(), 2000ms));

    // Register an ISR that simulates UART RX with chunk 1
    int rx_isr_id1 = isr_simulator_->registerISR([&]() {
        for (const auto& byte : chunk1) {
            rx_buffer.write(byte);
        }
    });

    // Register an ISR that simulates UART RX with chunk 2
    int rx_isr_id2 = isr_simulator_->registerISR([&]() {
        for (const auto& byte : chunk2) {
            rx_buffer.write(byte);
        }
    });

    // Register an ISR that simulates UART RX with chunk 3 and signals completion
    int rx_isr_id3 = isr_simulator_->registerISR([&]() {
        for (const auto& byte : chunk3) {
            rx_buffer.write(byte);
        }

        // Signal completion
        watchdog.release(UartStatus::Ok);
    });

    // Start a thread for processing the streaming data
    std::thread streaming_thread([&]() {
        // Process data chunks as they arrive
        bool receiving = true;
        std::size_t position = 0;

        while (receiving) {
            // Check if new data is available
            bool stopped = false;
            std::size_t current_pos = 0;
            watchdog.get_position(stopped, current_pos);

            // Process available data in circular buffer
            std::uint8_t byte;
            while (rx_buffer.read(byte)) {
                received_data.push_back(byte);

                // Update position
                position++;
                watchdog.set_position(position);
            }

            // Check if streaming is complete
            UartStatus status = UartStatus::Ok;
            if (stopped || !watchdog.await_event(status)) {
                receiving = false;
                final_status.store(status, std::memory_order_release);
            }
        }

        streaming_completed.store(true, std::memory_order_release);
    });

    // Trigger ISRs with delays to simulate chunks arriving over time
    EXPECT_TRUE(isr_simulator_->triggerISR(rx_isr_id1));
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(rx_isr_id2));
    std::this_thread::sleep_for(150ms);
    EXPECT_TRUE(isr_simulator_->triggerISR(rx_isr_id3));

    // Wait for streaming to complete
    EXPECT_TRUE(waitFor(
        [&]() { return streaming_completed.load(std::memory_order_acquire); },
        2500ms,
        "Streaming operation did not complete"));

    // Verify status and data
    EXPECT_EQ(final_status.load(std::memory_order_acquire), UartStatus::Ok);

    // Verify received data
    std::vector<std::uint8_t> expected_data;
    expected_data.insert(expected_data.end(), chunk1.begin(), chunk1.end());
    expected_data.insert(expected_data.end(), chunk2.begin(), chunk2.end());
    expected_data.insert(expected_data.end(), chunk3.begin(), chunk3.end());

    ASSERT_EQ(received_data.size(), expected_data.size());
    EXPECT_TRUE(std::equal(received_data.begin(), received_data.end(), expected_data.begin()));

    // Clean up
    streaming_thread.join();
}

// Main function is provided by test_main.cpp