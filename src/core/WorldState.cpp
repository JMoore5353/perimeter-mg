#include "perimeter/core/WorldState.h"

namespace perimeter::core {

void WorldState::rebuildOccupancy() {
    occupancy.clear();
    occupancy.reserve(agents.size());

    for (const AgentState& agent : agents) {
        occupancy[agent.position].push_back(agent.id);
    }
}

}  // namespace perimeter::core
