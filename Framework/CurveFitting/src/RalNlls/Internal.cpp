#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/DTRS.h"

#include <algorithm>
#include <limits>
#include <functional>
#include <string>

#include <gsl/gsl_blas.h>

#define for_do(i, a, n) for(int i=a; i <=n; ++i) {
#define then {
#define end_if }
#define end_do }

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

typedef double real;
typedef double REAL;
typedef int integer;
typedef int INTEGER;
typedef bool logical;
typedef bool LOGICAL;

namespace {

const double tenm3 = 1.0e-3;
const double tenm5 = 1.0e-5;
const double tenm8 = 1.0e-8;
const double hundred = 100.0;
const double ten = 10.0;
const double point9 = 0.9;
const double zero = 0.0;
const double one = 1.0;
const double two = 2.0;
const double half = 0.5;
const double sixteenth = 0.0625;

}

enum class NLLS_ERROR {
  OK = 0,
  MAXITS = -1,
  EVALUATION = -2,
  UNSUPPORTED_MODEL = -3,
  FROM_EXTERNAL = -4,
  UNSUPPORTED_METHOD = -5,
  ALLOCATION = -6,
  MAX_TR_REDUCTIONS = -7,
  X_NO_PROGRESS = -8,
  N_GT_M = -9,
  BAD_TR_STRATEGY = -10,
  FIND_BETA = -11,
  BAD_SCALING = -12,
  //     ! dogleg errors
  DOGLEG_MODEL = -101,
  //     ! AINT errors
  AINT_EIG_IMAG = -201,
  AINT_EIG_ODD = -202,
  //     ! More-Sorensen errors
  MS_MAXITS = -301,
  MS_TOO_MANY_SHIFTS = -302,
  MS_NO_PROGRESS = -303
  //     ! DTRS errors
};

struct nlls_options {

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! M A I N   R O U T I N E   C O N T R O L S !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //!   the maximum number of iterations performed
  int maxit = 100;

  //!   specify the model used. Possible values are
  //!
  //!      0  dynamic (*not yet implemented*)
  //!      1  Gauss-Newton (no 2nd derivatives)
  //!      2  second-order (exact Hessian)
  //!      3  hybrid (using Madsen, Nielsen and Tingleff's method)    
  int model = 3;

  //!   specify the method used to solve the trust-region sub problem
  //!      1 Powell's dogleg
  //!      2 AINT method (of Yuji Nat.)
  //!      3 More-Sorensen
  //!      4 Galahad's DTRS
  int nlls_method = 4;

  //!  which linear least squares solver should we use?
  int lls_solver = 1;

  //!   overall convergence tolerances. The iteration will terminate when the
  //!     norm of the gradient of the objective function is smaller than 
  //!       MAX( .stop_g_absolute, .stop_g_relative * norm of the initial gradient
  //!     or if the step is less than .stop_s
  double stop_g_absolute = tenm5;
  double stop_g_relative = tenm8;

  //!   should we scale the initial trust region radius?
  int relative_tr_radius = 0;

  //!   if relative_tr_radius == 1, then pick a scaling parameter
  //!   Madsen, Nielsen and Tingleff say pick this to be 1e-6, say, if x_0 is good,
  //!   otherwise 1e-3 or even 1 would be good starts...
  double initial_radius_scale = 1.0;

  //!   if relative_tr_radius /= 1, then set the 
  //!   initial value for the trust-region radius (-ve => ||g_0||)
  double initial_radius = hundred;

  //!   maximum permitted trust-region radius
  double maximum_radius = 1.0e8; //ten ** 8

  //!   a potential iterate will only be accepted if the actual decrease
  //!    f - f(x_new) is larger than .eta_successful times that predicted
  //!    by a quadratic model of the decrease. The trust-region radius will be
  //!    increased if this relative decrease is greater than .eta_very_successful
  //!    but smaller than .eta_too_successful
  double eta_successful = 1.0e-8; // ten ** ( - 8 )
  double eta_success_but_reduce = 1.0e-8; // ten ** ( - 8 )
  double eta_very_successful = point9;
  double eta_too_successful = two;

  //!   on very successful iterations, the trust-region radius will be increased by
  //!    the factor .radius_increase, while if the iteration is unsuccessful, the 
  //!    radius will be decreased by a factor .radius_reduce but no more than
  //!    .radius_reduce_max
  double radius_increase = two;
  double radius_reduce = half;
  double radius_reduce_max = sixteenth;

  //! Trust region update strategy
  //!    1 - usual step function
  //!    2 - continuous method of Hans Bruun Nielsen (IMM-REP-1999-05)
  int tr_update_strategy = 1;

  //!   if model=7, then the value with which we switch on second derivatives
  double hybrid_switch = 0.1;

  //!   shall we use explicit second derivatives, or approximate using a secant 
  //!   method
  bool exact_second_derivatives = false;

  //!   use a factorization (dsyev) to find the smallest eigenvalue for the subproblem
  //!    solve? (alternative is an iterative method (dsyevx)
  bool subproblem_eig_fact = false; //! undocumented....

  //!   scale the variables?
  //!   0 - no scaling
  //!   1 - use the scaling in GSL (W s.t. W_ii = ||J(i,:)||_2^2)
  //!       tiny values get set to one       
  //!   2 - scale using the approx to the Hessian (W s.t. W = ||H(i,:)||_2^2
  int scale = 1;
  double scale_max = 1e11;
  double scale_min = 1e-11;
  bool scale_trim_min = true;
  bool scale_trim_max = true;
  bool scale_require_increase = false;
  bool calculate_svd_J = true;

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! M O R E - S O R E N S E N   C O N T R O L S !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!     
  int more_sorensen_maxits = 500;
  real more_sorensen_shift = 1e-13;
  real  more_sorensen_tiny = 10.0 * epsmch;
  real  more_sorensen_tol = 1e-3;

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! H Y B R I D   C O N T R O L S !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //! what's the tolerance such that ||J^T f || < tol * 0.5 ||f||^2 triggers a switch
  real  hybrid_tol = 2.0;

  //! how many successive iterations does the above condition need to hold before we switch?
  integer   hybrid_switch_its = 1;

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! O U T P U T   C O N T R O L S !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //! Shall we output progess vectors at termination of the routine?
  logical  output_progress_vectors = false;

}; // struct nlls_options

//!  - - - - - - - - - - - - - - - - - - - - - - - 
//!   inform derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - - 
struct  nlls_inform {

  //!  return status
  //!  (see ERROR type for descriptions)
  NLLS_ERROR  status = NLLS_ERROR::OK;

  //! error message     
  //     CHARACTER ( LEN = 80 )  error_message = REPEAT( ' ', 80 )
  std::string error_message;

  //!  the status of the last attempted allocation/deallocation
  INTEGER  alloc_status = 0;

  //!  the name of the array for which an allocation/deallocation error ocurred
  //     CHARACTER ( LEN = 80 )  bad_alloc = REPEAT( ' ', 80 )
  std::string bad_alloc;

  //!  the total number of iterations performed
  INTEGER  iter;

  //!  the total number of CG iterations performed
  //!$$     INTEGER  cg_iter = 0

  //!  the total number of evaluations of the objective function
  INTEGER  f_eval = 0;

  //!  the total number of evaluations of the gradient of the objective function
  INTEGER  g_eval = 0;

  //!  the total number of evaluations of the Hessian of the objective function
  INTEGER  h_eval = 0;

  //!  test on the size of f satisfied?
  integer  convergence_normf = 0;

  //!  test on the size of the gradient satisfied?
  integer  convergence_normg = 0;

  //!  vector of residuals 
  //     real, allocatable  resvec(:)
  DoubleFortranVector resvec;

  //!  vector of gradients 
  //     real, allocatable  gradvec(:)
  DoubleFortranVector gradvec;

  //!  vector of smallest singular values
  //     real, allocatable  smallest_sv(:)
  DoubleFortranVector smallest_sv;

  //!  vector of largest singular values
  //     real, allocatable  largest_sv(:)
  DoubleFortranVector largest_sv;

  //!  the value of the objective function at the best estimate of the solution 
  //!   determined by NLLS_solve
  REAL obj = HUGE;

  //!  the norm of the gradient of the objective function at the best estimate 
  //!   of the solution determined by NLLS_solve
  REAL norm_g = HUGE;

  //! the norm of the gradient, scaled by the norm of the residual
  REAL scaled_g = HUGE;

  //! error returns from external subroutines 
  INTEGER  external_return = 0;

  //! name of external program that threw and error
  //     CHARACTER ( LEN = 80 )  external_name = REPEAT( ' ', 80 )
  std::string external_name;

}; //  END TYPE nlls_inform
 
// deliberately empty
typedef void* params_base_type;
  
//  abstract interface
//     subroutine eval_f_type(status, n, m, x, f, params)
//       import  params_base_type
//       implicit none
//       integer, intent(out)  status
//       integer, intent(in)  n,m 
//       double precision, dimension(*), intent(in)   x
//       double precision, dimension(*), intent(out)  f
//       class(params_base_type), intent(in)  params
//     end subroutine eval_f_type
//  end interface
typedef std::function<void(int &status, int n, int m,
                           const DoubleFortranVector &x, DoubleFortranVector &f,
                           params_base_type params)> eval_f_type;

//  abstract interface
//     subroutine eval_j_type(status, n, m, x, J, params)
//       import  params_base_type
//       implicit none
//       integer, intent(out)  status
//       integer, intent(in)  n,m 
//       double precision, dimension(*), intent(in)   x
//       double precision, dimension(*), intent(out)  J
//       class(params_base_type), intent(in)  params
//     end subroutine eval_j_type
//  end interface
typedef std::function<void(int &status, int n, int m,
                           const DoubleFortranVector &x, DoubleFortranVector &J,
                           params_base_type params)> eval_j_type;

//  abstract interface
//     subroutine eval_hf_type(status, n, m, x, f, h, params)
//       import  params_base_type
//       implicit none
//       integer, intent(out)  status
//       integer, intent(in)  n,m 
//       double precision, dimension(*), intent(in)   x
//       double precision, dimension(*), intent(in)   f
//       double precision, dimension(*), intent(out)  h
//       class(params_base_type), intent(in)  params
//     end subroutine eval_hf_type
//  end interface
typedef std::function<void(int &status, int n, int m,
                           const DoubleFortranVector &x,
                           const DoubleFortranVector &f, DoubleFortranVector &h,
                           params_base_type params)> eval_hf_type;

