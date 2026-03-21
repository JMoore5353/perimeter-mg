#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "core/WorldState.h"
#include "geometry/Hex.h"
#include "geometry/HexGrid.h"

namespace perimeter::environment {

struct InitializationConfig {
    int radius{3};
    int attackerCount{1};
    int defenderCount{1};
    std::uint32_t seed{1U};
    std::optional<std::vector<geometry::Hex>> baseTiles{};
};

struct InitializedEnvironment {
    geometry::HexGrid grid;
    core::WorldState world;
};

InitializedEnvironment createInitialWorld(const InitializationConfig& config);

}  // namespace perimeter::environment
