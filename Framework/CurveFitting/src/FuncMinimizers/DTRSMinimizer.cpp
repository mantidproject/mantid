// This code was originally translated from Fortran code on
// https://ccpforge.cse.rl.ac.uk/gf/project/ral_nlls June 2016
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/DTRSMinimizer.h"
#include "MantidCurveFitting/RalNlls/TrustRegion.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include <algorithm>
#include <limits>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

// clang-format off
///@cond nodoc
DECLARE_FUNCMINIMIZER(DTRSMinimizer, Trust Region)
///@endcond
// clang-format on

using namespace NLLS;

DTRSMinimizer::DTRSMinimizer() : TrustRegionMinimizer() {}

/// Name of the minimizer.
std::string DTRSMinimizer::name() const { return "Trust Region"; }

namespace {

const double huge = std::numeric_limits<double>::max();
const double epsmch = std::numeric_limits<double>::epsilon();
const double largest = huge;
const double lower_default = -0.5 * largest;
const double upper_default = largest;
const double point4 = 0.4;
const double two = 2.0;
const double three = 3.0;
const double four = 4.0;
const double six = 6.0;
const double onesixth = one / six;
const double sixth = onesixth;
const double onethird = one / three;
const double twothirds = two / three;
const double threequarters = 0.75;
const double twentyfour = 24.0;
const int max_degree = 3;
const int history_max = 100;
const double teneps = 10.0 * epsmch;
const double roots_tol = teneps;
const double infinity = huge;

enum class ErrorCode {
  ral_nlls_ok = 0,
  ral_nlls_error_allocate = -1,
  ral_nlls_error_deallocate = -2,
  ral_nlls_error_restrictions = -3,
  ral_nlls_error_bad_bounds = -4,
  ral_nlls_error_primal_infeasible = -5,
  ral_nlls_error_dual_infeasible = -6,
  ral_nlls_error_unbounded = -7,
  ral_nlls_error_no_center = -8,
  ral_nlls_error_analysis = -9,
  ral_nlls_error_factorization = -10,
  ral_nlls_error_solve = -11,
  ral_nlls_error_uls_analysis = -12,
  ral_nlls_error_uls_factorization = -13,
  ral_nlls_error_uls_solve = -14,
  ral_nlls_error_preconditioner = -15,
  ral_nlls_error_ill_conditioned = -16,
  ral_nlls_error_tiny_step = -17,
  ral_nlls_error_max_iterations = -18,
  ral_nlls_error_time_limit = -19,
  ral_nlls_error_cpu_limit = ral_nlls_error_time_limit,
  ral_nlls_error_inertia = -20,
  ral_nlls_error_file = -21,
  ral_nlls_error_io = -22,
  ral_nlls_error_upper_entry = -23,
  ral_nlls_error_sort = -24,
  ral_nlls_error_input_status = -25,
  ral_nlls_error_unknown_solver = -26,
  ral_nlls_not_yet_implemented = -27,
  ral_nlls_error_qp_solve = -28,
  ral_nlls_unavailable_option = -29,
  ral_nlls_warning_on_boundary = -30,
  ral_nlls_error_call_order = -31,
  ral_nlls_error_integer_ws = -32,
  ral_nlls_error_real_ws = -33,
  ral_nlls_error_pardiso = -34,
  ral_nlls_error_wsmp = -35,
  ral_nlls_error_mc64 = -36,
  ral_nlls_error_mc77 = -37,
  ral_nlls_error_lapack = -38,
  ral_nlls_error_permutation = -39,
  ral_nlls_error_alter_diagonal = -40,
  ral_nlls_error_access_pivots = -41,
  ral_nlls_error_access_pert = -42,
  ral_nlls_error_direct_access = -43,
  ral_nlls_error_f_min = -44,
  ral_nlls_error_unknown_precond = -45,
  ral_nlls_error_schur_complement = -46,
  ral_nlls_error_technical = -50,
  ral_nlls_error_reformat = -52,
  ral_nlls_error_ah_unordered = -53,
  ral_nlls_error_y_unallocated = -54,
  ral_nlls_error_z_unallocated = -55,
  ral_nlls_error_scale = -61,
  ral_nlls_error_presolve = -62,
  ral_nlls_error_qpa = -63,
  ral_nlls_error_qpb = -64,
  ral_nlls_error_qpc = -65,
  ral_nlls_error_cqp = -66,
  ral_nlls_error_dqp = -67,
  ral_nlls_error_mc61 = -69,
  ral_nlls_error_mc68 = -70,
  ral_nlls_error_metis = -71,
  ral_nlls_error_spral = -72,
  ral_nlls_warning_repeated_entry = -73,
  ral_nlls_error_rif = -74,
  ral_nlls_error_ls28 = -75,
  ral_nlls_error_ls29 = -76,
  ral_nlls_error_cutest = -77,
  ral_nlls_error_evaluation = -78,
  ral_nlls_error_optional = -79,
  ral_nlls_error_mi35 = -80,
  ral_nlls_error_spqr = -81,
  ral_nlls_error_alive = -82,
  ral_nlls_error_ccqp = -83
};

/// Replacement for FORTRAN's SIGN intrinsic function
inline double sign(double x, double y) { return y >= 0.0 ? fabs(x) : -fabs(x); }

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   control derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct dtrs_control_type {
  //!  maximum degree of Taylor approximant allowed
  int taylor_max_degree = 3;

  //!  any entry of H that is smaller than h_min * MAXVAL( H ) we be treated as
  // zero
  double h_min = epsmch;

  //!  lower and upper bounds on the multiplier, if known
  double lower = lower_default;
  double upper = upper_default;

  //!  stop when | ||x|| - radius | <=
  //!     max( stop_normal * radius, stop_absolute_normal )
  double stop_normal = epsmch;
  double stop_absolute_normal = epsmch;

  //!  is the solution is REQUIRED to lie on the boundary (i.e., is the
  // constraint
  //!  an equality)?
  bool equality_problem = false;
};

//!  - - - - - - - - - - - - - - - - - - - - - - - -
//!   history derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - - -
struct dtrs_history_type {
  //
  //!  value of lambda
  double lambda = 0.0;