//  ! define types for workspace arrays.

//! workspace for subroutine max_eig
struct max_eig_work {
  DoubleFortranVector alphaR, alphaI, beta;
  DoubleFortranMatrix vr;
  DoubleFortranVector work, ew_array;
  IntFortranVector nullindex;
  IntFortranVector vecisreal;  // logical, allocatable  vecisreal(:)
  integer nullevs_cols;
  DoubleFortranMatrix nullevs;
};

// ! workspace for subroutine solve_general
struct solve_general_work {
  DoubleFortranMatrix A;
  IntFortranVector ipiv;
};

//! workspace for subroutine evaluate_model
struct evaluate_model_work {
  DoubleFortranVector Jd, Hd;
};

//! workspace for subroutine solve_LLS
struct solve_LLS_work {
  DoubleFortranVector temp, work;
  DoubleFortranMatrix Jlls;
};

//! workspace for subroutine min_eig_work
struct min_eig_symm_work {
  DoubleFortranMatrix A;
  DoubleFortranVector work, ew;
  IntFortranVector iwork, ifail;
};

//! workspace for subroutine all_eig_symm
struct all_eig_symm_work {
  DoubleFortranVector work;
};

//! workspace for subrouine apply_scaling
struct apply_scaling_work {
  DoubleFortranVector diag;
  DoubleFortranMatrix ev;
  DoubleFortranVector tempvec;
  all_eig_symm_work all_eig_symm_ws;
};

//! workspace for subroutine dtrs_work
struct solve_dtrs_work {
  DoubleFortranMatrix A, ev;
  DoubleFortranVector ew, v, v_trans, d_trans;
  all_eig_symm_work all_eig_symm_ws;
  apply_scaling_work apply_scaling_ws;
};

//! workspace for subroutine more_sorensen
struct more_sorensen_work {
  //!      type( solve_spd_work )  solve_spd_ws
  DoubleFortranMatrix A, LtL, AplusSigma;
  DoubleFortranVector v, q, y1;
  //!       type( solve_general_work )  solve_general_ws
  min_eig_symm_work min_eig_symm_ws;
  apply_scaling_work apply_scaling_ws;
};

//! workspace for subroutine AINT_tr
struct AINT_tr_work {
  max_eig_work max_eig_ws;
  evaluate_model_work evaluate_model_ws;
  solve_general_work solve_general_ws;
  //!       type( solve_spd_work )  solve_spd_ws
  DoubleFortranMatrix A, LtL, B, M0, M1, gtg, M0_small, M1_small, y_hardcase;
  DoubleFortranVector v, p0, p1, y, q;
};

//! workspace for subroutine dogleg
struct dogleg_work {
  solve_LLS_work solve_LLS_ws;
  evaluate_model_work evaluate_model_ws;
  DoubleFortranVector d_sd, d_gn, ghat, Jg;
};

//! workspace for subroutine calculate_step
struct calculate_step_work {
  AINT_tr_work AINT_tr_ws;
  dogleg_work dogleg_ws;
  more_sorensen_work more_sorensen_ws;
  solve_dtrs_work solve_dtrs_ws;
};

//! workspace for subroutine get_svd_J
struct get_svd_J_work {
  DoubleFortranVector Jcopy, S, work;       
};

//! all workspaces called from the top level
struct NLLS_workspace {
  integer  first_call = 1;
  integer  iter = 0;
  real  normF0, normJF0, normF, normJF;
  real  normJFold, normJF_Newton;
  real  Delta;
  real  normd;
  logical  use_second_derivatives = false;
  integer  hybrid_count = 0;
  real  hybrid_tol = 1.0;
  DoubleFortranMatrix fNewton, JNewton, XNewton;
  DoubleFortranVector J;
  DoubleFortranVector f, fnew;
  DoubleFortranVector hf, hf_temp;
  DoubleFortranVector d, g, Xnew;
  DoubleFortranVector y, y_sharp, g_old, g_mixed;
  DoubleFortranVector ysharpSks, Sks;
  DoubleFortranVector resvec, gradvec;
  DoubleFortranVector largest_sv, smallest_sv;
  calculate_step_work calculate_step_ws;
  evaluate_model_work evaluate_model_ws;
  get_svd_J_work get_svd_J_ws;
  real  tr_nu = 2.0;
  integer  tr_p = 3;
};

//! -------------------------------------------------------
//! calculate_step, find the next step in the optimization
//! -------------------------------------------------------
void calculate_step(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf, const DoubleFortranVector& g,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform, calculate_step_work& w) {

     switch(options.nlls_method) {
     case 1: //! Powell's dogleg
        dogleg(J,f,hf,g,n,m,Delta,d,normd,options,inform,w.dogleg_ws);
        break;
     case 2: //! The AINT method
        aint_tr(J,f,hf,n,m,Delta,d,normd,options,inform,w.AINT_tr_ws);
        break;
     case 3: //! More-Sorensen
        more_sorensen(J,f,hf,n,m,Delta,d,normd,options,inform,w.more_sorensen_ws);
        break;
     case 4: //! Galahad
        solve_dtrs(J,f,hf,n,m,Delta,d,normd,options,inform,w.solve_dtrs_ws);
        break;
     default:
        inform.status = NLLS_ERROR::UNSUPPORTED_METHOD;
     }

} //   END SUBROUTINE calculate_step
   
///     !-------------------------------
///     ! apply_scaling
///     ! input  Jacobian matrix, J
///     ! ouput  scaled Hessisan, H, and J^Tf, v.
///     !
///     ! Calculates a diagonal scaling W, stored in w.diag
///     ! updates v(i) -> (1/W_i) * v(i)
///     !         A(i,j) -> (1 / (W_i * W_j)) * A(i,j)
///     !-------------------------------
void apply_scaling(const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A, 
  DoubleFortranVector& v, apply_scaling_work& w, const nlls_options options, nlls_inform inform) {

     switch (options.scale) {
     case 1:
     case 2:
        for(int ii=1; ii<=n; ++ii) {//do ii = 1,n
           double temp = zero;
           double Jij = 0.0;
           if (options.scale == 1) then
              //! use the scaling present in gsl:
              //! scale by W, W_ii = ||J(i,:)||_2^2
              for_do(jj, 1,m)
                 get_element_of_matrix(J,m,jj,ii,Jij);
                 temp = temp + pow(Jij, 2);
              end_do
           }else if ( options.scale == 2) then 
              //! scale using the (approximate) hessian
              for_do(jj, 1,n)
                 temp = temp + pow(A(ii,jj), 2);
              end_do
           end_if
           if (temp < options.scale_min) then 
              if (options.scale_trim_min) then 
                 temp = options.scale_min;
              }else{
                 temp = one;
              end_if
           }else if (temp > options.scale_max) then
              if (options.scale_trim_max) then 
                 temp = options.scale_max;
              }else{
                 temp = one;
              end_if
           end_if
           temp = sqrt(temp);
           if (options.scale_require_increase) then
              w.diag(ii) = std::max(temp,w.diag(ii));
           }else{
              w.diag(ii) = temp;
           end_if
        }
     default:
        inform.status = NLLS_ERROR::BAD_SCALING;
        return;
     }
          
//     ! now we have the w.diagonal scaling matrix, actually scale the 
//     ! Hessian approximation and J^Tf
     for_do(ii, 1,n)
       double temp = w.diag(ii);
       v(ii) = v(ii) / temp;
       for_do(jj,1,n)
         A(ii,jj) = A(ii,jj) / temp;
         A(jj,ii) = A(jj,ii) / temp;
       end_do
     end_do
   
} //   end subroutine apply_scaling

/// Copy a column from a matrix.
DoubleFortranVector getColumn(const DoubleFortranMatrix& A, int col) {
  int n = static_cast<int>(A.size1());
  DoubleFortranVector column(n);
  for_do(i,1,n)
    column(i) = A(i, col);
  end_do
  return column;
}

/// Negate a vector
DoubleFortranVector negative(const DoubleFortranVector& v) {
  DoubleFortranVector neg = v;
  neg *= -1.0;
  return neg;
}

void mult_J(const DoubleFortranMatrix& J, int n, int m, const DoubleFortranVector& x, DoubleFortranVector& Jx) {
       //dgemv('N',m,n,alpha,J,m,x,1,beta,Jx,1);
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jx.gsl());

}

void mult_Jt(const DoubleFortranMatrix& J, int n, int m, const DoubleFortranVector& x, DoubleFortranVector& Jtx) {
//       call dgemv('T',m,n,alpha,J,m,x,1,beta,Jtx,1)
  gsl_blas_dgemv(CblasTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jtx.gsl());

}

//       ! return the (ii,jj)th entry of a matrix 
//       ! J held by columns....
void get_element_of_matrix(const DoubleFortranMatrix& J, int m, int ii, int jj, double& Jij) {
//       Jij = J(ii + (jj-1)*m)
  Jij = J(ii, jj);
}

double norm2(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

///     Takes an m x n matrix J and forms the 
///     n x n matrix A given by
///     A = J' * J
void matmult_inner(const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A) {
  A.allocate(n, n);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());
} //     end subroutine matmult_inner

///     Takes an m x n matrix J and forms the 
///     m x m matrix A given by
///     A = J * J'
void matmult_outer( const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A) {
    gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());

} //     end subroutine matmult_outer

DoubleFortranVector matmul(const DoubleFortranMatrix& J, const DoubleFortranVector& x) {
  DoubleFortranVector y(int(x.size()));
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, y.gsl());
  return y;
}

DoubleFortranMatrix matmul(const DoubleFortranMatrix& A, const DoubleFortranMatrix& B) {
  int n = static_cast<int>(A.size1());
  int m = static_cast<int>(B.size2());
  DoubleFortranMatrix C(n, m);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A.gsl(), B.gsl(), 0.0, C.gsl());
  return C;
}

double dot_product(const DoubleFortranVector& x, const DoubleFortranVector& y) {
  return x.dot(y);
}

