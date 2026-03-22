#ifndef PERIMETER_PERIMETER_CORE_AGENT_H
#define PERIMETER_PERIMETER_CORE_AGENT_H

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"

namespace perimeter::core {

class Agent {
public:
    virtual ~Agent() = default;

    [[nodiscard]] virtual environment::Action act(const WorldState& world) = 0;
};

}  // namespace perimeter::core

#endif  // PERIMETER_PERIMETER_CORE_AGENT_H
