#!/bin/bash
# Build script for vanity project

set -e  # Exit on error

echo "Configuring CMake build..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
cmake --build build

echo "Build completed successfully!"
echo "Binary location: build/bin/vanity"
