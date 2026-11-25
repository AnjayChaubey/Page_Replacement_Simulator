#include "../../include/algorithms/SecondChance.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace simulator {

SimulationResult SecondChance::simulate(int frameCount, const std::vector<int>& referenceString) {
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
    std::vector<int> refBits(frameCount, 0);
    int clockHand = 0;

    for (size_t step = 0; step < referenceString.size(); ++step) {
        int page = referenceString[step];
        SimulationStep s;
        s.stepIndex = static_cast<int>(step);
        s.page = page;
        s.insertedPage = page;
        s.victimPage = -1;

        int handBeforeStep = clockHand;
        std::vector<int> inspectedFrames;

        // Check for hit
        auto it = std::find(frames.begin(), frames.end(), page);
        if (it != frames.end()) {
            // HIT
            int hitFrameIdx = static_cast<int>(std::distance(frames.begin(), it));
            refBits[hitFrameIdx] = 1;
            s.isHit = true;
            s.allocatedFrameIndex = hitFrameIdx;
            s.status = "HIT";
            result.pageHits++;

            std::ostringstream oss;
            oss << "Page " << page << " found in Frame " << hitFrameIdx 
                << " (Page Hit). Reference bit set to 1. Clock hand remains at Frame " << clockHand << ".";
            s.explanation = oss.str();
        } else {
            // PAGE FAULT
            s.isHit = false;
            result.pageFaults++;

            // Check if there is an empty frame
            auto emptyIt = std::find(frames.begin(), frames.end(), -1);
            if (emptyIt != frames.end()) {
                // Free allocation: place in the empty frame
                // In clock algorithm, if frame at clockHand is empty, place it there;
                // otherwise place in the first empty frame and adjust hand
                int targetFrame = -1;
                if (frames[clockHand] == -1) {
                    targetFrame = clockHand;
                } else {
                    targetFrame = static_cast<int>(std::distance(frames.begin(), emptyIt));
                }

                frames[targetFrame] = page;
                refBits[targetFrame] = 1;
                s.allocatedFrameIndex = targetFrame;
                s.status = "FAULT_ALLOCATE";
                result.freeAllocations++;

                clockHand = (targetFrame + 1) % frameCount;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Placed in empty Frame " 
                    << targetFrame << " (ref bit=1). Clock hand advanced to Frame " << clockHand << ".";
                s.explanation = oss.str();
            } else {
                // Full memory -> scan using Clock algorithm
                int victimFrameIdx = -1;

                while (true) {
                    inspectedFrames.push_back(clockHand);
                    if (refBits[clockHand] == 0) {
                        // Found victim with bit 0
                        victimFrameIdx = clockHand;
                        break;
                    } else {
                        // Give second chance
                        refBits[clockHand] = 0;
                        clockHand = (clockHand + 1) % frameCount;
                    }
                }

                s.victimPage = frames[victimFrameIdx];
                frames[victimFrameIdx] = page;
                refBits[victimFrameIdx] = 1;
                s.allocatedFrameIndex = victimFrameIdx;
                s.status = "FAULT_REPLACE";
                result.replacements++;

                // Advance clock hand past the replaced frame
                clockHand = (victimFrameIdx + 1) % frameCount;

                std::ostringstream oss;
                oss << "Page " << page << " not in memory (Page Fault). Clock inspected " 
                    << inspectedFrames.size() << " frame(s). Replaced page " << s.victimPage 
                    << " at Frame " << victimFrameIdx << ". Ref bit set to 1; clock hand advanced to Frame " 
                    << clockHand << ".";
                s.explanation = oss.str();
            }
        }

        s.frames = frames;

        // Build metadata
        nlohmann::json meta;
        meta["clockHand"] = clockHand;
        meta["handBeforeStep"] = handBeforeStep;
        meta["referenceBits"] = refBits;
        meta["inspectedFrames"] = inspectedFrames;
        
        std::vector<nlohmann::json> frameStates;
        for (int f = 0; f < frameCount; ++f) {
            nlohmann::json fInfo;
            fInfo["frameIndex"] = f;
            fInfo["page"] = frames[f];
            fInfo["refBit"] = (frames[f] != -1) ? refBits[f] : 0;
            fInfo["isHandHere"] = (f == clockHand);
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
