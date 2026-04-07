#!/usr/bin/env python3

import csv
import sys
import matplotlib.pyplot as plt
from pathlib import Path
from collections import defaultdict


def read_csv(csv_path):
    with open(csv_path, newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        rows = list(reader)
    return rows


def plot_solve_times(csv_path):
    rows = read_csv(csv_path)
    if not rows:
        print("No data found in CSV.")
        return

    # Assume columns: 'attackers', 'defenders', 'avg_solve_time_ms'
    attackers = [int(row["attackers"]) for row in rows]
    defenders = [int(row["defenders"]) for row in rows]
    total_agents = [int(row["total_agents"]) for row in rows]
    avg_times = [float(row["avg_solve_time_ms"]) for row in rows]

    # Group by (attackers, defenders) and plot vs total agents
    # grouped = defaultdict(List)
    # for a, d, t in zip(attackers, defenders, avg_times):
    #     total_agents = a + d
    #     grouped[(a, d)].append((total_agents, t))

    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot()
    # for (a, d), vals in sorted(grouped.items()):
    #     vals.sort()
    #     x = [total for total, _ in vals]
    #     y = [t for _, t in vals]
    #     plt.plot(x, y, marker="o", label=f"{a}A/{d}D")
    ax.plot(total_agents, avg_times)
    ax.scatter(total_agents, avg_times)

    plt.xlabel("Total Number of Agents")
    plt.ylabel("Average Solve Time (ms)")
    plt.title("Average Solve Time vs Number of Agents")
    # plt.legend(title="Attackers/Defenders")
    plt.grid(True)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    csv_path = (
        sys.argv[1]
        if len(sys.argv) > 1
        else str(Path(__file__).parent.parent / "docs/average_solve_times.csv")
    )
    plot_solve_times(csv_path)
