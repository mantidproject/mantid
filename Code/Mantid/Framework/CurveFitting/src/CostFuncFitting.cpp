//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncFitting.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidAPI/IConstraint.h"

#include <gsl/gsl_multifit_nlin.h>

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
    API::IConstraint* c = m_function->getConstraint(i);
    if (c)
    {
      c->setParamToSatisfyConstraint();
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
  * @param covar :: Returned covariance matrix
  * @param epsrel :: Is used to remove linear-dependent columns
  */
void CostFuncFitting::calCovarianceMatrix( Kernel::Matrix<double>& covar, double epsrel )
{
  // construct the jacobian
  GSLJacobian J( m_function, m_values->size() );
  size_t na = this->nParams(); // number of active parameters
  assert( J.getJ()->size2 == na );

  // calculate the derivatives
  m_function->functionDeriv( *m_domain, J );

  // let the GSL to compute the covariance matrix
  gsl_matrix *c = gsl_matrix_alloc( na, na );
  gsl_multifit_covar( J.getJ(), epsrel, c );

  size_t np = m_function->nParams();
  // the calculated matrix is for active parameters
  // put it into a temporary Kernel::Matrix
  Kernel::Matrix<double> activeCovar(np,np);
  activeCovar.zeroMatrix();
  for(size_t i = 0; i < np; ++i)
  {
    if ( !m_function->isActive(i) ) continue;
    for(size_t j = 0; j <= i; ++j)
    {
      if ( !m_function->isActive(j) ) continue;
      double val = gsl_matrix_get( c, i, j );
      activeCovar[i][j] = val;
      if ( i != j )
      {
        activeCovar[j][i] = val;
      }
    }
  }

  if (m_function->isTransformationIdentity())
  {
    // if the transformation is identity simply copy the matrix
    covar = activeCovar;
  }
  else
  {
    // else do the transformation
    m_function->getTransformationMatrix(covar);
    covar *= activeCovar;
  }

  // clean up
  gsl_matrix_free( c );
}

/**
 * Calculate the fitting errors and assign them to the fitting function.
 * @param covar :: A covariance matrix to use for error calculations.
 *   It can be calculated with calCovarianceMatrix().
 */
void CostFuncFitting::calFittingErrors(const Kernel::Matrix<double>& covar)
{
  size_t np = m_function->nParams();
  for(size_t i = 0; i < np; ++i)
  {
    double err = sqrt(covar[i][i]);
    m_function->setError(i,err);
  }
}

} // namespace CurveFitting
} // namespace Mantid
