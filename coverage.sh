#!/bin/sh

set -euo pipefail
BUILD_DIR="build"
PROFILE_FILE="$BUILD_DIR/tests/default.profraw"
PROFILE_DATA="$BUILD_DIR/tests/default.profdata"
HTML_DIR="$BUILD_DIR/coverage-html"

echo $PROFILE_FILE

# Run tests with LLVM profiling enabled
ctest --test-dir "$BUILD_DIR" --output-on-failure

# Merge raw profile data
llvm-profdata merge -sparse "$PROFILE_FILE" -o "$PROFILE_DATA"

# Generate console report, ignoring system headers and gtest
llvm-cov report "$BUILD_DIR/tests/void_compiler_tests" \
    -instr-profile="$PROFILE_DATA" \
    -ignore-filename-regex='/usr|googletest|test'

# Generate HTML report, ignoring system headers and gtest
llvm-cov show "$BUILD_DIR/tests/void_compiler_tests" \
    -instr-profile="$PROFILE_DATA" \
    -ignore-filename-regex='/usr|googletest|test' \
    -show-line-counts-or-regions \
    -format=html -o "$HTML_DIR"

echo "Coverage report generated: $HTML_DIR/index.html"
