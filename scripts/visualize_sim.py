#!/usr/bin/env python3
"""Visualize hex-world simulation logs from sim.json."""

import argparse
import json
import math
from pathlib import Path
from typing import Dict, List, Sequence, Tuple

from matplotlib.collections import PathCollection
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
import numpy as np


def hex_distance(q1: int, r1: int, q2: int, r2: int) -> int:
    dq = q1 - q2
    dr = r1 - r2
    ds = -dq - dr
    return max(abs(dq), abs(dr), abs(ds))


def hex_to_xy(q: int, r: int, size: float) -> Tuple[float, float]:
    x = size * (1.5 * q)
    y = size * (((3.0**0.5) / 2.0) * q + (3.0**0.5) * r)
    return x, y


def load_steps(path: Path) -> List[Dict]:
    """Load either a JSON array or concatenated JSON objects."""
    raw = path.read_text(encoding="utf-8").strip()
    if not raw:
        raise ValueError(f"Input file is empty: {path}")

    if raw.startswith("["):
        parsed = json.loads(raw)
        if not isinstance(parsed, list):
            raise ValueError("Expected top-level JSON array.")
        return parsed

    decoder = json.JSONDecoder()
    steps: List[Dict] = []
    idx = 0
    n = len(raw)
    while idx < n:
        while idx < n and raw[idx].isspace():
            idx += 1
        if idx >= n:
            break
        obj, next_idx = decoder.raw_decode(raw, idx)
        if not isinstance(obj, dict):
            raise ValueError("Expected each JSON record to be an object.")
        steps.append(obj)
        idx = next_idx
    return steps


def extract_static_tiles(steps: Sequence[Dict]) -> Tuple[List[Dict], List[Dict]]:
    world_tiles = []
    base_tiles = []
    for record in steps:
        if not world_tiles and "world_tiles" in record:
            world_tiles = record["world_tiles"]
        if not base_tiles and "base_tiles" in record:
            base_tiles = record["base_tiles"]
        if world_tiles and base_tiles:
            break
    return world_tiles, base_tiles


def build_points(tiles: Sequence[Dict], size: float) -> Tuple[List[float], List[float]]:
    xs: List[float] = []
    ys: List[float] = []
    for tile in tiles:
        x, y = hex_to_xy(int(tile["q"]), int(tile["r"]), size)
        xs.append(x)
        ys.append(y)
    return xs, ys


def infer_arrival_tiles(
    arrivals: Sequence[int],
    previous_attacker_positions: Dict[int, Tuple[int, int]],
    current_attacker_positions: Dict[int, Tuple[int, int]],
    base_tiles: Sequence[Dict],
) -> List[Tuple[int, int]]:
    """Infer which base tile to flash for each base-arrival event."""
    base_coords = [(int(tile["q"]), int(tile["r"])) for tile in base_tiles]
    base_coord_set = set(base_coords)
    highlighted = set()
    for attacker_id in arrivals:
        prev_pos = previous_attacker_positions.get(int(attacker_id))
        curr_pos = current_attacker_positions.get(int(attacker_id))
        if prev_pos and prev_pos in base_coord_set:
            highlighted.add(prev_pos)
            continue
        if curr_pos and curr_pos in base_coord_set:
            highlighted.add(curr_pos)
            continue
        if prev_pos and base_coords:
            closest_base = min(
                base_coords,
                key=lambda base_pos: hex_distance(
                    prev_pos[0], prev_pos[1], base_pos[0], base_pos[1]
                ),
            )
            highlighted.add(closest_base)
            continue
        if base_coords:
            highlighted.add(base_coords[0])
    return list(highlighted)


def infer_capture_tiles(
    captures: Sequence[int],
    previous_attacker_positions: Dict[int, Tuple[int, int]],
    current_attacker_positions: Dict[int, Tuple[int, int]],
) -> List[Tuple[int, int]]:
    """Infer likely capture locations from tracked attacker positions."""
    highlighted = set()
    for attacker_id in captures:
        prev_pos = previous_attacker_positions.get(int(attacker_id))
        curr_pos = current_attacker_positions.get(int(attacker_id))
        if prev_pos:
            highlighted.add(prev_pos)
        elif curr_pos:
            highlighted.add(curr_pos)
    return list(highlighted)


def set_scatter_offsets(
    scatter: PathCollection, points: Sequence[Tuple[float, float]]
) -> None:
    if points:
        scatter.set_offsets(points)
    else:
        scatter.set_offsets(np.empty((0, 2), dtype=float))