///  Takes an n vector x and forms the 
///  n x n matrix xtx given by
///  xtx = x * x'
void outer_product(const DoubleFortranVector& x, int n, DoubleFortranMatrix& xtx) {
  xtx.allocate(n, n);
  gsl_blas_dger(1.0, x.gsl(), x.gsl(), xtx.gsl());
}

///   Calculates norm_A_x = ||x||_A = sqrt(x'*A*x)
void matrix_norm(const DoubleFortranVector& x, const DoubleFortranMatrix& A, double &norm_A_x) {
       norm_A_x = sqrt(dot_product(x,matmul(A,x)));
}

//! -----------------------------------------
//! dogleg, implement Powell's dogleg method
//! -----------------------------------------
void dogleg(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf, const DoubleFortranVector& g, 
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform, dogleg_work& w) {

  //     !     Jg = J * g
     mult_J(J,n,m,g,w.Jg);

     double alpha = pow(norm2(g), 2) / pow(norm2( w.Jg ), 2);

     w.d_sd = g;
     w.d_sd *= alpha;

//     ! Solve the linear problem...
     switch(options.model) {
     case 1:
        //! linear model...
        solve_LLS(J,f,n,m,w.d_gn,inform,w.solve_LLS_ws);
        if ( inform.status != NLLS_ERROR::OK ) return; // ? goto 1000
     default:
        inform.status = NLLS_ERROR::DOGLEG_MODEL;
        return;
     }
     
     if (norm2(w.d_gn) <= Delta) then
        d = w.d_gn;
     }else if (alpha * norm2( w.d_sd ) >= Delta) then
        d = w.d_sd;
        d *= (Delta / norm2(w.d_sd) );
     }else{
        w.d_sd *= alpha;
        // w.ghat = w.d_gn - w.d_sd; =>
        w.ghat = w.d_gn;
        w.ghat -= w.d_sd;
        double beta;
        findbeta(w.d_sd,w.ghat,Delta,beta,inform);
        if ( inform.status != NLLS_ERROR::OK ) return; // goto 1000
        // d = w.d_sd + beta * w.ghat; =>
        d = w.ghat;
        d *= beta;
        d += w.d_sd;
     end_if

     normd = norm2(d);
     
//     return
//1000 continue 
//     ! bad error return from solve_LLS
//     return
} //   END SUBROUTINE dogleg

///     ! -----------------------------------------
///     ! AINT_tr
///     ! Solve the trust-region subproblem using 
///     ! the method of ADACHI, IWATA, NAKATSUKASA and TAKEDA
///     ! -----------------------------------------
void aint_tr(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options options, nlls_inform& inform, AINT_tr_work& w) {

   double keep_p0 = 0;
   double tau = 1e-4;
   double obj_p0 = HUGE;

//     ! The code finds 
//     !  min_p   v^T p + 0.5 * p^T A p
//     !       s.t. ||p||_B \leq Delta
//     !
//     ! set A and v for the model being considered here...

     matmult_inner(J,n,m,w.A);

//     ! add any second order information...
     //w.A = w.A + reshape(hf,(/n,n/))
     w.A += hf;
     mult_Jt(J,n,m,f,w.v);

//     ! Set B to I by hand  
//     ! todo: make this an option
     w.B.zero();
     for_do(i, 1,n)
        w.B(i,i) = 1.0;
     end_do
     
     auto nv = w.v;
     nv *= -1.0;
     switch(options.model) {
     case 1:
       solve_spd(w.A,nv,w.LtL,w.p0,n,inform);
       if (inform.status != NLLS_ERROR::OK) return; //goto 1000
     default:
       auto nv = w.v;
       nv *= -1.0;
       solve_general(w.A,nv,w.p0,n,inform,w.solve_general_ws);
       if (inform.status != NLLS_ERROR::OK) return; //goto 1000
     }
          
     double norm_p0 = 0.0;
     matrix_norm(w.p0,w.B,norm_p0);
     
     if (norm_p0 < Delta) then
        keep_p0 = 1;
        //! get obj_p0 : the value of the model at p0
        evaluate_model(f,J,hf,w.p0,obj_p0,m,n,options,w.evaluate_model_ws);
     end_if

     outer_product(w.v,n,w.gtg); // ! gtg = Jtg * Jtg^T
     double factor = (-1.0 / pow(Delta, 2));
     w.M0.allocate(2*n, 2*n);
     for_do(i,1,n)
       for_do(j,1,n)
         w.M0(i,j) = -w.B(i,j);                 // w.M0(1:n,1:n) = -w.B
         w.M0(n+i, j) = w.A(i, j);              // w.M0(n+1:2*n,1:n) = w.A
         w.M0(i, n+j) = w.A(i, j);              // w.M0(1:n,n+1:2*n) = w.A
         w.M0(n+i, n+j) = factor * w.gtg(i, j); // w.M0(n+1:2*n,n+1:2*n) = (-1.0 / Delta**2) * w.gtg
       end_do
     end_do

     w.M1.allocate(2*n, 2*n);
     w.M1.zero();
     for_do(i,1,n)
       for_do(j,1,n)
         double tmp = -w.B(i, j);
         w.M1(n+i, j) = tmp;
         w.M1(i, n+j) = tmp;
       end_do
     end_do

     double lam = 0.0;
     max_eig(w.M0,w.M1,2*n,lam, w.y, w.y_hardcase, options, inform, w.max_eig_ws);
     if ( inform.status != NLLS_ERROR::OK ) return; // goto 1000

     if (norm2(w.y) < tau) then
        //! Hard case
        //! overwrite H onto M0, and the outer prod onto M1...
        matmult_outer( matmul(w.B,w.y_hardcase), w.y_hardcase.size2(), n, w.M1_small);
        // w.M0_small = w.A(:,:) + lam*w.B(:,:) + w.M1_small;
        {
          DoubleFortranMatrix tmp = w.B;
          tmp *= lam;
          tmp += w.A;
          w.M0_small += tmp;
        }
        //! solve Hq + g = 0 for q
        DoubleFortranVector nv = w.v;
        nv *= -1;
        switch(options.model) {
        case 1:
           solve_spd(w.M0_small,nv,w.LtL,w.q,n,inform);
           if (inform.status != NLLS_ERROR::OK) return; // goto 1000
        default:
          solve_general(w.M0_small,nv,w.q,n,inform,w.solve_general_ws);
          if (inform.status != NLLS_ERROR::OK) return; // goto 1000
        }
        //! note -- a copy of the matrix is taken on entry to the solve routines
        //! (I think..) and inside...fix

        //! find max eta st ||q + eta v(:,1)||_B = Delta
        DoubleFortranVector y_hardcase = getColumn(w.y_hardcase,1);
        double eta = 0.0;
        findbeta(w.q, y_hardcase, Delta, eta, inform);
        if (inform.status != NLLS_ERROR::OK) return; // goto 1000

        //!!!!!      ^^TODO^^    !!!!!
        //! currently assumes B = I !!
        //!!!!       fixme!!      !!!!
        
        //w.p1(:) = w.q(:) + eta * w.y_hardcase(:,1)
        w.p1 = y_hardcase;
        w.p1 *= eta;
        w.p1 += w.q;
        
     }else{
        DoubleFortranMatrix AlamB = w.B;
        AlamB *= lam;
        AlamB += w.A;
        DoubleFortranVector negv = w.v;
        negv *= -1.0;
        switch(options.model) {
        case 1:
           solve_spd(AlamB,negv,w.LtL,w.p1,n,inform);
           if (inform.status != NLLS_ERROR::OK) return; // goto 1000
        default:
           solve_general(AlamB,negv,w.p1,n,inform,w.solve_general_ws);
           if (inform.status != NLLS_ERROR::OK) return; // goto 1000
        }
        //! note -- a copy of the matrix is taken on entry to the solve routines
        //! and inside...fix
     end_if
     
//     ! get obj_p1 : the value of the model at p1
     double obj_p1 = 0.0;
     evaluate_model(f,J,hf,w.p1,obj_p1,m,n,options,w.evaluate_model_ws);
//
//     ! what gives the smallest objective: p0 or p1?
     if (obj_p0 < obj_p1) then
        d = w.p0;
     }else{
        d = w.p1;
     end_if

     normd = norm2(d);
//1000 continue 
//     ! bad error return from external package
//     return
} //   END SUBROUTINE AINT_tr

