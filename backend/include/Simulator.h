#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "Models.h"
#include "algorithms/IPageReplacementAlgorithm.h"

namespace simulator {

class Simulator {
public:
    Simulator();
    ~Simulator() = default;

    std::unique_ptr<IPageReplacementAlgorithm> createAlgorithm(const std::string& name) const;
    std::vector<std::string> getSupportedAlgorithms() const;

    SimulationResult runSimulation(const std::string& algorithmName, int frameCount, const std::vector<int>& referenceString);
    AlgorithmComparisonResult runComparison(int frameCount, const std::vector<int>& referenceString);
    WorkloadAnalysisResult runWorkloadAnalysis(const std::string& workloadType, const std::vector<int>& referenceString, const std::vector<int>& frameCounts);
};

} // namespace simulator
