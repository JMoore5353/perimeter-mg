#include "perimeter/core/AgentState.h"
#include "perimeter/core/AgentType.h"
#include "perimeter/environment/Action.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/nash_q_learning.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"

#include <gtest/gtest.h>
#include <random>
#include <unordered_map>

namespace perimeter
{
namespace
{

class QLearnerTest : public ::testing::Test
{
public:
  QLearnerTest()
      : q{id, numAgents, gamma, jointActionSpace, agentType}
  {}

protected:
  int id{0};
  int numAgents{2};
  float gamma{0.9};
  JointActionSpace jointActionSpace{numAgents};
  core::AgentType agentType = core::AgentType::ATTACKER;
  NashQLearning q;

  core::AgentState a0State{id, agentType, {0, 0}};
  core::AgentState a1State{id + 1, core::AgentType::DEFENDER, {1, 0}};
  JointState jointState{a0State, a1State};
  JointAction action0{environment::Action::NORTHEAST, environment::Action::STAY};
  JointAction action1{environment::Action::STAY, environment::Action::STAY};

  core::AgentState a0State1{id, agentType, {0, 1}};
  core::AgentState a1State1{id + 1, core::AgentType::DEFENDER, {1, 1}};
  JointState jointState1{a0State1, a1State1};
};

TEST_F(QLearnerTest, GetRewardFunctionReturnsAccessToQTable)
{
  float qVal0{10.0};
  float qVal1{10.0};
  QTable qTable;
  qTable[jointState][action0] = qVal0;
  qTable[jointState][action1] = qVal1;
  q.setQTable(qTable);

  auto rewardFunc = q.getRewardFunction();

  float reward0 = rewardFunc(jointState, action0);
  float reward1 = rewardFunc(jointState, action1);
  ASSERT_EQ(reward0, qVal0);
  ASSERT_EQ(reward1, qVal1);
}

TEST_F(QLearnerTest, UpdateNUpdatesTheNTables)
{
  NsTable ns = q.getNsTable();
  NsaTable nsa = q.getNsaTable();

  q.updateN(jointState, action0);

  NsTable ns2 = q.getNsTable();
  NsaTable nsa2 = q.getNsaTable();

  ASSERT_TRUE(ns.empty());
  ASSERT_TRUE(ns2.contains(jointState));
  ns2.erase(jointState);
  ASSERT_TRUE(ns2.empty());

  ASSERT_TRUE(nsa.empty());
  ASSERT_TRUE(nsa2.contains(jointState) && nsa2[jointState].contains(action0));
  nsa2.erase(jointState);
  ASSERT_TRUE(nsa2.empty());
}

TEST_F(QLearnerTest, NTablesContainCorrectValues)
{
  q.updateN(jointState, action0);
  q.updateN(jointState, action1);
  q.updateN(jointState, action1);

  NsTable ns = q.getNsTable();
  NsaTable nsa = q.getNsaTable();

  ASSERT_EQ(ns[jointState], 3);
  ASSERT_EQ(nsa[jointState][action0], 1);
  ASSERT_EQ(nsa[jointState][action1], 2);
  ASSERT_EQ(nsa[jointState][action1], 2);
}

TEST_F(QLearnerTest, NTablesInitializeToZero)
{
  NsTable ns = q.getNsTable();
  NsaTable nsa = q.getNsaTable();

  ASSERT_EQ(ns[jointState1], 0);
  ASSERT_EQ(nsa[jointState1][action0], 0);
}

TEST(QLearningJointProbability, JointProbabilityReturnsCorrectValues)
{
  // Use only the first two actions for simplicity
  JointPolicy jp{SingleAgentSimpleGamePolicy{std::vector<float>{1.0, 0.0}},
                 SingleAgentSimpleGamePolicy{std::vector<float>{0.5, 0.5}}};
  JointAction ja0{environment::Action::EAST, environment::Action::EAST};
  JointAction ja1{environment::Action::EAST, environment::Action::NORTHEAST};
  JointAction ja2{environment::Action::NORTHEAST, environment::Action::NORTHEAST};
  JointAction ja3{environment::Action::NORTHEAST, environment::Action::EAST};

  float prob0 = jointActionProbability(jp, ja0);
  float prob1 = jointActionProbability(jp, ja1);
  float prob2 = jointActionProbability(jp, ja2);
  float prob3 = jointActionProbability(jp, ja3);

  ASSERT_EQ(prob0, 0.5);
  ASSERT_EQ(prob1, 0.5);
  ASSERT_EQ(prob2, 0.);
  ASSERT_EQ(prob3, 0.);
}

TEST_F(QLearnerTest, EpsGreedyPolicyReturnsCorrectValues)
{
  int stateCount{1000};
  for (int i{0}; i < stateCount; ++i) {
    q.updateN(jointState, action0);
  }
  NsTable ns = q.getNsTable();
  ASSERT_EQ(ns[jointState], stateCount); // Important since eps = 1 / N(s)

  std::mt19937 rng{3U};
  std::vector<float> policyWeights{0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.4};
  SingleAgentSimpleGamePolicy policy{policyWeights};
  JointPolicy jp{policy, policy};
  q.setEquilibriumPolicy(jp);

  int totalActions{1000};
  std::unordered_map<environment::Action, float> actionCount;
  for (int i{0}; i < totalActions; ++i) {
    environment::Action action = q.sampleEpsGreedyPolicy(rng, jointState);
    if (!actionCount.contains(action)) {
      actionCount[action] = 0;
    }
    actionCount[action]++;
  }

  for (int i{0}; i < policyWeights.size(); ++i) {
    auto action = static_cast<environment::Action>(i);
    // Since there is techncally an eps greedy part in there (should be rather small)
    EXPECT_NEAR(policyWeights[i], actionCount[action] / totalActions, 0.05);
  }
}

TEST_F(QLearnerTest, EpsGreedyPolicyReturnsCorrectValuesWhenNotVisitedState)
{
  std::mt19937 rng{3U};
  std::vector<float> policyWeights{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
  SingleAgentSimpleGamePolicy policy{policyWeights};
  JointPolicy jp{policy, policy};
  q.setEquilibriumPolicy(jp);

  int totalActions{1000};
  std::unordered_map<environment::Action, float> actionCount;
  for (int i{0}; i < totalActions; ++i) {
    environment::Action action = q.sampleEpsGreedyPolicy(rng, jointState);
    if (!actionCount.contains(action)) {
      actionCount[action] = 0;
    }
    actionCount[action]++;
  }

  float expectedProb = 1.0 / static_cast<float>(environment::Action::NUM_ACTIONS);
  for (int i{0}; i < policyWeights.size(); ++i) {
    auto action = static_cast<environment::Action>(i);
    // Since there is techncally an eps greedy part in there (should be rather small)
    EXPECT_NEAR(expectedProb, actionCount[action] / totalActions, 0.05);
  }
}

TEST_F(QLearnerTest, ExpectJointQTableToUpdateCorrectly)
{
  float qVal0{10.0};
  float qVal1{10.0};
  QTable qTable;
  qTable[jointState][action0] = qVal0;
  qTable[jointState][action1] = qVal1;
  qTable[jointState1][action0] = 10 * qVal0;
  qTable[jointState1][action1] = 10 * qVal1;
  q.setQTable(qTable);
  EXPECT_EQ(q.getQTable(), qTable);

  JointReward rewards{10.0, -10.0};
  std::vector<float> policyWeights{0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.4};
  JointPolicy jointPolicy{SingleAgentSimpleGamePolicy{policyWeights},
                          SingleAgentSimpleGamePolicy{policyWeights}};

  q.updateJointQTable(jointState, action0, rewards, jointState1, jointPolicy);

  QTable newQTable = q.getQTable();
  EXPECT_NE(newQTable, qTable);
  EXPECT_NE(newQTable[jointState], qTable[jointState]);
  // New Q Table initializes all joint actions to zero for a given state, so can't directly compare
  auto newMap = newQTable[jointState1];
  auto oldMap = qTable[jointState1];
  for (const std::pair<JointAction, float>& entry : newMap) {
    if (oldMap.contains(entry.first)) {
      EXPECT_EQ(oldMap[entry.first], entry.second);
    } else {
      EXPECT_EQ(entry.second, 0.0);
    }
  }
}

} // namespace
} // namespace perimeter
