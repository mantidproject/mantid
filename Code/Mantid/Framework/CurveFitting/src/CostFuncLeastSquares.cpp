//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/IConstraint.h"

const bool debug = false;

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
  
  retVal *= 0.5;

  for(size_t i=0;i<m_function->nParams();++i)
  {
    if ( !m_function->isActive(i) ) continue;
    API::IConstraint* c = m_function->getConstraint(i);
    if (c)
    {
      retVal += c->check();
    }
  }

  return retVal;
}

/// Calculate the derivatives of the cost function
/// @param der :: Container to output the derivatives
void CostFuncLeastSquares::deriv(std::vector<double>& der) const
{
  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size();       // number of data points
  Jacobian jacobian(ny,np);
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
      d += w * w * ( calc - obs ) * jacobian.get(i,ip);
    }
    API::IConstraint* c = m_function->getConstraint(ip);
    if (c)
    {
      d += c->checkDeriv();
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
      double w = m_values->getFitWeight(i);
      double y = ( calc - obs ) * w;
      d += y * jacobian.get(i,ip);
      if (iActiveP == 0)
      {
        fVal += y * y;
      }
    }
    API::IConstraint* c = m_function->getConstraint(ip);
    if (c)
    {
      d += c->checkDeriv();
    }
    der[iActiveP] = d;
    //std::cerr << "der " << ip << ' ' << der[iActiveP] << std::endl;
    ++iActiveP;
  }
  fVal *= 0.5;
  for(size_t i=0;i<np;++i)
  {
    API::IConstraint* c = m_function->getConstraint(i);
    if (c)
    {
      fVal += c->check();
    }
  }
  return fVal;
}

/** Calculate the value and the first and second derivatives of the cost function
 *  @param der :: Container to output the first derivatives
 *  @param hessian :: Container to output the second derivatives
 *  @param evalFunction :: If false cost function isn't evaluated and returned value (0.0) should be ignored.
 *    It is for efficiency reasons.
 *  @return :: The value of the function if evalFunction is true.
 */
double CostFuncLeastSquares::valDerivHessian(GSLVector& der, GSLMatrix& hessian, bool evalFunction) const
{
  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  if (evalFunction)
  {
    m_function->function(*m_domain,*m_values);
  }
  m_function->functionDeriv(*m_domain,jacobian);

  //---------------------------------------------
  if (debug)
  {
    std::cerr << "Jacobian: " << std::endl;
    for(size_t i = 0; i < ny; ++i)
    {
      for(size_t ip = 0; ip < np; ++ip)
      {
        if ( !m_function->isActive(ip) ) continue;
        std::cerr << jacobian.get(i,ip) << ' ' ;
      }
      std::cerr << std::endl;
    }
    std::cerr << std::endl;
  }
  //---------------------------------------------

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
      double w = m_values->getFitWeight(i);
      double y = ( calc - obs ) * w;
      d += y * jacobian.get(i,ip) * w;
      if (iActiveP == 0 && evalFunction)
      {
        fVal += y * y;
      }
    }
    API::IConstraint* c = m_function->getConstraint(ip);
    if (c)
    {
      d += c->checkDeriv();
    }
    der.set(iActiveP, d);
    //std::cerr << "der " << ip << ' ' << der[iActiveP] << std::endl;
    ++iActiveP;
  }

  if (evalFunction)
  {
    fVal *= 0.5;
    for(size_t i = 0; i < np; ++i)
    {
      API::IConstraint* c = m_function->getConstraint(i);
      if (c)
      {
        fVal += c->check();
      }
    }
  }

  size_t na = der.size(); // number of active parameters
  if (hessian.size1() != na || hessian.size2() != na)
  {
    hessian.resize(na,na);
  }

  size_t i1 = 0; // active parameter index
  for(size_t i = 0; i < np; ++i) // over parameters
  {
    if ( !m_function->isActive(i) ) continue;
    size_t i2 = 0; // active parameter index
    for(size_t j = 0; j <= i; ++j) // over ~ half of parameters
    {
      if ( !m_function->isActive(j) ) continue;
      double d = 0.0;
      for(size_t k = 0; k < ny; ++k) // over fitting data
      {
        double w = m_values->getFitWeight(k);
        d += jacobian.get(k,i) * jacobian.get(k,j) * w * w;
      }
      if (i == j)
      {
        API::IConstraint* c = m_function->getConstraint(i);
        if (c)
        {
          d += c->checkDeriv2();
        }
      }
      hessian.set(i1,i2,d);
      //std::cerr << "hess " << i1 << ' ' << i2 << std::endl;
      if (i1 != i2)
      {
        hessian.set(i2,i1,d);
      }
      ++i2;
    }
    ++i1;
  }

  return fVal;
}

void CostFuncLeastSquares::setParameters(const GSLVector& params)
{
  if (nParams() != params.size())
  {
    throw std::runtime_error("Parameter vector has wrong size in CostFuncLeastSquares.");
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    setParameter(i,params.get(i));
  }
}

void CostFuncLeastSquares::getParameters(GSLVector& params) const
{
  if (params.size() != nParams())
  {
    params.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    params.set(i,getParameter(i));
  }
}


} // namespace CurveFitting
} // namespace Mantid
