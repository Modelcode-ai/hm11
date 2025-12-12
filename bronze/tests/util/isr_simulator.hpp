/**
 * @file isr_simulator.hpp
 * @brief Utilities for simulating ISR behavior in tests
 *
 * This file provides utilities for simulating interrupt service routines (ISRs)
 * in unit and integration tests. It provides mechanisms to run code with similar
 * constraints and behaviors to real ISRs, allowing thorough testing of ISR-to-thread
 * synchronization mechanisms without requiring actual interrupt hardware.
 */

#ifndef HM11_TESTS_UTIL_ISR_SIMULATOR_HPP
#define HM11_TESTS_UTIL_ISR_SIMULATOR_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace hm11::test {

/**
 * @brief Class for simulating ISR behavior in tests
 *
 * This class provides facilities to simulate ISRs in test environments:
 * - Runs ISR callbacks in separate threads
 * - Provides synchronization to control when ISRs run
 * - Enforces restrictions similar to real ISRs (no blocking, etc.)
 * - Allows controlled testing of ISR-to-thread interactions
 *
 * Usage:
 * 1. Create an ISRSimulator
 * 2. Register ISR callbacks using registerISR()
 * 3. Control ISR execution with triggerISR() or triggerAllISRs()
 * 4. Wait for ISRs to complete with waitForISRs()
 */
class ISRSimulator {
public:
    using ISRCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;

    /**
     * @brief Constructor
     */
    ISRSimulator() : running_(false), isrs_triggered_(0), isrs_completed_(0) {}

    /**
     * @brief Destructor
     * 
     * Ensures all ISR threads are properly joined
     */
    ~ISRSimulator() {
        stop();
    }

    /**
     * @brief Register an ISR callback
     *
     * @param isr_callback The ISR function to register
     * @return int The ISR ID (used to trigger specific ISRs)
     */
    int registerISR(ISRCallback isr_callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        int isr_id = static_cast<int>(isr_threads_.size());
        isr_callbacks_.push_back(std::move(isr_callback));
        isr_trigger_flags_.push_back(false);
        isr_threads_.emplace_back(); // Placeholder for the thread
        return isr_id;
    }

    /**
     * @brief Start the ISR simulator
     *
     * This method starts threads for all registered ISRs
     */
    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }

        running_ = true;
        isrs_triggered_ = 0;
        isrs_completed_ = 0;

        // Start a thread for each ISR
        for (size_t i = 0; i < isr_callbacks_.size(); ++i) {
            isr_trigger_flags_[i] = false;
            isr_threads_[i] = std::thread([this, i]() {
                // ISR thread waits for trigger
                while (running_) {
                    // Wait for ISR to be triggered
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this, i]() {
                            return !running_ || isr_trigger_flags_[i];
                        });

                        // Exit if simulator is stopping
                        if (!running_) {
                            return;
                        }

                        // Clear the trigger flag
                        isr_trigger_flags_[i] = false;
                    }

                    // Execute the ISR callback outside the lock
                    isr_callbacks_[i]();
                    
                    // Increment completed count
                    isrs_completed_.fetch_add(1, std::memory_order_release);
                    cv_.notify_all();
                }
            });
        }
    }

    /**
     * @brief Stop the ISR simulator
     *
     * This method stops all ISR threads and joins them
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }
            running_ = false;
        }

        // Wake all threads so they can exit
        cv_.notify_all();

        // Join all ISR threads
        for (auto& thread : isr_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        // Clear state
        std::lock_guard<std::mutex> lock(mutex_);
        isr_callbacks_.clear();
        isr_trigger_flags_.clear();
        isr_threads_.clear();
    }

    /**
     * @brief Trigger a specific ISR
     *
     * @param isr_id The ID of the ISR to trigger
     * @return bool True if ISR was triggered, false otherwise
     */
    bool triggerISR(int isr_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || isr_id < 0 || isr_id >= static_cast<int>(isr_callbacks_.size())) {
            return false;
        }

        isr_trigger_flags_[static_cast<std::size_t>(isr_id)] = true;
        isrs_triggered_.fetch_add(1, std::memory_order_release);
        cv_.notify_all();
        return true;
    }

    /**
     * @brief Trigger all registered ISRs
     * 
     * @return int Number of ISRs triggered
     */
    int triggerAllISRs() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return 0;
        }

        int count = 0;
        for (size_t i = 0; i < isr_trigger_flags_.size(); ++i) {
            isr_trigger_flags_[i] = true;
            ++count;
        }

        isrs_triggered_.fetch_add(count, std::memory_order_release);
        cv_.notify_all();
        return count;
    }

    /**
     * @brief Wait for all triggered ISRs to complete
     *
     * @param timeout Maximum time to wait
     * @return bool True if all ISRs completed, false if timeout occurred
     */
    template <typename Rep, typename Period>
    bool waitForISRs(std::chrono::duration<Rep, Period> timeout) {
        auto deadline = Clock::now() + timeout;
        std::unique_lock<std::mutex> lock(mutex_);

        return cv_.wait_until(lock, deadline, [this]() {
            return isrs_completed_.load(std::memory_order_acquire) >= 
                   isrs_triggered_.load(std::memory_order_acquire);
        });
    }

    /**
     * @brief Wait for all triggered ISRs to complete (with infinite timeout)
     * 
     * @return bool Always returns true
     */
    bool waitForISRs() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return isrs_completed_.load(std::memory_order_acquire) >= 
                   isrs_triggered_.load(std::memory_order_acquire);
        });
        return true;
    }

    /**
     * @brief Get the number of triggered ISRs
     * 
     * @return int Number of ISRs triggered
     */
    int getTriggeredCount() const {
        return isrs_triggered_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get the number of completed ISRs
     * 
     * @return int Number of ISRs completed
     */
    int getCompletedCount() const {
        return isrs_completed_.load(std::memory_order_acquire);
    }

private:
    std::vector<ISRCallback> isr_callbacks_;
    std::vector<bool> isr_trigger_flags_;
    std::vector<std::thread> isr_threads_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_;
    std::atomic<int> isrs_triggered_;
    std::atomic<int> isrs_completed_;
};

