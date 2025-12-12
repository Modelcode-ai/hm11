# Testing Guide for HM11 Driver

Comprehensive testing strategy covering unit tests, mock integration tests, fuzzing, and stress testing.

## Test Coverage Overview

| Test Type | Purpose | Location | Runtime |
|-----------|---------|----------|---------|
| **Unit Tests** | Functional correctness | `tests/test_*.cpp` | ~5 seconds |
| **Mock Integration** | Example logic validation | `tests/examples/test_*.cpp` | ~10 seconds |
| **Fuzzing** | Find edge cases, crashes | `fuzz/*.cpp` | 5 min - 24 hours |
| **Stress Tests** | Concurrency, race conditions | `tests/stress_test_*.cpp` | ~1 minute |
| **Cross-Compilation** | ARM embedded verification | `cross_compilation_tests/*.cpp` | Manual (compile-only) |

## Mock Testing Coverage Analysis

### What Mock Tests Validate

**✅ Validated by Mock Tests:**
- **AT Command Protocol**: All 40+ AT commands formatted correctly
- **Response Parsing**: Correct parsing of all response formats (OK+Set:X, OK+Get:X, OK+CONN, etc.)
- **Sequence Correctness**: Commands sent in proper order (init → config → operations)
- **State Machine Logic**: Connection lifecycle, scanning, GATT operation flows
- **Error Handling**: Proper handling of timeout, error responses, invalid states
- **API Usage Patterns**: Correct use of driver API by example applications
- **Data Structures**: MacAddress, UUID, PinType, etc. formatting and validation
- **Streaming Data Parsing**: Device discovery callbacks, GATT characteristic parsing
- **Business Logic**: Application-level logic in all 6 examples

**❌ NOT Validated by Mock Tests:**
- **Real HM-11 Hardware**: Actual module responses, timing variations, edge cases
- **Bluetooth Radio**: BLE connectivity, advertising, scanning, pairing
- **Physical Layer**: UART electrical characteristics, DMA hardware, interrupts
- **RF Environment**: Signal strength, interference, range, multipath
- **Power Management**: Sleep modes, wake-up timing, power consumption
- **Hardware Timing**: Real module command processing delays, restart timing
- **Platform Integration**: STM32 GPIO, clocks, memory layout, cache coherency
- **Concurrent Hardware**: Actual interrupt latency, DMA race conditions

### Confidence Estimation Methodology

**Rough Confidence Estimate: 55-65%**

This estimate is based on:

**High Confidence Areas (90%+ within scope):**
- Command formatting and API correctness
- Response parsing for well-formed responses
- Application business logic
- State machine transitions for happy paths

**Medium Confidence Areas (60-70%):**
- Error handling (simulated errors may not match hardware)
- Timeout behavior (mock timing != hardware timing)
- Streaming data parsing (corpus coverage incomplete)

**Low/No Confidence Areas (0-20%):**
- Hardware integration and platform-specific code
- Real-time constraints and interrupt handling
- RF/BLE protocol compliance
- Power management behavior

**Important Disclaimers:**
1. Confidence percentages are inherently subjective and project-specific
2. Mock tests validate **logic correctness**, not **hardware integration**
3. Hardware testing is essential before production deployment
4. These estimates assume comprehensive mock test coverage
5. Real-world failures often occur at hardware/software boundaries that mocks cannot simulate

**Recommendation**: Use mock tests for rapid development and regression detection. Always validate on real hardware before deployment.

## Test Categories

### 1. Unit Tests (Existing)

**Location**: `tests/test_*.cpp`

**Coverage**:
- ✅ Circular buffer operations (SPSC)
- ✅ Fixed string validation
- ✅ Ranged integer bounds
- ✅ Binary semaphore
- ✅ Watchdog timeout manager
- ✅ UART port interface
- ✅ HM11 driver commands (47 security tests)

### 2. Fuzzing Tests (NEW)

**Location**: `fuzz/*.cpp`

**Targets**:
1. **Parsing Utilities** (`fuzz_parsing_utils.cpp`)
   - Hex validation, MAC parsing, integer parsing
   - Temperature/humidity range checks
   - CRLF handling, string operations

2. **Fixed String** (`fuzz_fixed_string.cpp`)
   - Character validators (hex, digit)
   - String construction with all 256 byte values
   - Bounds checking

3. **Ranged Integer** (`fuzz_ranged_integer.cpp`)
   - Arithmetic overflow detection
   - Range boundary conditions
   - Different type ranges (0-100, 0-120, 0-4, 1-9)

**Why Fuzz?**
- Automatically finds edge cases unit tests miss
- Tests all 256 possible byte values
- Discovers integer overflow vulnerabilities
- Runs millions of test cases

**See**: [fuzz/README.md](fuzz/README.md) for detailed guide.

### 3. Stress Tests

**Location**: `tests/stress_test_*.cpp`

