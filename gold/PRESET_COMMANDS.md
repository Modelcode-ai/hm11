# CMake Preset Commands for Stress Tests and Fuzzing

Commands compatible with your existing workflow using CMake presets.

## Your Commands (Reference)

```bash
COMMANDS__INSTALL = "cmake --preset clang-debug; cmake --preset clang-release; cmake --preset gcc-debug; cmake --preset gcc-release"
COMMANDS__BUILD = "cmake --build --preset clang-debug --parallel; cmake --build --preset clang-release --parallel; cmake --build --preset gcc-debug --parallel; cmake --build --preset gcc-release --parallel"
COMMANDS__TEST = "ctest --preset clang-debug; ctest --preset clang-release; ctest --preset gcc-debug; ctest --preset gcc-release"
COMMANDS__CLEAN_AND_BUILD = "rm -rf build/; cmake --preset clang-debug; cmake --preset clang-release; cmake --preset gcc-debug; cmake --preset gcc-release; cmake --build --preset clang-debug --parallel; cmake --build --preset clang-release --parallel; cmake --build --preset gcc-debug --parallel; cmake --build --preset gcc-release --parallel"
COMMANDS__LINT = "cmake --build --preset clang-debug --target run-clang-tidy; cmake --build --preset clang-debug --target format"
COMMANDS__STRESS_TEST = "cmake --preset clang-stress; cmake --preset gcc-stress; cmake --build --preset clang-stress --parallel; cmake --build --preset gcc-stress --parallel; ctest --preset clang-stress; ctest --preset gcc-stress"
COMMANDS__FUZZ_TEST = "cmake --preset clang-fuzz; cmake --build --preset clang-fuzz --parallel; cmake --build --preset clang-fuzz --target run-fuzz"
```

## Stress Test Command

**What this does:**
1. Configure stress test builds (Clang + GCC)
2. Build all stress tests with ThreadSanitizer
3. Run stress tests via CTest for both compilers

**Breakdown:**
- `cmake --preset clang-stress` - Configure Clang build with `ENABLE_STRESS_TESTS=ON`
- `cmake --preset gcc-stress` - Configure GCC build with `ENABLE_STRESS_TESTS=ON`
- `cmake --build --preset clang-stress --parallel` - Build stress tests (Clang)
- `cmake --build --preset gcc-stress --parallel` - Build stress tests (GCC)
- `ctest --preset clang-stress` - Run stress tests (Clang + ThreadSanitizer)
- `ctest --preset gcc-stress` - Run stress tests (GCC + ThreadSanitizer)

**Expected time:** ~2 minutes (1 min build + 1 min run)

### Fuzz Test Command

**What this does:**
1. Configure fuzzing build (Clang only - LibFuzzer requirement)
2. Build all fuzzing targets
3. Run all fuzzers (5 minutes each = 15 minutes total)

**Breakdown:**
- `cmake --preset clang-fuzz` - Configure with `ENABLE_FUZZING=ON`
- `cmake --build --preset clang-fuzz --parallel` - Build all fuzzers
- `cmake --build --preset clang-fuzz --target run-fuzz` - Run all fuzzers (15 min)

**Expected time:** ~16 minutes (1 min build + 15 min fuzzing)

**Note:** Fuzzing uses `make run-fuzz` target (not CTest) because fuzzers run continuously.

## Quick Reference Table

| Command | Purpose | Time | Compilers |
|---------|---------|------|-----------|
| `COMMANDS__INSTALL` | Configure all builds | ~10s | Clang, GCC |
| `COMMANDS__BUILD` | Build all targets | ~2m | Clang, GCC |
| `COMMANDS__TEST` | Run unit tests | ~5s | Clang, GCC |
| `COMMANDS__STRESS_TEST` | Run stress tests | ~2m | Clang, GCC |
| `COMMANDS__FUZZ_TEST` | Run fuzzing | ~16m | Clang only |
| `COMMANDS__COVERAGE` | Generate code coverage | ~3m | Clang (llvm-cov) |
| `COMMANDS__LINT` | Run linters | ~1m | Clang |
| `COMMANDS__CLEAN_AND_BUILD` | Clean rebuild | ~2m | Clang, GCC |

## Presets Added

### Configure Presets

| Preset | Compiler | Tests | Options |
|--------|----------|-------|---------|
| `clang-stress` | clang++-20 | Unit + Stress | ThreadSanitizer |
| `gcc-stress` | g++-14 | Unit + Stress | ThreadSanitizer |
| `clang-fuzz` | clang++-20 | Fuzzing only | LibFuzzer + ASan + UBSan |
| `gcc-coverage` | g++-14 | Unit tests | Code coverage (gcov) |
| `clang-coverage` | clang++-20 | Unit tests | Code coverage (llvm-cov) |

