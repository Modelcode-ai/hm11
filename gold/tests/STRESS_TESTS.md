# Stress Tests for HM11 Driver

This directory contains stress tests for verifying concurrency primitives under high contention.

## Overview

Stress tests differ from unit tests by:
- Running operations thousands/millions of times
- Using multiple concurrent threads
- Intentionally creating race conditions
- Testing timeout precision and edge cases
- Verifying memory ordering with ThreadSanitizer

**Goal**: Find concurrency bugs that only manifest under heavy load.

## Test Targets

### 1. `stress_test_circular_buffer.cpp`
Tests CircularBuffer SPSC implementation ([circular_buffer.hpp](../include/hm11/sync/circular_buffer.hpp)):

**Tests:**
- **RapidProducerConsumer**: 1M items, verify no data corruption
- **BurstyTraffic**: Overflow buffer intentionally, verify overwrite policy
- **WraparoundStress**: Force frequent wraparound with tiny buffer
- **OverwritePolicyStress**: Verify oldest data overwritten when full
- **PeekStress**: Verify peek matches subsequent read
- **AvailableCountStress**: Verify available() count accuracy
- **ClearUnderLoad**: Clear buffer while producer is writing
- **MemoryOrderingVerification**: Detect visibility issues with sequential data

**Why stress test?**
- Lock-free algorithm (subtle race conditions possible)
- ISR-to-thread communication (memory ordering critical)
- Power-of-2 wraparound math (overflow bugs possible)

**Expected Results:**
- No data corruption (sequential data verified)
- No assertion failures
- ThreadSanitizer finds no race conditions

### 2. `stress_test_watchdog.cpp`
Tests Watchdog timeout manager ([watchdog.hpp](../include/hm11/sync/watchdog.hpp)):

**Tests:**
- **DoubleSignalPrevention**: 3 ISRs call release() simultaneously - only first should win
- **TimeoutPrecision**: Measure timeout accuracy (should be <5ms error)
- **StartStopIdempotency**: Multiple start/stop calls should be safe
- **RapidCycles**: 10K start-release-await cycles
- **ConcurrentStartAttempts**: Multiple threads call start() simultaneously
- **ReleaseBeforeAwait**: Signal-before-wait should return immediately
- **DeadlineCheckUnderLoad**: ISR checks deadline 100 times per operation
- **MemoryOrderingVerification**: Verify buffer data visible after release
- **StopDuringAwait**: Call stop() while await_event() is blocked
- **LongDurationTimeout**: Test 1-second timeout (no overflow)

**Why stress test?**
- Complex state machine (started, event_occurred, status)
- Multiple threads (application + ISR)
- Atomic compare_exchange for double-signal prevention
- Deadline-based timeout (timing-sensitive)

**Expected Results:**
- Only first release() takes effect (double-signal prevention works)
- Timeout accuracy within 5ms
- No deadlocks or race conditions

### 3. `stress_test_binary_semaphore.cpp`
Tests BinarySemaphore ISR-safe signaling ([binary_semaphore.hpp](../include/hm11/sync/binary_semaphore.hpp)):

**Tests:**
- **RapidGiveTakeCycles**: 100K give/take cycles
- **MultipleGivesCoalesce**: 3 gives, only 1 take succeeds (binary semantics)
- **SignalBeforeWait**: Give before take should return immediately
- **TimeoutAccuracy**: Verify 10ms timeout precision
- **TryTakeContention**: Multiple threads try_take(), only one succeeds
- **MemoryOrderingVerification**: Verify shared data visible after give
- **ConcurrentGives**: Multiple ISRs give simultaneously
- **RapidAutoReset**: Verify second take fails after first (auto-reset)
- **TakeTimeoutExpiry**: Take without give should timeout
- **ProducerConsumerPattern**: 10K items, verify all consumed

**Why stress test?**
- Atomic flag with custom memory ordering
- ISR-safe operations
- Auto-reset semantics
- Binary coalescing (multiple gives = one signal)

