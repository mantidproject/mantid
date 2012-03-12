//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/IConstraint.h"

namespace
{
  const bool debug = false;
}

namespace Mantid
{
namespace CurveFitting
{

DECLARE_COSTFUNCTION(CostFuncLeastSquares,Least squares)

/// Calculate value of cost function
/// @return :: The value of the function
double CostFuncLeastSquares::val() const
{
  if ( !m_dirtyVal ) return m_value;

  checkValidity();
  m_domain->reset();
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

  m_dirtyVal = false;
  return retVal;
}

/// Calculate the derivatives of the cost function
/// @param der :: Container to output the derivatives
void CostFuncLeastSquares::deriv(std::vector<double>& der) const
{
  valDerivHessian(false,true,false);

  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    der[i] = m_der.get(i);
  }
}

/// Calculate the value and the derivatives of the cost function
/// @param der :: Container to output the derivatives
/// @return :: The value of the function
double CostFuncLeastSquares::valAndDeriv(std::vector<double>& der) const
{
  valDerivHessian(true,true,false);

  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    der[i] = m_der.get(i);
  }
  return m_value;
}

/** Calculate the value and the first and second derivatives of the cost function
 *  @param der :: Container to output the first derivatives
 *  @param hessian :: Container to output the second derivatives
 *  @param evalFunction :: If false cost function isn't evaluated and returned value (0.0) should be ignored.
 *    It is for efficiency reasons.
 *  @return :: The value of the function if evalFunction is true.
 */
double CostFuncLeastSquares::valDerivHessian(bool evalFunction, bool evalDeriv, bool evalHessian) const
{
  if (m_pushed || !evalDeriv)
  {
    return val();
  }

  if (!m_dirtyVal && !m_dirtyDeriv && !m_dirtyHessian) return m_value;
  if (m_dirtyVal) evalFunction = true;

  checkValidity();
  size_t np = m_function->nParams();  // number of parameters 
  size_t ny = m_domain->size(); // number of data points
  Jacobian jacobian(ny,np);
  if (evalFunction)
  {
    m_domain->reset();
    m_function->function(*m_domain,*m_values);
  }
  m_domain->reset();
  m_function->functionDeriv(*m_domain,jacobian);

  if (m_der.size() != nParams())
  {
    m_der.resize(nParams());
  }
  size_t iActiveP = 0;
  double fVal = 0.0;
  if (debug)
  {
    std::cerr << "Jacobian:\n";
    for(size_t i = 0; i < ny; ++i)
    {
      for(size_t ip = 0; ip < np; ++ip)
      {
        if ( !m_function->isActive(ip) ) continue;
        std::cerr << jacobian.get(i,ip) << ' ';
      }
      std::cerr << std::endl;
    }
  }
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
    m_der.set(iActiveP, d);
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
    m_value = fVal;
    m_dirtyVal = false;
  }
  m_dirtyDeriv = false;

  if (!evalHessian) return m_value;

  size_t na = m_der.size(); // number of active parameters
  if (m_hessian.size1() != na || m_hessian.size2() != na)
  {
    m_hessian.resize(na,na);
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
      m_hessian.set(i1,i2,d);
      //std::cerr << "hess " << i1 << ' ' << i2 << std::endl;
      if (i1 != i2)
      {
        m_hessian.set(i2,i1,d);
      }
      ++i2;
    }
    ++i1;
  }

  m_dirtyHessian = false;
  return m_value;
}

const GSLVector& CostFuncLeastSquares::getDeriv() const
{
  if (m_pushed)
  {
    return m_der;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian)
  {
    valDerivHessian();
  }
  return m_der;
}

const GSLMatrix& CostFuncLeastSquares::getHessian() const
{
  if (m_pushed)
  {
    return m_hessian;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian)
  {
    valDerivHessian();
  }
  return m_hessian;
}

void CostFuncLeastSquares::push()
{
  if (m_pushed)
  {
    throw std::runtime_error("Least squares: double push.");
  }
  // make sure we are not dirty
  m_pushedValue = valDerivHessian();
  getParameters(m_pushedParams);
  m_pushed = true;
}

void CostFuncLeastSquares::pop()
{
  if ( !m_pushed )
  {
    throw std::runtime_error("Least squares: empty stack.");
  }
  setParameters(m_pushedParams);
  m_value = m_pushedValue;
  m_pushed = false;
  m_dirtyVal = false;
  m_dirtyDeriv = false;
  m_dirtyHessian = false;
}

void CostFuncLeastSquares::drop()
{
  if ( !m_pushed )
  {
    throw std::runtime_error("Least squares: empty stack.");
  }
  m_pushed = false;
  setDirty();
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
  m_function->applyTies();
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

/**
  * Calculates covariance matrix for fitting function's active parameters. 
  */
void CostFuncLeastSquares::calActiveCovarianceMatrix(GSLMatrix& covar, double epsrel)
{
  if (m_hessian.isEmpty())
  {
    valDerivHessian();
  }
  covar = m_hessian;
  covar.invert();
}

} // namespace CurveFitting
} // namespace Mantid
