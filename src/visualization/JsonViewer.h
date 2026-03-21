#pragma once

#include <string>
#include <vector>

#include "visualization/ViewerBackend.h"

namespace perimeter::visualization {

class JsonViewer final : public ViewerBackend {
public:
    [[nodiscard]] std::string render(const core::WorldState& state,
                                     const geometry::Grid& grid,
                                     int step,
                                     const std::vector<double>& rewards) const override;
};

}  // namespace perimeter::visualization
