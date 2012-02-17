//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncFitting.h"

namespace Mantid
{
namespace CurveFitting
{

/// Get i-th parameter
/// @param i :: Index of a parameter
/// @return :: Value of the parameter
double CostFuncFitting::getParameter(size_t i)const
{
  checkValidity();
  return m_function->activeParameter(m_indexMap[i]);
}

/// Set i-th parameter
/// @param i :: Index of a parameter
/// @param value :: New value of the parameter
void CostFuncFitting::setParameter(size_t i, const double& value)
{
  checkValidity();
  m_function->setActiveParameter(m_indexMap[i],value);
}

/// Number of parameters
size_t CostFuncFitting::nParams()const
{
  checkValidity();
  return m_indexMap.size();
}

/** Set fitting function, domain it will operate on, and container for values.
 * @param function :: The fitting function.
 * @param domain :: The domain for the function.
 * @param values :: The FunctionValues object which receives the calculated values and 
 *  also contains the data to fit to and the fitting weights (reciprocal errors).
 */
void CostFuncFitting::setFittingFunction(API::IFunction_sptr function, 
  API::FunctionDomain_sptr domain, API::FunctionValues_sptr values)
{
  if (domain->size() != values->size())
  {
    throw std::runtime_error("Function domain and values objects are incompatible.");
  }
  m_function = function;
  m_domain = domain;
  m_values = values;
  m_indexMap.clear();
  for(size_t i = 0; i < m_function->nParams(); ++i)
  {
    if (m_function->isActive(i))
    {
      m_indexMap.push_back(i);
    }
  }
}

/**
 * Is the function set and valid?
 */
bool CostFuncFitting::isValid() const
{
  return m_function != API::IFunction_sptr();
}

/**
 * Throw a runtime_error if function is invalid.
 */
void CostFuncFitting::checkValidity() const
{
  if (!isValid())
  {
    throw std::runtime_error("Fitting cost function isn't set");
  }
}

/** Calculates covariance matrix
  *
  * @param covar :: Returned covariance matrix, here as 
  * @param epsrel :: Is used to remove linear-dependent columns
  */
void CostFuncFitting::calCovarianceMatrix(gsl_matrix * covar, double epsrel)
{
}

} // namespace CurveFitting
} // namespace Mantid
