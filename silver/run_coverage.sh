#!/bin/bash
# Code Coverage Generation with Clang-20/llvm-cov for HM11 Driver
# This script is tailored for this repository's structure
#
# Test Structure:
#   - hm11_hal_tests: HAL/driver tests (core_operations, configuration_api, error_handling, etc.)
#   - hm11_fuzzing_tests: Utility tests (parsing, string_builder, circular_buffer)
#   - hm11_type_tests: Type system tests
#   - hm11_sync_tests: Synchronization tests
#   - hm11_platform_tests: Platform abstraction tests
#   - hm11_performance_tests: Performance/streaming tests

set -e  # Exit on error

echo "=== HM11 Driver Code Coverage (Clang-20/llvm-cov) ==="
echo ""

# Step 1: Configure
echo "Step 1/5: Configuring build with Clang coverage..."
cmake --preset clang-coverage

# Step 2: Build
echo ""
echo "Step 2/5: Building with coverage instrumentation..."
cmake --build --preset clang-coverage -j$(nproc)

# Step 3: Run tests
echo ""
echo "Step 3/5: Running tests with coverage profiling..."
echo "  Running all test executables via ctest..."
cd build/clang-coverage
LLVM_PROFILE_FILE="coverage-%p.profraw" ctest -C Debug --output-on-failure
cd ../..

# Step 4: Merge profile data
echo ""
echo "Step 4/5: Merging profile data..."
llvm-profdata-20 merge -sparse build/clang-coverage/coverage-*.profraw -o build/clang-coverage/coverage.profdata

# Step 5: Generate coverage reports
echo ""
echo "Step 5/5: Generating coverage report..."

# Get list of test executables
TEST_EXECS=(
    "build/clang-coverage/hm11_type_tests"
    "build/clang-coverage/hm11_hal_tests"
    "build/clang-coverage/hm11_performance_tests"
    "build/clang-coverage/hm11_fuzzing_tests"
    "build/clang-coverage/hm11_sync_tests"
    "build/clang-coverage/hm11_platform_tests"
)

# Build the -object arguments for llvm-cov
OBJECT_ARGS=""
for exec in "${TEST_EXECS[@]}"; do
    if [ -f "$exec" ]; then
        OBJECT_ARGS="$OBJECT_ARGS -object=$exec"
    fi
done

# Use the first executable as the main one
MAIN_EXEC="${TEST_EXECS[0]}"

# Generate text summary
echo ""
echo "Generating text summary..."
llvm-cov-20 report $MAIN_EXEC $OBJECT_ARGS \
    -instr-profile=build/clang-coverage/coverage.profdata \
    -ignore-filename-regex='(tests|/usr)' \
    -use-color=false | tee build/clang-coverage/coverage_summary.txt

# Generate HTML report
echo ""
echo "Generating HTML report..."
llvm-cov-20 show $MAIN_EXEC $OBJECT_ARGS \
    -instr-profile=build/clang-coverage/coverage.profdata \
    -format=html \
    -output-dir=build/clang-coverage/coverage_html \
    -ignore-filename-regex='(tests|/usr)' \
    -show-line-counts-or-regions \
    -show-instantiation-summary \
    -Xdemangler c++filt

echo ""
echo "=== COVERAGE GENERATION COMPLETE ==="
echo ""
echo "Summary saved to: build/clang-coverage/coverage_summary.txt"
echo "HTML report at:   build/clang-coverage/coverage_html/index.html"
echo ""
echo "Coverage Summary:"
cat build/clang-coverage/coverage_summary.txt
