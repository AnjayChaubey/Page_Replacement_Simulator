#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <algorithm>
#include <set>

#include "../include/Simulator.h"
#include "../include/WorkloadGenerator.h"

using namespace simulator;

void verifySimulationInvariants(const SimulationResult& res) {
    // 1. Total references check
    assert(res.totalReferences == static_cast<int>(res.referenceString.size()));

    if (res.totalReferences == 0) {
        assert(res.pageHits == 0);
        assert(res.pageFaults == 0);
        assert(res.hitRatio == 0.0);
        assert(res.faultRatio == 0.0);
        assert(res.steps.empty());
        return;
    }

    // 2. Total hits + total faults = total references
    assert(res.pageHits + res.pageFaults == res.totalReferences);

    // 3. Replacements + Free Allocations = Total Page Faults
    assert(res.replacements + res.freeAllocations == res.pageFaults);

    // 4. Hit ratio + Fault ratio = 100% (within floating point precision)
    double sumRatio = res.hitRatio + res.faultRatio;
    assert(std::abs(sumRatio - 100.0) < 0.01);

    // 5. Steps invariants
    assert(res.steps.size() == res.referenceString.size());

    std::vector<int> previousFrames(res.frameCount, -1);

    for (size_t i = 0; i < res.steps.size(); ++i) {
        const auto& step = res.steps[i];
        int page = res.referenceString[i];

        assert(step.stepIndex == static_cast<int>(i));
        assert(step.page == page);
        assert(step.insertedPage == page);
        assert(step.frames.size() == static_cast<size_t>(res.frameCount));

        // Check if page was present in previous frames
        bool wasPresent = (std::find(previousFrames.begin(), previousFrames.end(), page) != previousFrames.end());

        if (wasPresent) {
            assert(step.isHit == true);
            assert(step.status == "HIT");
            assert(step.victimPage == -1);
            // Hits must never change frame contents
            assert(step.frames == previousFrames);
        } else {
            assert(step.isHit == false);
            assert(step.status == "FAULT_ALLOCATE" || step.status == "FAULT_REPLACE");

            // Page must now be in the frame
            assert(step.frames[step.allocatedFrameIndex] == page);

            if (step.status == "FAULT_ALLOCATE") {
                assert(step.victimPage == -1);
                assert(previousFrames[step.allocatedFrameIndex] == -1);
            } else {
                assert(step.victimPage != -1);
                assert(previousFrames[step.allocatedFrameIndex] == step.victimPage);
            }
        }

        // Check that no frame has duplicate valid pages
        std::set<int> uniquePages;
        for (int p : step.frames) {
            if (p != -1) {
                assert(uniquePages.find(p) == uniquePages.end()); // No duplicates!
                uniquePages.insert(p);
            }
        }

        previousFrames = step.frames;
    }
}

void testStandardString() {
    std::cout << "[RUNNING] testStandardString (7,0,1,2,0,3,0,4,2,3,0,3,2 on 3 frames)..." << std::endl;
    std::vector<int> ref = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    int frames = 3;

    Simulator sim;
    auto comp = sim.runComparison(frames, ref);

    for (const auto& r : comp.results) {
        verifySimulationInvariants(r);
        std::cout << "  " << r.algorithm << ": Faults=" << r.pageFaults 
                  << ", Hits=" << r.pageHits 
                  << ", HitRatio=" << r.hitRatio << "%" << std::endl;
    }

    // Optimal must have <= faults than any other algorithm
    int optFaults = -1;
    for (const auto& r : comp.results) {
        if (r.algorithm == "Optimal") optFaults = r.pageFaults;
    }
    assert(optFaults != -1);

    for (const auto& r : comp.results) {
        assert(optFaults <= r.pageFaults);
    }

    std::cout << "[PASSED] testStandardString!" << std::endl;
}

