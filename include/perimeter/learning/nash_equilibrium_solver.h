#ifndef NASH_EQUILIBRIUM_SOLVER_H
#define NASH_EQUILIBRIUM_SOLVER_H

#include "perimeter/core/AgentState.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/simple_game_solver_interface.h"
#include "perimeter/learning/single_agent_simple_game_policy.h"

namespace perimeter {

class NashEquilibriumSolver : public SimpleGameSolverInterface
{
public:
  NashEquilibriumSolver() = default;

  JointPolicy solve(const std::function<JointReward(JointAction)>& R,
                    const JointActionSpace& jointActionSpace,
                    const std::vector<core::AgentState>& agents) override final;

private:
};

} // namespace perimeter

#endif
