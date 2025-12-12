# Cross-Compilation Tests

This directory contains tests that verify correct compilation for embedded ARM targets. These tests are **not** executed as part of the regular test suite - they exist purely to verify that platform-specific code compiles correctly with the ARM toolchain and generates the expected object code.

## Purpose

Cross-compilation tests serve a different purpose than runtime tests:

| Test Type | Runtime Tests (`tests/`) | Cross-Compilation Tests |
|-----------|-------------------------|-------------------------|
| **Platform** | x86 Linux (host) | ARM Cortex-M (embedded) |
| **Executed** | Yes (automated CI/CD) | No (compile-only) |
| **Verifies** | API behavior, logic | Conditional compilation, CMSIS integration |
| **Tool** | Google Test | ARM toolchain + objdump |
| **When to run** | Every build | Before embedded releases |

## Why These Tests Exist

Embedded systems code often uses conditional compilation and platform-specific features that cannot be tested on the host platform:

1. **Conditional Compilation**: Code that compiles differently for Cortex-M4 vs M7
2. **CMSIS Integration**: Verifies CMSIS headers and functions are accessible
3. **Assembly Verification**: Ensures operations compile to expected machine code (no-ops vs actual instructions)
4. **Toolchain Validation**: Catches ARM-specific compilation issues early

## Test Files

### `cache_ops_compile_test.cpp`

Verifies that cache operations from `include/hm11/platform/cache_ops.hpp` compile correctly:

- **Cortex-M4 (STM32F429)**: Should compile to empty inline functions (no-ops)
- **Cortex-M7 (STM32F7/H7)**: Should compile to CMSIS `SCB_*DCache*` function calls

**Complements**: `tests/test_cache_ops.cpp` (runtime API tests on x86)

## Requirements

To run these tests, you need the ARM embedded toolchain:

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# macOS
brew install --cask gcc-arm-embedded

# Verify installation
arm-none-eabi-g++ --version
```

## Running Tests

### Test 1: Cortex-M4 (No Cache)

Compile for STM32F429 and verify cache operations are no-ops:

```bash
# Navigate to project root
cd /path/to/hm11-dst

# Compile for Cortex-M4
arm-none-eabi-g++ -c -mcpu=cortex-m4 -mthumb \
    -I include \
    -DSTM32F429xx -D__CORTEX_M=4 \
    -std=c++20 -O2 \
    -o build/cache_ops_test_m4.o \
    cross_compilation_tests/cache_ops_compile_test.cpp

# Inspect generated code
arm-none-eabi-objdump -d build/cache_ops_test_m4.o | grep -A 20 "test_cache_operations"
```

**Expected result**: The `test_cache_operations` function should contain only register moves and a return instruction (no cache operations).

### Test 2: Cortex-M7 (With Cache)

Compile for STM32F7 and verify cache operations call CMSIS functions:

```bash
# Compile for Cortex-M7 (requires CMSIS headers)
arm-none-eabi-g++ -c -mcpu=cortex-m7 -mthumb \
    -I include \
    -I vendor/CMSIS/Include \
    -I vendor/CMSIS/Device/ST/STM32F7xx/Include \
    -DSTM32F7 -D__CORTEX_M=7 \
    -std=c++20 -O2 \
    -o build/cache_ops_test_m7.o \
    cross_compilation_tests/cache_ops_compile_test.cpp

# Inspect generated code
arm-none-eabi-objdump -d build/cache_ops_test_m7.o | grep -A 20 "test_cache_operations"
```

**Expected result**: The function should contain calls to `SCB_InvalidateDCache_by_Addr`, `SCB_CleanDCache_by_Addr`, etc.

## Interpreting Results

### Successful Cortex-M4 Compilation

```asm
00000000 <_Z21test_cache_operationsv>:
   0:   4770        bx      lr
```

✅ Function is essentially empty (just returns) - cache operations optimized away as expected.

### Successful Cortex-M7 Compilation

```asm
00000000 <_Z21test_cache_operationsv>:
   0:   b510        push    {r4, lr}
   2:   4c0a        ldr     r4, [pc, #40]
   4:   f44f 7180   mov.w   r1, #256
   8:   4620        mov     r0, r4
   a:   f7ff fffe   bl      <SCB_InvalidateDCache_by_Addr>
   ...
```

✅ Function contains actual function calls to CMSIS cache operations.

### Compilation Failures

If compilation fails, common issues:

1. **Missing CMSIS headers**: Ensure `vendor/CMSIS/` directory exists with STM32 device headers
2. **Wrong CPU**: Verify `-mcpu=cortex-m4` or `-mcpu=cortex-m7` is set correctly
3. **Missing defines**: Ensure `__CORTEX_M` and device family defines are set

## Integration with CMake Presets

These tests are intentionally **not** included in the CMake build system. They are manual verification steps for platform-specific builds.

However, you can use the CMake presets as reference for compiler flags:

```bash
# See available embedded targets
cmake --list-presets | grep stm32

# Example presets that might exist:
# - stm32f429-debug
# - stm32f429-release
# - stm32f7-debug
```

## When to Run These Tests

Run cross-compilation tests:

1. **Before releasing embedded builds**: Verify conditional compilation works
2. **After modifying platform-specific code**: Ensure changes compile for all targets
3. **When updating CMSIS**: Verify new CMSIS version is compatible
4. **During toolchain upgrades**: Catch ARM GCC version-specific issues

## Adding New Tests

When adding new platform-specific code that uses conditional compilation:

1. Create a new `*_compile_test.cpp` file in this directory
2. Follow the pattern from `cache_ops_compile_test.cpp`:
   - Include the header being tested
   - Create a function that uses all conditional operations
   - Add static assertions for compile-time properties
   - Document expected behavior for each target
3. Add compilation instructions to this README
4. Document what to look for in the objdump output

## Relationship to Runtime Tests

Cross-compilation tests **complement** runtime tests, they don't replace them:

```
Platform-specific feature (e.g., cache_ops.hpp)
├── Runtime tests (tests/test_cache_ops.cpp)
│   ├── Runs on: x86 host
│   ├── Verifies: API behavior (no-ops on x86)
│   ├── Automated: Yes (CI/CD)
│   └── Coverage: Null safety, edge cases, API contracts
│
└── Cross-compilation tests (cross_compilation_tests/cache_ops_compile_test.cpp)
    ├── Runs on: Never executed
    ├── Verifies: ARM compilation, CMSIS calls
    ├── Automated: No (manual)
    └── Coverage: Conditional compilation, assembly output
```

Both are needed for comprehensive validation!

## References

- [ARM CMSIS Documentation](https://arm-software.github.io/CMSIS_5/)
- [Cortex-M7 Technical Reference Manual](https://developer.arm.com/documentation/ddi0489/latest)
- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain)
- [Using objdump](https://sourceware.org/binutils/docs/binutils/objdump.html)
