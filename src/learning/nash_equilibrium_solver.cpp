#include "perimeter/learning/nash_equilibrium_solver.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

#include <coin/IpIpoptApplication.hpp>
#include <coin/IpTNLP.hpp>

namespace perimeter
{

namespace
{

// NLP problem class for Nash equilibrium
class NashNLP : public Ipopt::TNLP
{
public:
  NashNLP(const JointState& state, const JointRewardFunction& R,
          const JointActionSpace& jointActionSpace, int numAgents, int numActions)
      : jointState_(state)
      , R_(R)
      , jointActionSpace_(jointActionSpace)
      , numAgents_(numAgents)
      , numActions_(numActions)
      , numPolicyVars_(numAgents * numActions)
      , numUtilityVars_(numAgents)
      , numVars_(numPolicyVars_ + numUtilityVars_)
      , numBestResponseConstraints_(numAgents * numActions)
      , numProbSumConstraints_(numAgents)
      , numConstraints_(numBestResponseConstraints_ + numProbSumConstraints_)
  {}

  // Get problem info
  virtual bool get_nlp_info(Ipopt::Index& n, Ipopt::Index& m, Ipopt::Index& nnz_jac_g,
                            Ipopt::Index& nnz_h_lag, IndexStyleEnum& index_style) override
  {
    n = numVars_;
    m = numConstraints_;
    // Jacobian is dense for utility computation
    nnz_jac_g = m * n;
    nnz_h_lag = 0; // We'll use finite differences for Hessian
    index_style = C_STYLE;
    return true;
  }

  // Get bounds on variables and constraints
  virtual bool get_bounds_info(Ipopt::Index n, Ipopt::Number* x_l, Ipopt::Number* x_u,
                               Ipopt::Index m, Ipopt::Number* g_l, Ipopt::Number* g_u) override
  {
    // Variable bounds
    for (int i = 0; i < numPolicyVars_; ++i) {
      x_l[i] = 0.0; // π(a) >= 0
      x_u[i] = 1.0; // π(a) <= 1
    }
    for (int i = 0; i < numUtilityVars_; ++i) {
      x_l[numPolicyVars_ + i] = -1e20; // U unbounded below
      x_u[numPolicyVars_ + i] = 1e20;  // U unbounded above
    }

    // Constraint bounds
    // Best response constraints: U^i - U^i(a^i, π^{-i}) >= 0
    for (int i = 0; i < numBestResponseConstraints_; ++i) {
      g_l[i] = 0.0;
      g_u[i] = 1e20;
    }
    // Probability sum constraints: Σ π^i(a) = 1
    for (int i = 0; i < numProbSumConstraints_; ++i) {
      g_l[numBestResponseConstraints_ + i] = 1.0;
      g_u[numBestResponseConstraints_ + i] = 1.0;
    }

    return true;
  }

  // Get starting point
  virtual bool get_starting_point(Ipopt::Index n, bool init_x, Ipopt::Number* x, bool init_z,
                                  Ipopt::Number* z_L, Ipopt::Number* z_U, Ipopt::Index m,
                                  bool init_lambda, Ipopt::Number* lambda) override
  {
    if (init_x) {
      // Initialize policies to uniform distribution
      for (int i = 0; i < numPolicyVars_; ++i) {
        x[i] = 1.0 / numActions_;
      }
      // Initialize utilities to 0
      for (int i = 0; i < numUtilityVars_; ++i) {
        x[numPolicyVars_ + i] = 0.0;
      }
    }
    return true;
  }

  // Evaluate objective
  virtual bool eval_f(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
                      Ipopt::Number& obj_value) override
  {
    obj_value = 0.0;
    for (int i = 0; i < numAgents_; ++i) {
      double U_i = x[numPolicyVars_ + i]; // U^i variable
      double expectedU_i = computeExpectedUtility(x, i);
      obj_value += (U_i - expectedU_i);
    }
    return true;
  }

  // Evaluate gradient of objective
  virtual bool eval_grad_f(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
                           Ipopt::Number* grad_f) override
  {
    // Initialize gradient
    for (int i = 0; i < n; ++i) {
      grad_f[i] = 0.0;
    }

    // Gradient w.r.t. U^i variables: ∂f/∂U^i = 1
    for (int i = 0; i < numAgents_; ++i) {
      grad_f[numPolicyVars_ + i] = 1.0;
    }

    // Gradient w.r.t. π variables: ∂f/∂π^j(a^j) = -∂U^i/∂π^j(a^j) for all i
    // Agents' policies are independent, so the grad of this is the same thing
    // without using i's product in the computation of U^i
    for (int i = 0; i < numAgents_; ++i) {
      computeUtilityGradient(x, i, grad_f, -1.0);
    }

    return true;
  }

