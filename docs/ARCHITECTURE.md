# Page Replacement Simulator - System Architecture & Engineering Specifications

## 1. System Overview

The **Page Replacement Simulator** is an Operating Systems educational and benchmarking platform designed to simulate, analyze, and visualize memory management page replacement policies under diverse workload distributions.

The project follows a clean separation of concerns:
- **Core Engine (C++17)**: High-performance, mathematically rigorous implementations of memory replacement algorithms (`FIFO`, `LRU`, `Optimal`, `LFU`, `Second Chance`).
- **RESTful API Service**: A lightweight C++ HTTP server built using `cpp-httplib` and `nlohmann/json`, serving both JSON API endpoints and static frontend assets with full CORS support.
- **Frontend Dashboard**: A responsive, dark-mode single page application (SPA) built with Vanilla JavaScript, CSS3 variables, Chart.js visualizations, and Lucide icons.

---

## 2. Architecture Diagram

```
+---------------------------------------------------------------------------------+
|                               WEB BROWSER (UI)                                  |
|  +--------------------+  +----------------------+  +-------------------------+  |
|  | Dashboard / Stats  |  | Live Frame Stepper   |  | Multi-Algorithm Compare |  |
|  +--------------------+  +----------------------+  +-------------------------+  |
|  | Workload Analysis  |  | Belady's Anomaly Lab |  | Learning Center / Guide |  |
|  +--------------------+  +----------------------+  +-------------------------+  |
|         ^                           ^                           ^               |
|         |                           |                           |               |
|         +---------------------------+---------------------------+               |
|                                     | HTTP REST (JSON)                          |
+-------------------------------------|-------------------------------------------+
                                      v
+---------------------------------------------------------------------------------+
|                         C++17 EMBEDDED HTTP SERVER                              |
|  (cpp-httplib server listening on 0.0.0.0:8080, serving /api/* & static assets)  |
|  +---------------------------------------------------------------------------+  |
|  | Routing: /api/simulate, /api/compare, /api/generate-workload, etc.        |  |
|  +---------------------------------------------------------------------------+  |
|                                     |                                           |
|                                     v                                           |
|  +---------------------------------------------------------------------------+  |
|  |                            Simulator Core                                 |  |
|  |   - WorkloadGenerator (random, sequential, repeated, locality, diversity) |  |
|  |   - Invariant Verifier & Snapshot Generator                               |  |
|  +---------------------------------------------------------------------------+  |
|                                     |                                           |
|         +---------------------------+---------------------------+               |
|         |                           |                           |               |
|         v                           v                           v               |
|   +-----------+               +-----------+               +-----------+         |
|   |   FIFO    |               |    LRU    |               |  Optimal  |         |
|   +-----------+               +-----------+               +-----------+         |
|         |                           |                           |               |
|         v                           v                           |               |
|   +-----------+               +-----------+                     |               |
|   |    LFU    |               |2nd Chance |                     |               |
|   +-----------+               +-----------+                     |               |
+---------------------------------------------------------------------------------+
```

---

## 3. Algorithm Implementation Invariants

Every algorithm adheres to the standard `IPageReplacementAlgorithm` interface:

```cpp
class IPageReplacementAlgorithm {
public:
    virtual ~IPageReplacementAlgorithm() = default;
    virtual std::string getName() const = 0;
    virtual std::string getFullName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getTieBreakerPolicy() const = 0;
    virtual SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) = 0;
};
```

### Invariants Maintained:
1. **Total Integrity**: $\text{Hits} + \text{Faults} = \text{Total References}$.
2. **Allocation Integrity**: $\text{Replacements} + \text{Free Allocations} = \text{Total Faults}$.
3. **No-Change on Hit**: On a page hit, the set of resident pages remains strictly identical.
4. **No Duplicate Frames**: A valid page ID appears at most once among the allocated frames.
5. **State Restoration**: Each snapshot completely captures memory frames, victim pages, reference bits, frequency counters, and clock hand positions, enabling flawless bi-directional stepping.

---

## 4. Workload Generators

| Workload Type | Access Distribution & Characteristics |
|---|---|
| **Locality-Based** | 80% of references stay within a localized cluster (temporal/spatial locality) with 20% phase transitions. |
| **Sequential** | Monotonically ascending page IDs looping over a modulus boundary (streaming patterns). |
| **Repeated Access** | High frequency references to a compact working set (3-4 pages) with occasional external page noise. |
| **High Diversity** | Uniform random distribution across a broad page range ($[0, 20]$), stressing frame capacity. |
| **Belady Classic** | The canonical 12-page sequence `1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5`. |
