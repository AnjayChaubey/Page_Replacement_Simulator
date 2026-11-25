#include "../include/WorkloadGenerator.h"
#include <random>
#include <chrono>
#include <algorithm>

namespace simulator {

std::vector<int> WorkloadGenerator::getClassicBeladyString() {
    return {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
}

std::vector<int> WorkloadGenerator::generate(const std::string& type, int count, int maxPageId, std::optional<unsigned int> seed) {
    if (count <= 0) return {};
    if (maxPageId < 0) maxPageId = 7;

    unsigned int actualSeed = seed.has_value() 
        ? seed.value() 
        : static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());

    std::mt19937 rng(actualSeed);
    std::vector<int> result;
    result.reserve(count);

    if (type == "belady_classic") {
        std::vector<int> classic = getClassicBeladyString();
        while (static_cast<int>(result.size()) < count) {
            for (int p : classic) {
                if (static_cast<int>(result.size()) >= count) break;
                result.push_back(p);
            }
        }
        return result;
    }

    if (type == "sequential") {
        int current = 0;
        int limit = std::max(1, maxPageId + 1);
        for (int i = 0; i < count; ++i) {
            result.push_back(current);
            current = (current + 1) % limit;
        }
        return result;
    }

    if (type == "repeated") {
        // Repeated access to small subset of 3-4 pages, with 10% chance of random access
        int workingSetSize = std::min(4, std::max(2, maxPageId + 1));
        std::uniform_int_distribution<int> wsDist(0, workingSetSize - 1);
        std::uniform_int_distribution<int> allDist(0, maxPageId);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        for (int i = 0; i < count; ++i) {
            if (probDist(rng) < 0.85) {
                result.push_back(wsDist(rng));
            } else {
                result.push_back(allDist(rng));
            }
        }
        return result;
    }

    if (type == "locality") {
        // Temporal & Spatial Locality (phase transitions)
        // 80% accesses stay within current local window [clusterBase, clusterBase + window]
        // 20% shift cluster or access outside
        int windowSize = std::max(2, std::min(4, (maxPageId + 1) / 2));
        int clusterBase = 0;
        std::uniform_real_distribution<double> probDist(0.0, 1.0);
        std::uniform_int_distribution<int> allDist(0, maxPageId);

        for (int i = 0; i < count; ++i) {
            if (i > 0 && i % (count / 3 + 1) == 0) {
                // Phase shift to new cluster
                clusterBase = allDist(rng);
            }

            if (probDist(rng) < 0.80) {
                std::uniform_int_distribution<int> localDist(0, windowSize - 1);
                int p = (clusterBase + localDist(rng)) % (maxPageId + 1);
                result.push_back(p);
            } else {
                result.push_back(allDist(rng));
            }
        }
        return result;
    }

    if (type == "high_diversity") {
        // Large page space uniformly distributed
        int effectiveMax = std::max(maxPageId, 15);
        std::uniform_int_distribution<int> dist(0, effectiveMax);
        for (int i = 0; i < count; ++i) {
            result.push_back(dist(rng));
        }
        return result;
    }

    // Default: Uniform Random
    std::uniform_int_distribution<int> dist(0, maxPageId);
    for (int i = 0; i < count; ++i) {
        result.push_back(dist(rng));
    }
    return result;
}

} // namespace simulator
