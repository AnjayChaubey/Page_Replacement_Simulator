#pragma once

#include "IPageReplacementAlgorithm.h"

namespace simulator {

class FIFO : public IPageReplacementAlgorithm {
public:
    std::string getName() const override { return "FIFO"; }
    std::string getFullName() const override { return "First-In, First-Out"; }
    std::string getDescription() const override {
        return "Replaces the page that has been in memory the longest (oldest arrival). Simple to implement using a FIFO queue, but vulnerable to Belady's Anomaly.";
    }
    std::string getTieBreakerPolicy() const override {
        return "Strict queue ordering based on arrival timestamp.";
    }

    SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) override;
};

} // namespace simulator
