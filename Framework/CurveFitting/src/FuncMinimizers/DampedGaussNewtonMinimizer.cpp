// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/DampedGaussNewtonMinimizer.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"

#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <cmath>
#include <gsl/gsl_blas.h>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

namespace {
/// static logger
Kernel::Logger g_log("DampedGaussNewtonMinimizer");
} // namespace

DECLARE_FUNCMINIMIZER(DampedGaussNewtonMinimizer, Damped GaussNewton)

/// Constructor
DampedGaussNewtonMinimizer::DampedGaussNewtonMinimizer(double relTol)
    : IFuncMinimizer(), m_relTol(relTol) {
  declareProperty("Damping", 0.0, "The damping parameter.");
  declareProperty("Verbose", false, "Make output more verbose.");
}

/// Initialize minimizer, i.e. pass a function to minimize.
void DampedGaussNewtonMinimizer::initialize(API::ICostFunction_sptr function,
                                            size_t /*maxIterations*/) {
  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          function);
  if (!m_leastSquares) {
    throw std::invalid_argument(
        "Damped Gauss-Newton minimizer works only with least "
        "squares. Different function was given.");
  }
}

/// Do one iteration.
bool DampedGaussNewtonMinimizer::iterate(size_t /*iteration*/) {
  const bool verbose = getProperty("Verbose");
  const double damping = getProperty("Damping");

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }
  size_t n = m_leastSquares->nParams();

  if (n == 0) {
    m_errorString = "No parameters to fit";
    return false;
  }

  // calculate the first and second derivatives of the cost function.
  m_leastSquares->valDerivHessian();

  // copy the hessian
  GSLMatrix H(m_leastSquares->getHessian());
  GSLVector dd(m_leastSquares->getDeriv());

  for (size_t i = 0; i < n; ++i) {
    double tmp = H.get(i, i) + damping;
    if (tmp == 0.0) {
      m_errorString = "Function doesn't depend on parameter " +
                      m_leastSquares->parameterName(i);
      return false;
    }
    H.set(i, i, tmp);
  }

  if (verbose) {
    g_log.warning() << "H:\n" << H;
    g_log.warning() << "-----------------------------\n";
    for (size_t j = 0; j < n; ++j) {
      g_log.warning() << dd.get(j) << ' ';
    }
    g_log.warning() << '\n';
  }

  /// Parameter corrections
  GSLVector dx(n);
  // To find dx solve the system of linear equations   H * dx == -m_der
  dd *= -1.0;
  try {
    H.solve(dd, dx);
  } catch (std::runtime_error &e) {
    m_errorString = e.what();
    return false;
  }

  if (verbose) {
    for (size_t j = 0; j < n; ++j) {
      g_log.warning() << dx.get(j) << ' ';
    }
    g_log.warning() << "\n\n";
  }

  // Update the parameters of the cost function.
  for (size_t i = 0; i < n; ++i) {
    if (!std::isfinite(dx[i])) {
      m_errorString = "Encountered an infinite number or NaN.";
      return false;
    }
    double d = m_leastSquares->getParameter(i) + dx.get(i);
    m_leastSquares->setParameter(i, d);
    if (verbose) {
      g_log.warning() << i << " Parameter " << m_leastSquares->parameterName(i)
                      << ' ' << d << '\n';
    }
  }
  m_leastSquares->getFittingFunction()->applyTies();

  // --- prepare for the next iteration --- //

  // Try the stop condition
  GSLVector p(n);
  m_leastSquares->getParameters(p);
  double dx_norm = gsl_blas_dnrm2(dx.gsl());
  return dx_norm >= m_relTol;
}

/// Return current value of the cost function
double DampedGaussNewtonMinimizer::costFunctionVal() {
  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }
  return m_leastSquares->val();
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
