#include "MantidCurveFitting/CostFunctions/CostFuncPoisson.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

DECLARE_COSTFUNCTION(CostFuncPoisson, Poisson)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CostFuncPoisson::CostFuncPoisson():CostFuncFitting() {}

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
    const double y = values->getCalculated(i);
    if (y <= 0.0)
    {
      retVal = 1e10;
    }
    else
    {
      const double N = values->getFitData(i);
      if (N == 0.0)
      {
        retVal = y;
      }
      else
      {
        retVal += (y - N) + N*(log(N) - log(y));
      }
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
  UNUSED_ARG(evalDeriv);
  size_t np = function->nParams(); // number of parameters
  size_t ny = domain->size();      // number of data points
  Jacobian jacobian(ny, np);
  function->function(*domain, *values);
  function->functionDeriv(*domain, jacobian);

  size_t iActiveP = 0;
  double fVal = 0.0;
  //std::vector<double> weights = getFitWeights(values);

  for (size_t ip = 0; ip < np; ++ip) {
    if (!function->isActive(ip))
      continue;
    double d = 0.0;
    for (size_t i = 0; i < ny; ++i) {
      double calc = values->getCalculated(i);
      double obs = values->getFitData(i);
      if (calc <= 0.0)
      {
        if (iActiveP == 0) {
          fVal += 1e10;
        }
        d += 1e10;
      }
      else
      {
        if (obs == 0.0)
        {
          if (iActiveP == 0) {
            fVal += calc;
          }
          d += jacobian.get(i, ip);
        }
        else
        {
          if (iActiveP == 0) {
            fVal += (calc - obs) + obs*(log(obs) - log(calc));
          }
          d += jacobian.get(i, ip) * (1.0 - obs / calc);
        }
      }

    }
    PARALLEL_CRITICAL(der_set) {
      double der = m_der.get(iActiveP);
      m_der.set(iActiveP, der + d);
    }
    ++iActiveP;
  }

  PARALLEL_ATOMIC
    m_value += 2.0 * fVal;

  if (!evalHessian)
    return;

  size_t i1 = 0;                  // active parameter index
  for (size_t i = 0; i < np; ++i) // over parameters
  {
    if (!function->isActive(i))
      continue;
    size_t i2 = 0;                  // active parameter index
    double p = function->getParameter(i);
    double dp = 1e-4;
    if (p != 0.0) {
      dp *= p;
    }
    function->setParameter(i, p + dp);
    Jacobian jacobian2(ny, np);
    function->functionDeriv(*domain, jacobian2);
    function->setParameter(i, p);
    for (size_t j = 0; j <= i; ++j) // over ~ half of parameters
    {
      if (!function->isActive(j))
        continue;
      double d = 0.0;
      for (size_t k = 0; k < ny; ++k) // over fitting data
      {
        double d2 = (jacobian2.get(k, j) - jacobian.get(k, j)) / dp;
        
        double calc = values->getCalculated(k);
        double obs = values->getFitData(k);
        if (calc <= 0.0)
        {
          d += 1e10;
        }
        else
        {
          if (obs == 0.0)
          {
            d += d2;
          }
          else
          {
            d += d2 * (1.0 - obs / calc) + jacobian.get(k, i) * jacobian.get(k, j) * obs / calc / calc;
          }
        }
      }
      PARALLEL_CRITICAL(hessian_set) {
        double h = m_hessian.get(i1, i2) + d;
        m_hessian.set(i1, i2, h);
        if (i1 != i2) {
          m_hessian.set(i2, i1, h);
        }
      }
      ++i2;
    }
    ++i1;
  }
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
