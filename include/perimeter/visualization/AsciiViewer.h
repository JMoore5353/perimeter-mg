#ifndef PERIMETER_PERIMETER_VISUALIZATION_ASCIIVIEWER_H
#define PERIMETER_PERIMETER_VISUALIZATION_ASCIIVIEWER_H

#include <string>
#include "perimeter/visualization/ViewerBackend.h"

namespace perimeter::visualization {

class AsciiViewer final : public ViewerBackend {
public:
    [[nodiscard]] std::string render(const core::WorldState& state,
                                     const geometry::Grid& grid,
                                     int step,
                                     const environment::StepResult& stepResult) const override;
};

}  // namespace perimeter::visualization

#endif  // PERIMETER_PERIMETER_VISUALIZATION_ASCIIVIEWER_H
