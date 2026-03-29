#include "perimeter/learning/joint.h"

namespace perimeter
{

JointActionSpace::JointActionSpace(const int numAgents)
{
  enumerateJointActionSpace(numAgents);
}

void JointActionSpace::enumerateJointActionSpace(const int numAgents)
{
  std::vector<environment::Action> possibleActions;
  for (int i = 0; i < sizeof(environment::Action); ++i) {
    possibleActions.push_back(static_cast<environment::Action>(i));
  }

  allJointActions_.clear();
  for (auto action : possibleActions) {
    std::vector<environment::Action> jointAction{action};
    addNewAgentToJointAction(jointAction, possibleActions, numAgents - 1);
  }
}

void JointActionSpace::addNewAgentToJointAction(
  const std::vector<environment::Action>& jointAction,
  const std::vector<environment::Action>& possibleActions, const int remainingAgentsToAdd)
{
  if (remainingAgentsToAdd == 1) {
    for (auto a_prime : possibleActions) {
      std::vector<environment::Action> newJointAction(jointAction);
      newJointAction.push_back(a_prime);
      allJointActions_.push_back(newJointAction);
    }
    return;
  }

  for (auto a_prime : possibleActions) {
    std::vector<environment::Action> newJointAction(jointAction);
    newJointAction.push_back(a_prime);
    addNewAgentToJointAction(newJointAction, possibleActions, remainingAgentsToAdd - 1);
  }
}

} // namespace perimeter
