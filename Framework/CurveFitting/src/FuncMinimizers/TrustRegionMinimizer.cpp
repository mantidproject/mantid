// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// This code was originally translated from Fortran code on
// https://ccpforge.cse.rl.ac.uk/gf/project/ral_nlls June 2016
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/TrustRegionMinimizer.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/RalNlls/TrustRegion.h"

#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

// clang-format off
///@cond nodoc
DECLARE_FUNCMINIMIZER(TrustRegionMinimizer, Trust Region)
///@endcond
// clang-format on

TrustRegionMinimizer::TrustRegionMinimizer() : m_function() {
  declareProperty("InitialRadius", 100.0,
                  "Initial radius of the trust region.");
}

/** Name of the minimizer.
 */
std::string TrustRegionMinimizer::name() const { return "Trust Region"; }

/** Initialise the minimizer.
 *  @param costFunction :: The cost function to minimize. Must be the least
 *  squares.
 *  @param maxIterations :: Maximum number of iterations that the minimiser will
 *  do.
 */
void TrustRegionMinimizer::initialize(API::ICostFunction_sptr costFunction,
                                      size_t maxIterations) {
  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          costFunction);
  if (!m_leastSquares) {
    throw std::runtime_error(
        "Trust-region minimizer can only be used with Least "
        "squares cost function.");
  }

  m_function = m_leastSquares->getFittingFunction();
  auto &values = *m_leastSquares->getValues();
  auto n = static_cast<int>(m_leastSquares->nParams());
  auto m = static_cast<int>(values.size());
  if (n > m) {
    throw std::runtime_error("More parameters than data.");
  }
  m_options.maxit = static_cast<int>(maxIterations);
  m_workspace.initialize(n, m, m_options);
  m_x.allocate(n);
  m_leastSquares->getParameters(m_x);
  int j = 0;
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (m_function->isActive(i)) {
      m_J.m_index.push_back(j);
      j++;
    } else
      m_J.m_index.push_back(-1);
  }
  m_options.initial_radius = getProperty("InitialRadius");
}

/** Evaluate the fitting function and calculate the residuals.
 *  @param x :: The fitting parameters as a fortran 1d array.
 *  @param f :: The output fortran vector with the weighted residuals.
 */
void TrustRegionMinimizer::evalF(const DoubleFortranVector &x,
                                 DoubleFortranVector &f) const {
  m_leastSquares->setParameters(x);
  auto &domain = *m_leastSquares->getDomain();
  auto &values = *m_leastSquares->getValues();
  m_function->function(domain, values);
  auto m = static_cast<int>(values.size());
  if (f.len() != m) {
    f.allocate(m);
  }
  for (size_t i = 0; i < values.size(); ++i) {
    f.set(i, (values.getCalculated(i) - values.getFitData(i)) *
                 values.getFitWeight(i));
  }
}

/** Evaluate the Jacobian
 *  @param x :: The fitting parameters as a fortran 1d array.
 *  @param J :: The output fortran matrix with the weighted Jacobian.
 */
void TrustRegionMinimizer::evalJ(const DoubleFortranVector &x,
                                 DoubleFortranMatrix &J) const {
  m_leastSquares->setParameters(x);
  auto &domain = *m_leastSquares->getDomain();
  auto &values = *m_leastSquares->getValues();
  auto n = static_cast<int>(m_leastSquares->nParams());
  auto m = static_cast<int>(values.size());
  if (J.len1() != m || J.len2() != n) {
    J.allocate(m, n);
  }
  m_J.setJ(J.gsl());
  m_function->functionDeriv(domain, m_J);
  for (int i = 1; i <= m; ++i) {
    double w = values.getFitWeight(i - 1);
    for (int j = 1; j <= n; ++j) {
      J(i, j) *= w;
    }
  }
}

/** Evaluate the Hessian
 *  @param x :: The fitting parameters as a fortran 1d array.
 *  @param f :: The fortran vector with the weighted residuals.
 *  @param h :: The fortran matrix with the Hessian.
 */
void TrustRegionMinimizer::evalHF(const DoubleFortranVector &x,
                                  const DoubleFortranVector &f,
                                  DoubleFortranMatrix &h) const {
  UNUSED_ARG(x);
  UNUSED_ARG(f);
  auto n = static_cast<int>(m_leastSquares->nParams());
  if (h.len1() != n || h.len2() != n) {
    h.allocate(n, n);
  }
  // Mantid fit functions don't calculate second derivatives.
  // For now the Hessian will not be used.
  h.zero();
}

/** Perform a single iteration.
 */