**Targets**:
1. **Circular Buffer** (`stress_test_circular_buffer.cpp`)
   - 1M item producer-consumer
   - Bursty traffic (intentional overflow)
   - Wraparound stress
   - Memory ordering verification

2. **Watchdog** (`stress_test_watchdog.cpp`)
   - Double-signal prevention (3 ISRs racing)
   - Timeout precision (<5ms error)
   - 10K rapid cycles
   - Concurrent start attempts

3. **Binary Semaphore** (`stress_test_binary_semaphore.cpp`)
   - 100K rapid give/take cycles
   - Binary coalescing (3 gives = 1 signal)
   - Signal-before-wait
   - Memory ordering verification

**Why Stress Test?**
- Lock-free algorithms need high-contention testing
- Race conditions only appear under load
- Memory ordering bugs are timing-dependent
- Validates production deployment readiness

**See**: [tests/STRESS_TESTS.md](tests/STRESS_TESTS.md) for detailed guide.

### 4. Cross-Compilation Tests

**Location**: `cross_compilation_tests/*.cpp`

**Purpose**: Verify platform-specific code compiles correctly for ARM embedded targets.

**Targets**:
1. **Cache Operations** (`cache_ops_compile_test.cpp`)
   - Cortex-M4: Verifies cache ops compile to no-ops
   - Cortex-M7: Verifies cache ops call CMSIS functions
   - Static assertions for noexcept and constexpr
   - Alignment verification

**Why Cross-Compilation Tests?**
- Validate conditional compilation for different ARM cores
- Verify CMSIS integration before embedded deployment
- Catch ARM toolchain-specific issues early
- Inspect generated assembly to ensure optimal code

**These tests are NOT automated** - they are compiled manually with the ARM toolchain and inspected using `objdump`. They complement runtime tests by verifying target-specific compilation.

**See**: [cross_compilation_tests/README.md](cross_compilation_tests/README.md) for detailed guide.

## Troubleshooting

### Fuzzer: "No new coverage"
✅ **Good!** Means fuzzer explored all code paths. Try longer run.

### Fuzzer: Crash found
❌ **Bug found!** See `crash-*` file. Reproduce with:
```bash
./build/fuzz/fuzz_parsing_utils crash-abc123
```

### Stress Test: ThreadSanitizer warning
❌ **Race condition!** Fix memory ordering:
- Check atomic operations use correct `memory_order`
- Verify happens-before relationships

### Stress Test: Assertion failure
❌ **Data corruption!** Indicates:
- Memory ordering issue
- Buffer overflow
- Race condition in lock-free algorithm

### Stress Test: Timeout/hang
❌ **Deadlock!** Check:
- Semaphore logic
- Circular dependencies
- Missing signal/release

## Interpreting Results

### Fuzzing Success
```
#1000000 DONE cov: 1234 ft: 5678 corp: 42/1234b exec/s: 10000 rss: 50Mb
```
- ✅ Completed without crashes
- `cov: 1234` = code coverage (edges)
- `corp: 42` = 42 interesting inputs found

### Fuzzing Crash
```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow
artifact_prefix='./'; Test unit written to ./crash-abc123
```
- ❌ Crash found!
- Reproduce with: `./build/fuzz/fuzz_parsing_utils crash-abc123`

### Stress Test Success
```
[==========] 8 tests from 1 test suite.
[  PASSED  ] 8 tests.
```
- ✅ All tests passed
- ThreadSanitizer found no race conditions

### Stress Test Race Condition
```
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 1 at 0x7b0400001234 by thread T1:
```
- ❌ Race condition detected!
- Review memory ordering in reported code

## Debugging

### If Fuzzer Finds Crash
```bash
# Reproduce crash
./build/fuzz/fuzz_parsing_utils crash-abc123

# Debug with GDB
gdb --args ./build/fuzz/fuzz_parsing_utils crash-abc123
```

### If ThreadSanitizer Reports Race
```bash
# Re-run with detailed output
TSAN_OPTIONS="verbosity=2" ./build/stress_test_circular_buffer
```

### If Test Hangs
```bash
# Run with timeout
timeout 60 ./build/stress_test_watchdog

# If timeout, investigate deadlock
# Add logging to trace execution order
```

## Contributing

When adding new code:

1. ✅ Add unit tests for functionality
2. ✅ Add fuzzing if parsing/validation logic
3. ✅ Add stress test if lock-free/concurrent
4. ✅ Ensure all tests pass before PR
5. ✅ Run ThreadSanitizer locally

## References

- **Unit Testing**: [Google Test](https://google.github.io/googletest/)
- **Fuzzing**: [LibFuzzer](https://llvm.org/docs/LibFuzzer.html)
- **Concurrency**: [ThreadSanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- **Memory Ordering**: [C++ Memory Model](https://en.cppreference.com/w/cpp/atomic/memory_order)
