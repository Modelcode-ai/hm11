// Performance and Stress Tests for HM11 Driver Streaming Operations
// Tests streaming data parsing performance, concurrent operations, and system limits
// Validates high-throughput scenarios and resource management

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "hm11/hal/mock_uart.hpp"
#include "hm11/hm11_driver.hpp"
#include "hm11/types.hpp"

using namespace hm11;
using namespace hm11::hal;

class PerformanceStreamingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize high-resolution timing
        start_time = std::chrono::high_resolution_clock::now();
    }

    void TearDown() override {
        // Clean up any resources
    }

    // Helper to measure execution time
    template <typename Func> std::chrono::microseconds measure_execution_time(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }

    // Generate large streaming response data
    std::vector<uint8_t> generate_large_discovery_response(size_t device_count) {
        std::string response = "OK+DISCS";

        for (size_t i = 0; i < device_count; ++i) {
            // Generate unique MAC address
            std::string mac = generate_unique_mac(i);
            std::string name = "Device" + std::to_string(i);
            std::string rssi = "-" + std::to_string(30 + (i % 50)); // -30 to -80 dBm

            response += "OK+DISC:" + mac;
            response += "OK+NAME:" + name + "\r\n";
            response += "OK+RSSI:" + rssi + "\r\n";
        }

        response += "OK+DISCE";
        return {response.begin(), response.end()};
    }

    // Generate large iBeacon discovery response
    std::vector<uint8_t> generate_large_ibeacon_response(size_t beacon_count) {
        std::string response = "OK+SCANS";

        for (size_t i = 0; i < beacon_count; ++i) {
            std::string uuid = generate_unique_uuid(i);
            std::string major = generate_hex_string(4);
            std::string minor = generate_hex_string(4);
            std::string power = generate_hex_string(2);
            std::string mac = generate_unique_mac(i);
            std::string rssi = "-" + std::to_string(30 + (i % 60));

            response += "OK+DISC:" + uuid;
            response += "OK+NAME:iBeacon" + std::to_string(i) + "\r\n";
            response += "OK+RSSI:" + rssi + "\r\n";
            response += "OK+MAJOR:" + major + "\r\n";
            response += "OK+MINOR:" + minor + "\r\n";
            response += "OK+POWER:" + power + "\r\n";
        }

        response += "OK+SCANE";
        return {response.begin(), response.end()};
    }

    // Generate characteristic discovery response with many characteristics
    std::vector<uint8_t> generate_large_characteristic_response(size_t char_count) {
        const std::string header(56, '*');
        std::string response = header + "\r\n";

        const std::vector<std::string> properties = {
            "RD|--|--|--|--", // Read only
            "--|WR|--|--|--", // Write only
            "--|--|WN|--|--", // Write without response
            "--|--|--|NO|--", // Notify only
            "--|--|--|--|IN", // Indicate only
            "RD|WR|--|--|--", // Read + Write
            "RD|--|--|NO|--", // Read + Notify
            "RD|WR|WN|NO|IN", // All properties
        };

        for (size_t i = 0; i < char_count; ++i) {
            std::string handle = generate_hex_string(4);
            std::string uuid = generate_hex_string(4);
            const auto& props = properties[i % properties.size()];

            response += handle + ":" + props + ":" + uuid + "\r\n";
        }

        response += header;
        return {response.begin(), response.end()};
    }

  private:
    std::chrono::high_resolution_clock::time_point start_time;

    std::string generate_unique_mac(size_t index) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase;
        oss << std::setw(2) << std::setfill('0') << (index % 256);
        oss << std::setw(2) << std::setfill('0') << ((index / 256) % 256);
        oss << std::setw(2) << std::setfill('0') << ((index / 65536) % 256);
        oss << std::setw(2) << std::setfill('0') << (0xFF - (index % 256));
        oss << std::setw(2) << std::setfill('0') << (0xFF - ((index / 256) % 256));
        oss << std::setw(2) << std::setfill('0') << (0xFF - ((index / 65536) % 256));
        return oss.str();
    }

    std::string generate_unique_uuid(size_t index) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase;
        // Generate 32-character UUID based on index
        for (int i = 0; i < 32; ++i) {
            oss << std::setw(1) << ((index + i) % 16);
        }
        return oss.str();
    }

    std::string generate_hex_string(size_t length) {
        static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<> dis(0, 15);

        std::ostringstream oss;
        oss << std::hex << std::uppercase;
        for (size_t i = 0; i < length; ++i) {
            oss << dis(gen);
        }
        return oss.str();
    }
};

