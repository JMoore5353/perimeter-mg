<img width="2912" height="1440" alt="cover_img" src="https://github.com/user-attachments/assets/16b810d1-05aa-4f3e-a5aa-4b79443e5461" />

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
./build/perimeter_mg --steps 500 --hex-radius 3 --num-attackers 1 --num-defenders 1
```

Then, run
```bash
# Use --help to see all CLI options
python3 scripts/visualize_sim.py --delay 0.1
```

At this point, you will see the visualizer show up with the hex world and a reward plot.

### Replicating results from this write-up
To replicate the single attacker results, use
```bash
./build/perimeter_mg --steps 999 --hex-radius 3 --num-attackers 1 --num-defenders 0 --output 1a.json

python3 scripts/visualize_sim.py --input 1a.json --delay 0.01
```

To replicate the single defender results, use
```bash
./build/perimeter_mg --steps 500 --hex-radius 3 --num-attackers 0 --num-defenders 1 --output 1d.json

python3 scripts/visualize_sim.py --input 1d.json --delay 0.01
```

To replicate the 1 attacker/1 defender results, use
```bash
./build/perimeter_mg --steps 500 --hex-radius 3 --num-attackers 1 --num-defenders 1 --load-qtable "qtables/scenario_1a_1d_3r/scenario_1a_1d_agent*_step45000.bin" --output 1a_1d.json

python3 scripts/visualize_sim.py --input 1a_1d.json --delay 0.1
```

> [!NOTE]
> This last command loads a Q-table that was pre-trained trained on 45000 simulation steps.
> The Q-tables are included in this repository in the `qtables` directory.

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
### Validation of Nash Q-learning approach
To validate that the Q-learning approach resulted in expected behavior, I ran two PerimeterMG experiments, one with a single attacker and another with a single defender.
In the single agent case, the Markov game reduces to a Markov Decision Problem, and agents should learn to maximize their reward.

#### Single defender case
As shown in the below graphic, the defender quickly learns to stop moving on a hex tile that is not a base tile.
This makes sense since defenders get penalized for both moving and being located on a base tile.
This validates the Nash Q-learning approach for the single defender case.

https://github.com/user-attachments/assets/efeeae50-d69e-4eb9-a7de-25d46bab2c3e

#### Single attacker case
As shown in the below graphic, the single attacker begins by moving randomly in the environment.
After a few hundred training steps, it learns to move directly toward the goal tiles to maximize its reward.
Since the attacker is only rewarded for arriving at a base tile, this behavior makes sense.
This validates the Nash Q-learning approach for the single attacker case.

https://github.com/user-attachments/assets/b767d363-c99d-4704-be01-9b9055219631

### Multi-agent PerimeterMG
#### 1 attacker, 1 defender
The below image shows one attacker and one defender agents playing each other in simulation after about 25000 simulation steps.
The attacker agent successfully learns to navigate toward the base tiles and tries to avoid the defender (steps 25150 to 25160).
More training is needed to converge to an optimal policy, since there are times where the defender is far away, but the attacker appears to move randomly without advancing toward the goal.
This is likely caused by insufficient exploration of these joint state spaces, resulting a random policy (according to the $\epsilon\text{-greedy}$ exploration strategy).

https://github.com/user-attachments/assets/f7d02ea7-0ab8-41f9-8be5-334c6be8f87f

The defender also learns sensible behavior.
It limits its movement while (usually) staying close to the base station, and does not come to rest on the base tiles.
When near the attacker, it chases the attacker, as in steps 25150-15160 and 25490-25500.
Much more training is needed for the defender to come to a better policy, since it sometimes tends to wander off and stop moving near the edge of the map--a more sensible policy might result in the defender staying closer to the base tiles. 

It is also interesting to note that occasionally, the attacker and defender get stuck in a gridlock.
This is seen in the beginning and end of the video.
Apparently, in these states, the Nash equilibrium results in a policy of both agents to not move.

After a while, the agents eventually start moving again, maybe due to the reward function (based on the Q-table) converging to a different value, or a non-STAY action being drawn from the equilibrium policy.

#### 1 attacker, 2 defenders


#### 2 attackers, 3 defenders


### Curse of dimensionality
One of the main challenges with this approach is computational complexity and the curse of dimensionality.
Nash Q-learning relies on a Q-table, which tabulates the value of state-action pairs.
In the single agent case, there are $O(|S||A|)$ of these state-action pairs.
In the multi-agent case, these are **joint** state and **joint** action pairs, so now $S= S^0 \times S^1 \times \dots \times S^n$ and $A=A^0 \times A^1 \times \dots \times A^n$.

Thus, for PerimeterMG on a hex world grid of radius 3, there are 37 states that each agent can occupy, and 7 actions, resulting in $|S|=37^n$ and $|A|=7^n$.
For even just 3 agents, each agent's Q-table is a $37^3$ by $7^3$ matrix, having 17373979 entries.

To reach convergence for the entire Q-table, I would have to run many simulation steps for each of the more than 17 million Q-table entries--just for the 3-agent case.

Since agents in my implementation of Nash Q-learning follow an $\epsilon\text{-greedy}$ policy, they choose a random action at a joint state with probability $\epsilon$, where $\epsilon$ is inversely proportional to the number of times that joint state has been visited.
Thus, as the number of agents and available hex grids increase, the number of training steps required to converge to a non-random policy increases dramatically.

In other words, in the previous Q-table example, to reduce the probability $\epsilon$ of choosing a random action at a given state to just 33%, then each joint state would be need to be visited 3 times, resulting in 151959 simulation steps.
I do not do target training of for each state, so in reality many, many more simulation steps would be required.

### Complexity of solving Nash equilibrium
Solving a Nash equilibrium for a simple game is PPAD-complete, meaning there is no known polynomial time solution for solving for a Nash equilibrium.
Nash Q-learning requires solving for a simple game Nash equilibrium at every time step.

This step is the bottleneck for my simulations.
For just 2 agents, it takes on average 28ms to compute a Nash equilibrium, and scales poorly with the number of agents.

<img width="800" height="600" alt="solve_times" src="https://github.com/user-attachments/assets/d6ae243d-9758-4eff-bd99-53879c55bf91" />

## AI Usage
Generative AI was used extensively in this project to generate code.
The simulation environment, simulation environment test cases, plotters, q-table checkpoint serializers, and the IPOPT simple game Nash solver were written using AI.
I thoroughly reviewed every line of AI-generated code, with the exception of some of the serializer unit tests.

I wrote everything else in the Nash Q-learning module (`learning/` directory) manually.
