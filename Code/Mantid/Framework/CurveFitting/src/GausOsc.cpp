//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GausOsc.h"
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(GausOsc)

void GausOsc::init()
{
   declareParameter("A", 10.0); 
   declareParameter("Sigma", 0.2);
   declareParameter("Frequency", 0.1); 
   declareParameter("Phi", 0.0);
}


void GausOsc::functionMW(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("A"); 
  const double& G = getParameter("Sigma");
  const double& gf = getParameter("Frequency"); 
  const double& gphi = getParameter("Phi"); 

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = A*exp(-G*G*x*x)*cos(2*M_PI*gf*x +gphi);
  } 
}

void GausOsc::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  calNumericalDeriv(out, xValues, nData);
}

void GausOsc::setActiveParameter(size_t i,double value)
{
  size_t j = indexOfActive(i);

  if (parameterName(j) == "Sigma") 
    setParameter(j,fabs(value),false);  // Make sigma positive
  else if (parameterName(j) == "Phi")
  {
    double a = fmod(value, 2*M_PI); // Put angle in range of 0 to 360 degrees
    if( a<0 ) a += 2*M_PI; 
    setParameter(j,a,false);
  }
  else
    setParameter(j,value,false);
}


} // namespace CurveFitting
} // namespace Mantid
