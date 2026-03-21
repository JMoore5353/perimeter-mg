#pragma once

#include "core/WorldState.h"
#include "environment/Action.h"

namespace perimeter::core {

class Agent {
public:
    virtual ~Agent() = default;

    [[nodiscard]] virtual environment::Action act(const WorldState& world) = 0;
};

}  // namespace perimeter::core
