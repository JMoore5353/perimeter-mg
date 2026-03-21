#pragma once

#include "core/AgentType.h"
#include "geometry/Hex.h"

namespace perimeter::core {

struct AgentState {
    int id{0};
    AgentType type{AgentType::ATTACKER};
    geometry::Hex position{};
};

}  // namespace perimeter::core
