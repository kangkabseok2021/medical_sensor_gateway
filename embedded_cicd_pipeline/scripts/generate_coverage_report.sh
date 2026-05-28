#!/bin/bash
set -e
mkdir -p build-coverage
cd build-coverage
cmake .. -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest --output-on-failure
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/_deps/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory ../coverage_html
LINE_PCT=$(lcov --summary coverage_filtered.info 2>&1 | grep 'lines' | grep -oP '[0-9]+\.[0-9]+' | head -1)
BRANCH_PCT=$(lcov --summary coverage_filtered.info 2>&1 | grep 'branches' | grep -oP '[0-9]+\.[0-9]+' | head -1 || echo "0.0")
echo "Line: $LINE_PCT%, Branch: $BRANCH_PCT%"
python3 -c "import sys; sys.exit(0 if float('$LINE_PCT') >= 90.0 else 1)" || (echo "FAIL: line coverage $LINE_PCT% < 90%"; exit 1)
