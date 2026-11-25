#pragma once

#include "IPageReplacementAlgorithm.h"

namespace simulator {

class Optimal : public IPageReplacementAlgorithm {
public:
    std::string getName() const override { return "Optimal"; }
    std::string getFullName() const override { return "Optimal Page Replacement (Belady's MIN)"; }
    std::string getDescription() const override {
        return "Replaces the page whose next reference occurs farthest in the future (or never). Serves as an unachievable theoretical lower bound for page faults since general-purpose OSes cannot predict future memory references.";
    }
    std::string getTieBreakerPolicy() const override {
        return "Pages never referenced again are prioritized for eviction, with ties broken by FIFO arrival order.";
    }

    SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) override;
};

} // namespace simulator
