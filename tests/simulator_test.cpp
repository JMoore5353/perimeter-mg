#include "perimeter/environment/Simulator.h"

#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "perimeter/environment/Action.h"
#include "perimeter/environment/Initialization.h"

namespace perimeter::environment
{
namespace
{

TEST(SimulatorTest, StepReturnsRewardVectorAlignedWithAgents)
{
  const InitializationConfig config{
    .radius = 3,
    .attackerCount = 2,
    .defenderCount = 2,
    .seed = 5U,
  };
  InitializedEnvironment initialized = createInitialWorld(config);
  Simulator simulator(std::move(initialized.grid), std::move(initialized.world), 17U);

  const std::vector<Action> actions{
    Action::STAY,
    Action::STAY,
    Action::STAY,
    Action::STAY,
  };

  const StepResult result = simulator.step(actions);
  EXPECT_EQ(result.rewards.size(), simulator.world().agents.size());
}

TEST(SimulatorTest, SteppingIsDeterministicForSameSeedAndInputs)
{
  const InitializationConfig config{
    .radius = 3,
    .attackerCount = 2,
    .defenderCount = 2,
    .seed = 9U,
  };

  InitializedEnvironment initA = createInitialWorld(config);
  InitializedEnvironment initB = createInitialWorld(config);

  Simulator simA(std::move(initA.grid), std::move(initA.world), 123U);
  Simulator simB(std::move(initB.grid), std::move(initB.world), 123U);

  const std::vector<Action> actions{
    Action::EAST,
    Action::WEST,
    Action::STAY,
    Action::NORTHEAST,
  };

  const StepResult resultA = simA.step(actions);
  const StepResult resultB = simB.step(actions);

  ASSERT_EQ(resultA.rewards.size(), resultB.rewards.size());
  for (std::size_t i = 0; i < resultA.rewards.size(); ++i) {
    EXPECT_DOUBLE_EQ(resultA.rewards[i], resultB.rewards[i]);
  }

  ASSERT_EQ(simA.world().agents.size(), simB.world().agents.size());
  for (std::size_t i = 0; i < simA.world().agents.size(); ++i) {
    EXPECT_EQ(simA.world().agents[i].position, simB.world().agents[i].position);
  }
}

TEST(SimulatorTest, GetRewardFunctionReturnsCallableFunction)
{
  const InitializationConfig config{
    .radius = 3,
    .attackerCount = 1,
    .defenderCount = 1,
    .seed = 42U,
  };
  InitializedEnvironment initialized = createInitialWorld(config);
  Simulator simulator(std::move(initialized.grid), std::move(initialized.world), 17U);

  auto rewardFunction = simulator.getRewardFunction();

  const perimeter::JointAction jointAction{Action::STAY, Action::STAY};
  const perimeter::JointReward rewards = rewardFunction(jointAction);

  EXPECT_EQ(rewards.size(), simulator.world().agents.size());
}

TEST(SimulatorTest, GetRewardFunctionDoesNotModifySimulatorState)
{
  const InitializationConfig config{
    .radius = 3,
    .attackerCount = 1,
    .defenderCount = 1,
    .seed = 42U,
  };
  InitializedEnvironment initialized = createInitialWorld(config);
  Simulator simulator(std::move(initialized.grid), std::move(initialized.world), 17U);

  const geometry::Hex originalAttackerPos = simulator.world().agents[0].position;
  const geometry::Hex originalDefenderPos = simulator.world().agents[1].position;

  auto rewardFunction = simulator.getRewardFunction();
  const perimeter::JointAction jointAction{Action::EAST, Action::WEST};
  const perimeter::JointReward rewards = rewardFunction(jointAction);

  // Verify state unchanged
  EXPECT_EQ(simulator.world().agents[0].position, originalAttackerPos);
  EXPECT_EQ(simulator.world().agents[1].position, originalDefenderPos);
}

} // namespace
} // namespace perimeter::environment
