// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include <sstream>

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
namespace {
/// static logger
Kernel::Logger g_log("CostFuncLeastSquares");
} // namespace

DECLARE_COSTFUNCTION(CostFuncLeastSquares, Least squares)

/**
 * Constructor
 */
CostFuncLeastSquares::CostFuncLeastSquares()
    : CostFuncFitting(), m_factor(0.5) {}
/**
 * Add a contribution to the cost function value from the fitting function
 * evaluated on a particular domain.
 * @param domain :: A domain
 * @param values :: Values
 */
void CostFuncLeastSquares::addVal(API::FunctionDomain_sptr domain,
                                  API::FunctionValues_sptr values) const {
  m_function->function(*domain, *values);
  size_t ny = values->size();

  double retVal = 0.0;

  std::vector<double> weights = getFitWeights(values);

  for (size_t i = 0; i < ny; i++) {
    double val =
        (values->getCalculated(i) - values->getFitData(i)) * weights[i];
    retVal += val * val;
  }

  PARALLEL_ATOMIC
  m_value += m_factor * retVal;
}

/**
 * Update the cost function, derivatives and hessian by adding values calculated
 * on a domain.
 * @param function :: Function to use to calculate the value and the derivatives
 * @param domain :: The domain.
 * @param values :: The fit function values
 * @param evalDeriv :: Flag to evaluate the derivatives
 * @param evalHessian :: Flag to evaluate the Hessian
 */
void CostFuncLeastSquares::addValDerivHessian(API::IFunction_sptr function,
                                              API::FunctionDomain_sptr domain,
                                              API::FunctionValues_sptr values,
                                              bool evalDeriv,
                                              bool evalHessian) const {
  UNUSED_ARG(evalDeriv);
  function->function(*domain, *values);
  size_t np = function->nParams(); // number of parameters
  size_t ny = values->size();      // number of data points
  Jacobian jacobian(ny, np);
  function->functionDeriv(*domain, jacobian);

  size_t iActiveP = 0;
  double fVal = 0.0;
  std::vector<double> weights = getFitWeights(values);

  for (size_t ip = 0; ip < np; ++ip) {
    if (!function->isActive(ip))
      continue;
    double d = 0.0;
    for (size_t i = 0; i < ny; ++i) {
      double calc = values->getCalculated(i);
      double obs = values->getFitData(i);
      double w = weights[i];
      double y = (calc - obs) * w;
      d += y * jacobian.get(i, ip) * w;
      if (iActiveP == 0) {
        fVal += y * y;
      }
    }
    PARALLEL_CRITICAL(der_set) {
      double der = m_der.get(iActiveP);
      m_der.set(iActiveP, der + d);
    }
    ++iActiveP;
  }

  PARALLEL_ATOMIC
  m_value += 0.5 * fVal;

  if (!evalHessian)
    return;

  size_t i1 = 0;                  // active parameter index
  for (size_t i = 0; i < np; ++i) // over parameters
  {
    if (!function->isActive(i))
      continue;
    size_t i2 = 0;                  // active parameter index
    for (size_t j = 0; j <= i; ++j) // over ~ half of parameters
    {
      if (!function->isActive(j))
        continue;
      double d = 0.0;
      for (size_t k = 0; k < ny; ++k) // over fitting data
      {
        double w = weights[k];
        d += jacobian.get(k, i) * jacobian.get(k, j) * w * w;
      }
      PARALLEL_CRITICAL(hessian_set) {
        double h = m_hessian.get(i1, i2);
        m_hessian.set(i1, i2, h + d);
        if (i1 != i2) {
          m_hessian.set(i2, i1, h + d);
        }
      }
      ++i2;
    }
    ++i1;
  }
}

std::vector<double>
CostFuncLeastSquares::getFitWeights(API::FunctionValues_sptr values) const {
  std::vector<double> weights(values->size());
  for (size_t i = 0; i < weights.size(); ++i) {
    weights[i] = values->getFitWeight(i);
  }

  return weights;
}

/**
 * Calculates covariance matrix for fitting function's active parameters.
 * @param covar :: Output cavariance matrix.
 * @param epsrel :: Tolerance.
 */
void CostFuncLeastSquares::calActiveCovarianceMatrix(GSLMatrix &covar,
                                                     double epsrel) {
  UNUSED_ARG(epsrel);

  if (m_hessian.isEmpty()) {
    valDerivHessian();
  }

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    std::ostringstream osHess;
    osHess << "== Hessian (H) ==\n";
    osHess << std::left << std::fixed;
    for (size_t i = 0; i < m_hessian.size1(); ++i) {
      for (size_t j = 0; j < m_hessian.size2(); ++j) {
        osHess << std::setw(10);
        osHess << m_hessian.get(i, j) << "  ";
      }
      osHess << "\n";
    }
    g_log.debug() << osHess.str();
  }

  covar = m_hessian;
  covar.invert();
  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    std::ostringstream osCovar;
    osCovar << "== Covariance matrix (H^-1) ==\n";
    osCovar << std::left << std::fixed;
    for (size_t i = 0; i < covar.size1(); ++i) {
      for (size_t j = 0; j < covar.size2(); ++j) {
        osCovar << std::setw(10);
        osCovar << covar.get(i, j) << "  ";
      }
      osCovar << "\n";
    }
    g_log.debug() << osCovar.str();
  }
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
