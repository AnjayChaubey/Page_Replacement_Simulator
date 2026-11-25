#include "../../include/algorithms/Optimal.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace simulator {

SimulationResult Optimal::simulate(int frameCount, const std::vector<int>& referenceString) {
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
    std::vector<int> arrivalTimes(frameCount, -1); // for deterministic tie-breaking

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
            s.isHit = true;
            s.allocatedFrameIndex = frameIdx;
            s.status = "HIT";
            result.pageHits++;

            std::ostringstream oss;
            oss << "Page " << page << " found in Frame " << frameIdx << " (Page Hit).";
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
                arrivalTimes[frameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = frameIdx;
                s.status = "FAULT_ALLOCATE";
                result.freeAllocations++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Allocated to free Frame " << frameIdx << ".";
                s.explanation = oss.str();
            } else {
                // Full memory -> find frame with farthest next reference
                int victimFrameIdx = -1;
                int farthestNextUse = -1;
                int earliestArrivalForTie = 1e9;

                for (int f = 0; f < frameCount; ++f) {
                    int p = frames[f];
                    int nextUse = 1e9; // infinity if never used again

                    for (size_t future = step + 1; future < referenceString.size(); ++future) {
                        if (referenceString[future] == p) {
                            nextUse = static_cast<int>(future);
                            break;
                        }
                    }

                    if (nextUse > farthestNextUse) {
                        farthestNextUse = nextUse;
                        victimFrameIdx = f;
                        earliestArrivalForTie = arrivalTimes[f];
                    } else if (nextUse == farthestNextUse) {
                        // Tie-breaking: oldest arrival (FIFO)
                        if (arrivalTimes[f] < earliestArrivalForTie) {
                            earliestArrivalForTie = arrivalTimes[f];
                            victimFrameIdx = f;
                        }
                    }
                }

                s.victimPage = frames[victimFrameIdx];
                frames[victimFrameIdx] = page;
                arrivalTimes[victimFrameIdx] = static_cast<int>(step);
                s.allocatedFrameIndex = victimFrameIdx;
                s.status = "FAULT_REPLACE";
                result.replacements++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Replaced page " << s.victimPage 
                    << " in Frame " << victimFrameIdx << " because its next access is ";
                if (farthestNextUse >= 1e8) {
                    oss << "never in the remaining sequence.";
                } else {
                    oss << "farthest in future (step " << farthestNextUse << ").";
                }
                s.explanation = oss.str();
            }
        }

        s.frames = frames;

        // Build metadata for frontend
        nlohmann::json meta;
        std::vector<nlohmann::json> frameStates;
        for (int f = 0; f < frameCount; ++f) {
            nlohmann::json fInfo;
            fInfo["frameIndex"] = f;
            fInfo["page"] = frames[f];
            if (frames[f] != -1) {
                int nextUse = -1;
                for (size_t future = step + 1; future < referenceString.size(); ++future) {
                    if (referenceString[future] == frames[f]) {
                        nextUse = static_cast<int>(future);
                        break;
                    }
                }
                fInfo["nextUseStep"] = nextUse;
                fInfo["neverUsedAgain"] = (nextUse == -1);
            } else {
                fInfo["nextUseStep"] = -1;
                fInfo["neverUsedAgain"] = false;
            }
            frameStates.push_back(fInfo);
        }
        meta["frameStats"] = frameStates;
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
