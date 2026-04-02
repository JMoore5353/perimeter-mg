#include "perimeter/learning/nash_equilibrium_solver.h"

#include <cmath>
#include <gtest/gtest.h>

#include "perimeter/core/AgentState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter {
namespace {

using core::AgentState;
using core::AgentType;
using environment::Action;
using geometry::Hex;

// Helper function to check if probabilities sum to 1.0
bool checkProbabilitySumToOne(const SingleAgentSimpleGamePolicy& policy, double tolerance = 1e-6) {
  double sum = 0.0;
  for (int a = 0; a < static_cast<int>(Action::NUM_ACTIONS); ++a) {
    sum += policy.getProbability(static_cast<Action>(a));
  }
  return std::abs(sum - 1.0) < tolerance;
}

// Helper function to check if all probabilities are non-negative
bool checkNonNegativeProbabilities(const SingleAgentSimpleGamePolicy& policy) {
  for (int a = 0; a < static_cast<int>(Action::NUM_ACTIONS); ++a) {
    if (policy.getProbability(static_cast<Action>(a)) < 0.0) {
      return false;
    }
  }
  return true;
}

// Helper to compute expected utility for an agent given policies
double computeExpectedUtility(const JointRewardFunction& R,
                              const JointActionSpace& jointActionSpace,
                              const JointPolicy& policy,
                              int agentIdx,
                              const JointState& state) {
  double utility = 0.0;
  const auto& allJointActions = jointActionSpace.getJointActionSpace();
  
  for (const auto& jointAction : allJointActions) {
    // Compute joint probability
    double jointProb = 1.0;
    for (size_t j = 0; j < policy.size(); ++j) {
      jointProb *= policy[j].getProbability(jointAction[j]);
    }
    
    // Get reward for this agent
    double reward = R[agentIdx](state, jointAction);
    utility += reward * jointProb;
  }
  
  return utility;
}

// Helper to check if policy is a Nash equilibrium (no profitable deviations)
bool isNashEquilibrium(const JointRewardFunction& R,
                       const JointActionSpace& jointActionSpace,
                       const JointPolicy& policy,
                       const JointState& state,
                       double tolerance = 1e-3) {
  int numAgents = policy.size();
  
  // For each agent, check if they can profitably deviate
  for (int i = 0; i < numAgents; ++i) {
    double currentUtility = computeExpectedUtility(R, jointActionSpace, policy, i, state);
    
    // Check all possible pure strategy deviations
    for (int a = 0; a < static_cast<int>(Action::NUM_ACTIONS); ++a) {
      // Create deviated policy where agent i plays pure strategy a
      JointPolicy deviatedPolicy = policy;
      deviatedPolicy[i] = SingleAgentSimpleGamePolicy(static_cast<Action>(a));
      
      double deviationUtility = computeExpectedUtility(R, jointActionSpace, deviatedPolicy, i, state);
      
      // If deviation is profitable (beyond tolerance), not a Nash equilibrium
      if (deviationUtility > currentUtility + tolerance) {
        return false;
      }
    }
  }
  
  return true;
}

TEST(NashEquilibriumSolverTest, SolverReturnsValidPolicies) {
  // Simple constant reward game
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double { return 1.0; },
    [](const JointState& js, const JointAction& ja) -> double { return 1.0; }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Check that probabilities sum to 1 for each agent
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  
  // Check non-negativity
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
}

TEST(NashEquilibriumSolverTest, CoordinationGameHasMultipleEquilibria) {
  // Coordination game: both agents get reward 2 if they choose same action, 0 otherwise
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      return (ja[0] == ja[1]) ? 2.0 : 0.0;
    },
    [](const JointState& js, const JointAction& ja) -> double {
      return (ja[0] == ja[1]) ? 2.0 : 0.0;
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Verify valid policies
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
  
  // Verify it's a Nash equilibrium
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
}

TEST(NashEquilibriumSolverTest, MatchingPenniesHasMixedEquilibrium) {
  // Matching pennies: Agent 0 wants to match, Agent 1 wants to mismatch
  // Using only first two actions (EAST and NORTHEAST) for simplicity
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Only consider first two actions
      if (action0 >= 2 || action1 >= 2) {
        return -10.0;  // Discourage other actions
      }
      
      if (action0 == action1) {
        return 1.0;  // Agent 0 wins
      } else {
        return -1.0;
      }
    },
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Only consider first two actions
      if (action0 >= 2 || action1 >= 2) {
        return -10.0;  // Discourage other actions
      }
      
