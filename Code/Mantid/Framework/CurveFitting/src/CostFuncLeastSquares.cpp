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

  return 0.5 * retVal;
}

/// Calculate the derivatives of the cost function
/// @param der :: Container to output the derivatives
void CostFuncLeastSquares::deriv(std::vector<double>& der) const
{
  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size();       // number of data points
  Jacobian jacobian(ny,np);
  // run function to make sure that the calculated values are upto date
  // although if all minimizers always run val() before deriv() it could be omitted
  m_function->function(*m_domain,*m_values);
  m_function->functionDeriv(*m_domain,jacobian);
  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  size_t iActiveP = 0;
  for(size_t ip = 0; ip < np; ++ip)
  {
    //std::cerr << "param " << ip << ' ' << m_function->getParameter(ip) << std::endl;
    if ( !m_function->isActive(ip) ) continue;
    double d = 0.0;
    for(size_t i = 0; i < ny; ++i)
    {
      double calc = m_values->getCalculated(i);
      double obs = m_values->getFitData(i);
      double w = m_values->getFitWeight(i);
      d += w * ( calc - obs ) * jacobian.get(i,ip);
    }
    der[iActiveP] = d;
    ++iActiveP;
  }
}

/// Calculate the value and the derivatives of the cost function
/// @param der :: Container to output the derivatives
/// @return :: The value of the function
double CostFuncLeastSquares::valAndDeriv(std::vector<double>& der) const
{
  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  m_function->function(*m_domain,*m_values);
  m_function->functionDeriv(*m_domain,jacobian);
  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  size_t iActiveP = 0;
  double fVal = 0.0;
  for(size_t ip = 0; ip < np; ++ip)
  {
    if ( !m_function->isActive(ip) ) continue;
    double d = 0.0;
    for(size_t i = 0; i < ny; ++i)
    {
      double calc = m_values->getCalculated(i);
      double obs = m_values->getFitData(i);
      double y = ( calc - obs ) * m_values->getFitWeight(i);
      d += y * jacobian.get(i,ip);
      if (iActiveP == 0)
      {
        fVal += y * y;
      }
    }
    der[iActiveP] = d;
    //std::cerr << "der " << ip << ' ' << der[iActiveP] << std::endl;
    ++iActiveP;
  }
  return fVal;
}

/** Calculate the value and the first and second derivatives of the cost function
 *  @param der :: Container to output the first derivatives
 *  @param hessian :: Container to output the second derivatives
 *  @return :: The value of the function
 */
double CostFuncLeastSquares::valDerivHessian(GSLVector& der, GSLMatrix& hessian) const
{
  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  m_function->function(*m_domain,*m_values);
  m_function->functionDeriv(*m_domain,jacobian);
  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  size_t iActiveP = 0;
  double fVal = 0.0;
  for(size_t ip = 0; ip < np; ++ip)
  {
    if ( !m_function->isActive(ip) ) continue;
    double d = 0.0;
    for(size_t i = 0; i < ny; ++i)
    {
      double calc = m_values->getCalculated(i);
      double obs = m_values->getFitData(i);
      double y = ( calc - obs ) * m_values->getFitWeight(i);
      d += y * jacobian.get(i,ip);
      if (iActiveP == 0)
      {
        fVal += y * y;
      }
    }
    der.set(iActiveP, d);
    //std::cerr << "der " << ip << ' ' << der[iActiveP] << std::endl;
    ++iActiveP;
  }

  size_t na = der.size(); // number of active parameters
  if (hessian.size1() != na || hessian.size2() != na)
  {
    hessian.resize(na,na);
  }

  for(size_t i = 0; i < na; ++i) // over active parameters
  {
    for(size_t j = 0; j <= i; ++j) // over ~ half of active parameters
    {
      double d = 0.0;
      for(size_t k = 0; k < ny; ++k) // over fitting data
      {
        d += jacobian.get(k,i) * jacobian.get(k,j);
      }
      hessian.set(i,j,d);
      if (j != i)
      {
        hessian.set(j,i,d);
      }
    }
  }

  return fVal;
}


} // namespace CurveFitting
} // namespace Mantid
