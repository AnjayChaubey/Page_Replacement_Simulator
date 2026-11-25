# Page Replacement Simulator

An interactive Operating Systems laboratory for simulating, visualizing, and comparing virtual-memory page replacement policies. The project uses a native C++17 simulation engine, an embedded HTTP server, and a browser dashboard.

## What this project provides

- Step-by-step simulations for **FIFO**, **LRU**, **Optimal (Belady's MIN)**, **LFU**, and **Second Chance (Clock)**.
- Hit, fault, replacement, allocation, and hit/fault-ratio statistics.
- Frame-by-frame snapshots suitable for animation or independent API clients.
- Side-by-side algorithm comparison on the same reference string.
- Deterministic and stochastic workload generation.
- Fault/hit-ratio analysis over multiple frame capacities.
- An interactive Belady's Anomaly experiment.
- Local browser history and CSV export from the dashboard.
- Native C++ unit tests and Python standard-library end-to-end API tests.

## Architecture

```text
Browser dashboard (Vanilla JS, CSS, Chart.js, Lucide)
                         |
                    HTTP + JSON
                         v
C++17 embedded server (cpp-httplib, nlohmann/json)
                         |
                  Simulator core library
                         |
       FIFO | LRU | Optimal | LFU | Second Chance
```

The server also serves the static files in `frontend/`, so the normal development workflow requires only one process.

## Repository layout

```text
Page_Replacement_Simulator/
├── backend/
│   ├── include/              # Public models, simulator, workload, and algorithm headers
│   ├── src/                  # C++ implementations and HTTP server entry point
│   ├── tests/                # Native C++ tests
│   ├── third_party/          # Header-only cpp-httplib and nlohmann/json
│   └── CMakeLists.txt
├── frontend/
│   ├── index.html             # Single-page dashboard
│   ├── js/                    # API client, simulation, charts, and history
│   ├── css/style.css
│   └── assets/                # Bundled Chart.js and Lucide
├── docs/ARCHITECTURE.md       # Extended architecture notes
├── tests/test_api_e2e.py      # REST API end-to-end tests
├── CMakeLists.txt
├── run_backend.bat / .sh     # Build-if-needed and start scripts
└── run_tests.bat / .sh       # Native and API test runners
```

## Requirements

- CMake **3.15 or newer**
- A C++17 compiler:
  - Windows: MSVC, MinGW-w64, or w64devkit
  - Linux/macOS: GCC or Clang
- Ninja or another CMake-supported build tool
- Python **3.8+** for `tests/test_api_e2e.py`
- A modern web browser

The Python API test uses only the standard library; no `pip install` step is required.

## Build and run

Run all commands from the repository root.

### Windows

```powershell
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\backend\page_replacement_server.exe 8080
```

Alternatively:

```powershell
.\run_backend.bat
```

Open [http://localhost:8080](http://localhost:8080). The server binds to `0.0.0.0` and accepts an optional port argument:

```powershell
.\build\backend\page_replacement_server.exe 9090
```

### Linux or macOS

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/backend/page_replacement_server 8080
```

Or:

```bash
./run_backend.sh
```

If the frontend does not load, start the executable from the repository root (or ensure `frontend/` is available at `./frontend`, `../frontend`, or `../../frontend`). The server searches those locations for its static mount point.

## Algorithms

| Name | Policy | Deterministic tie-breaker |
|---|---|---|
| `FIFO` | Evicts the page that arrived first. | Strict queue arrival order. |
| `LRU` | Evicts the page unused for the longest time. | Monotonic access timestamp. |
| `Optimal` | Evicts the page whose next use is farthest away, or never occurs again. | Pages never used again first, then FIFO arrival order. |
| `LFU` | Evicts the page with the lowest access frequency. | LRU among equal-frequency pages. |
| `SecondChance` | Clock policy using reference bits; referenced pages receive another pass. | Circular clock-hand traversal. |

`Optimal` is a theoretical, non-causal benchmark because it can inspect future references. `FIFO` can exhibit Belady's Anomaly; LRU and Optimal are stack algorithms and do not exhibit it for the same reference string.

## REST API

The API is available at `http://localhost:8080/api`. Every response is JSON and CORS is enabled for browser clients.

### Health check

```bash
curl http://localhost:8080/api/health
```

Example response:

```json
{
  "status": "healthy",
  "service": "Page Replacement Algorithm Simulator Backend",
  "version": "1.0.0",
  "algorithms": ["FIFO", "LRU", "Optimal", "LFU", "SecondChance"]
}
```

### Run one simulation: `POST /api/simulate`

Required fields:

- `algorithm`: `FIFO`, `LRU`, `Optimal`, `LFU`, or `SecondChance` (algorithm matching is case-insensitive; `opt`, `second_chance`, and `clock` are also accepted by the core).
- `frames`: integer from **1 through 64**.
- `referenceString`: array of non-negative page IDs, with at most **500** entries.

```bash
curl -X POST http://localhost:8080/api/simulate ^
  -H "Content-Type: application/json" ^
  -d "{\"algorithm\":\"LRU\",\"frames\":3,\"referenceString\":[7,0,1]}"
```

The response contains:

```json
{
  "algorithm": "LRU",
  "algorithmFullName": "Least Recently Used",
  "frameCount": 3,
  "referenceString": [7, 0, 1],
  "totalReferences": 3,
  "pageHits": 0,
  "pageFaults": 3,
  "hitRatio": 0.0,
  "faultRatio": 100.0,
  "replacements": 0,
  "freeAllocations": 3,
  "steps": [
    {
      "stepIndex": 0,
      "page": 7,
      "isHit": false,
      "frames": [7, -1, -1],
      "victimPage": -1,
      "insertedPage": 7,
      "allocatedFrameIndex": 0,
      "status": "FAULT_ALLOCATE",
      "explanation": "Page fault...",
      "algorithmState": {}
    }
  ],
  "tieBreakerPolicy": "Unique monotonic access timestamp...",
  "description": "Replaces the page..."
}
```

`-1` means an empty frame. Each step corresponds to one reference. Valid result invariants are:

```text
pageHits + pageFaults = totalReferences
replacements + freeAllocations = pageFaults
steps.length = referenceString.length
```

### Compare all algorithms: `POST /api/compare`

```bash
curl -X POST http://localhost:8080/api/compare ^
  -H "Content-Type: application/json" ^
  -d "{\"frames\":3,\"referenceString\":[7,0,1,2,0,3,0,4,2,3,0,3,2]}"
```

Response fields are `frameCount`, `referenceString`, `results` (five complete simulation results), `bestAlgorithm`, `lowestFaults`, and `theoreticalNote`. `bestAlgorithm` is the best **practical online** algorithm; `Optimal` is reported separately as the theoretical lower bound.

### Generate a workload: `POST /api/generate-workload`

```bash
curl -X POST http://localhost:8080/api/generate-workload ^
  -H "Content-Type: application/json" ^
  -d "{\"workloadType\":\"locality\",\"count\":25,\"maxPageId\":8,\"seed\":42}"
```

The response contains `workloadType`, `count`, `maxPageId`, and `referenceString`. Supported workload types:

| Type | Behavior |
|---|---|
| `random` | Uniform random pages from `0` to `maxPageId` (default). |
| `sequential` | Ascending pages, wrapping at `maxPageId + 1`. |
| `repeated` | 85% accesses to a 2–4 page working set and 15% random noise. |
| `locality` | 80% accesses within a local window with periodic phase shifts. |
| `high_diversity` | Uniform distribution over at least pages `0`–`15`. |
| `belady_classic` | Repeats `[1,2,3,4,1,2,5,1,2,3,4,5]` until `count` is reached. |

`count` is normalized to 1–300 and `maxPageId` to 1–100 when values are outside those ranges. Supplying a numeric `seed` makes generated random workloads reproducible.

### Analyze frame capacities: `POST /api/workload-analysis`

Use a generated workload:

```bash
curl -X POST http://localhost:8080/api/workload-analysis ^
  -H "Content-Type: application/json" ^
  -d "{\"workloadType\":\"locality\",\"frameCounts\":[2,3,4,5,6]}"
```

Or provide an explicit reference string:

```json
{
  "workloadType": "custom",
  "referenceString": [1, 2, 3, 1, 2, 4, 1],
  "frameCounts": [2, 4, 6]
}
```

The response contains `referenceString`, `frameCountsTested`, `dataPoints`, `detailedResultsAtDefaultFrames`, and `analysisSummary`. Each data point has `frameCount`, `algorithmFaults`, and `algorithmHitRatios`.

### Run Belady's Anomaly experiment: `POST /api/belady-experiment`

With the built-in classic string:

```bash
curl -X POST http://localhost:8080/api/belady-experiment ^
  -H "Content-Type: application/json" ^
  -d "{\"frame1\":3,\"frame2\":4}"
```

The default classic string is:

```text
[1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5]
```

The response includes FIFO, LRU, and Optimal results for both frame counts, `fifoHasAnomaly`, `lruHasAnomaly`, `anomalyOccurred`, and a human-readable `explanation`. For the classic experiment, FIFO produces 9 faults with 3 frames and 10 faults with 4 frames.

### Error responses

Invalid JSON, missing required fields, invalid page IDs, unknown algorithms, or simulation failures return HTTP `400` with this shape:

```json
{
  "error": "Simulation failed: <reason>"
}
```

## Calling the API from Python

The following standard-library client runs a simulation without installing a package:

```python
import json
import urllib.request

payload = {
    "algorithm": "FIFO",
    "frames": 3,
    "referenceString": [7, 0, 1, 2, 0, 3],
}
request = urllib.request.Request(
    "http://127.0.0.1:8080/api/simulate",
    data=json.dumps(payload).encode("utf-8"),
    headers={"Content-Type": "application/json"},
)

with urllib.request.urlopen(request) as response:
    result = json.load(response)

print(result["pageFaults"], result["pageHits"])
```

## Using the C++ core library

The reusable core is exposed through `backend/include/Simulator.h`:

```cpp
#include "Simulator.h"
#include <iostream>
#include <vector>

int main() {
    simulator::Simulator sim;
    std::vector<int> references{7, 0, 1, 2, 0, 3};

    auto result = sim.runSimulation("LRU", 3, references);
    std::cout << "Faults: " << result.pageFaults << "\n";
    std::cout << "Hits: " << result.pageHits << "\n";

    auto comparison = sim.runComparison(3, references);
    std::cout << "Best online algorithm: "
              << comparison.bestAlgorithm << "\n";
}
```

Build targets generated by CMake:

- `page_replacement_core`: static library containing algorithms and analysis.
- `page_replacement_server`: HTTP server and static frontend host.
- `test_algorithms`: native invariant and behavior tests.

## Testing

Build first, then run native tests:

```powershell
.\build\backend\test_algorithms.exe
```

Start the backend in a separate terminal before running the API tests:

```powershell
python tests\test_api_e2e.py
```

The API test covers health, all five algorithms, comparison, all workload types, workload analysis, and the classic Belady experiment. Run both suites with the helper script:

```powershell
.\run_tests.bat
```

On Linux/macOS:

```bash
./build/backend/test_algorithms
python3 tests/test_api_e2e.py
./run_tests.sh
```

## Troubleshooting

### Port already in use

Start the server on another port:

```powershell
.\build\backend\page_replacement_server.exe 9090
```

The bundled frontend defaults to port `8080`, so update the frontend API base URL or use the server's default port when loading the dashboard.

### `cmake` or compiler not found

Install CMake, Ninja, and a C++17 toolchain, then open a new terminal so they are on `PATH`. On Windows, `run_backend.bat` and `run_tests.bat` also prepend `C:\tools\w64devkit\bin` when that directory exists.

### API tests cannot connect

The Python test does not start the server. Launch `page_replacement_server` first, verify [http://127.0.0.1:8080/api/health](http://127.0.0.1:8080/api/health), and then run `tests/test_api_e2e.py`.

### Dashboard shows “Connecting”

Serve the dashboard through the C++ server rather than opening `frontend/index.html` directly. If it is opened from another origin, the bundled client targets `http://127.0.0.1:8080`.

## Further documentation

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the component diagram, simulator invariants, and implementation notes.

---

# Project Report Material

The following sections can be copied into a project report and adapted to the required institutional format.

## 1. Suggested report title

**Design and Implementation of an Interactive Page Replacement Algorithm Simulator Using C++17**

## 2. Abstract

Page replacement is a fundamental virtual-memory operation in which an operating system selects a page to remove when a requested page is not present in physical memory. This project implements and visualizes five page replacement policies: FIFO, LRU, Optimal, LFU, and Second Chance. A C++17 simulation engine performs the calculations, records every frame transition, and exposes the results through a lightweight JSON REST API. A browser-based dashboard consumes the API to provide live animation, algorithm comparison, workload analysis, and an experiment for Belady's Anomaly. The system also includes deterministic workload generation, native unit tests, and Python end-to-end API tests. The resulting platform combines algorithmic correctness with an accessible environment for studying the effect of reference patterns and frame capacity on page-fault performance.

## 3. Problem statement

Manually tracing page replacement algorithms is time-consuming and makes it difficult to compare policies across different workloads. A simulator is needed that:

1. accepts a page-reference string and frame count;
2. calculates hits, faults, replacements, and ratios correctly;
3. exposes intermediate frame states for visualization;
4. compares multiple algorithms under identical conditions; and
5. demonstrates workload-dependent behavior such as Belady's Anomaly.

## 4. Objectives

- Implement five standard page replacement algorithms.
- Preserve deterministic behavior through explicit tie-breaker policies.
- Record a complete state snapshot for every page reference.
- Provide a reusable C++ core and HTTP API.
- Create an interactive dashboard for learning and analysis.
- Validate the implementation with invariants and known textbook examples.

## 5. Scope

### Included

- FIFO, LRU, Optimal, LFU, and Second Chance simulation.
- Frame capacities from 1 to 64 for the single-simulation and comparison APIs.
- Reference strings containing non-negative integer page IDs.
- Generated random, sequential, repeated, locality, high-diversity, and classic Belady workloads.
- JSON API access and static frontend hosting.
- Native and REST-level automated testing.

### Not included

- Real operating-system page faults or physical memory access.
- Concurrent user accounts or server-side persistence.
- Predictive page replacement for real processes.
- Production authentication, authorization, or database storage.

## 6. Methodology

1. **Input:** receive an algorithm, frame count, and page-reference string.
2. **Initialization:** create empty frames represented by `-1` and initialize algorithm-specific state.
3. **Reference processing:** for each page, detect a hit or page fault.
4. **Allocation/replacement:** use a free frame when available; otherwise select a victim according to the policy.
5. **Snapshot creation:** store frame contents, victim, status, explanation, and algorithm metadata.
6. **Metrics:** calculate hits, faults, replacements, free allocations, and percentages.
7. **Presentation:** serialize the result as JSON and render it in the dashboard.
8. **Verification:** check mathematical invariants and known expected results.

## 7. Complexity summary

The implementations prioritize clarity and traceability because every step is retained for visualization.

| Algorithm | Main decision cost per reference | Additional state |
|---|---:|---|
| FIFO | `O(F)` frame lookup | Queue of frame indices |
| LRU | `O(F)` lookup and victim scan | Last-used step per frame |
| Optimal | `O(F × R)` worst-case future scan | Arrival time for ties |
| LFU | `O(F)` lookup and victim scan | Frequency and last-used step |
| Second Chance | `O(F)` worst-case circular scan | Reference bits and clock hand |

`F` is the number of frames and `R` is the reference-string length. The stored step snapshots require `O(R × F)` space, excluding metadata.

## 8. Expected results for the standard examples

### Standard comparison string

```text
Reference string: [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2]
Frames: 3
```

The `Optimal` result must have no more page faults than any other algorithm. The comparison API reports the best practical online algorithm separately from Optimal.

### Belady's Anomaly string

```text
Reference string: [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5]
FIFO with 3 frames: 9 faults
FIFO with 4 frames: 10 faults
```

This demonstrates that increasing the number of frames can increase FIFO page faults. LRU must not show an increase for this stack-algorithm test.

## 9. Report chapter outline

Use the following structure for a full academic report:

1. **Introduction** — virtual memory, paging, and page replacement.
2. **Problem Definition and Objectives** — motivation and project goals.
3. **Background** — FIFO, LRU, Optimal, LFU, Second Chance, and Belady's Anomaly.
4. **Requirements Analysis** — hardware/software requirements and functional requirements.
5. **System Design** — architecture, modules, data flow, and API design.
6. **Implementation** — C++ core, algorithms, JSON serialization, server, and frontend.
7. **Testing and Results** — unit tests, API tests, invariants, screenshots, and comparison tables.
8. **Discussion** — workload behavior, complexity, assumptions, and limitations.
9. **Conclusion and Future Work** — achievements and possible extensions.
10. **References** — Operating Systems textbooks, C++/CMake documentation, and project dependencies.

Recommended figures and tables:

- System architecture diagram from `docs/ARCHITECTURE.md`.
- Screenshot of the dashboard simulation stepper.
- Screenshot of the algorithm comparison chart.
- Screenshot of the Belady's Anomaly experiment.
- Table of faults and hit ratios for each algorithm.
- Table of faults as frame count changes.

---

# Important Source-Code Snippets

These excerpts are shortened for presentation but match the implementation patterns used in the project. For complete code, use the linked source files.

## A. Common algorithm interface

The polymorphic interface lets the simulator select any policy through one API.

```cpp
class IPageReplacementAlgorithm {
public:
    virtual ~IPageReplacementAlgorithm() = default;
    virtual std::string getName() const = 0;
    virtual std::string getFullName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getTieBreakerPolicy() const = 0;
    virtual SimulationResult simulate(
        int frameCount,
        const std::vector<int>& referenceString) = 0;
};
```

Source: [`IPageReplacementAlgorithm.h`](backend/include/algorithms/IPageReplacementAlgorithm.h).

## B. Simulation data model

Every reference produces a `SimulationStep`, while `SimulationResult` aggregates the complete run.

```cpp
struct SimulationStep {
    int stepIndex{0};
    int page{0};
    bool isHit{false};
    std::vector<int> frames;       // -1 means empty
    int victimPage{-1};
    int insertedPage{0};
    int allocatedFrameIndex{-1};
    std::string status;            // HIT, FAULT_ALLOCATE, FAULT_REPLACE
    std::string explanation;
    nlohmann::json algorithmState;
};

struct SimulationResult {
    std::string algorithm;
    int frameCount{0};
    std::vector<int> referenceString;
    int totalReferences{0};
    int pageHits{0};
    int pageFaults{0};
    double hitRatio{0.0};
    double faultRatio{0.0};
    int replacements{0};
    int freeAllocations{0};
    std::vector<SimulationStep> steps;
};
```

Source: [`Models.h`](backend/include/Models.h).

## C. FIFO replacement core

```cpp
std::vector<int> frames(frameCount, -1);
std::queue<int> fifoQueue;

if (std::find(frames.begin(), frames.end(), page) != frames.end()) {
    result.pageHits++;
    step.status = "HIT";
} else {
    result.pageFaults++;
    auto empty = std::find(frames.begin(), frames.end(), -1);

    if (empty != frames.end()) {
        int frame = static_cast<int>(
            std::distance(frames.begin(), empty));
        frames[frame] = page;
        fifoQueue.push(frame);
        result.freeAllocations++;
        step.status = "FAULT_ALLOCATE";
    } else {
        int victimFrame = fifoQueue.front();
        fifoQueue.pop();
        step.victimPage = frames[victimFrame];
        frames[victimFrame] = page;
        fifoQueue.push(victimFrame);
        result.replacements++;
        step.status = "FAULT_REPLACE";
    }
}
```

Source: [`FIFO.cpp`](backend/src/algorithms/FIFO.cpp).

## D. LRU victim selection

```cpp
int lruFrame = 0;
int oldestStep = lastUsedStep[0];

for (int frame = 1; frame < frameCount; ++frame) {
    if (lastUsedStep[frame] < oldestStep) {
        oldestStep = lastUsedStep[frame];
        lruFrame = frame;
    }
}

step.victimPage = frames[lruFrame];
frames[lruFrame] = page;
lastUsedStep[lruFrame] = static_cast<int>(stepIndex);
step.status = "FAULT_REPLACE";
result.replacements++;
```

Source: [`LRU.cpp`](backend/src/algorithms/LRU.cpp).

## E. Second Chance / Clock scan

```cpp
while (true) {
    inspectedFrames.push_back(clockHand);

    if (refBits[clockHand] == 0) {
        victimFrame = clockHand;
        break;
    }

    refBits[clockHand] = 0;
    clockHand = (clockHand + 1) % frameCount;
}

step.victimPage = frames[victimFrame];
frames[victimFrame] = page;
refBits[victimFrame] = 1;
clockHand = (victimFrame + 1) % frameCount;
```

Source: [`SecondChance.cpp`](backend/src/algorithms/SecondChance.cpp).

## F. REST API route

The server validates input, invokes the core simulator, and serializes the result.

```cpp
svr.Post("/api/simulate", [&sim](
    const httplib::Request& req,
    httplib::Response& res) {
    try {
        auto body = nlohmann::json::parse(req.body);
        std::string algorithm = body["algorithm"].get<std::string>();
        int frames = body["frames"].get<int>();
        std::vector<int> references =
            body["referenceString"].get<std::vector<int>>();

        auto result = sim.runSimulation(
            algorithm, frames, references);
        res.set_content(
            nlohmann::json(result).dump(),
            "application/json");
    } catch (const std::exception& error) {
        res.status = 400;
        res.set_content(
            nlohmann::json{
                {"error", std::string("Simulation failed: ")
                          + error.what()}
            }.dump(),
            "application/json");
    }
});
```

Source: [`main.cpp`](backend/src/main.cpp).

## G. Workload generation

The generator accepts an optional seed so a report can reproduce an experiment.

```cpp
std::mt19937 rng(actualSeed);
std::vector<int> result;
result.reserve(count);

std::uniform_int_distribution<int> distribution(0, maxPageId);
for (int i = 0; i < count; ++i) {
    result.push_back(distribution(rng));
}
return result;
```

Source: [`WorkloadGenerator.cpp`](backend/src/WorkloadGenerator.cpp).

## H. Frontend API call

The dashboard sends JSON to the backend using the browser Fetch API.

```javascript
async function runSimulation(algorithm, frames, referenceString) {
  const response = await fetch('/api/simulate', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    },
    body: JSON.stringify({ algorithm, frames, referenceString })
  });

  if (!response.ok) {
    throw new Error(`Server error (${response.status})`);
  }
  return await response.json();
}
```

Source: [`api.js`](frontend/js/api.js).

## I. Automated invariant verification

The tests confirm that the simulation is internally consistent, not just that it returns a response.

```cpp
assert(result.pageHits + result.pageFaults
       == result.totalReferences);
assert(result.replacements + result.freeAllocations
       == result.pageFaults);
assert(result.steps.size()
       == result.referenceString.size());
```

The Python end-to-end test also checks all five algorithms and verifies the classic FIFO result:

```python
belady = post("/api/belady-experiment", {
    "frame1": 3,
    "frame2": 4,
})
assert belady["anomalyOccurred"] is True
assert belady["fifoResult1"]["pageFaults"] == 9
assert belady["fifoResult2"]["pageFaults"] == 10
```

Sources: [`test_algorithms.cpp`](backend/tests/test_algorithms.cpp) and [`test_api_e2e.py`](tests/test_api_e2e.py).

## 11. Conclusion template

This project successfully implements an interactive and testable page replacement simulator. The modular C++ design supports multiple algorithms through a common interface, while the REST API and frontend make the results easy to inspect and compare. The recorded snapshots make each replacement decision transparent, which is useful for both classroom demonstration and experimental analysis. The test suite verifies core accounting invariants and textbook behavior such as FIFO's Belady's Anomaly. Future work could add additional policies such as Random, MRU, and NRU, persistent experiment reports, user-defined workload files, performance benchmarking, and accessibility improvements.