bool TrustRegionMinimizer::iterate(size_t /*iteration*/) {
  int max_tr_decrease = 100;
  auto &w = m_workspace;
  auto &options = m_options;
  auto &inform = m_inform;
  auto &X = m_x;
  int n = m_x.len();
  int m = static_cast<int>(m_leastSquares->getValues()->size());

  if (w.first_call == 0) {

    w.first_call = 1; // ?

    // evaluate the residual
    evalF(X, w.f);
    inform.f_eval = inform.f_eval + 1;

    // and evaluate the jacobian
    evalJ(X, w.J);
    inform.g_eval = inform.g_eval + 1;

    if (options.relative_tr_radius == 1) {
      // first, let's get diag(J^TJ)
      double Jmax = 0.0;
      for (int i = 1; i <= n; ++i) {
        // note:: assumes column-storage of J
        // JtJdiag = norm2( w.J( (i-1)*m + 1 : i*m ) );
        double JtJdiag = 0.0;
        for (int j = 1; j <= m; ++j) { // for_do(j, 1, m)
          JtJdiag += pow(w.J(j, i), 2);
        }
        JtJdiag = sqrt(JtJdiag);
        if (JtJdiag > Jmax)
          Jmax = JtJdiag;
      }
      w.Delta = options.initial_radius_scale * (pow(Jmax, 2));
    } else {
      w.Delta = options.initial_radius;
    }

    if (options.calculate_svd_J) {
      // calculate the svd of J (if needed)
      NLLS::getSvdJ(w.J, w.smallest_sv(1), w.largest_sv(1));
    }

    w.normF = NLLS::norm2(w.f);
    w.normF0 = w.normF;

    // g = -J^Tf
    NLLS::multJt(w.J, w.f, w.g);
    w.g *= -1.0;
    w.normJF = NLLS::norm2(w.g);
    w.normJF0 = w.normJF;
    w.normJFold = w.normJF;

    // save some data
    inform.obj = 0.5 * (pow(w.normF, 2));
    inform.norm_g = w.normJF;
    inform.scaled_g = w.normJF / w.normF;

    // if we need to output vectors of the history of the residual
    // and gradient, the set the initial values
    if (options.output_progress_vectors) {
      w.resvec(1) = inform.obj;
      w.gradvec(1) = inform.norm_g;
    }

    // Select the order of the model to be used..
    switch (options.model) {
    case 1: // first-order
    {
      w.hf.zero();
      w.use_second_derivatives = false;
      break;
    }
    case 2: // second order
    {
      if (options.exact_second_derivatives) {
        evalHF(X, w.f, w.hf);
        inform.h_eval = inform.h_eval + 1;
      } else {
        // S_0 = 0 (see Dennis, Gay and Welsch)
        w.hf.zero();
      }
      w.use_second_derivatives = true;
      break;
    }
    case 3: // hybrid (MNT)
    {
      // set the tolerance :: make this relative
      w.hybrid_tol =
          options.hybrid_tol * (w.normJF / (0.5 * (pow(w.normF, 2))));
      // use first-order method initially
      w.hf.zero();
      w.use_second_derivatives = false;
      if (!options.exact_second_derivatives) {
        // initialize hf_temp too
        w.hf_temp.zero();
      }
      break;
    }
    default:
      throw std::logic_error("Unsupported model.");
    }
  }

  w.iter = w.iter + 1;
  inform.iter = w.iter;

  bool success = false;
  int no_reductions = 0;
  double normFnew = 0.0;

  while (!success) { // loop until successful
    no_reductions = no_reductions + 1;
    if (no_reductions > max_tr_decrease + 1) {
      return true;
    }
    // Calculate the step d that the model thinks we should take next
    calculateStep(w.J, w.f, w.hf, w.Delta, w.d, w.normd, options);

    // Accept the step?
    w.Xnew = X;
    w.Xnew += w.d;
    evalF(w.Xnew, w.fnew);
    inform.f_eval = inform.f_eval + 1;
    normFnew = NLLS::norm2(w.fnew);

    // Get the value of the model
    //      md :=   m_k(d)
    // evaluated at the new step
    double md =
        evaluateModel(w.f, w.J, w.hf, w.d, options, w.evaluate_model_ws);

    // Calculate the quantity
    //   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction
    //         --------------------------   -------------------
    //             m_k(0)  - m_k(d)         predicted_reduction
    //
    // if model is good, rho should be close to one
    auto rho = calculateRho(w.normF, normFnew, md, options);
    if (!std::isfinite(rho) || rho <= options.eta_successful) {
      if ((w.use_second_derivatives) && (options.model == 3) &&
          (no_reductions == 1)) {
        // recalculate rho based on the approx GN model
        // (i.e. the Gauss-Newton model evaluated at the Quasi-Newton step)
        double rho_gn =
            calculateRho(w.normF, normFnew, w.evaluate_model_ws.md_gn, options);
        if (rho_gn > options.eta_successful) {
          // switch back to gauss-newton
          w.use_second_derivatives = false;
          w.hf_temp.zero(); // discard S_k, as it's been polluted
          w.hf.zero();
        }
      }
    } else {
      success = true;
    }

    // Update the TR radius
    updateTrustRegionRadius(rho, options, w);

    if (!success) {
      // finally, check d makes progress
      if (NLLS::norm2(w.d) <
          std::numeric_limits<double>::epsilon() * NLLS::norm2(w.Xnew)) {
        m_errorString = "Failed to make progress.";
        return false;
      }
    }
  } // loop
  // if we reach here, a successful step has been found

  // update X and f
  X = w.Xnew;
  w.f = w.fnew;

  if (!options.exact_second_derivatives) {
    // first, let's save some old values...
    // g_old = -J_k^T r_k
    w.g_old = w.g;
    // g_mixed = -J_k^T r_{k+1}
    NLLS::multJt(w.J, w.fnew, w.g_mixed);
    w.g_mixed *= -1.0;
  }

  // evaluate J and hf at the new point
  evalJ(X, w.J);
  inform.g_eval = inform.g_eval + 1;

  if (options.calculate_svd_J) {
    NLLS::getSvdJ(w.J, w.smallest_sv(w.iter + 1), w.largest_sv(w.iter + 1));
  }

  // g = -J^Tf
  NLLS::multJt(w.J, w.f, w.g);
  w.g *= -1.0;

  w.normJFold = w.normJF;
  w.normF = normFnew;
  w.normJF = NLLS::norm2(w.g);

  // setup the vectors needed if second derivatives are not available
  if (!options.exact_second_derivatives) {
    w.y = w.g_old;
    w.y -= w.g;
    w.y_sharp = w.g_mixed;
    w.y_sharp -= w.g;
  }

  if (options.model == 3) {
    // hybrid method -- check if we need second derivatives

    if (w.use_second_derivatives) {
      if (w.normJF > w.normJFold) {
        // switch to Gauss-Newton
        w.use_second_derivatives = false;
        // save hf as hf_temp
        w.hf_temp = w.hf;
        w.hf.zero();
      }
    } else {
      auto FunctionValue = 0.5 * (pow(w.normF, 2));
      if (w.normJF / FunctionValue < w.hybrid_tol) {
        w.hybrid_count = w.hybrid_count + 1;
        if (w.hybrid_count == options.hybrid_switch_its) {
          // use (Quasi-)Newton
          w.use_second_derivatives = true;
          w.hybrid_count = 0;
          // copy hf from hf_temp
          if (!options.exact_second_derivatives) {
            w.hf = w.hf_temp;
          }
        }
      } else {
        w.hybrid_count = 0;
      }
    }

    if (!w.use_second_derivatives) {
      // call apply_second_order_info anyway, so that we update the
      // second order approximation
      if (!options.exact_second_derivatives) {
        rankOneUpdate(w.hf_temp, w);
      }
    }
  }

  if (w.use_second_derivatives) {
    // apply_second_order_info(n, m, X, w, evalHF, params, options, inform,
    //                        weights);
    if (options.exact_second_derivatives) {
      evalHF(X, w.f, w.hf);
      inform.h_eval = inform.h_eval + 1;
    } else {
      // use the rank-one approximation...
      rankOneUpdate(w.hf, w);
    }
  }

  // update the stats
  inform.obj = 0.5 * (pow(w.normF, 2));
  inform.norm_g = w.normJF;
  inform.scaled_g = w.normJF / w.normF;
  if (options.output_progress_vectors) {
    w.resvec(w.iter + 1) = inform.obj;
    w.gradvec(w.iter + 1) = inform.norm_g;
  }

  // Test convergence
  testConvergence(w.normF, w.normJF, w.normF0, w.normJF0, options, inform);

  if (inform.convergence_normf == 1 || inform.convergence_normg == 1) {
    return false;
  }

  inform.iter = w.iter;
  inform.resvec = w.resvec;
  inform.gradvec = w.gradvec;

  return true;
}

