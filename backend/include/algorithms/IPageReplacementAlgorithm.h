#pragma once

#include <string>
#include <vector>
#include "../Models.h"

namespace simulator {

class IPageReplacementAlgorithm {
public:
    virtual ~IPageReplacementAlgorithm() = default;
    virtual std::string getName() const = 0;
    virtual std::string getFullName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getTieBreakerPolicy() const = 0;
    virtual SimulationResult simulate(int frameCount, const std::vector<int>& referenceString) = 0;
};

} // namespace simulator
