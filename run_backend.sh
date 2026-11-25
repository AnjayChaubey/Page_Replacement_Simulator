#!/bin/bash
set -e

echo "========================================================"
echo "  Page Replacement Simulator - Starting C++ Backend"
echo "========================================================"

if [ ! -f "build/backend/page_replacement_server" ]; then
    echo "[INFO] Compiling backend with CMake..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
fi

echo "[INFO] Launching server on http://localhost:8080..."
echo "[INFO] Open your web browser at: http://localhost:8080"
./build/backend/page_replacement_server 8080
