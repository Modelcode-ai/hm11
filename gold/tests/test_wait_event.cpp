/**
 * @file test_wait_event.cpp
 * @brief Comprehensive unit tests for WaitEvent platform abstraction
 *
 * These tests validate the WaitEvent implementation on both ARM Cortex-M
 * (WFE/SEV) and Linux x86 (futex) platforms. The tests focus on:
 *
 * - Basic signal/wait functionality
 * - Signal-before-wait behavior (no blocking)
 * - Multiple wait/signal cycles
 * - Concurrent signaling from multiple threads
 * - ISR simulation using std::thread
 * - Thread safety validation with ThreadSanitizer
 *
 * ## Running with ThreadSanitizer
 *
 * To validate thread safety and detect data races:
 *
 * ```bash
 * cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
 * cmake --build . --target hm11_tests
 * ./hm11_tests --gtest_filter=WaitEvent*
 * ```
 *
 * ThreadSanitizer will report any data races or memory ordering violations.
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "hm11/platform/wait_event.hpp"

using namespace hm11::platform;
using namespace std::chrono_literals;

// =============================================================================
// Basic Functionality Tests
// =============================================================================

/**
 * @brief Test basic signal/wait operation
 *
 * Verifies that:
 * - Thread blocks in wait() until signal() is called
 * - wait() returns after signal() is called
 * - Event can be cleared and reused
 */
TEST(WaitEvent, BasicSignalWait) {
    WaitEvent event;
    std::atomic<bool> thread_started{false};
    std::atomic<bool> thread_finished{false};

    // Start thread that waits for event
    std::thread waiter([&]() {
        thread_started.store(true, std::memory_order_release);
        // Loop until signaled (wait() may return spuriously)
        while (!event.is_signaled()) {
            event.wait();
        }
        thread_finished.store(true, std::memory_order_release);
    });

    // Wait for thread to start
    while (!thread_started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // Give thread time to enter wait state
    std::this_thread::sleep_for(10ms);

    // Thread should still be waiting
    EXPECT_FALSE(thread_finished.load(std::memory_order_acquire));

    // Signal the event
    event.signal();

    // Wait for thread to finish
    waiter.join();

    // Thread should have finished after signal
    EXPECT_TRUE(thread_finished.load(std::memory_order_acquire));
}

/**
 * @brief Test signal-before-wait behavior
 *
 * Verifies that if signal() is called before wait(), the subsequent wait()
 * returns immediately without blocking. This prevents lost wakeups.
 */
TEST(WaitEvent, SignalBeforeWait) {
    WaitEvent event;

    // Signal before wait
    event.signal();

    // This should return immediately without blocking
    auto start = std::chrono::steady_clock::now();
    // Loop until signaled (wait() may return spuriously, but event is already signaled)
    while (!event.is_signaled()) {
        event.wait();
    }
    auto end = std::chrono::steady_clock::now();

    // Wait should complete very quickly (no blocking since already signaled)
    // Allow up to 15ms for spurious wakeup + loop overhead
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 15);
}

/**
 * @brief Test multiple wait/signal cycles
 *
 * Verifies that the event can be cleared and reused for multiple wait/signal
 * cycles without state corruption.
 */
