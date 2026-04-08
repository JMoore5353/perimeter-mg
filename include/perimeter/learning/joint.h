#ifndef JOINT_ACTION_H
#define JOINT_ACTION_H

#include <functional>
#include <type_traits>
#include <vector>

#include "perimeter/core/AgentState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"

namespace perimeter
{

using JointAction = std::vector<environment::Action>;
struct ActionVectorHash
{
  size_t operator()(const JointAction& ja) const noexcept
  {
    size_t seed = 0;

    auto combine = [](size_t& seed, size_t value) {
      seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    };

    using Underlying = std::underlying_type_t<perimeter::environment::Action>;

    for (const auto& action : ja) {
      size_t h = std::hash<Underlying>{}(static_cast<Underlying>(action));
      combine(seed, h);
    }

    return seed;
  }
};

using JointPolicy = std::vector<SingleAgentSimpleGamePolicy>;
using JointReward = std::vector<float>;

using JointState = std::vector<core::AgentState>;
struct StateVectorHash
{
  size_t operator()(const JointState& js) const noexcept
  {
    size_t seed = 0;

    auto combine = [](size_t& seed, size_t value) {
      seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    };

    for (const auto& state : js) {
      size_t h = std::hash<core::AgentState>{}(state);
      combine(seed, h);
    }

    return seed;
  }
};

using JointRewardFunction = std::vector<std::function<float(JointState, JointAction)>>;

class JointActionSpace
{
public:
  JointActionSpace(const int numAgents);
  const std::vector<JointAction>& getJointActionSpace() const
  {
    return allJointActions_;
  }

private:
  std::vector<JointAction> allJointActions_;
  void enumerateJointActionSpace(const int numAgents);
  void addNewAgentToJointAction(const std::vector<environment::Action>& jointAction,
                                const std::vector<environment::Action>& possibleActions,
                                const int remainingAgentsToAdd);
};

} // namespace perimeter

#endif
