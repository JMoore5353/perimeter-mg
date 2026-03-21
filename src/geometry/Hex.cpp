#include "geometry/Hex.h"

#include <array>
#include <cstdlib>

namespace perimeter::geometry {

int hexDistance(const Hex& a, const Hex& b) noexcept {
    const int dq = a.q - b.q;
    const int dr = a.r - b.r;
    const int ds = a.s() - b.s();
    return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}

Hex add(const Hex& a, const Hex& b) noexcept {
    return Hex{a.q + b.q, a.r + b.r};
}

const std::array<Hex, 6>& axialDirections() noexcept {
    static constexpr std::array<Hex, 6> kDirections{
        Hex{1, 0},
        Hex{1, -1},
        Hex{0, -1},
        Hex{-1, 0},
        Hex{-1, 1},
        Hex{0, 1},
    };
    return kDirections;
}

}  // namespace perimeter::geometry
