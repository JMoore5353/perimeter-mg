#include "perimeter/visualization/AsciiViewer.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "perimeter/core/AgentType.h"
#include "perimeter/geometry/Hex.h"

namespace perimeter::visualization {

std::string AsciiViewer::render(const core::WorldState& state,
                                const geometry::Grid& grid,
                                int step,
                                const environment::StepResult& stepResult) const {
    std::unordered_map<geometry::Hex, char> cellGlyph;
    cellGlyph.reserve(grid.getGridCells().size());
    for (const geometry::Hex& cell : grid.getGridCells()) {
        cellGlyph[cell] = '.';
    }

    const std::vector<geometry::Hex> baseTiles = grid.getBaseTiles();
    const std::unordered_set<geometry::Hex> baseSet(baseTiles.begin(), baseTiles.end());
    for (const geometry::Hex& base : baseSet) {
        cellGlyph[base] = 'B';
    }

    for (const core::AgentState& agent : state.agents) {
        const char glyph = (agent.type == core::AgentType::ATTACKER) ? 'A' : 'D';
        auto found = cellGlyph.find(agent.position);
        if (found == cellGlyph.end()) {
            continue;
        }
        if (found->second != '.' && found->second != 'B') {
            found->second = '*';
        } else {
            found->second = glyph;
        }
    }

    int minQ = 0;
    int maxQ = 0;
    int minR = 0;
    int maxR = 0;
    bool first = true;
    for (const geometry::Hex& cell : grid.getGridCells()) {
        if (first) {
            minQ = maxQ = cell.q;
            minR = maxR = cell.r;
            first = false;
        } else {
            minQ = std::min(minQ, cell.q);
            maxQ = std::max(maxQ, cell.q);
            minR = std::min(minR, cell.r);
            maxR = std::max(maxR, cell.r);
        }
    }

    std::ostringstream out;
    out << "step: " << step << '\n';
    for (int r = minR; r <= maxR; ++r) {
        out << std::string(static_cast<std::size_t>(r - minR), ' ');
        for (int q = minQ; q <= maxQ; ++q) {
            const geometry::Hex cell{q, r};
            if (!grid.isValid(cell)) {
                out << "  ";
                continue;
            }
            out << cellGlyph[cell] << ' ';
        }
        out << '\n';
    }
    out << "rewards:\n";
    const std::size_t limit = std::min(state.agents.size(), stepResult.rewards.size());
    for (std::size_t i = 0; i < limit; ++i) {
        out << "  id=" << state.agents[i].id << " reward=" << stepResult.rewards[i] << '\n';
    }
    out << "captured_attackers:\n";
    for (const int id : stepResult.capturedAttackerIds) {
        out << "  " << id << '\n';
    }
    out << "base_arrival_attackers:\n";
    for (const int id : stepResult.baseArrivalAttackerIds) {
        out << "  " << id << '\n';
    }
    return out.str();
}

}  // namespace perimeter::visualization
