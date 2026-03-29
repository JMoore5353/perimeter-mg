#include "perimeter/environment/Action.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"
#include <gtest/gtest.h>
#include <random>

namespace perimeter
{
namespace
{

TEST(SimpleGamePolicy, TestDeterministicActionIsDeterministic)
{
  SingleAgentSimpleGamePolicy policy(environment::Action::EAST);
  std::mt19937 rg(123);

  for (int i{0}; i < 1000; ++i) {
    environment::Action action = policy.sampleAction(rg);
    ASSERT_EQ(action, environment::Action::EAST);
  }
}

TEST(SimpleGamePolicy, TestRandomActionsHaveCorrectFrequency)
{
  std::vector<double> weights = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.4};
  SingleAgentSimpleGamePolicy policy(weights);
  std::mt19937 rg(3);

  std::vector<int> countFreq(sizeof(environment::Action), 0);
  int totalRuns{5000};
  for (int i{0}; i < totalRuns; ++i) {
    int idx = static_cast<int>(policy.sampleAction(rg));
    countFreq[idx] += 1;
  }

  for (int i{0}; i < weights.size(); ++i) {
    double experimentalFrequency = static_cast<double>(countFreq[i]) / totalRuns;
    EXPECT_NEAR(experimentalFrequency, weights[i], 0.05);
  }
}

TEST(SimpleGamePolicy, WeightsAreNormalizedCorrectly)
{
  std::vector<double> weights{1, 1, 1, 1, 1, 1, 4};
  SingleAgentSimpleGamePolicy policy{weights};
  std::vector<double> normalizedWeights{0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.4};

  for (int i = 0; i < sizeof(environment::Action); ++i) {
    environment::Action action = static_cast<environment::Action>(i);
    double prob = policy.getProbability(action);

    EXPECT_EQ(normalizedWeights[i], prob);
  }
}

TEST(SimpleGamePolicy, TestGetProbabilityReturnsCorrectProbability)
{
  std::vector<double> weights{0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.4};
  SingleAgentSimpleGamePolicy policy{weights};

  for (int i = 0; i < sizeof(environment::Action); ++i) {
    environment::Action action = static_cast<environment::Action>(i);
    double prob = policy.getProbability(action);

    EXPECT_EQ(weights[i], prob);
  }
}

} // namespace
} // namespace perimeter