TEST(WaitEvent, MultipleWaitSignalCycles) {
    WaitEvent event;
    constexpr int NUM_CYCLES = 10;
    std::atomic<int> cycle_count{0};
    std::atomic<bool> cleared{false};

    std::thread waiter([&]() {
        for (int i = 0; i < NUM_CYCLES; ++i) {
            // Loop until signaled (wait() may return spuriously)
            while (!event.is_signaled()) {
                event.wait();
            }
            cycle_count.fetch_add(1, std::memory_order_release);
            event.clear();
            cleared.store(true, std::memory_order_release);
        }
    });

    // Give thread time to start
    std::this_thread::sleep_for(10ms);

    // Signal multiple times
    for (int i = 0; i < NUM_CYCLES; ++i) {
        cleared.store(false, std::memory_order_release);
        event.signal();

        // Wait for thread to process this cycle and clear
        while (!cleared.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    waiter.join();

    EXPECT_EQ(cycle_count.load(std::memory_order_acquire), NUM_CYCLES);
}

/**
 * @brief Test clear operation
 *
 * Verifies that clear() resets the event to non-signaled state.
 */
TEST(WaitEvent, ClearOperation) {
    WaitEvent event;

    // Signal the event
    event.signal();
    EXPECT_TRUE(event.is_signaled());

    // Clear the event
    event.clear();
    EXPECT_FALSE(event.is_signaled());
}

/**
 * @brief Test is_signaled query
 *
 * Verifies that is_signaled() correctly reports the event state.
 */
TEST(WaitEvent, IsSignaledQuery) {
    WaitEvent event;

    // Initially not signaled
    EXPECT_FALSE(event.is_signaled());

    // After signal
    event.signal();
    EXPECT_TRUE(event.is_signaled());

    // After clear
    event.clear();
    EXPECT_FALSE(event.is_signaled());
}

// =============================================================================
// Concurrency Tests
// =============================================================================

/**
 * @brief Test concurrent signaling from multiple threads
 *
 * Verifies that signal() is thread-safe and can be called from multiple
 * threads concurrently without race conditions.
 */
TEST(WaitEvent, ConcurrentSignaling) {
    WaitEvent event;
    constexpr int NUM_SIGNALERS = 5;
    constexpr int SIGNALS_PER_THREAD = 100;
    std::atomic<int> signal_count{0};

    // Create multiple signaler threads
    std::vector<std::thread> signalers;
    signalers.reserve(static_cast<size_t>(NUM_SIGNALERS));

    for (int i = 0; i < NUM_SIGNALERS; ++i) {
        signalers.emplace_back([&]() {
            for (int j = 0; j < SIGNALS_PER_THREAD; ++j) {
                event.signal();
                signal_count.fetch_add(1, std::memory_order_release);
                event.clear();
                std::this_thread::yield();
            }
        });
    }

    // Wait for all signalers to finish
    for (auto& t : signalers) {
        t.join();
    }

    // All signals should have been sent
    EXPECT_EQ(signal_count.load(std::memory_order_acquire), NUM_SIGNALERS * SIGNALS_PER_THREAD);
}

/**
 * @brief Test multiple waiters on same event
 *
 * Verifies that multiple threads can wait on the same event. Note that
 * WaitEvent uses "wake one" semantics, so only one waiter will be woken
 * per signal() call.
 */
TEST(WaitEvent, MultipleWaiters) {
    WaitEvent event;
    constexpr int NUM_WAITERS = 3;
    std::atomic<int> wakeup_count{0};

    // Create multiple waiter threads
    std::vector<std::thread> waiters;
    waiters.reserve(static_cast<size_t>(NUM_WAITERS));

    for (int i = 0; i < NUM_WAITERS; ++i) {
        waiters.emplace_back([&]() {
            // Loop until signaled (wait() may return spuriously)
            while (!event.is_signaled()) {
                event.wait();
            }
            wakeup_count.fetch_add(1, std::memory_order_release);
        });
    }

    // Give threads time to enter wait state
    std::this_thread::sleep_for(10ms);

    // Signal multiple times to wake all waiters
    // (WaitEvent uses wake-one semantics, so we need multiple signals)
    for (int i = 0; i < NUM_WAITERS; ++i) {
        event.signal();
        std::this_thread::sleep_for(5ms);
    }

    // Wait for all waiters to finish
    for (auto& t : waiters) {
        t.join();
    }

    // At least one waiter should have been woken
    // (exact count depends on timing and platform behavior)
    EXPECT_GE(wakeup_count.load(std::memory_order_acquire), 1);
}

// =============================================================================
// ISR Simulation Tests
// =============================================================================

/**
 * @brief Test ISR-to-thread signaling pattern
 *
 * Simulates an ISR (using std::thread) signaling a worker thread. This is
 * the primary use case for WaitEvent in the HM11 driver.
 */
TEST(WaitEvent, ISRToThreadSignaling) {
    WaitEvent event;
    std::atomic<int> data_ready_count{0};
    std::atomic<bool> stop{false};

    // Worker thread (simulates main thread waiting for ISR events)
    std::thread worker([&]() {
        while (true) {
            // Loop until signaled (wait() may return spuriously)
            while (!event.is_signaled()) {
                event.wait();
            }

            // Check if stop requested (before incrementing counter)
            if (stop.load(std::memory_order_acquire)) {
                break;
            }

            // Process "data" from ISR
            data_ready_count.fetch_add(1, std::memory_order_release);

            event.clear();
        }
    });

    // Give worker time to start
    std::this_thread::sleep_for(10ms);

    // ISR thread (simulates interrupt handler signaling data ready)
    constexpr int NUM_INTERRUPTS = 20;
    for (int i = 0; i < NUM_INTERRUPTS; ++i) {
        // Simulate interrupt arrival
        std::this_thread::sleep_for(5ms);

        // ISR signals event (data ready)
        event.signal();

        // Wait for worker to process
        while (data_ready_count.load(std::memory_order_acquire) <= i) {
            std::this_thread::yield();
        }
    }

    // Stop worker
    stop.store(true, std::memory_order_release);
    event.signal();
    worker.join();

    EXPECT_EQ(data_ready_count.load(std::memory_order_acquire), NUM_INTERRUPTS);
}

/**
 * @brief Test rapid ISR signaling (stress test)
 *
 * Simulates rapid ISR events to test the implementation under high load.
 * This validates that no signals are lost and the implementation is robust.
 */
TEST(WaitEvent, RapidISRSignaling) {
    WaitEvent event;
    std::atomic<int> event_count{0};
    std::atomic<bool> worker_ready{false};
    constexpr int TARGET_EVENTS = 1000;

    std::cerr << "[RapidISRSignaling] Starting test with " << TARGET_EVENTS << " events\n" << std::flush;

    // Worker thread
    std::thread worker([&]() {
        std::cerr << "[Worker] Thread started\n" << std::flush;
        for (int i = 0; i < TARGET_EVENTS; ++i) {
            if (i % 100 == 0) {
                std::cerr << "[Worker] Processing event " << i << "/" << TARGET_EVENTS << "\n" << std::flush;
            }

            // Signal ready for next event
            worker_ready.store(true, std::memory_order_release);

            // Wait for signal (handles spurious wakeups by looping)
            while (!event.is_signaled()) {
                event.wait();
            }

            // Process event
            const int COUNT = event_count.fetch_add(1, std::memory_order_release) + 1;
            event.clear();

            if (i % 100 == 0) {
                std::cerr << "[Worker] Processed event " << i << ", count=" << COUNT << "\n" << std::flush;
            }
        }
        std::cerr << "[Worker] Thread finishing, final count=" << event_count.load() << "\n" << std::flush;
    });

    // Rapid ISR signaling
    std::cerr << "[Main] Starting signaling loop\n" << std::flush;
    for (int i = 0; i < TARGET_EVENTS; ++i) {
        if (i % 100 == 0) {
            std::cerr << "[Main] Signaling event " << i << "/" << TARGET_EVENTS << "\n" << std::flush;
        }

        // Wait for worker to be ready for this event
        int spin_count = 0;
        while (!worker_ready.load(std::memory_order_acquire)) {
            std::this_thread::yield();
            ++spin_count;
            if (spin_count > 10000000) {
                std::cerr << "[Main] ERROR: Stuck waiting for worker_ready at i=" << i
                          << ", event_count=" << event_count.load() << ", worker_ready=" << worker_ready.load()
                          << ", is_signaled=" << event.is_signaled() << "\n"
                          << std::flush;
                FAIL() << "Deadlock detected in test";
            }
        }
        worker_ready.store(false, std::memory_order_release);

        // Signal event
        event.signal();
    }

    std::cerr << "[Main] Waiting for worker thread to finish\n" << std::flush;
    worker.join();
    std::cerr << "[Main] Worker joined\n" << std::flush;

    EXPECT_EQ(event_count.load(std::memory_order_acquire), TARGET_EVENTS);
    std::cerr << "[RapidISRSignaling] Test complete\n" << std::flush;
}

// =============================================================================
// Memory Ordering Tests
// =============================================================================

/**
 * @brief Test memory ordering guarantees (happens-before relationship)
 *
 * Verifies that signal() provides memory_order_release semantics and wait()
 * provides memory_order_acquire semantics, establishing a happens-before
 * relationship for shared data.
 */
TEST(WaitEvent, MemoryOrderingGuarantees) {
    WaitEvent event;
    std::atomic<int> shared_data{0};
    constexpr int EXPECTED_VALUE = 42;

    std::thread producer([&]() {
        // Write to shared data
        shared_data.store(EXPECTED_VALUE, std::memory_order_relaxed);

        // Signal event (memory_order_release)
        // All prior memory operations should be visible after wait() returns
        event.signal();
    });

    // Give producer time to run
    std::this_thread::sleep_for(10ms);

    // Wait for event (memory_order_acquire)
    // Loop until signaled (wait() may return spuriously)
    while (!event.is_signaled()) {
        event.wait();
    }

    // Read shared data - should see producer's write
    int value = shared_data.load(std::memory_order_relaxed);

    producer.join();

    EXPECT_EQ(value, EXPECTED_VALUE);
}

/**
 * @brief Test multiple data transfers with memory ordering
 *
 * Validates that multiple signal/wait cycles maintain proper memory ordering
 * for shared data transfers.
 */
TEST(WaitEvent, MultipleDataTransfersWithOrdering) {
    WaitEvent event;
    std::atomic<int> transfer_count{0};
    constexpr int NUM_TRANSFERS = 50;
    std::vector<int> sent_values;
    std::vector<int> received_values;

    // Sender thread
    std::thread sender([&]() {
        for (int i = 0; i < NUM_TRANSFERS; ++i) {
            // Write data
            sent_values.push_back(i);

            // Signal data ready (memory_order_release)
            event.signal();

            // Wait for receiver to process
            while (transfer_count.load(std::memory_order_acquire) <= i) {
                std::this_thread::yield();
            }
        }
    });

    // Receiver thread
    std::thread receiver([&]() {
        for (int i = 0; i < NUM_TRANSFERS; ++i) {
            // Wait for data (memory_order_acquire)
            // Loop until signaled (wait() may return spuriously)
            while (!event.is_signaled()) {
                event.wait();
            }

            // Read data - should see sender's write
            received_values.push_back(sent_values[static_cast<size_t>(i)]);

            transfer_count.fetch_add(1, std::memory_order_release);
            event.clear();
        }
    });

    sender.join();
    receiver.join();

    // Verify all data was transferred correctly
    ASSERT_EQ(received_values.size(), static_cast<size_t>(NUM_TRANSFERS));
    for (int i = 0; i < NUM_TRANSFERS; ++i) {
        EXPECT_EQ(received_values[static_cast<size_t>(i)], i);
    }
}

// =============================================================================
// Edge Cases and Robustness Tests
// =============================================================================

/**
 * @brief Test signal without wait (no crash)
 *
 * Verifies that calling signal() without a corresponding wait() doesn't
 * cause crashes or undefined behavior.
 */
TEST(WaitEvent, SignalWithoutWait) {
    WaitEvent event;

    // Signal multiple times without waiting
    event.signal();
    event.signal();
    event.signal();

    // Should be signaled
    EXPECT_TRUE(event.is_signaled());

    // Clear and verify
    event.clear();
    EXPECT_FALSE(event.is_signaled());
}

/**
 * @brief Test rapid clear/signal cycles
 *
 * Verifies robustness under rapid state changes.
 */
TEST(WaitEvent, RapidClearSignalCycles) {
    WaitEvent event;

    for (int i = 0; i < 100; ++i) {
        event.signal();
        EXPECT_TRUE(event.is_signaled());
        event.clear();
        EXPECT_FALSE(event.is_signaled());
    }
}

/**
 * @brief Test construction and destruction
 *
 * Verifies that WaitEvent can be constructed and destroyed safely,
 * even if signaled.
 */
TEST(WaitEvent, ConstructionDestruction) {
    // Create and destroy non-signaled event
    {
        WaitEvent event1;
        EXPECT_FALSE(event1.is_signaled());
    }

    // Create, signal, and destroy event
    {
        WaitEvent event2;
        event2.signal();
        EXPECT_TRUE(event2.is_signaled());
    }

    // Should not crash or leak resources
    SUCCEED();
}
