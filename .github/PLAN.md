# Perimeter-MG Implementation Plan

## 1. Objective

Build a modular C++ simulation environment for a multi-agent Markov Game in a hex world where defender drones protect a central base from attacker drones. The system must support stochastic transitions, simultaneous actions, and extensibility for future learning algorithms (e.g., Nash Q-learning), without implementing learning itself.

---

## 2. High-Level Architecture

The system should be decomposed into four core modules:

1. Geometry Layer (Hex grid + spatial rules)
2. Environment Layer (state, transitions, rewards)
3. Simulation Engine (time progression)
4. Agent Interface (external policy integration)

Each module must be independently testable.

---

## 3. Implementation Phases

### Phase 1: Geometry & Grid System

#### Goals

* Represent a circular hex grid
* Support extensibility to other geometries

#### Tasks

* Implement axial coordinate system (q, r)
* Implement distance function for hex grids
* Define circular boundary using radius R

  * Valid cells satisfy: hex_distance(center, cell) <= R
* Implement outer ring detection:

  * hex_distance(center, cell) == R
* Define base tiles:

  * Exactly 3 tiles near center (configurable pattern)

#### Deliverables

* Hex struct
* HexGrid class with:

  * isValid(Hex)
  * getNeighbors(Hex)
  * getOuterRing()
  * getBaseTiles()

---

### Phase 2: Core Data Structures

#### Goals

Efficient representation of agents and world state

#### Tasks

* Define AgentType enum (ATTACKER, DEFENDER)
* Define AgentState struct:

  * id
  * type
  * position
* Define WorldState:

  * vector<AgentState>
  * occupancy map: Hex -> vector<agent_id>

#### Deliverables

* AgentState
* WorldState with rebuildOccupancy()

---

### Phase 3: Action & Movement System

#### Goals

Implement simultaneous movement dynamics

#### Tasks

* Define Action enum (6 directions + STAY)
* Implement neighbor(Hex, Action)
* Implement movement resolution:

  * Compute intended positions. When the agent selects an action, the result is stochastic. Observe the following rules:

    * if the intended direction is a valid cell, the agent transitions to the cell in that direction with probability 0.7. The agent transitions in both adjacent directions with probability 0.15.
    * If the agent bumps into a wall (invalid cell), the agent transitions back into the previous cell.
    * If the intended action is to stay, the agent stays with probability 1.

  * Clamp invalid moves to current position
  * Apply all moves simultaneously

#### Deliverables

* Movement utility functions
* Verified movement correctness tests

---

### Phase 4: Environment Dynamics

#### Goals

Implement transition and interaction logic

#### Step Pipeline (must follow strictly)

1. Collect joint actions
2. Compute intended moves
3. Apply movement
4. Rebuild occupancy
5. Resolve captures
6. Handle base arrivals
7. Respawn agents
8. Compute rewards

---

#### Capture Logic

For each cell:

* Partition agents into attackers and defenders
* For each attacker:

  * If 1 defender → P = 0.7
  * If 2 defenders → P = 0.99
  * If ≥3 defenders → P = 0
* Sample stochastic outcome
* Mark captured attackers

Compute capture after all agents have completed their movement.

---

#### Base Interaction

* If attacker enters base tile:

  * Assign +100 reward to each attacker
  * Mark for respawn
* Defenders get -10 reward for occupying a base tile.

---

#### Respawn Logic

* Respawn attackers on outer ring
* Uniform random sampling over outer ring cells
* Allow multiple agents per cell

---

#### Deliverables

* Transition function implementation
* RNG integration (std::mt19937)

---

### Phase 5: Reward System

#### Goals

Compute per-agent rewards accurately

#### Rules

Defenders:

* Movement penalty: -0.1 per step
* Capture reward: m/n per defender (m attackers, n defenders in cell)
* Base breach penalty: -100 to each defender
* -10 reward for occupying base tile

Attackers:

* Movement: 0
* Capture penalty: -100
* Base reward: +100 for each attacker

#### Tasks

* Track capture participants per cell
* Assign shared rewards correctly
* Maintain reward vector aligned with agent IDs

#### Deliverables

* StepResult struct with reward vector

---

### Phase 6: Simulation Engine

#### Goals

Encapsulate environment stepping

#### Tasks

* Implement Simulator class:

  * Holds WorldState
  * step(joint_actions)
* Ensure deterministic ordering of operations

