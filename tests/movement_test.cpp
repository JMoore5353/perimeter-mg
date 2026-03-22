#include "perimeter/environment/Movement.h"

#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "perimeter/geometry/HexGrid.h"

namespace perimeter::environment {
namespace {

TEST(MovementTest, NeighborMapsAllActionsToExpectedHexes) {
    const geometry::Hex start{2, -1};

    EXPECT_EQ(neighbor(start, Action::EAST), (geometry::Hex{3, -1}));
    EXPECT_EQ(neighbor(start, Action::NORTHEAST), (geometry::Hex{3, -2}));
    EXPECT_EQ(neighbor(start, Action::NORTHWEST), (geometry::Hex{2, -2}));
    EXPECT_EQ(neighbor(start, Action::WEST), (geometry::Hex{1, -1}));
    EXPECT_EQ(neighbor(start, Action::SOUTHWEST), (geometry::Hex{1, 0}));
    EXPECT_EQ(neighbor(start, Action::SOUTHEAST), (geometry::Hex{2, 0}));
    EXPECT_EQ(neighbor(start, Action::STAY), start);
}

TEST(MovementTest, SampleActionOutcomeUsesExpectedProbabilityBuckets) {
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.00), Action::EAST);
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.69), Action::EAST);
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.70), Action::NORTHEAST);
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.84), Action::NORTHEAST);
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.85), Action::SOUTHEAST);
    EXPECT_EQ(sampleActionOutcome(Action::EAST, 0.99), Action::SOUTHEAST);
    EXPECT_EQ(sampleActionOutcome(Action::STAY, 0.50), Action::STAY);
}

TEST(MovementTest, ResolveSingleMoveStaysWhenIntendedDirectionHitsWall) {
    const geometry::HexGrid grid(1);
    std::mt19937 rng(42);

    const geometry::Hex current{1, 0};
    const geometry::Hex next = resolveSingleMove(current, Action::EAST, grid, rng);
    EXPECT_EQ(next, current);
}

TEST(MovementTest, ResolveSingleMoveClampsInvalidSampledAdjacentMove) {
    const geometry::HexGrid grid(1);
    const geometry::Hex current{1, 0};
    const geometry::Hex next = resolveSingleMoveWithRoll(current, Action::SOUTHWEST, grid, 0.80);
    EXPECT_EQ(next, current);
}

TEST(MovementTest, ResolveSingleMoveForStayAlwaysStays) {
    const geometry::HexGrid grid(3);
    std::mt19937 rng(123);

    const geometry::Hex current{0, -2};
    const geometry::Hex next = resolveSingleMove(current, Action::STAY, grid, rng);
    EXPECT_EQ(next, current);
}

TEST(MovementTest, ResolveIntendedPositionsRequiresMatchingJointActionCount) {
    core::WorldState world;
    world.agents = {
        core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{0, 1}},
    };

    const geometry::HexGrid grid(2);
    std::mt19937 rng(7);
    const std::vector<Action> invalidActions{Action::EAST};

    EXPECT_THROW(resolveIntendedPositions(world, invalidActions, grid, rng), std::invalid_argument);
}

TEST(MovementTest, ApplySimultaneousMovesUpdatesAllAgentsAtOnce) {
    core::WorldState world;
    world.agents = {
        core::AgentState{11, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{22, core::AgentType::DEFENDER, geometry::Hex{0, 1}},
    };

    const geometry::HexGrid grid(3);
    const std::vector<Action> actions{Action::EAST, Action::WEST};
    std::mt19937 expectedRng(1);
    const std::vector<geometry::Hex> expected =
        resolveIntendedPositions(world, actions, grid, expectedRng);

    std::mt19937 rng(1);

    applySimultaneousMoves(world, actions, grid, rng);

    EXPECT_EQ(world.agents[0].position, expected[0]);
    EXPECT_EQ(world.agents[1].position, expected[1]);

    ASSERT_EQ(world.occupancy.size(), 2U);
    ASSERT_NE(world.occupancy.find(expected[0]), world.occupancy.end());
    ASSERT_NE(world.occupancy.find(expected[1]), world.occupancy.end());
    EXPECT_EQ(world.occupancy.at(expected[0])[0], 11);
    EXPECT_EQ(world.occupancy.at(expected[1])[0], 22);
}

}  // namespace
}  // namespace perimeter::environment