/** DTRS method **/
namespace {

const double HUGEST = std::numeric_limits<double>::max();
const double EPSILON_MCH = std::numeric_limits<double>::epsilon();
const double LARGEST = HUGEST;
const double LOWER_DEFAULT = -0.5 * LARGEST;
const double UPPER_DEFAULT = LARGEST;
const double POINT4 = 0.4;
const double ZERO = 0.0;
const double ONE = 1.0;
const double TWO = 2.0;
const double THREE = 3.0;
const double FOUR = 4.0;
const double SIX = 6.0;
const double HALF = 0.5;
const double ONESIXTH = ONE / SIX;
const double SIXTH = ONESIXTH;
const double ONE_THIRD = ONE / THREE;
const double TWO_THIRDS = TWO / THREE;
const double THREE_QUARTERS = 0.75;
const double TWENTY_FOUR = 24.0;
const int MAX_DEGREE = 3;
const int HISTORY_MAX = 100;
const double TEN_EPSILON_MCH = 10.0 * EPSILON_MCH;
const double ROOTS_TOL = TEN_EPSILON_MCH;
const double INFINITE_NUMBER = HUGEST;

/** Replacement for FORTRAN's SIGN intrinsic function
 */
inline double sign(double x, double y) { return y >= 0.0 ? fabs(x) : -fabs(x); }

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   control derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct control_type {
  //!  maximum degree of Taylor approximant allowed
  int taylor_max_degree = 3;

  //!  any entry of H that is smaller than h_min * MAXVAL( H ) we be treated as
  // zero
  double h_min = EPSILON_MCH;

  //!  lower and upper bounds on the multiplier, if known
  double lower = LOWER_DEFAULT;
  double upper = UPPER_DEFAULT;

  //!  stop when | ||x|| - radius | <=
  //!     max( stop_normal * radius, stop_absolute_normal )
  double stop_normal = EPSILON_MCH;
  double stop_absolute_normal = EPSILON_MCH;

  //!  is the solution is REQUIRED to lie on the boundary (i.e., is the
  // constraint
  //!  an equality)?
  bool equality_problem = false;
};

//!  - - - - - - - - - - - - - - - - - - - - - - - -
//!   history derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - - -
struct history_type {
  //
  //!  value of lambda
  double lambda = 0.0;

