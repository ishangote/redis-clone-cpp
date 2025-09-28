#!/bin/bash

# Build script for redis-clone-cpp
# Usage: ./scripts/build.sh

set -euo pipefail  # Stricter error handling

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "🔨 Building redis-clone-cpp project..."

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Navigate to build directory and build
cd "$BUILD_DIR"
cmake ..
cmake --build .

echo "✅ Build completed successfully!"
echo ""
echo "Available test executables:"
# Look for test executables in bin directory
if [ -d "bin" ]; then
    find "bin" -type f -perm +111 -name "*test*" | while read -r file; do
        echo "  - $(basename "$file")"
    done
fi