  // Evaluate constraints
  virtual bool eval_g(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index m,
                      Ipopt::Number* g) override
  {
    int constraintIdx = 0;

    // Best response constraints: U^i - U^i(a^i, π^{-i}) >= 0
    for (int i = 0; i < numAgents_; ++i) {
      double U_i = x[numPolicyVars_ + i];
      for (int a = 0; a < numActions_; ++a) {
        double deviationUtility = computeDeviationUtility(x, i, a);
        g[constraintIdx++] = U_i - deviationUtility;
      }
    }

    // Probability sum constraints: Σ_a π^i(a) = 1
    for (int i = 0; i < numAgents_; ++i) {
      double sum = 0.0;
      for (int a = 0; a < numActions_; ++a) {
        sum += x[i * numActions_ + a];
      }
      g[constraintIdx++] = sum;
    }

    return true;
  }

  // Evaluate Jacobian of constraints
  virtual bool eval_jac_g(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index m,
                          Ipopt::Index nele_jac, Ipopt::Index* iRow, Ipopt::Index* jCol,
                          Ipopt::Number* values) override
  {
    if (values == nullptr) {
      // Return structure of Jacobian
      int idx = 0;
      for (int row = 0; row < m; ++row) {
        for (int col = 0; col < n; ++col) {
          iRow[idx] = row;
          jCol[idx] = col;
          ++idx;
        }
      }
    } else {
      // Return values of Jacobian
      // Initialize to zero
      for (int i = 0; i < nele_jac; ++i) {
        values[i] = 0.0;
      }

      int constraintIdx = 0;

      // Best response constraints Jacobian
      for (int i = 0; i < numAgents_; ++i) {
        for (int a = 0; a < numActions_; ++a) {
          int row = constraintIdx;
          // ∂g/∂U^i = 1
          values[row * n + (numPolicyVars_ + i)] = 1.0;
          // ∂g/∂π: Need to compute ∂U^i(a^i, π^{-i})/∂π
          computeDeviationUtilityGradient(x, i, a, &values[row * n], -1.0);
          constraintIdx++;
        }
      }

      // Probability sum constraints Jacobian
      for (int i = 0; i < numAgents_; ++i) {
        int row = constraintIdx;
        for (int a = 0; a < numActions_; ++a) {
          values[row * n + (i * numActions_ + a)] = 1.0;
        }
        constraintIdx++;
      }
    }
    return true;
  }

  virtual void finalize_solution(Ipopt::SolverReturn status, Ipopt::Index n, const Ipopt::Number* x,
                                 const Ipopt::Number* z_L, const Ipopt::Number* z_U, Ipopt::Index m,
                                 const Ipopt::Number* g, const Ipopt::Number* lambda,
                                 Ipopt::Number obj_value, const Ipopt::IpoptData* ip_data,
                                 Ipopt::IpoptCalculatedQuantities* ip_cq) override
  {
    // Store solution
    solution_.resize(n);
    for (int i = 0; i < n; ++i) {
      solution_[i] = x[i];
    }
  }

  const std::vector<double>& getSolution() const { return solution_; }

private:
  // Compute expected utility for agent i: U^i(π) = Σ_a R^i(a) Π_j π^j(a^j)
  double computeExpectedUtility(const double* x, int agentIdx) const
  {
    double utility = 0.0;
    const auto& allJointActions = jointActionSpace_.getJointActionSpace();

    for (const auto& jointAction : allJointActions) {
      // Compute joint probability Π_j π^j(a^j)
      double jointProb = 1.0;
      for (int j = 0; j < numAgents_; ++j) {
        int actionIdx = static_cast<int>(jointAction[j]);
        jointProb *= x[j * numActions_ + actionIdx];
      }

      // Get reward for this joint action
      double reward = R_[agentIdx](jointState_, jointAction);
      utility += reward * jointProb;
    }

    return utility;
  }

  // Compute deviation utility: U^i(a^i, π^{-i})
  double computeDeviationUtility(const double* x, int agentIdx, int actionIdx) const
  {
    double utility = 0.0;
    const auto& allJointActions = jointActionSpace_.getJointActionSpace();
    environment::Action deviationAction = static_cast<environment::Action>(actionIdx);

    for (const auto& jointAction : allJointActions) {
      // Skip if agent i's action doesn't match deviation
      if (jointAction[agentIdx] != deviationAction) {
        continue;
      }

      // Compute joint probability Π_{j≠i} π^j(a^j)
      double jointProb = 1.0;
      for (int j = 0; j < numAgents_; ++j) {
        if (j == agentIdx)
          continue; // Skip agent i
        int aIdx = static_cast<int>(jointAction[j]);
        jointProb *= x[j * numActions_ + aIdx];
      }

      // Get reward
      double reward = R_[agentIdx](jointState_, jointAction);
      utility += reward * jointProb;
    }

    return utility;
  }

