// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncPoisson.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include <cmath>
#include <limits>

using namespace Mantid::API;

namespace {
// predicted < 0 is forbidden as it causes inf cost
const double absoluteCutOff = 0.0;
const double effectiveCutOff = 0.0001;

double calculatePoissonLoss(double observedCounts, double predicted) {
  double retVal = (predicted - observedCounts);
  retVal += observedCounts * (log(observedCounts) - log(predicted));
  return retVal;
}

} // namespace

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

DECLARE_COSTFUNCTION(CostFuncPoisson, Poisson)
//----------------------------------------------------------------------------------------------
/** Constructor
 */
CostFuncPoisson::CostFuncPoisson() : CostFuncFitting() {}

/**
 * Add a contribution to the cost function value from the fitting function
 * evaluated on a particular domain.
 * @param domain :: A domain
 * @param values :: Values
 */
void CostFuncPoisson::addVal(API::FunctionDomain_sptr domain,
                             API::FunctionValues_sptr values) const {
  m_function->function(*domain, *values);
  size_t ny = values->size();

  double retVal = 0.0;

  for (size_t i = 0; i < ny; i++) {
    const double predicted = values->getCalculated(i);

    if (predicted <= absoluteCutOff) {
      retVal = std::numeric_limits<double>::infinity();
      break;
    }

    const double observed = values->getFitData(i);
    if (predicted <= effectiveCutOff) {
      retVal += (effectiveCutOff - predicted) / predicted;
    } else if (observed == 0.0) {
      // at observed = 0 the Poisson function reduces to simply adding predicted
      retVal += predicted;
    } else {
      retVal += calculatePoissonLoss(observed, predicted);
    }
  }

  PARALLEL_ATOMIC
  m_value += 2.0 * retVal;
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
void CostFuncPoisson::addValDerivHessian(API::IFunction_sptr function,
                                         API::FunctionDomain_sptr domain,
                                         API::FunctionValues_sptr values,
                                         bool evalDeriv,
                                         bool evalHessian) const {
  const size_t numParams = nParams();

  if (evalDeriv) {
    m_der.resize(numParams);
    m_der.zero();
    calculateDerivative(*function, *domain, *values);
  }

  if (evalHessian) {
    m_hessian.resize(numParams, numParams);
    m_hessian.zero();
    calculateHessian(*function, *domain, *values);
  }
}

void CostFuncPoisson::calculateDerivative(API::IFunction &function,
                                          FunctionDomain &domain,
                                          FunctionValues &values) const {
  const size_t numParams = function.nParams();
  const size_t numDataPoints = domain.size();

  Jacobian jacobian(numDataPoints, numParams);
  function.function(domain, values);
  function.functionDeriv(domain, jacobian);

  size_t activeParamIndex = 0;
  double costVal = 0.0;

  for (size_t paramIndex = 0; paramIndex < numParams; ++paramIndex) {
    if (!function.isActive(paramIndex))
      continue;

    double determinant = 0.0;
    for (size_t i = 0; i < numDataPoints; ++i) {
      double calc = values.getCalculated(i);
      double obs = values.getFitData(i);

      if (calc <= absoluteCutOff) {
        if (activeParamIndex == 0) {
          costVal += std::numeric_limits<double>::infinity();
        }
        determinant += std::numeric_limits<double>::infinity();
        continue;
      }

      if (calc <= effectiveCutOff) {
        if (activeParamIndex == 0) {
          costVal += (effectiveCutOff - calc) / (calc - absoluteCutOff);
        }
        double tmp = calc - absoluteCutOff;
        determinant += jacobian.get(i, paramIndex) *
                       (absoluteCutOff - effectiveCutOff) / (tmp * tmp);

      } else if (obs == 0.0) {
        if (activeParamIndex == 0) {
          costVal += calc;
        }
        determinant += jacobian.get(i, paramIndex);

      } else {
        if (activeParamIndex == 0) {
          costVal += calculatePoissonLoss(obs, calc);
        }
        determinant += jacobian.get(i, paramIndex) * (1.0 - obs / calc);
      }
    }
    PARALLEL_CRITICAL(der_set) {
      double der = m_der.get(activeParamIndex);
      m_der.set(activeParamIndex, der + determinant);
    }
    ++activeParamIndex;
  }

  PARALLEL_ATOMIC
  m_value += 2.0 * costVal;
}

void CostFuncPoisson::calculateHessian(API::IFunction &function,
                                       API::FunctionDomain &domain,
                                       API::FunctionValues &values) const {
  size_t numParams = function.nParams(); // number of parameters
  size_t numDataPoints = domain.size();  // number of data points

  Jacobian jacobian(numDataPoints, numParams);
  function.functionDeriv(domain, jacobian);

  size_t activeParamFirstIndex =
      0; // The params are split into two halves and iterated through
  for (size_t paramIndex = 0; paramIndex < numParams; ++paramIndex) {

    if (!function.isActive(paramIndex))
      continue;
    size_t activeParamSecondIndex = 0; // The counterpart index
    double parameter = function.getParameter(paramIndex);

    double scalingFactor = 1e-4;
    if (parameter != 0.0) {
      scalingFactor *= parameter;
    }

    function.setParameter(paramIndex, parameter + scalingFactor);
    Jacobian jacobian2(numDataPoints, numParams);
    function.functionDeriv(domain, jacobian2);
    function.setParameter(paramIndex, parameter);

    for (size_t j = 0; j <= paramIndex; ++j) // over ~ half of parameters
    {
      if (!function.isActive(j))
        continue;
      double d = 0.0;
      for (size_t k = 0; k < numDataPoints; ++k) // over fitting data
      {
        double d2 = (jacobian2.get(k, j) - jacobian.get(k, j)) / scalingFactor;

        double calc = values.getCalculated(k);
        double obs = values.getFitData(k);
        if (calc <= absoluteCutOff) {
          d += std::numeric_limits<double>::infinity();
        } else {
          if (calc <= effectiveCutOff) {
            double constrainedCalc = calc - absoluteCutOff;
            d += d2 * (absoluteCutOff - effectiveCutOff) /
                 (constrainedCalc * constrainedCalc);
            d += jacobian.get(k, paramIndex) * jacobian.get(k, j) *
                 (effectiveCutOff - absoluteCutOff) * 2 /
                 (constrainedCalc * constrainedCalc * constrainedCalc);
          } else if (obs == 0.0) {
            d += d2;
          } else {
            d += d2 * (1.0 - obs / calc);
            d += jacobian.get(k, paramIndex) * jacobian.get(k, j) * obs /
                 (calc * calc);
          }
        }
      }
      PARALLEL_CRITICAL(hessian_set) {
        double h =
            m_hessian.get(activeParamFirstIndex, activeParamSecondIndex) + d;
        m_hessian.set(activeParamFirstIndex, activeParamSecondIndex, h);
        if (activeParamFirstIndex != activeParamSecondIndex) {
          m_hessian.set(activeParamSecondIndex, activeParamFirstIndex, h);
        }
      }
      ++activeParamSecondIndex;
    }
    ++activeParamFirstIndex;
  }
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
