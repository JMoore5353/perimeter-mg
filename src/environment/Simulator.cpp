#include "environment/Simulator.h"

namespace perimeter::environment {

Simulator::Simulator(geometry::HexGrid grid, core::WorldState initialWorld, std::uint32_t seed)
    : grid_(std::move(grid)), world_(std::move(initialWorld)), rng_(seed) {}

StepResult Simulator::step(const std::vector<Action>& jointActions) {
    return stepWorld(world_, jointActions, grid_, rng_);
}

const core::WorldState& Simulator::world() const noexcept {
    return world_;
}

const geometry::HexGrid& Simulator::grid() const noexcept {
    return grid_;
}

}  // namespace perimeter::environment
