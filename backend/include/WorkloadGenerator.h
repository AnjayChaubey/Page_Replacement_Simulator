#pragma once

#include <vector>
#include <string>
#include <optional>

namespace simulator {

class WorkloadGenerator {
public:
    static std::vector<int> generate(const std::string& type, int count, int maxPageId, std::optional<unsigned int> seed = std::nullopt);
    static std::vector<int> getClassicBeladyString();
};

} // namespace simulator
