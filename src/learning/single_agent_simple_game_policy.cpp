#include "perimeter/learning/single_agent_simple_game_policy.h"

namespace perimeter
{

SingleAgentSimpleGamePolicy::SingleAgentSimpleGamePolicy(const std::vector<double>& weights)
    : weights_{weights.begin(), weights.end()}
    , dist_{weights.begin(), weights.end()}
{
  double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
  std::transform(weights.begin(), weights.end(), weights_.begin(),
                 [&sum](double val) { return val / sum; });
}

SingleAgentSimpleGamePolicy::SingleAgentSimpleGamePolicy(environment::Action deterministicAction)
{
  weights_ = std::vector<double>(static_cast<int>(environment::Action::NUM_ACTIONS), 0.0);
  weights_[static_cast<int>(deterministicAction)] = 1.0;
  dist_ = std::discrete_distribution<>(weights_.begin(), weights_.end());
}

environment::Action SingleAgentSimpleGamePolicy::sampleAction(std::mt19937& rg)
{
  return static_cast<environment::Action>(dist_(rg));
}

double SingleAgentSimpleGamePolicy::getProbability(const environment::Action action) const
{
  int idx = static_cast<int>(action);
  if (idx >= weights_.size()) {
    throw std::invalid_argument("Action not in weight vector!");
  }
  return weights_[idx];
}

} // namespace perimeter
