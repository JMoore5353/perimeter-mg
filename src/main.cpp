#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/environment/Initialization.h"
#include "perimeter/environment/Simulator.h"
#include "perimeter/geometry/HexGrid.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/nash_q_learning.h"
#include "perimeter/visualization/JsonViewer.h"

namespace perimeter
{

JointPolicy getRandomJointPolicy(std::mt19937& rng, const std::size_t numAgents)
{
  return JointPolicy(
    numAgents,
    std::vector<double>(static_cast<int>(environment::Action::NUM_ACTIONS),
                        1 / static_cast<double>(environment::Action::NUM_ACTIONS)));
}

void runSim(const int endT, const std::string filename)
{
  int t{0};
  std::ofstream outFile;
  outFile.open(filename);

  const environment::InitializationConfig config{.radius{3},
                                                 .attackerCount{2},
                                                 .defenderCount{4},
                                                 .seed{123U}};
  environment::InitializedEnvironment initialized = createInitialWorld(config);
  environment::Simulator simulator{std::move(initialized.grid), std::move(initialized.world),
                                   config.seed};
  std::cout << "Initialized environment with " << config.attackerCount << " attackers and "
            << config.defenderCount << " defenders." << std::endl;

  visualization::JsonViewer jsonViewer;
  std::mt19937 rng(config.seed);

  std::cout << "Computing joint action space...";
  int numAgents = simulator.world().agents.size();
  double gamma = 0.9;
  JointActionSpace jointActionSpace{numAgents};
  std::cout << " Done!" << std::endl;
  std::vector<NashQLearning> qLearners;
  for (int i{0}; i < numAgents; ++i) {
    qLearners.emplace_back(i, numAgents, gamma, jointActionSpace);
  }

  std::vector<core::AgentState> currAgentStates = simulator.world().agents;
  std::vector<core::AgentState> prevAgentStates = currAgentStates;
  JointAction prevJointAction(numAgents, environment::Action::STAY);
  std::vector<double> stepRewards(numAgents, 0.0);
  while (t < endT) {
    std::ostringstream outPrefix;
    outPrefix << "\rStarting simulation step " << t << "...";
    std::cout << outPrefix.str() << std::flush;

    // Compute stochastic policy (centralized module that computes Nash Equilibrium for simple game)
    std::cout << outPrefix.str() << " Computing stochastic policy..." << std::flush;
    // JointPolicy jointPolicy = computeNashEquilibriumPolicy();
    // JointPolicy jointPolicy = computeCorrelatedEquilibriumPolicy();
    // JointPolicy jointPolicy = computeFictitiousPlayPolicy();
    JointPolicy jointPolicy = getRandomJointPolicy(rng, numAgents);

    // Update Q tables
    std::cout << outPrefix.str() << " Updating Q tables..." << std::string(58, ' ') << std::flush;
    JointAction jointAction;
    for (auto& agent : qLearners) {
      agent.updateJointQTable(prevAgentStates, prevJointAction, stepRewards, currAgentStates,
                              jointPolicy);
      jointAction.push_back(agent.sampleEpsGreedyPolicy(rng, currAgentStates));
    }

    // Update N counts
    std::cout << outPrefix.str() << " Updating state and joint action visit counts (N tables)..."
              << std::flush;
    for (auto& agent : qLearners) {
      agent.updateN(currAgentStates, jointAction);
    }

    // Step action
    std::cout << outPrefix.str() << " Stepping simulator..." << std::string(58, ' ') << std::flush;
    environment::StepResult step = simulator.step(jointAction);

    // Update prev info
    prevAgentStates = currAgentStates;
    prevJointAction = jointAction;
    currAgentStates = simulator.world().agents;
    stepRewards = step.rewards;

    std::cout << outPrefix.str() << " Writing to file..." << std::string(58, ' ') << std::flush;
    if (outFile.is_open()) {
      outFile << jsonViewer.render(simulator.world(), simulator.grid(), t, step);
    }
    ++t;
    std::cout << outPrefix.str() << " Done!" << std::string(58, ' ') << std::flush;
  }
  std::cout << std::endl;

  outFile.close();
}

} // namespace perimeter

int main(int argc, char** argv)
{
  int endT{30};
  perimeter::runSim(endT, "sim.json");
  return 0;
}
