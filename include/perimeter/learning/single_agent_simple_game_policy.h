#ifndef SINGLE_AGENT_SIMPLE_GAME_POLICY_HPP
#define SINGLE_AGENT_SIMPLE_GAME_POLICY_HPP

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "perimeter/environment/Action.h"

namespace perimeter
{

class SingleAgentSimpleGamePolicy
{
public:
  SingleAgentSimpleGamePolicy(const std::vector<float>& weights);
  SingleAgentSimpleGamePolicy(environment::Action deterministicAction);

  environment::Action sampleAction(std::mt19937& rg);
  float getProbability(const environment::Action action) const;

private:
  std::vector<float> weights_;
  std::discrete_distribution<int> dist_;
};

} // namespace perimeter

#endif
