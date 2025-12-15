#!/bin/bash
# Code Coverage Generation with Clang-20/llvm-cov for HM11 Driver
# This script is tailored for this repository's structure
#
# Test Structure:
#   - driver_test: Core driver functionality tests
#   - driver_mock_test: Driver mock tests
#   - at_command_test: AT command builder tests
#   - response_parser_test: Response parser tests
#   - driver_connection_test: Connection management tests
#   - util_tests: Utility functions (status, error, utils, array_utils, circular_buffer)
#   - binary_semaphore_test: Binary semaphore tests
#   - watchdog_test: Watchdog tests
#   - hal_types_integration: HAL types integration tests
#   - error_handling_integration: Error handling integration tests
#   - memory_alignment_integration: Memory alignment integration tests
#   - basic_driver_integration: Basic driver integration tests
#   - dummy_test: Dummy integration test
#   - driver_integration_test: Driver integration tests
#   - minimal_test: Minimal test
#   - example_test: Example test

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
# Export LLVM_PROFILE_FILE with absolute path so it's inherited by all test processes
export LLVM_PROFILE_FILE="$(pwd)/coverage-%p.profraw"
ctest -C Debug --output-on-failure
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
    "build/clang-coverage/tests/driver/driver_test"
    "build/clang-coverage/tests/driver/driver_mock_test"
    "build/clang-coverage/tests/driver/at_command_test"
    "build/clang-coverage/tests/driver/response_parser_test"
    "build/clang-coverage/tests/driver/driver_connection_test"
    "build/clang-coverage/tests/util/util_tests"
    "build/clang-coverage/tests/sync/binary_semaphore_test"
    "build/clang-coverage/tests/sync/watchdog_test"
    "build/clang-coverage/tests/integration/hal_types_integration"
    "build/clang-coverage/tests/integration/error_handling_integration"
    "build/clang-coverage/tests/integration/memory_alignment_integration"
    "build/clang-coverage/tests/integration/basic_driver_integration"
    "build/clang-coverage/tests/integration/dummy_test"
    "build/clang-coverage/tests/integration/driver_integration_test"
    "build/clang-coverage/tests/minimal_test"
    "build/clang-coverage/tests/example_test"
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
