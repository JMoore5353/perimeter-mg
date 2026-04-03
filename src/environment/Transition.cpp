#include "perimeter/environment/Transition.h"

#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "perimeter/core/AgentType.h"
#include "perimeter/environment/Movement.h"

namespace perimeter::environment
{

namespace
{

using IdSet = std::unordered_set<int>;
using IdToIndex = std::unordered_map<int, std::size_t>;
using RewardById = std::unordered_map<int, double>;

struct CaptureRecord
{
  std::vector<int> defenderIds;
  std::size_t capturedAttackerCount{0};
};

IdToIndex buildIdToIndex(const core::WorldState& world)
{
  IdToIndex idToIndex;
  idToIndex.reserve(world.agents.size());
  for (std::size_t index = 0; index < world.agents.size(); ++index) {
    idToIndex[world.agents[index].id] = index;
  }
  return idToIndex;
}

void validateJointActions(const core::WorldState& world, const std::vector<Action>& jointActions)
{
  if (jointActions.size() != world.agents.size()) {
    throw std::invalid_argument("Joint action count must equal number of agents.");
  }
}

void initializeRewards(StepResult& result, std::size_t count)
{
  result.rewards.assign(count, 0.0);
}

RewardById initializeRewardById(const core::WorldState& world)
{
  RewardById rewardById;
  rewardById.reserve(world.agents.size());
  for (const core::AgentState& agent : world.agents) {
    rewardById[agent.id] = 0.0;
  }
  return rewardById;
}

void applyDefenderMovementPenalty(const core::WorldState& world,
                                  const std::vector<Action>& jointActions, RewardById& rewardById)
{
  for (std::size_t index = 0; index < world.agents.size(); ++index) {
    const core::AgentState& agent = world.agents[index];
    if (agent.type == core::AgentType::DEFENDER && jointActions[index] != Action::STAY) {
      rewardById[agent.id] += DEFENDER_MOVEMENT_REWARD;
    }
  }
}

void applyDeterministicMoves(core::WorldState& world, const std::vector<Action>& jointActions,
                             const geometry::Grid& grid)
{
  for (std::size_t index = 0; index < world.agents.size(); ++index) {
    const geometry::Hex current = world.agents[index].position;
    const Action action = jointActions[index];

    if (action == Action::STAY) {
      continue;
    }

    const geometry::Hex intended = neighbor(current, action);
    if (grid.isValid(intended)) {
      world.agents[index].position = intended;
    }
  }
}

std::vector<CaptureRecord> resolveCaptures(const core::WorldState& world,
                                           const IdToIndex& idToIndex, std::mt19937& rng,
                                           IdSet& capturedIds)
{
  std::vector<CaptureRecord> records;
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  for (const auto& entry : world.occupancy) {
    const std::vector<int>& ids = entry.second;
    std::vector<int> attackers;
    std::vector<int> defenders;

    for (const int agentId : ids) {
      const auto found = idToIndex.find(agentId);
      if (found == idToIndex.end()) {
        continue;
      }
      const core::AgentState& agent = world.agents[found->second];
      if (agent.type == core::AgentType::ATTACKER) {
        attackers.push_back(agentId);
      } else if (agent.type == core::AgentType::DEFENDER) {
        defenders.push_back(agentId);
      }
    }

    if (defenders.empty() || attackers.empty()) {
      continue;
    }

    std::size_t capturedInCell = 0;
    for (const int attackerId : attackers) {
      if (!isCaptureSuccessful(defenders.size(), distribution(rng))) {
        continue;
      }
      capturedIds.insert(attackerId);
      ++capturedInCell;
    }

    if (capturedInCell > 0) {
      records.push_back(CaptureRecord{defenders, capturedInCell});
    }
  }
  return records;
}

void applyAttackerCapturePenalty(const IdSet& capturedIds, RewardById& rewardById)
{
  for (const int attackerId : capturedIds) {
    rewardById[attackerId] += ATTACKER_CAPTURE_REWARD;
  }
}

void applyDefenderCaptureRewards(const std::vector<CaptureRecord>& records, RewardById& rewardById)
{
  for (const CaptureRecord& record : records) {
    const double rewardShare = static_cast<double>(record.capturedAttackerCount)
      / static_cast<double>(record.defenderIds.size());
    for (const int defenderId : record.defenderIds) {
      rewardById[defenderId] += DEFENDER_CAPTURE_PER_ATTACKER_BONUS * rewardShare;
    }
  }
}

void applyExpectedCaptureRewards(const core::WorldState& world, const IdToIndex& idToIndex,
                                 RewardById& rewardById)
{
  for (const auto& entry : world.occupancy) {
    const std::vector<int>& ids = entry.second;
    std::vector<int> attackers;
    std::vector<int> defenders;

    for (const int agentId : ids) {
      const auto found = idToIndex.find(agentId);
      if (found == idToIndex.end()) {
        continue;
      }
      const core::AgentState& agent = world.agents[found->second];
      if (agent.type == core::AgentType::ATTACKER) {
        attackers.push_back(agentId);
      } else if (agent.type == core::AgentType::DEFENDER) {
        defenders.push_back(agentId);
      }
    }

    if (defenders.empty() || attackers.empty()) {
      continue;
    }

    const double captureProb = captureProbabilityForDefenderCount(defenders.size());
    const double expectedCapturedCount = captureProb * static_cast<double>(attackers.size());

    // Apply expected attacker capture penalty
    const double attackerExpectedReward = captureProb * ATTACKER_CAPTURE_REWARD;
    for (const int attackerId : attackers) {
      rewardById[attackerId] += attackerExpectedReward;
    }

    // Apply expected defender capture rewards
    const double defenderRewardShare =
      expectedCapturedCount / static_cast<double>(defenders.size());
    const double defenderExpectedReward = DEFENDER_CAPTURE_PER_ATTACKER_BONUS * defenderRewardShare;
    for (const int defenderId : defenders) {
      rewardById[defenderId] += defenderExpectedReward;
    }
  }
}

std::unordered_set<geometry::Hex> makeBaseSet(const geometry::Grid& grid)
{
  const std::vector<geometry::Hex> baseTiles = grid.getBaseTiles();
  return std::unordered_set<geometry::Hex>(baseTiles.begin(), baseTiles.end());
}

void applyBaseInteractions(const core::WorldState& world,
                           const std::unordered_set<geometry::Hex>& baseSet, RewardById& rewardById,
                           IdSet& baseArrivalIds)
{
  for (const core::AgentState& agent : world.agents) {
    if (baseSet.find(agent.position) == baseSet.end()) {
      continue;
    }

    if (agent.type == core::AgentType::ATTACKER) {
      baseArrivalIds.insert(agent.id);
      rewardById[agent.id] += ATTACKER_BASE_ARRIVAL_REWARD;
    } else if (agent.type == core::AgentType::DEFENDER) {
      rewardById[agent.id] += DEFENDER_BASE_ARRIVAL_REWARD;
    }
  }
}

void applyDefenderBaseBreachPenalty(const core::WorldState& world, const IdSet& baseArrivalIds,
                                    RewardById& rewardById)
{
  if (baseArrivalIds.empty()) {
    return;
  }
  for (const core::AgentState& agent : world.agents) {
    if (agent.type == core::AgentType::DEFENDER) {
      rewardById[agent.id] += DEFENDER_BASE_BREACH_REWARD;
    }
  }
}

IdSet mergeRespawnIds(const IdSet& capturedIds, const IdSet& baseArrivalIds)
{
  IdSet merged = capturedIds;
  merged.insert(baseArrivalIds.begin(), baseArrivalIds.end());
  return merged;
}

void respawnAttackers(core::WorldState& world, const IdSet& respawnIds, const IdToIndex& idToIndex,
                      const geometry::Grid& grid, std::mt19937& rng, StepResult& result)
{
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

void finalizeStep(core::WorldState& world, const IdToIndex& idToIndex, const RewardById& rewardById,
                  const IdSet& capturedIds, const IdSet& baseArrivalIds, StepResult& result)
{
  world.rebuildOccupancy();
  for (const auto& entry : idToIndex) {
    const auto found = rewardById.find(entry.first);
    if (found != rewardById.end()) {
      result.rewards[entry.second] = found->second;
    }
  }
  result.capturedAttackerIds.assign(capturedIds.begin(), capturedIds.end());
  result.baseArrivalAttackerIds.assign(baseArrivalIds.begin(), baseArrivalIds.end());
}

} // namespace

double captureProbabilityForDefenderCount(std::size_t defenderCount) noexcept
{
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

bool isCaptureSuccessful(std::size_t defenderCount, double roll) noexcept
{
  const double probability = captureProbabilityForDefenderCount(defenderCount);
  return roll < probability;
}

StepResult stepWorld(core::WorldState& world, const std::vector<Action>& jointActions,
                     const geometry::Grid& grid, std::mt19937& rng)
{
  validateJointActions(world, jointActions);
  StepResult result;
  initializeRewards(result, world.agents.size());
  const IdToIndex idToIndex = buildIdToIndex(world);
  RewardById rewardById = initializeRewardById(world);

  // 1-4: collect, compute, apply moves
  applySimultaneousMoves(world, jointActions, grid, rng);
  applyDefenderMovementPenalty(world, jointActions, rewardById);

  IdSet capturedIds;
  IdSet baseArrivalIds;
  // 5: resolve captures
  const std::vector<CaptureRecord> captureRecords =
    resolveCaptures(world, idToIndex, rng, capturedIds);
  applyAttackerCapturePenalty(capturedIds, rewardById);
  applyDefenderCaptureRewards(captureRecords, rewardById);

  // 6: base arrivals and defender penalties on base occupancy
  const std::unordered_set<geometry::Hex> baseSet = makeBaseSet(grid);
  applyBaseInteractions(world, baseSet, rewardById, baseArrivalIds);
  applyDefenderBaseBreachPenalty(world, baseArrivalIds, rewardById);

  // 7: respawn marked attackers on outer ring uniformly
  const IdSet allRespawnIds = mergeRespawnIds(capturedIds, baseArrivalIds);
  respawnAttackers(world, allRespawnIds, idToIndex, grid, rng, result);

  // 8: finalize outputs and rebuild occupancy to include respawned agents
  finalizeStep(world, idToIndex, rewardById, capturedIds, baseArrivalIds, result);
  return result;
}

} // namespace perimeter::environment
