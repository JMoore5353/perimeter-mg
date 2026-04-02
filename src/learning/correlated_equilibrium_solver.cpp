#include "perimeter/learning/correlated_equilibrium_solver.h"

JointPolicy CorrelatedEquilibriumSolver::solve(const JointRewardFunction& R,
                                               const JointActionSpace& jointActionSpace,
                                               const JointState& state)
{
  // TODO: Continue here! Update the cost function for the problem based on the conversation with Gemini.
  // TODO: Verify Copilot's output. You asked it to make the R function, but I think it is computing utility...

  // TODO: Write this solve function using the HiGHS linear program solver. Use the following maximization equation
  // with the following constraints, reproduced here in LaTex:
  // \begin{equation}
  // \begin{split}
  // & \max_\pi \sum_i \sum_{\boldsymbol{a}} R^i(\boldsymbol{a}) \pi(\boldsymbol{a}) \\ 
  // & \text{subject to} \sum_{\boldsymbol{a}^{-i}} R^i(a^i) \pi(a^i, \boldsymbol{a}^{-1}) \geq \sum_{\boldsymbol{a}^{-i}} R^i(a^{i\prime},\boldsymbol{a}^{-1}) \pi(a^i, \boldsymbol{a}^{-i}) \\
  // &\sum_\boldsymbol{a} \pi(\boldsymbol{a}) = 1 \\
  // &\pi(\boldsymbol{a}) \geq 0 \hspace{10pt} \forall \boldsymbol{a}
  // \end{split}
  // \end{equation}
  //
  // Here, \boldsymbol{a} refers to a JointAction (defined in joint.h)
  // \boldsymbol{a}^{-i} refers to the other agents' actions (everyone but agent i)
  // \R^i refers to the ith agent's reward model. The R function passed into the solve function takes in a JointAction and returns a JointReward object, a reward for each agent.
  // \ The R(a^i, a^{-i}) notation simply refers to a call to R(a)
  //
  // IMPORTANT note: The type of agent can be determined using the vector of AgentState objects.
  // Defender agents should maximize the sum of the reward over only the i defender agents
  // Likewise, attacker agents should maximize the sum of the reward over only the i attacker agents
  // This likely means there will be two optimizations that are run.
  //
  // As a warning, please keep the following in mind. There will be at most 7 agents, but that means
  //   there will be >7^n constraints, so speed is paramount. This LP will need to be solved every
  //   simulation timestep. When possible, do the following:
  //
  // 1. Reuse the model
  // Build constraint matrix once
  // Only update objective coefficients
  // 2. Use warm starts
  // Reuse basis between iterations
  // 3. Use sparse structures
  // Avoid dense matrix construction
  // 4. Avoid full rebuild per step
  // This alone can dominate runtime
  //
  // Please keep all code readable, understandable, and modular. Use C++ best practices

  return JointPolicy();
}
