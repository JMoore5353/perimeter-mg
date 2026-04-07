# PerimeterMG

This repo contains a C++ based simulation environment to run **PerimeterMG**, a border-protection Markov game.
The scenario is based on the *predator-prey hex world* scenario from the *Algorithms for Decision Making* book by Kochenderfer, et al, and is a good environment to learn about game-theoretic approaches to decision problems.

PerimeterMG is modeled as a sequential Markov game, and is solved using Nash Q-learning.

## Installation
The main C++ dependency is the IPOPT solver to solve for a Nash equilibrium for simple games.
If you don't have `zlib`, you'll also need to install that for serializing the Q-tables during checkpoints.
Install with
```bash
sudo apt install coinor-libipopt-dev zlib1g-dev -y
```

You will need a Python virtual environment with `matplotlib` and `numpy` installed for visualization.

Clone this repository.

## Building and running
Building:
```bash
cd ~/path/to/cloned/perimeter-mg
mkdir build
cmake -B build -S . && cmake --build build

# Run unit tests
./build/geometry_tests
```

Running a simulation occurs in two steps.
First, we'll run the simulation, which will generate some output files.
Then, we'll run the Python visualizer that uses the output files.
```bash
cd ~/path/to/cloned/perimeter-mg

# Use --help to see all CLI options
./build/perimeter_mg --steps 10000 --hex-radius 3 --num-attackers 1 --num-defenders 1
```

Then, run
```bash
# Use --help to see all CLI options
python3 scripts/visualize_sim.py --delay 0.1
```

At this point, you will see the visualizer show up with the hex world and a reward plot.

## Game description
Two types of agents exit in PerimeterMG: attackers and defenders.
The goal of the defenders is to capture attackers before they reach base tiles (located in the center of the hex world).
The goal of the attackers is to reach the base without getting caught.

Upon reaching the base or getting caught by defenders, attackers respawn on the outer ring of the hex world.
Gameplay continues indefinitely.

### States
Each agent $i \in {I}$ occupies a single hex in the hex grid with coordinates `(q,r)`.
The number of agents is given by $|{I}|$.
The shared state state space is the tuple of all agents' hex grid coordinates.
Any hex grid (including base tiles) may be occupied by one or more agents.

In this formulation, it is assumed that each agent has **full observability** of all agents' states.

### Actions
Both defenders and attackers share the same action space.
Agents can choose to move in one of the 6 hex directions, or to stay.
The $i\text{th}$ agent's action space, ${A}^i$ is given by

$$
{A}^i = \\{ E, NE, NW, W, SW, SE, STAY \\}
$$

### Movement and transition
PerimeterMG uses a stochastic transition function for the agents.
After choosing an intended action, the actual movement occurs according to

$$
\text{Actual action} = \begin{cases}
\text{right of intended with } & P = (1 - P_\text{intended}) / 2 \\
\text{intended with } & P = P_\text{intended} \\
\text{left of intended with } & P = (1 - P_\text{intended}) / 2
\end{cases}
$$

where $P_\text{intended}$ is `INTENDED_ACTION_PROBABILITY` and is defined in `Movement.h`.

Defenders capture attackers by occupying the same hex cell as the attackers.
If 2 defenders occupy the same cell, then the probability of capture increases.
However, if there are more than 2 defenders in a cell, then the cell becomes too congested for a capture, and the capture fails.
Capture probabilities are given by

$$
P(capture) =
\begin{cases}
0.7  & \text{if 1 defender in cell} \\
0.99 & \text{if 2 defenders in cell} \\
0.0  & \text{if >2 defenders in cell} \\
\end{cases}
$$

Upon reaching the base or getting caught by defenders, attackers respawn on the outer ring of the hex world and gameplay continues.

Other transition models are identical to the hex-world problem in *Algorithms for Decision Making*.

### Reward models
Attackers are rewarded if they arrive at the base, and are penalized if they are captured.
Defenders are rewarded for capturing attackers, but are penalized for attackers arriving at the base.

- Defenders move tiles with $R=-0.1$
- $n$ defenders neutralizing $m$ attackers get $R=\text{BONUS} * m/n$ reward each
- Defenders get $R=-100$ if attacker makes it to base hex tile
- Attackers move tiles with $R=0$
- Attackers get $R=-100$ if they are neutralized
- Attackers get $R=100$ if they arrive at base hex tile

