#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/environment/Initialization.h"
#include "perimeter/environment/Simulator.h"
#include "perimeter/geometry/HexGrid.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/nash_equilibrium_solver.h"
#include "perimeter/learning/nash_q_learning.h"
#include "perimeter/learning/qtable_checkpoint.h"
#include "perimeter/visualization/JsonViewer.h"

namespace perimeter
{

// Command-line configuration
struct SimConfig
{
  int endT = 3000;
  std::string outputFile = "sim.json";
  std::string loadCheckpoint = "";  // Empty = start fresh
  int saveInterval = 1000;          // Save every N steps
  std::string checkpointDir = "qtables";
  int hexRadius = 3;
  int numAttackers = 1;
  int numDefenders = 1;
  std::uint16_t seed = 123U;
};

SimConfig parseCommandLine(int argc, char** argv)
{
  SimConfig config;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                << "Options:\n"
                << "  --steps <N>              Number of simulation steps (default: 3000)\n"
                << "  --output <file>          Output JSON file (default: sim.json)\n"
                << "  --load-qtable <path>     Load Q-tables from checkpoint (pattern with agent*)\n"
                << "  --save-interval <N>      Save checkpoint every N steps (default: 1000)\n"
                << "  --checkpoint-dir <dir>   Checkpoint directory (default: qtables)\n"
                << "  --hex-radius <N>         Radius of hex grid (default: 3)\n"
                << "  --num-attackers <N>      Number of attackers (default: 1)\n"
                << "  --num-defenders <N>      Number of defenders (default: 1)\n"
                << "  --seed <N>               Seed for random number generators (default: 123)\n"
                << "  --help, -h               Show this help message\n";
      std::exit(0);
    } else if (arg == "--steps" && i + 1 < argc) {
      config.endT = std::stoi(argv[++i]);
    } else if (arg == "--output" && i + 1 < argc) {
      config.outputFile = argv[++i];
    } else if (arg == "--load-qtable" && i + 1 < argc) {
      config.loadCheckpoint = argv[++i];
    } else if (arg == "--save-interval" && i + 1 < argc) {
      config.saveInterval = std::stoi(argv[++i]);
    } else if (arg == "--checkpoint-dir" && i + 1 < argc) {
      config.checkpointDir = argv[++i];
    } else if (arg == "--hex-radius" && i + 1 < argc) {
      config.hexRadius = std::stoi(argv[++i]);
    } else if (arg == "--num-attackers" && i + 1 < argc) {
      config.numAttackers = std::stoi(argv[++i]);
    } else if (arg == "--num-defenders" && i + 1 < argc) {
      config.numDefenders = std::stoi(argv[++i]);
    } else if (arg == "--seed" && i + 1 < argc) {
      config.seed = std::stoi(argv[++i]);
    }
  }

  return config;
}

JointPolicy getRandomJointPolicy(std::mt19937& rng, const std::size_t numAgents)
{
  return JointPolicy(
    numAgents,
    std::vector<float>(static_cast<int>(environment::Action::NUM_ACTIONS),
                        1 / static_cast<float>(environment::Action::NUM_ACTIONS)));
}

