#ifndef PERIMETER_PERIMETER_CORE_WORLDSTATE_H
#define PERIMETER_PERIMETER_CORE_WORLDSTATE_H

#include <unordered_map>
#include <vector>

#include "perimeter/core/AgentState.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter::core {

struct WorldState {
    std::vector<AgentState> agents;
    std::unordered_map<geometry::Hex, std::vector<int>> occupancy;

    void rebuildOccupancy();
};

}  // namespace perimeter::core

#endif  // PERIMETER_PERIMETER_CORE_WORLDSTATE_H
