#include "../include/Simulator.h"
#include "../include/algorithms/FIFO.h"
#include "../include/algorithms/LRU.h"
#include "../include/algorithms/Optimal.h"
#include "../include/algorithms/LFU.h"
#include "../include/algorithms/SecondChance.h"

#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace simulator {

Simulator::Simulator() = default;

std::vector<std::string> Simulator::getSupportedAlgorithms() const {
    return {"FIFO", "LRU", "Optimal", "LFU", "SecondChance"};
}

std::unique_ptr<IPageReplacementAlgorithm> Simulator::createAlgorithm(const std::string& name) const {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "fifo") {
        return std::make_unique<FIFO>();
    } else if (lower == "lru") {
        return std::make_unique<LRU>();
    } else if (lower == "opt" || lower == "optimal") {
        return std::make_unique<Optimal>();
    } else if (lower == "lfu") {
        return std::make_unique<LFU>();
    } else if (lower == "secondchance" || lower == "second_chance" || lower == "clock") {
        return std::make_unique<SecondChance>();
    }

    throw std::invalid_argument("Unknown page replacement algorithm: " + name);
}

SimulationResult Simulator::runSimulation(const std::string& algorithmName, int frameCount, const std::vector<int>& referenceString) {
    auto algo = createAlgorithm(algorithmName);
    return algo->simulate(frameCount, referenceString);
}

AlgorithmComparisonResult Simulator::runComparison(int frameCount, const std::vector<int>& referenceString) {
    AlgorithmComparisonResult comp;
    comp.frameCount = frameCount;
    comp.referenceString = referenceString;

    std::vector<std::string> algos = getSupportedAlgorithms();
    int minFaultsPractical = 1e9;
    std::string bestPracticalAlgo = "LRU";

    for (const auto& name : algos) {
        auto algo = createAlgorithm(name);
        SimulationResult r = algo->simulate(frameCount, referenceString);
        comp.results.push_back(r);

        // Optimal is a theoretical benchmark with future knowledge.
        // Compare practical online algorithms among themselves.
        if (name != "Optimal") {
            if (r.pageFaults < minFaultsPractical) {
                minFaultsPractical = r.pageFaults;
                bestPracticalAlgo = name;
            }
        }
    }

    comp.bestAlgorithm = bestPracticalAlgo;
    comp.lowestFaults = minFaultsPractical;
    comp.theoreticalNote = "Optimal (MIN) algorithm serves as the theoretical minimum bound for page faults using non-causal future knowledge. Among practical online algorithms, " + bestPracticalAlgo + " achieved the lowest page fault count in this test case.";

    return comp;
}

WorkloadAnalysisResult Simulator::runWorkloadAnalysis(const std::string& workloadType, const std::vector<int>& referenceString, const std::vector<int>& frameCounts) {
    WorkloadAnalysisResult analysis;
    analysis.workloadType = workloadType;
    analysis.referenceString = referenceString;
    analysis.frameCountsTested = frameCounts;

    std::vector<std::string> algos = getSupportedAlgorithms();
    int defaultFrames = frameCounts.empty() ? 3 : frameCounts[frameCounts.size() / 2];

    for (int frames : frameCounts) {
        WorkloadAnalysisPoint pt;
        pt.frameCount = frames;

        for (const auto& name : algos) {
            auto algo = createAlgorithm(name);
            SimulationResult r = algo->simulate(frames, referenceString);
            pt.algorithmFaults[name] = r.pageFaults;
            pt.algorithmHitRatios[name] = r.hitRatio;

            if (frames == defaultFrames) {
                analysis.detailedResultsAtDefaultFrames.push_back(r);
            }
        }
        analysis.dataPoints.push_back(pt);
    }

    std::ostringstream summary;
    summary << "Evaluated " << algos.size() << " algorithms across " 
            << frameCounts.size() << " frame configurations under '" << workloadType << "' workload. ";
    if (workloadType == "locality") {
        summary << "LRU and Second Chance thrive on strong temporal and spatial locality, matching or nearing the Optimal benchmark.";
    } else if (workloadType == "sequential") {
        summary << "Sequential streaming workloads exhibit low reuse where frame capacity expansion yields gradual gains.";
    } else if (workloadType == "repeated") {
        summary << "Repeated access patterns benefit significantly once the frame capacity matches the active working set size.";
    } else {
        summary << "Optimal consistently outlines the theoretical lower bound of page faults across all tested frame capacities.";
    }

    analysis.analysisSummary = summary.str();
    return analysis;
}

} // namespace simulator
