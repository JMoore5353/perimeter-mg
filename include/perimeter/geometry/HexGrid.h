#ifndef PERIMETER_PERIMETER_GEOMETRY_HEXGRID_H
#define PERIMETER_PERIMETER_GEOMETRY_HEXGRID_H

#include <vector>

#include "perimeter/geometry/Grid.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter::geometry {

class HexGrid : public Grid {
public:
    explicit HexGrid(int radius, std::vector<Hex> baseTiles = {});

    [[nodiscard]] int radius() const noexcept;
    [[nodiscard]] bool isValid(const Hex& cell) const noexcept override;

    [[nodiscard]] std::vector<Hex> getNeighbors(const Hex& cell) const;
    [[nodiscard]] std::vector<Hex> getOuterRing() const override;
    [[nodiscard]] std::vector<Hex> getBaseTiles() const override;
    [[nodiscard]] std::vector<Hex> getGridCells() const override;

private:
    [[nodiscard]] std::vector<Hex> defaultBaseTiles() const;
    void validateBaseTiles(const std::vector<Hex>& baseTiles) const;

    int radius_{0};
    std::vector<Hex> baseTiles_;
};

}  // namespace perimeter::geometry

#endif  // PERIMETER_PERIMETER_GEOMETRY_HEXGRID_H