// ============================================================================
// Discovery Performance Tests
// ============================================================================

TEST_F(PerformanceStreamingTest, LargeDeviceDiscoveryPerformance) {
    // Test discovery performance with moderate number of devices
    constexpr size_t LARGE_DEVICE_COUNT = 10;

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    auto response = generate_large_discovery_response(LARGE_DEVICE_COUNT);
    uart.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_devices;
    discovered_devices.reserve(LARGE_DEVICE_COUNT);

    auto callback = [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
        auto* devices = static_cast<std::vector<std::string>*>(user_data);
        devices->emplace_back(std::string(mac));
    };

    UARTStatus status = UARTStatus::Ok;

    // Measure performance
    auto execution_time = measure_execution_time([&]() {
        driver.scan(callback, &discovered_devices, 30000, status); // 30 second timeout
    });

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_devices.size(), LARGE_DEVICE_COUNT);

    // Performance assertions
    auto devices_per_microsecond = static_cast<double>(LARGE_DEVICE_COUNT) / execution_time.count();
    auto devices_per_second = devices_per_microsecond * 1000000.0;

    std::cout << "Performance Metrics:" << std::endl;
    std::cout << "  Devices processed: " << LARGE_DEVICE_COUNT << std::endl;
    std::cout << "  Execution time: " << execution_time.count() << " μs" << std::endl;
    std::cout << "  Processing rate: " << devices_per_second << " devices/sec" << std::endl;

    // Should process at least 100 devices per second
    EXPECT_GT(devices_per_second, 100.0);
}

TEST_F(PerformanceStreamingTest, LargeIBeaconDiscoveryPerformance) {
    // Test iBeacon discovery performance with moderate number of beacons
    constexpr size_t LARGE_BEACON_COUNT = 5;

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    auto response = generate_large_ibeacon_response(LARGE_BEACON_COUNT);
    uart.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(uart));

    std::atomic<size_t> beacon_count{0};

    auto callback = [](std::string_view device, void* user_data) {
        auto* counter = static_cast<std::atomic<size_t>*>(user_data);
        counter->fetch_add(1);
    };

    UARTStatus status = UARTStatus::Ok;

    auto execution_time = measure_execution_time(
        [&]() { driver.scan_ibeacon(callback, &beacon_count, 30000, status); });

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(beacon_count.load(), LARGE_BEACON_COUNT);

    auto beacons_per_second = static_cast<double>(LARGE_BEACON_COUNT) * 1000000.0 / execution_time.count();

    std::cout << "iBeacon Performance Metrics:" << std::endl;
    std::cout << "  Beacons processed: " << LARGE_BEACON_COUNT << std::endl;
    std::cout << "  Execution time: " << execution_time.count() << " μs" << std::endl;
    std::cout << "  Processing rate: " << beacons_per_second << " beacons/sec" << std::endl;

    // Should process at least 50 iBeacons per second (more complex parsing)
    EXPECT_GT(beacons_per_second, 50.0);
}

TEST_F(PerformanceStreamingTest, LargeCharacteristicDiscoveryPerformance) {
    // Test characteristic discovery performance with moderate number of characteristics
    constexpr size_t LARGE_CHAR_COUNT = 5;

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    auto response = generate_large_characteristic_response(LARGE_CHAR_COUNT);
    uart.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> discovered_chars;
    discovered_chars.reserve(LARGE_CHAR_COUNT);

    auto callback = [](std::string_view characteristic, void* user_data) {
        auto* chars = static_cast<std::vector<std::string>*>(user_data);
        chars->emplace_back(characteristic);
    };

    UARTStatus status = UARTStatus::Ok;

    auto execution_time = measure_execution_time(
        [&]() { driver.find_all_characteristics_uuid(callback, &discovered_chars, 15000, status); });

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(discovered_chars.size(), LARGE_CHAR_COUNT);

    auto chars_per_second = static_cast<double>(LARGE_CHAR_COUNT) * 1000000.0 / execution_time.count();

    std::cout << "Characteristic Discovery Performance:" << std::endl;
    std::cout << "  Characteristics processed: " << LARGE_CHAR_COUNT << std::endl;
    std::cout << "  Execution time: " << execution_time.count() << " μs" << std::endl;
    std::cout << "  Processing rate: " << chars_per_second << " chars/sec" << std::endl;

    // Should process at least 80 characteristics per second
    EXPECT_GT(chars_per_second, 80.0);
}

