#include "perimeter/visualization/JsonViewer.h"

#include <sstream>

#include "perimeter/core/AgentType.h"

namespace perimeter::visualization {

namespace {

std::string toTypeString(core::AgentType type) {
    return type == core::AgentType::ATTACKER ? "ATTACKER" : "DEFENDER";
}

}  // namespace

std::string JsonViewer::render(const core::WorldState& state,
                               const geometry::Grid& grid,
                               int step,
                               const environment::StepResult& stepResult) const {
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
        const double reward = (i < stepResult.rewards.size()) ? stepResult.rewards[i] : 0.0;
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
    out << "  ],\n";

    out << "  \"captured_attacker_ids\": [";
    for (std::size_t i = 0; i < stepResult.capturedAttackerIds.size(); ++i) {
        out << stepResult.capturedAttackerIds[i];
        if (i + 1 < stepResult.capturedAttackerIds.size()) {
            out << ",";
        }
    }
    out << "],\n";

    out << "  \"base_arrival_attacker_ids\": [";
    for (std::size_t i = 0; i < stepResult.baseArrivalAttackerIds.size(); ++i) {
        out << stepResult.baseArrivalAttackerIds[i];
        if (i + 1 < stepResult.baseArrivalAttackerIds.size()) {
            out << ",";
        }
    }
    out << "]\n";

    out << "}\n";
    return out.str();
}

}  // namespace perimeter::visualization