### Build Presets

| Preset | Description |
|--------|-------------|
| `clang-stress` | Build stress tests (Clang) |
| `gcc-stress` | Build stress tests (GCC) |
| `clang-fuzz` | Build fuzzing tests (Clang) |
| `gcc-coverage` | Build with coverage (GCC + gcov) |
| `clang-coverage` | Build with coverage (Clang + llvm-cov) |

### Test Presets

| Preset | Description |
|--------|-------------|
| `clang-stress` | Run stress tests (Clang + TSan) |
| `gcc-stress` | Run stress tests (GCC + TSan) |
| `gcc-coverage` | Run tests with coverage (GCC + gcov) |
| `clang-coverage` | Run tests with coverage (Clang + llvm-cov) |


## Directory Structure After Build

```
build/
├── clang-debug/              # Unit tests
│   └── hm11_tests
├── clang-release/            # Unit tests (optimized)
│   └── hm11_tests
├── gcc-debug/                # Unit tests (GCC)
│   └── hm11_tests
├── gcc-release/              # Unit tests (GCC, optimized)
│   └── hm11_tests
├── clang-stress/             # Stress tests (NEW)
│   ├── stress_test_circular_buffer
│   ├── stress_test_watchdog
│   └── stress_test_binary_semaphore
├── gcc-stress/               # Stress tests (NEW)
│   ├── stress_test_circular_buffer
│   ├── stress_test_watchdog
│   └── stress_test_binary_semaphore
└── clang-fuzz/               # Fuzzing tests (NEW)
    ├── fuzz/
    │   ├── fuzz_parsing_utils
    │   ├── fuzz_fixed_string
    │   └── fuzz_ranged_integer
    └── corpus/
        ├── parsing/
        ├── fixed_string/
        └── ranged_integer/
```

## Make Targets Available

After building with stress/fuzz presets, these targets are available:

### Stress Test Targets

```bash
cmake --build --preset clang-stress --target run-stress              # Run all via CTest
cmake --build --preset clang-stress --target stress_test_circular_buffer  # Build one
```

### Fuzzing Targets

```bash
cmake --build --preset clang-fuzz --target run-fuzz                  # Run all (15 min)
cmake --build --preset clang-fuzz --target run-fuzz-parsing          # Run one (5 min)
cmake --build --preset clang-fuzz --target fuzz_parsing_utils        # Build one
```

## Code Coverage Command

**Recommended: Use Clang (simpler, no version issues)**

**Command:**
```bash
COMMANDS__COVERAGE = "cmake --preset clang-coverage; cmake --build --preset clang-coverage --parallel; ctest --preset clang-coverage"
```

**Alternative (GCC):**
```bash
COMMANDS__COVERAGE_GCC = "cmake --preset gcc-coverage; cmake --build --preset gcc-coverage --parallel; ctest --preset gcc-coverage"
```

**What this does:**
1. Configure coverage build with coverage instrumentation
2. Build all tests with coverage tracking enabled
3. Run all tests to generate coverage data

**Breakdown:**
- `cmake --preset clang-coverage` - Configure Clang build with `ENABLE_COVERAGE=ON`
- `cmake --build --preset clang-coverage --parallel` - Build with llvm coverage instrumentation
- `ctest --preset clang-coverage` - Run all tests (generates .profraw coverage files)

**Expected time:** ~2-3 minutes (build + test execution)

**Why Clang?** You have clang++-20 in Docker. No version mismatch issues, cleaner workflow.

### Generating Coverage Reports

After running `COMMANDS__COVERAGE`, generate reports with these commands:

#### For GCC (gcov + lcov):

```bash
# Step 1: Capture coverage data
lcov --capture --directory build/gcc-coverage --output-file build/gcc-coverage/coverage.info

# Step 2: Filter out system headers and test code  
lcov --remove build/gcc-coverage/coverage.info \
    '/usr/*' '*/tests/*' '*/fuzz/*' '*/examples/*' \
    --output-file build/gcc-coverage/coverage_filtered.info

# Step 3: Generate HTML report
genhtml build/gcc-coverage/coverage_filtered.info \
    --output-directory build/gcc-coverage/coverage_html \
    --title "HM11 Driver Coverage Report" \
    --num-spaces 4 \
    --legend \
    --demangle-cpp

# Step 4: View report
# Open build/gcc-coverage/coverage_html/index.html in browser
```

