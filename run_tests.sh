#!/bin/bash
set -e

echo "========================================================"
echo "  Page Replacement Simulator - Running Automated Tests"
echo "========================================================"

if [ ! -f "build/backend/test_algorithms" ]; then
    echo "[INFO] Compiling tests with CMake..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
fi

echo ""
echo "[1/2] Running C++ Native Unit Tests..."
./build/backend/test_algorithms

echo ""
echo "[2/2] Running Python End-to-End REST API Invariant Tests..."
python3 tests/test_api_e2e.py

echo ""
echo "========================================================"
echo "  ALL TESTS FINISHED"
echo "========================================================"