  //!  corresponding value of ||x(lambda)||_M
  double x_norm = 0.0;
};

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   inform derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct dtrs_inform_type {
  //
  //!   reported return status:
  //!      0 the solution has been found
  //!     -3 n and/or Delta is not positive
  //!    -16 ill-conditioning has prevented furthr progress
  ErrorCode status = ErrorCode::ral_nlls_ok;

  //!  the number of (||x||_M,lambda) pairs in the history
  int len_history = 0;

  //!  the value of the quadratic function
  double obj = HUGE;

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
  std::vector<dtrs_history_type> history;
};

/// Get the largest of the four values.
/// @param a :: Value number 1.
/// @param b :: Value number 2.
/// @param c :: Value number 3.
/// @param d :: Value number 4.
double biggest(double a, double b, double c, double d) {
  return std::max(std::max(a, b), std::max(c, d));
}

/// Get the largest of the three values.
/// @param a :: Value number 1.
/// @param b :: Value number 2.
/// @param c :: Value number 3.
double biggest(double a, double b, double c) {
  return std::max(std::max(a, b), c);
}

/// Find the largest by absolute value element of a vector.
/// @param v :: The searched vector.
double maxAbsVal(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::max(fabs(v.get(p.first)), fabs(v.get(p.second)));
}

/// Find the minimum and maximum elements of a vector.
/// @param v :: The searched vector.
/// @returns :: A pair of doubles where the first is the minimum and
///     the second is the maxumum.
std::pair<double, double> minMaxValues(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::make_pair(v.get(p.first), v.get(p.second));
}

/// Compute the 2-norm of a vector which is a square root of the
/// sum of squares of its elements.
/// @param v :: The vector.
double twoNorm(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

/// Get the dot-product of two vectors of the same size.
/// @param v1 :: The first vector.
/// @param v2 :: The second vector.
double dotProduct(const DoubleFortranVector &v1,
                  const DoubleFortranVector &v2) {
  return v1.dot(v2);
}

/// Find the maximum element in the first n elements of a vector.
/// @param v :: The vector.
/// @param n :: The number of elements to examine.
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

///  Find the number and values of real roots of the quadratic equation
///
///                   a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1 and a2 are real
/// @param a0 :: The free coefficient.
/// @param a1 :: The coefficient at the linear term.
/// @param a2 :: The coefficient at the quadratic term.
/// @param tol :: A tolerance for comparing doubles.
/// @param nroots :: The output number of real roots.
/// @param root1 :: The first real root if nroots > 0.
/// @param root2 :: The second real root if nroots = 2.
void rootsQuadratic(double a0, double a1, double a2, double tol, int &nroots,
                    double &root1, double &root2) {

  auto rhs = tol * a1 * a1;
  if (fabs(a0 * a2) > rhs) { // really is quadratic
    root2 = a1 * a1 - four * a2 * a0;
    if (fabs(root2) <= pow(epsmch * a1, 2)) { // numerical double root
      nroots = 2;
      root1 = -half * a1 / a2;
      root2 = root1;
    } else if (root2 < zero) { // complex not real roots
      nroots = 0;
      root1 = zero;
      root2 = zero;
    } else { // distint real roots
      auto d = -half * (a1 + sign(sqrt(root2), a1));
      nroots = 2;
      root1 = d / a2;
      root2 = a0 / d;
      if (root1 > root2) {
        d = root1;
        root1 = root2;
        root2 = d;
      }
    }
  } else if (a2 == zero) {
    if (a1 == zero) {
      if (a0 == zero) { // the function is zero
        nroots = 1;
        root1 = zero;
        root2 = zero;
      } else { // the function is constant
        nroots = 0;
        root1 = zero;
        root2 = zero;
      }
    } else { // the function is linear
      nroots = 1;
      root1 = -a0 / a1;
      root2 = zero;
    }
  } else { // very ill-conditioned quadratic
    nroots = 2;
    if (-a1 / a2 > zero) {
      root1 = zero;
      root2 = -a1 / a2;
    } else {
      root1 = -a1 / a2;
      root2 = zero;
    }
  }

  //  perfom a Newton iteration to ensure that the roots are accurate

  if (nroots >= 1) {
    auto p = (a2 * root1 + a1) * root1 + a0;
    auto pprime = two * a2 * root1 + a1;
    if (pprime != zero) {
      root1 = root1 - p / pprime;
    }
    if (nroots == 2) {
      p = (a2 * root2 + a1) * root2 + a0;
      pprime = two * a2 * root2 + a1;
      if (pprime != zero) {
        root2 = root2 - p / pprime;
      }
    }
  }
}

///  Find the number and values of real roots of the cubic equation
///
///                a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1, a2 and a3 are real
/// @param a0 :: The free coefficient.
/// @param a1 :: The coefficient at the linear term.
/// @param a2 :: The coefficient at the quadratic term.
/// @param a3 :: The coefficient at the cubic term.
/// @param tol :: A tolerance for comparing doubles.
/// @param nroots :: The output number of real roots.
/// @param root1 :: The first real root.
/// @param root2 :: The second real root if nroots > 1.
/// @param root3 :: The third real root if nroots == 3.
void rootsCubic(double a0, double a1, double a2, double a3, double tol,
                int &nroots, double &root1, double &root2, double &root3) {

  //  Check to see if the cubic is actually a quadratic
  if (a3 == zero) {
    rootsQuadratic(a0, a1, a2, tol, nroots, root1, root2);
    root3 = infinity;
    return;
  }

  //  Deflate the polnomial if the trailing coefficient is zero
  if (a0 == zero) {
    root1 = zero;
    rootsQuadratic(a1, a2, a3, tol, nroots, root2, root3);
    nroots = nroots + 1;
    return;
  }

  //  1. Use Nonweiler's method (CACM 11:4, 1968, pp269)

  double c0 = a0 / a3;
  double c1 = a1 / a3;
  double c2 = a2 / a3;

  double s = c2 / three;
  double t = s * c2;
  double b = 0.5 * (s * (twothirds * t - c1) + c0);
  t = (t - c1) / three;
  double c = t * t * t;
  double d = b * b - c;

  // 1 real + 2 equal real or 2 complex roots
  if (d >= zero) {
    d = pow(sqrt(d) + fabs(b), onethird);
    if (d != zero) {
      if (b > zero) {
        b = -d;
      } else {
        b = d;
      }
      c = t / b;
    }
    d = sqrt(threequarters) * (b - c);
    b = b + c;
    c = -0.5 * b - s;
    root1 = b - s;
    if (d == zero) {
      nroots = 3;
      root2 = c;
      root3 = c;
    } else {
      nroots = 1;
    }
  } else { // 3 real roots
    if (b == zero) {
      d = twothirds * atan(one);
    } else {
      d = atan(sqrt(-d) / fabs(b)) / three;
    }
    if (b < zero) {
      b = two * sqrt(t);
    } else {
      b = -two * sqrt(t);
    }
    c = cos(d) * b;
    t = -sqrt(threequarters) * sin(d) * b - half * c;
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
  double pprime = (three * a3 * root1 + two * a2) * root1 + a1;
  if (pprime != zero) {
    root1 = root1 - p / pprime;
    // p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0; // never used
  }

  if (nroots == 3) {
    p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    pprime = (three * a3 * root2 + two * a2) * root2 + a1;
    if (pprime != zero) {
      root2 = root2 - p / pprime;
      // p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0; // never used
    }

    p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    pprime = (three * a3 * root3 + two * a2) * root3 + a1;
    if (pprime != zero) {
      root3 = root3 - p / pprime;
      // p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0; // never used
    }
  }
}

///  Compute pi_beta = ||x||^beta and its derivatives
///  Extracted wholesale from module RAL_NLLS_RQS
///
/// @param max_order :: Maximum order of derivative.
/// @param beta :: Power.
/// @param x_norm2 :: (0) value of ||x||^2,
///                   (i) ith derivative of ||x||^2, i = 1, max_order
/// @param pi_beta :: (0) value of ||x||^beta,
///                   (i) ith derivative of ||x||^beta, i = 1, max_order
void dtrsPiDerivs(int max_order, double beta,
                  const DoubleFortranVector &x_norm2,
                  DoubleFortranVector &pi_beta) {
  double hbeta = half * beta;
  pi_beta(0) = pow(x_norm2(0), hbeta);
  pi_beta(1) = hbeta * (pow(x_norm2(0), (hbeta - one))) * x_norm2(1);
  if (max_order == 1)
    return;
  pi_beta(2) = hbeta * (pow(x_norm2(0), (hbeta - two))) *
               ((hbeta - one) * pow(x_norm2(1), 2) + x_norm2(0) * x_norm2(2));
  if (max_order == 2)
    return;
  pi_beta(3) = hbeta * (pow(x_norm2(0), (hbeta - three))) *
               (x_norm2(3) * pow(x_norm2(0), 2) +
                (hbeta - one) * (three * x_norm2(0) * x_norm2(1) * x_norm2(2) +
                                 (hbeta - two) * pow(x_norm2(1), 3)));
}

///  Set initial values for the TRS control parameters
///
/// @param control :: A structure containing control information.
/// @param inform  :: A structure containing information.
void dtrsInitialize(dtrs_control_type &control, dtrs_inform_type &inform) {
  inform.status = ErrorCode::ral_nlls_ok;
  control.stop_normal = pow(epsmch, 0.75);
  control.stop_absolute_normal = pow(epsmch, 0.75);
}

///  Solve the trust-region subproblem
///
///      minimize     1/2 <x, H x> + <c, x> + f
///      subject to    ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// @param n :: The number of unknowns.
/// @param radius :: The trust-region radius.
/// @param f :: The value of constant term for the quadratic function
/// @param c :: A vector of values for the linear term c.
/// @param h :: A vector of values for the diagonal matrix H.
/// @param x :: The required solution vector x.
/// @param control :: A structure containing control information.
/// @param inform :: A structure containing information.
///
void dtrsSolveMain(int n, double radius, double f, const DoubleFortranVector &c,
                   const DoubleFortranVector &h, DoubleFortranVector &x,
                   const dtrs_control_type &control, dtrs_inform_type &inform) {

  //  set initial values

  if (x.len() != n) {
    x.allocate(n);
  }
  x.zero();
  inform.x_norm = zero;
  inform.obj = f;
  inform.hard_case = false;
  double delta_lambda = zero;

  //  check for n < 0 or delta < 0
  if (n < 0 || radius < 0) {
    inform.status = ErrorCode::ral_nlls_error_restrictions;
    return;
  }

  DoubleFortranVector x_norm2(0, max_degree), pi_beta(0, max_degree);

  //  compute the two-norm of c and the extreme eigenvalues of H

  double c_norm = twoNorm(c);
  double lambda_min = 0.0;
  double lambda_max = 0.0;
  std::tie(lambda_min, lambda_max) = minMaxValues(h);

  double lambda = 0.0;
  //!  check for the trivial case
  if (c_norm == zero && lambda_min >= zero) {
    if (control.equality_problem) {
      int i_hard = 1;                // TODO: is init value of 1 correct?
      for (int i = 1; i <= n; ++i) { // do i = 1, n
        if (h(i) == lambda_min) {
          i_hard = i;
          break;
        }
      }
      x(i_hard) = one / radius;
      inform.x_norm = radius;
      inform.obj = f + lambda_min * radius * radius;
    }
    inform.status = ErrorCode::ral_nlls_ok;
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
    lambda_l = biggest(control.lower, zero, -lambda_min,
                       c_norm_over_radius - lambda_max);
    lambda_u = std::min(control.upper,
                        std::max(zero, c_norm_over_radius - lambda_min));
  }
  lambda = lambda_l;

  //  check for the "hard case"
  if (lambda == -lambda_min) {
    int i_hard = 1; // TODO: is init value of 1 correct?
    double c2 = zero;
    inform.hard_case = true;
    for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
      if (h(i) == lambda_min) {
        if (fabs(c(i)) > epsmch * c_norm) {
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
          x(i) = zero;
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
        inform.obj = f + half * (dotProduct(c, x) - lambda * pow(radius, 2));
        inform.status = ErrorCode::ral_nlls_ok;
        return;

        //  the hard case didn't occur after all
      } else {
        inform.hard_case = false;

        //  compute the first derivative of ||x|(lambda)||^2 - radius^2
        auto w_norm2 = zero;
        for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
          if (h(i) != lambda_min)
            w_norm2 = w_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 3);
        }
        x_norm2(1) = -two * w_norm2;

        //  compute the newton correction

        lambda = lambda + (pow(inform.x_norm, 2) - pow(radius, 2)) / x_norm2(1);
        lambda_l = std::max(lambda_l, lambda);
      }

      //  there is a singularity at lambda. compute the point for which the
      //  sum of squares of the singular terms is equal to radius^2
    } else {
      lambda = lambda + std::max(sqrt(c2) / radius, lambda * epsmch);
      lambda_l = std::max(lambda_l, lambda);
    }
  }

  //  the iterates will all be in the L region. Prepare for the main loop
  auto max_order = std::max(1, std::min(max_degree, control.taylor_max_degree));

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

    if (lambda == zero && inform.x_norm <= radius) {
      inform.obj = f + half * dotProduct(c, x);
      inform.status = ErrorCode::ral_nlls_ok;
      return;
    }

    //!  the current estimate gives a good approximation to the required
    //!  root

    if (fabs(inform.x_norm - radius) <=
        std::max(control.stop_normal * radius, control.stop_absolute_normal)) {
      inform.status = ErrorCode::ral_nlls_ok;
      break;
    }

    lambda_l = std::max(lambda_l, lambda);

    //  record, for the future, values of lambda which give small ||x||
    if (inform.len_history < history_max) {
      dtrs_history_type history_item;
      history_item.lambda = lambda;
      history_item.x_norm = inform.x_norm;
      inform.history.push_back(history_item);
      inform.len_history = inform.len_history + 1;
    }

    //  a lambda in L has been found. It is now simply a matter of applying
    //  a variety of Taylor-series-based methods starting from this lambda

    //  precaution against rounding producing lambda outside L

    if (lambda > lambda_u) {
      inform.status = ErrorCode::ral_nlls_error_ill_conditioned;
      break;
    }

    //  compute first derivatives of x^T M x

    //  form ||w||^2 = x^T H^-1(lambda) x

    double w_norm2 = zero;
    for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
      w_norm2 = w_norm2 + pow(c(i), 2) / pow(h(i) + lambda, 3);
    }

