// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMinimizer.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_version.h>

namespace Mantid::CurveFitting::FuncMinimisers {
namespace {
// Get a reference to the logger
Kernel::Logger g_log("LevenbergMarquardtMinimizer");

bool cannotReachSpecifiedToleranceInF(int errorCode) { return errorCode == GSL_ETOLF; }
bool cannotReachSpecifiedToleranceInX(int errorCode) { return errorCode == GSL_ETOLX; }
bool solverIsMakingNoProgress(int errorCode) { return errorCode == GSL_ENOPROG; }
} // namespace

// clang-format off
DECLARE_FUNCMINIMIZER(LevenbergMarquardtMinimizer, Levenberg-Marquardt)
// clang-format on

LevenbergMarquardtMinimizer::LevenbergMarquardtMinimizer()
    : m_data(nullptr), gslContainer(), m_gslSolver(nullptr), m_function(), m_absError(1e-4), m_relError(1e-4) {
  declareProperty("AbsError", m_absError,
                  "Absolute error allowed for "
                  "parameters - a stopping parameter "
                  "in success.");
  declareProperty("RelError", m_relError,
                  "Relative error allowed for "
                  "parameters - a stopping parameter "
                  "in success.");
}

void LevenbergMarquardtMinimizer::initialize(API::ICostFunction_sptr costFunction, size_t /*maxIterations*/) {
  // set-up GSL container to be used with GSL simplex algorithm
  auto leastSquares = std::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(costFunction);
  if (leastSquares) {
    m_data = std::make_unique<GSL_FitData>(leastSquares);
  } else {
    throw std::runtime_error("LevenbergMarquardt can only be used with Least "
                             "squares cost function.");
  }

  // specify the type of GSL solver to use
  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;

  // setup GSL container
  gslContainer.f = &gsl_f;
  gslContainer.df = &gsl_df;
  gslContainer.fdf = &gsl_fdf;

  gslContainer.n = m_data->n;
  gslContainer.p = m_data->p;
  gslContainer.params = m_data.get();

  // setup GSL solver
  m_gslSolver = gsl_multifit_fdfsolver_alloc(T, m_data->n, m_data->p);
  if (!m_gslSolver) {
    throw std::runtime_error("Levenberg-Marquardt minimizer failed to initialize. \n" + std::to_string(m_data->n) +
                             " data points, " + std::to_string(m_data->p) + " fitting parameters. ");
  }
  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, m_data->initFuncParams);

  m_function = leastSquares->getFittingFunction();
}

LevenbergMarquardtMinimizer::~LevenbergMarquardtMinimizer() {
  if (m_gslSolver) {
    gsl_multifit_fdfsolver_free(m_gslSolver);
  }
}

bool LevenbergMarquardtMinimizer::iterate(size_t /*iteration*/) {
  m_absError = getProperty("AbsError");
  m_relError = getProperty("RelError");

  const int retVal = gsl_multifit_fdfsolver_iterate(m_gslSolver);

  if (retVal == GSL_CONTINUE) {
    size_t ia = 0;
    for (size_t i = 0; i < m_function->nParams(); i++) {
      if (m_function->isActive(i)) {
        m_function->setActiveParameter(i, gsl_vector_get(m_gslSolver->x, ia));
        ++ia;
      }
    }
    m_function->applyTies();
    const int gslConvergedStatus = hasConverged();
    return gslConvergedStatus != GSL_SUCCESS;
  }

  if (retVal) {
    m_errorString = gsl_strerror(retVal);
    if (cannotReachSpecifiedToleranceInF(retVal)) {
      m_errorString = "Changes in function value are too small";
    } else if (cannotReachSpecifiedToleranceInX(retVal)) {
      m_errorString = "Changes in parameter value are too small";
    } else if (solverIsMakingNoProgress(retVal)) {
      m_errorString = "Solver is making no progress towards a solution";
    }
    return false;
  }

  const int gslConvergedStatus = hasConverged();
  return gslConvergedStatus != GSL_SUCCESS;
}

int LevenbergMarquardtMinimizer::hasConverged() {
  return gsl_multifit_test_delta(m_gslSolver->dx, m_gslSolver->x, m_absError, m_relError);
}

double LevenbergMarquardtMinimizer::costFunctionVal() {
  double chi = gsl_blas_dnrm2(m_gslSolver->f);
  return chi * chi;
}

/* Calculates covariance matrix
 *
 * @param epsrel :: Is used to remove linear-dependent columns
 * @param covar :: Returned covariance matrix, here as
 */
void LevenbergMarquardtMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix *covar) {
#if GSL_MAJOR_VERSION < 2
  gsl_multifit_covar(m_gslSolver->J, epsrel, covar);
#else
  gsl_matrix *J = gsl_matrix_alloc(gslContainer.n, gslContainer.p);
  gsl_multifit_fdfsolver_jac(m_gslSolver, J);
  gsl_multifit_covar(J, epsrel, covar);
  gsl_matrix_free(J);
#endif
}

} // namespace Mantid::CurveFitting::FuncMinimisers