/**
 * @brief Helper class for simulating an ISR with deterministic timing
 * 
 * This class provides a simulated environment for testing ISR behaviors
 * with controlled timing. It allows running ISR code at specified intervals
 * and checking the effects on the system under test.
 */
class TimedISRSimulator {
public:
    using ISRCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    
    /**
     * @brief Constructor
     * 
     * @param isr_callback The ISR function to execute
     * @param interval_ms Interval between ISR executions in milliseconds
     */
    TimedISRSimulator(ISRCallback isr_callback, 
                      std::chrono::milliseconds interval_ms = std::chrono::milliseconds(10))
        : isr_callback_(std::move(isr_callback))
        , interval_ms_(interval_ms)
        , running_(false)
        , executions_(0) 
    {}
    
    /**
     * @brief Destructor
     * 
     * Ensures the simulator thread is properly stopped and joined
     */
    ~TimedISRSimulator() {
        stop();
    }
    
    /**
     * @brief Start the simulator
     * 
     * Starts running the ISR at the specified interval
     */
    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }
        
        running_ = true;
        executions_ = 0;
        
        // Start ISR thread
        isr_thread_ = std::thread([this]() {
            auto next_time = Clock::now() + interval_ms_;
            while (running_) {
                // Wait until next execution time or stop signal
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait_until(lock, next_time, [this]() {
                        return !running_;
                    });
                    
                    // Exit if stopped
                    if (!running_) {
                        return;
                    }
                }
                
                // Execute ISR callback
                isr_callback_();
                executions_.fetch_add(1, std::memory_order_release);
                
                // Calculate next execution time
                next_time = Clock::now() + interval_ms_;
            }
        });
    }
    
    /**
     * @brief Stop the simulator
     * 
     * Stops the simulator and waits for the ISR thread to join
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }
            running_ = false;
        }
        
        // Wake up ISR thread so it can exit
        cv_.notify_all();
        
        if (isr_thread_.joinable()) {
            isr_thread_.join();
        }
    }
    
    /**
     * @brief Get the number of ISR executions
     * 
     * @return int Number of times the ISR callback was executed
     */
    int getExecutions() const {
        return executions_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set the interval between ISR executions
     * 
     * @param interval_ms New interval in milliseconds
     */
    void setInterval(std::chrono::milliseconds interval_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        interval_ms_ = interval_ms;
    }
    
private:
    ISRCallback isr_callback_;
    std::chrono::milliseconds interval_ms_;
    std::thread isr_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_;
    std::atomic<int> executions_;
};

} // namespace hm11::test

#endif // HM11_TESTS_UTIL_ISR_SIMULATOR_HPP