    //  compute the first derivative of x_norm2 = x^T M x
    x_norm2(1) = -two * w_norm2;

    //  compute pi_beta = ||x||^beta and its first derivative when beta = - 1
    double beta = -one;
    dtrsPiDerivs(1, beta, x_norm2, pi_beta);

    //  compute the Newton correction (for beta = - 1)

    delta_lambda = -(pi_beta(0) - pow((radius), beta)) / pi_beta(1);

    DoubleFortranVector lambda_new(3);
    int n_lambda = 1;
    lambda_new(n_lambda) = lambda + delta_lambda;

    if (max_order >= 3) {

      //  compute the second derivative of x^T x

      double z_norm2 = zero;
      for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
        z_norm2 = z_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 4);
      }
      x_norm2(2) = six * z_norm2;

      //  compute the third derivatives of x^T x

      double v_norm2 = zero;
      for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
        v_norm2 = v_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 5);
      }
      x_norm2(3) = -twentyfour * v_norm2;

      //  compute pi_beta = ||x||^beta and its derivatives when beta = 2

      beta = two;
      dtrsPiDerivs(max_order, beta, x_norm2, pi_beta);

      //  compute the "cubic Taylor approximaton" step (beta = 2)

      auto a_0 = pi_beta(0) - pow((radius), beta);
      auto a_1 = pi_beta(1);
      auto a_2 = half * pi_beta(2);
      auto a_3 = sixth * pi_beta(3);
      auto a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > zero) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      int nroots = 0;
      double root1 = 0, root2 = 0, root3 = 0;

      rootsCubic(a_0, a_1, a_2, a_3, roots_tol, nroots, root1, root2, root3);
      n_lambda = n_lambda + 1;
      if (nroots == 3) {
        lambda_new(n_lambda) = lambda + root3;
      } else {
        lambda_new(n_lambda) = lambda + root1;
      }

      //  compute pi_beta = ||x||^beta and its derivatives when beta = - 0.4

      beta = -point4;
      dtrsPiDerivs(max_order, beta, x_norm2, pi_beta);

      //  compute the "cubic Taylor approximaton" step (beta = - 0.4)

      a_0 = pi_beta(0) - pow((radius), beta);
      a_1 = pi_beta(1);
      a_2 = half * pi_beta(2);
      a_3 = sixth * pi_beta(3);
      a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > zero) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      rootsCubic(a_0, a_1, a_2, a_3, roots_tol, nroots, root1, root2, root3);
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

    if (fabs(delta_lambda) < epsmch * std::max(one, fabs(lambda))) {
      inform.status = ErrorCode::ral_nlls_ok;
      break;
    }

  } // for(;;)
}

