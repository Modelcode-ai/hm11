# CMake Usage Guide - Fuzzing and Stress Tests

Complete reference for building and running fuzzing and stress tests using CMake.

## CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `BUILD_TESTS` | ON/OFF | ON | Build unit tests |
| `ENABLE_STRESS_TESTS` | ON/OFF | OFF | Build stress tests with ThreadSanitizer |
| `ENABLE_FUZZING` | ON/OFF | OFF | Build fuzzing tests with LibFuzzer (requires Clang) |
| `CMAKE_CXX_COMPILER` | path | (auto) | C++ compiler (use `clang++` for fuzzing) |
| `CMAKE_BUILD_TYPE` | Debug/Release | (none) | Build type |

## Configuration Examples

### 1. Unit Tests Only (Default)

```bash
cmake .. -DBUILD_TESTS=ON
make
ctest
```

### 2. Unit + Stress Tests

```bash
cmake .. -DBUILD_TESTS=ON -DENABLE_STRESS_TESTS=ON
make
ctest --output-on-failure
```

### 3. Unit + Stress + Fuzzing (Full Suite)

```bash
cmake .. -DBUILD_TESTS=ON \
         -DENABLE_STRESS_TESTS=ON \
         -DENABLE_FUZZING=ON \
         -DCMAKE_CXX_COMPILER=clang++
make -j$(nproc)
```

### 4. Fuzzing Only (For Security Team)

```bash
cmake .. -DENABLE_FUZZING=ON \
         -DCMAKE_CXX_COMPILER=clang++
make fuzz_parsing_utils fuzz_fixed_string fuzz_ranged_integer
```

### 5. Debug Build with Sanitizers

```bash
cmake .. -DBUILD_TESTS=ON \
         -DENABLE_STRESS_TESTS=ON \
         -DCMAKE_BUILD_TYPE=Debug \
         -DENABLE_SANITIZERS=ON
make
```

## Build Targets

### Fuzzing Targets

| Target | Description |
|--------|-------------|
| `fuzz_parsing_utils` | Build parsing utilities fuzzer |
| `fuzz_fixed_string` | Build fixed string fuzzer |
| `fuzz_ranged_integer` | Build ranged integer fuzzer |
| `run-fuzz` | Build and run all fuzzers (15 min) |
| `run-fuzz-parsing` | Build and run parsing fuzzer (5 min) |
| `run-fuzz-fixed-string` | Build and run fixed string fuzzer (5 min) |
| `run-fuzz-ranged-integer` | Build and run ranged integer fuzzer (5 min) |

**Example:**
```bash
make fuzz_parsing_utils        # Just build
make run-fuzz-parsing          # Build and run
```

### Stress Test Targets

| Target | Description |
|--------|-------------|
| `stress_test_circular_buffer` | Build circular buffer stress test |
| `stress_test_watchdog` | Build watchdog stress test |
| `stress_test_binary_semaphore` | Build binary semaphore stress test |
| `run-stress` | Build and run all stress tests via CTest |

**Example:**
```bash
make stress_test_circular_buffer    # Just build
make run-stress                     # Build and run all
```

### Convenience Targets

| Target | Description |
|--------|-------------|
| `hm11_tests` | Build all unit tests |
| `run-all-tests` | Run all tests (unit + stress if enabled) |
| `format` | Format all source files with clang-format |
| `format-check` | Check formatting without modifying files |

## CTest Commands

### Run Unit Tests

```bash
cd build
ctest                              # Run all tests
ctest --output-on-failure          # Show output on failure
ctest -V                           # Verbose output
ctest -j8                          # Run 8 tests in parallel
```

### Run Stress Tests

```bash
# Run only stress tests (labeled "stress")
ctest -L stress --output-on-failure

# Run specific stress test
ctest -R stress_circular_buffer --output-on-failure

# Run stress tests with verbose output
ctest -L stress -V
```

### Run All Tests

```bash
# Run unit + stress tests
ctest --output-on-failure

# With parallel execution
ctest -j$(nproc) --output-on-failure
```

### CTest Filters

```bash
# Run tests matching pattern
ctest -R circular                   # Tests with "circular" in name
ctest -R stress_                    # All stress tests
ctest -R "stress_circular|watchdog" # Circular buffer OR watchdog

# Exclude tests
ctest -E stress                     # Exclude stress tests

# By label
ctest -L stress                     # Tests labeled "stress"
```

## Make Targets Quick Reference

```bash
# Build targets
make                                # Build all configured targets
make hm11_tests                     # Build unit tests
make fuzz_parsing_utils             # Build one fuzzer
make stress_test_circular_buffer    # Build one stress test

# Run targets
make run-fuzz                       # Run all fuzzers (15 min)
make run-fuzz-parsing               # Run parsing fuzzer (5 min)
make run-stress                     # Run all stress tests via CTest
make run-all-tests                  # Run all tests via CTest

# Utility targets
make format                         # Format code
make format-check                   # Check formatting
```

