#pragma once

#include "IPageReplacementAlgorithm.h"

namespace simulator {

class LRU : public IPageReplacementAlgorithm {
public:
    std::string getName() const override { return "LRU"; }
    std::string getFullName() const override { return "Least Recently Used"; }
    std::string getDescription() const override {
        return "Replaces the page that has not been referenced for the longest period of time. Approximates Optimal by leveraging temporal locality, and belongs to the family of stack algorithms (free from Belady's Anomaly).";
    }
    std::string getTieBreakerPolicy() const override {
        return "Unique monotonic access timestamp (step index); earliest arrival breaks initial frame allocation ties.";
    }

    SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) override;
};

} // namespace simulator
