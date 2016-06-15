//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/MoreSorensenMinimizer.h"
#include "MantidCurveFitting/RalNlls/TrustRegion.h"

#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

// clang-format off
///@cond nodoc
DECLARE_FUNCMINIMIZER(MoreSorensenMinimizer,More-Sorensen)
///@endcond
// clang-format on

using namespace NLLS;

MoreSorensenMinimizer::MoreSorensenMinimizer()
    : TrustRegionMinimizer(){
}

/// Name of the minimizer.
std::string MoreSorensenMinimizer::name() const {
  return "More-Sorensen";
}

namespace {

void solve_spd(const DoubleFortranMatrix &A, const DoubleFortranVector &b,
               DoubleFortranMatrix &LtL, DoubleFortranVector &x, int n,
               nlls_inform &inform) {
  // A wrapper for the lapack subroutine dposv.f
  // get workspace for the factors....
  // dposv('L', n, 1, LtL, n, x, n, inform.external_return)
  LtL = A;
  auto res = gsl_linalg_cholesky_decomp(LtL.gsl());
  if (res == GSL_EDOM) {
    inform.status = NLLS_ERROR::MS_NOT_PO;
    return;
  }
  gsl_linalg_cholesky_solve(LtL.gsl(), b.gsl(), x.gsl());
}

/// calculate the leftmost eigenvalue of A
void min_eig_symm(const DoubleFortranMatrix &A, double &sigma,
                  DoubleFortranVector &y) {
  auto M = A;
  DoubleFortranVector ew;
  DoubleFortranMatrix ev;
  M.eigenSystem(ew, ev);
  auto ind = ew.sortIndices();
  int imin = static_cast<int>(ind[0]) + 1;
  sigma = ew(imin);
  int n = static_cast<int>(A.size1());
  y.allocate(n);
  for (int i = 1; i <= n; ++i) {
    y(i) = ev(i, imin);
  }
}

/// calculate AplusSigma = A + sigma * I
void shift_matrix(const DoubleFortranMatrix &A, double sigma,
                  DoubleFortranMatrix &AplusSigma, int n) {
  AplusSigma = A;
  for (int i = 1; i <= n; ++i) { // for_do(i,1,n)
    AplusSigma(i, i) = AplusSigma(i, i) + sigma;
  }
}

/// Negate a vector
DoubleFortranVector negative(const DoubleFortranVector &v) {
  DoubleFortranVector neg = v;
  neg *= -1.0;
  return neg;
}

///   Given an indefinite matrix w.A, find a shift sigma
///   such that (A + sigma I) is positive definite
void get_pd_shift(int n, double &sigma, DoubleFortranVector &d,
                  const nlls_options &options, nlls_inform &inform,
                  more_sorensen_work &w) {
  int no_shifts = 0;
  bool successful_shift = false;
  while (!successful_shift) {
    shift_matrix(w.A, sigma, w.AplusSigma, n);
    solve_spd(w.AplusSigma, negative(w.v), w.LtL, d, n, inform);
    if (inform.status != NLLS_ERROR::OK) {
      // reset the error calls -- handled in the code....
      inform.status = NLLS_ERROR::OK;
      inform.external_return = 0;
      inform.external_name = "";
      no_shifts = no_shifts + 1;
      if (no_shifts == 10) { // goto 3000 ! too many shifts -- exit
        inform.status = NLLS_ERROR::MS_TOO_MANY_SHIFTS;
        return;
      }
      sigma = sigma + (pow(10.0, no_shifts)) * options.more_sorensen_shift;
    } else {
      successful_shift = true;
    }
  }
}

///  A subroutine to find the optimal beta such that
///   || d || = Delta, where d = a + beta * b
///
///   uses the approach from equation (3.20b),
///    "Methods for non-linear least squares problems" (2nd edition, 2004)
///    by Madsen, Nielsen and Tingleff
void findbeta(const DoubleFortranVector &a, const DoubleFortranVector &b,
              double Delta, double &beta, nlls_inform &inform) {

  auto c = a.dot(b);

  auto norma2 = pow(norm2(a), 2);
  auto normb2 = pow(norm2(b), 2);

  double discrim = pow(c, 2) + (normb2) * (pow(Delta, 2) - norma2);
  if (discrim < zero) {
    inform.status = NLLS_ERROR::FIND_BETA;
    inform.external_name = "findbeta";
    return;
  }

  if (c <= 0) {
    beta = (-c + sqrt(discrim)) / normb2;
  } else {
    beta = (pow(Delta, 2) - norma2) / (c + sqrt(discrim));
  }
}


/// more_sorensen
/// Solve the trust-region subproblem using
/// the method of More and Sorensen
///
/// Using the implementation as in Algorithm 7.3.6
/// of Trust Region Methods
///
/// main output  d, the soln to the TR subproblem
void more_sorensen(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
                   const DoubleFortranMatrix &hf, int n, int m, double Delta,
                   DoubleFortranVector &d, double &nd,
                   const nlls_options &options, nlls_inform &inform,
                   more_sorensen_work &w) {

  // The code finds
  //  d = arg min_p   v^T p + 0.5 * p^T A p
  //       s.t. ||p|| \leq Delta
  //
  // set A and v for the model being considered here...

  // Set A = J^T J
  matmult_inner(J, n, m, w.A);
  // add any second order information...
  // so A = J^T J + HF
  w.A += hf;
  // now form v = J^T f
  mult_Jt(J, f, w.v);

  // if scaling needed, do it
  if (options.scale != 0) {
    apply_scaling(J, n, m, w.A, w.v, w.apply_scaling_ws, options, inform);
  }

  auto local_ms_shift = options.more_sorensen_shift;

  // d = -A\v
  DoubleFortranVector negv = w.v;
  negv *= -1.0;
  solve_spd(w.A, negv, w.LtL, d, n, inform);
  double sigma = 0.0;
  if (inform.status == NLLS_ERROR::OK) {
    // A is symmetric positive definite....
    sigma = zero;
  } else {
    // reset the error calls -- handled in the code....
    inform.status = NLLS_ERROR::OK;
    inform.external_return = 0;
    inform.external_name = "";
    min_eig_symm(w.A, sigma, w.y1);
    if (inform.status != NLLS_ERROR::OK)
      goto L1000;
    sigma = -(sigma - local_ms_shift);
    // find a shift that makes (A + sigma I) positive definite
    get_pd_shift(n, sigma, d, options, inform, w);
    if (inform.status != NLLS_ERROR::OK)
      goto L4000;
  }

  nd = norm2(d);

  // now, we're not in the trust region initally, so iterate....
  auto sigma_shift = zero;
  int no_restarts = 0;
  // set 'small' in the context of the algorithm
  double epsilon =
      std::max(options.more_sorensen_tol * Delta, options.more_sorensen_tiny);
  for (int i = 1; i <= options.more_sorensen_maxits; ++i) {

    if (nd <= Delta + epsilon) {
      // we're within the tr radius
      if (abs(sigma) < options.more_sorensen_tiny) {
        // we're good....exit
        goto L1020;
      } else if (abs(nd - Delta) < epsilon) {
        // also good...exit
        goto L1020;
      }
      if (w.y1.len() == n) {
        double alpha = 0.0;
        findbeta(d, w.y1, Delta, alpha, inform);
        if (inform.status != NLLS_ERROR::OK)
          goto L1000;
        DoubleFortranVector tmp = w.y1;
        tmp *= alpha;
        d += tmp;
      }
      // also good....exit
      goto L1020;
    }

    // w.q = R'\d
    // DTRSM( "Left", "Lower", "No Transpose", "Non-unit", n, 1, one, w.LtL, n,
    // w.q, n );
    for(int j=1; j <= w.LtL.len1(); ++j) {
      for(int k=j + 1; k <= w.LtL.len1(); ++k) {
        w.LtL(j, k) = 0.0;
      }
    }
    w.LtL.solve(d, w.q);

    auto nq = norm2(w.q);

    sigma_shift = (pow((nd / nq), 2)) * ((nd - Delta) / Delta);
    if (abs(sigma_shift) < options.more_sorensen_tiny * abs(sigma)) {
      if (no_restarts < 1) {
        // find a shift that makes (A + sigma I) positive definite
        get_pd_shift(n, sigma, d, options, inform, w);
        if (inform.status != NLLS_ERROR::OK)
          goto L4000;
        no_restarts = no_restarts + 1;
      } else {
        // we're not going to make progress...jump out
        inform.status = NLLS_ERROR::MS_NO_PROGRESS;
        goto L4000;
      }
    } else {
      sigma = sigma + sigma_shift;
    }

    shift_matrix(w.A, sigma, w.AplusSigma, n);
    DoubleFortranVector negv = w.v;
    negv *= -1.0;
    solve_spd(w.AplusSigma, negv, w.LtL, d, n, inform);
    if (inform.status != NLLS_ERROR::OK)
      goto L1000;

    nd = norm2(d);
  }

  goto L1040;

L1000:
  // bad error return from external package
  goto L4000;

L1020:
  // inital point was successful
  goto L4000;

L1040:
  // maxits reached, not converged
  inform.status = NLLS_ERROR::MS_MAXITS;
  goto L4000;

L4000:
  // exit the routine
  if (options.scale != 0) {
    for (int i = 1; i <= n; ++i) {
      d(i) = d(i) / w.apply_scaling_ws.diag(i);
    }
  }
}

} // namespace

void MoreSorensenMinimizer::calculate_step(const DoubleFortranMatrix &J,
                              const DoubleFortranVector &f,
                              const DoubleFortranMatrix &hf,
                              const DoubleFortranVector &g, int n, int m,
                              double Delta, DoubleFortranVector &d,
                              double &normd, const NLLS::nlls_options &options,
                              NLLS::nlls_inform &inform, NLLS::calculate_step_work &w) {
    more_sorensen(J, f, hf, n, m, Delta, d, normd, options, inform,
                  w.more_sorensen_ws);
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
