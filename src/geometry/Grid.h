#pragma once

#include <vector>

#include "geometry/Hex.h"

namespace perimeter::geometry {

class Grid {
public:
    virtual ~Grid() = default;

    [[nodiscard]] virtual bool isValid(const Hex& cell) const noexcept = 0;
    [[nodiscard]] virtual std::vector<Hex> getOuterRing() const = 0;
    [[nodiscard]] virtual std::vector<Hex> getBaseTiles() const = 0;
    [[nodiscard]] virtual std::vector<Hex> getGridCells() const = 0;
};

}  // namespace perimeter::geometry
