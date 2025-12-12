# Fuzzing Tests for HM11 Driver

Automated testing using LibFuzzer to discover crashes, edge cases, and security vulnerabilities.

## Test Targets

### 1. `fuzz_parsing_utils.cpp`
Tests parsing utilities with malformed UART data:
- Hex character validation
- MAC address parsing (12 hex chars)
- Temperature/humidity parsing with range checks
- Integer parsing using `std::from_chars`
- CRLF handling and string operations

### 2. `fuzz_fixed_string.cpp`
Tests FixedString character validators:
- HexCharValidator (validates '0'-'9', 'A'-'F')
- DigitCharValidator (validates '0'-'9')
- Pre-validates inputs before construction (embedded code uses assertions)

### 3. `fuzz_ranged_integer.cpp`
Tests RangedInteger arithmetic:
- Construction within bounds
- Arithmetic operations (addition, subtraction)
- Only tests valid operations (embedded code uses assertions for overflow)

## Expected Results

All fuzzers should complete without crashes:

- **Parsing utilities**: Tests parsing logic with malformed inputs
- **FixedString**: Tests validators by pre-checking characters before construction
- **RangedInteger**: Tests valid arithmetic operations within range bounds

The fuzz tests pre-validate inputs to avoid triggering assertions (embedded code uses
assertions for programmer errors in debug builds, not runtime validation).

## Corpus

Fuzzing corpus is automatically created in:
```
build/clang-fuzz/corpus/
├── parsing/
├── fixed_string/
└── ranged_integer/
```

**Recommendation**: Don't check corpus into git. It regenerates automatically and grows over time.

## References

- [LibFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [OSS-Fuzz](https://google.github.io/oss-fuzz/)
