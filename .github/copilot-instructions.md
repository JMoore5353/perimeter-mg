# Perimeter-MG Development Guide

A modular C++20 hex-world multi-agent Markov game simulator where defenders protect a base from attackers.

## Build, Test, and Run

### Build
```bash
cmake -S . -B build
cmake --build build -j
```

### Test
```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run a specific test
./build/geometry_tests --gtest_filter="HexGeometryTest.Distance"
./build/geometry_tests --gtest_filter="TransitionTest.*"
```

### Run Simulation
```bash
# Generate simulation log
./build/perimeter_mg

# Visualize (writes sim.json by default)
python3 scripts/visualize_sim.py --input sim.json --delay 0.35
```

### Format Code
```bash
# Uses ROS2 code style (.clang-format)
clang-format -i src/**/*.cpp include/**/*.h
```

## Architecture Overview

The system is organized into five core modules:

### 1. Geometry Layer (`geometry/`)
- **Hex**: Axial coordinate system (q, r, s) for hexagonal grids
- **Grid** interface: Abstract grid operations for extensibility
- **HexGrid**: Circular hex world implementation
  - Validates cells by `hex_distance(center, cell) <= radius`
  - Outer ring is cells where `hex_distance(center, cell) == radius`
  - Base tiles are 3 specific tiles near center

### 2. Core Layer (`core/`)
- **AgentState**: Agent properties (id, type, position)
- **WorldState**: Master state container
  - `agents` vector: All active agents
  - `occupancy` map: `Hex -> vector<agent_id>` for spatial queries
  - **Important**: Always call `rebuildOccupancy()` after modifying agent positions

### 3. Environment Layer (`environment/`)
- **Action**: 7 actions (6 directions + STAY)
- **Movement**: Stochastic movement resolution
- **Transition**: State transitions following strict step pipeline (see below)
- **Simulator**: Orchestrates environment stepping
- **Initialization**: Environment setup from config

### 4. Learning Layer (`learning/`)
- **JointPolicy**: Multi-agent policy representation
- **NashQLearning**: Nash Q-learning algorithm implementation
- **SimpleGameSolverInterface**: Abstract solver interface for game theory solvers
- **CorrelatedEquilibriumSolver**: Correlated equilibrium computation using HiGHS LP solver

### 5. Visualization Layer (`visualization/`)
- **ViewerBackend**: Abstract rendering interface
- **AsciiViewer**: Terminal-based visualization
- **JsonViewer**: Outputs `sim.json` for Python plotting

## Critical Behavioral Rules

### Stochastic Movement
When an agent selects a direction action:
- **Valid direction**: 0.7 probability for intended cell, 0.15 for each adjacent direction
- **Invalid direction** (hits wall): Agent stays in current cell
- **STAY action**: Deterministic (probability 1.0)

### Capture Mechanics
Applied **after** all movement is resolved:
- **1 defender** in cell with attacker(s): 0.7 capture probability per attacker
- **2 defenders**: 0.99 capture probability
- **≥3 defenders**: 0.0 capture probability (congestion rule)
- Each attacker is evaluated independently using `std::mt19937` RNG

### Step Pipeline (Strict Order)
The `stepWorld` function must execute in this exact order:
1. Collect joint actions
2. Compute intended moves (apply stochastic movement)
3. Apply all movements simultaneously
4. Rebuild occupancy map
5. Resolve captures
6. Handle base arrivals
7. Respawn captured/arrived attackers on outer ring
8. Compute rewards

### Reward Model
**Attackers**:
- Capture: -100
- Base arrival: +100
- Movement: 0

**Defenders**:
- Movement (action ≠ STAY): -0.1
- Capture: `m/n` per defender (m attackers captured, n defenders in cell)
- Base breach: -100 (when attacker reaches base)
- Base occupancy: -10 (for standing on base tile)

## Code Conventions

### Namespace Structure
- Use nested namespaces: `perimeter::geometry`, `perimeter::core`, `perimeter::environment`, `perimeter::learning`, `perimeter::visualization`
- Implementation files should open namespace only once at top

### Header Guards
- Use `#ifndef` guards, not `#pragma once`
- Format: `PERIMETER_PERIMETER_<MODULE>_<FILE>_H`
- Example: `PERIMETER_PERIMETER_GEOMETRY_HEX_H`

