# Page Replacement Simulator - C++17 Backend Engine

High-performance, mathematically verified C++17 implementations of Operating Systems page replacement algorithms with an embedded REST API server.

## Features

- **Five Replacement Policies**:
  - `FIFO` (First In, First Out)
  - `LRU` (Least Recently Used)
  - `Optimal` (Belady's MIN theoretical lower bound)
  - `LFU` (Least Frequently Used with deterministic LRU tie-breaking)
  - `Second Chance` (Circular buffer / Clock algorithm with reference bits)
- **Snapshot Generator**: Produces fine-grained execution trace snapshots with algorithm-specific state (clock hand position, reference bits, frequency counters, arrival order).
- **Embedded Web Server**: Lightweight, dependency-free HTTP server built with `cpp-httplib` and `nlohmann/json`.
- **Automated Unit Tests**: Invariant validation covering hit/fault conservation, Belady's Anomaly, edge cases (0 references, 1 frame, identical pages, more frames than unique pages).

---

## Build Instructions

### Prerequisites
- Modern C++ compiler supporting C++17 or later (GCC 7+, Clang 8+, or MSVC 2019+)
- CMake (v3.15+)
- Ninja or Make

### Windows (PowerShell)
```powershell
# From the backend directory:
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux / macOS
```bash
# From the backend directory:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Running the Unit Tests

Execute the automated test suite:
```powershell
# Windows
.\build\test_algorithms.exe

# Linux / macOS
./build/test_algorithms
```

Output:
```
========================================
RUNNING C++ PAGE REPLACEMENT UNIT TESTS
========================================
[RUNNING] testStandardString (7,0,1,2,0,3,0,4,2,3,0,3,2 on 3 frames)...
  FIFO: Faults=10, Hits=3, HitRatio=23.0769%
  LRU: Faults=9, Hits=4, HitRatio=30.7692%
  Optimal: Faults=7, Hits=6, HitRatio=46.1538%
  LFU: Faults=8, Hits=5, HitRatio=38.4615%
  SecondChance: Faults=9, Hits=4, HitRatio=30.7692%
[PASSED] testStandardString!
[RUNNING] testBeladyAnomaly...
  FIFO 3 frames faults: 9
  FIFO 4 frames faults: 10
  LRU 3 frames faults:  10
  LRU 4 frames faults:  8
[PASSED] testBeladyAnomaly!
[RUNNING] testEdgeCases...
[PASSED] testEdgeCases!
[RUNNING] testWorkloadGenerator...
[PASSED] testWorkloadGenerator!
========================================
ALL C++ UNIT TESTS PASSED SUCCESSFULLY! 
========================================
```

---

## Running the Server

```powershell
# Starts HTTP server on port 8080 (serves /api/* and frontend/)
.\build\page_replacement_server.exe 8080
```
