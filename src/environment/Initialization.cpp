#include "perimeter/environment/Initialization.h"

#include <random>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "perimeter/core/AgentType.h"

namespace perimeter::environment {

InitializedEnvironment createInitialWorld(const InitializationConfig& config) {
    if (config.radius < 0) {
        throw std::invalid_argument("Radius must be non-negative.");
    }
    if (config.attackerCount < 0 || config.defenderCount < 0) {
        throw std::invalid_argument("Agent counts must be non-negative.");
    }

    geometry::HexGrid grid =
        config.baseTiles.has_value() ? geometry::HexGrid(config.radius, *config.baseTiles) : geometry::HexGrid(config.radius);

    const std::vector<geometry::Hex> outerRing = grid.getOuterRing();
    if (config.attackerCount > 0 && outerRing.empty()) {
        throw std::invalid_argument("Cannot spawn attackers: outer ring is empty.");
    }

    const std::vector<geometry::Hex> baseTiles = grid.getBaseTiles();
    const std::unordered_set<geometry::Hex> baseSet(baseTiles.begin(), baseTiles.end());

    std::vector<geometry::Hex> defenderSpawnCells;
    for (const geometry::Hex& cell : grid.getGridCells()) {
        if (baseSet.find(cell) == baseSet.end()) {
            defenderSpawnCells.push_back(cell);
        }
    }
    if (config.defenderCount > 0 && defenderSpawnCells.empty()) {
        throw std::invalid_argument("Cannot spawn defenders: no non-base cells available.");
    }

    std::mt19937 rng(config.seed);
    std::uniform_int_distribution<std::size_t> attackerDistribution(0, outerRing.empty() ? 0 : outerRing.size() - 1);
    std::uniform_int_distribution<std::size_t> defenderDistribution(0, defenderSpawnCells.empty() ? 0 : defenderSpawnCells.size() - 1);

    core::WorldState world;
    world.agents.reserve(static_cast<std::size_t>(config.attackerCount + config.defenderCount));

    int nextId = 0;
    for (int i = 0; i < config.attackerCount; ++i) {
        const geometry::Hex spawn = outerRing[attackerDistribution(rng)];
        world.agents.push_back(core::AgentState{nextId++, core::AgentType::ATTACKER, spawn});
    }

    for (int i = 0; i < config.defenderCount; ++i) {
        const geometry::Hex spawn = defenderSpawnCells[defenderDistribution(rng)];
        world.agents.push_back(core::AgentState{nextId++, core::AgentType::DEFENDER, spawn});
    }

    world.rebuildOccupancy();
    return InitializedEnvironment{std::move(grid), std::move(world)};
}

}  // namespace perimeter::environment
