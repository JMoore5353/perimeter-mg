#include "visualization/JsonViewer.h"

#include <sstream>

#include "core/AgentType.h"

namespace perimeter::visualization {

namespace {

std::string toTypeString(core::AgentType type) {
    return type == core::AgentType::ATTACKER ? "ATTACKER" : "DEFENDER";
}

}  // namespace

std::string JsonViewer::render(const core::WorldState& state,
                               const geometry::Grid& grid,
                               int step,
                               const std::vector<double>& rewards) const {
    std::ostringstream out;
    out << "{\n";
    out << "  \"step\": " << step << ",\n";

    if (step == 0) {
        out << "  \"world_tiles\": [\n";
        const std::vector<geometry::Hex> cells = grid.getGridCells();
        for (std::size_t i = 0; i < cells.size(); ++i) {
            out << "    {\"q\":" << cells[i].q << ",\"r\":" << cells[i].r << "}";
            if (i + 1 < cells.size()) {
                out << ",";
            }
            out << "\n";
        }
        out << "  ],\n";

        out << "  \"base_tiles\": [\n";
        const std::vector<geometry::Hex> bases = grid.getBaseTiles();
        for (std::size_t i = 0; i < bases.size(); ++i) {
            out << "    {\"q\":" << bases[i].q << ",\"r\":" << bases[i].r << "}";
            if (i + 1 < bases.size()) {
                out << ",";
            }
            out << "\n";
        }
        out << "  ],\n";
    }

    out << "  \"agents\": [\n";
    for (std::size_t i = 0; i < state.agents.size(); ++i) {
        const core::AgentState& agent = state.agents[i];
        const double reward = (i < rewards.size()) ? rewards[i] : 0.0;
        out << "    {\"id\":" << agent.id
            << ",\"type\":\"" << toTypeString(agent.type)
            << "\",\"q\":" << agent.position.q
            << ",\"r\":" << agent.position.r
            << ",\"reward\":" << reward << "}";
        if (i + 1 < state.agents.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

}  // namespace perimeter::visualization
