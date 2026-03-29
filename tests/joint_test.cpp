#include "perimeter/learning/joint.h"

#include <gtest/gtest.h>

namespace perimeter
{
namespace
{

TEST(JointActionSpace, ConstructionReturnsCorrectNumberOfElements)
{
  std::vector<int> testNumsOfAgents{1, 3, 5, 7};
  int numActions = static_cast<int>(environment::Action::NUM_ACTIONS);
  for (int numAgents : testNumsOfAgents) {
    JointActionSpace jas{numAgents};

    const std::vector<JointAction>& jointActions = jas.getJointActionSpace();

    ASSERT_EQ(jointActions.size(), std::pow(numActions, numAgents));
  }
}

TEST(JointActionSpace, ConstructionContainsCertainElements)
{
  int numAgents{3};
  JointActionSpace jas{numAgents};

  const std::vector<JointAction>& jointActions = jas.getJointActionSpace();

  JointAction ja1{environment::Action::EAST, environment::Action::WEST, environment::Action::STAY};
  JointAction ja2{environment::Action::NORTHEAST, environment::Action::SOUTHEAST,
                  environment::Action::STAY};
  JointAction ja3{environment::Action::SOUTHEAST, environment::Action::WEST,
                  environment::Action::WEST};
  ASSERT_NE(std::find(jointActions.begin(), jointActions.end(), ja1), jointActions.end());
  ASSERT_NE(std::find(jointActions.begin(), jointActions.end(), ja2), jointActions.end());
  ASSERT_NE(std::find(jointActions.begin(), jointActions.end(), ja3), jointActions.end());
}
} // namespace
} // namespace perimeter
