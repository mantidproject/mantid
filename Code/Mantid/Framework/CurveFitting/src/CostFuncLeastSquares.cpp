//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/Jacobian.h"

namespace Mantid
{
namespace CurveFitting
{

DECLARE_COSTFUNCTION(CostFuncLeastSquares,Least squares)

/// Calculate value of cost function
/// @return :: The value of the function
double CostFuncLeastSquares::val() const
{
  checkValidity();
  m_function->function(*m_domain,*m_values);
  size_t ny = m_values->size();

  double retVal = 0.0;

  for (size_t i = 0; i < ny; i++)
  {
    double val = ( m_values->getCalculated(i) - m_values->getFitData(i) ) * m_values->getFitWeight(i);
    retVal += val * val;
  }

  return retVal;
}

/// Calculate the derivatives of the cost function
/// @param der :: Container to output the derivatives
void CostFuncLeastSquares::deriv(std::vector<double>& der) const
{
  checkValidity();
  size_t np = nParams();        // number of active parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  // run function to make sure that the calculated values are upto date
  // although if all minimizers always run val() before deriv() it could be omitted
  m_function->function(*m_domain,*m_values);
  m_function->functionDeriv(*m_domain,jacobian);
  if (der.size() != np)
  {
    der.resize(np);
  }
  for(size_t ip = 0; ip < np; ++ip)
  {
    double d = 0.0;
    for(size_t i = 0; i < ny; ++i)
    {
      double calc = m_values->getCalculated(i);
      double obs = m_values->getFitData(i);
      double w = m_values->getFitWeight(i);
      d += w * ( calc - obs ) * jacobian.get(i,ip);
    }
    der[ip] = 2 * d;
  }
}

/// Calculate the value and the derivatives of the cost function
/// @param der :: Container to output the derivatives
/// @return :: The value of the function
double CostFuncLeastSquares::valAndDeriv(std::vector<double>& der) const
{
  checkValidity();
  size_t np = nParams();        // number of active parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  m_function->function(*m_domain,*m_values);
  m_function->functionDeriv(*m_domain,jacobian);
  if (der.size() != np)
  {
    der.resize(np);
  }
  double fVal = 0.0;
  for(size_t ip = 0; ip < np; ++ip)
  {
    double d = 0.0;
    for(size_t i = 0; i < ny; ++i)
    {
      double calc = m_values->getCalculated(i);
      double obs = m_values->getFitData(i);
      double y = ( calc - obs ) * m_values->getFitWeight(i);
      d += y * jacobian.get(i,ip);
      if (ip == 0)
      {
        fVal += y * y;
      }
    }
    der[ip] = 2 * d;
  }
  return fVal;
}

} // namespace CurveFitting
} // namespace Mantid