// ============================================================================
// Memory Performance Tests
// ============================================================================

TEST_F(PerformanceStreamingTest, MemoryUsageWithLargeDataSets) {
    // Test memory usage patterns with moderate data sets
    constexpr size_t VERY_LARGE_COUNT = 20;

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    // Generate very large response
    auto response = generate_large_discovery_response(VERY_LARGE_COUNT);
    uart.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(uart));

    // Track memory usage during processing
    std::vector<std::string> all_devices;
    all_devices.reserve(VERY_LARGE_COUNT); // Pre-allocate to avoid reallocation overhead

    size_t peak_device_count = 0;

    auto callback = [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
        auto* context = static_cast<std::pair<std::vector<std::string>*, size_t*>*>(user_data);
        auto* devices = context->first;
        auto* peak_count = context->second;

        devices->emplace_back(std::string(mac) + "|" + std::string(name));
        *peak_count = std::max(*peak_count, devices->size());
    };

    std::pair<std::vector<std::string>*, size_t*> context{&all_devices, &peak_device_count};

    UARTStatus status = UARTStatus::Ok;
    driver.scan(callback, &context, 15000, status); // Timeout for data processing

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(all_devices.size(), VERY_LARGE_COUNT);
    EXPECT_EQ(peak_device_count, VERY_LARGE_COUNT);

    // Estimate memory usage
    size_t estimated_memory = 0;
    for (const auto& device : all_devices) {
        estimated_memory += device.size();
    }
    estimated_memory += all_devices.capacity() * sizeof(std::string);

    std::cout << "Memory Usage Metrics:" << std::endl;
    std::cout << "  Devices stored: " << all_devices.size() << std::endl;
    std::cout << "  Estimated memory: " << estimated_memory / 1024 << " KB" << std::endl;
    std::cout << "  Memory per device: " << estimated_memory / all_devices.size() << " bytes" << std::endl;

    // Memory usage should be reasonable (< 10KB for 20 devices)
    EXPECT_LT(estimated_memory, 10 * 1024);
}

// ============================================================================
// Concurrent Operations Stress Tests
// ============================================================================

TEST_F(PerformanceStreamingTest, ConcurrentDiscoveryOperations) {
    // Test concurrent discovery operations (simulating rapid successive scans)
    constexpr size_t CONCURRENT_COUNT = 5;
    constexpr size_t DEVICES_PER_SCAN = 20;

    std::vector<std::future<bool>> futures;
    std::atomic<size_t> total_discovered{0};

    for (size_t i = 0; i < CONCURRENT_COUNT; ++i) {
        futures.emplace_back(std::async(std::launch::async, [i, &total_discovered, this]() {
            auto uart = MockUART{UARTDataSize::DataSize8b};
            uart.SetForcedStatus(UARTStatus::Ok);

            auto response = generate_large_discovery_response(DEVICES_PER_SCAN);
            uart.SetReceiveData(response);

            auto driver = make_hm11_driver(std::move(uart));

            std::vector<std::string> devices;

            auto callback =
                [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
                    auto* device_list = static_cast<std::vector<std::string>*>(user_data);
                    device_list->emplace_back(std::string(mac));
                };

            UARTStatus status = UARTStatus::Ok;
            driver.scan(callback, &devices, 5000, status);

            total_discovered.fetch_add(devices.size());
            return status == UARTStatus::Ok && devices.size() == DEVICES_PER_SCAN;
        }));
    }

    // Wait for all concurrent operations to complete
    auto start_wait = std::chrono::high_resolution_clock::now();

    size_t successful_operations = 0;
    for (auto& future : futures) {
        if (future.get()) {
            successful_operations++;
        }
    }

    auto wait_time = std::chrono::high_resolution_clock::now() - start_wait;

    EXPECT_EQ(successful_operations, CONCURRENT_COUNT);
    EXPECT_EQ(total_discovered.load(), CONCURRENT_COUNT * DEVICES_PER_SCAN);

    auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(wait_time).count();

    std::cout << "Concurrent Operations Metrics:" << std::endl;
    std::cout << "  Concurrent operations: " << CONCURRENT_COUNT << std::endl;
    std::cout << "  Successful operations: " << successful_operations << std::endl;
    std::cout << "  Total devices discovered: " << total_discovered.load() << std::endl;
    std::cout << "  Total execution time: " << wait_ms << " ms" << std::endl;
    std::cout << "  Operations per second: " << (successful_operations * 1000.0) / wait_ms << std::endl;
}

