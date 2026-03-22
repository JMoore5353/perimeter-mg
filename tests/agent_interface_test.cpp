#include "perimeter/core/Agent.h"

#include <gtest/gtest.h>

namespace perimeter::core {
namespace {

class FixedActionAgent final : public Agent {
public:
    explicit FixedActionAgent(environment::Action action) : action_(action) {}

    environment::Action act(const WorldState& world) override {
        lastObservedAgentCount_ = world.agents.size();
        return action_;
    }

    [[nodiscard]] std::size_t lastObservedAgentCount() const noexcept {
        return lastObservedAgentCount_;
    }

private:
    environment::Action action_;
    std::size_t lastObservedAgentCount_{0};
};

TEST(AgentInterfaceTest, DerivedAgentCanObserveWorldAndReturnAction) {
    WorldState world;
    world.agents = {
        AgentState{1, AgentType::ATTACKER, geometry::Hex{0, 0}},
        AgentState{2, AgentType::DEFENDER, geometry::Hex{1, -1}},
    };

    FixedActionAgent agent(environment::Action::NORTHEAST);
    const environment::Action action = agent.act(world);

    EXPECT_EQ(action, environment::Action::NORTHEAST);
    EXPECT_EQ(agent.lastObservedAgentCount(), 2U);
}

}  // namespace
}  // namespace perimeter::core