## Directory Structure After Build

```
build/
├── hm11_tests                      # Unit test executable
├── stress_test_circular_buffer     # Stress test executables
├── stress_test_watchdog
├── stress_test_binary_semaphore
├── fuzz/                           # Fuzzing executables
│   ├── fuzz_parsing_utils
│   ├── fuzz_fixed_string
│   └── fuzz_ranged_integer
└── corpus/                         # Fuzzing corpus (auto-created)
    ├── parsing/
    ├── fixed_string/
    └── ranged_integer/
```

## Workflow Examples

### Developer Daily Workflow

```bash
# Configure once
cmake .. -DBUILD_TESTS=ON -DENABLE_STRESS_TESTS=ON -DCMAKE_CXX_COMPILER=clang++

# Each code change
make -j$(nproc)                     # Fast incremental build
ctest --output-on-failure           # Run tests
```

### Pre-Commit Checks

```bash
# Build and test everything
make -j$(nproc)
ctest --output-on-failure

# Check formatting
make format-check

# Quick fuzzing (optional)
timeout 300 ./fuzz/fuzz_parsing_utils corpus/parsing || true
```

### Security Audit

```bash
# Build with all sanitizers
cmake .. -DBUILD_TESTS=ON \
         -DENABLE_STRESS_TESTS=ON \
         -DENABLE_FUZZING=ON \
         -DENABLE_SANITIZERS=ON \
         -DCMAKE_CXX_COMPILER=clang++

# Build
make -j$(nproc)

# Run unit tests
ctest --output-on-failure

# Run stress tests (with ThreadSanitizer)
ctest -L stress --output-on-failure

# Run fuzzing (1 hour each)
make run-fuzz  # Or run each for 1 hour manually
```

### CI/CD Pipeline

```bash
# CI configuration
cmake .. -DBUILD_TESTS=ON \
         -DENABLE_STRESS_TESTS=ON \
         -DENABLE_FUZZING=ON \
         -DWARNINGS_AS_ERRORS=ON \
         -DCMAKE_CXX_COMPILER=clang++

# Build with warnings as errors
make -j$(nproc)

# Run all tests
ctest --output-on-failure

# Quick fuzzing (5 min per target)
make run-fuzz
```

## Troubleshooting

### Error: "Fuzzing requires Clang compiler"

**Solution**: Specify Clang compiler
```bash
cmake .. -DENABLE_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++
```

### Error: "GTest not found"

**Solution**: Install Google Test
```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev

# macOS
brew install googletest
```

### Fuzzing tests don't appear in CTest

**Expected behavior**: Fuzzing tests are NOT registered with CTest because:
- They run continuously (not pass/fail)
- They need special corpus directories
- They're meant for longer runs (hours/days)

Use `make run-fuzz` or run directly instead.

### Stress tests fail with ThreadSanitizer warnings

**This is the point!** ThreadSanitizer is detecting race conditions. Review the warning and fix the code.

### Build is too slow

**Solutions**:
```bash
# Parallel build
make -j$(nproc)

# Build only what you need
make hm11_tests                     # Just unit tests
make stress_test_circular_buffer    # Just one stress test

# Disable sanitizers in non-CI builds
cmake .. -DENABLE_SANITIZERS=OFF
```

### Can't find clang++

**Solutions**:
```bash
# Find clang++
which clang++

# Or specify full path
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/clang++-14

# Or install clang
sudo apt-get install clang-14       # Ubuntu
brew install llvm                   # macOS
```

## Advanced Usage

### Custom Fuzzing Duration

```bash
# Modify CMakeLists.txt line 443 (max_total_time=300)
# Or run manually:
./build/fuzz/fuzz_parsing_utils -max_total_time=3600 build/corpus/parsing
```

### Parallel Fuzzing

```bash
# Run 8 fuzzing jobs in parallel
./build/fuzz/fuzz_parsing_utils -jobs=8 -workers=8 build/corpus/parsing
```

### Disable ThreadSanitizer for Faster Stress Tests

```bash
# Edit CMakeLists.txt line 504: set(TSAN_FLAGS "")
# Or rebuild without ENABLE_STRESS_TESTS and compile manually:
g++ -O2 -std=c++20 -I./include tests/stress_test_circular_buffer.cpp \
    -lgtest -lgtest_main -pthread -o build/stress_test_circular_buffer
```

### Custom CTest Output

```bash
# JUnit XML for CI
ctest --output-junit results.xml

# Dashboard submission
ctest -D Experimental

# Repeat tests (flaky test detection)
ctest --repeat until-pass:3
```

## References

- CMake documentation: https://cmake.org/documentation/
- CTest documentation: https://cmake.org/cmake/help/latest/manual/ctest.1.html
- LibFuzzer: https://llvm.org/docs/LibFuzzer.html
- ThreadSanitizer: https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual
