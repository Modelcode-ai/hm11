// Unit tests for hm11::CircularBuffer
#include <gtest/gtest.h>

#include "hm11/sync/circular_buffer.hpp"

using namespace hm11;

// Helper type for easier testing
using TestBuffer = CircularBuffer<int, 8>; // size is power of two, capacity 7 elements

TEST(CircularBufferTest, PushPopBasic) {
    TestBuffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.full());

    // Write three values
    buf.write(1);
    buf.write(2);
    buf.write(3);
    EXPECT_FALSE(buf.empty());
    EXPECT_FALSE(buf.full());

    int out = 0;
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 1);
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 2);
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 3);
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.full());
}

TEST(CircularBufferTest, FullDetection) {
    TestBuffer buf;
    // Buffer can hold Size-1 = 7 elements
    for (int i = 0; i < 7; ++i) {
        buf.write(i);
    }
    EXPECT_TRUE(buf.full());

    // Write when full - this overwrites oldest data
    buf.write(100);
    // Buffer is still full after overwrite
    EXPECT_TRUE(buf.full());

    // Pop elements - oldest data (0) was overwritten, should get 1,2,3,4,5,6,100
    int out = 0;
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 1);  // First element (0) was overwritten

    // After one pop, buffer is no longer full
    EXPECT_FALSE(buf.full());

    // Pop remaining elements
    const std::array<int, 6> EXPECTED = {2, 3, 4, 5, 6, 100};
    for (int expected_val : EXPECTED) {
        EXPECT_TRUE(buf.pop(out));
        EXPECT_EQ(out, expected_val);
    }
    EXPECT_TRUE(buf.empty());
}

TEST(CircularBufferTest, EmptyDetection) {
    TestBuffer buf;
    int out = 0;
    EXPECT_FALSE(buf.pop(out));
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.full());
    buf.write(42);
    EXPECT_FALSE(buf.empty());
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 42);
    EXPECT_TRUE(buf.empty());
}

TEST(CircularBufferTest, WrapAroundBehavior) {
    TestBuffer buf;
    // Fill half capacity
    for (int i = 0; i < 4; ++i) {
        buf.write(i);
    }
    // Pop two elements
    int out = 0;
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 0);
    EXPECT_TRUE(buf.pop(out));
    EXPECT_EQ(out, 1);
    // Now write more to cause wrap of write index
    buf.write(4); // idx 4
    buf.write(5); // idx 5
    buf.write(6); // idx 6
    buf.write(7); // idx wraps to 0 (since mask 7)

    // Write one more element to fill the buffer completely (capacity 7)
    buf.write(8);

    // Buffer should now be full (7 elements total)
    EXPECT_TRUE(buf.full());

    // Pop remaining elements and verify order
    const std::array<int, 7> EXPECTED_VALS = {2, 3, 4, 5, 6, 7, 8};
    for (int expected_val : EXPECTED_VALS) {
        EXPECT_TRUE(buf.pop(out));
        EXPECT_EQ(out, expected_val);
    }
    EXPECT_TRUE(buf.empty());
}

// Simulate producer (ISR) and consumer interaction
TEST(CircularBufferTest, ProducerConsumerInteraction) {
    TestBuffer buf;
    // Producer writes a series of values
    for (int i = 0; i < 5; ++i) {
        buf.write(i * 10);
    }
    // Consumer pops them
    int out = 0;
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(buf.pop(out));
        EXPECT_EQ(out, i * 10);
    }
    EXPECT_TRUE(buf.empty());
}
