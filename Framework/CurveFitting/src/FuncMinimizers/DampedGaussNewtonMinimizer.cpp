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
#include <gsl/gsl_blas.h>
#include <iostream>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

namespace {
/// static logger
Kernel::Logger g_log("DampedGaussNewtonMinimizer");
}

DECLARE_FUNCMINIMIZER(DampedGaussNewtonMinimizer, Damped GaussNewton)

/// Constructor
DampedGaussNewtonMinimizer::DampedGaussNewtonMinimizer(double relTol)
    : IFuncMinimizer(), m_relTol(relTol) {
  declareProperty("Damping", 0.0, "The damping parameter.");
}

/// Initialize minimizer, i.e. pass a function to minimize.
void DampedGaussNewtonMinimizer::initialize(API::ICostFunction_sptr function,
                                            size_t) {
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
bool DampedGaussNewtonMinimizer::iterate(size_t) {
  const bool debug = false;

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
    H.set(i, i, tmp);
  }

  if (debug) {
    std::cerr << "H:\n" << H;
    std::cerr << "-----------------------------\n";
    for (size_t j = 0; j < n; ++j) {
      std::cerr << dd.get(j) << ' ';
    }
    std::cerr << '\n';
  }

  /// Parameter corrections
  GSLVector dx(n);
  // To find dx solve the system of linear equations   H * dx == -m_der
  dd *= -1.0;
  H.solve(dd, dx);

  if (debug) {
    for (size_t j = 0; j < n; ++j) {
      std::cerr << dx.get(j) << ' ';
    }
    std::cerr << "\n\n";
  }

  // Update the parameters of the cost function.
  for (size_t i = 0; i < n; ++i) {
    double d = m_leastSquares->getParameter(i) + dx.get(i);
    m_leastSquares->setParameter(i, d);
    if (debug) {
      std::cerr << "P" << i << ' ' << d << '\n';
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
