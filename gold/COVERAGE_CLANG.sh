#!/bin/bash
# Code Coverage Generation with Clang/llvm-cov
# RECOMMENDED for Docker with clang++-20
# See PRESET_COMMANDS.md for full documentation

set -e  # Exit on error

echo "=== HM11 Driver Code Coverage (Clang/llvm-cov) ==="
echo ""
echo "Step 1/5: Configuring build with Clang coverage..."
cmake --preset clang-coverage

echo ""
echo "Step 2/5: Building with coverage instrumentation..."
cmake --build --preset clang-coverage -j$(nproc)

echo ""
echo "Step 3/5: Running tests with coverage profiling..."
cd build/clang-coverage
LLVM_PROFILE_FILE="coverage-%p.profraw" ctest -C Debug --output-on-failure
cd ../..

echo ""
echo "Step 4/5: Merging profile data..."
llvm-profdata-20 merge -sparse build/clang-coverage/coverage-*.profraw -o build/clang-coverage/coverage.profdata

echo ""
echo "Step 5/5: Generating coverage report..."
# Get list of test executables
TEST_EXEC="build/clang-coverage/hm11_tests"

# Generate text summary
llvm-cov-20 report $TEST_EXEC \
    -instr-profile=build/clang-coverage/coverage.profdata \
    -ignore-filename-regex='(tests|fuzz|examples|/usr)' \
    -use-color=false | tee build/clang-coverage/coverage_summary.txt

# Generate HTML report
llvm-cov-20 show $TEST_EXEC \
    -instr-profile=build/clang-coverage/coverage.profdata \
    -format=html \
    -output-dir=build/clang-coverage/coverage_html \
    -ignore-filename-regex='(tests|fuzz|examples|/usr)' \
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
