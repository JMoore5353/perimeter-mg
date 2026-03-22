#ifndef PERIMETER_PERIMETER_CORE_AGENTSTATE_H
#define PERIMETER_PERIMETER_CORE_AGENTSTATE_H

#include "perimeter/core/AgentType.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter::core {

struct AgentState {
    int id{0};
    AgentType type{AgentType::ATTACKER};
    geometry::Hex position{};
};

}  // namespace perimeter::core

#endif  // PERIMETER_PERIMETER_CORE_AGENTSTATE_H
