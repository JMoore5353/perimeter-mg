#ifndef PERIMETER_PERIMETER_ENVIRONMENT_TRANSITION_H
#define PERIMETER_PERIMETER_ENVIRONMENT_TRANSITION_H

#include <cstddef>
#include <random>
#include <vector>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/geometry/Grid.h"
#include "perimeter/learning/joint.h"

#define ATTACKER_BASE_ARRIVAL_REWARD 100
#define ATTACKER_CAPTURE_REWARD -100
#define DEFENDER_BASE_ARRIVAL_REWARD -10
#define DEFENDER_MOVEMENT_REWARD -0.1
#define DEFENDER_BASE_BREACH_REWARD -100
#define DEFENDER_CAPTURE_PER_ATTACKER_BONUS 1

namespace perimeter::environment
{

struct StepResult
{
  std::vector<double> rewards;
  std::vector<int> capturedAttackerIds;
  std::vector<int> baseArrivalAttackerIds;
  std::vector<int> respawnedAttackerIds;
};

double captureProbabilityForDefenderCount(std::size_t defenderCount) noexcept;
bool isCaptureSuccessful(std::size_t defenderCount, double roll) noexcept;

StepResult stepWorld(core::WorldState& world, const std::vector<Action>& jointActions,
                     const geometry::Grid& grid, std::mt19937& rng);

} // namespace perimeter::environment

#endif // PERIMETER_PERIMETER_ENVIRONMENT_TRANSITION_H
