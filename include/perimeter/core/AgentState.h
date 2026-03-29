#ifndef PERIMETER_PERIMETER_CORE_AGENTSTATE_H
#define PERIMETER_PERIMETER_CORE_AGENTSTATE_H

#include "perimeter/core/AgentType.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter::core
{

struct AgentState
{
  int id{0};
  AgentType type{AgentType::ATTACKER};
  geometry::Hex position{};

  constexpr bool operator==(const AgentState& other) const noexcept
  {
    return id == other.id && type == other.type && position == other.position;
  }
};

} // namespace perimeter::core

namespace std
{
template<>
struct hash<perimeter::core::AgentState>
{
  size_t operator()(const perimeter::core::AgentState& s) const noexcept
  {
    size_t seed = 0;

    auto combine = [](size_t& seed, size_t value) {
      seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    };

    combine(seed, std::hash<int>{}(s.id));
    combine(seed, std::hash<perimeter::core::AgentType>{}(s.type));
    combine(seed, std::hash<perimeter::geometry::Hex>{}(s.position));

    return seed;
  }
};
} // namespace std

#endif // PERIMETER_PERIMETER_CORE_AGENTSTATE_H
