// This code was originally translated from Fortran code on
// https://ccpforge.cse.rl.ac.uk/gf/project/ral_nlls June 2016
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/RalNlls/TrustRegion.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <string>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <math.h>

namespace Mantid {
namespace CurveFitting {
namespace NLLS {

/// Too small values don't work well with numerical derivatives.
const double epsmch = std::numeric_limits<double>::epsilon();

///  Takes an m x n matrix J and forms the
///  n x n matrix A given by
///  A = J' * J
/// @param J :: The matrix.
/// @param A :: The result.
void matmultInner(const DoubleFortranMatrix &J, DoubleFortranMatrix &A) {
  auto n = J.len2();
  A.allocate(n, n);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());
}

///  Given an (m x n)  matrix J held by columns as a vector,
///  this routine returns the largest and smallest singular values
///  of J.
/// @param J :: The matrix.
/// @param s1 :: The largest sv.
/// @param sn :: The smalles sv.
void getSvdJ(const DoubleFortranMatrix &J, double &s1, double &sn) {

  auto n = J.len2();
  DoubleFortranMatrix U = J;
  DoubleFortranMatrix V(n, n);
  DoubleFortranVector S(n);
  DoubleFortranVector work(n);
  gsl_linalg_SV_decomp(U.gsl(), V.gsl(), S.gsl(), work.gsl());
  s1 = S(1);
  sn = S(n);
}

/// Compute the 2-norm of a vector which is a square root of the
/// sum of squares of its elements.
/// @param v :: The vector.
double norm2(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

/// Multiply a matrix by a vector.
/// @param J :: The matrix.
/// @param x :: The vector.
/// @param Jx :: The result vector.
void multJ(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
           DoubleFortranVector &Jx) {
  // dgemv('N',m,n,alpha,J,m,x,1,beta,Jx,1);
  if (Jx.len() != J.len1()) {
    Jx.allocate(J.len1());
  }
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jx.gsl());
}

/// Multiply a transposed matrix by a vector.
/// @param J :: The matrix.
/// @param x :: The vector.
/// @param Jtx :: The result vector.
void multJt(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
            DoubleFortranVector &Jtx) {
  // dgemv('T',m,n,alpha,J,m,x,1,beta,Jtx,1)
  if (Jtx.len() != J.len2()) {
    Jtx.allocate(J.len2());
  }
  gsl_blas_dgemv(CblasTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jtx.gsl());
}

/// Dot product of two vectors.
double dotProduct(const DoubleFortranVector &x, const DoubleFortranVector &y) {
  return x.dot(y);
}

/// Input:
/// f = f(x_k), J = J(x_k),
/// hf = \\sum_{i=1}^m f_i(x_k) \\nabla^2 f_i(x_k) (or an approx)
///
/// We have a model
///      m_k(d) = 0.5 f^T f  + d^T J f + 0.5 d^T (J^T J + HF) d
///
/// This subroutine evaluates the model at the point d
/// This value is returned as the scalar
///       md :=m_k(d)
/// @param f :: Vector of the residuals (at d = 0).
/// @param J :: The Jacobian matrix at d = 0.
/// @param hf :: The Hessian matrix at d = 0.
/// @param d :: The point where to evaluate the model.
/// @param options :: The options.
/// @param w :: The work struct.
double evaluateModel(const DoubleFortranVector &f, const DoubleFortranMatrix &J,
                     const DoubleFortranMatrix &hf,
                     const DoubleFortranVector &d, const nlls_options &options,
                     evaluate_model_work &w) {

  // Jd = J*d
  multJ(J, d, w.Jd);

  // First, get the base
  // 0.5 (f^T f + f^T J d + d^T' J ^T J d )
  DoubleFortranVector temp = f;
  temp += w.Jd;
  w.md_gn = 0.5 * pow(norm2(temp), 2);
  double md = 0.0;
  switch (options.model) {
  case 1: // first-order (no Hessian)
    md = w.md_gn;
    break;
  default:
    // these have a dynamic H -- recalculate
    // H = J^T J + HF, HF is (an approx?) to the Hessian
    multJ(hf, d, w.Hd);
    md = w.md_gn + 0.5 * dotProduct(d, w.Hd);
  }
  return md;
}

/// Calculate the quantity
///         0.5||f||^2 - 0.5||fnew||^2     actual_reduction
///   rho = -------------------------- = -------------------
///             m_k(0)  - m_k(d)         predicted_reduction
///
/// if model is good, rho should be close to one
/// @param normf :: The 2-norm of the residuals vector at d = 0.
/// @param normfnew :: The 2-norm of the residuals vector at d != 0.
/// @param md :: The value of the model at the same d as normfnew.
/// @param options :: The options.
double calculateRho(double normf, double normfnew, double md,
                    const nlls_options &options) {
  UNUSED_ARG(options);
  auto actual_reduction = (0.5 * pow(normf, 2)) - (0.5 * pow(normfnew, 2));
  auto predicted_reduction = ((0.5 * pow(normf, 2)) - md);
  double rho = 0.0;
  if (fabs(actual_reduction) < 10 * epsmch) {
    rho = one;
  } else if (fabs(predicted_reduction) < 10 * epsmch) {
    rho = one;
  } else {
    rho = actual_reduction / predicted_reduction;
  }
  return rho;
}

/// Update the Hessian matrix without actually evaluating it (quasi-Newton?)
/// @param hf :: The matrix to update.
/// @param w :: The work struct.
void rankOneUpdate(DoubleFortranMatrix &hf, NLLS_workspace &w) {

  auto yts = dotProduct(w.d, w.y);
  if (fabs(yts) < sqrt(10 * epsmch)) {
    //! safeguard: skip this update
    return;
  }

  multJ(hf, w.d, w.Sks); // hfs = S_k * d

  w.ysharpSks = w.y_sharp;
  w.ysharpSks -= w.Sks;

  // now, let's scale hd (Nocedal and Wright, Section 10.2)
  auto dSks = fabs(dotProduct(w.d, w.Sks));
  auto alpha = fabs(dotProduct(w.d, w.y_sharp)) / dSks;
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
  alpha = -dotProduct(w.ysharpSks, w.d) / (pow(yts, 2));
  // call dGER(n,n,alpha,w.y,1,w.y,1,hf,n)
  gsl_blas_dger(alpha, w.y.gsl(), w.y.gsl(), hf.gsl());
}

/// Update the trust region radius which is hidden in NLLS_workspace w
/// (w.Delta).
/// @param rho :: The rho calculated by calculateRho(...). It may also be
/// updated.
/// @param options :: The options.
/// @param inform :: The information.
/// @param w :: The work struct containing the radius that is to be updated
/// (w.Delta).
void updateTrustRegionRadius(double &rho, const nlls_options &options,
                             nlls_inform &inform, NLLS_workspace &w) {

  switch (options.tr_update_strategy) {
  case 1: // default, step-function
    if (!std::isfinite(rho)) {
      w.Delta =
          std::max(options.radius_reduce, options.radius_reduce_max) * w.Delta;
      rho = -one; // set to be negative, so that the logic works....
    } else if (rho < options.eta_success_but_reduce) {
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
    } else {
      // too successful....accept step, but don't change w.Delta
    }
    break;
  case 2: //  Continuous method
          //  Based on that proposed by Hans Bruun Nielsen, TR
          //  IMM-REP-1999-05
          //  http://www2.imm.dtu.dk/documents/ftp/tr99/tr05_99.pdf
    if (!std::isfinite(rho)) {
      w.Delta =
          std::max(options.radius_reduce, options.radius_reduce_max) * w.Delta;
      rho = -one; // set to be negative, so that the logic works....
    } else if (rho >= options.eta_too_successful) {
      // too successful....accept step, but don't change w.Delta
    } else if (rho > options.eta_successful) {
      w.Delta =
          w.Delta * std::min(options.radius_increase,
                             std::max(options.radius_reduce,
                                      1 - ((options.radius_increase - 1) *
                                           (pow((1 - 2 * rho), w.tr_p)))));
      w.tr_nu = options.radius_reduce;
    } else {
      w.Delta = w.Delta * w.tr_nu;
      w.tr_nu = w.tr_nu * 0.5;
    }
    break;
  default:
    inform.status = NLLS_ERROR::BAD_TR_STRATEGY;
  }
}

/// Test the convergence.
void testConvergence(double normF, double normJF, double normF0, double normJF0,
                     const nlls_options &options, nlls_inform &inform) {

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

///  Apply_scaling
///  input  Jacobian matrix, J
///  ouput  scaled Hessisan, H, and J^Tf, v.
///
///  Calculates a diagonal scaling W, stored in w.diag
///  updates v(i) -> (1/W_i) * v(i)
///          A(i,j) -> (1 / (W_i * W_j)) * A(i,j)
/// @param J :: The Jacobian.
/// @param A :: The Hessian.
/// @param v :: The gradient (?).
/// @param w :: Stored scaling data.
/// @param options :: The options.
/// @param inform :: The information.
void applyScaling(const DoubleFortranMatrix &J, DoubleFortranMatrix &A,
                  DoubleFortranVector &v, apply_scaling_work &w,
                  const nlls_options &options, nlls_inform &inform) {
  auto m = J.len1();
  auto n = J.len2();
  if (w.diag.len() != n) {
    w.diag.allocate(n);
  }

  switch (options.scale) {
  case 1:
  case 2:
    for (int ii = 1; ii <= n; ++ii) { // do ii = 1,n
      double temp = zero;
      if (options.scale == 1) {
        //! use the scaling present in gsl:
        //! scale by W, W_ii = ||J(i,:)||_2^2
        for (int jj = 1; jj <= m; ++jj) { // for_do(jj, 1,m)
          // get_element_of_matrix(J,m,jj,ii,Jij);
          temp = temp + pow(J(jj, ii), 2);
        }
      } else if (options.scale == 2) {
        //! scale using the (approximate) hessian
        for (int jj = 1; jj <= n; ++jj) { // for_do(jj, 1,n)
          temp = temp + pow(A(ii, jj), 2);
        }
      }
      if (temp < options.scale_min) {
        if (options.scale_trim_min) {
          temp = options.scale_min;
        } else {
          temp = one;
        }
      } else if (temp > options.scale_max) {
        if (options.scale_trim_max) {
          temp = options.scale_max;
        } else {
          temp = one;
        }
      }
      temp = sqrt(temp);
      if (options.scale_require_increase) {
        w.diag(ii) = std::max(temp, w.diag(ii));
      } else {
        w.diag(ii) = temp;
      }
    }
    break;
  default:
    inform.status = NLLS_ERROR::BAD_SCALING;
    return;
  }

  // Now we have the w.diagonal scaling matrix, actually scale the
  // Hessian approximation and J^Tf
  for (int ii = 1; ii <= n; ++ii) { // for_do(ii, 1,n)
    double temp = w.diag(ii);
    v(ii) = v(ii) / temp;
    for (int jj = 1; jj <= n; ++jj) { // for_do(jj,1,n)
      A(ii, jj) = A(ii, jj) / temp;
      A(jj, ii) = A(jj, ii) / temp;
    }
  }
}

/// Calculate all the eigenvalues of a symmetric matrix.
/// @param A :: The input matrix.
/// @param ew :: The output eigenvalues.
/// @param ev :: The output eigenvectors.
void allEigSymm(const DoubleFortranMatrix &A, DoubleFortranVector &ew,
                DoubleFortranMatrix &ev) {
  auto M = A;
  M.eigenSystem(ew, ev);
}

// This isn't used because we don't calculate second derivatives in Mantid
// If we start using them the method should be un-commented and used here
//
// void apply_second_order_info(int n, int m, const DoubleFortranVector &X,
//                             NLLS_workspace &w, eval_hf_type evalHF,
//                             params_base_type params,
//                             const nlls_options &options, nlls_inform &inform,
//                             const DoubleFortranVector &weights) {
//
//  if (options.exact_second_derivatives) {
//    DoubleFortranVector temp = w.f;
//    temp *= weights;
//    evalHF(inform.external_return, n, m, X, temp, w.hf, params);
//    inform.h_eval = inform.h_eval + 1;
//  } else {
//    // use the rank-one approximation...
//    rankOneUpdate(w.hf, w, n);
//  }
//}

} // NLLS
} // CurveFitting
} // Mantid
