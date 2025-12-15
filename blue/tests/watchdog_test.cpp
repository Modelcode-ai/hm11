#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

#include "hm11/stm32/uart_port.hpp"
#include "hm11/sync/watchdog.hpp"

using namespace hm11;

class WatchdogTest : public ::testing::Test {
  protected:
    Watchdog wd;
};

// Test basic start and stop
TEST_F(WatchdogTest, StartAndStop) {
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    bool closed = true;
    std::size_t position = 0;
    wd.readed(closed, position);
    EXPECT_FALSE(closed);
    EXPECT_EQ(position, 0);

    wd.stop();
    wd.readed(closed, position);
    EXPECT_TRUE(closed);
}

// Test that starting when already started does nothing
TEST_F(WatchdogTest, StartWhenAlreadyStarted) {
    std::array<std::uint8_t, 10> buffer1{};
    std::array<std::uint8_t, 20> buffer2{};

    wd.start(buffer1.data(), buffer1.size(), std::chrono::milliseconds(1000), false);

    bool closed = false;
    std::size_t position = 0;
    wd.readed(closed, position);
    EXPECT_FALSE(closed);

    // Try to start again - should be ignored
    wd.start(buffer2.data(), buffer2.size(), std::chrono::milliseconds(500), true);

    // Verify still using first configuration
    wd.readed(closed, position);
    EXPECT_FALSE(closed);

    wd.stop();
}

// Test await_event returns false when not started
TEST_F(WatchdogTest, AwaitEventNotStarted) {
    Status status = Status::Ok;
    EXPECT_FALSE(wd.await_event(status));
}

// Test await_event timeout
TEST_F(WatchdogTest, AwaitEventTimeout) {
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(50), false);

    Status status = Status::Ok;
    auto start_time = std::chrono::steady_clock::now();
    bool result = wd.await_event(status);
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_FALSE(result);
    EXPECT_GE(elapsed.count(), 45); // Allow some tolerance
    wd.stop();
}

// Test release and await_event
TEST_F(WatchdogTest, ReleaseAndAwaitEvent) {
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    // Release from another thread
    std::thread releaser([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        wd.release(Status::Ok);
    });

    Status status = Status::Error;
    bool result = wd.await_event(status);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);

    releaser.join();
    wd.stop();
}

// Test release with error status
TEST_F(WatchdogTest, ReleaseWithError) {
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    std::thread releaser([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        wd.release(Status::Error);
    });

    Status status = Status::Ok;
    bool result = wd.await_event(status);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Error);

    releaser.join();
    wd.stop();
}

// Test that release only triggers once
TEST_F(WatchdogTest, ReleaseOnlyOnce) {
    std::array<std::uint8_t, 10> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    // Release multiple times - only first should count
    wd.release(Status::Ok);
    wd.release(Status::Error); // Should be ignored

    Status status = Status::Error;
    bool result = wd.await_event(status);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok); // Should be first status

    wd.stop();
}

// Test store_byte in non-stream mode
TEST_F(WatchdogTest, StoreBytesNonStream) {
    std::array<std::uint8_t, 5> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    bool closed = false;
    std::size_t position = 0;

    wd.store_byte(0x11);
    wd.readed(closed, position);
    EXPECT_EQ(position, 1);
    EXPECT_EQ(buffer[0], 0x11);

    wd.store_byte(0x22);
    wd.readed(closed, position);
    EXPECT_EQ(position, 2);
    EXPECT_EQ(buffer[1], 0x22);

    wd.stop();
}

// Test store_byte fills buffer and auto-releases in non-stream mode
TEST_F(WatchdogTest, StoreByteFillBufferNonStream) {
    std::array<std::uint8_t, 3> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    wd.store_byte(0xAA);
    wd.store_byte(0xBB);
    wd.store_byte(0xCC); // Should trigger release

    Status status = Status::Error;
    bool result = wd.await_event(status);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);
    EXPECT_EQ(buffer[0], 0xAA);
    EXPECT_EQ(buffer[1], 0xBB);
    EXPECT_EQ(buffer[2], 0xCC);

    wd.stop();
}

