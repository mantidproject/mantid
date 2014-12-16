#include "MantidCurveFitting/AugmentedLagrangianOptimizer.h"
#include "MantidKernel/Exception.h"

#include <boost/make_shared.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <gsl/gsl_multimin.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <sstream>

namespace Mantid {
namespace CurveFitting {
using Kernel::DblMatrix;
using std::fabs;
using std::max;
using std::min;

namespace {
// Absolute tolerance on function value
double FTOL_ABS = 1e-10;
// Relative tolerance on function value
double FTOL_REL = 1e-10;
// Absolute tolerance on the X values
double XTOL_ABS = 1e-8;
// Relative toleranceon the X values
double XTOL_REL = 1e-8;
// Tolerance on constraint violation
double CONSTRAINT_TOL = 1e-08;
/// Maximum number of iterations of unconstrained sub optimizer
int MAX_SUBOPT_ITER = 100;

/// Holder for data to pass to gsl functions
struct FunctionData {
  size_t n; // number of parameters
  const AugmentedLagrangianOptimizer::ObjFunction *
      userfunc;                      // user supplied function
  const DblMatrix *eqmatrix;         // equality constraints
  const std::vector<double> *lambda; // lagrange multiplier for equality
  const DblMatrix *ineqmatrix;       // inequality constraints
  const std::vector<double> *mu;     // lagrange multiplier for inequality
  double rho;                        // scaling parameter
  gsl_vector *tmp; // gsl vector of size n (used for numerical derivative calc
                   // to avoid constant reallocation)
};

/**
 * Evaluate a constaint given by a constraint matrix
 * @param cmatrix A matrix of constraint coefficients
 * @param index Index of the row defining the constraint
 * @param n The number of parameters
 * @param x The current parameter set
 * @return A value for the constraint
 */
double evaluateConstraint(const DblMatrix &cmatrix, const size_t index,
                          const size_t, const double *x) {
  assert(index < cmatrix.numRows());
  const double *row = cmatrix[index];

  double res(0.0);
  for (size_t j = 0; j < cmatrix.numCols(); ++j) {
    res += row[j] * x[j];
  }
  return res;
}

/**
 * Evaulate stopping criteria
 * @param vold Old value
 * @param vnew New value
 * @param reltol Relative tolerance
 * @param abstol Absolute tolerance
 * @return 1 if criteria satisfied, 0 otherwise
 */
int relstop(double vold, double vnew, double reltol, double abstol) {
  if (vold != vold)
    return 0; // nan
  return (fabs(vnew - vold) < abstol ||
          fabs(vnew - vold) < reltol * (fabs(vnew) + fabs(vold)) * 0.5 ||
          (reltol > 0 && vnew == vold)); /* catch vnew == vold == 0 */
}

/**
 * Evaluate stopping criteria for X values in std::vector
 * @param xvold Old X values
 * @param xvnew New X values
 * @param reltol Relative tolerance
 * @param abstol Absolute tolerance
 * @return 1 if criteria satisfied
 */
int relstopX(const std::vector<double> &xvOld, const std::vector<double> &xvNew,
             double reltol, double abstol) {
  for (size_t i = 0; i < xvOld.size(); ++i) {
    if (!relstop(xvOld[i], xvNew[i], reltol, abstol))
      return 0;
  }
  return 1;
}
/**
 * Evaluate stopping criteria for X values in gsl vector
 * @param xvold Old X values
 * @param xvnew New X values
 * @param reltol Relative tolerance
 * @param abstol Absolute tolerance
 * @return 1 if criteria satisfied
 */
int relstopX(const std::vector<double> &xvOld, const gsl_vector *xvNew,
             double reltol, double abstol) {
  for (size_t i = 0; i < xvOld.size(); ++i) {
    if (boost::math::isnan(gsl_vector_get(xvNew, i)))
      return 1;
    if (!relstop(xvOld[i], gsl_vector_get(xvNew, i), reltol, abstol))
      return 0;
  }
  return 1;
}
}

//---------------------------------------------------------------------------------------------
// AugmentedLagrangianOptimizer
//---------------------------------------------------------------------------------------------

/**
 * Perform the minimization using the Augmented Lagrangian routine
 * @param xv The starting parameter values. They will get updated with the
 * values as the routine progresses
 */
void AugmentedLagrangianOptimizer::minimize(std::vector<double> &xv) const {
  assert(numParameters() == xv.size());

  OptimizerResult ret = Success;
  double ICM(HUGE_VAL), minf_penalty(HUGE_VAL), rho(0.0);
  double fcur(0.0), minf(HUGE_VAL), penalty(0.0);
  std::vector<double> xcur(xv), lambda(numEqualityConstraints(), 0),
      mu(numInequalityConstraints());
  int minfIsFeasible = 0;
  int auglagIters = 0;

  /* magic parameters from Birgin & Martinez */
  const double tau = 0.5, gam = 10;
  const double lam_min = -1e20, lam_max = 1e20, mu_max = 1e20;

  if (numEqualityConstraints() > 0 || numInequalityConstraints() > 0) {
    double con2 = 0;
    fcur = m_userfunc(numParameters(), xcur.data());
    int feasible = 1;
    for (size_t i = 0; i < numEqualityConstraints(); ++i) {
      double hi = evaluateConstraint(m_eq, i, numParameters(), xcur.data());
      penalty += fabs(hi);
      feasible = (feasible && fabs(hi) <= CONSTRAINT_TOL);
      con2 += hi * hi;
    }
    for (size_t i = 0; i < numInequalityConstraints(); ++i) {
      double fci = evaluateConstraint(m_ineq, i, numParameters(), xcur.data());
      penalty += fci > 0 ? fci : 0;
      feasible = feasible && fci <= CONSTRAINT_TOL;
      if (fci > 0)
        con2 += fci * fci;
    }
    minf = fcur;
    minf_penalty = penalty;
    minfIsFeasible = feasible;
    rho = max(1e-6, min(10.0, 2.0 * fabs(minf) / con2));
  } else {
    rho = 1; /* doesn't matter */
  }

  do {
    double prevICM = ICM;

    unconstrainedOptimization(lambda, mu, rho, xcur);

    fcur = m_userfunc(numParameters(), xcur.data());
    ICM = 0.0;
    penalty = 0.0;
    int feasible = 1;
    for (size_t i = 0; i < numEqualityConstraints(); ++i) {
      double hi = evaluateConstraint(m_eq, i, numParameters(), xcur.data());
      double newlam = lambda[i] + rho * hi;
      penalty += fabs(hi);
      feasible = feasible && (fabs(hi) <= CONSTRAINT_TOL);
      ICM = max(ICM, fabs(hi));
      lambda[i] = min(max(lam_min, newlam), lam_max);
    }
    for (size_t i = 0; i < numInequalityConstraints(); ++i) {
      double fci = evaluateConstraint(m_ineq, i, numParameters(), xcur.data());
      double newmu = mu[i] + rho * fci;
      penalty += fci > 0 ? fci : 0;
      feasible = feasible && fci <= CONSTRAINT_TOL;
      ICM = max(ICM, fabs(max(fci, -mu[i] / rho)));
      mu[i] = min(max(0.0, newmu), mu_max);
    }

    if (ICM > tau * prevICM) {
      rho *= gam;
    }
    ++auglagIters;

    if ((feasible &&
         (!minfIsFeasible || penalty <= minf_penalty || fcur < minf)) ||
        (!minfIsFeasible && penalty <= minf_penalty)) {
      ret = Success;
      if (feasible) {
        if (relstop(minf, fcur, FTOL_REL, FTOL_ABS))
          ret = FTolReached;
        else if (relstopX(xv, xcur, XTOL_REL, XTOL_ABS))
          ret = XTolReached;
      }
      minf = fcur;
      minf_penalty = penalty;
      minfIsFeasible = feasible;
      std::copy(xcur.begin(), xcur.end(), xv.begin());
      if (ret != Success)
        break;
    }
    if (ICM == 0.0) {
      ret = FTolReached;
      break;
    }
  } while (auglagIters < m_maxIter);
}

//--------------------------------------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------------------------------------

namespace {
/**
 * GSL-style function for evaluating the cost function
 * @param v GSL vector of current parameter values
 * @param params Associated function data
 * @return Value at the current parameter point
 */
double costf(const gsl_vector *v, void *params) {
  FunctionData *d = static_cast<FunctionData *>(params);

  double lagrangian = (*d->userfunc)(d->n, v->data);
  for (size_t i = 0; i < d->eqmatrix->numRows(); ++i) {
    double h = evaluateConstraint(*d->eqmatrix, i, d->n, v->data) +
               ((*d->lambda)[i] / d->rho);
    lagrangian += 0.5 * d->rho * h * h;
  }
  for (size_t i = 0; i < d->ineqmatrix->numRows(); ++i) {
    double fc = evaluateConstraint(*d->ineqmatrix, i, d->n, v->data) +
                ((*d->mu)[i] / d->rho);
    if (fc > 0.0)
      lagrangian += 0.5 * d->rho * fc * fc;
  }
  return lagrangian;
}

/**
 * The gradient of f, df = (df/dx, df/dy)
 * @param v Current parameter set
 * @param params Function data
 * @param df Holder for output  derivatives
 */
void costdf(const gsl_vector *v, void *params, gsl_vector *df) {
  FunctionData *d = static_cast<FunctionData *>(params);
  double f0 = costf(v, params);
  gsl_vector *tmp = d->tmp;
  std::copy(v->data, v->data + d->n, tmp->data);

  const double epsilon(1e-08);
  for (size_t i = 0; i < d->n; ++i) {
    const double curx = gsl_vector_get(tmp, i);
    gsl_vector_set(tmp, i, curx + epsilon);
    gsl_vector_set(df, i, (costf(tmp, params) - f0) / epsilon);
    gsl_vector_set(tmp, i, curx);
  }
}

/**
 * The gradient of f and f computed together
 * @param v Current parameter set
 * @param params Function data
 * @param f Outputs the cost function value here
 * @param df Holder for output  derivatives
 */
void costfdf(const gsl_vector *x, void *params, double *f, gsl_vector *df) {
  *f = costf(x, params);
  costdf(x, params, df);
}
}

/**
 * @param lambda
 * @param mu
 * @param rho
 * @param xcur The starting parameters for the limited unconstrained
 * optimization. They will
 *             be updated as it proceeds
 */
void AugmentedLagrangianOptimizer::unconstrainedOptimization(
    const std::vector<double> &lambda, const std::vector<double> &mu,
    const double rho, std::vector<double> &xcur) const {
  // Data required to calculate function
  FunctionData d;
  d.n = numParameters();
  d.userfunc = &m_userfunc;
  d.eqmatrix = &m_eq;
  d.ineqmatrix = &m_ineq;
  d.lambda = &lambda;
  d.mu = &mu;
  d.rho = rho;

  gsl_vector *x = gsl_vector_alloc(d.n);
  std::copy(xcur.begin(), xcur.end(), x->data);
  gsl_vector *tmp =
      gsl_vector_alloc(d.n); // Used for numerical derivative calculation
  d.tmp = tmp;

  // Unconstrained const function
  gsl_multimin_function_fdf costFunc;
  costFunc.n = d.n;
  costFunc.f = costf;
  costFunc.df = costdf;
  costFunc.fdf = costfdf;
  costFunc.params = (void *)&d;

  // Declare minimizer
  const gsl_multimin_fdfminimizer_type *T =
      gsl_multimin_fdfminimizer_conjugate_pr;
  gsl_multimin_fdfminimizer *s = gsl_multimin_fdfminimizer_alloc(T, costFunc.n);
  double tol =
      (xcur[0] > 1e-3
           ? 1e-4
           : 1e-3); // Adjust the tolerance for the scale of the first param
  gsl_multimin_fdfminimizer_set(s, &costFunc, x, 0.01, tol);

  int iter = 0;
  int status = 0;

  do {
    iter++;
    status = gsl_multimin_fdfminimizer_iterate(s);
    if (status)
      break;
    status = gsl_multimin_test_gradient(s->gradient, 1e-3);

    if (relstopX(xcur, s->x, XTOL_REL, XTOL_ABS))
      break; // If the X's don't change then assume we're done
    std::copy(s->x->data, s->x->data + d.n, xcur.begin());
  } while (status == GSL_CONTINUE && iter < MAX_SUBOPT_ITER);
  // Final parameter update
  std::copy(s->x->data, s->x->data + d.n, xcur.begin());

  gsl_multimin_fdfminimizer_free(s);
  gsl_vector_free(x);
  gsl_vector_free(tmp);
}

/**
 * @param equality A matrix of equality constraints \f$A_{eq}\f$(the number of
 * columns must match number of parameters)
 *                 where \f$A_{eq} x = 0\f$
 * @param inequality A matrix of inequality constraints (the number of columns
 * must match number of parameters
 *                 where \f$A_{eq} x \geq 0\f$
 */
void
AugmentedLagrangianOptimizer::checkConstraints(const DblMatrix &equality,
                                               const DblMatrix &inequality) {
  const size_t totalNumConstr =
      numEqualityConstraints() + numInequalityConstraints();
  if (totalNumConstr == 0)
    return;

  // Sanity checks on matrix sizes
  for (size_t i = 0; i < 2; ++i) {
    size_t ncols(0);
    std::string matrix("");
    if (i == 0) {
      ncols = equality.numCols();
      matrix = "equality";
    } else {
      ncols = inequality.numCols();
      matrix = "inequality";
    }

    if (ncols > 0 && ncols != numParameters()) {
      std::ostringstream os;
      os << "AugmentedLagrangianOptimizer::initializeConstraints - Invalid "
         << matrix << " constraint matrix. Number of columns must match number "
                      "of parameters. ncols=" << ncols
         << ", nparams=" << numParameters();
      throw std::invalid_argument(os.str());
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