///! -----------------------------------------
///! more_sorensen
///! Solve the trust-region subproblem using 
///! the method of More and Sorensen
///!
///! Using the implementation as in Algorithm 7.3.6
///! of Trust Region Methods
///! 
///! main output  d, the soln to the TR subproblem
///! -----------------------------------------
void more_sorensen( const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& nd, const nlls_options& options, nlls_inform& inform,
  more_sorensen_work& w) {

//     ! The code finds 
//     !  d = arg min_p   v^T p + 0.5 * p^T A p
//     !       s.t. ||p|| \leq Delta
//     !
//     ! set A and v for the model being considered here...

//     ! Set A = J^T J
     matmult_inner(J,n,m,w.A);
//     ! add any second order information...
//     ! so A = J^T J + HF
//     w.A = w.A + reshape(hf,(/n,n/))
     w.A += hf;
//     ! now form v = J^T f 
     mult_Jt(J,n,m,f,w.v);

//     ! if scaling needed, do it
     if ( options.scale != 0) then
        apply_scaling(J,n,m,w.A,w.v,w.apply_scaling_ws,options,inform);
     end_if
     
     auto local_ms_shift = options.more_sorensen_shift;

//     ! d = -A\v
     DoubleFortranVector negv = w.v;
     negv *= -1.0;
     solve_spd(w.A,negv,w.LtL,d,n,inform);
     double sigma = 0.0;
     if (inform.status == NLLS_ERROR::OK) then
        //! A is symmetric positive definite....
        sigma = zero;
     }else{
        //! reset the error calls -- handled in the code....
        inform.status = NLLS_ERROR::OK;
        inform.external_return = 0;
        inform.external_name = "";
        min_eig_symm(w.A,n,sigma,w.y1,options,inform,w.min_eig_symm_ws);
        if (inform.status != NLLS_ERROR::OK) goto L1000;
        sigma = -(sigma - local_ms_shift);
        //! find a shift that makes (A + sigma I) positive definite
        get_pd_shift(n,sigma,d,options,inform,w);
        if (inform.status != NLLS_ERROR::OK) goto L4000;
     end_if
     
     nd = norm2(d);
     
//     ! now, we're not in the trust region initally, so iterate....
     auto sigma_shift = zero;
     int no_restarts = 0;
//     ! set 'small' in the context of the algorithm
     double epsilon = std::max( options.more_sorensen_tol * Delta, options.more_sorensen_tiny );
     for_do(i, 1, options.more_sorensen_maxits)
                
        if (nd <= Delta + epsilon) then
           //! we're within the tr radius
           if ( abs(sigma) < options.more_sorensen_tiny ) then
              //! we're good....exit
              goto L1020;
           }else if ( abs( nd - Delta ) < epsilon ) then
              //! also good...exit
              goto L1020;
           end_if
           double alpha = 0.0;
           findbeta(d,w.y1,Delta,alpha,inform);
           if (inform.status != NLLS_ERROR::OK ) goto L1000;
           DoubleFortranVector tmp = w.y1;
           tmp *= alpha;
           d += tmp;
           //! also good....exit
           goto L1020;
        end_if

        w.q = d; //! w.q = R'\d
        DTRSM( "Left", "Lower", "No Transpose", "Non-unit", n, 1, one, w.LtL, n, w.q, n );
        
        auto nq = norm2(w.q);
        
        sigma_shift = ( pow((nd/nq), 2) ) * ( (nd - Delta) / Delta );
        if (abs(sigma_shift) < options.more_sorensen_tiny * abs(sigma) ) then
           if (no_restarts < 1) then 
              //! find a shift that makes (A + sigma I) positive definite
              get_pd_shift(n,sigma,d,options,inform,w);
              if (inform.status != NLLS_ERROR::OK) goto L4000;
              no_restarts = no_restarts + 1;
           }else{
              //! we're not going to make progress...jump out 
              inform.status = NLLS_ERROR::MS_NO_PROGRESS;
              goto L4000;
           end_if
        }else{ 
           sigma = sigma + sigma_shift;
        end_if

        shift_matrix(w.A,sigma,w.AplusSigma,n);
        DoubleFortranVector negv = w.v;
        negv *= -1.0;
        solve_spd(w.AplusSigma,negv,w.LtL,d,n,inform);
        if (inform.status != NLLS_ERROR::OK) goto L1000;
        
        nd = norm2(d);

     end_do

     goto L1040;
     
L1000:
//     ! bad error return from external package
     goto L4000;
//     
L1020:
//     ! inital point was successful
     goto L4000;

L1040:
//     ! maxits reached, not converged
     inform.status = NLLS_ERROR::MS_MAXITS;
     goto L4000;

L3000:
//     ! too many shifts
     inform.status = NLLS_ERROR::MS_TOO_MANY_SHIFTS;
     goto L4000;
//     
L4000:
//     ! exit the routine
     if (options.scale != 0 ) then 
        for_do(i, 1, n)
           d(i) = d(i) / w.apply_scaling_ws.diag(i);
        end_do
     end_if
//     return 
} //   end subroutine more_sorensen

///     !--------------------------------------------------
///     ! get_pd_shift
///     !
///     ! Given an indefinite matrix w.A, find a shift sigma
///     ! such that (A + sigma I) is positive definite
///     !--------------------------------------------------
void get_pd_shift(int n, double& sigma, DoubleFortranVector& d, const nlls_options& options, nlls_inform& inform, more_sorensen_work& w) {
     int no_shifts = 0;
     bool successful_shift = false;
     while( ! successful_shift ) {
        shift_matrix(w.A,sigma,w.AplusSigma,n);
        solve_spd(w.AplusSigma,negative(w.v),w.LtL,d,n,inform);
        if ( inform.status != NLLS_ERROR::OK ) then
           //! reset the error calls -- handled in the code....
           inform.status = NLLS_ERROR::OK;
           inform.external_return = 0;
           inform.external_name = "";
           no_shifts = no_shifts + 1;
           if ( no_shifts == 10 ) {// goto 3000 ! too many shifts -- exit
             inform.status = NLLS_ERROR::MS_TOO_MANY_SHIFTS;
             return;
           }
           sigma =  sigma + (pow(10.0, no_shifts)) * options.more_sorensen_shift;
       }else{
           successful_shift = true;
        end_if
     }

//3000 continue
//     ! too many shifts
//     inform.status = ERROR.MS_TOO_MANY_SHIFTS
//     return     
} //   end subroutine get_pd_shift
   
///     !---------------------------------------------
///     ! solve_dtrs
///     ! Solve the trust-region subproblem using
///     ! the DTRS method from Galahad
///     ! 
///     ! This method needs H to be diagonal, so we need to 
///     ! pre-process
///     !
///     ! main output  d, the soln to the TR subproblem
///     !--------------------------------------------
void solve_dtrs(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform,
  solve_dtrs_work& w) {

     dtrs_control_type dtrs_options;
     dtrs_inform_type dtrs_inform;

//!     real, allocatable  diag(:)
//     integer  ii

//     ! The code finds 
//     !  d = arg min_p   w^T p + 0.5 * p^T D p
//     !       s.t. ||p|| \leq Delta
//     !
//     ! where D is diagonal
//     !
//     ! our probem in naturally in the form
//     ! 
//     ! d = arg min_p   v^T p + 0.5 * p^T H p
//     !       s.t. ||p|| \leq Delta
//     !
//     ! first, find the matrix H and vector v
//     ! Set A = J^T J
     matmult_inner(J,n,m,w.A);
//     ! add any second order information...
//     ! so A = J^T J + HF
     w.A += hf;

//     ! now form v = J^T f 
     mult_Jt(J,n,m,f,w.v);

//     ! if scaling needed, do it
     if ( options.scale != 0) then
        apply_scaling(J,n,m,w.A,w.v,w.apply_scaling_ws,options,inform);
     end_if

//     ! Now that we have the unprocessed matrices, we need to get an 
//     ! eigendecomposition to make A diagonal
//     !
     all_eig_symm(w.A,n,w.ew,w.ev,w.all_eig_symm_ws,inform);
     if (inform.status != NLLS_ERROR::OK) return; // goto 1000

//     ! We can now change variables, setting y = Vp, getting
//     ! Vd = arg min_(Vx) v^T p + 0.5 * (Vp)^T D (Vp)
//     !       s.t.  ||x|| \leq Delta
//     ! <=>
//     ! Vd = arg min_(Vx) V^Tv^T (Vp) + 0.5 * (Vp)^T D (Vp)
//     !       s.t.  ||x|| \leq Delta
//     ! <=>

//     ! we need to get the transformed vector v
     mult_Jt(w.ev,n,n,w.v,w.v_trans);

//     ! we've now got the vectors we need, pass to dtrs_solve
     dtrs_initialize( dtrs_options, dtrs_inform );

     for_do(ii, 1,n)
        if (abs(w.v_trans(ii)) < epsmch) then
           w.v_trans(ii) = zero;
        end_if
        if (abs(w.ew(ii)) < epsmch) then
           w.ew(ii) = zero;
        end_if
     end_do

     dtrs_solve(n, Delta, zero, w.v_trans, w.ew, w.d_trans, dtrs_options, dtrs_inform );
     if ( dtrs_inform.status != ErrorCode::ral_nlls_ok) then
        inform.external_return = int(dtrs_inform.status);
        inform.external_name = "galahad_dtrs";
        inform.status = NLLS_ERROR::FROM_EXTERNAL;
        return; // goto 1000
     end_if
     
//     ! and return the un-transformed vector
     mult_J(w.ev,n,n,w.d_trans,d);

     normd = norm2(d); // ! ||d||_D
     
     if (options.scale != 0 ) then 
        for_do(ii, 1, n)
           d(ii) = d(ii) / w.apply_scaling_ws.diag(ii);
        end_do
     end_if

//1000 continue 
//     ! bad error return from external package
//     return
//     
} //   end subroutine solve_dtrs


//!  -----------------------------------------------------------------
//!  solve_LLS, a subroutine to solve a linear least squares problem
//!  -----------------------------------------------------------------
void solve_LLS( const DoubleFortranMatrix& J, const DoubleFortranVector& f,
  int n, int m, DoubleFortranVector& d_gn, nlls_inform& inform, solve_LLS_work w) {
//
//       REAL, DIMENSION(:), INTENT(IN)  J
//       REAL, DIMENSION(:), INTENT(IN)  f
//       INTEGER, INTENT(IN)  n, m
//       REAL, DIMENSION(:), INTENT(OUT)  d_gn
//       type(NLLS_inform), INTENT(INOUT)  inform
//
//       character(1)  trans = 'N'
//       integer  nrhs = 1, lwork, lda, ldb
//       type( solve_LLS_work )  w
       
       
       auto lda = m;
       auto ldb = std::max(m,n);
       w.temp = f;
       auto lwork = int(w.work.size());
       int nrhs = 1;
       
       w.Jlls = J;
       
       dgels(trans, m, n, nrhs, w.Jlls, lda, w.temp, ldb, w.work, lwork,
            inform.external_return);
       if (inform.external_return != 0 ) then
          inform.status = NLLS_ERROR::FROM_EXTERNAL;
          inform.external_name = "lapack_dgels";
          return;
       end_if

       d_gn = negative(w.temp);
              
} //     END SUBROUTINE solve_LLS
     