### Include Order
1. Corresponding header (for .cpp files)
2. Standard library headers
3. Third-party library headers (e.g., HiGHS)
4. Project headers (use `perimeter/` prefix)

### Formatting
- Follow ROS2 code style (see `.clang-format`)
- 100 character line limit
- 2-space indentation
- Pointer/reference alignment: left (`Type* ptr`, not `Type *ptr`)

### Memory and Performance
- Avoid dynamic allocation in inner loops
- Prefer value types and contiguous memory (vectors over lists)
- Use `const` and `noexcept` where applicable
- Reserve vector capacity when size is known

### Random Number Generation
- Use `std::mt19937` for all stochastic operations
- Pass RNG by reference through function calls
- Seed is configurable via `InitializationConfig`

## Key Data Structures

### StepResult
Returned by `stepWorld`, contains:
- `rewards`: Per-agent reward vector
- `capturedAttackerIds`: IDs of attackers captured this step
- `baseArrivalAttackerIds`: IDs of attackers that reached base
- `respawnedAttackerIds`: IDs of attackers respawned this step

### InitializationConfig
```cpp
struct InitializationConfig {
    int radius;              // Hex grid radius
    int attackerCount;       // Number of attackers
    int defenderCount;       // Number of defenders  
    unsigned int seed;       // RNG seed
};
```

## JSON Output Format

The `JsonViewer` produces `sim.json` consumed by `scripts/visualize_sim.py`:

**Static fields** (emitted at step 0):
- `world_tiles`: All valid hex coordinates
- `base_tiles`: Base tile coordinates

**Per-step fields**:
- `step`: Current timestep
- `agents`: Array of `{id, type, q, r, reward}`
- `captured_attacker_ids`: Event list
- `base_arrival_attacker_ids`: Event list
- `respawned_attacker_ids`: Event list

## Python Visualizer

`scripts/visualize_sim.py` features:
- **Upper plot**: Hex grid with agents (stable per-agent colors), base tiles, event boxes
- **Lower plot**: Cumulative reward curves per agent
- Handles agent overlap with circular spread pattern
- Agent ID labels with stroke for readability

## Testing Strategy

### Unit Tests
- One test file per module (e.g., `hex_geometry_test.cpp`, `transition_test.cpp`)
- Use GoogleTest framework
- Test naming: `<ModuleName>Test.<Behavior>`

### Key Test Areas
- Hex distance and neighbor computation
- Outer ring detection and base tile placement
- Movement stochasticity (empirical validation with many samples)
- Capture probabilities (1, 2, 3+ defenders)
- Reward computation correctness
- Respawn distribution uniformity

### Running Specific Tests
```bash
# By test suite
./build/geometry_tests --gtest_filter="HexGeometryTest.*"

# By specific test
./build/geometry_tests --gtest_filter="TransitionTest.CaptureWithTwoDefenders"

# List available tests
./build/geometry_tests --gtest_list_tests
```

## Dependencies

- **CMake**: ≥3.16
- **C++ Standard**: C++20
- **HiGHS**: Linear programming solver (fetched via CMake)
  - Used for correlated equilibrium computation
  - Automatically downloaded and built
- **GoogleTest**: Testing framework (fetched via CMake)
- **Python 3**: For visualization (matplotlib required)

## Design Principles

### Extensibility
- Grid system is abstract (`Grid` interface) to support future geometries
- Learning algorithms plug into the `SimpleGameSolverInterface`
- Visualization is fully decoupled from simulation core

### Determinism
- All stochastic behavior is RNG-controlled
- Given same seed, simulation is fully reproducible
- Use `std::mt19937` consistently

### Zero External Dependencies (Core)
- Simulation core uses only STL
- HiGHS is only for learning algorithms
- Visualization is optional and decoupled

## Common Pitfalls

1. **Forgetting to rebuild occupancy**: After modifying agent positions, always call `world.rebuildOccupancy()`
2. **Wrong step order**: The 8-step pipeline in `stepWorld` must be followed exactly
3. **Mixing up coordinate systems**: Use axial (q, r) internally; convert to Cartesian only for visualization
4. **Capture timing**: Captures are resolved after all movement, not during
5. **Congestion rule**: ≥3 defenders means 0.0 capture probability, not 1.0
6. **Include guard names**: Must match the verbose format (`PERIMETER_PERIMETER_...`)