void testBeladyAnomaly() {
    std::cout << "[RUNNING] testBeladyAnomaly..." << std::endl;
    std::vector<int> beladyStr = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};

    Simulator sim;
    auto fifo3 = sim.runSimulation("FIFO", 3, beladyStr);
    auto fifo4 = sim.runSimulation("FIFO", 4, beladyStr);
    auto lru3 = sim.runSimulation("LRU", 3, beladyStr);
    auto lru4 = sim.runSimulation("LRU", 4, beladyStr);

    verifySimulationInvariants(fifo3);
    verifySimulationInvariants(fifo4);
    verifySimulationInvariants(lru3);
    verifySimulationInvariants(lru4);

    std::cout << "  FIFO 3 frames faults: " << fifo3.pageFaults << std::endl;
    std::cout << "  FIFO 4 frames faults: " << fifo4.pageFaults << std::endl;
    std::cout << "  LRU 3 frames faults:  " << lru3.pageFaults << std::endl;
    std::cout << "  LRU 4 frames faults:  " << lru4.pageFaults << std::endl;

    // Belady's anomaly: FIFO with 4 frames has MORE page faults than with 3 frames!
    // Textbook values: FIFO 3 frames = 9 faults; FIFO 4 frames = 10 faults!
    assert(fifo3.pageFaults == 9);
    assert(fifo4.pageFaults == 10);
    assert(fifo4.pageFaults > fifo3.pageFaults); // True anomaly!

    // LRU (stack algorithm) must NEVER experience Belady's anomaly
    assert(lru4.pageFaults <= lru3.pageFaults);

    std::cout << "[PASSED] testBeladyAnomaly!" << std::endl;
}

void testEdgeCases() {
    std::cout << "[RUNNING] testEdgeCases..." << std::endl;
    Simulator sim;
    std::vector<std::string> algos = sim.getSupportedAlgorithms();

    // 1. Empty reference string
    for (const auto& a : algos) {
        auto r = sim.runSimulation(a, 3, {});
        verifySimulationInvariants(r);
        assert(r.totalReferences == 0);
    }

    // 2. Single frame
    std::vector<int> singleRef = {1, 2, 1, 3, 1, 2};
    for (const auto& a : algos) {
        auto r = sim.runSimulation(a, 1, singleRef);
        verifySimulationInvariants(r);
        assert(r.frameCount == 1);
    }

    // 3. All references identical
    std::vector<int> identicalRef = {4, 4, 4, 4, 4, 4};
    for (const auto& a : algos) {
        auto r = sim.runSimulation(a, 3, identicalRef);
        verifySimulationInvariants(r);
        assert(r.pageFaults == 1);
        assert(r.pageHits == 5);
        assert(r.replacements == 0);
        assert(r.freeAllocations == 1);
    }

    // 4. More frames than unique pages
    std::vector<int> smallRef = {1, 2, 1, 3, 2, 1, 3};
    for (const auto& a : algos) {
        auto r = sim.runSimulation(a, 5, smallRef);
        verifySimulationInvariants(r);
        assert(r.pageFaults == 3); // pages 1, 2, 3
        assert(r.replacements == 0);
        assert(r.freeAllocations == 3);
    }

    std::cout << "[PASSED] testEdgeCases!" << std::endl;
}

void testWorkloadGenerator() {
    std::cout << "[RUNNING] testWorkloadGenerator..." << std::endl;
    auto seq = WorkloadGenerator::generate("sequential", 20, 5, 42);
    assert(seq.size() == 20);
    for (int p : seq) {
        assert(p >= 0 && p <= 5);
    }

    auto rnd = WorkloadGenerator::generate("random", 30, 9, 123);
    assert(rnd.size() == 30);

    auto rep = WorkloadGenerator::generate("repeated", 25, 8, 999);
    assert(rep.size() == 25);

    auto loc = WorkloadGenerator::generate("locality", 40, 10, 777);
    assert(loc.size() == 40);

    std::cout << "[PASSED] testWorkloadGenerator!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RUNNING C++ PAGE REPLACEMENT UNIT TESTS" << std::endl;
    std::cout << "========================================" << std::endl;

    testStandardString();
    testBeladyAnomaly();
    testEdgeCases();
    testWorkloadGenerator();

    std::cout << "========================================" << std::endl;
    std::cout << "ALL C++ UNIT TESTS PASSED SUCCESSFULLY! " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
