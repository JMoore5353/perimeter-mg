#ifndef NASH_Q_LEARNING_HPP
#define NASH_Q_LEARNING_HPP

#include <cmath>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "perimeter/core/AgentState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"

namespace perimeter
{

class NashQLearning
{
public:
  NashQLearning(int id, int numAgents, double gamma, const JointActionSpace& jointActionSpace);

  void computePolicy(const std::vector<core::AgentState>& agentStates);
  void updateJointQTable(const std::vector<core::AgentState>& prevAgentStates,
                         const JointAction& prevJointAction, const std::vector<double>& stepRewards,
                         const std::vector<core::AgentState>& currAgentStates,
                         const JointPolicy& jointPolicy);
  const environment::Action sampleEpsGreedyPolicy(std::mt19937& rg,
                                                  const std::vector<core::AgentState>& agentStates);
  void updateN(const std::vector<core::AgentState>& agentStates, const JointAction& jointAction);
  double Q(const core::AgentState& state, const JointAction& jointAction);

private:
  int id_;
  int numAgents_;
  double gamma_;
  SingleAgentSimpleGamePolicy randomPolicy_;
  SingleAgentSimpleGamePolicy policy_;
  const JointActionSpace& jointActionSpace_;

  std::unordered_map<core::AgentState, int> N_s_;
  std::unordered_map<core::AgentState, std::unordered_map<JointAction, int, ActionVectorHash>>
    N_s_a_;
  std::unordered_map<core::AgentState, std::unordered_map<JointAction, double, ActionVectorHash>>
    Q_s_a_;

  double computeInitialQTableValue(const core::AgentState& state,
                                   const JointAction& jointAction) const;
  int N(const core::AgentState& state, const JointAction& jointAction);
};

double jointActionProbability(const JointPolicy& jointPolicy, const JointAction& jointAction);

} // namespace perimeter

#endif
