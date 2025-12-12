// HM11 Synchronization Primitives - Stress Tests
// High-load testing for race condition detection and ThreadSanitizer validation

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <random>
#include <thread>
#include <vector>

#include "hm11/sync.hpp"

using namespace hm11::sync;
using namespace hm11::hal;

/// Stress test with multiple producers and consumers on circular buffer
TEST(SyncStressTest, MultipleProducerConsumerBuffer) {
    constexpr int NUM_PRODUCERS = 4;
    constexpr int NUM_CONSUMERS = 2;
    constexpr int ITEMS_PER_PRODUCER = 1000;
    constexpr int BUFFER_SIZE = 64;

    CircularBuffer<std::uint32_t, BUFFER_SIZE> buffer;
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start multiple producers
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            std::random_device rd;
            std::mt19937 gen(rd() + p);
            std::uniform_int_distribution<> delay_dist(0, 5);

            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                std::uint32_t value = (p << 16) | i; // Producer ID + sequence

                // OVERWRITE semantics - always succeeds
                buffer.WriteFromISR(value);

                total_produced.fetch_add(1);

                // Random delay to create contention
                if (i % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen) * 10));
                }
            }
        });
    }

    // Start multiple consumers
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&, c]() {
            std::vector<std::uint32_t> consumed_values;

            while (!producers_done.load() || !buffer.IsEmpty()) {
                auto data = buffer.ReadAvailable();

                if (!data.empty()) {
                    // Copy data before marking consumed
                    for (auto value : data) {
                        consumed_values.push_back(value);
                    }

                    buffer.MarkConsumed(data.size());
                    total_consumed.fetch_add(data.size());
                } else {
                    std::this_thread::yield();
                }
            }

            // Verify no duplicate values consumed by this consumer
            std::sort(consumed_values.begin(), consumed_values.end());
            auto unique_end = std::unique(consumed_values.begin(), consumed_values.end());
            EXPECT_EQ(unique_end, consumed_values.end()) << "Consumer " << c << " found duplicates";
        });
    }

    // Wait for all producers
    for (auto& producer : producers) {
        producer.join();
    }
    producers_done.store(true);

    // Wait for all consumers
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(total_produced.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    // With OVERWRITE semantics, consumed may be <= produced (some data may have been overwritten)
    EXPECT_LE(total_consumed.load(), total_produced.load());
    EXPECT_TRUE(buffer.IsEmpty());
}

/// Stress test with rapid semaphore signal/wait cycles
TEST(SyncStressTest, RapidSemaphoreSignaling) {
    constexpr int NUM_THREADS = 8;
    constexpr int CYCLES_PER_THREAD = 500;

    std::vector<std::unique_ptr<BinarySemaphore>> semaphores;
    std::vector<std::thread> workers;
    std::atomic<int> successful_operations{0};
    std::atomic<bool> error_occurred{false};

    // Create semaphores for each worker
    for (int i = 0; i < NUM_THREADS; ++i) {
        semaphores.push_back(std::make_unique<BinarySemaphore>());
    }

    // Start worker threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([&, i]() {
            auto& sem = *semaphores[i];
            std::random_device rd;
            std::mt19937 gen(rd() + i);
            std::uniform_int_distribution<> delay_dist(1, 10);

            for (int cycle = 0; cycle < CYCLES_PER_THREAD && !error_occurred.load(); ++cycle) {
                // Random delay before signaling
                std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen)));

                // Signal from "ISR" context
                if (gen() % 10 == 0) {
                    sem.GiveFromISRWithStatus(UARTStatus::ErrError);
                } else {
                    sem.GiveFromISR();
                }

                // Wait for signal
                auto status = sem.TakeWithTimeout(std::chrono::milliseconds(100));

                if (status == UARTStatus::ErrTimeout) {
                    error_occurred.store(true);
                    break;
                } else if (status == UARTStatus::Ok || status == UARTStatus::ErrError) {
                    successful_operations.fetch_add(1);
                } else {
                    error_occurred.store(true);
                    break;
                }
            }
        });
    }

    // Wait for all workers
    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_FALSE(error_occurred.load());
    EXPECT_EQ(successful_operations.load(), NUM_THREADS * CYCLES_PER_THREAD);
}

