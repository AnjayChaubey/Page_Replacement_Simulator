#pragma once

#include "IPageReplacementAlgorithm.h"

namespace simulator {

class LFU : public IPageReplacementAlgorithm {
public:
    std::string getName() const override { return "LFU"; }
    std::string getFullName() const override { return "Least Frequently Used"; }
    std::string getDescription() const override {
        return "Replaces the page with the lowest access frequency counter. Uses Least Recently Used (LRU) as a deterministic tie-breaker when multiple pages share the same minimum frequency.";
    }
    std::string getTieBreakerPolicy() const override {
        return "Deterministic LRU tie-breaker: among pages with the identical lowest frequency, the one not accessed for the longest time is evicted.";
    }

    SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) override;
};

} // namespace simulator
