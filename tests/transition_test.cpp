#include "environment/Transition.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

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
    EXPECT_TRUE(result.capturedAttackerIds.empty());
    EXPECT_TRUE(result.respawnedAttackerIds.empty());
    EXPECT_EQ(world.agents[0].position, (geometry::Hex{1, -1}));
}

}  // namespace
}  // namespace perimeter::environment