def run_viewer(steps: Sequence[Dict], hex_size: float, delay: float) -> None:
    world_tiles, base_tiles = extract_static_tiles(steps)
    if not world_tiles:
        raise ValueError("No world_tiles found in simulation log.")
    if not base_tiles:
        raise ValueError("No base_tiles found in simulation log.")

    world_x, world_y = build_points(world_tiles, hex_size)
    base_x, base_y = build_points(base_tiles, hex_size)

    plt.ion()
    fig, (ax, reward_ax) = plt.subplots(
        2, 1, figsize=(11, 10), gridspec_kw={"height_ratios": [3.0, 1.5]}
    )
    fig.canvas.manager.set_window_title("Hex World Simulation Viewer")
    ax.set_aspect("equal", adjustable="box")
    ax.set_facecolor("#f8f9fa")
    ax.grid(False)

    ax.scatter(
        world_x,
        world_y,
        marker="H",
        s=1500,
        facecolors="#e9ecef",
        edgecolors="#adb5bd",
        linewidths=0.8,
    )
    ax.scatter(
        base_x,
        base_y,
        marker="H",
        s=1500,
        facecolors="#74c69d",
        edgecolors="#2d6a4f",
        linewidths=1.2,
    )
    arrival_tile_overlay = ax.scatter(
        [],
        [],
        marker="H",
        s=1750,
        facecolors="#e03131",
        edgecolors="#9b2226",
        linewidths=1.3,
        alpha=0.65,
        zorder=2.2,
    )
    capture_tile_overlay = ax.scatter(
        [],
        [],
        marker="H",
        s=1750,
        facecolors="#ffb703",
        edgecolors="#fb8500",
        linewidths=1.3,
        alpha=0.55,
        zorder=2.1,
    )

    attacker_scatter = ax.scatter(
        [], [], s=120, c="#d62828", marker="o", label="Attackers", zorder=3
    )
    defender_scatter = ax.scatter(
        [], [], s=120, c="#1d4ed8", marker="s", label="Defenders", zorder=3
    )
    event_text = ax.text(
        0.01,
        0.99,
        "",
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=10,
        bbox=dict(
            boxstyle="round,pad=0.4", facecolor="white", alpha=0.85, edgecolor="#ced4da"
        ),
    )

    ax.legend(loc="lower right")
    reward_ax.set_title("Accumulated Reward by Agent", fontsize=12, fontweight="bold")
    reward_ax.set_xlabel("Step")
    reward_ax.set_ylabel("Total Reward")
    reward_ax.grid(True, alpha=0.25)

    all_x = world_x + base_x
    all_y = world_y + base_y
    margin = hex_size * 2.5
    ax.set_xlim(min(all_x) - margin, max(all_x) + margin)
    ax.set_ylim(min(all_y) - margin, max(all_y) + margin)

    all_agent_ids = sorted(
        {int(agent["id"]) for record in steps for agent in record.get("agents", [])}
    )
    cumulative_rewards = {agent_id: 0.0 for agent_id in all_agent_ids}
    cumulative_captures = 0
    cumulative_base_arrivals = 0
    reward_history = {agent_id: [] for agent_id in all_agent_ids}
    step_history: List[int] = []
    cmap = plt.get_cmap("tab10")
    agent_colors = {agent_id: cmap(i % 10) for i, agent_id in enumerate(all_agent_ids)}
    reward_lines = {}
    id_labels = []
    last_attacker_positions: Dict[int, Tuple[int, int]] = {}
    for i, agent_id in enumerate(all_agent_ids):
        (line,) = reward_ax.plot(
            [], [], label=f"id {agent_id}", color=agent_colors[agent_id], linewidth=2.0
        )
        reward_lines[agent_id] = line
    if all_agent_ids:
        reward_ax.legend(loc="upper left", ncol=2, fontsize=9)

    for record in steps:
        attackers_x: List[float] = []
        attackers_y: List[float] = []
        defenders_x: List[float] = []
        defenders_y: List[float] = []
        attacker_colors: List[Tuple[float, float, float, float]] = []
        defender_colors: List[Tuple[float, float, float, float]] = []

        for label in id_labels:
            label.remove()
        id_labels.clear()

        agents = record.get("agents", [])
        current_attacker_positions: Dict[int, Tuple[int, int]] = {}
        grouped_agents: Dict[Tuple[int, int], List[Dict]] = {}
        for agent in agents:
            key = (int(agent["q"]), int(agent["r"]))
            grouped_agents.setdefault(key, []).append(agent)
            if str(agent.get("type")) == "ATTACKER":
                current_attacker_positions[int(agent["id"])] = key

        captured = [
            int(attacker_id) for attacker_id in record.get("captured_attacker_ids", [])
        ]
        arrivals = [
            int(attacker_id)
            for attacker_id in record.get("base_arrival_attacker_ids", [])
        ]
        arrival_tiles = infer_arrival_tiles(
            arrivals,
            previous_attacker_positions=last_attacker_positions,
            current_attacker_positions=current_attacker_positions,
            base_tiles=base_tiles,
        )
        capture_tiles = infer_capture_tiles(
            captured,
            previous_attacker_positions=last_attacker_positions,
            current_attacker_positions=current_attacker_positions,
        )

        for (q, r), group in grouped_agents.items():
            center_x, center_y = hex_to_xy(q, r, hex_size)
            ordered_group = sorted(group, key=lambda a: int(a["id"]))
            count = len(ordered_group)
            spread_radius = min(0.35, 0.14 + 0.03 * count) * hex_size

            for idx, agent in enumerate(ordered_group):
                if count == 1:
                    x, y = center_x, center_y
                else:
                    angle = (2.0 * math.pi * idx) / count
                    x = center_x + spread_radius * math.cos(angle)
                    y = center_y + spread_radius * math.sin(angle)

                agent_id = int(agent["id"])
                color = agent_colors.get(agent_id, "#000000")
                if str(agent.get("type")) == "ATTACKER":
                    attackers_x.append(x)
                    attackers_y.append(y)
                    attacker_colors.append(color)
                else:
                    defenders_x.append(x)
                    defenders_y.append(y)
                    defender_colors.append(color)

                id_labels.append(
                    ax.text(
                        x + 0.07 * hex_size,
                        y + 0.07 * hex_size,
                        str(agent_id),
                        fontsize=9,
                        color="k",
                        weight="bold",
                        ha="left",
                        va="bottom",
                        zorder=4,
                        path_effects=[
                            pe.Stroke(linewidth=2, foreground="white"),
                            pe.Normal(),
                        ],
                    )
                )

        set_scatter_offsets(attacker_scatter, list(zip(attackers_x, attackers_y)))
        if attacker_colors:
            attacker_scatter.set_facecolors(attacker_colors)
        set_scatter_offsets(defender_scatter, list(zip(defenders_x, defenders_y)))
        if defender_colors:
            defender_scatter.set_facecolors(defender_colors)

        arrival_tile_xy = [hex_to_xy(q, r, hex_size) for (q, r) in arrival_tiles]
        capture_tile_xy = [hex_to_xy(q, r, hex_size) for (q, r) in capture_tiles]
        set_scatter_offsets(arrival_tile_overlay, arrival_tile_xy)
        set_scatter_offsets(capture_tile_overlay, capture_tile_xy)

        cumulative_captures += len(captured)
        cumulative_base_arrivals += len(arrivals)
        event_lines = [
            f"Captured attackers: {cumulative_captures}",
            f"Base arrivals: {cumulative_base_arrivals}",
        ]
        event_text.set_text("\n".join(event_lines))

        step = int(record.get("step", -1))
        step_history.append(step)
        for agent in agents:
            agent_id = int(agent["id"])
            cumulative_rewards[agent_id] += float(agent.get("reward", 0.0))
        for agent_id in all_agent_ids:
            reward_history[agent_id].append(cumulative_rewards[agent_id])
            reward_lines[agent_id].set_data(step_history, reward_history[agent_id])

        reward_ax.set_xlim(0, max(step_history[-1], 1))
        y_values = [value for history in reward_history.values() for value in history]
        if y_values:
            y_min = min(y_values)
            y_max = max(y_values)
            pad = max(1.0, (y_max - y_min) * 0.15)
            reward_ax.set_ylim(y_min - pad, y_max + pad)

        ax.set_title(
            f"Hex World Simulation - Step {step}", fontsize=14, fontweight="bold"
        )
        fig.canvas.draw_idle()
        plt.pause(delay)
        last_attacker_positions = dict(current_attacker_positions)

    plt.ioff()
    plt.show()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Visualize hex-world simulation JSON logs."
    )
    parser.add_argument(
        "--input",
        default="sim.json",
        help="Path to sim json log file (default: sim.json)",
    )
    parser.add_argument(
        "--delay",
        type=float,
        default=0.35,
        help="Delay in seconds between timesteps (default: 0.35)",
    )
    parser.add_argument(
        "--hex-size", type=float, default=1.0, help="Hex size scaling (default: 1.0)"
    )
    args = parser.parse_args()

    steps = load_steps(Path(args.input))
    if not steps:
        raise ValueError("No simulation records found in input.")
    run_viewer(steps, hex_size=args.hex_size, delay=args.delay)


if __name__ == "__main__":
    main()
