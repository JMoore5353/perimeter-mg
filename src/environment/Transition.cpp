#include "environment/Transition.h"

#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "core/AgentType.h"
#include "environment/Movement.h"

namespace perimeter::environment {

namespace {

using IdSet = std::unordered_set<int>;
using IdToIndex = std::unordered_map<int, std::size_t>;

IdToIndex buildIdToIndex(const core::WorldState& world) {
    IdToIndex idToIndex;
    idToIndex.reserve(world.agents.size());
    for (std::size_t index = 0; index < world.agents.size(); ++index) {
        idToIndex[world.agents[index].id] = index;
    }
    return idToIndex;
}

void validateJointActions(const core::WorldState& world, const std::vector<Action>& jointActions) {
    if (jointActions.size() != world.agents.size()) {
        throw std::invalid_argument("Joint action count must equal number of agents.");
    }
}

void initializeRewards(StepResult& result, std::size_t count) {
    result.rewards.assign(count, 0.0);
}

void resolveCaptures(core::WorldState& world,
                     const IdToIndex& idToIndex,
                     std::mt19937& rng,
                     StepResult& result,
                     IdSet& capturedIds) {
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    for (const auto& entry : world.occupancy) {
        const std::vector<int>& ids = entry.second;
        std::vector<int> attackers;
        std::size_t defenderCount = 0;

        for (const int agentId : ids) {
            const auto found = idToIndex.find(agentId);
            if (found == idToIndex.end()) {
                continue;
            }
            const core::AgentState& agent = world.agents[found->second];
            if (agent.type == core::AgentType::ATTACKER) {
                attackers.push_back(agentId);
            } else if (agent.type == core::AgentType::DEFENDER) {
                ++defenderCount;
            }
        }

        if (defenderCount == 0 || attackers.empty()) {
            continue;
        }

        for (const int attackerId : attackers) {
            if (!isCaptureSuccessful(defenderCount, distribution(rng))) {
                continue;
            }
            capturedIds.insert(attackerId);
            const auto found = idToIndex.find(attackerId);
            if (found != idToIndex.end()) {
                result.rewards[found->second] -= 100.0;
            }
        }
    }
}

std::unordered_set<geometry::Hex> makeBaseSet(const geometry::HexGrid& grid) {
    const std::vector<geometry::Hex> baseTiles = grid.getBaseTiles();
    return std::unordered_set<geometry::Hex>(baseTiles.begin(), baseTiles.end());
}

void applyBaseInteractions(const core::WorldState& world,
                           const std::unordered_set<geometry::Hex>& baseSet,
                           StepResult& result,
                           IdSet& baseArrivalIds) {
    for (std::size_t index = 0; index < world.agents.size(); ++index) {
        const core::AgentState& agent = world.agents[index];
        if (baseSet.find(agent.position) == baseSet.end()) {
            continue;
        }

        if (agent.type == core::AgentType::ATTACKER) {
            baseArrivalIds.insert(agent.id);
            result.rewards[index] += 100.0;
        } else if (agent.type == core::AgentType::DEFENDER) {
            result.rewards[index] -= 10.0;
        }
    }
}

IdSet mergeRespawnIds(const IdSet& capturedIds, const IdSet& baseArrivalIds) {
    IdSet merged = capturedIds;
    merged.insert(baseArrivalIds.begin(), baseArrivalIds.end());
    return merged;
}

void respawnAttackers(core::WorldState& world,
                      const IdSet& respawnIds,
                      const IdToIndex& idToIndex,
                      const geometry::HexGrid& grid,
                      std::mt19937& rng,
                      StepResult& result) {
    const std::vector<geometry::Hex> outerRing = grid.getOuterRing();
    if (!respawnIds.empty() && outerRing.empty()) {
        throw std::runtime_error("Cannot respawn attackers: outer ring is empty.");
    }
    if (outerRing.empty()) {
        return;
    }

    std::uniform_int_distribution<std::size_t> ringDistribution(0, outerRing.size() - 1);
    for (const int attackerId : respawnIds) {
        const auto found = idToIndex.find(attackerId);
        if (found == idToIndex.end()) {
            continue;
        }
        world.agents[found->second].position = outerRing[ringDistribution(rng)];
        result.respawnedAttackerIds.push_back(attackerId);
    }
}

void finalizeStep(core::WorldState& world,
                  const IdSet& capturedIds,
                  const IdSet& baseArrivalIds,
                  StepResult& result) {
    world.rebuildOccupancy();
    result.capturedAttackerIds.assign(capturedIds.begin(), capturedIds.end());
    result.baseArrivalAttackerIds.assign(baseArrivalIds.begin(), baseArrivalIds.end());
}

}  // namespace

double captureProbabilityForDefenderCount(std::size_t defenderCount) noexcept {
    if (defenderCount == 1) {
        return 0.7;
    }
    if (defenderCount == 2) {
        return 0.99;
    }
    if (defenderCount >= 3) {
        return 0.0;
    }
    return 0.0;
}

bool isCaptureSuccessful(std::size_t defenderCount, double roll) noexcept {
    const double probability = captureProbabilityForDefenderCount(defenderCount);
    return roll < probability;
}

StepResult stepWorld(core::WorldState& world,
                     const std::vector<Action>& jointActions,
                     const geometry::HexGrid& grid,
                     std::mt19937& rng) {
    validateJointActions(world, jointActions);
    StepResult result;
    initializeRewards(result, world.agents.size());
    const IdToIndex idToIndex = buildIdToIndex(world);

    // 1-4: collect, compute, apply moves
    applySimultaneousMoves(world, jointActions, grid, rng);

    IdSet capturedIds;
    IdSet baseArrivalIds;
    // 5: resolve captures
    resolveCaptures(world, idToIndex, rng, result, capturedIds);

    // 6: base arrivals and defender penalties on base occupancy
    const std::unordered_set<geometry::Hex> baseSet = makeBaseSet(grid);
    applyBaseInteractions(world, baseSet, result, baseArrivalIds);

    // 7: respawn marked attackers on outer ring uniformly
    const IdSet allRespawnIds = mergeRespawnIds(capturedIds, baseArrivalIds);
    respawnAttackers(world, allRespawnIds, idToIndex, grid, rng, result);

    // 8: finalize outputs and rebuild occupancy to include respawned agents
    finalizeStep(world, capturedIds, baseArrivalIds, result);
    return result;
}

}  // namespace perimeter::environment
