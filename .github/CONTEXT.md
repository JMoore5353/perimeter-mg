# Perimeter-MG Agent Context

## What this repo is
- A modular C++17 hex-world multi-agent Markov game simulator.
- Core scenario: defenders protect base tiles; attackers try to reach base.
- Outputs simulation logs (`sim.json`) consumed by a Python visualizer.

## Current architecture
- `include/perimeter/**`: public headers (header guards, no `#pragma once`).
- `src/**`: implementations.
- `tests/**`: GoogleTest suite.
- `scripts/visualize_sim.py`: matplotlib playback + reward plotting.

### Main modules
- `geometry`: `Hex`, `Grid` interface, `HexGrid` implementation.
- `core`: `AgentState`, `WorldState` + occupancy rebuild.
- `environment`: actions, stochastic movement, transitions/rewards, init, simulator.
- `visualization` (C++): backend interface + ASCII/JSON renderers.

## Important behavior implemented
- Simultaneous stochastic movement:
  - intended valid direction: `0.7`, adjacent directions: `0.15/0.15`
  - invalid moves clamp to prior cell
  - `STAY` is deterministic
- Capture probabilities by defenders in same cell:
  - 1 defender: `0.7`
  - 2 defenders: `0.99`
  - >=3 defenders: `0.0` (congestion rule)
- Attackers reaching base are marked and respawned.
- Captured attackers are respawned on outer ring (uniform sampling).

## Reward model (per step)
- Attackers:
  - capture: `-100`
  - base arrival: `+100`
- Defenders:
  - movement penalty: `-0.1` when action != `STAY`
  - capture reward sharing: `m/n` per defender in a capture cell
  - base breach penalty: `-100`
  - base occupancy penalty: `-10`

## Key data contracts
- `environment::StepResult` includes:
  - `rewards`
  - `capturedAttackerIds`
  - `baseArrivalAttackerIds`
  - `respawnedAttackerIds`
- Viewer backend API consumes full `StepResult`:
  - `render(state, grid, step, stepResult)`

## JSON logging format (C++ `JsonViewer`)
- Static fields (`world_tiles`, `base_tiles`) emitted once at `step == 0`.
- Per-step fields include agents (`id`, `type`, `q`, `r`, `reward`) and event ID lists.

## Python visualization state
- File: `scripts/visualize_sim.py`
- Supports concatenated JSON objects or JSON array input.
- Upper plot:
  - hex grid + base tiles
  - agents with per-agent stable colors
  - agent ID labels with stroke for readability
  - overlap handling: agents sharing one hex are arranged in a small circular spread
  - event box for captured/base-arrival attacker IDs
- Lower plot:
  - accumulated reward curves per agent
  - same colors as upper plot for traceability

## Build, test, run
- Configure/build:
  - `cmake -S . -B build`
  - `cmake --build build -j`
- Tests:
  - `ctest --test-dir build --output-on-failure`
- Run sim executable:
  - `./build/perimeter_mg` (writes `sim.json`)
- Run visualizer:
  - `python3 scripts/visualize_sim.py --input sim.json --delay 0.35`

## Notable files
- `CMakeLists.txt`: library/executable + GTest integration.
- `src/main.cpp`: simple random-policy driver producing `sim.json`.
- `include/perimeter/environment/Transition.h`: reward constants + `StepResult`.
- `include/perimeter/visualization/ViewerBackend.h`: backend interface.
- `tests/transition_test.cpp`, `tests/visualization_test.cpp`: highest-signal behavior checks.

## Current caveats / next likely improvements
- `src/main.cpp` still has exploratory comments and duplicate include of `Action.h`.
- Include guard names are functional but verbose (`PERIMETER_PERIMETER_*`).
- If agent counts become large, label clutter in matplotlib may need optional toggles.