      if (action0 == action1) {
        return -1.0;  // Agent 1 loses
      } else {
        return 1.0;  // Agent 1 wins
      }
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Verify valid policies
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
  
  // In matching pennies, Nash equilibrium should be mixed (approximately 50-50 on the two actions)
  double prob0_action0 = policy[0].getProbability(Action::EAST);
  double prob0_action1 = policy[0].getProbability(Action::NORTHEAST);
  double prob1_action0 = policy[1].getProbability(Action::EAST);
  double prob1_action1 = policy[1].getProbability(Action::NORTHEAST);
  
  // Most probability should be on first two actions
  EXPECT_GT(prob0_action0 + prob0_action1, 0.9);
  EXPECT_GT(prob1_action0 + prob1_action1, 0.9);
  
  // Should be approximately balanced (within 30% of 50-50 due to solver tolerance)
  EXPECT_NEAR(prob0_action0, prob0_action1, 0.3);
  EXPECT_NEAR(prob1_action0, prob1_action1, 0.3);
}

TEST(NashEquilibriumSolverTest, PrisonersDilemmaFindsEquilibrium) {
  // Prisoner's dilemma: defect (action 0) dominates cooperate (action 1)
  // Using only first two actions
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Discourage other actions
      if (action0 >= 2 || action1 >= 2) {
        return -100.0;
      }
      
      // action 0 = defect, action 1 = cooperate
      if (action0 == 1 && action1 == 1) {  // both cooperate
        return 3.0;
      } else if (action0 == 0 && action1 == 1) {  // agent 0 defects, agent 1 cooperates
        return 5.0;
      } else if (action0 == 1 && action1 == 0) {  // agent 0 cooperates, agent 1 defects
        return 0.0;
      } else {  // both defect
        return 1.0;
      }
    },
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Discourage other actions
      if (action0 >= 2 || action1 >= 2) {
        return -100.0;
      }
      
      // action 0 = defect, action 1 = cooperate
      if (action0 == 1 && action1 == 1) {  // both cooperate
        return 3.0;
      } else if (action0 == 0 && action1 == 1) {  // agent 0 defects, agent 1 cooperates
        return 0.0;
      } else if (action0 == 1 && action1 == 0) {  // agent 0 cooperates, agent 1 defects
        return 5.0;
      } else {  // both defect
        return 1.0;
      }
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Verify valid policies
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
  
  // Verify it's a Nash equilibrium
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
  
  // Nash equilibrium in prisoner's dilemma should heavily favor defecting (action 0)
  double prob0_defect = policy[0].getProbability(Action::EAST);
  double prob1_defect = policy[1].getProbability(Action::EAST);
  
  EXPECT_GT(prob0_defect, 0.5);
  EXPECT_GT(prob1_defect, 0.5);
}

TEST(NashEquilibriumSolverTest, ThreePlayerGameFindsEquilibrium) {
  // Simple three-player game
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      // Reward agents for coordinating on same action
      if (ja[0] == ja[1] && ja[1] == ja[2]) {
        return 3.0;
      }
      // Partial coordination
      if (ja[0] == ja[1] || ja[1] == ja[2] || ja[0] == ja[2]) {
        return 1.0;
      }
      return 0.0;
    },
    [](const JointState& js, const JointAction& ja) -> double {
      // Reward agents for coordinating on same action
      if (ja[0] == ja[1] && ja[1] == ja[2]) {
        return 3.0;
      }
      // Partial coordination
      if (ja[0] == ja[1] || ja[1] == ja[2] || ja[0] == ja[2]) {
        return 1.0;
      }
      return 0.0;
    },
    [](const JointState& js, const JointAction& ja) -> double {
      // Reward agents for coordinating on same action
      if (ja[0] == ja[1] && ja[1] == ja[2]) {
        return 3.0;
      }
      // Partial coordination
      if (ja[0] == ja[1] || ja[1] == ja[2] || ja[0] == ja[2]) {
        return 1.0;
      }
      return 0.0;
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}},
    AgentState{2, AgentType::ATTACKER, Hex{2, 0}}
  };
  
  JointActionSpace jointActionSpace(3);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 3U);
  
  // Verify valid policies
  for (size_t i = 0; i < policy.size(); ++i) {
    EXPECT_TRUE(checkProbabilitySumToOne(policy[i]));
    EXPECT_TRUE(checkNonNegativeProbabilities(policy[i]));
  }
  
  // Verify it's a Nash equilibrium
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
}

