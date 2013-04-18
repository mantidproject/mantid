/*WIKI*
This function calculates a partial Chebyshev expansion

:<math> \sum_{n=0}^N a_n T_n(a+bx) </math>

where <math> a_n </math> are the expansion coefficients and <math> T_n(x) </math> are
Chebyshev polynomials of the first kind defined by the reccurence relation

:<math>T_0(x)=1 \,\!</math>

:<math>T_1(x)=x \,\!</math>

:<math>T_{n+1}(x)= 2xT_n(x)-T_{n-1}(x) \,\!</math>

Coefficients <math> a </math> and <math> b </math> are defined to map the fitting interval
into [-1,1] interval.

Chebyshev function has tree attributes (non-fitting parameters). First is 'n' which has
integer type and sets the expansion order and creates n+1 expansion coefficients (fitting
parameters). The parameter names have the form 'Ai' where 'A' is letter 'A' and 'i' is the
parameter's index starting from 0.

The other two attributes are doubles 'StartX' and 'EndX' which define the expansion (fitting) interval.

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Chebyshev.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/lexical_cast.hpp>

#include <stdexcept>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Chebyshev)

Chebyshev::Chebyshev():m_n(0),m_StartX(-1.),m_EndX(1.) 
{
  declareParameter("A0");
  declareAttribute("n",Attribute( m_n ));
  declareAttribute("StartX",Attribute( m_StartX ));
  declareAttribute("EndX",Attribute( m_EndX ));
};

void Chebyshev::function1D(double* out, const double* xValues, const size_t nData)const
{
  if (m_StartX >= m_EndX)
  {
    throw std::runtime_error("Chebyshev: invalid x-range");
  }
  double b = 2./(m_EndX - m_StartX);
  double a = 1. - b * m_EndX;
  if (static_cast<int>(m_b.size()) != m_n + 3)
  {
    m_b.resize(m_n + 3);
  }
  for(size_t i = 0; i< nData; ++i)
  {
    // scale to interval -1 < x < 1
    double x = a + b * xValues[i];
    m_b[m_n+2] = 0.;
    m_b[m_n+1] = 0.;
    for(int j = m_n; j > 0; --j)
    {
      m_b[j] = m_b[j+1]*x*2 - m_b[j+2] + getParameter(j);
    }
    out[i] = x * m_b[1] - m_b[2] +  getParameter(0);
  }
}

void Chebyshev::functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
{
  if (m_n < 0)
  {
    return;
  }

  double b = 2./(m_EndX - m_StartX);
  double a = 1. - b * m_EndX;

  for (size_t i = 0; i < nData; i++) {
    double t0 = 1.;
    double x = a + b * xValues[i];
    double t1 = x;
    out->set(i,0, 1);
    if (m_n == 0) continue;
    out->set(i,1, x);
    for(int j = 2; j <= m_n; ++j)
    {
      double t = 2*x*t1 - t0;
      out->set(i,j, t);
      t0 = t1;
      t1 = t;
    }
  }
}

/**
 * @param attName :: The attribute name. If it is not "n" exception is thrown.
 * @param att :: An int attribute containing the new value. The value cannot be negative.
 */
void Chebyshev::setAttribute(const std::string& attName,const API::IFunction::Attribute& att)
{
  storeAttributeValue( attName, att );

  if (attName == "n")
  {// set the polynomial order
    if (m_n >= 0)
    {
      clearAllParameters();
    }
    m_n = att.asInt();
    if (m_n < 0)
    {
      throw std::invalid_argument("Chebyshev: polynomial order cannot be negative.");
    }
    for(int i=0;i<=m_n;++i)
    {
      std::string parName = "A" + boost::lexical_cast<std::string>(i);
      declareParameter(parName);
    }
  }
  else if (attName == "StartX")
  {
    m_StartX = att.asDouble();
  }
  else if (attName == "EndX")
  {
    m_EndX = att.asDouble();
  }
}

} // namespace CurveFitting
} // namespace Mantid
