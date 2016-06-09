#include "MantidCurveFitting/RalNlls/TrustRegion.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <string>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

namespace Mantid {
namespace CurveFitting {
namespace NLLS {

const double epsmch = std::numeric_limits<double>::epsilon();

///  Given an (m x n)  matrix J held by columns as a vector,
///  this routine returns the largest and smallest singular values
///  of J.
void get_svd_J(const DoubleFortranMatrix &J, double &s1, double &sn) {

  auto n = J.len2();
  DoubleFortranMatrix U = J;
  DoubleFortranMatrix V(n, n);
  DoubleFortranVector S(n);
  DoubleFortranVector work(n);
  gsl_linalg_SV_decomp(U.gsl(), V.gsl(), S.gsl(), work.gsl());
  s1 = S(1);
  sn = S(n);
}

double norm2(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

void mult_J(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
            DoubleFortranVector &Jx) {
  // dgemv('N',m,n,alpha,J,m,x,1,beta,Jx,1);
  if (Jx.len() != J.len1()) {
    Jx.allocate(J.len1());
  }
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jx.gsl());
}

void mult_Jt(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
             DoubleFortranVector &Jtx) {
  // dgemv('T',m,n,alpha,J,m,x,1,beta,Jtx,1)
  if (Jtx.len() != J.len2()) {
    Jtx.allocate(J.len2());
  }
  gsl_blas_dgemv(CblasTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jtx.gsl());
}

double dot_product(const DoubleFortranVector &x, const DoubleFortranVector &y) {
  return x.dot(y);
}


/// Input:
/// f = f(x_k), J = J(x_k),
/// hf = \sum_{i=1}^m f_i(x_k) \nabla^2 f_i(x_k) (or an approx)
///
/// We have a model
///      m_k(d) = 0.5 f^T f  + d^T J f + 0.5 d^T (J^T J + HF) d
///
/// This subroutine evaluates the model at the point d
/// This value is returned as the scalar
///       md :=m_k(d)
void evaluate_model(const DoubleFortranVector &f, const DoubleFortranMatrix &J,
                    const DoubleFortranMatrix &hf, const DoubleFortranVector &d,
                    double &md, int m, int n, const nlls_options options,
                    evaluate_model_work &w) {

  // Jd = J*d
  mult_J(J, d, w.Jd);

  // First, get the base
  // 0.5 (f^T f + f^T J d + d^T' J ^T J d )
  DoubleFortranVector temp = f;
  temp += w.Jd;
  md = 0.5 * pow(norm2(temp), 2);
  switch (options.model) {
  case 1: // first-order (no Hessian)
    //! nothing to do here...
    break;
  default:
    // these have a dynamic H -- recalculate
    // H = J^T J + HF, HF is (an approx?) to the Hessian
    mult_J(hf, d, w.Hd);
    md = md + 0.5 * dot_product(d, w.Hd);
  }
}

/// Calculate the quantity
///   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction
///         --------------------------   -------------------
///             m_k(0)  - m_k(d)         predicted_reduction
///
/// if model is good, rho should be close to one
void calculate_rho(double normf, double normfnew, double md, double &rho,
                   const nlls_options &options) {

  auto actual_reduction = (0.5 * pow(normf, 2)) - (0.5 * pow(normfnew, 2));
  auto predicted_reduction = ((0.5 * pow(normf, 2)) - md);

  if (abs(actual_reduction) < 10 * epsmch) {
    rho = one;
  } else if (abs(predicted_reduction) < 10 * epsmch) {
    rho = one;
  } else {
    rho = actual_reduction / predicted_reduction;
  }
}

void rank_one_update(DoubleFortranMatrix &hf, NLLS_workspace w, int n) {

  auto yts = dot_product(w.d, w.y);
  if (abs(yts) < 10 * epsmch) {
    //! safeguard: skip this update
    return;
  }

  mult_J(hf, w.d, w.Sks); // hfs = S_k * d

  w.ysharpSks = w.y_sharp;
  w.ysharpSks -= w.Sks;

  // now, let's scale hd (Nocedal and Wright, Section 10.2)
  auto dSks = abs(dot_product(w.d, w.Sks));
  auto alpha = abs(dot_product(w.d, w.y_sharp)) / dSks;
  alpha = std::min(one, alpha);
  hf *= alpha;

  // update S_k (again, as in N&W, Section 10.2)

  // hf = hf + (1/yts) (y# - Sk d)^T y:
  alpha = 1 / yts;
  // call dGER(n,n,alpha,w.ysharpSks,1,w.y,1,hf,n)
  gsl_blas_dger(alpha, w.ysharpSks.gsl(), w.y.gsl(), hf.gsl());
  // hf = hf + (1/yts) y^T (y# - Sk d):
  // call dGER(n,n,alpha,w.y,1,w.ysharpSks,1,hf,n)
  gsl_blas_dger(alpha, w.y.gsl(), w.ysharpSks.gsl(), hf.gsl());
  // hf = hf - ((y# - Sk d)^T d)/((yts)**2)) * y y^T
  alpha = -dot_product(w.ysharpSks, w.d) / (pow(yts, 2));
  // call dGER(n,n,alpha,w.y,1,w.y,1,hf,n)
  gsl_blas_dger(alpha, w.y.gsl(), w.y.gsl(), hf.gsl());
}

void update_trust_region_radius(double &rho, const nlls_options &options,
                                nlls_inform &inform, NLLS_workspace &w) {

  switch (options.tr_update_strategy) {
  case 1: // default, step-function
    if (rho < options.eta_success_but_reduce) {
      // unsuccessful....reduce Delta
      w.Delta =
          std::max(options.radius_reduce, options.radius_reduce_max) * w.Delta;
    } else if (rho < options.eta_very_successful) {
      //  doing ok...retain status quo
    } else if (rho < options.eta_too_successful) {
      // more than very successful -- increase delta
      w.Delta =
          std::min(options.maximum_radius, options.radius_increase * w.normd);
      // increase based on normd = ||d||_D
      // if d is on the tr boundary, this is Delta
      // otherwise, point was within the tr, and there's no point
      // increasing the radius
    } else if (rho >= options.eta_too_successful) {
      // too successful....accept step, but don't change w.Delta
    } else {
      // just incase (NaNs and the like...)
      w.Delta =
          std::max(options.radius_reduce, options.radius_reduce_max) * w.Delta;
      rho = -one; // set to be negative, so that the logic works....
    }
    break;
  case 2: //  Continuous method
          //  Based on that proposed by Hans Bruun Nielsen, TR
          //  IMM-REP-1999-05
          //  http://www2.imm.dtu.dk/documents/ftp/tr99/tr05_99.pdf
    if (rho >= options.eta_too_successful) {
      // too successful....accept step, but don't change w.Delta
    } else if (rho > options.eta_successful) {
      w.Delta =
          w.Delta * std::min(options.radius_increase,
                             std::max(options.radius_reduce,
                                      1 - ((options.radius_increase - 1) *
                                           (pow((1 - 2 * rho), w.tr_p)))));
      w.tr_nu = options.radius_reduce;
    } else if (rho <= options.eta_successful) {
      w.Delta = w.Delta * w.tr_nu;
      w.tr_nu = w.tr_nu * 0.5;
    } else {
      // just incase (NaNs and the like...)
      w.Delta =
          std::max(options.radius_reduce, options.radius_reduce_max) * w.Delta;
      rho = -one; // set to be negative, so that the logic works....
    }
    break;
  default:
    inform.status = NLLS_ERROR::BAD_TR_STRATEGY;
  }
}

void test_convergence(double normF, double normJF, double normF0,
                      double normJF0, const nlls_options &options,
                      nlls_inform &inform) {

  if (normF <=
      std::max(options.stop_g_absolute, options.stop_g_relative * normF0)) {
    inform.convergence_normf = 1;
    return;
  }

  if ((normJF / normF) <=
      std::max(options.stop_g_absolute,
               options.stop_g_relative * (normJF0 / normF0))) {
    inform.convergence_normg = 1;
  }
}

///// Copy a column from a matrix.
//DoubleFortranVector getColumn(const DoubleFortranMatrix &A, int col) {
//  int n = static_cast<int>(A.size1());
//  DoubleFortranVector column(n);
//  for (int i = 1; i <= n; ++i) { // for_do(i,1,n)
//    column(i) = A(i, col);
//  }
//  return column;
//}
//
/////  Takes an m x n matrix J and forms the
/////  m x m matrix A given by
/////  A = J * J'
//void matmult_outer(const DoubleFortranMatrix &J, int n, int m,
//                   DoubleFortranMatrix &A) {
//  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());
//}
//
//DoubleFortranVector matmul(const DoubleFortranMatrix &J,
//                           const DoubleFortranVector &x) {
//  DoubleFortranVector y(int(x.size()));
//  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, y.gsl());
//  return y;
//}
//
//DoubleFortranMatrix matmul(const DoubleFortranMatrix &A,
//                           const DoubleFortranMatrix &B) {
//  int n = static_cast<int>(A.size1());
//  int m = static_cast<int>(B.size2());
//  DoubleFortranMatrix C(n, m);
//  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A.gsl(), B.gsl(), 0.0,
//                 C.gsl());
//  return C;
//}
/////  Takes an n vector x and forms the
/////  n x n matrix xtx given by
/////  xtx = x * x'
//void outer_product(const DoubleFortranVector &x, int n,
//                   DoubleFortranMatrix &xtx) {
//  xtx.allocate(n, n);
//  gsl_blas_dger(1.0, x.gsl(), x.gsl(), xtx.gsl());
//}
//
/////   Calculates norm_A_x = ||x||_A = sqrt(x'*A*x)
//void matrix_norm(const DoubleFortranVector &x, const DoubleFortranMatrix &A,
//                 double &norm_A_x) {
//  norm_A_x = sqrt(dot_product(x, matmul(A, x)));
//}

///// calculate all the eigenvalues of A (symmetric)
//void all_eig_symm(const DoubleFortranMatrix &A, int n, DoubleFortranVector &ew,
//                  DoubleFortranMatrix &ev, all_eig_symm_work &w,
//                  nlls_inform &inform) {
//  auto M = A;
//  M.eigenSystem(ew, ev);
//}
//
/////   Solve the trust-region subproblem using
/////   the DTRS method from Galahad
/////
/////   This method needs H to be diagonal, so we need to
/////   pre-process
/////
/////   main output  d, the soln to the TR subproblem
//void solve_dtrs(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
//                const DoubleFortranMatrix &hf, int n, int m, double Delta,
//                DoubleFortranVector &d, double &normd,
//                const nlls_options &options, nlls_inform &inform,
//                solve_dtrs_work &w) {
//
//  dtrs_control_type dtrs_options;
//  dtrs_inform_type dtrs_inform;
//
//  //  The code finds
//  //   d = arg min_p   w^T p + 0.5 * p^T D p
//  //        s.t. ||p|| \leq Delta
//  //
//  //  where D is diagonal
//  //
//  //  our probem in naturally in the form
//  //
//  //  d = arg min_p   v^T p + 0.5 * p^T H p
//  //        s.t. ||p|| \leq Delta
//  //
//  //  first, find the matrix H and vector v
//  //  Set A = J^T J
//  matmult_inner(J, n, m, w.A);
//  // add any second order information...
//  // so A = J^T J + HF
//  w.A += hf;
//
//  // now form v = J^T f
//  mult_Jt(J, f, w.v);
//
//  // if scaling needed, do it
//  if (options.scale != 0) {
//    apply_scaling(J, n, m, w.A, w.v, w.apply_scaling_ws, options, inform);
//  }
//
//  // Now that we have the unprocessed matrices, we need to get an
//  // eigendecomposition to make A diagonal
//  //
//  all_eig_symm(w.A, n, w.ew, w.ev, w.all_eig_symm_ws, inform);
//  if (inform.status != NLLS_ERROR::OK) {
//    return; // goto 1000
//  }
//
//  // We can now change variables, setting y = Vp, getting
//  // Vd = arg min_(Vx) v^T p + 0.5 * (Vp)^T D (Vp)
//  //       s.t.  ||x|| \leq Delta
//  // <=>
//  // Vd = arg min_(Vx) V^Tv^T (Vp) + 0.5 * (Vp)^T D (Vp)
//  //       s.t.  ||x|| \leq Delta
//  // <=>
//  // we need to get the transformed vector v
//  mult_Jt(w.ev, w.v, w.v_trans);
//
//  // we've now got the vectors we need, pass to dtrs_solve
//  dtrs_initialize(dtrs_options, dtrs_inform);
//
//  if (w.v_trans.len() != n) {
//    w.v_trans.allocate(n);
//  }
//
//  for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1,n)
//    if (abs(w.v_trans(ii)) < epsmch) {
//      w.v_trans(ii) = zero;
//    }
//    if (abs(w.ew(ii)) < epsmch) {
//      w.ew(ii) = zero;
//    }
//  }
//
//  dtrs_solve(n, Delta, zero, w.v_trans, w.ew, w.d_trans, dtrs_options,
//             dtrs_inform);
//  if (dtrs_inform.status != ErrorCode::ral_nlls_ok) {
//    inform.external_return = int(dtrs_inform.status);
//    inform.external_name = "galahad_dtrs";
//    inform.status = NLLS_ERROR::FROM_EXTERNAL;
//    return; // goto 1000
//  }
//
//  // and return the un-transformed vector
//  mult_J(w.ev, w.d_trans, d);
//
//  normd = norm2(d); // ! ||d||_D
//
//  if (options.scale != 0) {
//    for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1, n)
//      d(ii) = d(ii) / w.apply_scaling_ws.diag(ii);
//    }
//  }
//
//} // solve_dtrs
//

//void apply_second_order_info(int n, int m, const DoubleFortranVector &X,
//                             NLLS_workspace &w, eval_hf_type eval_HF,
//                             params_base_type params,
//                             const nlls_options &options, nlls_inform &inform,
//                             const DoubleFortranVector &weights) {
//
//  if (options.exact_second_derivatives) {
//    DoubleFortranVector temp = w.f;
//    temp *= weights;
//    eval_HF(inform.external_return, n, m, X, temp, w.hf, params);
//    inform.h_eval = inform.h_eval + 1;
//  } else {
//    // use the rank-one approximation...
//    rank_one_update(w.hf, w, n);
//  }
//}

//void solve_general(const DoubleFortranMatrix &A, const DoubleFortranVector &b,
//                   DoubleFortranVector &x, int n, nlls_inform &inform,
//                   solve_general_work &w) {
//  // A wrapper for the lapack subroutine dposv.f
//  // NOTE: A would be destroyed
//  w.A = A;
//  w.A.solve(b, x);
//  // x(1:n) = b(1:n)
//  // call dgesv( n, 1, w.A, n, w.ipiv, x, n, inform.external_return)
//}

//void more_sorensen(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
//                   const DoubleFortranMatrix &hf, int n, int m, double Delta,
//                   DoubleFortranVector &d, double &nd,
//                   const nlls_options &options, nlls_inform &inform,
//                   more_sorensen_work &w);
//
//void solve_dtrs(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
//                const DoubleFortranMatrix &hf, int n, int m, double Delta,
//                DoubleFortranVector &d, double &normd,
//                const nlls_options &options, nlls_inform &inform,
//                solve_dtrs_work &w);
//
//// calculate_step, find the next step in the optimization
//void calculate_step(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
//                    const DoubleFortranMatrix &hf, const DoubleFortranVector &g,
//                    int n, int m, double Delta, DoubleFortranVector &d,
//                    double &normd, const nlls_options &options,
//                    nlls_inform &inform, calculate_step_work &w) {
//
//  switch (options.nlls_method) {
//  case 1: //! Powell's dogleg
//    throw std::logic_error("Dog leg method isn't implemented.");
//    break;
//  case 2: //! The AINT method
//    throw std::logic_error("AINT method isn't implemented.");
//    ;
//    break;
//  case 3: //! More-Sorensen
//    more_sorensen(J, f, hf, n, m, Delta, d, normd, options, inform,
//                  w.more_sorensen_ws);
//    break;
//  case 4: //! Galahad
//    solve_dtrs(J, f, hf, n, m, Delta, d, normd, options, inform,
//               w.solve_dtrs_ws);
//    break;
//  default:
//    inform.status = NLLS_ERROR::UNSUPPORTED_METHOD;
//  }
//}

} // NLLS
} // CurveFitting
} // Mantid