//!  -----------------------------------------------------------------
//!  findbeta, a subroutine to find the optimal beta such that 
//!   || d || = Delta, where d = a + beta * b
//!   
//!   uses the approach from equation (3.20b), 
//!    "Methods for non-linear least squares problems" (2nd edition, 2004)
//!    by Madsen, Nielsen and Tingleff      
//!  -----------------------------------------------------------------
void findbeta(const DoubleFortranVector& a, const DoubleFortranVector& b, double Delta, double& beta, nlls_inform& inform) {
//
//     real, dimension(:), intent(in)  a, b 
//     real, intent(in)   Delta
//     real, intent(out)  beta
//     type( nlls_inform ), intent(out)  inform
//     
//     real  c, normb2, norma2, discrim, denom
     
     auto c = dot_product(a,b);

     auto norma2 = pow(norm2(a), 2);
     auto normb2 = pow(norm2(b), 2);

     double discrim = pow(c, 2) + (normb2)*(pow(Delta, 2) - norma2);
     if ( discrim < zero ) then
        inform.status = NLLS_ERROR::FIND_BETA;
        inform.external_name = "findbeta";
        return;
     end_if

     if (c <= 0) then
        beta = (-c + sqrt(discrim) ) / normb2;
     }else{
        beta = (Delta**2 - norma2) / ( c + sqrt(discrim) );
     end_if

} //     END SUBROUTINE findbeta

//! --------------------------------------------------
//! Input:
//! f = f(x_k), J = J(x_k), 
//! hf = \sum_{i=1}^m f_i(x_k) \nabla^2 f_i(x_k) (or an approx)
//!
//! We have a model 
//!      m_k(d) = 0.5 f^T f  + d^T J f + 0.5 d^T (J^T J + HF) d
//!
//! This subroutine evaluates the model at the point d 
//! This value is returned as the scalar
//!       md :=m_k(d)
//! --------------------------------------------------       
void evaluate_model(const DoubleFortranVector& f, const DoubleFortranMatrix& J, const DoubleFortranMatrix& hf,
  const DoubleFortranVector& d, double& md, int m, int n, const nlls_options options, evaluate_model_work& w) {

       
//       !Jd = J*d
       mult_J(J,n,m,d,w.Jd);

//       ! First, get the base 
//       ! 0.5 (f^T f + f^T J d + d^T' J ^T J d )
       DoubleFortranVector temp = f;
       temp += w.Jd;
       md = 0.5 * pow(norm2(temp), 2);
       switch(options.model) {
       case 1: //! first-order (no Hessian)
          //! nothing to do here...
          break;
       default:
          //! these have a dynamic H -- recalculate
          //! H = J^T J + HF, HF is (an approx?) to the Hessian
          mult_J(hf,n,n,d,w.Hd);
          md = md + 0.5 * dot_product(d,w.Hd);
       }

} //     end subroutine evaluate_model

//       !++++++++++++++++++++++++++++++++++++++++++++++++++++++++++!
//       ! Calculate the quantity                                   ! 
//       !   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction  !
//       !         --------------------------   ------------------- !
//       !             m_k(0)  - m_k(d)         predicted_reduction !
//       !                                                          !
//       ! if model is good, rho should be close to one             !
//       !+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void calculate_rho(double normf, double normfnew,double md, double& rho, const nlls_options& options) {

       auto actual_reduction = ( 0.5 * pow(normf, 2) ) - ( 0.5 * pow(normfnew, 2) );
       auto predicted_reduction = ( ( 0.5 * pow(normf, 2) ) - md );
       
       if ( abs(actual_reduction) < 10*epsmch ) then 
          rho = one;
       }else if (abs( predicted_reduction ) < 10 * epsmch ) then 
          rho = one;
       }else{
          rho = actual_reduction / predicted_reduction;
       end_if

} //     end subroutine calculate_rho

void apply_second_order_info(int n, int m, const DoubleFortranVector& X,
  NLLS_workspace& w, eval_hf_type eval_Hf, params_base_type params, 
  const nlls_options& options, nlls_inform& inform, const DoubleFortranVector* weights) {

       if (options.exact_second_derivatives) then
          if ( weights ) then
            DoubleFortranVector temp(m);
            for_do(i,1,m)
              temp(i) = (*weights)(i) * w.f(i);
            end_do
             eval_HF(inform.external_return, n, m, X, temp, w.hf, params);
          }else{
             eval_HF(inform.external_return, n, m, X, w.f, w.hf, params);
          end_if
          inform.h_eval = inform.h_eval + 1;
       }else{
          //! use the rank-one approximation...
          rank_one_update(w.hf,w,n);
       end_if

} //     end subroutine apply_second_order_info


void rank_one_update(DoubleFortranMatrix& hf, NLLS_workspace w, int n) {

//       real, intent(inout)  hf(:)
//       type( NLLS_workspace ), intent(inout)  w
//       integer, intent(in)  n
//      
//       real  yts, alpha, dSks

       auto yts = dot_product(w.d,w.y);
       if ( abs(yts) < 10 * epsmch ) then
          //! safeguard: skip this update
          return;
       end_if

       mult_J(hf,n,n,w.d,w.Sks); // ! hfs = S_k * d

       w.ysharpSks = w.y_sharp;
       w.ysharpSks -= w.Sks;

//       ! now, let's scale hd (Nocedal and Wright, Section 10.2)
       auto dSks = abs(dot_product(w.d,w.Sks));
       auto alpha = abs(dot_product(w.d,w.y_sharp))/ dSks;
       alpha = std::min(one,alpha);
       hf *= alpha;

//       ! update S_k (again, as in N&W, Section 10.2)

//       ! hf = hf + (1/yts) (y# - Sk d)^T y:
       alpha = 1/yts;
       //call dGER(n,n,alpha,w.ysharpSks,1,w.y,1,hf,n)
       gsl_blas_dger(alpha, w.ysharpSks.gsl(), w.y.gsl(), hf.gsl());
//       ! hf = hf + (1/yts) y^T (y# - Sk d):
       //call dGER(n,n,alpha,w.y,1,w.ysharpSks,1,hf,n)
       gsl_blas_dger(alpha, w.y.gsl(), w.ysharpSks.gsl(), hf.gsl());
//       ! hf = hf - ((y# - Sk d)^T d)/((yts)**2)) * y y^T
       alpha = -dot_product(w.ysharpSks,w.d)/(pow(yts, 2));
       //call dGER(n,n,alpha,w.y,1,w.y,1,hf,n)
       gsl_blas_dger(alpha, w.y.gsl(), w.y.gsl(), hf.gsl());

} //     end subroutine rank_one_update


void update_trust_region_radius(double& rho, const nlls_options& options, nlls_inform& inform, NLLS_workspace& w) {
//
//       real, intent(inout)  rho ! ratio of actual to predicted reduction
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(inout)  inform
//       type( nlls_workspace ), intent(inout)  w

       switch(options.tr_update_strategy) {
       case 1: //! default, step-function
          if (rho < options.eta_success_but_reduce) then
//             ! unsuccessful....reduce Delta
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
          }else if (rho < options.eta_very_successful) then 
//             ! doing ok...retain status quo
          }else if (rho < options.eta_too_successful ) then
//             ! more than very successful -- increase delta
             w.Delta = std::min(options.maximum_radius,  options.radius_increase * w.normd);
//             ! increase based on normd = ||d||_D
//             ! if d is on the tr boundary, this is Delta
//             ! otherwise, point was within the tr, and there's no point increasing 
//             ! the radius
          }else if (rho >= options.eta_too_successful) then
//             ! too successful....accept step, but don't change w.Delta
          }else{
//             ! just incase (NaNs and the like...)
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
             rho = -one; //! set to be negative, so that the logic works....
          end_if
       case 2: // ! Continuous method
//          ! Based on that proposed by Hans Bruun Nielsen, TR IMM-REP-1999-05
//          ! http://www2.imm.dtu.dk/documents/ftp/tr99/tr05_99.pdf
          if (rho >= options.eta_too_successful) then
//             ! too successful....accept step, but don't change w.Delta
          }else if (rho > options.eta_successful) then 
             w.Delta = w.Delta * std::min(options.radius_increase, std::max(options.radius_reduce,
                  1 - ( (options.radius_increase - 1) * (pow((1 - 2*rho), w.tr_p))) ));
             w.tr_nu = options.radius_reduce;
          }else if ( rho <= options.eta_successful ) then 
             w.Delta = w.Delta * w.tr_nu;
             w.tr_nu =  w.tr_nu * 0.5;
          }else{
//             ! just incase (NaNs and the like...)
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
             rho = -one; // ! set to be negative, so that the logic works....
          end_if
       default:
          inform.status = NLLS_ERROR::BAD_TR_STRATEGY;
       }

} //     end subroutine update_trust_region_radius

void test_convergence(double normF, double normJF, double normF0, double normJF0,
  const nlls_options& options, nlls_inform& inform) {

       if ( normF <= std::max(options.stop_g_absolute, 
            options.stop_g_relative * normF0) ) then
          inform.convergence_normf = 1;
          return;
       end_if

       if ( (normJF/normF) <= std::max(options.stop_g_absolute, 
            options.stop_g_relative * (normJF0/normF0)) ) then
          inform.convergence_normg = 1;
       end_if

} //     end subroutine test_convergence

void solve_spd(const DoubleFortranMatrix& A, const DoubleFortranVector& b, 
  DoubleFortranMatrix& LtL, DoubleFortranVector& x, int n, nlls_inform& inform) {
//       REAL, intent(in)  A(:,:)
//       REAL, intent(in)  b(:)
//       REAL, intent(out)  LtL(:,:)
//       REAL, intent(out)  x(:)
//       integer, intent(in)  n
//       type( nlls_inform), intent(inout)  inform
//
//       ! A wrapper for the lapack subroutine dposv.f
//       ! get workspace for the factors....
       LtL = A;
       //x = b;
//       call dposv('L', n, 1, LtL, n, x, n, inform.external_return)
//       if (inform.external_return .ne. 0) then
//          inform.status = ERROR.FROM_EXTERNAL
//          inform.external_name = 'lapack_dposv'
//          return
//       end if
       LtL.solve(b, x);
} //     end subroutine solve_spd

void solve_general(const DoubleFortranMatrix& A, const DoubleFortranVector& b,
   DoubleFortranVector& x, int n, nlls_inform& inform, solve_general_work& w) {
//       ! A wrapper for the lapack subroutine dposv.f
//       ! NOTE: A would be destroyed
       w.A = A;
       w.A.solve(b, x);
//       x(1:n) = b(1:n)
//       call dgesv( n, 1, w.A, n, w.ipiv, x, n, inform.external_return)
} //     end subroutine solve_general

