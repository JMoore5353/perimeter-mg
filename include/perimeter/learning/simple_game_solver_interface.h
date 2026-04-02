#ifndef SIMPLE_GAME_SOLVER_INTERFACE_H
#define SIMPLE_GAME_SOLVER_INTERFACE_H

#include "perimeter/core/AgentState.h"
#include "perimeter/learning/joint.h"

namespace perimeter
{

class SimpleGameSolverInterface
{
public:
  /*
  *  Solves a simple game, in the sense that it provides a (somewhat) optimal solution
  *  @param R is the joint reward function that takes in a JointAction and returns rewards for each agent
  *  @param jointActionSpace enumerates all possible JointActions. Use this object to iterate over JointActions.
  *  @param agents is a vector of AgentStates that contains the type of each agent (attacker vs defender)
  *  @return JointPolicy, one SingleAgentSimpleGamePolicy per agent.
  */
  virtual JointPolicy solve(const JointRewardFunction& R, const JointActionSpace& jointActionSpace,
                            const JointState& state) = 0;
  virtual ~SimpleGameSolverInterface() = default;
};

} // namespace perimeter

#endif
