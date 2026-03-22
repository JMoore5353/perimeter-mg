#ifndef PERIMETER_PERIMETER_VISUALIZATION_VIEWERBACKEND_H
#define PERIMETER_PERIMETER_VISUALIZATION_VIEWERBACKEND_H

#include <string>

#include "perimeter/core/WorldState.h"
#include "perimeter/environment/Transition.h"
#include "perimeter/geometry/Grid.h"

namespace perimeter::visualization {

class ViewerBackend {
public:
    virtual ~ViewerBackend() = default;

    [[nodiscard]] virtual std::string render(const core::WorldState& state,
                                             const geometry::Grid& grid,
                                             int step,
                                             const environment::StepResult& stepResult) const = 0;
};

}  // namespace perimeter::visualization

#endif  // PERIMETER_PERIMETER_VISUALIZATION_VIEWERBACKEND_H