**Expected Results:**
- Binary coalescing works (multiple gives don't accumulate)
- Auto-reset works (semaphore clears after take)
- Timeout accuracy within 5ms
- No memory ordering violations

## Building

### Prerequisites
- C++20 compiler with std::chrono, std::thread, std::atomic
- Google Test framework
- **ThreadSanitizer** (highly recommended)

### Build with ThreadSanitizer (Recommended)

ThreadSanitizer (TSan) detects:
- Data races
- Use of uninitialized memory
- Incorrect memory ordering
- Deadlocks

```bash
# Using Clang (recommended)
mkdir -p build
cd build

# Build stress tests with ThreadSanitizer
clang++ -fsanitize=thread -g -O1 -std=c++20 \
        -I../include \
        ../tests/stress_test_circular_buffer.cpp \
        -lgtest -lgtest_main -pthread \
        -o stress_test_circular_buffer

clang++ -fsanitize=thread -g -O1 -std=c++20 \
        -I../include \
        ../tests/stress_test_watchdog.cpp \
        ../src/sync/watchdog.cpp \
        ../src/sync/binary_semaphore.cpp \
        ../src/platform/wait_event_linux.cpp \
        -lgtest -lgtest_main -pthread \
        -o stress_test_watchdog

clang++ -fsanitize=thread -g -O1 -std=c++20 \
        -I../include \
        ../tests/stress_test_binary_semaphore.cpp \
        ../src/sync/binary_semaphore.cpp \
        ../src/platform/wait_event_linux.cpp \
        -lgtest -lgtest_main -pthread \
        -o stress_test_binary_semaphore
```

### Build without ThreadSanitizer (Faster, Less Safe)

```bash
g++ -g -O2 -std=c++20 \
    -I../include \
    ../tests/stress_test_circular_buffer.cpp \
    -lgtest -lgtest_main -pthread \
    -o stress_test_circular_buffer
```

## Running

### Run Individual Tests
```bash
./stress_test_circular_buffer
./stress_test_watchdog
./stress_test_binary_semaphore
```

### Run All Tests
```bash
./stress_test_circular_buffer && \
./stress_test_watchdog && \
./stress_test_binary_semaphore
```

### Run Specific Test Case
```bash
./stress_test_circular_buffer --gtest_filter=CircularBufferStressTest.RapidProducerConsumer
```

### Run with Increased Iterations (Overnight Testing)
Edit source files and increase ITERATIONS constants:
- `CircularBufferStressTest`: Change `NUM_ITEMS = 1'000'000` to `10'000'000`
- `WatchdogStressTest`: Change `ITERATIONS = 10'000` to `100'000`
- `BinarySemaphoreStressTest`: Change `ITERATIONS = 100'000` to `1'000'000`

Then rebuild and run overnight.

## Interpreting Results

### Success Output
```
[==========] Running 8 tests from 1 test suite.
[----------] 8 tests from CircularBufferStressTest
[ RUN      ] CircularBufferStressTest.RapidProducerConsumer_Sequential
Write failures (buffer full): 12345
[       OK ] CircularBufferStressTest.RapidProducerConsumer_Sequential (1234 ms)
...
[==========] 8 tests from 1 test suite ran. (12345 ms total)
[  PASSED  ] 8 tests.
```

### ThreadSanitizer Warning (Race Condition Found)
```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 1 at 0x7b0400001234 by thread T1:
    #0 CircularBuffer::write() circular_buffer.hpp:72
  Previous read of size 1 at 0x7b0400001234 by thread T2:
    #0 CircularBuffer::read() circular_buffer.hpp:94
  ...
==================
```
**Action Required**: Fix the race condition! The code is not thread-safe.

### Assertion Failure
```
stress_test_circular_buffer.cpp:123: Failure
Value of: data
  Actual: 42
Expected: expected
Which is: 43
Data corruption detected at index 12345: expected 43, got 42
```
**Action Required**: Data corruption indicates memory ordering issue or buffer overflow.

### Timeout (Test Hangs)
```
[ RUN      ] WatchdogStressTest.RapidCycles
...
(no output for minutes)
```
**Action Required**: Deadlock detected. Check semaphore logic.

## Performance Benchmarks

Expected performance on modern hardware (AMD Ryzen 7 / Intel Core i7):

| Test | Iterations | Time | Throughput |
|------|-----------|------|------------|
| CircularBuffer Rapid | 1M items | ~2s | 500K items/s |
| CircularBuffer Bursty | 1K bursts | ~0.5s | 2K bursts/s |
| Watchdog DoubleSignal | 10K cycles | ~3s | 3K cycles/s |
| Watchdog RapidCycles | 10K cycles | ~2s | 5K cycles/s |
| Semaphore RapidCycles | 100K cycles | ~5s | 20K cycles/s |

If your hardware is significantly slower, reduce ITERATIONS.

## CI Integration

### GitHub Actions Example

```yaml
name: Stress Tests

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 0 * * 0'  # Weekly on Sunday

jobs:
  stress-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y clang-14 libgtest-dev

      - name: Build with ThreadSanitizer
        run: |
          mkdir build && cd build
          clang++-14 -fsanitize=thread -g -O1 -std=c++20 \
                     -I../include \
                     ../tests/stress_test_circular_buffer.cpp \
                     -lgtest -lgtest_main -pthread \
                     -o stress_test_circular_buffer

      - name: Run Stress Tests
        run: |
          cd build
          ./stress_test_circular_buffer
          ./stress_test_watchdog
          ./stress_test_binary_semaphore
        timeout-minutes: 30
```

## Debugging Race Conditions

If ThreadSanitizer reports a race:

1. **Identify the conflicting accesses**:
   ```
   Write at 0x7b0400001234 by thread T1:
   Read at 0x7b0400001234 by thread T2:
   ```

2. **Check memory ordering**:
   - Are atomic operations using correct memory_order?
   - Should use `memory_order_release` for writes
   - Should use `memory_order_acquire` for reads

3. **Verify happens-before relationships**:
   - Does thread T2 observe thread T1's writes?
   - Is there proper synchronization (semaphore, atomic, mutex)?

4. **Reproduce with simpler test**:
   - Reduce ITERATIONS to minimum that reproduces
   - Add logging to trace execution order
   - Use `std::this_thread::sleep_for()` to control timing

## Valgrind/Helgrind Alternative

If ThreadSanitizer is not available, use Helgrind:

```bash
valgrind --tool=helgrind ./stress_test_circular_buffer
```

Note: Helgrind is slower but more portable than ThreadSanitizer.

## Known Issues

### False Positives
ThreadSanitizer may report false positives for:
- Intentional benign races (rare)
- Third-party library issues

**Solution**: Add suppressions file:
```bash
cat > tsan.supp <<EOF
# Suppress false positive in gtest
race:testing::internal::*
EOF

export TSAN_OPTIONS="suppressions=tsan.supp"
./stress_test_circular_buffer
```

### Platform-Specific Timing
Timeout tests may fail on slow CI runners. If you see:
```
Timeout error too large (>5ms)
```

Increase acceptable error in test assertions (lines with `EXPECT_LT(max_error, 5000)`).

## Expected Outcomes

Based on the security review, these stress tests should:

✅ **PASS**: All tests complete without failures
✅ **CLEAN**: ThreadSanitizer reports no race conditions
✅ **FAST**: Complete in under 1 minute on modern hardware
✅ **DETERMINISTIC**: Same results on repeated runs

If any test fails, investigate immediately - it may indicate:
- Concurrency bug (race condition, deadlock)
- Memory ordering issue
- Platform-specific timing problem

## References

- [ThreadSanitizer Documentation](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [C++ Memory Model](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [Google Test Documentation](https://google.github.io/googletest/)