  //!  corresponding value of ||x(lambda)||_M
  double x_norm = 0.0;
};

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   inform derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct inform_type {
  //

  //!  the number of (||x||_M,lambda) pairs in the history
  int len_history = 0;

  //!  the value of the quadratic function
  double obj = HUGEST;

  //!  the M-norm of x, ||x||_M
  double x_norm = 0.0;

  //!  the Lagrange multiplier corresponding to the trust-region constraint
  double multiplier = 0.0;

  //!  a lower bound max(0,-lambda_1), where lambda_1 is the left-most
  //!  eigenvalue of (H,M)
  double pole = 0.0;

  //!  has the hard case occurred?
  bool hard_case = false;

  //!  history information
  std::vector<history_type> history;
};

/** Get the largest of the four values.
 *  @param a :: Value number 1.
 *  @param b :: Value number 2.
 *  @param c :: Value number 3.
 *  @param d :: Value number 4.
 */
double biggest(double a, double b, double c, double d) {
  return std::max(std::max(a, b), std::max(c, d));
}

/** Get the largest of the three values.
 *  @param a :: Value number 1.
 *  @param b :: Value number 2.
 *  @param c :: Value number 3.
 */
double biggest(double a, double b, double c) {
  return std::max(std::max(a, b), c);
}

/** Find the largest by absolute value element of a vector.
 *  @param v :: The searched vector.
 */
double maxAbsVal(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::max(fabs(v.get(p.first)), fabs(v.get(p.second)));
}

/** Find the minimum and maximum elements of a vector.
 *  @param v :: The searched vector.
 *  @returns :: A pair of doubles where the first is the minimum and
 *      the second is the maxumum.
 */
std::pair<double, double> minMaxValues(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::make_pair(v.get(p.first), v.get(p.second));
}

/** Compute the 2-norm of a vector which is a square root of the
 *  sum of squares of its elements.
 *  @param v :: The vector.
 */
double twoNorm(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

/** Get the dot-product of two vectors of the same size.
 *  @param v1 :: The first vector.
 *  @param v2 :: The second vector.
 */
double dotProduct(const DoubleFortranVector &v1,
                  const DoubleFortranVector &v2) {
  return v1.dot(v2);
}

/** Find the maximum element in the first n elements of a vector.
 *  @param v :: The vector.
 *  @param n :: The number of elements to examine.
 */
double maxVal(const DoubleFortranVector &v, int n) {
  double res = std::numeric_limits<double>::lowest();
  for (int i = 1; i <= n; ++i) {
    auto val = v(i);
    if (val > res) {
      res = val;
    }
  }
  return res;
}

/**  Find the number and values of real roots of the quadratic equation
 *
 *                    a2 * x**2 + a1 * x + a0 = 0
 *
 *   where a0, a1 and a2 are real
 *  @param a0 :: The free coefficient.
 *  @param a1 :: The coefficient at the linear term.
 *  @param a2 :: The coefficient at the quadratic term.
 *  @param tol :: A tolerance for comparing doubles.
 *  @param nroots :: The output number of real roots.
 *  @param root1 :: The first real root if nroots > 0.
 *  @param root2 :: The second real root if nroots = 2.
 */
void rootsQuadratic(double a0, double a1, double a2, double tol, int &nroots,
                    double &root1, double &root2) {

  auto rhs = tol * a1 * a1;
  if (fabs(a0 * a2) > rhs) { // really is quadratic
    root2 = a1 * a1 - FOUR * a2 * a0;
    if (fabs(root2) <= pow(EPSILON_MCH * a1, 2)) { // numerical double root
      nroots = 2;
      root1 = -HALF * a1 / a2;
      root2 = root1;
    } else if (root2 < ZERO) { // complex not real roots
      nroots = 0;
      root1 = ZERO;
      root2 = ZERO;
    } else { // distint real roots
      auto d = -HALF * (a1 + sign(sqrt(root2), a1));
      nroots = 2;
      root1 = d / a2;
      root2 = a0 / d;
      if (root1 > root2) {
        d = root1;
        root1 = root2;
        root2 = d;
      }
    }
  } else if (a2 == ZERO) {
    if (a1 == ZERO) {
      if (a0 == ZERO) { // the function is zero
        nroots = 1;
        root1 = ZERO;
        root2 = ZERO;
      } else { // the function is constant
        nroots = 0;
        root1 = ZERO;
        root2 = ZERO;
      }
    } else { // the function is linear
      nroots = 1;
      root1 = -a0 / a1;
      root2 = ZERO;
    }
  } else { // very ill-conditioned quadratic
    nroots = 2;
    if (-a1 / a2 > ZERO) {
      root1 = ZERO;
      root2 = -a1 / a2;
    } else {
      root1 = -a1 / a2;
      root2 = ZERO;
    }
  }

  //  perfom a Newton iteration to ensure that the roots are accurate

  if (nroots >= 1) {
    auto p = (a2 * root1 + a1) * root1 + a0;
    auto pprime = TWO * a2 * root1 + a1;
    if (pprime != ZERO) {
      root1 = root1 - p / pprime;
    }
    if (nroots == 2) {
      p = (a2 * root2 + a1) * root2 + a0;
      pprime = TWO * a2 * root2 + a1;
      if (pprime != ZERO) {
        root2 = root2 - p / pprime;
      }
    }
  }
}

/**  Find the number and values of real roots of the cubic equation
 *
 *                 a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
 *
 *   where a0, a1, a2 and a3 are real
 *  @param a0 :: The free coefficient.
 *  @param a1 :: The coefficient at the linear term.
 *  @param a2 :: The coefficient at the quadratic term.
 *  @param a3 :: The coefficient at the cubic term.
 *  @param tol :: A tolerance for comparing doubles.
 *  @param nroots :: The output number of real roots.
 *  @param root1 :: The first real root.
 *  @param root2 :: The second real root if nroots > 1.
 *  @param root3 :: The third real root if nroots == 3.
 */
void rootsCubic(double a0, double a1, double a2, double a3, double tol,
                int &nroots, double &root1, double &root2, double &root3) {

  //  Check to see if the cubic is actually a quadratic
  if (a3 == ZERO) {
    rootsQuadratic(a0, a1, a2, tol, nroots, root1, root2);
    root3 = INFINITE_NUMBER;
    return;
  }

  //  Deflate the polnomial if the trailing coefficient is zero
  if (a0 == ZERO) {
    root1 = ZERO;
    rootsQuadratic(a1, a2, a3, tol, nroots, root2, root3);
    nroots = nroots + 1;
    return;
  }

  //  1. Use Nonweiler's method (CACM 11:4, 1968, pp269)

  double c0 = a0 / a3;
  double c1 = a1 / a3;
  double c2 = a2 / a3;

  double s = c2 / THREE;
  double t = s * c2;
  double b = 0.5 * (s * (TWO_THIRDS * t - c1) + c0);
  t = (t - c1) / THREE;
  double c = t * t * t;
  double d = b * b - c;

  // 1 real + 2 equal real or 2 complex roots
  if (d >= ZERO) {
    d = pow(sqrt(d) + fabs(b), ONE_THIRD);
    if (d != ZERO) {
      if (b > ZERO) {
        b = -d;
      } else {
        b = d;
      }
      c = t / b;
    }
    d = sqrt(THREE_QUARTERS) * (b - c);
    b = b + c;
    c = -0.5 * b - s;
    root1 = b - s;
    if (d == ZERO) {
      nroots = 3;
      root2 = c;
      root3 = c;
    } else {
      nroots = 1;
    }
  } else { // 3 real roots
    if (b == ZERO) {
      d = TWO_THIRDS * atan(ONE);
    } else {
      d = atan(sqrt(-d) / fabs(b)) / THREE;
    }
    if (b < ZERO) {
      b = TWO * sqrt(t);
    } else {
      b = -TWO * sqrt(t);
    }
    c = cos(d) * b;
    t = -sqrt(THREE_QUARTERS) * sin(d) * b - HALF * c;
    d = -t - c - s;
    c = c - s;
    t = t - s;
    if (fabs(c) > fabs(t)) {
      root3 = c;
    } else {
      root3 = t;
      t = c;
    }
    if (fabs(d) > fabs(t)) {
      root2 = d;
    } else {
      root2 = t;
      t = d;
    }
    root1 = t;
    nroots = 3;
  }

  //  reorder the roots

  if (nroots == 3) {
    if (root1 > root2) {
      double a = root2;
      root2 = root1;
      root1 = a;
    }
    if (root2 > root3) {
      double a = root3;
      if (root1 > root3) {
        a = root1;
        root1 = root3;
      }
      root3 = root2;
      root2 = a;
    }
  }

  //  perfom a Newton iteration to ensure that the roots are accurate
  double p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0;
  double pprime = (THREE * a3 * root1 + TWO * a2) * root1 + a1;
  if (pprime != ZERO) {
    root1 = root1 - p / pprime;
    // p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0; // never used
  }

  if (nroots == 3) {
    p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    pprime = (THREE * a3 * root2 + TWO * a2) * root2 + a1;
    if (pprime != ZERO) {
      root2 = root2 - p / pprime;
      // p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0; // never used
    }

    p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    pprime = (THREE * a3 * root3 + TWO * a2) * root3 + a1;
    if (pprime != ZERO) {
      root3 = root3 - p / pprime;
      // p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0; // never used
    }
  }
}

/**  Compute pi_beta = ||x||^beta and its derivatives
 *   Extracted wholesale from module RAL_NLLS_RQS
 *
 *  @param max_order :: Maximum order of derivative.
 *  @param beta :: Power.
 *  @param x_norm2 :: (0) value of ||x||^2,
 *                    (i) ith derivative of ||x||^2, i = 1, max_order
 *  @param pi_beta :: (0) value of ||x||^beta,
 *                    (i) ith derivative of ||x||^beta, i = 1, max_order
 */
void PiBetaDerivs(int max_order, double beta,
                  const DoubleFortranVector &x_norm2,
                  DoubleFortranVector &pi_beta) {
  double hbeta = HALF * beta;
  pi_beta(0) = pow(x_norm2(0), hbeta);
  pi_beta(1) = hbeta * (pow(x_norm2(0), (hbeta - ONE))) * x_norm2(1);
  if (max_order == 1)
    return;
  pi_beta(2) = hbeta * (pow(x_norm2(0), (hbeta - TWO))) *
               ((hbeta - ONE) * pow(x_norm2(1), 2) + x_norm2(0) * x_norm2(2));
  if (max_order == 2)
    return;
  pi_beta(3) = hbeta * (pow(x_norm2(0), (hbeta - THREE))) *
               (x_norm2(3) * pow(x_norm2(0), 2) +
                (hbeta - ONE) * (THREE * x_norm2(0) * x_norm2(1) * x_norm2(2) +
                                 (hbeta - TWO) * pow(x_norm2(1), 3)));
}

/**  Set initial values for the TRS control parameters
 *
 *  @param control :: A structure containing control information.
 */
void intitializeControl(control_type &control) {
  control.stop_normal = pow(EPSILON_MCH, 0.75);
  control.stop_absolute_normal = pow(EPSILON_MCH, 0.75);
}

/**  Solve the trust-region subproblem
 *
 *       minimize     1/2 <x, H x> + <c, x> + f
 *       subject to    ||x||_2 <= radius  or ||x||_2 = radius
 *
 *   where H is diagonal, using a secular iteration
 *
 *  @param n :: The number of unknowns.
 *  @param radius :: The trust-region radius.
 *  @param f :: The value of constant term for the quadratic function
 *  @param c :: A vector of values for the linear term c.
 *  @param h :: A vector of values for the diagonal matrix H.
 *  @param x :: The required solution vector x.
 *  @param control :: A structure containing control information.
 *  @param inform :: A structure containing information.
 */
void solveSubproblemMain(int n, double radius, double f,
                         const DoubleFortranVector &c,
                         const DoubleFortranVector &h, DoubleFortranVector &x,
                         const control_type &control, inform_type &inform) {

  //  set initial values

  if (x.len() != n) {
    x.allocate(n);
  }
  x.zero();
  inform.x_norm = ZERO;
  inform.obj = f;
  inform.hard_case = false;

  //  Check that arguments are OK
  if (n < 0) {
    throw std::runtime_error(
        "Number of unknowns for trust-region subproblem is negative.");
  }
  if (radius < 0) {
    throw std::runtime_error(
        "Trust-region radius for trust-region subproblem is negative");
  }

  DoubleFortranVector x_norm2(0, MAX_DEGREE), pi_beta(0, MAX_DEGREE);

  //  compute the two-norm of c and the extreme eigenvalues of H

  double c_norm = twoNorm(c);
  double lambda_min = 0.0;
  double lambda_max = 0.0;
  std::tie(lambda_min, lambda_max) = minMaxValues(h);

  double lambda = 0.0;
  //!  check for the trivial case
  if (c_norm == ZERO && lambda_min >= ZERO) {
    if (control.equality_problem) {
      int i_hard = 1;                // TODO: is init value of 1 correct?
      for (int i = 1; i <= n; ++i) { // do i = 1, n
        if (h(i) == lambda_min) {
          i_hard = i;
          break;
        }
      }
      x(i_hard) = ONE / radius;
      inform.x_norm = radius;
      inform.obj = f + lambda_min * radius * radius;
    }
    return;
  }

  //  construct values lambda_l and lambda_u for which lambda_l <=
  //  lambda_optimal
  //   <= lambda_u, and ensure that all iterates satisfy lambda_l <= lambda
  //   <= lambda_u

  double c_norm_over_radius = c_norm / radius;
  double lambda_l = 0.0, lambda_u = 0.0;
  if (control.equality_problem) {
    lambda_l =
        biggest(control.lower, -lambda_min, c_norm_over_radius - lambda_max);
    lambda_u = std::min(control.upper, c_norm_over_radius - lambda_min);
  } else {
    lambda_l = biggest(control.lower, ZERO, -lambda_min,
                       c_norm_over_radius - lambda_max);
    lambda_u = std::min(control.upper,
                        std::max(ZERO, c_norm_over_radius - lambda_min));
  }
  lambda = lambda_l;

  //  check for the "hard case"
  if (lambda == -lambda_min) {
    int i_hard = 1; // TODO: is init value of 1 correct?
    double c2 = ZERO;
    inform.hard_case = true;
    for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
      if (h(i) == lambda_min) {
        if (fabs(c(i)) > EPSILON_MCH * c_norm) {
          inform.hard_case = false;
          c2 = c2 + pow(c(i), 2);
        } else {
          i_hard = i;
        }
      }
    }

    //  the hard case may occur
    if (inform.hard_case) {
      for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
        if (h(i) != lambda_min) {
          x(i) = -c(i) / (h(i) + lambda);
        } else {
          x(i) = ZERO;
        }
      }
      inform.x_norm = twoNorm(x);

      //  the hard case does occur

      if (inform.x_norm <= radius) {
        if (inform.x_norm < radius) {

          //  compute the step alpha so that x + alpha e_i_hard lies on the
          //  trust-region
          //  boundary and gives the smaller value of q

          auto utx = x(i_hard) / radius;
          auto distx =
              (radius - inform.x_norm) * ((radius + inform.x_norm) / radius);
          auto alpha = sign(
              distx / (fabs(utx) + sqrt(pow(utx, 2) + distx / radius)), utx);

          //  record the optimal values

          x(i_hard) = x(i_hard) + alpha;
        }
        inform.x_norm = twoNorm(x);
        inform.obj = f + HALF * (dotProduct(c, x) - lambda * pow(radius, 2));
        return;

        //  the hard case didn't occur after all
      } else {
        inform.hard_case = false;

        //  compute the first derivative of ||x|(lambda)||^2 - radius^2
        auto w_norm2 = ZERO;
        for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
          if (h(i) != lambda_min)
            w_norm2 = w_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 3);
        }
        x_norm2(1) = -TWO * w_norm2;

        //  compute the newton correction

        lambda = lambda + (pow(inform.x_norm, 2) - pow(radius, 2)) / x_norm2(1);
        lambda_l = std::max(lambda_l, lambda);
      }

