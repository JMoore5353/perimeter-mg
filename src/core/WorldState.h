#pragma once

#include <unordered_map>
#include <vector>

#include "core/AgentState.h"
#include "geometry/Hex.h"

namespace perimeter::core {

struct WorldState {
    std::vector<AgentState> agents;
    std::unordered_map<geometry::Hex, std::vector<int>> occupancy;

    void rebuildOccupancy();
};

}  // namespace perimeter::core