///  Solve the trust-region subproblem
///
///      minimize     q(x) = 1/2 <x, H x> + <c, x> + f
///      subject to   ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// @param n :: The number of unknowns.
/// @param radius :: The trust-region radius.
/// @param f :: The value of constant term for the quadratic function.
/// @param c :: A vector of values for the linear term c.
/// @param h ::  A vector of values for the diagonal matrix H.
/// @param x :: The required solution vector x.
/// @param control :: A structure containing control information.
/// @param inform :: A structure containing information.
///
void dtrsSolve(int n, double radius, double f, const DoubleFortranVector &c,
               const DoubleFortranVector &h, DoubleFortranVector &x,
               const dtrs_control_type &control, dtrs_inform_type &inform) {
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
  if (scale_h > zero) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (fabs(h(i)) >= control.h_min * scale_h) {
        h_scale(i) = h(i) / scale_h;
      } else {
        h_scale(i) = zero;
      }
    }
  } else {
    scale_h = one;
    h_scale.zero();
  }

  //  scale c by the largest c and remove relatively tiny c

  DoubleFortranVector c_scale(n);
  auto scale_c = maxAbsVal(c); // maxval( abs( c ) )
  if (scale_c > zero) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (fabs(c(i)) >= control.h_min * scale_c) {
        c_scale(i) = c(i) / scale_c;
      } else {
        c_scale(i) = zero;
      }
    }
  } else {
    scale_c = one;
    c_scale.zero();
  }

  double radius_scale = (scale_h / scale_c) * radius;
  double f_scale = (scale_h / pow(scale_c, 2)) * f;

  auto control_scale = control;
  if (control_scale.lower != lower_default) {
    control_scale.lower = control_scale.lower / scale_h;
  }
  if (control_scale.upper != upper_default) {
    control_scale.upper = control_scale.upper / scale_h;
  }

  //  solve the scaled problem

  dtrsSolveMain(n, radius_scale, f_scale, c_scale, h_scale, x, control_scale,
                inform);

  //  unscale the solution, function value, multiplier and related values

  //  x = ( scale_c / scale_h ) * x
  x *= scale_c / scale_h;
  inform.obj *= pow(scale_c, 2) / scale_h;
  inform.multiplier *= scale_h;
  inform.pole *= scale_h;
  for (size_t i = 0; i < inform.history.size();
       ++i) { //      do i = 1, inform.len_history
    inform.history[i].lambda *= scale_h;
    inform.history[i].x_norm *= scale_c / scale_h;
  }
}

