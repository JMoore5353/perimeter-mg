#pragma once

#include <string>
#include <vector>

#include "core/WorldState.h"
#include "geometry/Grid.h"

namespace perimeter::visualization {

class ViewerBackend {
public:
    virtual ~ViewerBackend() = default;

    [[nodiscard]] virtual std::string render(const core::WorldState& state,
                                             const geometry::Grid& grid,
                                             int step,
                                             const std::vector<double>& rewards) const = 0;
};

}  // namespace perimeter::visualization
