#pragma once

#include <array>
#include <cstddef>
#include <functional>

namespace perimeter::geometry {

struct Hex {
    int q{0};
    int r{0};

    constexpr Hex() = default;
    constexpr Hex(int qCoord, int rCoord) : q(qCoord), r(rCoord) {}

    constexpr int s() const noexcept {
        return -q - r;
    }

    constexpr bool operator==(const Hex& other) const noexcept {
        return q == other.q && r == other.r;
    }

    constexpr bool operator<(const Hex& other) const noexcept {
        return (q < other.q) || (q == other.q && r < other.r);
    }
};

int hexDistance(const Hex& a, const Hex& b) noexcept;
Hex add(const Hex& a, const Hex& b) noexcept;
const std::array<Hex, 6>& axialDirections() noexcept;

}  // namespace perimeter::geometry

namespace std {
template <>
struct hash<perimeter::geometry::Hex> {
    size_t operator()(const perimeter::geometry::Hex& hex) const noexcept {
        size_t seed = std::hash<int>{}(hex.q);
        seed ^= std::hash<int>{}(hex.r) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};
}  // namespace std
