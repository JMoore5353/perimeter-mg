#include "environment/Movement.h"

#include <stdexcept>

namespace perimeter::environment {

namespace {

Action turnLeft(Action action) noexcept {
    switch (action) {
        case Action::EAST:
            return Action::NORTHEAST;
        case Action::NORTHEAST:
            return Action::NORTHWEST;
        case Action::NORTHWEST:
            return Action::WEST;
        case Action::WEST:
            return Action::SOUTHWEST;
        case Action::SOUTHWEST:
            return Action::SOUTHEAST;
        case Action::SOUTHEAST:
            return Action::EAST;
        case Action::STAY:
            return Action::STAY;
    }
    return Action::STAY;
}

Action turnRight(Action action) noexcept {
    switch (action) {
        case Action::EAST:
            return Action::SOUTHEAST;
        case Action::NORTHEAST:
            return Action::EAST;
        case Action::NORTHWEST:
            return Action::NORTHEAST;
        case Action::WEST:
            return Action::NORTHWEST;
        case Action::SOUTHWEST:
            return Action::WEST;
        case Action::SOUTHEAST:
            return Action::SOUTHWEST;
        case Action::STAY:
            return Action::STAY;
    }
    return Action::STAY;
}

}  // namespace

geometry::Hex neighbor(const geometry::Hex& origin, Action action) noexcept {
    switch (action) {
        case Action::EAST:
            return geometry::Hex{origin.q + 1, origin.r};
        case Action::NORTHEAST:
            return geometry::Hex{origin.q + 1, origin.r - 1};
        case Action::NORTHWEST:
            return geometry::Hex{origin.q, origin.r - 1};
        case Action::WEST:
            return geometry::Hex{origin.q - 1, origin.r};
        case Action::SOUTHWEST:
            return geometry::Hex{origin.q - 1, origin.r + 1};
        case Action::SOUTHEAST:
            return geometry::Hex{origin.q, origin.r + 1};
        case Action::STAY:
            return origin;
    }
    return origin;
}

Action sampleActionOutcome(Action intended, double roll) noexcept {
    if (intended == Action::STAY) {
        return Action::STAY;
    }

    if (roll < 0.70) {
        return intended;
    }
    if (roll < 0.85) {
        return turnLeft(intended);
    }
    return turnRight(intended);
}

geometry::Hex resolveSingleMove(const geometry::Hex& current,
                                Action intended,
                                const geometry::Grid& grid,
                                std::mt19937& rng) {
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return resolveSingleMoveWithRoll(current, intended, grid, distribution(rng));
}

geometry::Hex resolveSingleMoveWithRoll(const geometry::Hex& current,
                                        Action intended,
                                        const geometry::Grid& grid,
                                        double roll) {
    if (intended == Action::STAY) {
        return current;
    }

    // TODO: This doesn't seem super necessary since we clamp the pos after
    // we sample the next pos. So if we happen to sample the intended, then
    // we'd clamp anyway. We could, however, select an invalid action but
    // sample a vaild one. This makes sense, but it is slightly different
    // than what the book implements (probably?).
    const geometry::Hex intendedCell = neighbor(current, intended);
    if (!grid.isValid(intendedCell)) {
        return current;
    }

    const Action sampled = sampleActionOutcome(intended, roll);
    const geometry::Hex next = neighbor(current, sampled);

    if (!grid.isValid(next)) {
        return current;
    }
    return next;
}

std::vector<geometry::Hex> resolveIntendedPositions(const core::WorldState& world,
                                                    const std::vector<Action>& jointActions,
                                                    const geometry::Grid& grid,
                                                    std::mt19937& rng) {
    if (jointActions.size() != world.agents.size()) {
        throw std::invalid_argument("Joint action count must equal number of agents.");
    }

    std::vector<geometry::Hex> nextPositions;
    nextPositions.reserve(world.agents.size());

    for (std::size_t index = 0; index < world.agents.size(); ++index) {
        nextPositions.push_back(resolveSingleMove(world.agents[index].position, jointActions[index], grid, rng));
    }

    return nextPositions;
}

void applySimultaneousMoves(core::WorldState& world,
                            const std::vector<Action>& jointActions,
                            const geometry::Grid& grid,
                            std::mt19937& rng) {
    const std::vector<geometry::Hex> nextPositions = resolveIntendedPositions(world, jointActions, grid, rng);

    for (std::size_t index = 0; index < world.agents.size(); ++index) {
        world.agents[index].position = nextPositions[index];
    }
    world.rebuildOccupancy();
}

}  // namespace perimeter::environment