/// Stress test for watchdog timeout accuracy under load
TEST(SyncStressTest, WatchdogTimeoutAccuracyUnderLoad) {
    constexpr int NUM_WATCHDOGS = 16;
    constexpr int TIMEOUT_MS = 50;
    constexpr int TOLERANCE_MS = 20;

    std::vector<std::unique_ptr<Watchdog>> watchdogs;
    std::vector<std::future<void>> futures;
    std::atomic<int> timeout_count{0};
    std::atomic<int> completion_count{0};

    // Create watchdogs
    for (int i = 0; i < NUM_WATCHDOGS; ++i) {
        watchdogs.push_back(std::make_unique<Watchdog>());
    }

    // Start watchdog operations concurrently
    for (int i = 0; i < NUM_WATCHDOGS; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            auto& wd = *watchdogs[i];
            std::random_device rd;
            std::mt19937 gen(rd() + i);
            std::uniform_int_distribution<> completion_delay(0, TIMEOUT_MS * 2);

            auto start_time = std::chrono::steady_clock::now();
            wd.Start(std::chrono::milliseconds(TIMEOUT_MS));

            // Randomly complete or let timeout
            int delay = completion_delay(gen);

            if (delay < TIMEOUT_MS) {
                // Complete before timeout
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                wd.SignalCompletionFromISR(UARTStatus::Ok);
            } else {
                // Let timeout occur
                std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS + 10));
                wd.CheckTimeoutFromISR();
            }

            auto status = wd.AwaitCompletion();
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

            if (status == UARTStatus::ErrTimeout) {
                timeout_count.fetch_add(1);
                // Verify timeout timing
                EXPECT_GE(elapsed_ms.count(), TIMEOUT_MS - 5); // Small tolerance for early
                EXPECT_LE(elapsed_ms.count(), TIMEOUT_MS + TOLERANCE_MS);
            } else if (status == UARTStatus::Ok) {
                completion_count.fetch_add(1);
                // Should complete faster than timeout
                EXPECT_LT(elapsed_ms.count(), TIMEOUT_MS + 10);
            } else {
                FAIL() << "Unexpected status: " << static_cast<int>(status);
            }
        }));
    }

    // Wait for all operations
    for (auto& future : futures) {
        future.wait();
    }

    // All watchdogs should have completed (either timeout or normal)
    EXPECT_EQ(timeout_count.load() + completion_count.load(), NUM_WATCHDOGS);
    EXPECT_GT(timeout_count.load(), 0);    // Some should timeout
    EXPECT_GT(completion_count.load(), 0); // Some should complete normally
}

/// Stress test for memory ordering under extreme contention
TEST(SyncStressTest, MemoryOrderingExtremeContention) {
    constexpr int NUM_PRODUCERS = 8;
    constexpr int NUM_CONSUMERS = 4;
    constexpr int DATA_SIZE = 1000;

    CircularBuffer<std::uint8_t, 128> buffer;
    std::atomic<std::uint64_t> checksum_produced{0};
    std::atomic<std::uint64_t> checksum_consumed{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start producers that write sequential data with checksums
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            std::uint64_t local_checksum = 0;

            for (int i = 0; i < DATA_SIZE / NUM_PRODUCERS; ++i) {
                std::uint8_t value = static_cast<std::uint8_t>((p * 256 + i) & 0xFF);
                local_checksum += value;

                // OVERWRITE semantics - always succeeds
                buffer.WriteFromISR(value);
            }

            checksum_produced.fetch_add(local_checksum);
        });
    }

    // Start consumers that verify data integrity
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&]() {
            std::uint64_t local_checksum = 0;

            while (!producers_done.load() || !buffer.IsEmpty()) {
                auto data = buffer.ReadAvailable();

                if (!data.empty()) {
                    for (auto byte : data) {
                        local_checksum += byte;
                    }
                    buffer.MarkConsumed(data.size());
                }
                // No yield - maximum pressure
            }

            checksum_consumed.fetch_add(local_checksum);
        });
    }

    // Wait for producers
    for (auto& producer : producers) {
        producer.join();
    }
    producers_done.store(true);

    // Wait for consumers
    for (auto& consumer : consumers) {
        consumer.join();
    }

    // With OVERWRITE semantics, consumed checksum may be <= produced (some data overwritten)
    EXPECT_LE(checksum_consumed.load(), checksum_produced.load());
    EXPECT_TRUE(buffer.IsEmpty());
}

/// Combined stress test using all primitives together
TEST(SyncStressTest, CombinedPrimitivesStressTest) {
    constexpr int NUM_OPERATIONS = 200;

    BinarySemaphore completion_sem;
    Watchdog operation_timer;
    CircularBuffer<std::uint16_t, 32> data_buffer;

    std::atomic<int> operations_completed{0};
    std::atomic<bool> test_failed{false};

    // Producer thread using all primitives
    std::thread producer([&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay_dist(1, 20);

        for (int i = 0; i < NUM_OPERATIONS && !test_failed.load(); ++i) {
            // Start watchdog for this operation
            operation_timer.Start(std::chrono::milliseconds(100));

            // Generate test data
            std::uint16_t data = static_cast<std::uint16_t>(i);

            // Write to buffer (OVERWRITE semantics - always succeeds)
            data_buffer.WriteFromISR(data);

            // Signal completion
            if (operation_timer.IsStarted()) {
                operation_timer.SignalCompletionFromISR(UARTStatus::Ok);
            }

            // Random delay
            std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen)));
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < NUM_OPERATIONS && !test_failed.load(); ++i) {
            // Wait for operation to complete (either success or timeout)
            auto status = operation_timer.AwaitCompletion();

            if (status == UARTStatus::Ok) {
                // Read data from buffer
                auto data = data_buffer.ReadAvailable();
                if (!data.empty()) {
                    // Verify data integrity
                    for (auto value : data) {
                        EXPECT_LE(value, NUM_OPERATIONS);
                    }
                    data_buffer.MarkConsumed(data.size());
                    operations_completed.fetch_add(data.size());
                } else {
                    test_failed.store(true);
                    break;
                }
            } else if (status == UARTStatus::ErrTimeout) {
                // Timeout is acceptable under high load
                continue;
            } else {
                test_failed.store(true);
                break;
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_FALSE(test_failed.load());
    EXPECT_GT(operations_completed.load(), NUM_OPERATIONS / 2); // At least half should succeed
}