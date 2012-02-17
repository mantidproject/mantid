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
  if (!isValid())
  {
    throw std::runtime_error("Fitting cost function isn't set");
  }
  return m_function->activeParameter(i);
}

/// Set i-th parameter
/// @param i :: Index of a parameter
/// @param value :: New value of the parameter
void CostFuncFitting::setParameter(size_t i, const double& value)
{
  if (!isValid())
  {
    throw std::runtime_error("Fitting cost function isn't set");
  }
  m_function->setActiveParameter(i,value);
}

/// Number of parameters
size_t CostFuncFitting::nParams()const
{
  if (!isValid())
  {
    throw std::runtime_error("Fitting cost function isn't set");
  }
  return m_function->nActive();
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
}

/**
 * Is the function set and valid?
 */
bool CostFuncFitting::isValid() const
{
  return m_function != API::IFunction_sptr();
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
