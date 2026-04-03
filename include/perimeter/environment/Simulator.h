#ifndef PERIMETER_PERIMETER_ENVIRONMENT_SIMULATOR_H
#define PERIMETER_PERIMETER_ENVIRONMENT_SIMULATOR_H

#include <cstdint>
#include <functional>
#include <random>
#include <vector>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/environment/Transition.h"
#include "perimeter/geometry/Grid.h"
#include "perimeter/geometry/HexGrid.h"
#include "perimeter/learning/joint.h"

namespace perimeter::environment
{

class Simulator
{
public:
  Simulator(geometry::HexGrid grid, core::WorldState initialWorld, std::uint32_t seed);

  StepResult step(const std::vector<Action>& jointActions);

  [[nodiscard]] const core::WorldState& world() const noexcept;
  [[nodiscard]] const geometry::Grid& grid() const noexcept;

private:
  geometry::HexGrid grid_;
  core::WorldState world_;
  std::mt19937 rng_;
};

} // namespace perimeter::environment

#endif // PERIMETER_PERIMETER_ENVIRONMENT_SIMULATOR_H