  // Compute gradient of expected utility w.r.t. policy variables
  void computeUtilityGradient(const double* x, int agentIdx, double* grad, double scale) const
  {
    const auto& allJointActions = jointActionSpace_.getJointActionSpace();

    for (const auto& jointAction : allJointActions) {
      double reward = R_[agentIdx](jointState_, jointAction);

      // For each agent j and action a^j
      for (int j = 0; j < numAgents_; ++j) {
        int aIdx = static_cast<int>(jointAction[j]);

        // Compute Π_{k≠j} π^k(a^k)
        double productWithoutJ = 1.0;
        for (int k = 0; k < numAgents_; ++k) {
          if (k == j)
            continue;
          int akIdx = static_cast<int>(jointAction[k]);
          productWithoutJ *= x[k * numActions_ + akIdx];
        }

        grad[j * numActions_ + aIdx] += scale * reward * productWithoutJ;
      }
    }
  }

  // Compute gradient of deviation utility
  void computeDeviationUtilityGradient(const double* x, int agentIdx, int actionIdx, double* grad,
                                       double scale) const
  {
    const auto& allJointActions = jointActionSpace_.getJointActionSpace();
    environment::Action deviationAction = static_cast<environment::Action>(actionIdx);

    for (const auto& jointAction : allJointActions) {
      if (jointAction[agentIdx] != deviationAction) {
        continue;
      }

      double reward = R_[agentIdx](jointState_, jointAction);

      // For each agent j ≠ i
      for (int j = 0; j < numAgents_; ++j) {
        if (j == agentIdx)
          continue;

        int aIdx = static_cast<int>(jointAction[j]);

        // Compute Π_{k≠i,k≠j} π^k(a^k)
        double product = 1.0;
        for (int k = 0; k < numAgents_; ++k) {
          if (k == agentIdx || k == j)
            continue;
          int akIdx = static_cast<int>(jointAction[k]);
          product *= x[k * numActions_ + akIdx];
        }

        grad[j * numActions_ + aIdx] += scale * reward * product;
      }
    }
  }

  JointState jointState_;
  JointRewardFunction R_;
  const JointActionSpace& jointActionSpace_;
  int numAgents_;
  int numActions_;
  int numPolicyVars_;
  int numUtilityVars_;
  int numVars_;
  int numBestResponseConstraints_;
  int numProbSumConstraints_;
  int numConstraints_;
  std::vector<double> solution_;
};

} // anonymous namespace

JointPolicy NashEquilibriumSolver::solve(const JointRewardFunction& R,
                                         const JointActionSpace& jointActionSpace,
                                         const JointState& state)
{
  const int numAgents = state.size();
  const int numActions = static_cast<int>(environment::Action::NUM_ACTIONS);

  // Create Ipopt application
  Ipopt::SmartPtr<Ipopt::IpoptApplication> app = IpoptApplicationFactory();

  // Set options
  app->Options()->SetNumericValue("tol", 1e-6);
  app->Options()->SetIntegerValue("max_iter", 1000);
  app->Options()->SetIntegerValue("print_level", 0); // Reduce output
  app->Options()->SetStringValue("hessian_approximation", "limited-memory");

  // Initialize
  Ipopt::ApplicationReturnStatus status = app->Initialize();
  if (status != Ipopt::Solve_Succeeded) {
    std::cerr << "Ipopt initialization failed!" << std::endl;
    return JointPolicy();
  }

  // Create NLP problem
  Ipopt::SmartPtr<NashNLP> nlp = new NashNLP(state, R, jointActionSpace, numAgents, numActions);

  // Solve
  status = app->OptimizeTNLP(nlp);

  if (status != Ipopt::Solve_Succeeded && status != Ipopt::Solved_To_Acceptable_Level) {
    std::cerr << "Ipopt solve failed with status: " << status << std::endl;
    // Return uniform policy as fallback
    JointPolicy policy;
    std::vector<double> uniformWeights(numActions, 1.0 / numActions);
    for (int i = 0; i < numAgents; ++i) {
      policy.push_back(SingleAgentSimpleGamePolicy(uniformWeights));
    }
    return policy;
  }

  // Extract solution and construct JointPolicy
  const std::vector<double>& solution = nlp->getSolution();
  JointPolicy policy;

  for (int i = 0; i < numAgents; ++i) {
    std::vector<double> agentPolicy(numActions);
    for (int a = 0; a < numActions; ++a) {
      agentPolicy[a] = solution[i * numActions + a];
    }
    policy.push_back(SingleAgentSimpleGamePolicy(agentPolicy));
  }

  return policy;
}

} // namespace perimeter
