#pragma once

#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

namespace simulator {

struct SimulationStep {
    int stepIndex{0};
    int page{0};
    bool isHit{false};
    std::vector<int> frames;        // -1 represents empty
    int victimPage{-1};             // -1 if none
    int insertedPage{0};
    int allocatedFrameIndex{-1};
    std::string status;             // "HIT", "FAULT_ALLOCATE", "FAULT_REPLACE"
    std::string explanation;
    nlohmann::json algorithmState;  // Clock hand, frequencies, queue, etc.
};

struct SimulationResult {
    std::string algorithm;
    std::string algorithmFullName;
    int frameCount{0};
    std::vector<int> referenceString;
    int totalReferences{0};
    int pageHits{0};
    int pageFaults{0};
    double hitRatio{0.0};       // Percentage (0.0 to 100.0)
    double faultRatio{0.0};     // Percentage (0.0 to 100.0)
    int replacements{0};
    int freeAllocations{0};
    std::vector<SimulationStep> steps;
    std::string tieBreakerPolicy;
    std::string description;
};

struct AlgorithmComparisonResult {
    int frameCount{0};
    std::vector<int> referenceString;
    std::vector<SimulationResult> results;
    std::string bestAlgorithm;
    int lowestFaults{0};
    std::string theoreticalNote;
};

struct WorkloadAnalysisPoint {
    int frameCount{0};
    std::map<std::string, int> algorithmFaults;
    std::map<std::string, double> algorithmHitRatios;
};

struct WorkloadAnalysisResult {
    std::string workloadType;
    std::vector<int> referenceString;
    std::vector<int> frameCountsTested;
    std::vector<WorkloadAnalysisPoint> dataPoints;
    std::vector<SimulationResult> detailedResultsAtDefaultFrames;
    std::string analysisSummary;
};

// JSON conversions
inline void to_json(nlohmann::json& j, const SimulationStep& s) {
    j = nlohmann::json{
        {"stepIndex", s.stepIndex},
        {"page", s.page},
        {"isHit", s.isHit},
        {"frames", s.frames},
        {"victimPage", s.victimPage},
        {"insertedPage", s.insertedPage},
        {"allocatedFrameIndex", s.allocatedFrameIndex},
        {"status", s.status},
        {"explanation", s.explanation},
        {"algorithmState", s.algorithmState}
    };
}

inline void to_json(nlohmann::json& j, const SimulationResult& r) {
    j = nlohmann::json{
        {"algorithm", r.algorithm},
        {"algorithmFullName", r.algorithmFullName},
        {"frameCount", r.frameCount},
        {"referenceString", r.referenceString},
        {"totalReferences", r.totalReferences},
        {"pageHits", r.pageHits},
        {"pageFaults", r.pageFaults},
        {"hitRatio", r.hitRatio},
        {"faultRatio", r.faultRatio},
        {"replacements", r.replacements},
        {"freeAllocations", r.freeAllocations},
        {"steps", r.steps},
        {"tieBreakerPolicy", r.tieBreakerPolicy},
        {"description", r.description}
    };
}

inline void to_json(nlohmann::json& j, const AlgorithmComparisonResult& c) {
    j = nlohmann::json{
        {"frameCount", c.frameCount},
        {"referenceString", c.referenceString},
        {"results", c.results},
        {"bestAlgorithm", c.bestAlgorithm},
        {"lowestFaults", c.lowestFaults},
        {"theoreticalNote", c.theoreticalNote}
    };
}

inline void to_json(nlohmann::json& j, const WorkloadAnalysisPoint& p) {
    j = nlohmann::json{
        {"frameCount", p.frameCount},
        {"algorithmFaults", p.algorithmFaults},
        {"algorithmHitRatios", p.algorithmHitRatios}
    };
}

inline void to_json(nlohmann::json& j, const WorkloadAnalysisResult& w) {
    j = nlohmann::json{
        {"workloadType", w.workloadType},
        {"referenceString", w.referenceString},
        {"frameCountsTested", w.frameCountsTested},
        {"dataPoints", w.dataPoints},
        {"detailedResultsAtDefaultFrames", w.detailedResultsAtDefaultFrames},
        {"analysisSummary", w.analysisSummary}
    };
}

} // namespace simulator
