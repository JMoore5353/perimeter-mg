#include "environment/Transition.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "geometry/HexGrid.h"

namespace perimeter::environment {
namespace {

TEST(TransitionTest, CaptureProbabilityMatchesSpec) {
    EXPECT_DOUBLE_EQ(captureProbabilityForDefenderCount(0), 0.0);
    EXPECT_DOUBLE_EQ(captureProbabilityForDefenderCount(1), 0.7);
    EXPECT_DOUBLE_EQ(captureProbabilityForDefenderCount(2), 0.99);
    EXPECT_DOUBLE_EQ(captureProbabilityForDefenderCount(3), 0.0);
    EXPECT_DOUBLE_EQ(captureProbabilityForDefenderCount(5), 0.0);
}

TEST(TransitionTest, CaptureSuccessUsesStrictRollComparison) {
    EXPECT_TRUE(isCaptureSuccessful(1, 0.69));
    EXPECT_FALSE(isCaptureSuccessful(1, 0.70));
    EXPECT_TRUE(isCaptureSuccessful(2, 0.98));
    EXPECT_FALSE(isCaptureSuccessful(2, 0.99));
    EXPECT_FALSE(isCaptureSuccessful(3, 0.00));
}

TEST(TransitionTest, StepWorldRejectsJointActionSizeMismatch) {
    core::WorldState world;
    world.agents = {
        core::AgentState{10, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{20, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(2);
    std::mt19937 rng(3);
    const std::vector<Action> actions{Action::STAY};

    EXPECT_THROW(stepWorld(world, actions, grid, rng), std::invalid_argument);
}

TEST(TransitionTest, AttackerOnBaseGetsRewardAndIsRespawnedToOuterRing) {
    core::WorldState world;
    world.agents = {
        core::AgentState{100, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(2);
    std::mt19937 rng(8);
    const std::vector<Action> actions{Action::STAY};

    const StepResult result = stepWorld(world, actions, grid, rng);

    ASSERT_EQ(result.rewards.size(), 1U);
    EXPECT_DOUBLE_EQ(result.rewards[0], 100.0);
    EXPECT_EQ(result.baseArrivalAttackerIds.size(), 1U);
    EXPECT_EQ(result.baseArrivalAttackerIds[0], 100);
    EXPECT_EQ(result.respawnedAttackerIds.size(), 1U);
    EXPECT_EQ(result.respawnedAttackerIds[0], 100);

    const geometry::Hex newPosition = world.agents[0].position;
    EXPECT_EQ(geometry::hexDistance(geometry::Hex{0, 0}, newPosition), grid.radius());
}

TEST(TransitionTest, DefenderOnBaseGetsPenalty) {
    core::WorldState world;
    world.agents = {
        core::AgentState{3, core::AgentType::DEFENDER, geometry::Hex{0, 0}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(2);
    std::mt19937 rng(11);
    const std::vector<Action> actions{Action::STAY};

    const StepResult result = stepWorld(world, actions, grid, rng);

    ASSERT_EQ(result.rewards.size(), 1U);
    EXPECT_DOUBLE_EQ(result.rewards[0], -10.0);
    EXPECT_TRUE(result.baseArrivalAttackerIds.empty());
    EXPECT_TRUE(result.capturedAttackerIds.empty());
}

TEST(TransitionTest, ThreeDefendersPreventCapture) {
    core::WorldState world;
    world.agents = {
        core::AgentState{101, core::AgentType::ATTACKER, geometry::Hex{1, -1}},
        core::AgentState{201, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
        core::AgentState{202, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
        core::AgentState{203, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(3);
    std::mt19937 rng(2);
    const std::vector<Action> actions{
        Action::STAY, Action::STAY, Action::STAY, Action::STAY,
    };

    const StepResult result = stepWorld(world, actions, grid, rng);

    ASSERT_EQ(result.rewards.size(), 4U);
    EXPECT_DOUBLE_EQ(result.rewards[0], 0.0);
    EXPECT_DOUBLE_EQ(result.rewards[1], 0.0);
    EXPECT_DOUBLE_EQ(result.rewards[2], 0.0);
    EXPECT_DOUBLE_EQ(result.rewards[3], 0.0);
    EXPECT_TRUE(result.capturedAttackerIds.empty());
    EXPECT_TRUE(result.respawnedAttackerIds.empty());
    EXPECT_EQ(world.agents[0].position, (geometry::Hex{1, -1}));
}

TEST(TransitionTest, BaseBreachPenalizesAllDefenders) {
    core::WorldState world;
    world.agents = {
        core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
        core::AgentState{3, core::AgentType::DEFENDER, geometry::Hex{0, 1}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(2);
    std::mt19937 rng(5);
    const std::vector<Action> actions{Action::STAY, Action::STAY, Action::STAY};

    const StepResult result = stepWorld(world, actions, grid, rng);

    ASSERT_EQ(result.rewards.size(), 3U);
    EXPECT_DOUBLE_EQ(result.rewards[0], 100.0);
    EXPECT_DOUBLE_EQ(result.rewards[1], -100.0);
    EXPECT_DOUBLE_EQ(result.rewards[2], -110.0);
}

TEST(TransitionTest, CaptureRewardIsSharedAsMOverNPerDefender) {
    core::WorldState world;
    world.agents = {
        core::AgentState{10, core::AgentType::ATTACKER, geometry::Hex{1, -1}},
        core::AgentState{11, core::AgentType::ATTACKER, geometry::Hex{1, -1}},
        core::AgentState{20, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
        core::AgentState{21, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(3);
    std::mt19937 rng(1);
    const std::vector<Action> actions{
        Action::STAY, Action::STAY, Action::STAY, Action::STAY,
    };

    const StepResult result = stepWorld(world, actions, grid, rng);
    const double captured = static_cast<double>(result.capturedAttackerIds.size());
    const double expectedPerDefender = (captured / 2.0);

    ASSERT_EQ(result.rewards.size(), 4U);
    EXPECT_DOUBLE_EQ(result.rewards[2], expectedPerDefender);
    EXPECT_DOUBLE_EQ(result.rewards[3], expectedPerDefender);
}

TEST(TransitionTest, DefenderMovementPenaltyAppliesOnlyWhenNotStaying) {
    core::WorldState world;
    world.agents = {
        core::AgentState{50, core::AgentType::ATTACKER, geometry::Hex{2, -2}},
        core::AgentState{60, core::AgentType::DEFENDER, geometry::Hex{-1, 1}},
    };
    world.rebuildOccupancy();

    const geometry::HexGrid grid(3);
    std::mt19937 rng(12);
    const std::vector<Action> actions{Action::STAY, Action::WEST};

    const StepResult result = stepWorld(world, actions, grid, rng);

    ASSERT_EQ(result.rewards.size(), 2U);
    EXPECT_DOUBLE_EQ(result.rewards[0], 0.0);
    EXPECT_DOUBLE_EQ(result.rewards[1], -0.1);
}

TEST(TransitionTest, CaptureProbabilityEmpiricallyMatchesSingleDefenderCase) {
    constexpr int kTrials = 5000;
    int captures = 0;
    std::mt19937 rng(12345);
    const geometry::HexGrid grid(3);

    for (int trial = 0; trial < kTrials; ++trial) {
        core::WorldState world;
        world.agents = {
            core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{2, -2}},
            core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{2, -2}},
        };
        world.rebuildOccupancy();
        const std::vector<Action> actions{Action::STAY, Action::STAY};

        const StepResult result = stepWorld(world, actions, grid, rng);
        if (!result.capturedAttackerIds.empty()) {
            ++captures;
        }
    }

    const double rate = static_cast<double>(captures) / static_cast<double>(kTrials);
    EXPECT_NEAR(rate, 0.7, 0.05);
}

TEST(TransitionTest, CaptureProbabilityEmpiricallyMatchesTwoDefenderCase) {
    constexpr int kTrials = 5000;
    int captures = 0;
    std::mt19937 rng(67890);
    const geometry::HexGrid grid(3);

    for (int trial = 0; trial < kTrials; ++trial) {
        core::WorldState world;
        world.agents = {
            core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{2, -2}},
            core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{2, -2}},
            core::AgentState{3, core::AgentType::DEFENDER, geometry::Hex{2, -2}},
        };
        world.rebuildOccupancy();
        const std::vector<Action> actions{Action::STAY, Action::STAY, Action::STAY};

        const StepResult result = stepWorld(world, actions, grid, rng);
        if (!result.capturedAttackerIds.empty()) {
            ++captures;
        }
    }

    const double rate = static_cast<double>(captures) / static_cast<double>(kTrials);
    EXPECT_NEAR(rate, 0.99, 0.02);
}

TEST(TransitionTest, RespawnSamplingCoversOuterRingReasonablyEvenly) {
    constexpr int kTrials = 3600;
    std::mt19937 rng(24680);
    const geometry::HexGrid grid(3);
    const std::vector<geometry::Hex> outerRing = grid.getOuterRing();
    std::unordered_map<geometry::Hex, int> counts;
    counts.reserve(outerRing.size());
    for (const geometry::Hex& cell : outerRing) {
        counts[cell] = 0;
    }

    for (int trial = 0; trial < kTrials; ++trial) {
        core::WorldState world;
        world.agents = {
            core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        };
        world.rebuildOccupancy();
        const std::vector<Action> actions{Action::STAY};

        stepWorld(world, actions, grid, rng);
        counts[world.agents[0].position] += 1;
    }

    int minCount = kTrials;
    int maxCount = 0;
    for (const auto& [cell, count] : counts) {
        (void)cell;
        minCount = std::min(minCount, count);
        maxCount = std::max(maxCount, count);
    }

    EXPECT_GT(minCount, 0);
    EXPECT_LT(static_cast<double>(maxCount) / static_cast<double>(minCount), 1.5);
}

}  // namespace
}  // namespace perimeter::environment