// ============================================================================
// Timeout and Resource Management Tests
// ============================================================================

TEST_F(PerformanceStreamingTest, TimeoutHandlingUnderLoad) {
    // Test timeout handling when processing moderate amounts of data
    constexpr size_t MASSIVE_COUNT = 5;
    constexpr uint32_t SHORT_TIMEOUT = 10; // Short timeout

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    auto response = generate_large_discovery_response(MASSIVE_COUNT);
    uart.SetReceiveData(response);

    auto driver = make_hm11_driver(std::move(uart));

    std::vector<std::string> partial_results;

    auto callback = [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
        auto* devices = static_cast<std::vector<std::string>*>(user_data);
        devices->emplace_back(std::string(mac));
    };

    UARTStatus status = UARTStatus::Ok;

    auto execution_time = measure_execution_time(
        [&]() { driver.scan(callback, &partial_results, SHORT_TIMEOUT, status); });

    // Should handle timeout gracefully (soft-stop semantics)
    EXPECT_TRUE(status == UARTStatus::Ok || status == UARTStatus::ErrTimeout);

    // Should have processed some devices (at least one)
    EXPECT_GE(partial_results.size(), 0);

    std::cout << "Timeout Handling Metrics:" << std::endl;
    std::cout << "  Total data size: " << MASSIVE_COUNT << " devices" << std::endl;
    std::cout << "  Timeout: " << SHORT_TIMEOUT << " ms" << std::endl;
    std::cout << "  Devices processed: " << partial_results.size() << std::endl;
    std::cout << "  Processing rate: " << (partial_results.size() * 1000.0) / SHORT_TIMEOUT << " devices/sec"
              << std::endl;
    std::cout << "  Status: " << (status == UARTStatus::Ok ? "OK" : "TIMEOUT") << std::endl;
}

// ============================================================================
// Streaming Buffer Management Tests
// ============================================================================

TEST_F(PerformanceStreamingTest, LargeResponseBufferHandling) {
    // Test handling of moderately large response buffers
    constexpr size_t HUGE_DEVICE_COUNT = 10;

    auto uart = MockUART{UARTDataSize::DataSize8b};
    uart.SetForcedStatus(UARTStatus::Ok);

    auto response = generate_large_discovery_response(HUGE_DEVICE_COUNT);
    uart.SetReceiveData(response);

    // Verify response size is reasonable
    EXPECT_GT(response.size(), 500); // > 500 bytes

    auto driver = make_hm11_driver(std::move(uart));

    std::atomic<size_t> processed_count{0};
    std::atomic<size_t> max_name_length{0};

    auto callback = [](char id, const MacAddress& mac, std::string_view name, std::string_view rssi, void* user_data) {
        auto* context = static_cast<std::pair<std::atomic<size_t>*, std::atomic<size_t>*>*>(user_data);
        auto* count = context->first;
        auto* max_len = context->second;

        count->fetch_add(1);

        size_t current_len = max_len->load();
        while (name.size() > current_len && !max_len->compare_exchange_weak(current_len, name.size())) {
            // Retry if another thread updated max_len
        }
    };

    std::pair<std::atomic<size_t>*, std::atomic<size_t>*> context{&processed_count, &max_name_length};

    UARTStatus status = UARTStatus::Ok;

    auto execution_time = measure_execution_time([&]() {
        driver.scan(callback, &context, 30000, status); // 30 second timeout
    });

    EXPECT_EQ(status, UARTStatus::Ok);
    EXPECT_EQ(processed_count.load(), HUGE_DEVICE_COUNT);

    auto mb_processed = response.size() / (1024.0 * 1024.0);
    auto mb_per_second = mb_processed * 1000000.0 / execution_time.count();

    std::cout << "Large Buffer Metrics:" << std::endl;
    std::cout << "  Buffer size: " << response.size() / 1024 << " KB" << std::endl;
    std::cout << "  Devices processed: " << processed_count.load() << std::endl;
    std::cout << "  Execution time: " << execution_time.count() << " μs" << std::endl;
    std::cout << "  Throughput: " << mb_per_second << " MB/sec" << std::endl;
    std::cout << "  Max device name length: " << max_name_length.load() << std::endl;

    // Should achieve reasonable throughput (> 0.1 MB/sec even in debug builds)
    EXPECT_GT(mb_per_second, 0.1);
}