      //  there is a singularity at lambda. compute the point for which the
      //  sum of squares of the singular terms is equal to radius^2
    } else {
      lambda = lambda + std::max(sqrt(c2) / radius, lambda * EPSILON_MCH);
      lambda_l = std::max(lambda_l, lambda);
    }
  }

  //  the iterates will all be in the L region. Prepare for the main loop
  auto max_order = std::max(1, std::min(MAX_DEGREE, control.taylor_max_degree));

  //  start the main loop
  for (;;) {

    //  if h(lambda) is positive definite, solve  h(lambda) x = - c

    for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
      x(i) = -c(i) / (h(i) + lambda);
    }

    //  compute the two-norm of x
    inform.x_norm = twoNorm(x);
    x_norm2(0) = pow(inform.x_norm, 2);

    //  if the newton step lies within the trust region, exit

    if (lambda == ZERO && inform.x_norm <= radius) {
      inform.obj = f + HALF * dotProduct(c, x);
      return;
    }

    //!  the current estimate gives a good approximation to the required
    //!  root

    if (fabs(inform.x_norm - radius) <=
        std::max(control.stop_normal * radius, control.stop_absolute_normal)) {
      break;
    }

    lambda_l = std::max(lambda_l, lambda);

    //  record, for the future, values of lambda which give small ||x||
    if (inform.len_history < HISTORY_MAX) {
      history_type history_item;
      history_item.lambda = lambda;
      history_item.x_norm = inform.x_norm;
      inform.history.push_back(history_item);
      inform.len_history = inform.len_history + 1;
    }

    //  a lambda in L has been found. It is now simply a matter of applying
    //  a variety of Taylor-series-based methods starting from this lambda

    //  precaution against rounding producing lambda outside L

    if (lambda > lambda_u) {
      throw std::runtime_error(
          "Lambda for trust-region subproblem is ill conditioned");
    }

    //  compute first derivatives of x^T M x

    //  form ||w||^2 = x^T H^-1(lambda) x

    double w_norm2 = ZERO;
    for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
      w_norm2 = w_norm2 + pow(c(i), 2) / pow(h(i) + lambda, 3);
    }

    //  compute the first derivative of x_norm2 = x^T M x
    x_norm2(1) = -TWO * w_norm2;

    //  compute pi_beta = ||x||^beta and its first derivative when beta = - 1
    double beta = -ONE;
    PiBetaDerivs(1, beta, x_norm2, pi_beta);

    //  compute the Newton correction (for beta = - 1)

    auto delta_lambda = -(pi_beta(0) - pow((radius), beta)) / pi_beta(1);

    DoubleFortranVector lambda_new(3);
    int n_lambda = 1;
    lambda_new(n_lambda) = lambda + delta_lambda;

    if (max_order >= 3) {

      //  compute the second derivative of x^T x

      double z_norm2 = ZERO;
      for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
        z_norm2 = z_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 4);
      }
      x_norm2(2) = SIX * z_norm2;

      //  compute the third derivatives of x^T x

      double v_norm2 = ZERO;
      for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
        v_norm2 = v_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 5);
      }
      x_norm2(3) = -TWENTY_FOUR * v_norm2;

      //  compute pi_beta = ||x||^beta and its derivatives when beta = 2

      beta = TWO;
      PiBetaDerivs(max_order, beta, x_norm2, pi_beta);

      //  compute the "cubic Taylor approximaton" step (beta = 2)

      auto a_0 = pi_beta(0) - pow((radius), beta);
      auto a_1 = pi_beta(1);
      auto a_2 = HALF * pi_beta(2);
      auto a_3 = SIXTH * pi_beta(3);
      auto a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > ZERO) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      int nroots = 0;
      double root1 = 0, root2 = 0, root3 = 0;

      rootsCubic(a_0, a_1, a_2, a_3, ROOTS_TOL, nroots, root1, root2, root3);
      n_lambda = n_lambda + 1;
      if (nroots == 3) {
        lambda_new(n_lambda) = lambda + root3;
      } else {
        lambda_new(n_lambda) = lambda + root1;
      }

      //  compute pi_beta = ||x||^beta and its derivatives when beta = - 0.4

      beta = -POINT4;
      PiBetaDerivs(max_order, beta, x_norm2, pi_beta);

      //  compute the "cubic Taylor approximaton" step (beta = - 0.4)

      a_0 = pi_beta(0) - pow((radius), beta);
      a_1 = pi_beta(1);
      a_2 = HALF * pi_beta(2);
      a_3 = SIXTH * pi_beta(3);
      a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > ZERO) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      rootsCubic(a_0, a_1, a_2, a_3, ROOTS_TOL, nroots, root1, root2, root3);
      n_lambda = n_lambda + 1;
      if (nroots == 3) {
        lambda_new(n_lambda) = lambda + root3;
      } else {
        lambda_new(n_lambda) = lambda + root1;
      }
    }

    //  compute the best Taylor improvement

    auto lambda_plus = maxVal(lambda_new, n_lambda);
    delta_lambda = lambda_plus - lambda;
    lambda = lambda_plus;

    //  improve the lower bound if possible

    lambda_l = std::max(lambda_l, lambda_plus);

    //  check that the best Taylor improvement is significant

    if (std::abs(delta_lambda) <
        EPSILON_MCH * std::max(ONE, std::abs(lambda))) {
      break;
    }

  } // for(;;)
}

