#pragma once

#include <random>
#include <vector>

#include "core/WorldState.h"
#include "environment/Action.h"
#include "geometry/Hex.h"
#include "geometry/Grid.h"

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
