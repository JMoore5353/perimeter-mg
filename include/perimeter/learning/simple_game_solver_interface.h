#ifndef SIMPLE_GAME_SOLVER_INTERFACE_H
#define SIMPLE_GAME_SOLVER_INTERFACE_H

#include "perimeter/learning/joint.h"

class SimpleGameSolverInterface
{
public:
  virtual JointPolicy solve() = 0;
};

#endif