/**  Solve the trust-region subproblem
 *
 *       minimize     q(x) = 1/2 <x, H x> + <c, x> + f
 *       subject to   ||x||_2 <= radius  or ||x||_2 = radius
 *
 *   where H is diagonal, using a secular iteration
 *
 *  @param n :: The number of unknowns.
 *  @param radius :: The trust-region radius.
 *  @param f :: The value of constant term for the quadratic function.
 *  @param c :: A vector of values for the linear term c.
 *  @param h ::  A vector of values for the diagonal matrix H.
 *  @param x :: The required solution vector x.
 *  @param control :: A structure containing control information.
 *  @param inform :: A structure containing information.
 */
void solveSubproblem(int n, double radius, double f,
                     const DoubleFortranVector &c, const DoubleFortranVector &h,
                     DoubleFortranVector &x, const control_type &control,
                     inform_type &inform) {
  //  scale the problem to solve instead
  //      minimize    q_s(x_s) = 1/2 <x_s, H_s x_s> + <c_s, x_s> + f_s
  //      subject to    ||x_s||_2 <= radius_s  or ||x_s||_2 = radius_s

  //  where H_s = H / s_h and c_s = c / s_c for scale factors s_h and s_c

  //  This corresponds to
  //    radius_s = ( s_h / s_c ) radius,
  //    f_s = ( s_h / s_c^2 ) f
  //  and the solution may be recovered as
  //    x = ( s_c / s_h ) x_s
  //    lambda = s_h lambda_s
  //    q(x) = ( s_c^2 / s_ h ) q_s(x_s)

  //  scale H by the largest H and remove relatively tiny H

  DoubleFortranVector h_scale(n);
  auto scale_h = maxAbsVal(h); // MAXVAL( ABS( H ) )
  if (scale_h > ZERO) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (fabs(h(i)) >= control.h_min * scale_h) {
        h_scale(i) = h(i) / scale_h;
      } else {
        h_scale(i) = ZERO;
      }
    }
  } else {
    scale_h = ONE;
    h_scale.zero();
  }

  //  scale c by the largest c and remove relatively tiny c

  DoubleFortranVector c_scale(n);
  auto scale_c = maxAbsVal(c); // maxval( abs( c ) )
  if (scale_c > ZERO) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (fabs(c(i)) >= control.h_min * scale_c) {
        c_scale(i) = c(i) / scale_c;
      } else {
        c_scale(i) = ZERO;
      }
    }
  } else {
    scale_c = ONE;
    c_scale.zero();
  }

  double radius_scale = (scale_h / scale_c) * radius;
  double f_scale = (scale_h / pow(scale_c, 2)) * f;

  auto control_scale = control;
  if (control_scale.lower != LOWER_DEFAULT) {
    control_scale.lower = control_scale.lower / scale_h;
  }
  if (control_scale.upper != UPPER_DEFAULT) {
    control_scale.upper = control_scale.upper / scale_h;
  }

  //  solve the scaled problem

  solveSubproblemMain(n, radius_scale, f_scale, c_scale, h_scale, x,
                      control_scale, inform);

  //  unscale the solution, function value, multiplier and related values

  //  x = ( scale_c / scale_h ) * x
  x *= scale_c / scale_h;
  inform.obj *= pow(scale_c, 2) / scale_h;
  inform.multiplier *= scale_h;
  inform.pole *= scale_h;
  for (auto &history_item :
       inform.history) { //      do i = 1, inform.len_history
    history_item.lambda *= scale_h;
    history_item.x_norm *= scale_c / scale_h;
  }
}

} // namespace

