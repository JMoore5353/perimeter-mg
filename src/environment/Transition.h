#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include "core/WorldState.h"
#include "environment/Action.h"
#include "geometry/Grid.h"

namespace perimeter::environment {

struct StepResult {
    std::vector<double> rewards;
    std::vector<int> capturedAttackerIds;
    std::vector<int> baseArrivalAttackerIds;
    std::vector<int> respawnedAttackerIds;
};

double captureProbabilityForDefenderCount(std::size_t defenderCount) noexcept;
bool isCaptureSuccessful(std::size_t defenderCount, double roll) noexcept;

StepResult stepWorld(core::WorldState& world,
                     const std::vector<Action>& jointActions,
                     const geometry::Grid& grid,
                     std::mt19937& rng);

}  // namespace perimeter::environment
