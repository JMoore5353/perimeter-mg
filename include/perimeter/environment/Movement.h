#ifndef PERIMETER_PERIMETER_ENVIRONMENT_MOVEMENT_H
#define PERIMETER_PERIMETER_ENVIRONMENT_MOVEMENT_H

#include <random>
#include <vector>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/geometry/Hex.h"
#include "perimeter/geometry/Grid.h"

#define INTENDED_ACTION_PROBABILITY 0.99
#define DEVIATED_ACTION_PROBABILITY (1 - INTENDED_ACTION_PROBABILITY) / 2.0

namespace perimeter::environment {

geometry::Hex neighbor(const geometry::Hex& origin, Action action) noexcept;

Action sampleActionOutcome(Action intended, double roll) noexcept;

geometry::Hex resolveSingleMove(const geometry::Hex& current,
                                Action intended,
                                const geometry::Grid& grid,
                                std::mt19937& rng);

geometry::Hex resolveSingleMoveWithRoll(const geometry::Hex& current,
                                        Action intended,
                                        const geometry::Grid& grid,
                                        double roll);

std::vector<geometry::Hex> resolveIntendedPositions(const core::WorldState& world,
                                                    const std::vector<Action>& jointActions,
                                                    const geometry::Grid& grid,
                                                    std::mt19937& rng);

void applySimultaneousMoves(core::WorldState& world,
                            const std::vector<Action>& jointActions,
                            const geometry::Grid& grid,
                            std::mt19937& rng);

}  // namespace perimeter::environment

#endif  // PERIMETER_PERIMETER_ENVIRONMENT_MOVEMENT_H