#### For Clang (llvm-cov):

If using clang-coverage preset:

```bash
# Step 1: Merge profiling data
llvm-profdata merge -sparse build/clang-coverage/default.profraw \
    -o build/clang-coverage/coverage.profdata

# Step 2: Generate HTML report
llvm-cov show build/clang-coverage/hm11_tests \
    -instr-profile=build/clang-coverage/coverage.profdata \
    -format=html \
    -output-dir=build/clang-coverage/coverage_html \
    -ignore-filename-regex='(tests|fuzz|examples)' \
    --show-line-counts-or-regions \
    --show-instantiation-summary

# Step 3: View report  
# Open build/clang-coverage/coverage_html/index.html in browser
```

### Quick Coverage Report (Text Summary)

For a quick text summary without HTML:

```bash
# GCC (gcov)
lcov --capture --directory build/gcc-coverage --output-file build/gcc-coverage/coverage.info
lcov --summary build/gcc-coverage/coverage.info

# Or for detailed per-file summary
lcov --list build/gcc-coverage/coverage.info
```

### Docker-Compatible Coverage Commands

For running in a Docker container (all-in-one command):

```bash
# Configure, build, test, and generate coverage report
cmake --preset gcc-coverage && \
cmake --build --preset gcc-coverage --parallel && \
ctest --preset gcc-coverage && \
lcov --capture --directory build/gcc-coverage --output-file build/gcc-coverage/coverage.info && \
lcov --remove build/gcc-coverage/coverage.info '/usr/*' '*/tests/*' '*/fuzz/*' '*/examples/*' --output-file build/gcc-coverage/coverage_filtered.info && \
genhtml build/gcc-coverage/coverage_filtered.info --output-directory build/gcc-coverage/coverage_html --title "HM11 Driver Coverage Report" && \
lcov --summary build/gcc-coverage/coverage_filtered.info
```

**To extract just the summary for reporting:**
```bash
lcov --summary build/gcc-coverage/coverage_filtered.info > coverage_summary.txt
cat coverage_summary.txt
```

### Coverage Report Interpretation

Example output from `lcov --summary`:

```
Summary coverage rate:
  lines......: 87.3% (1234 of 1414 lines)
  functions..: 92.1% (156 of 169 functions)
  branches...: 78.4% (891 of 1137 branches)
```

**Target Coverage Goals:**
- **Lines:** ≥80% (minimum acceptable)
- **Functions:** ≥85% (most functions exercised)
- **Branches:** ≥75% (error paths may be untested)

### Troubleshooting Coverage

**Issue:** No .gcda files generated
```bash
# Solution: Ensure tests actually ran
ctest --preset gcc-coverage --verbose
```

**Issue:** lcov: command not found
```bash
# Ubuntu/Debian
sudo apt-get install lcov

# macOS
brew install lcov

# Or use gcov directly (less friendly)
gcov build/gcc-coverage/CMakeFiles/hm11_driver.dir/src/*.cpp.gcno
```

**Issue:** Low coverage (< 70%)
- Review untested code in HTML report
- Add tests for uncovered branches
- Focus on error handling paths


## Important: Docker/GCC Version Compatibility

### Version Mismatch Fix

If you get this error:
```
geninfo: ERROR: Incompatible GCC/GCOV version found
```

**Solution:** Add `--gcov-tool gcov-14` to all lcov commands:

```bash
# Docker-compatible coverage command (corrected flag order and error handling)
cmake --preset gcc-coverage && \
cmake --build --preset gcc-coverage -j$(nproc) && \
ctest --preset gcc-coverage && \
lcov --capture -d build/gcc-coverage -o build/gcc-coverage/coverage.info --gcov-tool gcov-14 --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=0 && \
lcov --remove build/gcc-coverage/coverage.info '/usr/*' '*/tests/*' '*/fuzz/*' '*/examples/*' -o build/gcc-coverage/coverage_filtered.info --gcov-tool gcov-14 --ignore-errors mismatch,unused && \
genhtml build/gcc-coverage/coverage_filtered.info -o build/gcc-coverage/coverage_html -t "HM11 Driver Coverage Report" --legend --demangle-cpp && \
lcov --summary build/gcc-coverage/coverage_filtered.info --gcov-tool gcov-14 | tee build/gcc-coverage/coverage_summary.txt
```

**Note:** Optional flags like `--gcov-tool` must come AFTER main arguments in lcov commands.

**See [COVERAGE_HOWTO.md](COVERAGE_HOWTO.md) for detailed instructions and [COVERAGE_GCC.sh](COVERAGE_GCC.sh) for executable script.**
