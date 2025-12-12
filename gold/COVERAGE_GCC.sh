#!/bin/bash
# Code Coverage Generation with GCC/gcov
# Alternative if Clang not available (use COVERAGE_CLANG.sh if you have clang++-20)
# See PRESET_COMMANDS.md for full documentation

set -e  # Exit on error

echo "=== HM11 Driver Code Coverage Generation ==="
echo ""
echo "Step 1/7: Configuring build with coverage..."
cmake --preset gcc-coverage

echo ""
echo "Step 2/7: Building with coverage instrumentation..."
cmake --build --preset gcc-coverage -j$(nproc)

echo ""
echo "Step 3/7: Running tests..."
ctest --preset gcc-coverage

echo ""
echo "Step 4/7: Capturing coverage data..."
lcov --capture --gcov-tool gcov-14 --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=0 \
     -d build/gcc-coverage -o build/gcc-coverage/coverage.info

echo ""
echo "Step 5/7: Filtering system headers and test code..."
lcov --remove build/gcc-coverage/coverage.info \
     '/usr/*' '*/tests/*' '*/fuzz/*' '*/examples/*' \
     --gcov-tool gcov-14 --ignore-errors mismatch,unused \
     -o build/gcc-coverage/coverage_filtered.info

echo ""
echo "Step 6/7: Generating HTML report..."
genhtml build/gcc-coverage/coverage_filtered.info \
        -o build/gcc-coverage/coverage_html \
        -t "HM11 Driver Coverage Report" \
        --legend --demangle-cpp

echo ""
echo "Step 7/7: Generating summary..."
lcov --summary build/gcc-coverage/coverage_filtered.info --gcov-tool gcov-14 | tee build/gcc-coverage/coverage_summary.txt

echo ""
echo "=== COVERAGE GENERATION COMPLETE ==="
echo ""
echo "Summary saved to: build/gcc-coverage/coverage_summary.txt"
echo "HTML report at:   build/gcc-coverage/coverage_html/index.html"
echo ""
echo "Coverage Summary:"
cat build/gcc-coverage/coverage_summary.txt
