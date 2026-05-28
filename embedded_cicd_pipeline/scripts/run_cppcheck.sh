#!/bin/bash
set -e
mkdir -p build-cppcheck
cmake -S . -B build-cppcheck -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cppcheck --enable=all --error-exitcode=1 --suppress=missingInclude --suppress=unmatchedSuppression --inline-suppr --std=c++17 -i build-cppcheck/_deps --project=build-cppcheck/compile_commands.json --output-file=cppcheck_report.xml 2>&1 | tee cppcheck_output.txt
exit ${PIPESTATUS[0]}
