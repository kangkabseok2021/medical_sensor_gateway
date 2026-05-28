#!/bin/bash
set -e
lizard src/ include/ --CCN 10 --warnings_only --output_file complexity_report.txt
VIOLATIONS=$(grep -c 'VIOLATION' complexity_report.txt 2>/dev/null || echo 0)
echo "Cyclomatic complexity violations: $VIOLATIONS"
[ $VIOLATIONS -eq 0 ] || (cat complexity_report.txt; exit 1)
