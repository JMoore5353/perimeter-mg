#include "perimeter/core/WorldState.h"

#include <gtest/gtest.h>

namespace perimeter::core {
namespace {

TEST(WorldStateTest, RebuildOccupancyIndexesAllAgentsByPosition) {
    WorldState world;
    world.agents = {
        AgentState{10, AgentType::ATTACKER, geometry::Hex{0, 0}},
        AgentState{20, AgentType::DEFENDER, geometry::Hex{1, 0}},
        AgentState{30, AgentType::ATTACKER, geometry::Hex{0, 0}},
    };

    world.rebuildOccupancy();

    ASSERT_EQ(world.occupancy.size(), 2U);
    ASSERT_NE(world.occupancy.find(geometry::Hex{0, 0}), world.occupancy.end());
    ASSERT_NE(world.occupancy.find(geometry::Hex{1, 0}), world.occupancy.end());

    const std::vector<int>& atCenter = world.occupancy.at(geometry::Hex{0, 0});
    EXPECT_EQ(atCenter.size(), 2U);
    EXPECT_EQ(atCenter[0], 10);
    EXPECT_EQ(atCenter[1], 30);

    const std::vector<int>& atNeighbor = world.occupancy.at(geometry::Hex{1, 0});
    EXPECT_EQ(atNeighbor.size(), 1U);
    EXPECT_EQ(atNeighbor[0], 20);
}

TEST(WorldStateTest, RebuildOccupancyClearsStaleDataAndReflectsNewPositions) {
    WorldState world;
    world.agents = {
        AgentState{1, AgentType::ATTACKER, geometry::Hex{0, 0}},
        AgentState{2, AgentType::DEFENDER, geometry::Hex{1, -1}},
    };

    world.rebuildOccupancy();
    ASSERT_EQ(world.occupancy.size(), 2U);

    world.agents[0].position = geometry::Hex{2, -1};
    world.agents.pop_back();
    world.rebuildOccupancy();

    ASSERT_EQ(world.occupancy.size(), 1U);
    ASSERT_NE(world.occupancy.find(geometry::Hex{2, -1}), world.occupancy.end());
    EXPECT_EQ(world.occupancy.find(geometry::Hex{0, 0}), world.occupancy.end());
    EXPECT_EQ(world.occupancy.at(geometry::Hex{2, -1}).size(), 1U);
    EXPECT_EQ(world.occupancy.at(geometry::Hex{2, -1})[0], 1);
}

TEST(WorldStateTest, RebuildOccupancyOnEmptyWorldProducesEmptyMap) {
    WorldState world;
    world.occupancy[geometry::Hex{5, 5}] = {999};

    world.rebuildOccupancy();
    EXPECT_TRUE(world.occupancy.empty());
}

}  // namespace
}  // namespace perimeter::core