void all_eig_symm(const DoubleFortranMatrix& A, int n, DoubleFortranVector& ew, 
  DoubleFortranMatrix& ev,all_eig_symm_work& w,nlls_inform& inform) {
//       ! calculate all the eigenvalues of A (symmetric)
  auto M = A;
  M.eigenSystem(ew, ev);
} //     end subroutine all_eig_symm

//       ! calculate the leftmost eigenvalue of A
void min_eig_symm(const DoubleFortranMatrix& A, int n, DoubleFortranVector& ew,
  DoubleFortranMatrix& ev,const nlls_options& options,nlls_inform& inform,min_eig_symm_work& w) {
  auto M = A;
  M.eigenSystem(ew, ev);
} //     end subroutine min_eig_symm

//     subroutine max_eig(A,B,n,ew,ev,nullevs,options,inform,w)
//
//       real, intent(inout)  A(:,:), B(:,:)
//       integer, intent(in)  n 
//       real, intent(out)  ew, ev(:)
//       real, intent(inout), allocatable  nullevs(:,:)
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(inout)  inform
//       type( max_eig_work )  w
//
//       integer  lwork, maxindex(1), no_null, halfn
//       real tau
//       integer  i 
//
//       ! Find the max eigenvalue/vector of the generalized eigenproblem
//       !     A * y = lam * B * y
//       ! further, if ||y(1:n/2)|| \approx 0, find and return the 
//       ! eigenvectors y(n/2+1:n) associated with this
//
//       ! check that n is even (important for hard case -- see below)
//       if (modulo(n,2).ne.0) goto 1010
//
//       halfn = n/2
//       lwork = size(w.work)
//       call dggev('N', & ! No left eigenvectors
//            'V', &! Yes right eigenvectors
//            n, A, n, B, n, &
//            w.alphaR, w.alphaI, w.beta, & ! eigenvalue data
//            w.vr, n, & ! not referenced
//            w.vr, n, & ! right eigenvectors
//            w.work, lwork, inform.external_return)
//       if (inform.external_return .ne. 0) then
//          inform.status = ERROR.FROM_EXTERNAL
//          inform.external_name = 'lapack_dggev'
//          return
//       end if
//
//       ! now find the rightmost real eigenvalue
//       w.vecisreal = .true.
//       where ( abs(w.alphaI) > 1e-8 ) w.vecisreal = .false.
//
//       w.ew_array(:) = w.alphaR(:)/w.beta(:)
//       maxindex = maxloc(w.ew_array,w.vecisreal)
//       if (maxindex(1) == 0) goto 1000
//
//       tau = 1e-4 ! todo -- pass this through from above...
//       ! note n/2 always even -- validated by test on entry
//       if (norm2( w.vr(1:halfn,maxindex(1)) ) < tau) then 
//          ! hard case
//          ! let's find which ev's are null...
//          w.nullindex = 0
//          no_null = 0
//          do i = 1,n
//             if (norm2( w.vr(1:halfn,i)) < 1e-4 ) then
//                no_null = no_null + 1 
//                w.nullindex(no_null) = i
//             end if
//          end do
//!          allocate(nullevs(halfn,no_null))
//          if (no_null > size(nullevs,2)) then
//             ! increase the size of the allocated array only if we need to
//             if(allocated(nullevs)) deallocate( nullevs )
//             allocate( nullevs(halfn,no_null) , stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 2000
//          end if 
//          nullevs(1:halfn,1:no_null) = w.vr(halfn+1 : n,w.nullindex(1:no_null))
//       end if
//
//       ew = w.alphaR(maxindex(1))/w.beta(maxindex(1))
//       ev(:) = w.vr(:,maxindex(1))
//
//       return 
//
//1000   continue 
//       inform.status = ERROR.AINT_EIG_IMAG ! Eigs imaginary error
//       return
//
//1010   continue
//       inform.status = ERROR.AINT_EIG_ODD
//       return
//       
//2000   continue
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "max_eig"
//       return
//
//     end subroutine max_eig
//
//     subroutine shift_matrix(A,sigma,AplusSigma,n)
//
//       real, intent(in)   A(:,:), sigma
//       real, intent(out)  AplusSigma(:,:)
//       integer, intent(in)  n 
//
//       integer  i 
//       ! calculate AplusSigma = A + sigma * I
//
//       AplusSigma(:,:) = A(:,:)
//       do i = 1,n
//          AplusSigma(i,i) = AplusSigma(i,i) + sigma
//       end do
//
//     end subroutine shift_matrix
//
//     subroutine get_svd_J(n,m,J,s1,sn,options,status,w)
//       integer, intent(in)  n,m 
//       real, intent(in)  J(:)
//       real, intent(out)  s1, sn
//       type( nlls_options )  options
//       integer, intent(out)  status
//       type( get_svd_J_work )  w
//
//       !  Given an (m x n)  matrix J held by columns as a vector,
//       !  this routine returns the largest and smallest singular values
//       !  of J.
//
//       character  jobu(1), jobvt(1)
//       integer  lwork
//
//       w.Jcopy(:) = J(:)
//
//       jobu  = 'N' ! calculate no left singular vectors
//       jobvt = 'N' ! calculate no right singular vectors
//
//       lwork = size(w.work)
//
//       call dgesvd( JOBU, JOBVT, n, m, w.Jcopy, n, w.S, w.S, 1, w.S, 1, & 
//            w.work, lwork, status )
//       if ( (status .ne. 0) .and. (options.print_level > 3) ) then 
//          write(options.error,'(a,i0)') 'Error when calculating svd, dgesvd returned', &
//               status
//          s1 = -1.0
//          sn = -1.0
//          ! allow to continue, but warn user and return zero singular values
//       else
//          s1 = w.S(1)
//          sn = w.S(n)
//          if (options.print_level > 2) then 
//             write(options.out,'(a,es12.4,a,es12.4)') 's1 = ', s1, '    sn = ', sn
//             write(options.out,'(a,es12.4)') 'k(J) = ', s1/sn
//          end if
//       end if
//
//     end subroutine get_svd_J
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//     !!                                                       !!
//     !! W O R K S P A C E   S E T U P   S U B R O U T I N E S !!
//     !!                                                       !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//     subroutine setup_workspaces(workspace,n,m,options,inform)
//
//       type( NLLS_workspace ), intent(out)  workspace
//       type( nlls_options ), intent(in)  options
//       integer, intent(in)  n,m
//       type( NLLS_inform ), intent(out)  inform
//
//       workspace.first_call = 0
//
//       workspace.tr_nu = options.radius_increase
//       workspace.tr_p = 7
//
//       if (.not. allocated(workspace.y)) then
//          allocate(workspace.y(n), stat = inform.alloc_status)
//          if (inform.alloc_status .ne. 0) goto 1000
//          workspace.y = zero
//       end if
//       if (.not. allocated(workspace.y_sharp)) then
//          allocate(workspace.y_sharp(n), stat = inform.alloc_status)
//          if (inform.alloc_status .ne. 0) goto 1000
//          workspace.y_sharp = zero
//       end if
//
//       if (.not. options.exact_second_derivatives) then
//          if (.not. allocated(workspace.g_old)) then
//             allocate(workspace.g_old(n), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          if (.not. allocated(workspace.g_mixed)) then
//             allocate(workspace.g_mixed(n), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          if (.not. allocated(workspace.Sks)) then
//             allocate(workspace.Sks(n), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          if (.not. allocated(workspace.ysharpSks)) then
//             allocate(workspace.ysharpSks(n), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//
//       end if
//
//       if( options.output_progress_vectors ) then 
//          if (.not. allocated(workspace.resvec)) then
//             allocate(workspace.resvec(options.maxit+1), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          if (.not. allocated(workspace.gradvec)) then
//             allocate(workspace.gradvec(options.maxit+1), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//       end if
//
//       if( options.calculate_svd_J ) then
//          if (.not. allocated(workspace.largest_sv)) then
//             allocate(workspace.largest_sv(options.maxit + 1), &
//                  stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          if (.not. allocated(workspace.smallest_sv)) then
//             allocate(workspace.smallest_sv(options.maxit + 1), &
//                  stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//          call setup_workspace_get_svd_J(n,m,workspace.get_svd_J_ws, & 
//               options, inform)
//          if (inform.alloc_status > 0) goto 1010
//       end if
//
//       if( .not. allocated(workspace.J)) then
//          allocate(workspace.J(n*m), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( .not. allocated(workspace.f)) then
//          allocate(workspace.f(m), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( .not. allocated(workspace.fnew)) then 
//          allocate(workspace.fnew(m), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( .not. allocated(workspace.hf)) then
//          allocate(workspace.hf(n*n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( options.model == 3 ) then
//          if( .not. allocated(workspace.hf_temp)) then 
//             allocate(workspace.hf_temp(n*n), stat = inform.alloc_status)
//             if (inform.alloc_status > 0) goto 1000
//          end if
//       end if
//       if( .not. allocated(workspace.d)) then
//          allocate(workspace.d(n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( .not. allocated(workspace.g)) then
//          allocate(workspace.g(n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//       if( .not. allocated(workspace.Xnew)) then
//          allocate(workspace.Xnew(n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 1000
//       end if
//
//
//       select case (options.nlls_method)
//
//       case (1) ! use the dogleg method
//          call setup_workspace_dogleg(n,m,workspace.calculate_step_ws.dogleg_ws, & 
//               options, inform)
//          if (inform.alloc_status > 0) goto 1010
//
//       case(2) ! use the AINT method
//          call setup_workspace_AINT_tr(n,m,workspace.calculate_step_ws.AINT_tr_ws, & 
//               options, inform)
//          if (inform.alloc_status > 0) goto 1010
//
//       case(3) ! More-Sorensen 
//          call setup_workspace_more_sorensen(n,m,&
//               workspace.calculate_step_ws.more_sorensen_ws,options,inform)
//          if (inform.alloc_status > 0) goto 1010
//
//       case (4) ! dtrs (Galahad)
//          call setup_workspace_solve_dtrs(n,m, & 
//               workspace.calculate_step_ws.solve_dtrs_ws, options, inform)
//          if (inform.alloc_status > 0) goto 1010
//       end select
//
//       ! evaluate model in the main routine...       
//       call setup_workspace_evaluate_model(n,m,workspace.evaluate_model_ws,options,inform)
//       if (inform.alloc_status > 0) goto 1010
//
//       return
//
//       ! Error statements
//1000   continue ! bad allocation from this subroutine
//       inform.bad_alloc = 'setup_workspaces'
//       inform.status = ERROR.ALLOCATION
//       return
//
//1010   continue ! bad allocation from called subroutine
//       return
//
//     end subroutine setup_workspaces
//
//     subroutine remove_workspaces(workspace,options)
//
//       type( NLLS_workspace ), intent(out)  workspace
//       type( nlls_options ), intent(in)  options
//
//       workspace.first_call = 0
//
//       if(allocated(workspace.y)) deallocate(workspace.y)
//       if(allocated(workspace.y_sharp)) deallocate(workspace.y_sharp)
//       if(allocated(workspace.g_old)) deallocate(workspace.g_old)
//       if(allocated(workspace.g_mixed)) deallocate(workspace.g_mixed)
//
//       if(allocated(workspace.resvec)) deallocate(workspace.resvec)
//       if(allocated(workspace.gradvec)) deallocate(workspace.gradvec)
//
//       if(allocated(workspace.largest_sv)) deallocate(workspace.largest_sv)
//       if(allocated(workspace.smallest_sv)) deallocate(workspace.smallest_sv)
//
//       if(allocated(workspace.fNewton)) deallocate(workspace.fNewton )
//       if(allocated(workspace.JNewton)) deallocate(workspace.JNewton )
//       if(allocated(workspace.XNewton)) deallocate(workspace.XNewton ) 
//
//       if( options.calculate_svd_J ) then
//          if (allocated(workspace.largest_sv)) deallocate(workspace.largest_sv)
//          if (allocated(workspace.smallest_sv)) deallocate(workspace.smallest_sv)
//          call remove_workspace_get_svd_J(workspace.get_svd_J_ws, options)
//       end if
//
//       if(allocated(workspace.J)) deallocate(workspace.J ) 
//       if(allocated(workspace.f)) deallocate(workspace.f ) 
//       if(allocated(workspace.fnew)) deallocate(workspace.fnew ) 
//       if(allocated(workspace.hf)) deallocate(workspace.hf ) 
//       if(allocated(workspace.hf_temp)) deallocate(workspace.hf_temp) 
//       if(allocated(workspace.d)) deallocate(workspace.d ) 
//       if(allocated(workspace.g)) deallocate(workspace.g ) 
//       if(allocated(workspace.Xnew)) deallocate(workspace.Xnew ) 
//
//       select case (options.nlls_method)
//
//       case (1) ! use the dogleg method
//          call remove_workspace_dogleg(workspace.calculate_step_ws.dogleg_ws, & 
//               options)
//
//       case(2) ! use the AINT method
//          call remove_workspace_AINT_tr(workspace.calculate_step_ws.AINT_tr_ws, & 
//               options)
//
//       case(3) ! More-Sorensen 
//          call remove_workspace_more_sorensen(&
//               workspace.calculate_step_ws.more_sorensen_ws,options)
//
//       case (4) ! dtrs (Galahad)
//          call remove_workspace_solve_dtrs(& 
//               workspace.calculate_step_ws.solve_dtrs_ws, options)
//
//       end select
//
//       ! evaluate model in the main routine...       
//       call remove_workspace_evaluate_model(workspace.evaluate_model_ws,options)
//
//       return
//
//     end subroutine remove_workspaces
//
//     subroutine setup_workspace_get_svd_J(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( get_svd_J_work ), intent(out)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       integer  lwork
//       character  jobu(1), jobvt(1)
//
//       allocate( w.Jcopy(n*m), stat = inform.alloc_status)
//       if (inform.alloc_status .ne. 0) goto 9000
//
//       allocate( w.S(n), stat = inform.alloc_status)
//       if (inform.alloc_status .ne. 0) goto 9000
//
//       allocate(w.work(1))
//       jobu  = 'N' ! calculate no left singular vectors
//       jobvt = 'N' ! calculate no right singular vectors
//
//       ! make a workspace query....
//       call dgesvd( jobu, jobvt, n, m, w.Jcopy, n, w.S, w.S, 1, w.S, 1, & 
//            w.work, -1, inform.alloc_status )
//       if ( inform.alloc_status .ne. 0 ) goto 9000
//
//       lwork = int(w.work(1))
//       deallocate(w.work)
//       allocate(w.work(lwork))     
//
//       return
//
//       ! Error statements
//9000   continue  ! bad allocations in this subroutine
//       inform.bad_alloc = 'setup_workspace_get_svd_J'
//       inform.status = ERROR.ALLOCATION
//       return
//
//     end subroutine setup_workspace_get_svd_J
//
//     subroutine remove_workspace_get_svd_J(w,options)
//       type( get_svd_J_work )  w
//       type( nlls_options )  options
//
//       if( allocated(w.Jcopy) ) deallocate( w.Jcopy )
//       if( allocated(w.S) ) deallocate( w.S )
//       if( allocated(w.work) ) deallocate( w.work )
//
//       return
//
//     end subroutine remove_workspace_get_svd_J
//
//     subroutine setup_workspace_dogleg(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( dogleg_work ), intent(out)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.d_sd(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.d_gn(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000           
//       allocate(w.ghat(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.Jg(m),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       ! setup space for 
//       !   solve_LLS
//       call setup_workspace_solve_LLS(n,m,w.solve_LLS_ws,options,inform)
//       if (inform.status > 0 ) goto 9010
//       ! setup space for 
//       !   evaluate_model
//       call setup_workspace_evaluate_model(n,m,w.evaluate_model_ws,options,inform)
//       if (inform.status > 0 ) goto 9010
//
//       return
//
//       ! Error statements
//9000   continue  ! bad allocations in this subroutine
//       inform.bad_alloc = 'setup_workspace_dogleg'
//       inform.status = ERROR.ALLOCATION
//       return
//
//9010   continue  ! bad allocations from dependent subroutine
//       return
//
//
//     end subroutine setup_workspace_dogleg
//
//     subroutine remove_workspace_dogleg(w,options)
//       type( dogleg_work ), intent(out)  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.d_sd )) deallocate(w.d_sd ) 
//       if(allocated( w.d_gn )) deallocate(w.d_gn )
//       if(allocated( w.ghat )) deallocate(w.ghat )
//       if(allocated( w.Jg )) deallocate(w.Jg )
//
//       ! deallocate space for 
//       !   solve_LLS
//       call remove_workspace_solve_LLS(w.solve_LLS_ws,options)
//       ! deallocate space for 
//       !   evaluate_model
//       call remove_workspace_evaluate_model(w.evaluate_model_ws,options)
//
//       return
//
//     end subroutine remove_workspace_dogleg
//
//     subroutine setup_workspace_solve_LLS(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( solve_LLS_work )  w 
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//       integer  lwork
//
//       allocate( w.temp(max(m,n)), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       lwork = max(1, min(m,n) + max(min(m,n), 1)*4) 
//       allocate( w.work(lwork), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.Jlls(n*m), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue  ! local allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_LLS"
//       return
//
//     end subroutine setup_workspace_solve_LLS
//
//     subroutine remove_workspace_solve_LLS(w,options)
//       type( solve_LLS_work )  w 
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.temp )) deallocate( w.temp)
//       if(allocated( w.work )) deallocate( w.work ) 
//       if(allocated( w.Jlls )) deallocate( w.Jlls ) 
//
//       return
//
//     end subroutine remove_workspace_solve_LLS
//
//     subroutine setup_workspace_evaluate_model(n,m,w,options,inform)
//       integer, intent(in)  n, m        
//       type( evaluate_model_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate( w.Jd(m), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.Hd(n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = 'evaluate_model'
//       return
//     end subroutine setup_workspace_evaluate_model
//
//     subroutine remove_workspace_evaluate_model(w,options)
//       type( evaluate_model_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.Jd )) deallocate( w.Jd ) 
//       if(allocated( w.Hd )) deallocate( w.Hd ) 
//
//       return
//
//     end subroutine remove_workspace_evaluate_model
//
//     subroutine setup_workspace_AINT_tr(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( AINT_tr_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.B(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.p0(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.p1(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.M0(2*n,2*n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.M1(2*n,2*n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.M0_small(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.M1_small(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.y(2*n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.gtg(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.q(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.LtL(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.y_hardcase(n,2), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       ! setup space for max_eig
//       call setup_workspace_max_eig(n,m,w.max_eig_ws,options,inform)
//       if (inform.status > 0) goto 9010
//       call setup_workspace_evaluate_model(n,m,w.evaluate_model_ws,options,inform)
//       if (inform.status > 0) goto 9010
//       ! setup space for the solve routine
//       if ((options.model .ne. 1)) then
//          call setup_workspace_solve_general(n,m,w.solve_general_ws,options,inform)
//          if (inform.status > 0 ) goto 9010
//       end if
//
//       return
//
//9000   continue ! local allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "AINT_tr"
//       !call allocation_error(options,'AINT_tr')
//       return
//
//9010   continue ! allocation error from called subroutine
//       return
//
//     end subroutine setup_workspace_AINT_tr
//
//     subroutine remove_workspace_AINT_tr(w,options)
//       type( AINT_tr_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)
//       if(allocated( w.v )) deallocate(w.v)
//       if(allocated( w.B )) deallocate(w.B)
//       if(allocated( w.p0 )) deallocate(w.p0)
//       if(allocated( w.p1 )) deallocate(w.p1)
//       if(allocated( w.M0 )) deallocate(w.M0)
//       if(allocated( w.M1 )) deallocate(w.M1)
//       if(allocated( w.M0_small )) deallocate(w.M0_small)
//       if(allocated( w.M1_small )) deallocate(w.M1_small)
//       if(allocated( w.y )) deallocate(w.y)
//       if(allocated( w.gtg )) deallocate(w.gtg)
//       if(allocated( w.q )) deallocate(w.q)
//       if(allocated( w.LtL )) deallocate(w.LtL)
//       if(allocated( w.y_hardcase )) deallocate(w.y_hardcase)
//       ! setup space for max_eig
//       call remove_workspace_max_eig(w.max_eig_ws,options)
//       call remove_workspace_evaluate_model(w.evaluate_model_ws,options)
//       ! setup space for the solve routine
//       if (options.model .ne. 1) then
//          call remove_workspace_solve_general(w.solve_general_ws,options)
//       end if
//
//       return
//
//     end subroutine remove_workspace_AINT_tr
//
//     subroutine setup_workspace_min_eig_symm(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( min_eig_symm_work)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       real, allocatable  workquery(:)
//       integer  lwork, eigsout
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       allocate(workquery(1),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       if (options.subproblem_eig_fact) then 
//          allocate(w.ew(n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 9000
//
//          call dsyev('V', & ! both ew's and ev's 
//               'U', & ! upper triangle of A
//               n, w.A, n, & ! data about A
//               w.ew, workquery, -1, & 
//               inform.external_return)
//          if (inform.external_return .ne. 0) goto 9010
//       else
//          allocate( w.iwork(5*n), stat = inform.alloc_status )
//          if (inform.alloc_status > 0) goto 9000
//          allocate( w.ifail(n), stat = inform.alloc_status ) 
//          if (inform.alloc_status > 0) goto 9000
//
//          ! make a workspace query to dsyevx
//          call dsyevx( 'V',& ! get both ew's and ev's
//               'I',& ! just the numbered eigenvalues
//               'U',& ! upper triangle of A
//               n, w.A, n, & 
//               1.0, 1.0, & ! not used for RANGE = 'I'
//               1, 1, & ! only find the first eigenpair
//               0.5, & ! abstol for the eigensolver
//               eigsout, & ! total number of eigs found
//               1.0, 1.0, & ! the eigenvalue and eigenvector
//               n, & ! ldz (the eigenvector array)
//               workquery, -1, w.iwork, &  ! workspace
//               w.ifail, & ! array containing indicies of non-converging ews
//               inform.external_return)
//          if (inform.external_return .ne. 0) goto 9020
//       end if
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue      
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "min_eig_symm"
//       return
//
//9010   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyev"
//
//9020   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyevx"
//       return
//
//     end subroutine setup_workspace_min_eig_symm
//
//     subroutine remove_workspace_min_eig_symm(w,options)
//       type( min_eig_symm_work)  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)        
//
//       if (options.subproblem_eig_fact) then 
//          if(allocated( w.ew )) deallocate(w.ew)
//       else
//          if(allocated( w.iwork )) deallocate( w.iwork )
//          if(allocated( w.ifail )) deallocate( w.ifail ) 
//       end if
//       if(allocated( w.work )) deallocate( w.work ) 
//
//       return
//
//     end subroutine remove_workspace_min_eig_symm
//
//     subroutine setup_workspace_max_eig(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( max_eig_work)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform), intent(out)  inform
//       real, allocatable  workquery(:)
//       integer  lwork
//
//       allocate( w.alphaR(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.alphaR = zero
//       allocate( w.alphaI(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.alphaI = zero
//       allocate( w.beta(2*n),   stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.beta = zero
//       allocate( w.vr(2*n,2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.ew_array(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.ew_array = zero
//       allocate(workquery(1),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       ! make a workspace query to dggev
//       call dggev('N', & ! No left eigenvectors
//            'V', &! Yes right eigenvectors
//            2*n, 1.0, 2*n, 1.0, 2*n, &
//            1.0, 0.1, 0.1, & ! eigenvalue data
//            0.1, 2*n, & ! not referenced
//            0.1, 2*n, & ! right eigenvectors
//            workquery, -1, inform.external_return)
//       if (inform.external_return > 0) goto 9020
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.nullindex(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.vecisreal(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "max_eig"
//       return
//
//9020   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dggev"
//       return
//
//     end subroutine setup_workspace_max_eig
//
//     subroutine remove_workspace_max_eig(w,options)
//       type( max_eig_work)  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.alphaR )) deallocate( w.alphaR)
//       if(allocated( w.alphaI )) deallocate( w.alphaI )
//       if(allocated( w.beta )) deallocate( w.beta ) 
//       if(allocated( w.vr )) deallocate( w.vr ) 
//       if(allocated( w.ew_array )) deallocate( w.ew_array ) 
//       if(allocated( w.work )) deallocate( w.work ) 
//       if(allocated( w.nullindex )) deallocate( w.nullindex ) 
//       if(allocated( w.vecisreal )) deallocate( w.vecisreal )
//
//       return
//
//     end subroutine remove_workspace_max_eig
//
//     subroutine setup_workspace_solve_general(n, m, w, options, inform)
//       integer, intent(in)  n, m 
//       type( solve_general_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform), intent(out)  inform
//
//       allocate( w.A(n,n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.ipiv(n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue ! allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_general"
//       return
//
//     end subroutine setup_workspace_solve_general
//
//     subroutine remove_workspace_solve_general(w, options)
//       type( solve_general_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate( w.A ) 
//       if(allocated( w.ipiv )) deallocate( w.ipiv ) 
//       return
//
//     end subroutine remove_workspace_solve_general
//
//     subroutine setup_workspace_solve_dtrs(n,m,w,options,inform)
//       integer, intent(in)  n,m
//       type( solve_dtrs_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.ev(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v_trans(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.ew(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.d_trans(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       call setup_workspace_all_eig_symm(n,m,w.all_eig_symm_ws,options,inform)
//       if (inform.status > 0) goto 9010
//
//       if (options.scale > 0) then
//          call setup_workspace_apply_scaling(n,m,w.apply_scaling_ws,options,inform)
//          if (inform.status > 0) goto 9010
//       end if
//
//       return
//
//9000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_dtrs"
//       return
//
//9010   continue  ! allocation error from called subroutine
//       return
//
//     end subroutine setup_workspace_solve_dtrs
//
//     subroutine remove_workspace_solve_dtrs(w,options)
//       type( solve_dtrs_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)
//       if(allocated( w.ev )) deallocate(w.ev)
//       if(allocated( w.v )) deallocate(w.v)
//       if(allocated( w.v_trans )) deallocate(w.v_trans)
//       if(allocated( w.ew )) deallocate(w.ew)
//       if(allocated( w.d_trans )) deallocate(w.d_trans)
//
//       call remove_workspace_all_eig_symm(w.all_eig_symm_ws,options)
//       if (options.scale > 0) then
//          call remove_workspace_apply_scaling(w.apply_scaling_ws,options)
//       end if
//
//       return
//
//     end subroutine remove_workspace_solve_dtrs
//
//     subroutine setup_workspace_all_eig_symm(n,m,w,options,inform)
//       integer, intent(in)  n,m
//       type( all_eig_symm_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       real, allocatable  workquery(:)
//       real  A,ew
//       integer  lwork
//
//       A = 1.0_wp
//       ew = 1.0_wp
//
//       allocate(workquery(1), stat = inform.alloc_status)
//       if (inform.alloc_status .ne. 0 ) goto 8000
//       call dsyev('V', & ! both ew's and ev's 
//            'U', & ! upper triangle of A
//            n, A, n, & ! data about A
//            ew, workquery, -1, & 
//            inform.external_return)  
//       if (inform.external_return .ne. 0) goto 9000
//
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 8000
//
//       return
//
//8000   continue  ! allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "all_eig_sym"
//       return
//
//9000   continue ! error from lapack
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyev"
//       return
//
//     end subroutine setup_workspace_all_eig_symm
//
//     subroutine remove_workspace_all_eig_symm(w,options)
//       type( all_eig_symm_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.work )) deallocate( w.work ) 
//
//     end subroutine remove_workspace_all_eig_symm
//
//     subroutine setup_workspace_more_sorensen(n,m,w,options,inform)
//       integer, intent(in)  n,m
//       type( more_sorensen_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.LtL(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.q(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.y1(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.AplusSigma(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       call setup_workspace_min_eig_symm(n,m,w.min_eig_symm_ws,options,inform)
//       if (inform.status > 0) goto 9010
//
//       if (options.scale > 0) then
//          call setup_workspace_apply_scaling(n,m,w.apply_scaling_ws,options,inform)
//          if (inform.status > 0) goto 9010
//       end if
//
//       return
//
//9000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "more_sorenesen"
//       return
//
//9010   continue ! error from called subroutine
//       return
//
//     end subroutine setup_workspace_more_sorensen
//
//     subroutine remove_workspace_more_sorensen(w,options)
//       type( more_sorensen_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)
//       if(allocated( w.LtL )) deallocate(w.LtL)
//       if(allocated( w.v )) deallocate(w.v)
//       if(allocated( w.q )) deallocate(w.q)
//       if(allocated( w.y1 )) deallocate(w.y1)
//       if(allocated( w.AplusSigma )) deallocate(w.AplusSigma)
//
//       call remove_workspace_min_eig_symm(w.min_eig_symm_ws,options)
//       if (options.scale > 0) then
//          call remove_workspace_apply_scaling(w.apply_scaling_ws,options)
//       end if
//
//       return
//
//     end subroutine remove_workspace_more_sorensen
//
//
//     subroutine setup_workspace_apply_scaling(n,m,w,options,inform)
//
//       integer, intent(in)  n,m
//       type( apply_scaling_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.diag(n), stat = inform.alloc_status )
//       if (inform.alloc_status .ne. 0) goto 1000
//       w.diag = one
//       allocate(w.ev(n,n), stat = inform.alloc_status )
//       if (inform.alloc_status .ne. 0) goto 1000
//
//       if (options.scale == 4) then
//          allocate(w.tempvec(n))
//          call setup_workspace_all_eig_symm(n,m,w.all_eig_symm_ws,options,inform)
//          if (inform.status .ne. 0) goto 1010
//       end if
//
//       return
//
//1000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "apply_scaling"
//       return
//
//1010   continue ! error from lower down subroutine
//       return
//
//     end subroutine setup_workspace_apply_scaling
//
//     subroutine remove_workspace_apply_scaling(w,options)
//       type( apply_scaling_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.diag )) deallocate( w.diag )
//       if(allocated( w.ev )) deallocate( w.ev ) 
//
//       return 
//
//     end subroutine remove_workspace_apply_scaling
//
//
//
//   end module ral_nlls_internal
//


} // namespace RalNlls
}
}
