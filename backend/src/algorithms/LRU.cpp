#include "../../include/algorithms/LRU.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace simulator {

SimulationResult LRU::simulate(int frameCount, const std::vector<int>& referenceString) {
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
    std::unordered_map<int, int> lastUsedStep; // frameIndex -> last used step

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
            lastUsedStep[frameIdx] = static_cast<int>(step);
            s.isHit = true;
            s.allocatedFrameIndex = frameIdx;
            s.status = "HIT";
            result.pageHits++;

            std::ostringstream oss;
            oss << "Page " << page << " found in Frame " << frameIdx << " (Page Hit). Updated last used step to " << step << ".";
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
                lastUsedStep[frameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = frameIdx;
                s.status = "FAULT_ALLOCATE";
                result.freeAllocations++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Allocated to free Frame " << frameIdx 
                    << " with last used step " << step << ".";
                s.explanation = oss.str();
            } else {
                // Full memory -> replace least recently used frame
                int lruFrameIdx = 0;
                int minStep = lastUsedStep[0];

                for (int f = 1; f < frameCount; ++f) {
                    if (lastUsedStep[f] < minStep) {
                        minStep = lastUsedStep[f];
                        lruFrameIdx = f;
                    }
                }

                s.victimPage = frames[lruFrameIdx];
                frames[lruFrameIdx] = page;
                lastUsedStep[lruFrameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = lruFrameIdx;
                s.status = "FAULT_REPLACE";
                result.replacements++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). LRU page " << s.victimPage 
                    << " in Frame " << lruFrameIdx << " (last used at step " << minStep 
                    << ") replaced by page " << page << ".";
                s.explanation = oss.str();
            }
        }

        s.frames = frames;

        // Build metadata
        nlohmann::json meta;
        std::vector<nlohmann::json> frameStates;
        int nextLruIdx = -1;
        int oldest = 1e9;

        for (int f = 0; f < frameCount; ++f) {
            nlohmann::json fInfo;
            fInfo["frameIndex"] = f;
            fInfo["page"] = frames[f];
            if (frames[f] != -1) {
                int lu = lastUsedStep[f];
                fInfo["lastUsedStep"] = lu;
                fInfo["idleSteps"] = static_cast<int>(step) - lu;
                if (lu < oldest) {
                    oldest = lu;
                    nextLruIdx = f;
                }
            } else {
                fInfo["lastUsedStep"] = -1;
                fInfo["idleSteps"] = -1;
            }
            frameStates.push_back(fInfo);
        }

        meta["frameStats"] = frameStates;
        meta["nextVictimFrame"] = nextLruIdx;
        meta["nextVictimPage"] = (nextLruIdx >= 0) ? frames[nextLruIdx] : -1;
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
