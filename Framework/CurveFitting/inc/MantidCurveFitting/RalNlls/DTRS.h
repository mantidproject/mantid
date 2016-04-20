#ifndef CURVEFITTING_RAL_NLLS_DTRST_H_
#define CURVEFITTING_RAL_NLLS_DTRST_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FortranDefs.h"

#include <limits>

namespace Mantid {
namespace CurveFitting {

namespace RalNlls {

const double HUGE = std::numeric_limits<double>::max();
const double epsmch = std::numeric_limits<double>::epsilon();
const double largest = HUGE;
const double lower_default = - 0.5 * largest;
const double upper_default = largest;

enum class ErrorCode {
      ral_nlls_ok                      = 0,
      ral_nlls_error_allocate          = - 1,
      ral_nlls_error_deallocate        = - 2,
      ral_nlls_error_restrictions      = - 3,
      ral_nlls_error_bad_bounds        = - 4,
      ral_nlls_error_primal_infeasible = - 5,
      ral_nlls_error_dual_infeasible   = - 6,
      ral_nlls_error_unbounded         = - 7,
      ral_nlls_error_no_center         = - 8,
      ral_nlls_error_analysis          = - 9,
      ral_nlls_error_factorization     = - 10,
      ral_nlls_error_solve             = - 11,
      ral_nlls_error_uls_analysis      = - 12,
      ral_nlls_error_uls_factorization = - 13,
      ral_nlls_error_uls_solve         = - 14,
      ral_nlls_error_preconditioner    = - 15,
      ral_nlls_error_ill_conditioned   = - 16,
      ral_nlls_error_tiny_step         = - 17,
      ral_nlls_error_max_iterations    = - 18,
      ral_nlls_error_time_limit        = - 19,
      ral_nlls_error_cpu_limit         =   ral_nlls_error_time_limit,
      ral_nlls_error_inertia           = - 20,
      ral_nlls_error_file              = - 21,
      ral_nlls_error_io                = - 22,
      ral_nlls_error_upper_entry       = - 23,
      ral_nlls_error_sort              = - 24,
      ral_nlls_error_input_status      = - 25,
      ral_nlls_error_unknown_solver    = - 26,
      ral_nlls_not_yet_implemented     = - 27,
      ral_nlls_error_qp_solve          = - 28,
      ral_nlls_unavailable_option      = - 29,
      ral_nlls_warning_on_boundary     = - 30,
      ral_nlls_error_call_order        = - 31,
      ral_nlls_error_integer_ws        = - 32,
      ral_nlls_error_real_ws           = - 33,
      ral_nlls_error_pardiso           = - 34,
      ral_nlls_error_wsmp              = - 35,
      ral_nlls_error_mc64              = - 36,
      ral_nlls_error_mc77              = - 37,
      ral_nlls_error_lapack            = - 38,
      ral_nlls_error_permutation       = - 39,
      ral_nlls_error_alter_diagonal    = - 40,
      ral_nlls_error_access_pivots     = - 41,
      ral_nlls_error_access_pert       = - 42,
      ral_nlls_error_direct_access     = - 43,
      ral_nlls_error_f_min             = - 44,
      ral_nlls_error_unknown_precond   = - 45,
      ral_nlls_error_schur_complement  = - 46,
      ral_nlls_error_technical         = - 50,
      ral_nlls_error_reformat          = - 52,
      ral_nlls_error_ah_unordered      = - 53,
      ral_nlls_error_y_unallocated     = - 54,
      ral_nlls_error_z_unallocated     = - 55,
      ral_nlls_error_scale             = - 61,
      ral_nlls_error_presolve          = - 62,
      ral_nlls_error_qpa               = - 63,
      ral_nlls_error_qpb               = - 64,
      ral_nlls_error_qpc               = - 65,
      ral_nlls_error_cqp               = - 66,
      ral_nlls_error_dqp               = - 67,
      ral_nlls_error_mc61              = - 69,
      ral_nlls_error_mc68              = - 70,
      ral_nlls_error_metis             = - 71,
      ral_nlls_error_spral             = - 72,
      ral_nlls_warning_repeated_entry  = - 73,
      ral_nlls_error_rif               = - 74,
      ral_nlls_error_ls28              = - 75,
      ral_nlls_error_ls29              = - 76,
      ral_nlls_error_cutest            = - 77,
      ral_nlls_error_evaluation        = - 78,
      ral_nlls_error_optional          = - 79,
      ral_nlls_error_mi35              = - 80,
      ral_nlls_error_spqr              = - 81,
      ral_nlls_error_alive             = - 82,
      ral_nlls_error_ccqp              = - 83
};

/// Replacement for FORTRAN's SIGN intrinsic function
inline double sign(double x, double y) {
  return y >= 0.0 ? std::abs(x) : -std::abs(x);
};

/// Find roots of a quadratic equation.
MANTID_CURVEFITTING_DLL void roots_quadratic(double a0, double a1, double a2, double tol, int &nroots, 
                     double& root1, double &root2, bool debug = false );

/// Find roots of a cubic equation.
MANTID_CURVEFITTING_DLL void roots_cubic(double a0, double a1, double a2, double a3, double tol, int &nroots, 
                     double& root1, double &root2, double &root3, bool debug = false );

/// Find roots of a quartic equation.
MANTID_CURVEFITTING_DLL void roots_quartic(double a0, double a1, double a2, double a3, double a4, double tol, int &nroots, 
                     double& root1, double &root2, double &root3, double &root4, bool debug = false);


//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   control derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct  dtrs_control_type {
//!  controls level of diagnostic output
int  print_level = 0;

//!  maximum degree of Taylor approximant allowed
int  taylor_max_degree = 3;

//!  any entry of H that is smaller than h_min * MAXVAL( H ) we be treated as zero
double   h_min = epsmch;

//!  any entry of C that is smaller than c_min * MAXVAL( C ) we be treated as zero
double   c_min = epsmch;

//!  lower and upper bounds on the multiplier, if known
double   lower = lower_default;
double   upper = upper_default;

//!  stop when | ||x|| - radius | <=
//!     max( stop_normal * radius, stop_absolute_normal )
double   stop_normal = epsmch;
double   stop_absolute_normal = epsmch;

//!  is the solution is REQUIRED to lie on the boundary (i.e., is the constraint
//!  an equality)?
bool equality_problem= false;
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
ErrorCode  status = ErrorCode::ral_nlls_ok;

//!  the number of (||x||_M,lambda) pairs in the history
int  len_history = 0;

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

MANTID_CURVEFITTING_DLL
void dtrs_initialize(dtrs_control_type &control, dtrs_inform_type &inform);

MANTID_CURVEFITTING_DLL void
dtrs_solve_main(int n, double radius, double f, const DoubleFortranVector &c,
                const DoubleFortranVector &h, DoubleFortranVector &x,
                const dtrs_control_type &control, dtrs_inform_type &inform);

MANTID_CURVEFITTING_DLL void dtrs_pi_derivs(int max_order, double beta,
                                            const DoubleFortranVector &x_norm2,
                                            DoubleFortranVector &pi_beta);

MANTID_CURVEFITTING_DLL 
void dtrs_solve(int n, double radius, double f, const DoubleFortranVector &c,
                const DoubleFortranVector &h, DoubleFortranVector &x,
                const dtrs_control_type &control, dtrs_inform_type &inform);

}
}
}

#endif // CURVEFITTING_RAL_NLLS_DTRS_H_
