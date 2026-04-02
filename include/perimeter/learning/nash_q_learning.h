#ifndef NASH_Q_LEARNING_HPP
#define NASH_Q_LEARNING_HPP

#include <cmath>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "perimeter/environment/Action.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"

namespace perimeter
{

class NashQLearning
{
public:
  NashQLearning(int id, int numAgents, double gamma, const JointActionSpace& jointActionSpace);

  const std::function<double(JointState, JointAction)> getRewardFunction();

  void setEquilibriumPolicy(const JointPolicy& newPolicy);
  void updateJointQTable(const JointState& prevAgentStates, const JointAction& prevJointAction,
                         const std::vector<double>& stepRewards, const JointState& currAgentStates,
                         const JointPolicy& jointPolicy);
  const environment::Action sampleEpsGreedyPolicy(std::mt19937& rg, const JointState& agentStates);
  void updateN(const JointState& agentStates, const JointAction& jointAction);
  double Q(const JointState& state, const JointAction& jointAction);

private:
  int id_;
  int numAgents_;
  double gamma_;
  SingleAgentSimpleGamePolicy randomPolicy_;
  SingleAgentSimpleGamePolicy policy_;
  const JointActionSpace& jointActionSpace_;

  std::unordered_map<JointState, int, StateVectorHash> N_s_;
  std::unordered_map<JointState, std::unordered_map<JointAction, int, ActionVectorHash>,
                     StateVectorHash>
    N_s_a_;
  std::unordered_map<JointState, std::unordered_map<JointAction, double, ActionVectorHash>,
                     StateVectorHash>
    Q_s_a_;

  double computeInitialQTableValue(const JointState& state, const JointAction& jointAction) const;
  int N(const JointState& state, const JointAction& jointAction);
};

double jointActionProbability(const JointPolicy& jointPolicy, const JointAction& jointAction);

} // namespace perimeter

#endif
