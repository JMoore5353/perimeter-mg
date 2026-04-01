#include "perimeter/learning/nash_q_learning.h"
#include <stdexcept>

namespace perimeter
{

NashQLearning::NashQLearning(int id, int numAgents, double gamma,
                             const JointActionSpace& jointActionSpace)
    : id_{id}
    , numAgents_{numAgents}
    , gamma_{gamma}
    , randomPolicy_{std::vector<double>(static_cast<int>(environment::Action::NUM_ACTIONS),
                                        1 / static_cast<double>(environment::Action::NUM_ACTIONS))}
    , policy_{randomPolicy_}
    , jointActionSpace_{jointActionSpace}
{}

void NashQLearning::setEquilibriumPolicy(const JointPolicy& newPolicy)
{
  // TODO: NEXT STEPS: Add unit tests to this qlearning module. (Ask AI? I'm not sure really the best way to test this)
  policy_ = newPolicy.at(id_);
}

void NashQLearning::updateJointQTable(const std::vector<core::AgentState>& prevAgentStates,
                                      const JointAction& prevJointAction,
                                      const std::vector<double>& stepRewards,
                                      const std::vector<core::AgentState>& currAgentStates,
                                      const JointPolicy& jointPolicy)
{
  const core::AgentState& s = prevAgentStates.at(id_);
  const core::AgentState& s_prime = currAgentStates.at(id_);
  const JointAction& a = prevJointAction;
  double reward = stepRewards.at(id_);

  double utility{0.0};
  for (const JointAction& a_prime : jointActionSpace_.getJointActionSpace()) {
    // TODO: You could pass in a single vector that has precomputed the jointActionProbability for each joint action in the jointaction space. Would save some time.
    utility += Q(s_prime, a_prime) * jointActionProbability(jointPolicy, a_prime);
  }

  double alpha = 1 / std::sqrt(static_cast<double>(N(s, a)));
  Q_s_a_[s][a] = Q(s, a) + alpha * (reward + gamma_ * utility - Q(s, a));
}

const environment::Action
NashQLearning::sampleEpsGreedyPolicy(std::mt19937& rg,
                                     const std::vector<core::AgentState>& agentStates)
{
  const core::AgentState& selfState = agentStates[id_];
  if (!N_s_.contains(selfState)) {
    N_s_[selfState] = 1;
  }

  double eps = 1 / N_s_[selfState];
  std::bernoulli_distribution dist(eps);
  if (dist(rg) <= eps) {
    return randomPolicy_.sampleAction(rg);
  }
  return policy_.sampleAction(rg);
}

void NashQLearning::updateN(const std::vector<core::AgentState>& agentStates,
                            const JointAction& jointAction)
{
  const core::AgentState state = agentStates.at(id_);

  if (!N_s_a_.contains(state) || !N_s_a_[state].contains(jointAction)) {
    N_s_a_[state][jointAction] = 1;
  } else {
    N_s_a_[state][jointAction] += 1;
  }
}

double NashQLearning::Q(const core::AgentState& state, const JointAction& jointAction)
{
  if (!Q_s_a_.contains(state) || !Q_s_a_[state].contains(jointAction)) {
    Q_s_a_[state][jointAction] = computeInitialQTableValue(state, jointAction);
  }
  return Q_s_a_[state][jointAction];
}

int NashQLearning::N(const core::AgentState& state, const JointAction& jointAction)
{
  if (!N_s_a_.contains(state) || !N_s_a_[state].contains(jointAction)) {
    N_s_a_[state][jointAction] = 1;
  }
  return N_s_a_[state][jointAction];
}

double NashQLearning::computeInitialQTableValue(const core::AgentState& state,
                                                const JointAction& jointAction) const
{
  return 0.0;
}

double jointActionProbability(const JointPolicy& jointPolicy, const JointAction& jointAction)
{
  if (jointPolicy.size() != jointAction.size()) {
    throw std::invalid_argument("Joint policy must be the same size as the joint action");
  }

  double product{1.0};
  for (int i{0}; i < jointPolicy.size(); ++i) {
    const SingleAgentSimpleGamePolicy& policyI = jointPolicy[i];
    product *= policyI.getProbability(jointAction[i]);
  }

  return product;
}

} // namespace perimeter
