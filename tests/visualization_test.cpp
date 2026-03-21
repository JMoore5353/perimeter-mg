#include "visualization/AsciiViewer.h"
#include "visualization/JsonViewer.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/AgentType.h"
#include "core/WorldState.h"
#include "geometry/HexGrid.h"
#include "visualization/ViewerBackend.h"

namespace perimeter::visualization {
namespace {

TEST(VisualizationTest, BackendsAreSwappableThroughCommonInterface) {
    geometry::HexGrid grid(2);
    core::WorldState world;
    world.agents = {
        core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
    };

    std::vector<std::unique_ptr<ViewerBackend>> backends;
    backends.emplace_back(std::make_unique<AsciiViewer>());
    backends.emplace_back(std::make_unique<JsonViewer>());
    const std::vector<double> rewards{1.25, -0.5};

    for (const auto& backend : backends) {
        const std::string rendered = backend->render(world, grid, 7, rewards);
        EXPECT_FALSE(rendered.empty());
        EXPECT_NE(rendered.find("1.25"), std::string::npos);
    }
}

TEST(VisualizationTest, JsonViewerFollowsSchemaFields) {
    geometry::HexGrid grid(2);
    core::WorldState world;
    world.agents = {
        core::AgentState{42, core::AgentType::ATTACKER, geometry::Hex{2, -2}},
    };

    JsonViewer viewer;
    const std::vector<double> rewards{99.5};
    const std::string jsonStep0 = viewer.render(world, grid, 0, rewards);
    const std::string jsonStep1 = viewer.render(world, grid, 1, rewards);

    EXPECT_NE(jsonStep0.find("\"step\": 0"), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"agents\""), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"id\":42"), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"type\":\"ATTACKER\""), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"reward\":99.5"), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"base_tiles\""), std::string::npos);
    EXPECT_NE(jsonStep0.find("\"world_tiles\""), std::string::npos);

    EXPECT_NE(jsonStep1.find("\"step\": 1"), std::string::npos);
    EXPECT_NE(jsonStep1.find("\"agents\""), std::string::npos);
    EXPECT_EQ(jsonStep1.find("\"base_tiles\""), std::string::npos);
    EXPECT_EQ(jsonStep1.find("\"world_tiles\""), std::string::npos);
}

TEST(VisualizationTest, AsciiViewerIncludesStepAndSymbols) {
    geometry::HexGrid grid(2);
    core::WorldState world;
    world.agents = {
        core::AgentState{1, core::AgentType::ATTACKER, geometry::Hex{0, 0}},
        core::AgentState{2, core::AgentType::DEFENDER, geometry::Hex{1, -1}},
    };

    AsciiViewer viewer;
    const std::vector<double> rewards{-2.0, 3.75};
    const std::string ascii = viewer.render(world, grid, 3, rewards);

    EXPECT_NE(ascii.find("step: 3"), std::string::npos);
    EXPECT_NE(ascii.find('A'), std::string::npos);
    EXPECT_NE(ascii.find('D'), std::string::npos);
    EXPECT_NE(ascii.find("rewards:"), std::string::npos);
    EXPECT_NE(ascii.find("id=1 reward=-2"), std::string::npos);
    EXPECT_NE(ascii.find("id=2 reward=3.75"), std::string::npos);
}

}  // namespace
}  // namespace perimeter::visualization
