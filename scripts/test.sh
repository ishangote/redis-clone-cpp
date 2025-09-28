#!/bin/bash

# Test script for redis-clone-cpp
# Usage:
#   ./scripts/test.sh                  # build + run all tests
#   ./scripts/test.sh database_test    # build + run a specific test
#   ./scripts/test.sh --no-build database_test  # run without rebuilding

set -euo pipefail

BUILD_DIR="build"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

NO_BUILD=false

# Parse flags
if [ "${1:-}" = "--no-build" ]; then
    NO_BUILD=true
    shift
fi

# Optionally rebuild
if [ "$NO_BUILD" = false ]; then
    echo "🔨 Building project..."
    "$SCRIPT_DIR/build.sh"
else
    echo "⏩ Skipping build step (--no-build)"
fi

echo ""
echo "🧪 Running tests..."

cd "$PROJECT_ROOT/$BUILD_DIR"

if [ $# -eq 0 ]; then
    # Run all tests with CTest
    echo "Running all tests..."
    ctest --output-on-failure
else
    TEST_NAME="$1"
    echo "Running test: $TEST_NAME"

    # Try to locate the executable in the bin directory
    TEST_PATH=$(find "bin" -type f -name "$TEST_NAME" | head -n 1 || true)

    if [ -n "$TEST_PATH" ]; then
        "$TEST_PATH"
    else
        echo "❌ Test executable '$TEST_NAME' not found!"
        echo ""
        echo "Available tests:"
        ctest -N  # lists test names without running them
        exit 1
    fi
fi

echo "✅ Tests completed!"