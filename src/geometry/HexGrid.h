#pragma once

#include <vector>

#include "geometry/Hex.h"

namespace perimeter::geometry {

class HexGrid {
public:
    explicit HexGrid(int radius, std::vector<Hex> baseTiles = {});

    [[nodiscard]] int radius() const noexcept;
    [[nodiscard]] bool isValid(const Hex& cell) const noexcept;

    [[nodiscard]] std::vector<Hex> getNeighbors(const Hex& cell) const;
    [[nodiscard]] std::vector<Hex> getOuterRing() const;
    [[nodiscard]] std::vector<Hex> getBaseTiles() const;
    [[nodiscard]] std::vector<Hex> getGridCells() const;

private:
    [[nodiscard]] std::vector<Hex> defaultBaseTiles() const;
    void validateBaseTiles(const std::vector<Hex>& baseTiles) const;

    int radius_{0};
    std::vector<Hex> baseTiles_;
};

}  // namespace perimeter::geometry