void runSim(const SimConfig& simConfig)
{
  int t{0};
  int checkpointStepOffset{0};
  std::ofstream outFile;
  outFile.open(simConfig.outputFile);

  const environment::InitializationConfig config{.radius{simConfig.hexRadius},
                                                 .attackerCount{simConfig.numAttackers},
                                                 .defenderCount{simConfig.numDefenders},
                                                 .seed{simConfig.seed}};
  environment::InitializedEnvironment initialized = createInitialWorld(config);
  environment::Simulator simulator{std::move(initialized.grid), std::move(initialized.world),
                                   config.seed};
  std::cout << "Initialized environment with " << config.attackerCount << " attackers and "
            << config.defenderCount << " defenders." << std::endl;

  visualization::JsonViewer jsonViewer;
  std::mt19937 rng(config.seed);

  std::cout << "Computing joint action space...";
  int numAgents = simulator.world().agents.size();
  float gamma = 0.9;
  JointActionSpace jointActionSpace{numAgents};
  std::cout << " Done!" << std::endl;
  std::vector<NashQLearning> qLearners;
  for (int i{0}; i < numAgents; ++i) {
    qLearners.emplace_back(i, numAgents, gamma, jointActionSpace, 
                           simulator.world().agents[i].type);
  }

  NashEquilibriumSolver nashSolver;
  JointRewardFunction jointRewardFunction;
  for (auto& agent : qLearners) {
    jointRewardFunction.push_back(agent.getRewardFunction());
  }
  std::vector<int> solveTimes;

  // Load checkpoint if specified
  if (!simConfig.loadCheckpoint.empty()) {
    try {
      std::cout << "Loading Q-tables from " << simConfig.loadCheckpoint << "..." << std::endl;
      learning::QTableCheckpoint::loadAll(qLearners, simConfig.loadCheckpoint);
      checkpointStepOffset = learning::QTableCheckpoint::extractStepNumber(simConfig.loadCheckpoint);
      std::cout << "Resuming checkpoint timeline from step " << checkpointStepOffset << "."
                << std::endl;
      std::cout << "Successfully loaded checkpoint!" << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to load checkpoint: " << e.what() << std::endl;
      std::cerr << "Starting with fresh Q-tables instead." << std::endl;
      checkpointStepOffset = 0;
    }
  }

  std::vector<core::AgentState> currAgentStates = simulator.world().agents;
  std::vector<core::AgentState> prevAgentStates = currAgentStates;
  JointAction prevJointAction(numAgents, environment::Action::STAY);
  std::vector<float> stepRewards(numAgents, 0.0);
  while (t <= simConfig.endT) {
    std::ostringstream outPrefix;
    outPrefix << "\rStarting simulation step " << t << "...";
    std::cout << outPrefix.str() << std::flush;

    // Compute stochastic policy (centralized module that computes Nash Equilibrium for simple game)
    std::cout << outPrefix.str() << " Computing stochastic policy..." << std::flush;
    auto start = std::chrono::steady_clock::now();
    JointPolicy jointPolicy =
      nashSolver.solve(jointRewardFunction, jointActionSpace, simulator.world().agents);
    // JointPolicy jointPolicy = computeCorrelatedEquilibriumPolicy();
    // JointPolicy jointPolicy = computeFictitiousPlayPolicy();
    // JointPolicy jointPolicy = computeRegretMatchingPolicy();
    // JointPolicy jointPolicy = getRandomJointPolicy(rng, numAgents);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    solveTimes.push_back(duration.count());

    // Update Q tables
    std::cout << outPrefix.str() << " Updating Q tables..." << std::string(58, ' ') << std::flush;
    JointAction jointAction;
    for (auto& agent : qLearners) {
      agent.setEquilibriumPolicy(jointPolicy);
      agent.updateJointQTable(prevAgentStates, prevJointAction, stepRewards, currAgentStates,
                              jointPolicy);
      if (agent.getAgentType() == core::AgentType::DEFENDER) {
        jointAction.push_back(environment::Action::STAY);
      } else {
        jointAction.push_back(agent.sampleEpsGreedyPolicy(rng, currAgentStates));
      }
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

    // Save checkpoint periodically
    int globalStep = checkpointStepOffset + t;
    bool isCheckpointStep =
      globalStep > checkpointStepOffset && simConfig.saveInterval > 0 &&
      globalStep % simConfig.saveInterval == 0;
    if (isCheckpointStep) {
      try {
        std::cout << outPrefix.str() << " Saving checkpoint..." << std::string(58, ' ')
                  << std::flush;
        learning::QTableCheckpoint::saveAll(qLearners, config, gamma, globalStep,
                                            simConfig.checkpointDir);
        std::cout << outPrefix.str() << " Checkpoint saved!" << std::string(58, ' ') << std::flush;
      } catch (const std::exception& e) {
        std::cerr << "\nWarning: Failed to save checkpoint: " << e.what() << std::endl;
      }
    }

    ++t;
    std::cout << outPrefix.str() << " Done!" << std::string(58, ' ') << std::flush;
  }
  std::cout << std::endl;

  // Timing analysis
  std::cout << "Solve times: "
            << std::accumulate(solveTimes.begin(), solveTimes.end(), 0.0)
      / static_cast<float>(solveTimes.size())
            << std::endl;

  outFile.close();
}

} // namespace perimeter

int main(int argc, char** argv)
{
  perimeter::SimConfig config = perimeter::parseCommandLine(argc, argv);
  perimeter::runSim(config);
  return 0;
}
