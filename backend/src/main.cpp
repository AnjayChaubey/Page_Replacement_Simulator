#define CPPHTTPLIB_OPENSSL_SUPPORT_NOT_DEFINED
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "../include/Simulator.h"
#include "../include/WorkloadGenerator.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

void addCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0) port = 8080;
    }

    httplib::Server svr;
    simulator::Simulator sim;

    // Resolve frontend directory path
    std::string frontendPath = "./frontend";
    if (!fs::exists(frontendPath)) {
        if (fs::exists("../frontend")) {
            frontendPath = "../frontend";
        } else if (fs::exists("../../frontend")) {
            frontendPath = "../../frontend";
        }
    }

    std::cout << "[INFO] Serving frontend static files from: " << fs::absolute(frontendPath).string() << std::endl;
    svr.set_mount_point("/", frontendPath);

    // OPTIONS preflight for CORS
    svr.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        res.status = 204;
    });

    // GET /api/health
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json response = {
            {"status", "healthy"},
            {"service", "Page Replacement Algorithm Simulator Backend"},
            {"version", "1.0.0"},
            {"algorithms", {"FIFO", "LRU", "Optimal", "LFU", "SecondChance"}}
        };
        res.set_content(response.dump(2), "application/json");
    });

    // POST /api/simulate
    svr.Post("/api/simulate", [&sim](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            auto body = json::parse(req.body);

            if (!body.contains("algorithm") || !body.contains("frames") || !body.contains("referenceString")) {
                res.status = 400;
                res.set_content(json{{"error", "Missing required fields: 'algorithm', 'frames', 'referenceString'"}}.dump(), "application/json");
                return;
            }

            std::string algorithm = body["algorithm"].get<std::string>();
            int frames = body["frames"].get<int>();

            if (frames < 1 || frames > 64) {
                res.status = 400;
                res.set_content(json{{"error", "Frame count must be between 1 and 64"}}.dump(), "application/json");
                return;
            }

            std::vector<int> refString;
            for (const auto& item : body["referenceString"]) {
                int page = item.get<int>();
                if (page < 0) {
                    res.status = 400;
                    res.set_content(json{{"error", "Page IDs must be non-negative integers"}}.dump(), "application/json");
                    return;
                }
                refString.push_back(page);
            }

            if (refString.size() > 500) {
                res.status = 400;
                res.set_content(json{{"error", "Reference string length exceeds maximum limit of 500 pages"}}.dump(), "application/json");
                return;
            }

            simulator::SimulationResult result = sim.runSimulation(algorithm, frames, refString);
            json out = result;
            res.status = 200;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Simulation failed: ") + e.what()}}.dump(), "application/json");
        }
    });

    // POST /api/compare
    svr.Post("/api/compare", [&sim](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            auto body = json::parse(req.body);

            if (!body.contains("frames") || !body.contains("referenceString")) {
                res.status = 400;
                res.set_content(json{{"error", "Missing required fields: 'frames', 'referenceString'"}}.dump(), "application/json");
                return;
            }

            int frames = body["frames"].get<int>();
            if (frames < 1 || frames > 64) {
                res.status = 400;
                res.set_content(json{{"error", "Frame count must be between 1 and 64"}}.dump(), "application/json");
                return;
            }

            std::vector<int> refString;
            for (const auto& item : body["referenceString"]) {
                int page = item.get<int>();
                if (page < 0) {
                    res.status = 400;
                    res.set_content(json{{"error", "Page IDs must be non-negative integers"}}.dump(), "application/json");
                    return;
                }
                refString.push_back(page);
            }

            simulator::AlgorithmComparisonResult comp = sim.runComparison(frames, refString);
            json out = comp;
            res.status = 200;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Comparison failed: ") + e.what()}}.dump(), "application/json");
        }
    });

    // POST /api/generate-workload
    svr.Post("/api/generate-workload", [](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            auto body = json::parse(req.body);

            std::string type = body.value("workloadType", "random");
            int count = body.value("count", 20);
            int maxPageId = body.value("maxPageId", 7);
            
            std::optional<unsigned int> seed = std::nullopt;
            if (body.contains("seed") && !body["seed"].is_null()) {
                seed = body["seed"].get<unsigned int>();
            }

            if (count < 1 || count > 300) count = 20;
            if (maxPageId < 1 || maxPageId > 100) maxPageId = 7;

            std::vector<int> ref = simulator::WorkloadGenerator::generate(type, count, maxPageId, seed);
            json out = {
                {"workloadType", type},
                {"count", count},
                {"maxPageId", maxPageId},
                {"referenceString", ref}
            };
            res.status = 200;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Workload generation failed: ") + e.what()}}.dump(), "application/json");
        }
    });

    // POST /api/workload-analysis
    svr.Post("/api/workload-analysis", [&sim](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            auto body = json::parse(req.body);

            std::string workloadType = body.value("workloadType", "locality");
            std::vector<int> refString;
            if (body.contains("referenceString") && body["referenceString"].is_array() && !body["referenceString"].empty()) {
                for (const auto& item : body["referenceString"]) {
                    refString.push_back(item.get<int>());
                }
            } else {
                refString = simulator::WorkloadGenerator::generate(workloadType, 25, 8, 42);
            }

            std::vector<int> frameCounts = {2, 3, 4, 5, 6};
            if (body.contains("frameCounts") && body["frameCounts"].is_array() && !body["frameCounts"].empty()) {
                frameCounts.clear();
                for (const auto& item : body["frameCounts"]) {
                    int f = item.get<int>();
                    if (f >= 1 && f <= 32) frameCounts.push_back(f);
                }
                std::sort(frameCounts.begin(), frameCounts.end());
                frameCounts.erase(std::unique(frameCounts.begin(), frameCounts.end()), frameCounts.end());
            }

            simulator::WorkloadAnalysisResult analysis = sim.runWorkloadAnalysis(workloadType, refString, frameCounts);
            json out = analysis;
            res.status = 200;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Workload analysis failed: ") + e.what()}}.dump(), "application/json");
        }
    });

    // POST /api/belady-experiment
    svr.Post("/api/belady-experiment", [&sim](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            auto body = json::parse(req.body);

            std::vector<int> refString;
            if (body.contains("referenceString") && body["referenceString"].is_array() && !body["referenceString"].empty()) {
                for (const auto& item : body["referenceString"]) {
                    refString.push_back(item.get<int>());
                }
            } else {
                refString = simulator::WorkloadGenerator::getClassicBeladyString();
            }

            int frame1 = body.value("frame1", 3);
            int frame2 = body.value("frame2", 4);

            auto fifo1 = sim.runSimulation("FIFO", frame1, refString);
            auto fifo2 = sim.runSimulation("FIFO", frame2, refString);
            auto lru1 = sim.runSimulation("LRU", frame1, refString);
            auto lru2 = sim.runSimulation("LRU", frame2, refString);
            auto opt1 = sim.runSimulation("Optimal", frame1, refString);
            auto opt2 = sim.runSimulation("Optimal", frame2, refString);

            bool fifoAnomaly = (fifo2.pageFaults > fifo1.pageFaults);
            bool lruAnomaly = (lru2.pageFaults > lru1.pageFaults);

            json out = {
                {"referenceString", refString},
                {"frame1", frame1},
                {"frame2", frame2},
                {"fifoResult1", fifo1},
                {"fifoResult2", fifo2},
                {"lruResult1", lru1},
                {"lruResult2", lru2},
                {"optimalResult1", opt1},
                {"optimalResult2", opt2},
                {"fifoHasAnomaly", fifoAnomaly},
                {"lruHasAnomaly", lruAnomaly},
                {"anomalyOccurred", fifoAnomaly},
                {"explanation", fifoAnomaly 
                    ? "Belady's Anomaly detected! Increasing memory frames from " + std::to_string(frame1) + " to " + std::to_string(frame2) + " caused page faults to INCREASE from " + std::to_string(fifo1.pageFaults) + " to " + std::to_string(fifo2.pageFaults) + " under FIFO."
                    : "No Belady's Anomaly observed for this string/frame combination. Page faults did not increase when frames grew from " + std::to_string(frame1) + " to " + std::to_string(frame2) + "."
                }
            };
            res.status = 200;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Belady experiment failed: ") + e.what()}}.dump(), "application/json");
        }
    });

    std::cout << "========================================================" << std::endl;
    std::cout << "  PAGE REPLACEMENT SIMULATOR BACKEND (C++17)" << std::endl;
    std::cout << "  Server listening on: http://localhost:" << port << std::endl;
    std::cout << "  API endpoints available at: /api/*" << std::endl;
    std::cout << "========================================================" << std::endl;

    svr.listen("0.0.0.0", port);
    return 0;
}