These rewards can be adjusted in `Transition.h`

## Solution Approach
I chose to use Nash Q-learning to solve this Markov game.
Specifically, each agent maintains an estimate of a Q-table $Q(s,a)$, where $s$ is the **joint state** and $a$ is a **joint action**.

At each timestep, a simple game (i.e. one that has no state) is formed using the same agents with the same action spaces, but the $i\text{th}$ agent's reward function $R^i$ is derived from

$$
R^i(a) = Q^i(s,a).
$$

The reward function $\boldsymbol{R}$ is just a vector of each agent's reward function $R^i$.

This simple game is solved by computing a Nash equilibrium using a linear program, which returns a joint (but jointly independent) policy, $\pi$.
I used the IPOPT solver to solve this linear program.

We can compute the expected utility under the joint policy using

$$
U^i(s) = \sum_{a^\prime \in {A}} Q(s, a^\prime) \prod_{j\in {I}} \pi^{j\prime}(a^{j\prime})
$$

where $\pi^{j\prime}$ is the $j\text{th}$ agent's policy (computed from before), and ${A}$ is the joint action space.
The learning rate $\alpha$ is computed using $\alpha = 1 / \sqrt{N(s, a)}$, where $N(s,a)$ is the state-action count.

The Q-table is then updated with

$$
Q^i(s^\prime,a) = Q^i(s^\prime,a) + \alpha (R^i(s^\prime,a) + \gamma U^\pi(s,a) - Q(s^\prime,a))
$$

where $s\prime$ is the **previous state**.

Similar to the approach in the book, I used an epsilon-greedy approach to encourage exploration.
With probability $\epsilon=1 / N(s)$, the agent would choose a uniformly random action, otherwise it used the Nash equilibrium policy.

### Approach justification
The Markov game (MG) formulation is necessary since there are multiple agents with different reward functions and each agent has multiple states.
Other frameworks presented in the *Algorithms for Decision Making* book (e.g. simple games or MDPs) are unable to capture multi-agent problems with multiple states.

To solve the MG, I could compute the Nash equilibrium of the game, but this is computationally intensive, requiring a nonlinear program which is PPAD-complete.
Instead, I will use the Nash Q-learning algorithm presented in the book, where all agents (attackers and defenders) learn in simulation how to best respond to other agents' policies.

In hindsight, it would have been interesting to compute the Nash equilibrium using nonlinear programming and compare runtime and solutions to the Nash Q-learning approach.
While in theory the linear programs solved inside of the Nash Q-learning approach can be solved in polynomial time, I have to solve one at each simulation step, where the nonlinear program would only have to be run one time.

## Results and discussion
### Curse of dimensionality
One of the main challenges with this approach is computational complexity and the curse of dimensionality.
Nash Q-learning relies on a Q-table, which tabulates the value of state-action pairs.
In the single agent case, there are $O(|S||A|)$ of these state-action pairs.
In the multi-agent case, these are **joint** state and **joint** action pairs, so now $S= S^0 \times S^1 \times \dots \times S^n$ and $A=A^0 \times A^1 \times \dots \times A^n$.

Thus, for PerimeterMG on a hex world grid of radius 3, there are 37 states that each agent can occupy, and 7 actions, resulting in $|S|=37^n$ and $|A|=7^n$.
For even just 3 agents, the Q-table is a $37^3$ by $7^3$ matrix, having 17373979 entries.

To reach convergence for the entire Q-table, I would have to run well over 17 million simulation steps just for the 3-agent case.

### Complexity of solving Nash equilibrium
Solving a Nash equilibrium for a simple game is PPAD-complete, meaning there is no known polynomial time solution for solving for a Nash equilibrium.
However, Nash Q-learning requires solving for a Nash equilibrium at every time step.

This step is the bottleneck for my simulations.
For just 2 agents, it takes on average 28ms to compute a Nash equilibrium on new-ish hardware.


## AI Usage
Generative AI was used extensively in this project to generate code.
The simulation environment, simulation environment test cases, plotters, q-table checkpoint serializers, and the IPOPT solver were written using AI.
I thoroughly reviewed **every line of AI-generated code**, with the exception of some of the serializer test cases.

I wrote everything else in the Nash Q-learning module (`learning/` directory) manually.
