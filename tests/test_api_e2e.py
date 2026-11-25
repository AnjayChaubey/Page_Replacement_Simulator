import urllib.request
import json
import sys

BASE_URL = "http://127.0.0.1:8080"

def post(endpoint, data):
    req = urllib.request.Request(
        f"{BASE_URL}{endpoint}",
        data=json.dumps(data).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode("utf-8"))

def get(endpoint):
    with urllib.request.urlopen(f"{BASE_URL}{endpoint}") as resp:
        return json.loads(resp.read().decode("utf-8"))

def test_all():
    print("[1] Testing /api/health...")
    health = get("/api/health")
    assert health["status"] == "healthy"
    assert len(health["algorithms"]) == 5
    print("    -> Health OK!")

    print("[2] Testing /api/simulate for all 5 algorithms...")
    standard_ref = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2]
    expected_faults = {}
    for algo in ["FIFO", "LRU", "Optimal", "LFU", "SecondChance"]:
        res = post("/api/simulate", {
            "algorithm": algo,
            "frames": 3,
            "referenceString": standard_ref
        })
        assert res["totalReferences"] == 13
        assert res["pageHits"] + res["pageFaults"] == 13
        assert res["replacements"] + res["freeAllocations"] == res["pageFaults"]
        assert len(res["steps"]) == 13
        expected_faults[algo] = res["pageFaults"]
        print(f"    -> {algo}: Faults={res['pageFaults']}, Hits={res['pageHits']}, HitRatio={res['hitRatio']:.2f}%")

    # In standard reference string on 3 frames: Optimal <= others
    assert expected_faults["Optimal"] <= min(expected_faults.values())
    print("    -> Invariant check passed: Optimal produced minimum page faults!")

    print("[3] Testing /api/compare...")
    comp = post("/api/compare", {
        "frames": 3,
        "referenceString": standard_ref
    })
    assert len(comp["results"]) == 5
    assert comp["bestAlgorithm"] in ["FIFO", "LRU", "Optimal", "LFU", "SecondChance"]
    print(f"    -> Compare OK! Best online algorithm: {comp['bestAlgorithm']} ({comp['lowestFaults']} faults)")

    print("[4] Testing /api/generate-workload...")
    for w_type in ["random", "sequential", "repeated", "locality", "high_diversity", "belady_classic"]:
        gen = post("/api/generate-workload", {
            "workloadType": w_type,
            "count": 25,
            "maxPageId": 8,
            "seed": 42
        })
        assert len(gen["referenceString"]) == 25
        assert all(p >= 0 for p in gen["referenceString"])
        print(f"    -> Workload {w_type}: 25 items generated OK")

    print("[5] Testing /api/workload-analysis across frames [2, 3, 4, 5, 6]...")
    analysis = post("/api/workload-analysis", {
        "workloadType": "locality",
        "frameCounts": [2, 3, 4, 5, 6]
    })
    assert len(analysis["dataPoints"]) == 5
    for pt in analysis["dataPoints"]:
        assert len(pt["algorithmFaults"]) == 5
    print("    -> Workload Analysis OK!")

    print("[6] Testing /api/belady-experiment on classic string...")
    belady = post("/api/belady-experiment", {
        "frame1": 3,
        "frame2": 4
    })
    assert belady["anomalyOccurred"] == True
    assert belady["fifoResult1"]["pageFaults"] == 9
    assert belady["fifoResult2"]["pageFaults"] == 10
    assert belady["lruResult1"]["pageFaults"] >= belady["lruResult2"]["pageFaults"]
    print("    -> Belady Experiment OK! 9 -> 10 faults verified.")

    print("\nALL BACKEND REST API END-TO-END TESTS PASSED PERFECTLY!")

if __name__ == "__main__":
    test_all()
