#include <fstream>
#include <random>
#include <vector>
#include <iostream>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/environment/Initialization.h"
#include "perimeter/environment/Simulator.h"
#include "perimeter/environment/Action.h"
#include "perimeter/geometry/HexGrid.h"
#include "perimeter/visualization/JsonViewer.h"

namespace perimeter
{

std::vector<environment::Action> get_random_actions(std::mt19937& rng, const std::size_t numActions) {
  std::uniform_int_distribution<int> distr{0, 6};

  std::vector<environment::Action> actions;
  actions.reserve(numActions);
  for (std::size_t i{0}; i < numActions; ++i) {
    actions.emplace_back(static_cast<environment::Action>(distr(rng)));
  }

  return actions;
}

void runSim(const int endT, const std::string filename) {
  int t{0};
  std::ofstream outFile;
  outFile.open(filename);

  const environment::InitializationConfig config{
      .radius{3},
      .attackerCount{3},
      .defenderCount{5},
      .seed{123U}
  };
  environment::InitializedEnvironment initialized = createInitialWorld(config);
  environment::Simulator simulator{std::move(initialized.grid), std::move(initialized.world), config.seed};

  visualization::JsonViewer jsonViewer;
  std::mt19937 rng(config.seed);

  while (t < endT) {
    std::vector<environment::Action> jointActions = get_random_actions(rng, simulator.world().agents.size());
    environment::StepResult step = simulator.step(jointActions);
    if (outFile.is_open()) {
      outFile << jsonViewer.render(simulator.world(), simulator.grid(), t, step);
    }

    ++t;
  }

  outFile.close();
}

} // namespace perimeter


int main(int argc, char** argv) {
  int endT{30};
  perimeter::runSim(endT, "sim.json");
  return 0;
}

