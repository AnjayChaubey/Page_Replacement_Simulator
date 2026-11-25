#pragma once

#include "IPageReplacementAlgorithm.h"

namespace simulator {

class SecondChance : public IPageReplacementAlgorithm {
public:
    std::string getName() const override { return "SecondChance"; }
    std::string getFullName() const override { return "Second Chance (Clock)"; }
    std::string getDescription() const override {
        return "Circular page replacement policy using a reference bit for each frame. When a replacement is required, the clock hand scans circularly: if a page has reference bit 1, it is given a second chance (bit cleared to 0); if bit is 0, it is evicted.";
    }
    std::string getTieBreakerPolicy() const override {
        return "Deterministic circular pointer traversal (clock hand advances sequentially modulo frame count).";
    }

    SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) override;
};

} // namespace simulator
