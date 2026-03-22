#include "perimeter/geometry/Hex.h"
#include "perimeter/geometry/HexGrid.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace perimeter::geometry {
namespace {

TEST(HexDistanceTest, HandlesKnownPairsAndZeroDistance) {
    EXPECT_EQ(hexDistance(Hex{0, 0}, Hex{0, 0}), 0);
    EXPECT_EQ(hexDistance(Hex{0, 0}, Hex{1, 0}), 1);
    EXPECT_EQ(hexDistance(Hex{0, 0}, Hex{2, -1}), 2);
    EXPECT_EQ(hexDistance(Hex{-2, 1}, Hex{1, -1}), 3);
}

TEST(HexDistanceTest, IsSymmetric) {
    const Hex a{2, -3};
    const Hex b{-1, 2};
    EXPECT_EQ(hexDistance(a, b), hexDistance(b, a));
}

TEST(HexGridTest, ValidityUsesCircularRadius) {
    const HexGrid grid(2);
    EXPECT_TRUE(grid.isValid(Hex{0, 0}));
    EXPECT_TRUE(grid.isValid(Hex{2, -2}));
    EXPECT_TRUE(grid.isValid(Hex{2, 0}));
    EXPECT_FALSE(grid.isValid(Hex{3, 0}));
    EXPECT_FALSE(grid.isValid(Hex{-3, 1}));
}

TEST(HexGridTest, InteriorCellHasSixNeighbors) {
    const HexGrid grid(2);
    const std::vector<Hex> neighbors = grid.getNeighbors(Hex{0, 0});

    EXPECT_EQ(neighbors.size(), 6U);

    std::set<Hex> expected{
        Hex{1, 0}, Hex{1, -1}, Hex{0, -1}, Hex{-1, 0}, Hex{-1, 1}, Hex{0, 1},
    };
    std::set<Hex> actual(neighbors.begin(), neighbors.end());
    EXPECT_EQ(actual, expected);
}

TEST(HexGridTest, BoundaryCellNeighborsAreFilteredByValidity) {
    const HexGrid grid(2);
    const std::vector<Hex> neighbors = grid.getNeighbors(Hex{2, 0});

    EXPECT_EQ(neighbors.size(), 3U);

    for (const Hex& cell : neighbors) {
        EXPECT_TRUE(grid.isValid(cell));
    }
}

TEST(HexGridTest, OuterRingContainsOnlyDistanceRadiusCells) {
    const HexGrid grid(3);
    const std::vector<Hex> ring = grid.getOuterRing();

    EXPECT_EQ(ring.size(), 18U);
    for (const Hex& cell : ring) {
        EXPECT_EQ(hexDistance(Hex{0, 0}, cell), grid.radius());
    }
}

TEST(HexGridTest, DefaultBaseTilesMatchConfiguredPattern) {
    const HexGrid grid(3);
    std::vector<Hex> baseTiles = grid.getBaseTiles();

    std::sort(baseTiles.begin(), baseTiles.end());
    const std::vector<Hex> expected{Hex{0, 0}, Hex{0, 1}, Hex{1, 0}};
    EXPECT_EQ(baseTiles, expected);
}

TEST(HexGridTest, AcceptsValidCustomBaseTiles) {
    const std::vector<Hex> custom{Hex{0, 0}, Hex{1, -1}, Hex{-1, 1}};
    const HexGrid grid(3, custom);
    EXPECT_EQ(grid.getBaseTiles(), custom);
}

TEST(HexGridTest, RejectsInvalidBaseTileConfigurations) {
    EXPECT_THROW((HexGrid{3, std::vector<Hex>{Hex{0, 0}, Hex{0, 1}}}), std::invalid_argument);
    EXPECT_THROW((HexGrid{3, std::vector<Hex>{Hex{0, 0}, Hex{0, 1}, Hex{0, 1}}}), std::invalid_argument);
    EXPECT_THROW((HexGrid{3, std::vector<Hex>{Hex{0, 0}, Hex{0, 1}, Hex{2, 0}}}), std::invalid_argument);
}

}  // namespace
}  // namespace perimeter::geometry
