#include "perimeter/learning/nash_q_learning.h"

namespace perimeter
{

NashQLearning::NashQLearning(int id, int numAgents, float gamma,
                             const JointActionSpace& jointActionSpace, core::AgentType agentType)
    : id_{id}
    , agentType_{agentType}
    , numAgents_{numAgents}
    , gamma_{gamma}
    , randomPolicy_{std::vector<float>(static_cast<int>(environment::Action::NUM_ACTIONS),
                                        1 / static_cast<float>(environment::Action::NUM_ACTIONS))}
    , policy_{randomPolicy_}
    , jointActionSpace_{jointActionSpace}
{}

const std::function<float(JointState, JointAction)> NashQLearning::getRewardFunction()
{
  return [this](const JointState& state, const JointAction& action) { return Q(state, action); };
}

void NashQLearning::setEquilibriumPolicy(const JointPolicy& newPolicy)
{
  policy_ = newPolicy.at(id_);
}

void NashQLearning::updateJointQTable(const JointState& prevAgentStates,
                                      const JointAction& prevJointAction,
                                      const JointReward& stepRewards,
                                      const JointState& currAgentStates,
                                      const JointPolicy& jointPolicy)
{
  const JointState& s = prevAgentStates;
  const JointState& s_prime = currAgentStates;
  const JointAction& a = prevJointAction;
  float reward = stepRewards.at(id_);

  float utility{0.0};
  for (const JointAction& a_prime : jointActionSpace_.getJointActionSpace()) {
    utility += Q(s_prime, a_prime) * jointActionProbability(jointPolicy, a_prime);
  }

  float alpha = 1 / std::sqrt(static_cast<float>(N(s, a)));
  Q_s_a_[s][a] = Q(s, a) + alpha * (reward + gamma_ * utility - Q(s, a));
}

const environment::Action NashQLearning::sampleEpsGreedyPolicy(std::mt19937& rg,
                                                               const JointState& state)
{
  if (!N_s_.contains(state)) {
    N_s_[state] = 1;
  }

  float eps = 1 / N_s_[state];
  std::bernoulli_distribution dist(eps);
  if (dist(rg)) {
    return randomPolicy_.sampleAction(rg);
  }
  return policy_.sampleAction(rg);
}

void NashQLearning::updateN(const JointState& state, const JointAction& jointAction)
{
  if (!N_s_a_.contains(state) || !N_s_a_[state].contains(jointAction)) {
    N_s_a_[state][jointAction] = 0;
  }
  if (!N_s_.contains(state)) {
    N_s_[state] = 0;
  }

  N_s_a_[state][jointAction] += 1;
  N_s_[state] += 1;
}

float NashQLearning::Q(const JointState& state, const JointAction& jointAction)
{
  if (!Q_s_a_.contains(state) || !Q_s_a_[state].contains(jointAction)) {
    Q_s_a_[state][jointAction] = computeInitialQTableValue(state, jointAction);
  }
  return Q_s_a_[state][jointAction];
}

int NashQLearning::N(const JointState& state, const JointAction& jointAction)
{
  if (!N_s_a_.contains(state) || !N_s_a_[state].contains(jointAction)) {
    N_s_a_[state][jointAction] = 1;
  }
  return N_s_a_[state][jointAction];
}

float NashQLearning::computeInitialQTableValue(const JointState& state,
                                                const JointAction& jointAction) const
{
  // Bootstrap your Q table here
  return 0.0;
}

float jointActionProbability(const JointPolicy& jointPolicy, const JointAction& jointAction)
{
  if (jointPolicy.size() != jointAction.size()) {
    throw std::invalid_argument("Joint policy must be the same size as the joint action");
  }

  float product{1.0};
  for (int i{0}; i < jointPolicy.size(); ++i) {
    const SingleAgentSimpleGamePolicy& policyI = jointPolicy[i];
    product *= policyI.getProbability(jointAction[i]);
  }

  return product;
}

} // namespace perimeter