///   Solve the trust-region subproblem using
///   the DTRS method from Galahad
///
///   This method needs H to be diagonal, so we need to
///   pre-process
///
///   main output  d, the soln to the TR subproblem
/// @param J :: The Jacobian.
/// @param f :: The residuals.
/// @param hf :: The Hessian (sort of).
/// @param Delta :: The raduis of the trust region.
/// @param d :: The output vector of corrections to the parameters giving the
///       solution to the TR subproblem.
/// @param normd :: The 2-norm of d.
/// @param options :: The options.
/// @param inform :: The inform struct.
/// @param w :: The work struct.
void solveDtrs(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
               const DoubleFortranMatrix &hf, double Delta,
               DoubleFortranVector &d, double &normd,
               const nlls_options &options, nlls_inform &inform,
               solve_dtrs_work &w) {

  dtrs_control_type dtrs_options;
  dtrs_inform_type dtrs_inform;

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
  matmultInner(J, w.A);
  // add any second order information...
  // so A = J^T J + HF
  w.A += hf;

  // now form v = J^T f
  multJt(J, f, w.v);

  // if scaling needed, do it
  if (options.scale != 0) {
    applyScaling(J, w.A, w.v, w.apply_scaling_ws, options, inform);
  }

  // Now that we have the unprocessed matrices, we need to get an
  // eigendecomposition to make A diagonal
  //
  allEigSymm(w.A, w.ew, w.ev);
  if (inform.status != NLLS_ERROR::OK) {
    return;
  }

  // We can now change variables, setting y = Vp, getting
  // Vd = arg min_(Vx) v^T p + 0.5 * (Vp)^T D (Vp)
  //       s.t.  ||x|| \leq Delta
  // <=>
  // Vd = arg min_(Vx) V^Tv^T (Vp) + 0.5 * (Vp)^T D (Vp)
  //       s.t.  ||x|| \leq Delta
  // <=>
  // we need to get the transformed vector v
  multJt(w.ev, w.v, w.v_trans);

  // we've now got the vectors we need, pass to dtrsSolve
  dtrsInitialize(dtrs_options, dtrs_inform);

  auto n = J.len2();
  if (w.v_trans.len() != n) {
    w.v_trans.allocate(n);
  }

  for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1,n)
    if (fabs(w.v_trans(ii)) < epsmch) {
      w.v_trans(ii) = zero;
    }
    if (fabs(w.ew(ii)) < epsmch) {
      w.ew(ii) = zero;
    }
  }

  dtrsSolve(n, Delta, zero, w.v_trans, w.ew, w.d_trans, dtrs_options,
            dtrs_inform);
  if (dtrs_inform.status != ErrorCode::ral_nlls_ok) {
    inform.external_return = int(dtrs_inform.status);
    inform.external_name = "galahad_dtrs";
    inform.status = NLLS_ERROR::FROM_EXTERNAL;
    return;
  }

  // and return the un-transformed vector
  multJ(w.ev, w.d_trans, d);

  normd = norm2(d); // ! ||d||_D

  if (options.scale != 0) {
    for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1, n)
      d(ii) = d(ii) / w.apply_scaling_ws.diag(ii);
    }
  }

} // solveDtrs

} // namespace

/// Implements the abstarct method of TrustRegionMinimizer.
void DTRSMinimizer::calculateStep(
    const DoubleFortranMatrix &J, const DoubleFortranVector &f,
    const DoubleFortranMatrix &hf, const DoubleFortranVector &, double Delta,
    DoubleFortranVector &d, double &normd, const NLLS::nlls_options &options,
    NLLS::nlls_inform &inform, NLLS::calculate_step_work &w) {
  solveDtrs(J, f, hf, Delta, d, normd, options, inform, w.solve_dtrs_ws);
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