TEST(NashEquilibriumSolverTest, PureStrategyDominanceIsRespected) {
  // Game where action 0 strictly dominates all other actions for agent 0
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      // Agent 0 always gets more reward with action 0
      return (action0 == 0) ? 10.0 : 1.0;
    },
    [](const JointState& js, const JointAction& ja) -> double {
      int action1 = static_cast<int>(ja[1]);
      // Agent 1's reward is independent
      return (action1 == 0) ? 5.0 : 2.0;
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Agent 0 should play action 0 with high probability (dominant strategy)
  double prob0_action0 = policy[0].getProbability(Action::EAST);
  EXPECT_GT(prob0_action0, 0.9);
  
  // Agent 1 should also play action 0 (dominant strategy)
  double prob1_action0 = policy[1].getProbability(Action::EAST);
  EXPECT_GT(prob1_action0, 0.9);
  
  // Verify it's a Nash equilibrium
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
}

TEST(NashEquilibriumSolverTest, AsymmetricGameFindsEquilibrium) {
  // Asymmetric game where agents have different roles
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Limit to first 3 actions for faster testing
      if (action0 >= 3 || action1 >= 3) {
        return -50.0;
      }
      
      // Agent 0 is a "leader", agent 1 is a "follower"
      // Leader prefers action 0, follower should respond optimally
      if (action0 == 0 && action1 == 0) {
        return 5.0;  // Leader happy
      } else if (action0 == 0 && action1 == 1) {
        return 3.0;  // Leader okay
      } else if (action0 == 1 && action1 == 1) {
        return 2.0;  // Leader okay
      } else {
        return 1.0;  // Leader unhappy
      }
    },
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Limit to first 3 actions for faster testing
      if (action0 >= 3 || action1 >= 3) {
        return -50.0;
      }
      
      // Agent 1 is a "follower"
      if (action0 == 0 && action1 == 0) {
        return 3.0;  // Follower okay
      } else if (action0 == 0 && action1 == 1) {
        return 1.0;  // Follower worse
      } else if (action0 == 1 && action1 == 1) {
        return 4.0;  // Follower happy
      } else {
        return 1.0;  // Follower unhappy
      }
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Verify valid policies
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
  
  // Verify it's a Nash equilibrium
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
}

TEST(NashEquilibriumSolverTest, ZeroSumGameFindsEquilibrium) {
  // Zero-sum game: agent 0's gain is agent 1's loss
  JointRewardFunction R = {
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Only use first 3 actions
      if (action0 >= 3 || action1 >= 3) {
        return -100.0;
      }
      
      // Rock-paper-scissors style payoffs
      int diff = (action0 - action1 + 3) % 3;
      if (diff == 0) {
        return 0.0;  // tie
      } else if (diff == 1) {
        return 1.0;  // agent 0 wins
      } else {
        return -1.0;  // agent 1 wins
      }
    },
    [](const JointState& js, const JointAction& ja) -> double {
      int action0 = static_cast<int>(ja[0]);
      int action1 = static_cast<int>(ja[1]);
      
      // Only use first 3 actions
      if (action0 >= 3 || action1 >= 3) {
        return 100.0;
      }
      
      // Rock-paper-scissors style payoffs (opposite of agent 0)
      int diff = (action0 - action1 + 3) % 3;
      if (diff == 0) {
        return 0.0;  // tie
      } else if (diff == 1) {
        return -1.0;  // agent 0 wins (agent 1 loses)
      } else {
        return 1.0;  // agent 1 wins
      }
    }
  };
  
  std::vector<AgentState> agents = {
    AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
    AgentState{1, AgentType::DEFENDER, Hex{1, 0}}
  };
  
  JointActionSpace jointActionSpace(2);
  NashEquilibriumSolver solver;
  JointPolicy policy = solver.solve(R, jointActionSpace, agents);
  
  ASSERT_EQ(policy.size(), 2U);
  
  // Verify valid policies
  EXPECT_TRUE(checkProbabilitySumToOne(policy[0]));
  EXPECT_TRUE(checkProbabilitySumToOne(policy[1]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[0]));
  EXPECT_TRUE(checkNonNegativeProbabilities(policy[1]));
  
  // Verify it's a Nash equilibrium (most important check)
  EXPECT_TRUE(isNashEquilibrium(R, jointActionSpace, policy, agents));
  
  // Note: In rock-paper-scissors, the Nash equilibrium is uniform over the three actions,
  // but due to numerical optimization, the solver might find slightly different distributions
  // The key is that it satisfies the Nash equilibrium condition
}

}  // namespace
}  // namespace perimeter