/**   Solve the trust-region subproblem using
 *    the DTRS method from Galahad
 *
 *    This method needs H to be diagonal, so we need to
 *    pre-process
 *
 *    main output  d, the soln to the TR subproblem
 *  @param J :: The Jacobian.
 *  @param f :: The residuals.
 *  @param hf :: The Hessian (sort of).
 *  @param Delta :: The raduis of the trust region.
 *  @param d :: The output vector of corrections to the parameters giving the
 *        solution to the TR subproblem.
 *  @param normd :: The 2-norm of d.
 *  @param options :: The options.
 */
void TrustRegionMinimizer::calculateStep(const DoubleFortranMatrix &J,
                                         const DoubleFortranVector &f,
                                         const DoubleFortranMatrix &hf,
                                         double Delta, DoubleFortranVector &d,
                                         double &normd,
                                         const NLLS::nlls_options &options) {

  control_type controlOptions;
  inform_type inform;

  //  The code finds
  //   d = arg min_p   w^T p + 0.5 * p^T D p
  //        s.t. ||p|| \leq Delta
  //
  //  where D is diagonal
  //
  //  our probem in naturally in the form
  //
  //  d = arg min_p   v^T p + 0.5 * p^T H p
  //        s.t. ||p|| \leq Delta
  //
  //  first, find the matrix H and vector v
  //  Set A = J^T J
  NLLS::matmultInner(J, m_A);
  // add any second order information...
  // so A = J^T J + HF
  m_A += hf;

  // now form v = J^T f
  NLLS::multJt(J, f, m_v);

  // if scaling needed, do it
  if (options.scale != 0) {
    applyScaling(J, m_A, m_v, m_scale, options);
  }

  // Now that we have the unprocessed matrices, we need to get an
  // eigendecomposition to make A diagonal
  //
  NLLS::allEigSymm(m_A, m_ew, m_ev);

  // We can now change variables, setting y = Vp, getting
  // Vd = arg min_(Vx) v^T p + 0.5 * (Vp)^T D (Vp)
  //       s.t.  ||x|| \leq Delta
  // <=>
  // Vd = arg min_(Vx) V^Tv^T (Vp) + 0.5 * (Vp)^T D (Vp)
  //       s.t.  ||x|| \leq Delta
  // <=>
  // we need to get the transformed vector v
  NLLS::multJt(m_ev, m_v, m_v_trans);

  // we've now got the vectors we need, pass to solveSubproblem
  intitializeControl(controlOptions);

  auto n = J.len2();
  if (m_v_trans.len() != n) {
    m_v_trans.allocate(n);
  }

  for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1,n)
    if (fabs(m_v_trans(ii)) < EPSILON_MCH) {
      m_v_trans(ii) = ZERO;
    }
    if (fabs(m_ew(ii)) < EPSILON_MCH) {
      m_ew(ii) = ZERO;
    }
  }

  solveSubproblem(n, Delta, ZERO, m_v_trans, m_ew, m_d_trans, controlOptions,
                  inform);

  // and return the un-transformed vector
  NLLS::multJ(m_ev, m_d_trans, d);

  normd = NLLS::norm2(d); // ! ||d||_D

  if (options.scale != 0) {
    for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1, n)
      d(ii) = d(ii) / m_scale(ii);
    }
  }

} // calculateStep

/** Return the current value of the cost function.
 */
double TrustRegionMinimizer::costFunctionVal() { return m_leastSquares->val(); }

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
