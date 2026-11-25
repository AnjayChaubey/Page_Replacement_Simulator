#include "../../include/algorithms/LFU.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace simulator {

SimulationResult LFU::simulate(int frameCount, const std::vector<int>& referenceString) {
    SimulationResult result;
    result.algorithm = getName();
    result.algorithmFullName = getFullName();
    result.description = getDescription();
    result.tieBreakerPolicy = getTieBreakerPolicy();
    result.frameCount = frameCount;
    result.referenceString = referenceString;
    result.totalReferences = static_cast<int>(referenceString.size());
    result.pageHits = 0;
    result.pageFaults = 0;
    result.replacements = 0;
    result.freeAllocations = 0;

    if (frameCount <= 0 || referenceString.empty()) {
        result.hitRatio = 0.0;
        result.faultRatio = 0.0;
        return result;
    }

    std::vector<int> frames(frameCount, -1);
    std::vector<int> frequencies(frameCount, 0);
    std::vector<int> lastUsedStep(frameCount, -1);

    for (size_t step = 0; step < referenceString.size(); ++step) {
        int page = referenceString[step];
        SimulationStep s;
        s.stepIndex = static_cast<int>(step);
        s.page = page;
        s.insertedPage = page;
        s.victimPage = -1;

        // Check for hit
        auto it = std::find(frames.begin(), frames.end(), page);
        if (it != frames.end()) {
            // HIT
            int frameIdx = static_cast<int>(std::distance(frames.begin(), it));
            frequencies[frameIdx]++;
            lastUsedStep[frameIdx] = static_cast<int>(step);
            s.isHit = true;
            s.allocatedFrameIndex = frameIdx;
            s.status = "HIT";
            result.pageHits++;

            std::ostringstream oss;
            oss << "Page " << page << " found in Frame " << frameIdx << " (Page Hit). Frequency incremented to " 
                << frequencies[frameIdx] << "; last used updated to step " << step << ".";
            s.explanation = oss.str();
        } else {
            // PAGE FAULT
            s.isHit = false;
            result.pageFaults++;

            // Check if there is an empty frame
            auto emptyIt = std::find(frames.begin(), frames.end(), -1);
            if (emptyIt != frames.end()) {
                // Free allocation
                int frameIdx = static_cast<int>(std::distance(frames.begin(), emptyIt));
                frames[frameIdx] = page;
                frequencies[frameIdx] = 1;
                lastUsedStep[frameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = frameIdx;
                s.status = "FAULT_ALLOCATE";
                result.freeAllocations++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Allocated to empty Frame " << frameIdx 
                    << " with initial frequency 1.";
                s.explanation = oss.str();
            } else {
                // Full memory -> replace least frequently used with LRU tie-breaker
                int victimFrameIdx = 0;
                int minFreq = frequencies[0];
                int minLastUsed = lastUsedStep[0];

                for (int f = 1; f < frameCount; ++f) {
                    if (frequencies[f] < minFreq) {
                        minFreq = frequencies[f];
                        minLastUsed = lastUsedStep[f];
                        victimFrameIdx = f;
                    } else if (frequencies[f] == minFreq) {
                        // Tie-breaker: LRU
                        if (lastUsedStep[f] < minLastUsed) {
                            minLastUsed = lastUsedStep[f];
                            victimFrameIdx = f;
                        }
                    }
                }

                s.victimPage = frames[victimFrameIdx];
                int evictedFreq = frequencies[victimFrameIdx];
                frames[victimFrameIdx] = page;
                frequencies[victimFrameIdx] = 1;
                lastUsedStep[victimFrameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = victimFrameIdx;
                s.status = "FAULT_REPLACE";
                result.replacements++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Replaced page " << s.victimPage 
                    << " in Frame " << victimFrameIdx << " (frequency=" << evictedFreq 
                    << ", tie-breaker LRU step=" << minLastUsed << ") with frequency reset to 1.";
                s.explanation = oss.str();
            }
        }

        s.frames = frames;

        // Build metadata for frontend
        nlohmann::json meta;
        std::vector<nlohmann::json> frameStates;
        int nextVictim = -1;
        int lowestF = 1e9;
        int oldestT = 1e9;

        for (int f = 0; f < frameCount; ++f) {
            nlohmann::json fInfo;
            fInfo["frameIndex"] = f;
            fInfo["page"] = frames[f];
            fInfo["frequency"] = (frames[f] != -1) ? frequencies[f] : 0;
            fInfo["lastUsedStep"] = (frames[f] != -1) ? lastUsedStep[f] : -1;

            if (frames[f] != -1) {
                if (frequencies[f] < lowestF) {
                    lowestF = frequencies[f];
                    oldestT = lastUsedStep[f];
                    nextVictim = f;
                } else if (frequencies[f] == lowestF) {
                    if (lastUsedStep[f] < oldestT) {
                        oldestT = lastUsedStep[f];
                        nextVictim = f;
                    }
                }
            }
            frameStates.push_back(fInfo);
        }

        meta["frameStats"] = frameStates;
        meta["nextVictimFrame"] = nextVictim;
        meta["nextVictimPage"] = (nextVictim >= 0) ? frames[nextVictim] : -1;
        s.algorithmState = meta;

        result.steps.push_back(s);
    }

    if (result.totalReferences > 0) {
        result.hitRatio = (static_cast<double>(result.pageHits) / result.totalReferences) * 100.0;
        result.faultRatio = (static_cast<double>(result.pageFaults) / result.totalReferences) * 100.0;
    } else {
        result.hitRatio = 0.0;
        result.faultRatio = 0.0;
    }

    return result;
}

} // namespace simulator
