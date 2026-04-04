#include "perimeter/geometry/HexGrid.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace perimeter::geometry
{

HexGrid::HexGrid(int radius, std::vector<Hex> baseTiles)
    : radius_(radius)
{
  if (radius_ < 0) {
    throw std::invalid_argument("HexGrid radius must be non-negative.");
  }

  if (baseTiles.empty()) {
    baseTiles = defaultBaseTiles();
  }
  validateBaseTiles(baseTiles);
  baseTiles_ = std::move(baseTiles);
}

int HexGrid::radius() const noexcept
{
  return radius_;
}

bool HexGrid::isValid(const Hex& cell) const noexcept
{
  return hexDistance(Hex{0, 0}, cell) <= radius_;
}

std::vector<Hex> HexGrid::getNeighbors(const Hex& cell) const
{
  std::vector<Hex> neighbors;
  if (!isValid(cell)) {
    return neighbors;
  }

  neighbors.reserve(6);
  for (const Hex& direction : axialDirections()) {
    Hex next = add(cell, direction);
    if (isValid(next)) {
      neighbors.push_back(next);
    }
  }
  return neighbors;
}

std::vector<Hex> HexGrid::getOuterRing() const
{
  std::vector<Hex> ring;
  std::vector<Hex> cells = getGridCells();
  ring.reserve(radius_ * 6U);

  for (const Hex& cell : cells) {
    if (hexDistance(Hex{0, 0}, cell) == radius_) {
      ring.push_back(cell);
    }
  }
  return ring;
}

std::vector<Hex> HexGrid::getBaseTiles() const
{
  return baseTiles_;
}

std::vector<Hex> HexGrid::getGridCells() const
{
  std::vector<Hex> cells;

  for (int q = -radius_; q <= radius_; ++q) {
    const int rMin = std::max(-radius_, -q - radius_);
    const int rMax = std::min(radius_, -q + radius_);
    for (int r = rMin; r <= rMax; ++r) {
      cells.emplace_back(q, r);
    }
  }

  return cells;
}

std::vector<Hex> HexGrid::defaultBaseTiles() const
{
  return {Hex{0, 0}, Hex{1, 0}, Hex{0, 1}};
}

void HexGrid::validateBaseTiles(const std::vector<Hex>& baseTiles) const
{
  if (baseTiles.size() != 3) {
    throw std::invalid_argument("Base tile configuration must contain exactly 3 tiles.");
  }

  std::set<Hex> uniqueTiles(baseTiles.begin(), baseTiles.end());
  if (uniqueTiles.size() != baseTiles.size()) {
    throw std::invalid_argument("Base tiles must be unique.");
  }

  for (const Hex& tile : baseTiles) {
    if (!isValid(tile)) {
      throw std::invalid_argument("Each base tile must lie inside the grid radius.");
    }
    if (hexDistance(Hex{0, 0}, tile) > 1) {
      throw std::invalid_argument("Each base tile must be near the center (distance <= 1).");
    }
  }
}

} // namespace perimeter::geometry
