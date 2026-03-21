#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "core/WorldState.h"
#include "environment/Action.h"
#include "environment/Transition.h"
#include "geometry/Grid.h"
#include "geometry/HexGrid.h"

namespace perimeter::environment {

class Simulator {
public:
    Simulator(geometry::HexGrid grid, core::WorldState initialWorld, std::uint32_t seed);

    StepResult step(const std::vector<Action>& jointActions);

    [[nodiscard]] const core::WorldState& world() const noexcept;
    [[nodiscard]] const geometry::Grid& grid() const noexcept;

private:
    geometry::HexGrid grid_;
    core::WorldState world_;
    std::mt19937 rng_;
};

}  // namespace perimeter::environment