#### Deliverables

* Simulator class
* Fully functional step() method

---

### Phase 7: Initialization System

#### Goals

Flexible environment setup

#### Tasks

* Initialize grid with radius R
* Place base tiles
* Spawn attackers on outer ring
* Spawn defenders away from base

#### Deliverables

* Initialization utilities
* Config struct (recommended)

---

### Phase 8: Agent Interface (No Learning)

#### Goals

Allow external control of agents

#### Tasks

* Define abstract Agent class:

  * act(WorldState) -> Action
* Do NOT implement learning

#### Deliverables

* Agent interface header

---

### Phase 9: Testing & Validation

#### Unit Tests

* Hex distance correctness
* Neighbor computation
* Outer ring detection

#### Simulation Tests

* Movement validity
* Capture probabilities (empirical validation)
* Base interaction correctness
* Respawn distribution

#### Edge Cases

* Multiple defenders (≥3 congestion case)
* Multiple attackers in same cell
* Invalid movement attempts

---

### Phase 10: Extensibility Hooks

#### Requirements

Design must allow:

* Plug-in learning systems

#### Tasks

* Abstract Grid interface
* Avoid hardcoding circular assumptions outside grid module

---

## 4. Suggested File Structure

```
/src
  /geometry
    Hex.h
    HexGrid.h
  /core
    Agent.h
    AgentState.h
    WorldState.h
  /environment
    Simulator.h
    Simulator.cpp
    Transition.cpp
    Reward.cpp
  /utils
    Random.h
  main.cpp
```

---

## 5. Constraints & Guidelines

* Use only STL (no external dependencies)
* Avoid dynamic allocation in inner loops
* Prefer value types and contiguous memory
* Keep logic deterministic except RNG-controlled events

---


## 6. Visualization Considerations

Visualization must remain optional, lightweight, and fully decoupled from the simulation core. The system should support multiple visualization strategies with minimal or zero dependencies.

### Core Design Principle

The simulation environment must expose **pure data only**. No rendering logic, graphics types, or visualization dependencies are allowed in core modules.

---

### Three-Tier Visualization Strategy

#### Tier 1 (Required): Logging Interface (Zero Dependency)

Provide a generic logging interface:

```
class StateLogger {
public:
    virtual void log(const WorldState& state, int step) = 0;
};
```

Implementations:

* JSONLogger

Example JSON schema:

```
{
  "step": 10,
  "agents": [
    {"id":0,"type":"ATTACKER","q":2,"r":-1}
  ],
  "base_tiles": [
    {"q":0,"r":0}
  ]
}
```

Purpose:

* Offline visualization
* Debugging
* Integration with Python, web tools, or notebooks

---

#### Tier 2 (Default): Console Renderer (Zero Dependency)

Provide a simple ASCII-based renderer:

```
class ConsoleRenderer {
public:
    void render(const WorldState& state);
};
```

Features:

* Displays grid approximately in terminal
* Symbols:

  * A = attacker
  * D = defender
  * B = base
* Step-by-step playback

Purpose:

* Debugging
* CI-friendly visualization

---

#### Tier 3 (Optional): Real-Time Renderer (Minimal Dependency)

Provide an optional SFML-based renderer:

* Must be compiled conditionally
* Must not affect core build

Example compile flag:

```
#ifdef ENABLE_VISUALIZATION
#include <SFML/Graphics.hpp>
#endif
```

Renderer responsibilities:

* Convert hex coordinates to 2D positions
* Draw agents and base tiles
* Animate simulation steps

---

### Geometry to Visualization Mapping

Implement hex-to-Cartesian conversion:

* x = size * (3/2 * q)
* y = size * (sqrt(3)/2 * q + sqrt(3) * r)

Ensure consistent scaling and coordinate transforms.

---

### Data Exposure Requirements

Provide read-only accessors:

* getAgentStates()
* getBaseTiles()
* getGridCells()

Do not expose mutable internal structures.

---

### Dependency Policy

* Visualization must not be required to build or run the simulator
* Default build must use only STL
* Optional visualization (e.g., SFML) must be behind compile-time flags
* Avoid heavy frameworks (Qt, OpenGL toolkits)

---

### Recommended Workflow

1. Use ConsoleRenderer for early debugging
2. Use JSON logging for analysis and offline visualization
3. Add SFML renderer only if real-time visualization is needed

