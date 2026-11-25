#include "../../include/algorithms/FIFO.h"
#include <queue>
#include <algorithm>
#include <sstream>

namespace simulator {

SimulationResult FIFO::simulate(int frameCount, const std::vector<int>& referenceString) {
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
    std::queue<int> fifoQueue; // stores frame indices in order of arrival

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
            s.isHit = true;
            s.allocatedFrameIndex = static_cast<int>(std::distance(frames.begin(), it));
            s.status = "HIT";
            result.pageHits++;

            std::ostringstream oss;
            oss << "Page " << page << " is already present in Frame " << s.allocatedFrameIndex << " (Page Hit). FIFO queue remains unchanged.";
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
                fifoQueue.push(frameIdx);
                s.allocatedFrameIndex = frameIdx;
                s.status = "FAULT_ALLOCATE";
                result.freeAllocations++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Allocated to empty Frame " << frameIdx << ".";
                s.explanation = oss.str();
            } else {
                // Full memory -> replace oldest
                int victimFrameIdx = fifoQueue.front();
                fifoQueue.pop();

                s.victimPage = frames[victimFrameIdx];
                frames[victimFrameIdx] = page;
                fifoQueue.push(victimFrameIdx);
                s.allocatedFrameIndex = victimFrameIdx;
                s.status = "FAULT_REPLACE";
                result.replacements++;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Memory full. Oldest page " 
                    << s.victimPage << " in Frame " << victimFrameIdx << " replaced by page " << page << ".";
                s.explanation = oss.str();
            }
        }

        s.frames = frames;

        // Build metadata for frontend inspection
        std::vector<int> queueOrderFrames;
        std::queue<int> tempQueue = fifoQueue;
        while (!tempQueue.empty()) {
            queueOrderFrames.push_back(tempQueue.front());
            tempQueue.pop();
        }

        nlohmann::json meta;
        meta["queueOrder"] = queueOrderFrames;
        meta["nextVictimFrame"] = queueOrderFrames.empty() ? -1 : queueOrderFrames.front();
        meta["nextVictimPage"] = (queueOrderFrames.empty() || queueOrderFrames.front() < 0) ? -1 : frames[queueOrderFrames.front()];
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
