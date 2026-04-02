#ifndef CORRELATED_EQUILIBRIUM_SOLVER_H
#define CORRELATED_EQUILIBRIUM_SOLVER_H

#include "perimeter/learning/joint.h"
#include "perimeter/learning/simple_game_solver_interface.h"
#include "perimeter/single_agent_simple_game_policy.h"

class CorrelatedEquilibriumSolver : public SimpleGameSolverInterface
{
public:
  CorrelatedEquilibriumSolver() = default;

  JointPolicy solve(const JointRewardFunction& R, const JointActionSpace& jointActionSpace,
                    const JointState& state) override final;

private:
};

#endif