// Test store_byte in stream mode with wrap-around
TEST_F(WatchdogTest, StoreBytesStreamMode) {
    std::array<std::uint8_t, 3> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), true);

    bool closed = false;
    std::size_t position = 0;

    wd.store_byte(0x11);
    wd.readed(closed, position);
    EXPECT_EQ(position, 1);

    wd.store_byte(0x22);
    wd.readed(closed, position);
    EXPECT_EQ(position, 2);

    wd.store_byte(0x33);
    wd.readed(closed, position);
    EXPECT_EQ(position, 3);

    // Next byte should wrap around - position wraps to 0 and stores there
    wd.store_byte(0x44);
    wd.readed(closed, position);
    EXPECT_EQ(position, 0); // Wrapped to position 0, stays at 0
    EXPECT_EQ(buffer[0], 0x44);

    wd.stop();
}

// Test store_byte when not started
TEST_F(WatchdogTest, StoreByteNotStarted) {
    std::array<std::uint8_t, 5> buffer{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Don't start watchdog
    wd.store_byte(0xAA);

    // Buffer should remain unchanged
    EXPECT_EQ(buffer[0], 0xFF);
    EXPECT_EQ(buffer[1], 0xFF);
}

// Test store_byte with null buffer
TEST_F(WatchdogTest, StoreByteNullBuffer) {
    wd.start(nullptr, 10, std::chrono::milliseconds(1000), false);

    // Should not crash
    wd.store_byte(0xAA);

    bool closed = false;
    std::size_t position = 0;
    wd.readed(closed, position);
    EXPECT_EQ(position, 0); // Position should not advance

    wd.stop();
}

// Test buffer clearing in non-stream mode
TEST_F(WatchdogTest, BufferClearingNonStream) {
    std::array<std::uint8_t, 5> buffer{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), false);

    // In non-stream mode, buffer should be cleared
    EXPECT_EQ(buffer[0], 0x00);
    EXPECT_EQ(buffer[1], 0x00);
    EXPECT_EQ(buffer[2], 0x00);
    EXPECT_EQ(buffer[3], 0x00);
    EXPECT_EQ(buffer[4], 0x00);

    wd.stop();
}

// Test buffer NOT cleared in stream mode
TEST_F(WatchdogTest, BufferNotClearingStreamMode) {
    std::array<std::uint8_t, 5> buffer{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(1000), true);

    // In stream mode, buffer should NOT be cleared
    EXPECT_EQ(buffer[0], 0xFF);
    EXPECT_EQ(buffer[1], 0xFF);
    EXPECT_EQ(buffer[2], 0xFF);

    wd.stop();
}

// Test buffer clearing with null pointer (should not crash)
TEST_F(WatchdogTest, BufferClearingNullPointer) {
    wd.start(nullptr, 10, std::chrono::milliseconds(1000), false);

    // Should not crash
    bool closed = false;
    std::size_t position = 0;
    wd.readed(closed, position);
    EXPECT_FALSE(closed);

    wd.stop();
}

// Test multiple start-stop cycles
TEST_F(WatchdogTest, MultipleStartStopCycles) {
    std::array<std::uint8_t, 5> buffer{};

    for (int i = 0; i < 3; ++i) {
        wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(100), false);

        bool closed = false;
        std::size_t position = 0;
        wd.readed(closed, position);
        EXPECT_FALSE(closed);

        wd.stop();

        wd.readed(closed, position);
        EXPECT_TRUE(closed);
    }
}

// Test concurrent store_byte and await_event
TEST_F(WatchdogTest, ConcurrentStoreAndAwait) {
    std::array<std::uint8_t, 100> buffer{};
    wd.start(buffer.data(), buffer.size(), std::chrono::milliseconds(2000), false);

    std::thread storer([this]() {
        for (int i = 0; i < 100; ++i) {
            wd.store_byte(static_cast<std::uint8_t>(i));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    Status status = Status::Error;
    bool result = wd.await_event(status);

    EXPECT_TRUE(result);
    EXPECT_EQ(status, Status::Ok);

    storer.join();

    // Verify all bytes stored
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_EQ(buffer.at(i), static_cast<std::uint8_t>(i));
    }

    wd.stop();
}
