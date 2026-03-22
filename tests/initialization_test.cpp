#include "perimeter/environment/Initialization.h"

#include <stdexcept>
#include <unordered_set>

#include <gtest/gtest.h>

#include "perimeter/geometry/Hex.h"

namespace perimeter::environment {
namespace {

TEST(InitializationTest, CreatesRequestedAgentCountsAndValidPositions) {
    const InitializationConfig config{
        .radius = 3,
        .attackerCount = 4,
        .defenderCount = 5,
        .seed = 42U,
    };

    const InitializedEnvironment initialized = createInitialWorld(config);
    const geometry::HexGrid& grid = initialized.grid;
    const core::WorldState& world = initialized.world;

    ASSERT_EQ(world.agents.size(), 9U);

    const std::vector<geometry::Hex> baseTiles = grid.getBaseTiles();
    const std::unordered_set<geometry::Hex> baseSet(baseTiles.begin(), baseTiles.end());

    int attackers = 0;
    int defenders = 0;
    for (const core::AgentState& agent : world.agents) {
        ASSERT_TRUE(grid.isValid(agent.position));
        if (agent.type == core::AgentType::ATTACKER) {
            ++attackers;
            EXPECT_EQ(geometry::hexDistance(geometry::Hex{0, 0}, agent.position), grid.radius());
        } else {
            ++defenders;
            EXPECT_EQ(baseSet.find(agent.position), baseSet.end());
        }
    }
    EXPECT_EQ(attackers, 4);
    EXPECT_EQ(defenders, 5);
}

TEST(InitializationTest, IsDeterministicForSameSeedAndConfig) {
    const InitializationConfig config{
        .radius = 3,
        .attackerCount = 3,
        .defenderCount = 3,
        .seed = 7U,
    };

    const InitializedEnvironment first = createInitialWorld(config);
    const InitializedEnvironment second = createInitialWorld(config);

    ASSERT_EQ(first.world.agents.size(), second.world.agents.size());
    for (std::size_t index = 0; index < first.world.agents.size(); ++index) {
        EXPECT_EQ(first.world.agents[index].id, second.world.agents[index].id);
        EXPECT_EQ(first.world.agents[index].type, second.world.agents[index].type);
        EXPECT_EQ(first.world.agents[index].position, second.world.agents[index].position);
    }
}

TEST(InitializationTest, RejectsNegativeCounts) {
    const InitializationConfig config{
        .radius = 3,
        .attackerCount = -1,
        .defenderCount = 0,
        .seed = 3U,
    };

    EXPECT_THROW(createInitialWorld(config), std::invalid_argument);
}

}  // namespace
}  // namespace perimeter::environment
